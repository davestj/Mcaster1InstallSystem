/*
 * InnoSetupImporter.cpp — Full Phase 2 Inno Setup script parser
 */
#include "InnoSetupImporter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

// Helper: convert Inno {app} / {pf} tokens → our .mis tokens
static QString resolveIssPath(const QString &p)
{
    QString r = p;
    r.replace("{app}",          "{install-dir}",              Qt::CaseInsensitive);
    r.replace("{pf}",           "C:/Program Files",           Qt::CaseInsensitive);
    r.replace("{pf64}",         "C:/Program Files",           Qt::CaseInsensitive);
    r.replace("{commonappdata}","C:/ProgramData",              Qt::CaseInsensitive);
    r.replace("{localappdata}", "C:/Users/Default/AppData/Local", Qt::CaseInsensitive);
    r.replace('\\', '/');
    return r;
}

// Helper: convert Inno ValueType string → REG_* constant
static QString issRegType(const QString &t)
{
    const QString lt = t.toLower();
    if (lt == "dword")    return "REG_DWORD";
    if (lt == "expandsz") return "REG_EXPAND_SZ";
    if (lt == "multisz")  return "REG_MULTI_SZ";
    if (lt == "binary")   return "REG_BINARY";
    return "REG_SZ";  // string / none / default
}

bool InnoSetupImporter::parse(const QString &issPath)
{
    m_warnings.clear();
    m_errors.clear();
    m_manifest = Manifest::newProject();

    QFile f(issPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_errors << QString("Cannot open file: %1").arg(issPath);
        return false;
    }

    // ── Split into named sections ─────────────────────────────────────────────
    QStringList lines;
    {
        QTextStream in(&f);
        while (!in.atEnd()) lines << in.readLine();
    }

    QString currentSection;
    QMap<QString, QStringList> sections;

    for (const QString &line : lines) {
        QString t = line.trimmed();
        if (t.startsWith('[') && t.endsWith(']'))
            currentSection = t.mid(1, t.length() - 2).toLower();
        else if (!currentSection.isEmpty() && !t.startsWith(';') && !t.isEmpty())
            sections[currentSection] << t;
    }

    // ── Parse [Setup] first so metadata is available ──────────────────────────
    if (sections.contains("setup"))
        parseSetupSection(sections["setup"]);

    // ── Parse [Components] → build component map (id → index) ────────────────
    QMap<QString, int> compIndex;
    if (sections.contains("components"))
        parseComponentsSection(sections["components"], compIndex);

    // ── Default "Core Application" component (for files with no [Components] match)
    Component defaultComp;
    defaultComp.id       = "core";
    defaultComp.name     = "Core Application";
    defaultComp.required = true;
    defaultComp.selected = true;

    // ── Parse [Files] ─────────────────────────────────────────────────────────
    if (sections.contains("files"))
        parseFilesSection(sections["files"], defaultComp, compIndex);

    // Prepend default component only if it picked up files
    if (!defaultComp.files.isEmpty())
        m_manifest.components.prepend(defaultComp);

    // ── Parse remaining sections ──────────────────────────────────────────────
    if (sections.contains("icons"))    parseIconsSection(sections["icons"]);
    if (sections.contains("registry")) parseRegistrySection(sections["registry"]);
    if (sections.contains("run"))      parseRunSection(sections["run"]);

    m_manifest.targets << "windows";

    if (m_manifest.app.name.isEmpty())
        m_warnings << "Could not detect AppName from [Setup] section.";

    return m_errors.isEmpty();
}

// ── [Setup] ───────────────────────────────────────────────────────────────────
void InnoSetupImporter::parseSetupSection(const QStringList &lines)
{
    for (const QString &line : lines) {
        const QString name  = issValue(line, "AppName");
        const QString ver   = issValue(line, "AppVersion");
        const QString pub   = issValue(line, "AppPublisher");
        const QString url   = issValue(line, "AppURL");
        const QString dir   = issValue(line, "DefaultDirName");
        const QString id    = issValue(line, "AppId");
        const QString desc  = issValue(line, "AppComments");
        const QString supp  = issValue(line, "AppSupportURL");

        if (!name.isEmpty())  m_manifest.app.name        = name;
        if (!ver.isEmpty())   m_manifest.app.version     = ver;
        if (!pub.isEmpty())   m_manifest.app.publisher   = pub;
        if (!url.isEmpty())   m_manifest.app.url         = url;
        if (!supp.isEmpty())  m_manifest.app.supportUrl  = supp;
        if (!desc.isEmpty())  m_manifest.app.description = desc;
        if (!id.isEmpty()) {
            QString cleanId = id;
            cleanId.remove('{');
            cleanId.remove('}');
            m_manifest.app.identifier = cleanId;
        }

        if (!dir.isEmpty())
            m_manifest.defaults.installDirWindows = resolveIssPath(dir);
    }
}

// ── [Components] ─────────────────────────────────────────────────────────────
void InnoSetupImporter::parseComponentsSection(const QStringList &lines,
                                               QMap<QString, int> &compIndex)
{
    static const QRegularExpression reName(
        R"re(Name:\s*"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reDesc(
        R"re(Description:\s*"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reFlags(
        R"re(Flags:\s*([^;]+))re", QRegularExpression::CaseInsensitiveOption);

    for (const QString &line : lines) {
        auto mN = reName.match(line);
        if (!mN.hasMatch()) continue;

        Component comp;
        // Inno component names can be hierarchical: "main/core" — normalise to underscore
        comp.id = mN.captured(1).replace('/', '_').replace('\\', '_');

        auto mD = reDesc.match(line);
        comp.name        = mD.hasMatch() ? mD.captured(1) : comp.id;
        comp.description = comp.name;
        comp.selected    = true;

        auto mF = reFlags.match(line);
        if (mF.hasMatch() &&
            mF.captured(1).contains("fixed", Qt::CaseInsensitive))
            comp.required = true;

        compIndex[mN.captured(1).toLower()] = m_manifest.components.size();
        m_manifest.components.append(comp);
    }
}

// ── [Files] ──────────────────────────────────────────────────────────────────
void InnoSetupImporter::parseFilesSection(const QStringList &lines,
                                          Component &defaultComp,
                                          const QMap<QString, int> &compIndex)
{
    static const QRegularExpression reSrc(
        R"re(Source:\s*"([^"]+)")re",         QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reDst(
        R"re(DestDir:\s*"([^"]+)")re",        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reComp(
        R"re(Components:\s*([\w/\\]+))re",    QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reChmod(
        R"re(Attribs:\s*([^;]+))re",          QRegularExpression::CaseInsensitiveOption);

    for (const QString &line : lines) {
        auto mSrc = reSrc.match(line);
        if (!mSrc.hasMatch()) continue;

        FileEntry fe;
        fe.src = mSrc.captured(1).replace('\\', '/');

        auto mDst = reDst.match(line);
        const QString dstDir = mDst.hasMatch()
            ? resolveIssPath(mDst.captured(1))
            : "{install-dir}";
        fe.dst = dstDir + "/" + QFileInfo(fe.src).fileName();

        if (line.contains("recursesubdirs", Qt::CaseInsensitive))
            fe.isDir = true;

        // Assign to the named component if one is given
        auto mC = reComp.match(line);
        if (mC.hasMatch()) {
            const QString cid = mC.captured(1).toLower();
            if (compIndex.contains(cid)) {
                m_manifest.components[compIndex[cid]].files.append(fe);
                continue;
            }
        }
        // Fallback: add to default "core" component
        defaultComp.files.append(fe);
    }
}

// ── [Icons] ───────────────────────────────────────────────────────────────────
void InnoSetupImporter::parseIconsSection(const QStringList &lines)
{
    static const QRegularExpression reName(
        R"re(Name:\s*"([^"]+)")re",     QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reFile(
        R"re(Filename:\s*"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);

    for (const QString &line : lines) {
        auto mN = reName.match(line);
        auto mF = reFile.match(line);
        if (!mN.hasMatch() || !mF.hasMatch()) continue;

        Shortcut sc;
        sc.name   = QFileInfo(mN.captured(1).replace('\\', '/')).baseName();
        sc.target = resolveIssPath(mF.captured(1));
        sc.type   = "app";
        m_manifest.shortcuts.append(sc);
    }
}

// ── [Registry] ────────────────────────────────────────────────────────────────
void InnoSetupImporter::parseRegistrySection(const QStringList &lines)
{
    static const QRegularExpression reRoot(
        R"re(Root:\s*(\w+))re",             QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reSubkey(
        R"re(Subkey:\s*"([^"]+)")re",       QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reVType(
        R"re(ValueType:\s*(\w+))re",        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reVName(
        R"re(ValueName:\s*"([^"]*)")re",    QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reVData(
        R"re(ValueData:\s*"([^"]*)")re",    QRegularExpression::CaseInsensitiveOption);

    for (const QString &line : lines) {
        auto mR = reRoot.match(line);
        if (!mR.hasMatch()) continue;

        // Skip standard uninstall keys — they are auto-generated
        auto mSub = reSubkey.match(line);
        if (mSub.hasMatch() &&
            mSub.captured(1).contains("Uninstall", Qt::CaseInsensitive))
            continue;

        RegistryEntry re;
        re.hive = mR.captured(1).toUpper();

        if (mSub.hasMatch()) re.key = mSub.captured(1);

        auto mT = reVType.match(line);
        re.valueType = mT.hasMatch() ? issRegType(mT.captured(1)) : "REG_SZ";

        auto mN = reVName.match(line);
        if (mN.hasMatch()) re.valueName = mN.captured(1);

        auto mD = reVData.match(line);
        if (mD.hasMatch())
            re.valueData = resolveIssPath(mD.captured(1));

        if (!re.key.isEmpty())
            m_manifest.registry.append(re);
    }
}

// ── [Run] ─────────────────────────────────────────────────────────────────────
void InnoSetupImporter::parseRunSection(const QStringList &lines)
{
    static const QRegularExpression reFilename(
        R"re(Filename:\s*"([^"]+)")re",     QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reParams(
        R"re(Parameters:\s*"([^"]*)")re",   QRegularExpression::CaseInsensitiveOption);

    int idx = 0;
    for (const QString &line : lines) {
        auto mF = reFilename.match(line);
        if (!mF.hasMatch()) continue;

        CustomAction ca;
        ca.id = QString("run_%1").arg(idx++);

        QString cmd = resolveIssPath(mF.captured(1));
        auto mP = reParams.match(line);
        if (mP.hasMatch() && !mP.captured(1).isEmpty())
            cmd += " " + mP.captured(1);

        ca.command  = cmd;
        ca.type     = "shell";
        ca.trigger  = "after-install";
        ca.platforms << "windows";
        m_manifest.customActions.append(ca);
    }
}

// ── issValue helper ───────────────────────────────────────────────────────────
QString InnoSetupImporter::issValue(const QString &line, const QString &key) const
{
    // Build and cache per-key regex on first use
    static QMap<QString, QRegularExpression> cache;
    if (!cache.contains(key))
        cache[key] = QRegularExpression(
            key + R"(\s*=\s*(.+)$)", QRegularExpression::CaseInsensitiveOption);

    auto m = cache[key].match(line);
    return m.hasMatch() ? m.captured(1).trimmed().remove('"') : QString();
}
