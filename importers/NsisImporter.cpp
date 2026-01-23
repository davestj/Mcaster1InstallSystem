/*
 * NsisImporter.cpp — Full Phase 2 NSIS script parser
 */
#include "NsisImporter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

// Converts NSIS path variables to .mis tokens and normalises separators
static QString resolveNsisPath(const QString &p)
{
    QString r = p;
    // Common NSIS install-dir variables → our {install-dir} token
    r.replace("$INSTDIR",        "{install-dir}", Qt::CaseInsensitive);
    r.replace("$PROGRAMFILES64", "{install-dir}", Qt::CaseInsensitive);
    r.replace("$PROGRAMFILES",   "{install-dir}", Qt::CaseInsensitive);
    r.replace("$COMMONFILES64",  "{install-dir}", Qt::CaseInsensitive);
    r.replace("$COMMONFILES",    "{install-dir}", Qt::CaseInsensitive);
    r.replace('\\', '/');
    return r;
}

bool NsisImporter::parse(const QString &nsiPath)
{
    m_warnings.clear();
    m_errors.clear();
    m_manifest = Manifest::newProject();

    QFile f(nsiPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_errors << QString("Cannot open file: %1").arg(nsiPath);
        return false;
    }

    QStringList lines;
    {
        QTextStream in(&f);
        while (!in.atEnd())
            lines << in.readLine();
    }

    // ── Static regex patterns ─────────────────────────────────────────────────
    // Raw-string delimiter "re" prevents )" in patterns from terminating the literal
    using RE = QRegularExpression;
    static const RE reName(
        R"re(^\s*Name\s+"([^"]+)")re",                            RE::CaseInsensitiveOption);
    static const RE reVer(
        R"re(VIProductVersion\s+"([^"]+)")re",                    RE::CaseInsensitiveOption);
    static const RE rePub(
        R"re(VIAddVersionKey\s+"CompanyName"\s+"([^"]+)")re",     RE::CaseInsensitiveOption);
    static const RE reUrl(
        R"re(VIAddVersionKey\s+"(?:FileDescription|LegalCopyright)"\s+"([^"]+)")re",
        RE::CaseInsensitiveOption);
    static const RE reDir(
        R"re(InstallDir\s+"([^"]+)")re",                          RE::CaseInsensitiveOption);

    // Section with quoted name (optionally /o for unchecked-by-default)
    static const RE reSection(
        R"re(^\s*Section\s+(?:/o\s+)?"([^"]+)")re",              RE::CaseInsensitiveOption);
    // Hidden/system section:  Section -Post   or  Section -Prerequisites
    static const RE reSectionHidden(
        R"re(^\s*Section\s+(-\w+))re",                            RE::CaseInsensitiveOption);
    static const RE reSectionEnd(
        R"re(^\s*SectionEnd\b)re",                                RE::CaseInsensitiveOption);
    static const RE reSectionIn(
        R"re(^\s*SectionIn\s+RO\b)re",                           RE::CaseInsensitiveOption);
    static const RE reOptional(
        R"re(\s+/o\s+)re",                                        RE::CaseInsensitiveOption);

    static const RE reSetOutPath(
        R"re(^\s*SetOutPath\s+"([^"]+)")re",                      RE::CaseInsensitiveOption);
    static const RE reFile(
        R"re(^\s*File\s+((?:/r\s+)?)?"([^"]+)")re",              RE::CaseInsensitiveOption);

    static const RE reCreateShortCut(
        R"re(^\s*CreateShortCut\s+"([^"]+)"\s+"([^"]+)")re",     RE::CaseInsensitiveOption);

    static const RE reWriteRegStr(
        R"re(^\s*WriteRegStr\s+(\w+)\s+"([^"]*)"\s+"([^"]*)"\s+"([^"]*)")re",
        RE::CaseInsensitiveOption);
    static const RE reWriteRegDWORD(
        R"re(^\s*WriteRegDWORD\s+(\w+)\s+"([^"]*)"\s+"([^"]*)"\s+(\S+))re",
        RE::CaseInsensitiveOption);

    // ── State ────────────────────────────────────────────────────────────────
    m_manifest.components.clear();
    bool      inSection  = false;
    bool      isSystem   = false;   // hidden / Uninstall — parse SC+reg, skip component
    Component currentComp;
    QString   currentOutPath = "$INSTDIR";

    // ── Single-pass parse ────────────────────────────────────────────────────
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();

        // Skip blank lines, comments (;), and preprocessor (! or #)
        if (line.isEmpty() || line.startsWith(';') ||
            line.startsWith('!') || line.startsWith('#'))
            continue;

        // ── App metadata ─────────────────────────────────────────────────────
        auto mN = reName.match(line);
        if (mN.hasMatch()) { m_manifest.app.name = mN.captured(1); continue; }

        auto mV = reVer.match(line);
        if (mV.hasMatch()) { m_manifest.app.version = mV.captured(1); continue; }

        auto mP = rePub.match(line);
        if (mP.hasMatch()) { m_manifest.app.publisher = mP.captured(1); continue; }

        auto mD = reDir.match(line);
        if (mD.hasMatch()) {
            m_manifest.defaults.installDirWindows = resolveNsisPath(mD.captured(1));
            continue;
        }

        // ── Section start (quoted) ────────────────────────────────────────────
        auto mSec = reSection.match(line);
        if (mSec.hasMatch()) {
            // Flush previous component
            if (inSection && !isSystem && !currentComp.name.isEmpty())
                m_manifest.components.append(currentComp);

            currentComp = Component();
            currentComp.name     = mSec.captured(1);
            currentComp.id       = QString("sec_%1").arg(m_manifest.components.size());
            currentComp.selected = !reOptional.match(line).hasMatch();

            const QString lcName = currentComp.name.toLower();
            isSystem = (lcName == "uninstall" || lcName.startsWith("un."));
            currentOutPath = "$INSTDIR";
            inSection = true;
            continue;
        }

        // ── Section start (hidden, e.g. Section -Post) ────────────────────────
        auto mSecH = reSectionHidden.match(line);
        if (mSecH.hasMatch()) {
            if (inSection && !isSystem && !currentComp.name.isEmpty())
                m_manifest.components.append(currentComp);
            currentComp    = Component();
            currentComp.name = mSecH.captured(1);
            isSystem       = true;    // hidden sections never become user components
            currentOutPath = "$INSTDIR";
            inSection      = true;
            continue;
        }

        // ── Section end ───────────────────────────────────────────────────────
        if (reSectionEnd.match(line).hasMatch()) {
            if (inSection && !isSystem && !currentComp.name.isEmpty())
                m_manifest.components.append(currentComp);
            inSection  = false;
            isSystem   = false;
            currentComp = Component();
            continue;
        }

        // ── SectionIn RO → required ───────────────────────────────────────────
        if (reSectionIn.match(line).hasMatch() && inSection && !isSystem) {
            currentComp.required = true;
            continue;
        }

        // ── SetOutPath ────────────────────────────────────────────────────────
        auto mOut = reSetOutPath.match(line);
        if (mOut.hasMatch()) {
            currentOutPath = mOut.captured(1);
            continue;
        }

        // ── File ─────────────────────────────────────────────────────────────
        auto mFile = reFile.match(line);
        if (mFile.hasMatch() && inSection && !isSystem) {
            FileEntry fe;
            const bool recursive = line.contains("/r ", Qt::CaseInsensitive);
            // Group 1 = optional /r flag, group 2 = path
            fe.src   = mFile.captured(2).replace('\\', '/');
            fe.isDir = recursive;
            const QString dstDir = resolveNsisPath(currentOutPath);
            fe.dst = dstDir + "/" + (recursive ? QFileInfo(fe.src).fileName()
                                                : QFileInfo(fe.src).fileName());
            currentComp.files.append(fe);
            continue;
        }

        // ── CreateShortCut ────────────────────────────────────────────────────
        auto mSC = reCreateShortCut.match(line);
        if (mSC.hasMatch()) {
            Shortcut sc;
            sc.name   = QFileInfo(mSC.captured(1).replace('\\', '/')).baseName();
            sc.target = resolveNsisPath(mSC.captured(2));
            sc.type   = "app";
            // Avoid duplicate shortcuts (desktop + start menu often point to same target)
            bool dup = false;
            for (const auto &existing : m_manifest.shortcuts)
                if (existing.target == sc.target) { dup = true; break; }
            if (!dup && !sc.target.isEmpty())
                m_manifest.shortcuts.append(sc);
            continue;
        }

        // ── WriteRegStr ───────────────────────────────────────────────────────
        auto mRS = reWriteRegStr.match(line);
        if (mRS.hasMatch()) {
            // Skip the standard uninstall keys — those are auto-generated on build
            const QString key = mRS.captured(2);
            if (key.contains("Uninstall", Qt::CaseInsensitive)) continue;

            RegistryEntry re;
            re.hive      = mRS.captured(1).toUpper();
            re.key       = key;
            re.valueName = mRS.captured(3);
            re.valueData = mRS.captured(4)
                               .replace("$INSTDIR", "{install-dir}", Qt::CaseInsensitive);
            re.valueType = "REG_SZ";
            m_manifest.registry.append(re);
            continue;
        }

        // ── WriteRegDWORD ──────────────────────────────────────────────────────
        auto mRD = reWriteRegDWORD.match(line);
        if (mRD.hasMatch()) {
            RegistryEntry re;
            re.hive      = mRD.captured(1).toUpper();
            re.key       = mRD.captured(2);
            re.valueName = mRD.captured(3);
            re.valueData = mRD.captured(4);
            re.valueType = "REG_DWORD";
            m_manifest.registry.append(re);
            continue;
        }
    }

    // Flush last open section (rare but possible if SectionEnd is missing)
    if (inSection && !isSystem && !currentComp.name.isEmpty())
        m_manifest.components.append(currentComp);

    // The first user-facing component is always required
    if (!m_manifest.components.isEmpty())
        m_manifest.components[0].required = true;

    m_manifest.targets << "windows";

    if (m_manifest.app.name.isEmpty())
        m_warnings << "Could not detect app name — check for a 'Name \"...\"' directive.";
    if (m_manifest.components.isEmpty())
        m_warnings << "No Section blocks found. Check NSIS script format.";

    return m_errors.isEmpty();
}
