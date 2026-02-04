#pragma once
#include "BuildBackend.h"

/*
 * WindowsBackend — packages a Windows installer using the Mcaster1 runtime.
 *
 * Output structure (staged, then zipped):
 *   <output-dir>/<Publisher>-<Name>-<Version>-win64-setup/
 *     manifest.mis             — project manifest (read by Mcaster1Installer.exe)
 *     payload/                 — application files (copied from project payload/)
 *     Mcaster1Installer.exe    — bundled runtime installer (if found)
 *     LICENSE.txt              — licence file (if declared in manifest)
 *
 * The staged directory is also zipped to produce:
 *   <output-dir>/<Publisher>-<Name>-<Version>-win64-setup.zip
 *
 * No third-party compiler (NSIS, Inno Setup, WiX etc.) is required.
 * We import .nsi / .iss files only as a conversion step — we never invoke
 * makensis.exe or iscc.exe to compile them.
 */
class WindowsBackend : public BuildBackend
{
public:
    QString platform()    const override { return "windows"; }
    QString displayName() const override { return "Windows Installer Package"; }

    QStringList validate(const Manifest &m, const QString &projectDir) const override;

    bool build(const Manifest   &m,
               const QString    &projectDir,
               const QString    &outputDir,
               const ProgressFn &progress,
               QString          *errOut = nullptr) override;

    QString outputFilename(const Manifest &m) const override;

private:
    // Locate the Windows runtime installer binary.
    // Searches relative to applicationDirPath() and the runtime/build/ tree.
    QString findRuntimeExe() const;

    // Copy src directory tree into dst, creating dst if needed.
    bool copyDir(const QString &src, const QString &dst) const;

    // Create a zip archive of srcDir at zipPath using the system zip command.
    bool zipDir(const QString &srcDir, const QString &zipPath,
                const ProgressFn &progress) const;
};
