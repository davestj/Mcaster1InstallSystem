#pragma once
/*
 * Manifest.h — Mcaster1 Install Spec (.mis) data model
 *
 * File format: YAML  (extension: .mis)
 *
 * Canonical .mis example:
 *
 *   format: mis/1
 *   app:
 *     name: "My Application"
 *     version: "1.0.0"
 *     publisher: "ACME Corp"
 *     description: "Brief description."
 *     url: "https://example.com"
 *     identifier: "com.example.myapp"
 *     icon: "resources/myapp.icns"
 *
 *   defaults:
 *     install-dir:
 *       macos:   "/Applications/ACME"
 *       windows: "C:\\Program Files\\ACME\\MyApp"
 *       linux:   "/opt/acme"
 *
 *   components:
 *     - id: core
 *       name: "Core Application"
 *       description: "Main application files (required)."
 *       required: true
 *       selected: true
 *       files:
 *         - src: "payload/MyApp.app"
 *           dst: "{install-dir}/MyApp.app"
 *
 *   shortcuts:
 *     - name: "Launch My Application"
 *       target: "{install-dir}/MyApp.app"
 *       type: app
 *
 *   targets:
 *     - macos
 *     - windows
 *     - linux
 */

#include <QString>
#include <QStringList>
#include <QList>

// ── FileEntry ─────────────────────────────────────────────────────────────────
struct FileEntry {
    QString     src;            // relative path inside payload/
    QString     dst;            // destination — may contain {install-dir}
    QString     chmod;          // "+x" for executables, "" otherwise
    bool        isDir = false;  // true → recursive directory copy
    QStringList platforms;      // empty = all platforms; else ["macos","windows","linux"]
};

// ── Shortcut ──────────────────────────────────────────────────────────────────
struct Shortcut {
    QString name;           // display name
    QString target;         // path or URL, may contain {install-dir}
    QString icon;           // optional icon path
    QString type;           // "app" | "webloc" | "command" | "url"
};

// ── RegistryEntry (Windows) ───────────────────────────────────────────────────
struct RegistryEntry {
    QString hive;           // HKLM | HKCU | HKCR
    QString key;
    QString valueName;
    QString valueData;
    QString valueType;      // REG_SZ | REG_DWORD | REG_EXPAND_SZ
};

// ── Prerequisite ──────────────────────────────────────────────────────────────
struct Prerequisite {
    QString id;             // e.g. "dotnet48", "vcredist2022"
    QString name;
    QString checkCmd;       // shell command to test presence
    QString installCmd;     // command/URL to install it
    QStringList platforms;  // platforms where this prereq applies
};

// ── CustomAction ──────────────────────────────────────────────────────────────
struct CustomAction {
    QString id;
    QString trigger;        // "before-install" | "after-install" | "before-uninstall" | "after-uninstall"
    QString type;           // "shell" | "script"
    QString command;
    QStringList platforms;
};

// ── AppGroup ──────────────────────────────────────────────────────────────────
// Logical grouping of components (e.g. one group per deliverable application).
// Groups are stored in Manifest::appGroups; Component::appGroup is the group ID tag.
struct AppGroup {
    QString id;
    QString name;
    QString description;
};

// ── Component ─────────────────────────────────────────────────────────────────
struct Component {
    QString            id;
    QString            name;
    QString            description;
    bool               required = false;
    bool               selected = true;
    QList<FileEntry>   files;
    QStringList        depends;   // other component IDs
    QStringList        platforms; // empty = all platforms
    QString            appGroup;  // references AppGroup::id; empty = ungrouped
};

// ── AppInfo ───────────────────────────────────────────────────────────────────
struct AppInfo {
    QString name;
    QString version;
    QString publisher;
    QString description;
    QString url;
    QString supportUrl;
    QString identifier;   // reverse-DNS: com.example.myapp
    QString iconPath;     // relative to project root
    QString licenseFile;  // path to LICENSE.txt / .rtf shown in wizard
    QString outputName;   // optional: override generated installer filename stem
                          // e.g. "MyApp-Setup" → MyApp-Setup-win64.zip
                          //   (omit to use default: <Publisher>-<Name>-<Version>-<platform>)
};

// ── InstallDefaults ───────────────────────────────────────────────────────────
struct InstallDefaults {
    QString installDirMacos   = "/Applications/{publisher}";
    QString installDirWindows = "C:\\Program Files\\{publisher}\\{name}";
    QString installDirLinux   = "/opt/{publisher}";
    bool    requireAdmin      = true;
    bool    allowCustomDir    = true;
    bool    launchAfter       = true;
    bool    createUninstaller = true;
};

// ── WizardTheme ───────────────────────────────────────────────────────────────
struct WizardTheme {
    QString bannerImage;          // path to top banner PNG
    QString sidePanelImage;       // path to side panel PNG
    QString accentColor;          // hex e.g. #00c9ff
    QString backgroundColor;      // hex e.g. #1a1a2e
    bool    darkMode = false;
};

// ── SigningConfig ─────────────────────────────────────────────────────────────
// Stored inside the .mis file under the "signing:" key.
// Passwords are NOT persisted here — always entered at sign-time.
struct SigningConfig {
    // macOS Authenticode / Developer ID
    QString macosSigner;          // "Developer ID Application: ACME Corp (XXXXXXXXXX)"
    QString macosTeamId;          // 10-char Team ID
    bool    macosHardened  = true; // --options runtime (required for notarization)
    bool    macosNotarize  = false;
    QString macosProfile;         // xcrun notarytool store-credentials profile name

    // Windows Authenticode
    QString winPfxPath;           // path to .pfx / .p12 certificate
    QString winTimestampUrl;      // e.g. http://timestamp.sectigo.com

    // Linux GPG
    QString linuxGpgKey;          // GPG key ID or fingerprint
    bool    linuxSignDebs  = false;
};

// ── Manifest ──────────────────────────────────────────────────────────────────
class Manifest
{
public:
    static constexpr const char *kFormatVersion = "mis/1";

    AppInfo              app;
    InstallDefaults      defaults;
    WizardTheme          theme;
    SigningConfig        signing;
    QList<AppGroup>      appGroups;     // logical application groups for the FilesEditor tree
    QList<Component>     components;
    QList<Shortcut>      shortcuts;
    QList<RegistryEntry> registry;     // Windows registry entries
    QList<Prerequisite>  prerequisites;
    QList<CustomAction>  customActions;
    QStringList          targets;      // "macos" | "windows" | "linux"

    // ── YAML serialization ────────────────────────────────────────────────────
    QString     toYaml()   const;
    bool        fromYaml(const QString &yaml, QString *errOut = nullptr);

    // ── File I/O ──────────────────────────────────────────────────────────────
    bool save(const QString &path, QString *errOut = nullptr) const;
    bool load(const QString &path, QString *errOut = nullptr);

    // ── Token resolution ──────────────────────────────────────────────────────
    // Resolve {install-dir}, {name}, {publisher}, {version} in a template string
    QString resolve(const QString &templateStr, const QString &platform) const;
    QString defaultInstallDir(const QString &platform) const;

    // ── Validation ────────────────────────────────────────────────────────────
    QStringList validate() const;

    // ── Factory: blank project template ──────────────────────────────────────
    static Manifest newProject();
};
