#pragma once
#include "BuildBackend.h"

/*
 * LinuxBackend — generates a .deb package + AppImage skeleton from a Manifest.
 *
 * Phase 1 outputs:
 *   - Debian control file + install tree → .deb (via dpkg-deb if available)
 *   - AppDir skeleton for AppImage (requires appimagetool to finalize)
 *
 * Output: <output-dir>/<publisher>-<name>-<version>-linux-amd64.deb
 */
class LinuxBackend : public BuildBackend
{
public:
    QString platform()    const override { return "linux"; }
    QString displayName() const override { return "Linux (.deb / AppImage)"; }

    QStringList validate(const Manifest &m, const QString &projectDir) const override;

    bool build(const Manifest   &m,
               const QString    &projectDir,
               const QString    &outputDir,
               const ProgressFn &progress,
               QString          *errOut = nullptr) override;

    QString outputFilename(const Manifest &m) const override;

private:
    QString generateDebControl(const Manifest &m, const QString &payloadPath) const;
    bool    buildDebPackage(const QString &stageDir, const QString &debPath) const;
    bool    buildAppDir(const Manifest &m, const QString &projectDir,
                        const QString &appDir) const;
};
