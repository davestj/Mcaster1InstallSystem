#pragma once
/*
 * InnoSetupImporter — Phase 2
 * Parses Inno Setup scripts (.iss) and converts them to .mis Manifests.
 *
 * ISS sections handled:
 *   [Setup]      → AppInfo (AppName, AppVersion, AppPublisher, DefaultDirName)
 *   [Files]      → Component files (Source, DestDir, Flags)
 *   [Icons]      → Shortcuts (Name, Filename)
 *   [Components] → Components (Name, Description, Flags)
 *   [Tasks]      → Optional install tasks (future)
 *   [Registry]   → Registry entries (future)
 *   [Run]        → Post-install actions (future)
 */

#include "../manifest/Manifest.h"
#include <QStringList>

class InnoSetupImporter
{
public:
    bool parse(const QString &issPath);

    const Manifest    &manifest()  const { return m_manifest; }
    const QStringList &warnings()  const { return m_warnings; }
    const QStringList &errors()    const { return m_errors; }

private:
    void parseSetupSection(const QStringList &lines);
    void parseFilesSection(const QStringList &lines, Component &defaultComp);
    void parseIconsSection(const QStringList &lines);
    QString issValue(const QString &line, const QString &key) const;

    Manifest    m_manifest;
    QStringList m_warnings;
    QStringList m_errors;
};
