#pragma once
/*
 * CodeSigner.h — Cross-platform code signing engine
 *
 * ┌────────────────────────────────────────────────────────────────────────┐
 * │  Platform          Tool                   Cert Format                 │
 * ├────────────────────────────────────────────────────────────────────────┤
 * │  macOS .app        codesign               Keychain identity (string)  │
 * │  macOS .dmg        codesign + notarize    Apple ID + App-specific pw  │
 * │  macOS .pkg        productsign            Keychain identity (string)  │
 * │  Windows .exe/.msi signtool (Win)         PFX / P12 cert + password   │
 * │                    osslsigncode (x-plat)  Same PFX, any OS            │
 * │  Linux .deb        dpkg-sig / debsigs     GPG key ID                  │
 * │  Linux .rpm        rpm --addsign          GPG key ID                  │
 * │  AppImage          appimagetool + GPG      GPG key ID                 │
 * └────────────────────────────────────────────────────────────────────────┘
 *
 * All sign*() methods return true on success and populate errOut on failure.
 * Each method checks that the required tool is available before proceeding.
 */

#include <QString>
#include <QStringList>
#include <functional>

class CodeSigner
{
public:
    // Callback for progress reporting: (percent 0-100, message)
    using ProgressFn = std::function<void(int, const QString &)>;

    // ── macOS ─────────────────────────────────────────────────────────────────

    /*  Sign a .app bundle or individual binary.
     *  identity: "Developer ID Application: Name (TEAMID)"
     *            or "-" for ad-hoc self-sign (no Apple Developer account needed)
     *  entitlementsPath: path to .entitlements plist, or empty to skip.
     *  hardened: true = --options runtime (required for notarization).           */
    static bool signMacosApp(const QString &appPath,
                              const QString &identity,
                              const QString &entitlementsPath = {},
                              bool hardened = true,
                              QString *errOut = nullptr,
                              const ProgressFn &progress = {});

    /*  Notarize a .dmg, .zip, or .pkg via Apple's notary service.
     *  password: app-specific password (NOT your Apple ID password).
     *  On success, staples the notarization ticket to the package.             */
    static bool notarizeMacos(const QString &packagePath,
                               const QString &appleId,
                               const QString &teamId,
                               const QString &appPassword,
                               QString *errOut = nullptr,
                               const ProgressFn &progress = {});

    /*  Sign a macOS Installer .pkg.
     *  identity: "Developer ID Installer: Name (TEAMID)"                       */
    static bool signMacosPkg(const QString &pkgPath,
                              const QString &identity,
                              QString *errOut = nullptr,
                              const ProgressFn &progress = {});

    // ── Windows ───────────────────────────────────────────────────────────────

    /*  Sign a Windows .exe or .msi using osslsigncode (cross-platform)
     *  or signtool.exe (Windows only, auto-detected).
     *  pfxPath:       path to PFX/P12 certificate file
     *  pfxPassword:   PFX password (may be empty for password-less PFX)
     *  timestampUrl:  RFC 3161 timestamp server, e.g.
     *                   http://timestamp.digicert.com
     *                   http://timestamp.sectigo.com
     *                   http://ts.ssl.com
     *  description:   /d "App Name" field embedded in signature              */
    static bool signWindowsExe(const QString &exePath,
                                const QString &pfxPath,
                                const QString &pfxPassword,
                                const QString &timestampUrl,
                                const QString &description = {},
                                QString *errOut = nullptr,
                                const ProgressFn &progress = {});

    // ── Linux ─────────────────────────────────────────────────────────────────

    /*  Sign a .deb package with a GPG key (uses dpkg-sig or debsigs).
     *  gpgKeyId: GPG key fingerprint or email                                 */
    static bool signDebPackage(const QString &debPath,
                                const QString &gpgKeyId,
                                QString *errOut = nullptr,
                                const ProgressFn &progress = {});

    /*  Sign a .rpm package.  Requires ~/.rpmmacros with %_gpg_name set,
     *  or pass gpgKeyId which is temporarily exported.                        */
    static bool signRpmPackage(const QString &rpmPath,
                                const QString &gpgKeyId,
                                QString *errOut = nullptr,
                                const ProgressFn &progress = {});

    /*  Sign an AppImage using GPG (embeds .sig alongside the file).           */
    static bool signAppImage(const QString &appImagePath,
                              const QString &gpgKeyId,
                              QString *errOut = nullptr,
                              const ProgressFn &progress = {});

    // ── Utility ───────────────────────────────────────────────────────────────

    /*  Returns true if the given tool is available on PATH.                   */
    static bool toolAvailable(const QString &tool);

    /*  Returns the best available signing tool for Windows on the current OS. */
    static QString windowsSigningTool();  // "signtool", "osslsigncode", or ""

    /*  Returns a human-readable summary of available signing tools.           */
    static QStringList availableTools();

private:
    static bool runTool(const QString &program, const QStringList &args,
                        const ProgressFn &progress, QString *errOut,
                        int timeoutMs = 300000);
};
