#pragma once
/*
 * InnoSetupImporter — Inno Setup script (.iss) → .mis Manifest converter
 *
 * ISS sections handled:
 *   [Setup]      → AppInfo (AppName, AppVersion, AppPublisher, DefaultDirName, etc.)
 *   [Components] → Component objects (Name, Description, Flags: fixed → required)
 *   [Files]      → FileEntry per component (Source, DestDir, Components, Flags)
 *   [Icons]      → Shortcut objects (Name, Filename)
 *   [Registry]   → RegistryEntry objects (Root, Subkey, ValueType, ValueName, ValueData)
 *   [Run]        → CustomAction with trigger=after-install (Filename, Flags)
 */

#include "../manifest/Manifest.h"
#include <QStringList>
#include <QMap>

class InnoSetupImporter
{
public:
    bool parse(const QString &issPath);

    const Manifest    &manifest()  const { return m_manifest; }
    const QStringList &warnings()  const { return m_warnings; }
    const QStringList &errors()    const { return m_errors; }

private:
    void parseSetupSection(const QStringList &lines);
    void parseComponentsSection(const QStringList &lines,
                                QMap<QString, int> &compIndex); // id → index in m_manifest.components
    void parseFilesSection(const QStringList &lines,
                           Component &defaultComp,
                           const QMap<QString, int> &compIndex);
    void parseIconsSection(const QStringList &lines);
    void parseRegistrySection(const QStringList &lines);
    void parseRunSection(const QStringList &lines);

    QString issValue(const QString &line, const QString &key) const;

    Manifest    m_manifest;
    QStringList m_warnings;
    QStringList m_errors;
};
