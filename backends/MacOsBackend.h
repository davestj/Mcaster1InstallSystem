#pragma once
#include "BuildBackend.h"

#include <QStringList>
#include <QMap>

/*
 * MacOsBackend — builds a macOS DMG installer.
 *
 * Build pipeline:
 *   1. Validate Homebrew dependency manifest (parallel probes via QtConcurrent)
 *   2. Validate payload directory, tool availability
 *   3. Copy runtime Mcaster1Installer.app into a temp staging area
 *   4. Inject manifest.mis into runtime wizard bundle (Resources/)
 *   5. Inject payload/ into runtime Resources/payload/
 *      5a. BAKE HOMEBREW ARTIFACTS: scan payload .app bundles for Homebrew dylib
 *          dependencies (otool -L), copy into Contents/Frameworks/, fix load
 *          commands with install_name_tool (portable @executable_path refs)
 *   6. Run macdeployqt on the runtime app (Qt6 frameworks)
 *   7. Codesign (identity from manifest.signing.macosSigner or ad-hoc)
 *   8. Optionally notarize (xcrun notarytool, if manifest.signing.macosNotarize)
 *   9. Create DMG (create-dmg → hdiutil fallback)
 *
 * Homebrew parallel build support:
 *   checkHomebrewDeps() probes all required formulas concurrently and returns a
 *   map of formula → installed/prefix status. Missing deps are reported but only
 *   fail the build if they are marked required=true.
 *
 * Output: <output-dir>/<Publisher>-<Name>-<Version>-macOS-<arch>-installer.dmg
 *         or     ...     -macOS-universal-installer.dmg  (if universalBinary=true)
 */

// ── Homebrew dependency descriptor ───────────────────────────────────────────
struct HomebrewDep {
    QString formula;       // e.g. "openssl@3", "libxml2", "libyaml"
    bool    kegOnly;       // true → brew --prefix <formula> returns isolated path
    bool    required;      // true → build aborts if missing; false → warning only
    QString provides;      // human description e.g. "TLS/SSL encryption"
};

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

    // ── Homebrew section ──────────────────────────────────────────────────────

    // Returns the canonical list of Homebrew formulas required to run Mcaster1DNAS
    // and any audio-codec applications. Extended by manifest's build.homebrewDeps.
    static QList<HomebrewDep> mcaster1dnasDeps();

    // Probe each formula's install status in parallel.
    // Returns map: formula → prefix path (empty string if not installed).
    // missing is populated with formula names that are required but absent.
    static QMap<QString, QString> checkHomebrewDeps(
        const QList<HomebrewDep> &deps,
        const ProgressFn         &progress,
        QStringList              *missing = nullptr);

    // Returns the output of `brew --prefix` (cached after first call).
    static QString homebrewPrefix();

    // Returns `brew --prefix <formula>` for keg-only packages.
    static QString formulaPrefix(const QString &formula);

private:
    // ── Core pipeline helpers ─────────────────────────────────────────────────
    bool runShell(const QString &cmd, QString *out = nullptr,
                  int timeoutMs = 120000) const;
    bool injectManifest(const QString &runtimeApp, const Manifest &m,
                        const QString &projectDir) const;
    bool createDmg(const QString &appPath,  const QString &volName,
                   const QString &dmgPath,  const QString &bgImage) const;

    // ── Homebrew artifact baking ──────────────────────────────────────────────

    // Collect all /opt/homebrew (arm64) or /usr/local (x86_64) dylib paths
    // referenced by a Mach-O binary's load commands (otool -L).
    QStringList collectHomebrewDylibs(const QString &binary) const;

    // Recursively find all Mach-O binaries inside an .app bundle
    // (binary, dylibs inside Contents/Frameworks, plugins inside Contents/PlugIns)
    QStringList findMachOFiles(const QString &appBundle) const;

    // For each payload .app bundle, copy referenced Homebrew dylibs into
    // Contents/Frameworks/ and fix load commands to use @executable_path refs.
    // Uses dylibbundler if installed, falls back to manual otool+install_name_tool.
    bool bakeHomebrewArtifacts(const QString &payloadDir,
                               const ProgressFn &progress,
                               QString *errOut) const;

    // Fix all Homebrew-absolute load commands inside a single .app bundle.
    // Uses install_name_tool -change on every Mach-O file found.
    bool fixInstallNames(const QString &appBundle,
                         const QMap<QString,QString> &dyMap) const;

    // Codesign an .app bundle (deep + identity from manifest, or ad-hoc).
    bool codesignApp(const QString &appPath,
                     const QString &identity,
                     const ProgressFn &progress) const;
};
