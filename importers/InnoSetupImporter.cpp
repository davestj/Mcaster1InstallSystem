#include "InnoSetupImporter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

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

    QStringList lines;
    QTextStream in(&f);
    while (!in.atEnd())
        lines << in.readLine();

    // Split into named sections
    QString currentSection;
    QMap<QString, QStringList> sections;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            currentSection = trimmed.mid(1, trimmed.length() - 2).toLower();
        } else if (!currentSection.isEmpty() && !trimmed.startsWith(';')) {
            sections[currentSection] << trimmed;
        }
    }

    if (sections.contains("setup"))
        parseSetupSection(sections["setup"]);

    Component defaultComp;
    defaultComp.id       = "core";
    defaultComp.name     = "Core Application";
    defaultComp.required = true;
    defaultComp.selected = true;

    if (sections.contains("files"))
        parseFilesSection(sections["files"], defaultComp);

    if (!defaultComp.files.isEmpty())
        m_manifest.components.prepend(defaultComp);

    if (sections.contains("icons"))
        parseIconsSection(sections["icons"]);

    m_manifest.targets << "windows";

    if (m_manifest.app.name.isEmpty())
        m_warnings << "Could not detect AppName from [Setup] section.";

    // TODO Phase 2: [Components], [Tasks], [Registry], [Run] sections
    m_warnings << "Phase 2 TODO: [Components], [Tasks], [Registry], [Run] sections not yet parsed.";

    return m_errors.isEmpty();
}

void InnoSetupImporter::parseSetupSection(const QStringList &lines)
{
    for (const QString &line : lines) {
        QString name  = issValue(line, "AppName");
        QString ver   = issValue(line, "AppVersion");
        QString pub   = issValue(line, "AppPublisher");
        QString url   = issValue(line, "AppURL");
        QString dir   = issValue(line, "DefaultDirName");
        QString id    = issValue(line, "AppId");
        QString desc  = issValue(line, "AppComments");

        if (!name.isEmpty())  m_manifest.app.name        = name;
        if (!ver.isEmpty())   m_manifest.app.version     = ver;
        if (!pub.isEmpty())   m_manifest.app.publisher   = pub;
        if (!url.isEmpty())   m_manifest.app.url         = url;
        if (!desc.isEmpty())  m_manifest.app.description = desc;
        if (!id.isEmpty())    m_manifest.app.identifier  = id.remove('{').remove('}');
        if (!dir.isEmpty()) {
            // Resolve {pf} = "C:\Program Files", {app} = install dir
            dir.replace("{pf}", "C:\\Program Files");
            m_manifest.defaults.installDirWindows = dir;
        }
    }
}

void InnoSetupImporter::parseFilesSection(const QStringList &lines, Component &comp)
{
    static const QRegularExpression reSrc(R"re(Source:\s*"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reDst(R"re(DestDir:\s*"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);

    for (const QString &line : lines) {
        auto mSrc = reSrc.match(line);
        if (!mSrc.hasMatch()) continue;

        FileEntry fe;
        fe.src = mSrc.captured(1).replace("\\", "/");

        auto mDst = reDst.match(line);
        fe.dst = mDst.hasMatch()
            ? mDst.captured(1).replace("{app}", "{install-dir}").replace("\\", "/") + "/" + QFileInfo(fe.src).fileName()
            : "{install-dir}/" + QFileInfo(fe.src).fileName();

        if (line.contains("recursesubdirs", Qt::CaseInsensitive))
            fe.isDir = true;

        comp.files.append(fe);
    }
}

void InnoSetupImporter::parseIconsSection(const QStringList &lines)
{
    static const QRegularExpression reName(R"re(Name:\s*"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reFile(R"re(Filename:\s*"([^"]+)")re", QRegularExpression::CaseInsensitiveOption);

    for (const QString &line : lines) {
        auto mN = reName.match(line);
        auto mF = reFile.match(line);
        if (!mN.hasMatch() || !mF.hasMatch()) continue;

        Shortcut sc;
        sc.name   = QFileInfo(mN.captured(1)).baseName();
        sc.target = mF.captured(1).replace("{app}", "{install-dir}").replace("\\", "/");
        sc.type   = "app";
        m_manifest.shortcuts.append(sc);
    }
}

QString InnoSetupImporter::issValue(const QString &line, const QString &key) const
{
    static QMap<QString, QRegularExpression> cache;
    if (!cache.contains(key)) {
        cache[key] = QRegularExpression(key + R"(\s*=\s*(.+)$)", QRegularExpression::CaseInsensitiveOption);
    }
    auto m = cache[key].match(line);
    return m.hasMatch() ? m.captured(1).trimmed().remove('"') : QString();
}
