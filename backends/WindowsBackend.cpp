/*
 * WindowsBackend.cpp — Mcaster1 native Windows installer packager
 *
 * Produces a self-contained distribution package from a .mis manifest
 * using ONLY the Mcaster1 runtime installer — no NSIS, no Inno Setup,
 * no makensis.exe, no iscc.exe, no third-party build tool required.
 *
 * Output:
 *   <output-dir>/<Publisher>-<Name>-<Version>-win64-setup/   ← staged dir
 *   <output-dir>/<Publisher>-<Name>-<Version>-win64-setup.zip ← distributable
 */

#include "WindowsBackend.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

// ── outputFilename ─────────────────────────────────────────────────────────────
QString WindowsBackend::outputFilename(const Manifest &m) const
{
    const QString base = QString("%1-%2-%3-win64-setup")
        .arg(m.app.publisher.isEmpty() ? "Mcaster1" : m.app.publisher,
             m.app.name.isEmpty()      ? "App"       : m.app.name,
             m.app.version.isEmpty()   ? "1.0.0"     : m.app.version)
        .replace(' ', '_');
    return base + ".zip";
}

// ── validate ──────────────────────────────────────────────────────────────────
QStringList WindowsBackend::validate(const Manifest &m, const QString &projectDir) const
{
    Q_UNUSED(projectDir)
    QStringList issues = m.validate();

    // Runtime installer is optional at validate time — missing is a warning,
    // not a hard error (the package can still be staged without it).
    if (findRuntimeExe().isEmpty())
        issues << "WARNING: Mcaster1Installer.exe not found — "
                  "the Windows package will be staged without a bundled installer. "
                  "Build the runtime project first (cmake --build runtime/build).";

    return issues;
}

// ── build ─────────────────────────────────────────────────────────────────────
bool WindowsBackend::build(const Manifest   &m,
                            const QString    &projectDir,
                            const QString    &outputDir,
                            const ProgressFn &progress,
                            QString          *errOut)
{
    report(progress, 2,  "=== Windows Build Started ===");
    report(progress, 3,  QString("App: %1 v%2  |  Publisher: %3")
           .arg(m.app.name, m.app.version, m.app.publisher));

    QDir().mkpath(outputDir);

    // ── [1] Determine staging directory name ──────────────────────────────────
    const QString pkgBase = QFileInfo(outputFilename(m)).completeBaseName(); // strip .zip
    const QString stageDir = outputDir + "/" + pkgBase;

    report(progress, 8, "── [1/5] Creating staging directory…");
    QDir(stageDir).removeRecursively();
    if (!QDir().mkpath(stageDir)) {
        if (errOut) *errOut = "Cannot create staging directory: " + stageDir;
        return false;
    }
    report(progress, 12, "  Stage: " + stageDir);

    // ── [2] Write manifest.mis into staging root ───────────────────────────────
    report(progress, 15, "── [2/5] Writing manifest…");
    QString saveErr;
    if (!m.save(stageDir + "/manifest.mis", &saveErr)) {
        if (errOut) *errOut = "Cannot write manifest: " + saveErr;
        QDir(stageDir).removeRecursively();
        return false;
    }
    report(progress, 20, "  manifest.mis written.");

    // ── [3] Copy payload ──────────────────────────────────────────────────────
    report(progress, 22, "── [3/5] Copying payload…");
    const QString payloadSrc = projectDir + "/payload";
    if (QDir(payloadSrc).exists()) {
        if (!copyDir(payloadSrc, stageDir + "/payload")) {
            if (errOut) *errOut = "Failed to copy payload directory.";
            QDir(stageDir).removeRecursively();
            return false;
        }
        report(progress, 48, QString("  Payload copied from: %1").arg(payloadSrc));
    } else {
        report(progress, 48, "  WARN: No payload/ directory found — package will contain only the manifest.");
    }

    // ── [4] Bundle runtime installer (optional) ────────────────────────────────
    report(progress, 50, "── [4/5] Bundling Mcaster1 runtime installer…");
    const QString runtimeExe = findRuntimeExe();
    if (!runtimeExe.isEmpty()) {
        const QString dstExe = stageDir + "/Mcaster1Installer.exe";
        if (!QFile::copy(runtimeExe, dstExe)) {
            report(progress, 55, "  WARN: Could not copy Mcaster1Installer.exe — package will be staged without it.");
        } else {
            report(progress, 58, "  Mcaster1Installer.exe bundled.");
        }
    } else {
        report(progress, 55, "  INFO: Mcaster1Installer.exe not found — omitting from package.");
        report(progress, 56, "        Build runtime/build/Mcaster1Installer.exe then rebuild.");
    }

    // Copy license file if declared in manifest
    if (!m.app.licenseFile.isEmpty()) {
        const QString licenseSrc = projectDir + "/" + m.app.licenseFile;
        if (QFile::exists(licenseSrc))
            QFile::copy(licenseSrc, stageDir + "/" + QFileInfo(m.app.licenseFile).fileName());
    }

    // ── [5] Zip the staged directory ──────────────────────────────────────────
    report(progress, 60, "── [5/5] Creating distributable zip package…");
    const QString zipPath = outputDir + "/" + outputFilename(m);
    if (!zipDir(stageDir, zipPath, progress)) {
        // Non-fatal: staged directory is still usable
        report(progress, 90, "  WARN: zip step failed — staged directory is ready at:");
        report(progress, 91, "        " + stageDir);
        report(progress, 92, "  Distribute the staged directory or zip it manually.");
    } else {
        report(progress, 95, "  Package: " + zipPath);
    }

    report(progress, 100, "=== Windows Build Complete ===");
    return true;
}

// ── Private: findRuntimeExe ───────────────────────────────────────────────────
QString WindowsBackend::findRuntimeExe() const
{
    // Probe paths relative to the Studio executable and project root
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        // cmake build tree (macOS Studio build → runtime sibling)
        exeDir + "/../../../../runtime/build/Mcaster1Installer.exe",
        exeDir + "/../../../runtime/build/Mcaster1Installer.exe",
        exeDir + "/../../runtime/build/Mcaster1Installer.exe",
        exeDir + "/../runtime/build/Mcaster1Installer.exe",
        // Flat layout (installer next to studio)
        exeDir + "/Mcaster1Installer.exe",
        // Windows VS2022 build tree layouts
        exeDir + "/../../../../windows/x64/Debug/Mcaster1Installer.exe",
        exeDir + "/../../../../windows/x64/Release/Mcaster1Installer.exe",
    };
    for (const QString &c : candidates) {
        const QString canonical = QFileInfo(c).absoluteFilePath();
        if (QFile::exists(canonical))
            return canonical;
    }
    return QString();
}

// ── Private: copyDir ──────────────────────────────────────────────────────────
bool WindowsBackend::copyDir(const QString &src, const QString &dst) const
{
    QDir srcDir(src);
    if (!srcDir.exists()) return false;
    if (!QDir().mkpath(dst)) return false;

    QDirIterator it(src, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString srcPath = it.next();
        const QString rel     = srcDir.relativeFilePath(srcPath);
        const QString dstPath = dst + "/" + rel;

        if (QFileInfo(srcPath).isDir()) {
            QDir().mkpath(dstPath);
        } else {
            QDir().mkpath(QFileInfo(dstPath).absolutePath());
            if (!QFile::copy(srcPath, dstPath))
                return false;
        }
    }
    return true;
}

// ── Private: zipDir ───────────────────────────────────────────────────────────
bool WindowsBackend::zipDir(const QString &srcDir, const QString &zipPath,
                             const ProgressFn &progress) const
{
    // Remove any existing zip at the target path
    QFile::remove(zipPath);

    // Use the system `zip` command (available on macOS and Linux;
    // on Windows use PowerShell's Compress-Archive via cmd)
    QProcess p;

#if defined(Q_OS_WIN)
    // PowerShell Compress-Archive — available on all Windows 8+ systems
    const QString cmd = QString(
        "powershell -NoProfile -NonInteractive -Command "
        "\"Compress-Archive -Path '%1\\*' -DestinationPath '%2' -Force\"")
        .arg(QDir::toNativeSeparators(srcDir),
             QDir::toNativeSeparators(zipPath));
    p.start("cmd.exe", {"/C", cmd});
#else
    // POSIX zip — always available on macOS; install on Linux: apt install zip
    const QString parentDir = QFileInfo(srcDir).absolutePath();
    const QString dirName   = QFileInfo(srcDir).fileName();
    p.setWorkingDirectory(parentDir);
    p.start("zip", {"-r", zipPath, dirName});
#endif

    report(progress, 72, "  Running zip…");
    if (!p.waitForFinished(120000)) {
        p.kill();
        report(progress, 75, "  WARN: zip timed out.");
        return false;
    }

    if (p.exitCode() != 0) {
        const QString err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        report(progress, 75, "  WARN: zip exited with code " +
               QString::number(p.exitCode()) + (err.isEmpty() ? "" : ": " + err));
        return false;
    }

    return QFile::exists(zipPath);
}
