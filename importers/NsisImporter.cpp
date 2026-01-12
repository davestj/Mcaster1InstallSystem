#include "NsisImporter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

// ── Phase 2 TODO ──────────────────────────────────────────────────────────────
// Full NSIS parser implementation.  Phase 1 stub reads basic metadata only.

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
    QTextStream in(&f);
    while (!in.atEnd())
        lines << in.readLine();

    // ── Extract basic metadata from header directives ─────────────────────────
    // Use named raw-string delimiter "re" so patterns containing )" don't terminate early
    static const QRegularExpression reName(R"re(^\s*Name\s+"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reVer(R"re(VIProductVersion\s+"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression rePub(R"re(VIAddVersionKey\s+"CompanyName"\s+"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reDir(R"re(InstallDir\s+"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reSec(R"re(^\s*Section\s+"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reFile(R"re(^\s*File\s+(?:/r\s+)?"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);

    m_manifest.components.clear();

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];

        auto mName = reName.match(line);
        if (mName.hasMatch()) {
            m_manifest.app.name = mName.captured(1);
            continue;
        }
        auto mVer = reVer.match(line);
        if (mVer.hasMatch()) {
            m_manifest.app.version = mVer.captured(1);
            continue;
        }
        auto mPub = rePub.match(line);
        if (mPub.hasMatch()) {
            m_manifest.app.publisher = mPub.captured(1);
            continue;
        }
        auto mDir = reDir.match(line);
        if (mDir.hasMatch()) {
            m_manifest.defaults.installDirWindows = mDir.captured(1);
            continue;
        }
        auto mSec = reSec.match(line);
        if (mSec.hasMatch()) {
            // Parse this section block
            Component comp;
            comp.id   = QString("sec_%1").arg(m_manifest.components.size());
            comp.name = mSec.captured(1);

            while (i + 1 < lines.size()) {
                ++i;
                const QString &sl = lines[i].trimmed();
                if (sl.compare("SectionEnd", Qt::CaseInsensitive) == 0) break;
                if (sl.compare("SectionIn RO", Qt::CaseInsensitive) == 0) {
                    comp.required = true;
                    continue;
                }
                auto mFile = reFile.match(sl);
                if (mFile.hasMatch()) {
                    FileEntry fe;
                    fe.src = mFile.captured(1).replace("\\", "/");
                    fe.dst = "{install-dir}/" + QFileInfo(fe.src).fileName();
                    comp.files.append(fe);
                }
            }

            m_manifest.components.append(comp);
            continue;
        }
    }

    // Ensure at least one required component
    if (!m_manifest.components.isEmpty())
        m_manifest.components[0].required = true;

    m_manifest.targets << "windows";

    if (m_manifest.app.name.isEmpty())
        m_warnings << "Could not detect app name from .nsi file.";
    if (m_manifest.components.isEmpty())
        m_warnings << "No Section blocks found — check NSIS script format.";

    // TODO Phase 2: parse CreateShortCut, WriteRegStr, MUI_PAGE_* directives
    m_warnings << "Phase 2 TODO: shortcuts, registry, and MUI dialog extraction not yet implemented.";

    return m_errors.isEmpty();
}

void NsisImporter::parseFile(const QString &) {}  // Phase 2
void NsisImporter::parseSection(const QStringList &, int &, Component &) {}  // Phase 2
AppInfo NsisImporter::parseFileHeader(const QStringList &) { return {}; }  // Phase 2
