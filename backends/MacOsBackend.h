#pragma once
#include "BuildBackend.h"

/*
 * MacOsBackend — builds a macOS DMG installer.
 *
 * Phase 1 build steps:
 *   1. Validate payload directory (required .app / service binary)
 *   2. Inject manifest.mis into runtime wizard bundle (Resources/)
 *   3. Copy runtime Mcaster1Installer.app into a temp staging area
 *   4. Run macdeployqt on the runtime app
 *   5. Inject payload/ into runtime Resources/payload/
 *   6. Codesign (ad-hoc)
 *   7. Create DMG (create-dmg or hdiutil fallback)
 *
 * Output: <output-dir>/<publisher>-<name>-<version>-macOS-<arch>.dmg
 */
class MacOsBackend : public BuildBackend
{
public:
    QString platform()    const override { return "macos"; }
    QString displayName() const override { return "macOS (.dmg)"; }

    QStringList validate(const Manifest &m, const QString &projectDir) const override;

    bool build(const Manifest   &m,
               const QString    &projectDir,
               const QString    &outputDir,
               const ProgressFn &progress,
               QString          *errOut = nullptr) override;

    QString outputFilename(const Manifest &m) const override;

private:
    bool runShell(const QString &cmd, QString *out = nullptr) const;
    bool injectManifest(const QString &runtimeApp, const Manifest &m,
                        const QString &projectDir) const;
    bool createDmg(const QString &appPath,   const QString &volName,
                   const QString &dmgPath,   const QString &bgImage) const;
};
