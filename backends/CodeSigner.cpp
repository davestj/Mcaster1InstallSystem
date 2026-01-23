/*
 * CodeSigner.cpp — Code signing implementation
 *
 * Tool requirements by platform:
 *   macOS  : codesign (Xcode CLI), xcrun notarytool (Xcode 13+), productsign
 *   Windows: signtool.exe (Windows SDK) or osslsigncode (cross-platform)
 *   Linux  : dpkg-sig / debsigs (for .deb), rpm + gpg (for .rpm),
 *            gpg (for AppImage detached signature)
 *
 * osslsigncode cross-compile signing from macOS/Linux:
 *   brew install osslsigncode
 */

#include "CodeSigner.h"

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QRegularExpression>

// ── Private helper ────────────────────────────────────────────────────────────

bool CodeSigner::runTool(const QString &program, const QStringList &args,
                          const ProgressFn &progress, QString *errOut,
                          int timeoutMs)
{
    if (progress) progress(0, QString("  Running: %1 %2").arg(program, args.join(' ')));

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(program, args);
    if (!p.waitForStarted(5000)) {
        if (errOut) *errOut = QString("Could not start '%1': %2").arg(program, p.errorString());
        return false;
    }
    p.waitForFinished(timeoutMs);

    const QString output = QString::fromLocal8Bit(p.readAll()).trimmed();
    if (p.exitCode() != 0) {
        if (errOut) *errOut = QString("%1 failed (exit %2):\n%3")
                                  .arg(program).arg(p.exitCode()).arg(output);
        return false;
    }
    if (progress && !output.isEmpty())
        progress(50, "  " + output);
    return true;
}

// ── Utility ───────────────────────────────────────────────────────────────────

bool CodeSigner::toolAvailable(const QString &tool)
{
#ifdef Q_OS_WIN
    QProcess p;
    p.start("where", {tool});
    p.waitForFinished(3000);
    return p.exitCode() == 0;
#else
    QProcess p;
    p.start("which", {tool});
    p.waitForFinished(3000);
    return p.exitCode() == 0;
#endif
}

QString CodeSigner::windowsSigningTool()
{
#ifdef Q_OS_WIN
    // Prefer signtool.exe (from Windows SDK) on Windows
    if (toolAvailable("signtool")) return "signtool";
#endif
    if (toolAvailable("osslsigncode")) return "osslsigncode";
    return {};
}

QStringList CodeSigner::availableTools()
{
    QStringList result;

    // macOS tools
    if (toolAvailable("codesign"))       result << "codesign (macOS .app signing)";
    if (toolAvailable("productsign"))    result << "productsign (macOS .pkg signing)";

    // notarytool lives inside xcrun
    {
        QProcess p;
        p.start("xcrun", {"notarytool", "--version"});
        p.waitForFinished(5000);
        if (p.exitCode() == 0) result << "xcrun notarytool (macOS notarization)";
    }

    // Windows cross-signing
    if (toolAvailable("osslsigncode")) result << "osslsigncode (Windows .exe/.msi cross-signing)";
#ifdef Q_OS_WIN
    if (toolAvailable("signtool"))     result << "signtool (Windows .exe/.msi native signing)";
#endif

    // Linux
    if (toolAvailable("dpkg-sig"))     result << "dpkg-sig (.deb signing)";
    if (toolAvailable("debsigs"))      result << "debsigs (.deb signing)";
    if (toolAvailable("rpmsign"))      result << "rpmsign (.rpm signing)";
    if (toolAvailable("gpg"))          result << "gpg (GPG key operations, AppImage signing)";

    if (result.isEmpty())
        result << "(no code-signing tools detected)";

    return result;
}

// ── macOS ─────────────────────────────────────────────────────────────────────

bool CodeSigner::signMacosApp(const QString &appPath,
                               const QString &identity,
                               const QString &entitlementsPath,
                               bool hardened,
                               QString *errOut,
                               const ProgressFn &progress)
{
    if (!toolAvailable("codesign")) {
        if (errOut) *errOut = "codesign not found. Install Xcode Command Line Tools.";
        return false;
    }

    if (progress) progress(10, "Signing " + appPath + "…");

    QStringList args = {"--force", "--sign", identity, "--deep"};
    if (hardened)
        args << "--options" << "runtime";
    if (!entitlementsPath.isEmpty())
        args << "--entitlements" << entitlementsPath;
    args << "--timestamp";   // embed trusted timestamp in signature
    args << appPath;

    if (!runTool("codesign", args, progress, errOut))
        return false;

    // Verify the signature
    if (progress) progress(80, "Verifying signature…");
    QStringList verArgs = {"--verify", "--deep", "--strict", "--verbose=2", appPath};
    return runTool("codesign", verArgs, progress, errOut);
}

bool CodeSigner::notarizeMacos(const QString &packagePath,
                                const QString &appleId,
                                const QString &teamId,
                                const QString &appPassword,
                                QString *errOut,
                                const ProgressFn &progress)
{
    if (progress) progress(5, "Submitting to Apple notary service…");

    // Submit for notarization
    QStringList submitArgs = {
        "notarytool", "submit", packagePath,
        "--apple-id",   appleId,
        "--team-id",    teamId,
        "--password",   appPassword,
        "--wait",        // block until Apple finishes (up to ~30min)
        "--output-format", "json"
    };

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start("xcrun", submitArgs);
    if (!p.waitForStarted(5000)) {
        if (errOut) *errOut = "xcrun notarytool failed to start.";
        return false;
    }

    if (progress) progress(20, "Waiting for Apple notary service…");
    p.waitForFinished(30 * 60 * 1000);  // 30 min timeout

    const QString output = QString::fromLocal8Bit(p.readAll());
    if (p.exitCode() != 0) {
        if (errOut) *errOut = "Notarization failed:\n" + output;
        return false;
    }

    if (progress) progress(80, "Stapling notarization ticket…");

    // Staple the notarization ticket to the package
    if (!runTool("xcrun", {"stapler", "staple", packagePath}, progress, errOut))
        return false;

    if (progress) progress(100, "Notarization complete: " + packagePath);
    return true;
}

bool CodeSigner::signMacosPkg(const QString &pkgPath,
                               const QString &identity,
                               QString *errOut,
                               const ProgressFn &progress)
{
    if (!toolAvailable("productsign")) {
        if (errOut) *errOut = "productsign not found. Install Xcode Command Line Tools.";
        return false;
    }

    if (progress) progress(10, "Signing .pkg…");

    const QString outPath = pkgPath + ".signed.pkg";
    QStringList args = {"--sign", identity, pkgPath, outPath};
    if (!runTool("productsign", args, progress, errOut))
        return false;

    // Replace original with signed version
    QFile::remove(pkgPath);
    QFile::rename(outPath, pkgPath);

    if (progress) progress(100, "Package signed: " + pkgPath);
    return true;
}

// ── Windows ───────────────────────────────────────────────────────────────────

bool CodeSigner::signWindowsExe(const QString &exePath,
                                 const QString &pfxPath,
                                 const QString &pfxPassword,
                                 const QString &timestampUrl,
                                 const QString &description,
                                 QString *errOut,
                                 const ProgressFn &progress)
{
    const QString tool = windowsSigningTool();
    if (tool.isEmpty()) {
        if (errOut) *errOut =
            "No Windows signing tool found.\n"
            "• macOS/Linux: brew install osslsigncode\n"
            "• Windows:     install Windows SDK (includes signtool.exe)";
        return false;
    }

    if (progress) progress(10, QString("Signing with %1…").arg(tool));

    QStringList args;

    if (tool == "osslsigncode") {
        // Cross-platform PFX signing
        args << "sign"
             << "-pkcs12"  << pfxPath
             << "-t"       << timestampUrl;
        if (!pfxPassword.isEmpty())
            args << "-pass" << pfxPassword;
        if (!description.isEmpty())
            args << "-n" << description;

        const QString signedPath = exePath + ".signed";
        args << "-in" << exePath << "-out" << signedPath;

        if (!runTool("osslsigncode", args, progress, errOut))
            return false;

        QFile::remove(exePath);
        QFile::rename(signedPath, exePath);

    } else {
        // signtool.exe (Windows native)
        args << "sign"
             << "/f"   << pfxPath
             << "/t"   << timestampUrl
             << "/fd"  << "sha256";
        if (!pfxPassword.isEmpty())
            args << "/p" << pfxPassword;
        if (!description.isEmpty())
            args << "/d" << description;
        args << exePath;

        if (!runTool("signtool", args, progress, errOut))
            return false;
    }

    if (progress) progress(100, "Signed: " + exePath);
    return true;
}

// ── Linux ─────────────────────────────────────────────────────────────────────

bool CodeSigner::signDebPackage(const QString &debPath,
                                 const QString &gpgKeyId,
                                 QString *errOut,
                                 const ProgressFn &progress)
{
    if (progress) progress(10, "Signing .deb package…");

    // Prefer dpkg-sig over debsigs (dpkg-sig is more widely supported)
    if (toolAvailable("dpkg-sig")) {
        QStringList args = {"--sign", "builder", "-k", gpgKeyId, debPath};
        return runTool("dpkg-sig", args, progress, errOut);
    }
    if (toolAvailable("debsigs")) {
        QStringList args = {"--sign=origin", QString("-k%1").arg(gpgKeyId), debPath};
        return runTool("debsigs", args, progress, errOut);
    }

    // Fallback: raw gpg detached signature alongside the .deb
    if (toolAvailable("gpg")) {
        if (progress) progress(15, "Using gpg detached signature (dpkg-sig/debsigs not found)…");
        QStringList args = {"--default-key", gpgKeyId,
                            "--armor", "--detach-sign", debPath};
        return runTool("gpg", args, progress, errOut);
    }

    if (errOut) *errOut = "No .deb signing tool found (tried dpkg-sig, debsigs, gpg).";
    return false;
}

bool CodeSigner::signRpmPackage(const QString &rpmPath,
                                 const QString &gpgKeyId,
                                 QString *errOut,
                                 const ProgressFn &progress)
{
    if (progress) progress(10, "Signing .rpm package…");

    if (toolAvailable("rpmsign")) {
        QStringList args = {"--addsign",
                            QString("--define=_gpg_name %1").arg(gpgKeyId),
                            rpmPath};
        return runTool("rpmsign", args, progress, errOut);
    }
    if (toolAvailable("rpm")) {
        QStringList args = {"--addsign",
                            "--define", QString("_gpg_name %1").arg(gpgKeyId),
                            rpmPath};
        return runTool("rpm", args, progress, errOut);
    }

    if (errOut) *errOut = "rpm/rpmsign not found.";
    return false;
}

bool CodeSigner::signAppImage(const QString &appImagePath,
                               const QString &gpgKeyId,
                               QString *errOut,
                               const ProgressFn &progress)
{
    if (!toolAvailable("gpg")) {
        if (errOut) *errOut = "gpg not found for AppImage signing.";
        return false;
    }

    if (progress) progress(10, "Creating GPG detached signature for AppImage…");

    QStringList args = {"--default-key", gpgKeyId, "--armor",
                        "--detach-sign", appImagePath};
    if (!runTool("gpg", args, progress, errOut))
        return false;

    if (progress) progress(100, "AppImage signature: " + appImagePath + ".asc");
    return true;
}
