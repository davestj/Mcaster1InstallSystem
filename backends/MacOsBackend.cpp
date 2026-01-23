/*
 * MacOsBackend.cpp — macOS DMG installer build pipeline
 *
 * Homebrew artifact baking strategy:
 *   1. otool -L <binary> → collect lines starting with /opt/homebrew or /usr/local
 *   2. For each unique dylib: cp dylib → Contents/Frameworks/
 *   3. install_name_tool -id @executable_path/../Frameworks/<name> <Frameworks/name>
 *   4. install_name_tool -change <old-abs-path> @executable_path/../Frameworks/<name> <binary>
 *   5. Repeat step 4 for every Mach-O file in the bundle (dylibs reference each other)
 *
 * dylibbundler integration (preferred when available):
 *   dylibbundler -od -b -x <binary> -d <appBundle>/Contents/Frameworks/
 *   This handles transitive dependencies automatically.
 */

#include "MacOsBackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QSysInfo>
#include <QDateTime>
#include <QSet>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

// ── Static: Homebrew dep manifest for Mcaster1DNAS ───────────────────────────
QList<HomebrewDep> MacOsBackend::mcaster1dnasDeps()
{
    return {
        // Audio codecs
        { "libogg",    false, true,  "Ogg container format (base for Vorbis/Theora)" },
        { "libvorbis", false, true,  "Vorbis audio codec"                             },
        { "theora",    false, false, "Theora video codec (optional stream support)"   },
        { "speex",     false, false, "Speex voice codec (optional)"                   },

        // Network & security (keg-only — not in standard PATH)
        { "openssl@3", true,  true,  "TLS/SSL encryption (keg-only)"                 },
        { "curl",      true,  true,  "HTTP relay / libcurl (keg-only)"               },

        // XML/XSLT (keg-only — macOS ships incompatible older versions)
        { "libxml2",   true,  true,  "XML config parser (keg-only)"                  },
        { "libxslt",   true,  true,  "XSLT admin interface renderer (keg-only)"      },

        // Config
        { "libyaml",   false, true,  "YAML configuration file parser"                },

        // Build toolchain (optional — only needed to build from source)
        { "autoconf",  false, false, "Build system (source builds only)"             },
        { "automake",  false, false, "Build system (source builds only)"             },
        { "libtool",   false, false, "Build system (source builds only)"             },
        { "pkg-config",false, false, "Build system (source builds only)"             },
    };
}

// ── Static: Homebrew parallel dep checker ─────────────────────────────────────
// Uses QtConcurrent::mapped() to probe all formulas in parallel.
// Each probe runs `brew --prefix <formula>` in its own QProcess.
QMap<QString, QString> MacOsBackend::checkHomebrewDeps(
    const QList<HomebrewDep> &deps,
    const ProgressFn         &progress,
    QStringList              *missing)
{
    QMap<QString, QString> result;

    // ── Build list of (formula, kegOnly) pairs to probe ──────────────────────
    QList<QPair<QString,bool>> toProbe;
    for (const HomebrewDep &d : deps)
        toProbe.append({d.formula, d.kegOnly});

    // ── Parallel probe using QtConcurrent::mapped ─────────────────────────────
    // Lambda: (formula, kegOnly) → (formula, prefix-or-empty)
    auto probe = [](const QPair<QString,bool> &entry) -> QPair<QString,QString> {
        QProcess p;
        if (entry.second) {
            // keg-only: brew --prefix <formula>
            p.start("brew", {"--prefix", entry.first});
        } else {
            // regular formula: brew list <formula> to check if installed
            p.start("brew", {"list", "--formula", entry.first});
        }
        p.waitForFinished(8000);
        if (p.exitCode() != 0)
            return {entry.first, QString()};   // not installed
        if (entry.second) {
            // keg-only: return the prefix path
            return {entry.first,
                    QString::fromUtf8(p.readAllStandardOutput()).trimmed()};
        }
        return {entry.first, "installed"};
    };

    QFuture<QPair<QString,QString>> future =
        QtConcurrent::mapped(toProbe, probe);
    future.waitForFinished();

    for (const auto &kv : future.results())
        result[kv.first] = kv.second;

    // ── Classify results ──────────────────────────────────────────────────────
    if (missing) {
        missing->clear();
        for (const HomebrewDep &d : deps) {
            if (d.required && result.value(d.formula).isEmpty())
                *missing << d.formula;
        }
    }

    if (progress) {
        int ok = 0, warn = 0;
        for (const HomebrewDep &d : deps) {
            if (!result.value(d.formula).isEmpty()) ++ok;
            else if (d.required) ++warn;
        }
        progress(0, QString("  Homebrew: %1/%2 deps present (%3 required missing)")
                    .arg(ok).arg(deps.size()).arg(warn));
    }

    return result;
}

// ── Static: brew --prefix (cached) ────────────────────────────────────────────
QString MacOsBackend::homebrewPrefix()
{
    static QString cached;
    if (!cached.isEmpty()) return cached;
    QProcess p;
    p.start("brew", {"--prefix"});
    p.waitForFinished(5000);
    cached = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if (cached.isEmpty()) cached = "/opt/homebrew";   // Apple Silicon default
    return cached;
}

QString MacOsBackend::formulaPrefix(const QString &formula)
{
    QProcess p;
    p.start("brew", {"--prefix", formula});
    p.waitForFinished(5000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

// ── validate ──────────────────────────────────────────────────────────────────
QStringList MacOsBackend::validate(const Manifest &m, const QString &projectDir) const
{
    Q_UNUSED(projectDir)
    QStringList issues = m.validate();

    // macdeployqt
    QProcess p;
    p.start("which", {"macdeployqt"});
    p.waitForFinished(3000);
    if (p.exitCode() != 0)
        issues << "macdeployqt not found. Install Qt: brew install qt";

    // hdiutil (built-in)
    QProcess p2;
    p2.start("which", {"hdiutil"});
    p2.waitForFinished(3000);
    if (p2.exitCode() != 0)
        issues << "hdiutil not found (should always exist on macOS).";

    // runtime installer
    QString studioDir   = QCoreApplication::applicationDirPath();
    QString projectRoot = QFileInfo(studioDir + "/../../../../..").canonicalFilePath();
    QString runtimeApp  = projectRoot + "/runtime/build/Mcaster1Installer.app";
    if (!QDir(runtimeApp).exists()) {
        runtimeApp = QFileInfo(studioDir + "/../../runtime/build/Mcaster1Installer.app").canonicalFilePath();
    }
    if (!QDir(runtimeApp).exists())
        issues << QString("Runtime installer not found: %1\n  Run: make runtime").arg(runtimeApp);

    // Homebrew required deps
    ProgressFn nopProgress = nullptr;
    QStringList missing;
    checkHomebrewDeps(mcaster1dnasDeps(), nopProgress, &missing);
    for (const QString &f : missing)
        issues << QString("Required Homebrew formula not installed: %1  (brew install %1)").arg(f);

    return issues;
}

// ── outputFilename ─────────────────────────────────────────────────────────────
QString MacOsBackend::outputFilename(const Manifest &m) const
{
    QString arch = QSysInfo::currentCpuArchitecture() == "arm64" ? "arm64" : "x86_64";
    return QString("%1-%2-%3-macOS-%4-installer.dmg")
        .arg(m.app.publisher.isEmpty() ? "Mcaster1" : m.app.publisher)
        .arg(m.app.name.isEmpty()      ? "App"       : m.app.name)
        .arg(m.app.version.isEmpty()   ? "1.0.0"     : m.app.version)
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
    report(progress, 3, QString("App: %1 v%2  |  Publisher: %3")
           .arg(m.app.name, m.app.version, m.app.publisher));

    // ── [0] Homebrew dependency check (parallel) ──────────────────────────────
    report(progress, 5, "── [0/8] Checking Homebrew dependencies (parallel)…");
    const QList<HomebrewDep> deps = mcaster1dnasDeps();
    QStringList missingRequired;
    const QMap<QString,QString> depPrefixes =
        checkHomebrewDeps(deps, progress, &missingRequired);

    for (const HomebrewDep &d : deps) {
        const QString prefix = depPrefixes.value(d.formula);
        const QString status = prefix.isEmpty()
                               ? (d.required ? "MISSING (required)" : "not installed")
                               : (d.kegOnly  ? "OK (keg-only: " + prefix + ")" : "OK");
        report(progress, 5, QString("    %-16s  %2").arg(d.formula, status));
    }

    if (!missingRequired.isEmpty()) {
        // Warn but don't abort — user may have custom install paths
        report(progress, 5,
               QString("  WARNING: Missing required formulas: %1\n"
                       "  Artifact baking may be incomplete.")
               .arg(missingRequired.join(", ")));
    }

    // ── [1] Locate runtime installer ──────────────────────────────────────────
    report(progress, 8, "── [1/8] Locating runtime installer…");
    QString studioDir   = QCoreApplication::applicationDirPath();
    QString projectRoot = QFileInfo(studioDir + "/../../../../..").canonicalFilePath();
    QString runtimeApp  = projectRoot + "/runtime/build/Mcaster1Installer.app";
    if (!QDir(runtimeApp).exists())
        runtimeApp = QFileInfo(studioDir + "/../../runtime/build/Mcaster1Installer.app").canonicalFilePath();
    if (!QDir(runtimeApp).exists()) {
        if (errOut) *errOut = QString("Runtime installer not found: %1\n\nRun: make runtime").arg(runtimeApp);
        return false;
    }
    report(progress, 10, "  Runtime: " + runtimeApp);

    // ── [2] Create temp staging area ──────────────────────────────────────────
    report(progress, 12, "── [2/8] Setting up staging area…");
    const QString tmpDir    = QDir::tempPath() + "/mis_build_"
                              + QString::number(QDateTime::currentMSecsSinceEpoch());
    QDir().mkpath(tmpDir);
    const QString stagedApp = tmpDir + "/Mcaster1Installer.app";

    if (!runShell(QString("cp -R \"%1\" \"%2\"").arg(runtimeApp, stagedApp))) {
        if (errOut) *errOut = "Failed to copy runtime installer app.";
        QDir(tmpDir).removeRecursively();
        return false;
    }
    report(progress, 18, "  Staging: " + tmpDir);

    // ── [3] Inject manifest.mis ────────────────────────────────────────────────
    report(progress, 20, "── [3/8] Injecting project manifest…");
    if (!injectManifest(stagedApp, m, projectDir)) {
        if (errOut) *errOut = "Failed to inject manifest into runtime bundle.";
        QDir(tmpDir).removeRecursively();
        return false;
    }

    // ── [4] Inject payload ────────────────────────────────────────────────────
    report(progress, 25, "── [4/8] Injecting payload…");
    const QString payloadSrc = projectDir + "/payload";
    const QString payloadDst = stagedApp  + "/Contents/Resources/payload";
    if (QDir(payloadSrc).exists()) {
        runShell(QString("cp -R \"%1\" \"%2\"").arg(payloadSrc, payloadDst));
        report(progress, 32, QString("  Copied payload: %1").arg(payloadSrc));
    } else {
        report(progress, 32,
               "  WARN: No payload/ directory — installer will deploy an empty payload.");
    }

    // ── [5] HOMEBREW ARTIFACT BAKING ─────────────────────────────────────────
    report(progress, 35, "── [5/8] Baking Homebrew artifacts into payload bundles…");
    if (QDir(payloadDst).exists()) {
        QString bakeErr;
        if (!bakeHomebrewArtifacts(payloadDst, progress, &bakeErr)) {
            // Non-fatal: warn and continue
            report(progress, 45,
                   QString("  WARN: Homebrew artifact baking incomplete: %1\n"
                           "  App may require Homebrew on target machine.").arg(bakeErr));
        } else {
            report(progress, 45, "  Homebrew dylibs baked and install names fixed.");
        }
    } else {
        report(progress, 45, "  (No payload — skipping artifact bake)");
    }

    // ── [6] macdeployqt on runtime + codesign ────────────────────────────────
    report(progress, 48, "── [6/8] Bundling Qt6 frameworks (macdeployqt)…");
    if (!runShell(QString("macdeployqt \"%1\" -verbose=0 -hardened-runtime -always-overwrite")
                  .arg(stagedApp))) {
        report(progress, 55, "  WARN: macdeployqt reported errors (may be non-fatal).");
    } else {
        report(progress, 55, "  Qt6 frameworks bundled.");
    }

    // ── [7] Codesign ─────────────────────────────────────────────────────────
    report(progress, 58, "── [7/8] Codesigning…");
    const QString identity = m.signing.macosSigner.isEmpty()
                             ? QString("-")   // ad-hoc
                             : m.signing.macosSigner;
    if (!codesignApp(stagedApp, identity, progress)) {
        report(progress, 65, "  WARN: Codesign step reported errors.");
    }

    // ── [8] Create DMG ────────────────────────────────────────────────────────
    report(progress, 68, "── [8/8] Creating DMG…");
    QDir().mkpath(outputDir);
    const QString dmgPath = outputDir + "/" + outputFilename(m);
    const QString volName = QString("%1 %2 %3")
                            .arg(m.app.publisher, m.app.name, m.app.version);
    const QString bgImage = projectDir + "/resources/splash.png";

    if (!createDmg(stagedApp, volName, dmgPath, bgImage)) {
        if (errOut) *errOut = "Failed to create DMG.";
        QDir(tmpDir).removeRecursively();
        return false;
    }

    report(progress, 95, "  DMG created: " + dmgPath);
    QDir(tmpDir).removeRecursively();
    report(progress, 100, "=== macOS Build Complete! ===");
    return true;
}

// ── Private: injectManifest ────────────────────────────────────────────────────
bool MacOsBackend::injectManifest(const QString &runtimeApp,
                                   const Manifest &m,
                                   const QString  &projectDir) const
{
    Q_UNUSED(projectDir)
    const QString resDir = runtimeApp + "/Contents/Resources";
    QDir().mkpath(resDir);
    QString err;
    return m.save(resDir + "/manifest.mis", &err);
}

// ── Private: Homebrew artifact baking ─────────────────────────────────────────
bool MacOsBackend::bakeHomebrewArtifacts(const QString   &payloadDir,
                                          const ProgressFn &progress,
                                          QString          *errOut) const
{
    // Find all .app bundles inside payloadDir
    QDir pd(payloadDir);
    const QStringList appBundles = pd.entryList({"*.app"}, QDir::Dirs);

    // Also look one level deeper (payload/ may have sub-dirs)
    QStringList allApps;
    for (const QString &sub : appBundles)
        allApps << pd.absoluteFilePath(sub);

    // If none found at root, recurse one level
    if (allApps.isEmpty()) {
        for (const QString &sub : pd.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir sub2(pd.absoluteFilePath(sub));
            for (const QString &a : sub2.entryList({"*.app"}, QDir::Dirs))
                allApps << sub2.absoluteFilePath(a);
        }
    }

    if (allApps.isEmpty()) {
        // No .app bundles — check if there are plain Mach-O binaries to bake
        report(progress, 40,
               "  No .app bundles found in payload — skipping Homebrew artifact bake.");
        return true;
    }

    // Check if dylibbundler is available (preferred)
    QProcess which;
    which.start("which", {"dylibbundler"});
    which.waitForFinished(3000);
    const bool hasDylibBundler = (which.exitCode() == 0);

    bool allOk = true;
    const QString brew = homebrewPrefix();

    for (const QString &appBundle : allApps) {
        report(progress, 38, QString("  Baking: %1").arg(QFileInfo(appBundle).fileName()));

        const QString frameworksDir = appBundle + "/Contents/Frameworks";
        QDir().mkpath(frameworksDir);

        // Find the main executable
        // Conventionally: Contents/MacOS/<AppName>
        const QString macOsDir = appBundle + "/Contents/MacOS";
        const QStringList execs = QDir(macOsDir).entryList(
            QDir::Files | QDir::Executable);

        if (hasDylibBundler && !execs.isEmpty()) {
            // ── Use dylibbundler (handles transitive deps automatically) ──────
            const QString mainBin = macOsDir + "/" + execs.first();
            const QString cmd = QString(
                "dylibbundler -od -b"
                " -x \"%1\""
                " -d \"%2\""
                " -p @executable_path/../Frameworks/")
                .arg(mainBin, frameworksDir);

            QString cmdOut;
            if (!runShell(cmd, &cmdOut)) {
                report(progress, 40,
                       "    dylibbundler failed, falling back to manual baking.");
                allOk = false;
            } else {
                report(progress, 40,
                       QString("    dylibbundler: %1 dylibs bundled.")
                           .arg(QDir(frameworksDir).entryList({"*.dylib"}).size()));
                continue;   // next app bundle
            }
        }

        // ── Manual baking: otool -L + install_name_tool ───────────────────────
        // Step 1: collect all Homebrew dylib paths from all Mach-O files in the bundle
        const QStringList machOFiles = findMachOFiles(appBundle);
        QMap<QString, QString> dyMap;  // abs-path → Frameworks/name.dylib

        for (const QString &mo : machOFiles) {
            for (const QString &dep : collectHomebrewDylibs(mo)) {
                if (!dyMap.contains(dep)) {
                    const QString name = QFileInfo(dep).fileName();
                    dyMap[dep] = frameworksDir + "/" + name;
                }
            }
        }

        if (dyMap.isEmpty()) {
            report(progress, 40, "    No Homebrew dylibs found — bundle is already self-contained.");
            continue;
        }

        // Step 2: copy dylibs into Contents/Frameworks/
        for (auto it = dyMap.constBegin(); it != dyMap.constEnd(); ++it) {
            const QString &src = it.key();
            const QString &dst = it.value();
            if (!QFile::exists(dst)) {
                if (!QFile::copy(src, dst)) {
                    report(progress, 40,
                           QString("    WARN: Failed to copy %1 → %2").arg(src, dst));
                    allOk = false;
                    continue;
                }
                // Make writable so install_name_tool can patch it
                QFile::setPermissions(dst, QFile::ReadOwner | QFile::WriteOwner |
                                           QFile::ReadGroup | QFile::ReadOther);
            }
            // Step 3: fix the dylib's own id
            const QString newId = "@executable_path/../Frameworks/" + QFileInfo(dst).fileName();
            runShell(QString("install_name_tool -id \"%1\" \"%2\"").arg(newId, dst));
        }

        // Step 4: fix load commands in every Mach-O file in the bundle
        if (!fixInstallNames(appBundle, dyMap)) {
            report(progress, 42, "    WARN: Some install name fixes failed.");
            allOk = false;
        } else {
            report(progress, 42,
                   QString("    Fixed install names for %1 dylibs in %2.")
                       .arg(dyMap.size()).arg(QFileInfo(appBundle).fileName()));
        }
    }

    if (!allOk && errOut)
        *errOut = "One or more Homebrew artifact baking steps reported warnings.";
    return true;   // always continue — non-fatal
}

QStringList MacOsBackend::collectHomebrewDylibs(const QString &binary) const
{
    QStringList result;
    const QString brew = homebrewPrefix();

    QProcess p;
    p.start("otool", {"-L", binary});
    p.waitForFinished(10000);
    const QString output = QString::fromUtf8(p.readAllStandardOutput());

    static const QRegularExpression rx(
        R"(^\s+(/opt/homebrew[^\s]+|/usr/local[^\s]+)\s+\()",
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = rx.globalMatch(output);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const QString dep = m.captured(1);
        // Skip the binary referencing itself
        if (!dep.endsWith(QFileInfo(binary).fileName()))
            result << dep;
    }
    return result;
}

QStringList MacOsBackend::findMachOFiles(const QString &appBundle) const
{
    QStringList result;
    // Binary in MacOS/
    const QString macOsDir = appBundle + "/Contents/MacOS";
    for (const QString &f : QDir(macOsDir).entryList(QDir::Files))
        result << macOsDir + "/" + f;
    // Dylibs in Frameworks/
    const QString fwDir = appBundle + "/Contents/Frameworks";
    for (const QString &f : QDir(fwDir).entryList({"*.dylib"}, QDir::Files))
        result << fwDir + "/" + f;
    // Bundles in PlugIns/
    const QString piDir = appBundle + "/Contents/PlugIns";
    for (const QFileInfo &fi : QDir(piDir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString sub = fi.absoluteFilePath() + "/" + fi.fileName();
        if (QFile::exists(sub)) result << sub;
    }
    return result;
}

bool MacOsBackend::fixInstallNames(const QString &appBundle,
                                    const QMap<QString,QString> &dyMap) const
{
    bool ok = true;
    const QStringList machOFiles = findMachOFiles(appBundle);
    for (const QString &mo : machOFiles) {
        for (auto it = dyMap.constBegin(); it != dyMap.constEnd(); ++it) {
            const QString oldPath = it.key();
            const QString newName = "@executable_path/../Frameworks/"
                                    + QFileInfo(it.value()).fileName();
            const QString cmd = QString("install_name_tool -change \"%1\" \"%2\" \"%3\"")
                                .arg(oldPath, newName, mo);
            if (!runShell(cmd)) ok = false;
        }
    }
    return ok;
}

// ── Private: codesign ─────────────────────────────────────────────────────────
bool MacOsBackend::codesignApp(const QString &appPath,
                                const QString &identity,
                                const ProgressFn &progress) const
{
    // Step 1: deep sign (signs all nested dylibs, plugins, frameworks)
    const bool step1 = runShell(QString(
        "codesign --force --deep --sign \"%1\" \"%2\"")
        .arg(identity, appPath));
    report(progress, 63, step1 ? "  Step 1/2 codesign OK (deep)" : "  Step 1/2 codesign: errors");

    // Step 2: re-sign the main bundle (deep may have stripped entitlements)
    const bool step2 = runShell(QString(
        "codesign --force --sign \"%1\" \"%2\"")
        .arg(identity, appPath));
    report(progress, 66, step2 ? "  Step 2/2 codesign OK" : "  Step 2/2 codesign: errors");

    return step1 && step2;
}

// ── Private: createDmg ─────────────────────────────────────────────────────────
bool MacOsBackend::createDmg(const QString &appPath,
                              const QString &volName,
                              const QString &dmgPath,
                              const QString &bgImage) const
{
    QProcess which;
    which.start("which", {"create-dmg"});
    which.waitForFinished(3000);
    const bool hasCreateDmg = (which.exitCode() == 0);

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
            " --app-drop-link 450 280"
            " --no-internet-enable"
            " \"%3\" \"%4/\"")
            .arg(volName, bgImage, dmgPath, QFileInfo(appPath).absolutePath());
    } else {
        // hdiutil fallback (no background image, basic layout)
        const QString stagingDir = QFileInfo(appPath).absolutePath();
        cmd = QString(
            "hdiutil create -volname \"%1\""
            " -srcfolder \"%2\""
            " -ov -format UDZO"
            " \"%3\"")
            .arg(volName, stagingDir, dmgPath);
    }
    return runShell(cmd, nullptr, 300000);   // 5-min timeout for large payloads
}

// ── Private: runShell ─────────────────────────────────────────────────────────
bool MacOsBackend::runShell(const QString &cmd, QString *out, int timeoutMs) const
{
    QProcess p;
    p.start("/bin/bash", {"-c", cmd});
    p.waitForFinished(timeoutMs);
    if (out) *out = QString::fromUtf8(p.readAllStandardOutput());
    return p.exitCode() == 0;
}
