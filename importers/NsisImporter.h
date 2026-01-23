#pragma once
/*
 * NsisImporter — NSIS script (.nsi) → .mis Manifest converter
 *
 * Single-pass state-machine parser. Handles:
 *   Section / SectionEnd blocks → Component objects
 *   SectionIn RO               → required component
 *   SetOutPath                  → per-file destination directory
 *   File / File /r              → FileEntry (isDir when /r)
 *   CreateShortCut              → Shortcut
 *   WriteRegStr / WriteRegDWORD → RegistryEntry
 *   Name / VIProductVersion / VIAddVersionKey / InstallDir → AppInfo + InstallDefaults
 *
 * Hidden sections (Section -Post, Section -Prerequisites) are parsed for
 * shortcuts and registry entries but not surfaced as user components.
 * The Uninstall section is skipped entirely.
 */

#include "../manifest/Manifest.h"
#include <QStringList>

class NsisImporter
{
public:
    bool parse(const QString &nsiPath);

    const Manifest    &manifest()  const { return m_manifest; }
    const QStringList &warnings()  const { return m_warnings; }
    const QStringList &errors()    const { return m_errors; }

private:
    Manifest    m_manifest;
    QStringList m_warnings;
    QStringList m_errors;
};
