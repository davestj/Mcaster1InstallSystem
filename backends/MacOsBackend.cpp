#include "MacOsBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QSysInfo>

// ── validate ──────────────────────────────────────────────────────────────────
QStringList MacOsBackend::validate(const Manifest &m, const QString &projectDir) const
{
    QStringList issues = m.validate();

    // Check macdeployqt is available
    QProcess p;
    p.start("which", {"macdeployqt"});
    p.waitForFinished(3000);
    if (p.exitCode() != 0)
        issues << "macdeployqt not found in PATH. Install Homebrew Qt: brew install qt";

    // Check hdiutil (always present on macOS)
    QProcess p2;
    p2.start("which", {"hdiutil"});
    p2.waitForFinished(3000);
    if (p2.exitCode() != 0)
        issues << "hdiutil not found (should always exist on macOS).";

    // Check runtime app exists
    QString runtimeApp = QCoreApplication::applicationDirPath()
                         + "/../../../runtime/build/Mcaster1Installer.app";
    QDir rd(QDir::cleanPath(runtimeApp));
    if (!rd.exists())
        issues << QString("Runtime installer not found: %1\n  Run: make runtime").arg(rd.absolutePath());

    return issues;
}

// ── outputFilename ─────────────────────────────────────────────────────────────
QString MacOsBackend::outputFilename(const Manifest &m) const
{
    QString arch = QSysInfo::currentCpuArchitecture() == "arm64" ? "arm64" : "x86_64";
    return QString("%1-%2-%3-macOS-%4-installer.dmg")
        .arg(m.app.publisher)
        .arg(m.app.name)
        .arg(m.app.version)
        .arg(arch)
        .replace(' ', '_');
}

// ── build ─────────────────────────────────────────────────────────────────────
bool MacOsBackend::build(const Manifest   &m,
                          const QString    &projectDir,
                          const QString    &outputDir,
                          const ProgressFn &progress,
                          QString          *errOut)
{
    report(progress, 2, "=== macOS Build Started ===");
    report(progress, 5, QString("App: %1 %2").arg(m.app.name, m.app.version));

    // ── Locate runtime installer app ──────────────────────────────────────────
    // The studio knows where the runtime was built (relative to project root)
    QString studioDir   = QCoreApplication::applicationDirPath();
    // In .app bundle: .../Mcaster1InstallStudio.app/Contents/MacOS/
    // Project root is 4 levels up: Contents/MacOS/ → build/ → studio/ → root
    QString projectRoot = QFileInfo(studioDir + "/../../../../..").canonicalFilePath();
    QString runtimeApp  = projectRoot + "/runtime/build/Mcaster1Installer.app";
    if (!QDir(runtimeApp).exists()) {
        // Fallback: next to studio build
        runtimeApp = QFileInfo(studioDir + "/../../runtime/build/Mcaster1Installer.app").canonicalFilePath();
    }

    if (!QDir(runtimeApp).exists()) {
        if (errOut) *errOut = QString("Runtime installer not found: %1\n\nRun: make runtime").arg(runtimeApp);
        return false;
    }
    report(progress, 10, "  Runtime: " + runtimeApp);

    // ── Create temp staging area ───────────────────────────────────────────────
    QString tmpDir = QDir::tempPath() + "/mis_build_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(tmpDir);

    QString stagedApp = tmpDir + "/Mcaster1Installer.app";

    report(progress, 15, "[1/5] Copying runtime installer...");
    if (!runShell(QString("cp -R \"%1\" \"%2\"").arg(runtimeApp, stagedApp))) {
        if (errOut) *errOut = "Failed to copy runtime installer app.";
        QDir(tmpDir).removeRecursively();
        return false;
    }

    // ── Inject manifest.mis ────────────────────────────────────────────────────
    report(progress, 25, "[2/5] Injecting project manifest...");
    if (!injectManifest(stagedApp, m, projectDir)) {
        if (errOut) *errOut = "Failed to inject manifest into runtime bundle.";
        QDir(tmpDir).removeRecursively();
        return false;
    }

    // ── Inject payload/ ────────────────────────────────────────────────────────
    report(progress, 35, "[3/5] Injecting payload...");
    QString payloadSrc = projectDir + "/payload";
    QString payloadDst = stagedApp  + "/Contents/Resources/payload";
    if (QDir(payloadSrc).exists()) {
        runShell(QString("cp -R \"%1\" \"%2\"").arg(payloadSrc, payloadDst));
        report(progress, 45, QString("  Payload: %1 MB")
            .arg(QDir(payloadDst).entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot).size()));
    } else {
        report(progress, 45, "  WARN: No payload/ directory — installer will have empty payload.");
    }

    // ── macdeployqt + codesign ─────────────────────────────────────────────────
    report(progress, 50, "[4/5] Bundling Qt6 frameworks...");
    if (!runShell(QString("macdeployqt \"%1\" -verbose=0 -hardened-runtime -always-overwrite").arg(stagedApp))) {
        report(progress, 55, "  WARN: macdeployqt reported errors (may be non-fatal)");
    }
    runShell(QString("codesign --force --deep --sign - \"%1\"").arg(stagedApp));
    report(progress, 65, "  Codesigned (ad-hoc)");

    // ── Create DMG ────────────────────────────────────────────────────────────
    report(progress, 70, "[5/5] Creating DMG...");
    QDir().mkpath(outputDir);
    QString dmgPath  = outputDir + "/" + outputFilename(m);
    QString volName  = m.app.publisher + " " + m.app.name + " " + m.app.version;
    QString bgImage  = projectDir + "/resources/splash.png";

    if (!createDmg(stagedApp, volName, dmgPath, bgImage)) {
        if (errOut) *errOut = "Failed to create DMG.";
        QDir(tmpDir).removeRecursively();
        return false;
    }

    report(progress, 95, "  DMG: " + dmgPath);
    QDir(tmpDir).removeRecursively();

    report(progress, 100, "=== macOS Build Complete! ===");
    return true;
}

// ── Private helpers ───────────────────────────────────────────────────────────
bool MacOsBackend::injectManifest(const QString &runtimeApp,
                                   const Manifest &m,
                                   const QString  &projectDir) const
{
    Q_UNUSED(projectDir)
    QString resDir = runtimeApp + "/Contents/Resources";
    QDir().mkpath(resDir);

    // Save manifest as manifest.mis inside the runtime bundle
    QString err;
    return m.save(resDir + "/manifest.mis", &err);
}

bool MacOsBackend::createDmg(const QString &appPath,
                              const QString &volName,
                              const QString &dmgPath,
                              const QString &bgImage) const
{
    // Try create-dmg first (polished layout), fall back to hdiutil
    QProcess which;
    which.start("which", {"create-dmg"});
    which.waitForFinished(3000);
    bool hasCreateDmg = (which.exitCode() == 0);

    QString cmd;
    if (hasCreateDmg && QFile::exists(bgImage)) {
        cmd = QString(
            "create-dmg"
            " --volname \"%1\""
            " --background \"%2\""
            " --window-size 600 420"
            " --icon-size 128"
            " --icon \"Mcaster1Installer.app\" 300 280"
            " --hide-extension \"Mcaster1Installer.app\""
            " --no-internet-enable"
            " \"%3\" \"%4/\"")
            .arg(volName, bgImage, dmgPath, QFileInfo(appPath).absolutePath());
    } else {
        // hdiutil fallback
        QString stagingDir = QFileInfo(appPath).absolutePath();
        cmd = QString(
            "hdiutil create -volname \"%1\""
            " -srcfolder \"%2\""
            " -ov -format UDZO"
            " \"%3\"")
            .arg(volName, stagingDir, dmgPath);
    }
    return runShell(cmd);
}

bool MacOsBackend::runShell(const QString &cmd, QString *out) const
{
    QProcess p;
    p.start("/bin/bash", {"-c", cmd});
    p.waitForFinished(120000);  // 2-minute timeout
    if (out) *out = QString::fromUtf8(p.readAllStandardOutput());
    return p.exitCode() == 0;
}
