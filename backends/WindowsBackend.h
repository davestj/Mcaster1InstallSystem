#pragma once
#include "BuildBackend.h"

/*
 * WindowsBackend — generates a NSIS installer script (.nsi) from a Manifest
 * and optionally compiles it to a .exe using makensis.
 *
 * Phase 1: generates the .nsi script (human-readable, fully usable).
 * Phase 2: auto-compile via makensis if available.
 *
 * Output: <output-dir>/<publisher>-<name>-<version>-win64-setup.exe
 *         (or <publisher>-<name>-<version>-win64-setup.nsi if makensis absent)
 */
class WindowsBackend : public BuildBackend
{
public:
    QString platform()    const override { return "windows"; }
    QString displayName() const override { return "Windows (.exe NSIS)"; }

    QStringList validate(const Manifest &m, const QString &projectDir) const override;

    bool build(const Manifest   &m,
               const QString    &projectDir,
               const QString    &outputDir,
               const ProgressFn &progress,
               QString          *errOut = nullptr) override;

    QString outputFilename(const Manifest &m) const override;

private:
    QString generateNsiScript(const Manifest &m, const QString &projectDir) const;
    bool    compileMakensis(const QString &nsiPath, const QString &exePath) const;
};
