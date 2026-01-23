/*
 * miscc.cpp — Mcaster1 Install Spec Compiler (CLI)
 *
 * Usage:
 *   miscc --build-file=<path.mis> [options]
 *
 * Options:
 *   --build-file=<path>        .mis project file to compile (required)
 *   --platform=<p>             Build only this platform: macos|windows|linux|all
 *   --output-dir=<path>        Output directory (default: same dir as .mis file)
 *   --profile=<id>             Apply a saved builder profile by id or display name
 *   --signing-id=<identity>    Override macOS signing identity
 *   --pfx=<path>               Override Windows PFX certificate path
 *   --no-sign                  Skip all code signing
 *   --notarize                 Force macOS notarization (implies signing)
 *   --dry-run                  Validate only; do not build
 *   --validate                 Alias for --dry-run
 *   --verbose                  Extra progress detail
 *   --list-platforms           List platforms in the .mis file then exit
 *   --version                  Print version and exit
 *   --help / -h                Print this help and exit
 *
 * Exit codes:
 *   0  — success (all requested builds completed)
 *   1  — validation failure (bad .mis file or missing prerequisites)
 *   2  — build failure (backend reported an error)
 *   3  — usage error (bad arguments)
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>
#include <QDebug>

#include <cstdio>
#include <cstring>

#ifdef Q_OS_UNIX
#  include <unistd.h>   // isatty()
#else
#  include <io.h>
#  define isatty _isatty
#endif

#include "../manifest/Manifest.h"
#include "../studio/BuilderProfile.h"

// ── Platform backend includes (platform-guarded) ──────────────────────────────
#ifdef Q_OS_MAC
#  include "../backends/MacOsBackend.h"
#endif
#ifdef Q_OS_WIN
#  include "../backends/WindowsBackend.h"
#endif
#ifdef Q_OS_LINUX
#  include "../backends/LinuxBackend.h"
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Version
// ─────────────────────────────────────────────────────────────────────────────
static constexpr const char *kVersion = "1.0.0";
static constexpr const char *kToolName = "miscc";

// ─────────────────────────────────────────────────────────────────────────────
// ANSI colour helpers
// ─────────────────────────────────────────────────────────────────────────────
static bool gColorEnabled = false;

static void initColor()
{
    gColorEnabled = isatty(STDOUT_FILENO) != 0;
}

static const char *col(const char *ansi)
{
    return gColorEnabled ? ansi : "";
}

static const char *cReset()  { return col("\033[0m");  }
static const char *cBold()   { return col("\033[1m");  }
static const char *cGreen()  { return col("\033[32m"); }
static const char *cYellow() { return col("\033[33m"); }
static const char *cRed()    { return col("\033[31m"); }
static const char *cCyan()   { return col("\033[36m"); }
static const char *cGray()   { return col("\033[90m"); }

// ─────────────────────────────────────────────────────────────────────────────
// Logging helpers (all output goes to stdout; errors also to stderr)
// ─────────────────────────────────────────────────────────────────────────────
static void printInfo(const QString &msg)
{
    fprintf(stdout, "%s%s%s  %s\n",
            cBold(), kToolName, cReset(),
            msg.toLocal8Bit().constData());
    fflush(stdout);
}

static void printProgress(int pct, const QString &msg)
{
    fprintf(stdout, "%s%s%s  %s[%3d%%%s]%s %s\n",
            cGray(), kToolName, cReset(),
            cCyan(), pct, cReset(), cGray(),
            msg.toLocal8Bit().constData());
    fflush(stdout);
}

static void printSuccess(const QString &msg)
{
    fprintf(stdout, "%s%s%s  %s✓%s  %s\n",
            cBold(), kToolName, cReset(),
            cGreen(), cReset(),
            msg.toLocal8Bit().constData());
    fflush(stdout);
}

static void printWarn(const QString &msg)
{
    fprintf(stdout, "%s%s%s  %s⚠%s  %s\n",
            cBold(), kToolName, cReset(),
            cYellow(), cReset(),
            msg.toLocal8Bit().constData());
    fflush(stdout);
}

static void printError(const QString &msg)
{
    fprintf(stderr, "%s%s%s  %s✗%s  %s\n",
            cBold(), kToolName, cReset(),
            cRed(), cReset(),
            msg.toLocal8Bit().constData());
    fflush(stderr);
}

static void printSection(const QString &title)
{
    fprintf(stdout, "\n%s%s  ── %s%s\n",
            cBold(), kToolName,
            title.toLocal8Bit().constData(),
            cReset());
    fflush(stdout);
}

// ─────────────────────────────────────────────────────────────────────────────
// Build a single platform
// ─────────────────────────────────────────────────────────────────────────────
static int buildPlatform(const QString &platform,
                          Manifest       &manifest,
                          const QString  &projectDir,
                          const QString  &outputDir,
                          bool            verbose,
                          bool            dryRun)
{
    // Instantiate the correct backend
#ifdef Q_OS_MAC
    MacOsBackend   macBackend;
#endif
#ifdef Q_OS_WIN
    WindowsBackend winBackend;
#endif
#ifdef Q_OS_LINUX
    LinuxBackend   linBackend;
#endif

    BuildBackend *backend = nullptr;

#ifdef Q_OS_MAC
    if (platform == "macos")  backend = &macBackend;
#endif
#ifdef Q_OS_WIN
    if (platform == "windows") backend = &winBackend;
#endif
#ifdef Q_OS_LINUX
    if (platform == "linux")   backend = &linBackend;
#endif

    if (!backend) {
        printWarn(QString("Platform '%1' is not supported on this host OS — skipping.").arg(platform));
        return 0;  // Not a hard error — user may be building subset on wrong host
    }

    printSection(QString("Building %1").arg(backend->displayName()));

    // ── Validate ──────────────────────────────────────────────────────────────
    const QStringList issues = backend->validate(manifest, projectDir);
    for (const QString &issue : issues) {
        if (issue.startsWith("WARNING:", Qt::CaseInsensitive))
            printWarn(issue);
        else
            printError(issue);
    }

    const bool hasErrors = std::any_of(issues.begin(), issues.end(), [](const QString &s) {
        return !s.startsWith("WARNING:", Qt::CaseInsensitive);
    });

    if (hasErrors) {
        printError(QString("Validation failed for '%1'.").arg(platform));
        return 1;
    }

    if (dryRun) {
        printSuccess(QString("[DRY-RUN] Validation passed for '%1'.").arg(platform));
        return 0;
    }

    // ── Build ─────────────────────────────────────────────────────────────────
    QElapsedTimer timer;
    timer.start();

    QString errMsg;
    const bool ok = backend->build(manifest, projectDir, outputDir,
        [&verbose](int pct, const QString &msg) {
            if (verbose || pct == 0 || pct == 100 || pct % 10 == 0)
                printProgress(pct, msg);
        },
        &errMsg);

    const qint64 elapsedMs = timer.elapsed();
    const QString elapsed  = QString("%1.%2s")
        .arg(elapsedMs / 1000)
        .arg((elapsedMs % 1000) / 100);

    if (!ok) {
        printError(QString("Build failed for '%1': %2").arg(platform, errMsg));
        return 2;
    }

    const QString artifact = QDir(outputDir).filePath(backend->outputFilename(manifest));
    printSuccess(QString("Wrote: %1  %2(%3)%4")
        .arg(artifact, cGray(), elapsed, cReset()));
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    // Headless Qt application — no GUI, no display required
    QCoreApplication app(argc, argv);
    app.setApplicationName(kToolName);
    app.setApplicationVersion(kVersion);

    initColor();

    // ── Argument parser ───────────────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Mcaster1 Install Spec Compiler — builds installer packages from .mis project files.");

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption optBuildFile(
        QStringList{"build-file", "f"},
        "Path to the .mis project file (required).",
        "path");

    QCommandLineOption optPlatform(
        QStringList{"platform", "p"},
        "Build only this platform: macos | windows | linux | all  (default: all targets in .mis file).",
        "platform", "all");

    QCommandLineOption optOutputDir(
        QStringList{"output-dir", "o"},
        "Output directory for installer packages (default: same directory as .mis file).",
        "dir");

    QCommandLineOption optProfile(
        "profile",
        "Apply a saved builder profile by id or display name.",
        "id");

    QCommandLineOption optSigningId(
        "signing-id",
        "Override macOS code-signing identity (e.g. 'Developer ID Application: ACME Corp (XXXXXXX)').",
        "identity");

    QCommandLineOption optPfx(
        "pfx",
        "Override Windows Authenticode certificate path (.pfx).",
        "path");

    QCommandLineOption optNoSign(
        "no-sign",
        "Skip all code signing.");

    QCommandLineOption optNotarize(
        "notarize",
        "Force macOS notarization (requires --signing-id or a profile with a signing identity).");

    QCommandLineOption optDryRun(
        QStringList{"dry-run", "n", "validate"},
        "Validate only; do not run the build pipeline.");

    QCommandLineOption optVerbose(
        "verbose",     // no short form — Qt reserves -v for --version
        "Print all progress steps (not just 10% intervals).");

    QCommandLineOption optListPlatforms(
        "list-platforms",
        "List target platforms declared in the .mis file, then exit.");

    QCommandLineOption optListProfiles(
        "list-profiles",
        "List all saved builder profiles, then exit.");

    parser.addOption(optBuildFile);
    parser.addOption(optPlatform);
    parser.addOption(optOutputDir);
    parser.addOption(optProfile);
    parser.addOption(optSigningId);
    parser.addOption(optPfx);
    parser.addOption(optNoSign);
    parser.addOption(optNotarize);
    parser.addOption(optDryRun);
    parser.addOption(optVerbose);
    parser.addOption(optListPlatforms);
    parser.addOption(optListProfiles);

    parser.process(app);

    const bool verbose      = parser.isSet(optVerbose);
    const bool dryRun       = parser.isSet(optDryRun);
    const bool noSign       = parser.isSet(optNoSign);
    const bool doNotarize   = parser.isSet(optNotarize);

    // ── --list-profiles (no build-file needed) ────────────────────────────────
    if (parser.isSet(optListProfiles)) {
        BuilderProfileManager mgr;
        if (!mgr.load() || mgr.profiles().isEmpty()) {
            printInfo("No builder profiles saved.");
            return 0;
        }
        printSection("Saved Builder Profiles");
        for (const BuilderProfile &p : mgr.profiles()) {
            fprintf(stdout, "  %s%-36s%s  %s\n",
                    cBold(),  p.id.toLocal8Bit().constData(), cReset(),
                    p.displayName.toLocal8Bit().constData());
            if (!p.company.isEmpty())
                fprintf(stdout, "    company:     %s\n", p.company.toLocal8Bit().constData());
            if (!p.macosSigningId.isEmpty())
                fprintf(stdout, "    macos-sign:  %s\n", p.macosSigningId.toLocal8Bit().constData());
            if (!p.windowsSigningCert.isEmpty())
                fprintf(stdout, "    win-pfx:     %s\n", p.windowsSigningCert.toLocal8Bit().constData());
            if (!p.linuxGpgKeyId.isEmpty())
                fprintf(stdout, "    gpg-key:     %s\n", p.linuxGpgKeyId.toLocal8Bit().constData());
            fprintf(stdout, "\n");
        }
        return 0;
    }

    // ── --build-file (required for everything else) ────────────────────────────
    if (!parser.isSet(optBuildFile)) {
        printError("--build-file=<path> is required.");
        fprintf(stderr, "Run '%s --help' for usage.\n", kToolName);
        return 3;
    }

    const QString buildFilePath = QFileInfo(parser.value(optBuildFile)).absoluteFilePath();
    if (!QFile::exists(buildFilePath)) {
        printError(QString("Project file not found: %1").arg(buildFilePath));
        return 1;
    }

    // ── Load manifest ─────────────────────────────────────────────────────────
    printInfo(QString("Loading %1%2%3")
              .arg(cBold(), QFileInfo(buildFilePath).fileName(), cReset()));

    Manifest manifest;
    QString  loadErr;
    if (!manifest.load(buildFilePath, &loadErr)) {
        printError(QString("Failed to parse .mis file: %1").arg(loadErr));
        return 1;
    }

    // ── Manifest-level validation ──────────────────────────────────────────────
    const QStringList misIssues = manifest.validate();
    for (const QString &issue : misIssues) {
        if (issue.startsWith("WARNING:", Qt::CaseInsensitive))
            printWarn(issue);
        else
            printError(issue);
    }
    const bool hasMisErrors = std::any_of(misIssues.begin(), misIssues.end(), [](const QString &s) {
        return !s.startsWith("WARNING:", Qt::CaseInsensitive);
    });
    if (hasMisErrors) {
        printError("Project file has validation errors. Fix them before building.");
        return 1;
    }

    // ── --list-platforms ───────────────────────────────────────────────────────
    if (parser.isSet(optListPlatforms)) {
        printSection("Target platforms in project");
        for (const QString &t : manifest.targets)
            fprintf(stdout, "  • %s\n", t.toLocal8Bit().constData());
        return 0;
    }

    // ── Apply builder profile ──────────────────────────────────────────────────
    if (parser.isSet(optProfile)) {
        const QString profileId = parser.value(optProfile);
        BuilderProfileManager mgr;
        mgr.load();
        const BuilderProfile *prof = mgr.findById(profileId);
        if (!prof) {
            // Try by display name
            for (const BuilderProfile &p : mgr.profiles()) {
                if (p.displayName.compare(profileId, Qt::CaseInsensitive) == 0) {
                    prof = &p;
                    break;
                }
            }
        }
        if (!prof) {
            printError(QString("Builder profile not found: '%1'. Use --list-profiles to see available profiles.").arg(profileId));
            return 3;
        }
        prof->applyToManifest(manifest);
        printInfo(QString("Applied builder profile: %1").arg(prof->displayName));
    }

    // ── CLI overrides ──────────────────────────────────────────────────────────
    if (noSign) {
        manifest.signing.macosSigner  = QString();
        manifest.signing.macosNotarize = false;
        manifest.signing.winPfxPath   = QString();
        manifest.signing.linuxGpgKey  = QString();
        printInfo("Code signing disabled (--no-sign).");
    }

    if (parser.isSet(optSigningId)) {
        manifest.signing.macosSigner = parser.value(optSigningId);
        printInfo(QString("Signing identity override: %1").arg(manifest.signing.macosSigner));
    }

    if (parser.isSet(optPfx)) {
        manifest.signing.winPfxPath = parser.value(optPfx);
        printInfo(QString("Windows PFX override: %1").arg(manifest.signing.winPfxPath));
    }

    if (doNotarize) {
        manifest.signing.macosNotarize = true;
        printInfo("macOS notarization enabled (--notarize).");
    }

    // ── Determine platforms to build ──────────────────────────────────────────
    QStringList platforms;
    const QString platformOpt = parser.value(optPlatform).toLower();

    if (platformOpt == "all" || platformOpt.isEmpty()) {
        platforms = manifest.targets;
    } else {
        const QStringList valid = {"macos", "windows", "linux"};
        for (const QString &p : platformOpt.split(',', Qt::SkipEmptyParts)) {
            const QString pt = p.trimmed();
            if (!valid.contains(pt)) {
                printError(QString("Unknown platform '%1'. Valid values: macos, windows, linux, all.").arg(pt));
                return 3;
            }
            platforms.append(pt);
        }
    }

    if (platforms.isEmpty()) {
        printError("No target platforms to build. Add 'targets:' to your .mis file or use --platform=.");
        return 1;
    }

    // ── Output directory ───────────────────────────────────────────────────────
    const QString projectDir = QFileInfo(buildFilePath).absolutePath();
    QString outputDir = projectDir;
    if (parser.isSet(optOutputDir)) {
        outputDir = QDir(parser.value(optOutputDir)).absolutePath();
        if (!QDir().mkpath(outputDir)) {
            printError(QString("Cannot create output directory: %1").arg(outputDir));
            return 1;
        }
    }

    // ── Print build plan ───────────────────────────────────────────────────────
    printSection("Build plan");
    fprintf(stdout, "  Project:    %s%s%s\n",
            cBold(), buildFilePath.toLocal8Bit().constData(), cReset());
    fprintf(stdout, "  App:        %s %s\n",
            manifest.app.name.toLocal8Bit().constData(),
            manifest.app.version.toLocal8Bit().constData());
    fprintf(stdout, "  Publisher:  %s\n", manifest.app.publisher.toLocal8Bit().constData());
    fprintf(stdout, "  Platforms:  %s\n", platforms.join(", ").toLocal8Bit().constData());
    fprintf(stdout, "  Output:     %s\n", outputDir.toLocal8Bit().constData());
    fprintf(stdout, "  Sign:       %s\n", noSign ? "disabled" :
            (manifest.signing.macosSigner.isEmpty() ? "ad-hoc" :
             manifest.signing.macosSigner.toLocal8Bit().constData()));
    if (dryRun)
        fprintf(stdout, "  %s[DRY-RUN — validate only]%s\n", cYellow(), cReset());
    fprintf(stdout, "\n");

    // ── Run builds ────────────────────────────────────────────────────────────
    int succeeded = 0;
    int failed    = 0;

    QElapsedTimer totalTimer;
    totalTimer.start();

    for (const QString &platform : platforms) {
        const int rc = buildPlatform(platform, manifest, projectDir, outputDir, verbose, dryRun);
        if (rc == 0)
            ++succeeded;
        else
            ++failed;
    }

    // ── Summary ───────────────────────────────────────────────────────────────
    const qint64 totalMs = totalTimer.elapsed();
    const QString totalElapsed = QString("%1.%2s")
        .arg(totalMs / 1000)
        .arg((totalMs % 1000) / 100);

    printSection("Summary");
    if (failed == 0) {
        printSuccess(QString("%1 platform(s) built successfully in %2")
                     .arg(succeeded)
                     .arg(totalElapsed));
    } else {
        if (succeeded > 0)
            printWarn(QString("%1 succeeded, %2 failed  (total: %3)")
                      .arg(succeeded).arg(failed).arg(totalElapsed));
        else
            printError(QString("All %1 platform build(s) failed  (total: %2)")
                       .arg(failed).arg(totalElapsed));
    }

    return failed > 0 ? 2 : 0;
}
