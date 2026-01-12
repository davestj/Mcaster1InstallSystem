#pragma once
/*
 * NsisImporter — Phase 2
 * Parses NSIS installer scripts (.nsi) and converts them to .mis Manifests.
 *
 * Supported sections:
 *   [Files]       → Component.files
 *   Section/SectionEnd blocks → Components
 *   CreateShortCut → Shortcuts
 *   Name / OutFile / InstallDir → AppInfo + InstallDefaults
 *   WriteRegStr → registry actions (future)
 *   VIProductVersion / VIAddVersionKey → AppInfo
 */

#include "../manifest/Manifest.h"
#include <QStringList>

class NsisImporter
{
public:
    // Parse a .nsi file and produce a Manifest.
    // Returns true on success; errors/warnings accumulated in messages().
    bool parse(const QString &nsiPath);

    const Manifest    &manifest()  const { return m_manifest; }
    const QStringList &warnings()  const { return m_warnings; }
    const QStringList &errors()    const { return m_errors; }

private:
    // TODO Phase 2: implement section-by-section parser
    void parseFile(const QString &nsiPath);
    void parseSection(const QStringList &lines, int &idx, Component &comp);
    AppInfo parseFileHeader(const QStringList &lines);

    Manifest    m_manifest;
    QStringList m_warnings;
    QStringList m_errors;
};
