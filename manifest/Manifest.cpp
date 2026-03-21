#include "Manifest.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSysInfo>

// ══════════════════════════════════════════════════════════════════════════════
// Lightweight YAML emitter
// ══════════════════════════════════════════════════════════════════════════════
namespace {

// Quote a YAML scalar value — add quotes if it contains special chars
static QString yq(const QString &v)
{
    if (v.isEmpty()) return "\"\"";
    static const QRegularExpression needsQuote("[:#\\[\\]{}|>&!'\",\\n\\r]");
    if (needsQuote.match(v).hasMatch() || v.startsWith(' ') || v.endsWith(' '))
        return '"' + QString(v).replace('"', "\\\"") + '"';
    return v;
}

static QString indent(int n) { return QString(n * 2, ' '); }

static QString emitFileEntry(const FileEntry &fe, int ind)
{
    QString s;
    s += indent(ind) + "- src: " + yq(fe.src) + "\n";
    s += indent(ind + 1) + "dst: " + yq(fe.dst) + "\n";
    if (!fe.chmod.isEmpty())
        s += indent(ind + 1) + "chmod: " + yq(fe.chmod) + "\n";
    if (fe.isDir)
        s += indent(ind + 1) + "dir: true\n";
    if (!fe.platforms.isEmpty())
        s += indent(ind + 1) + "platforms: [" + fe.platforms.join(", ") + "]\n";
    return s;
}

} // namespace

// ══════════════════════════════════════════════════════════════════════════════
// toYaml
// ══════════════════════════════════════════════════════════════════════════════
QString Manifest::toYaml() const
{
    QString s;
    QTextStream ts(&s);

    ts << "format: " << kFormatVersion << "\n\n";

    // ── app ──────────────────────────────────────────────────────────────────
    ts << "app:\n";
    ts << "  name:        " << yq(app.name)        << "\n";
    ts << "  version:     " << yq(app.version)      << "\n";
    ts << "  publisher:   " << yq(app.publisher)    << "\n";
    ts << "  description: " << yq(app.description)  << "\n";
    ts << "  url:         " << yq(app.url)           << "\n";
    if (!app.supportUrl.isEmpty())
    ts << "  support-url: " << yq(app.supportUrl)   << "\n";
    ts << "  identifier:  " << yq(app.identifier)   << "\n";
    if (!app.iconPath.isEmpty())
    ts << "  icon:        " << yq(app.iconPath)      << "\n";
    if (!app.licenseFile.isEmpty())
    ts << "  license:     " << yq(app.licenseFile)   << "\n";
    if (!app.outputName.isEmpty())
    ts << "  output-name: " << yq(app.outputName)    << "\n";

    // ── defaults ──────────────────────────────────────────────────────────────
    ts << "\ndefaults:\n";
    ts << "  install-dir:\n";
    ts << "    macos:   " << yq(defaults.installDirMacos)   << "\n";
    ts << "    windows: " << yq(defaults.installDirWindows) << "\n";
    ts << "    linux:   " << yq(defaults.installDirLinux)   << "\n";
    ts << "  require-admin:      " << (defaults.requireAdmin      ? "true" : "false") << "\n";
    ts << "  allow-custom-dir:   " << (defaults.allowCustomDir    ? "true" : "false") << "\n";
    ts << "  launch-after:       " << (defaults.launchAfter       ? "true" : "false") << "\n";
    ts << "  create-uninstaller: " << (defaults.createUninstaller ? "true" : "false") << "\n";

    // ── theme ─────────────────────────────────────────────────────────────────
    if (!theme.bannerImage.isEmpty() || !theme.accentColor.isEmpty()) {
        ts << "\ntheme:\n";
        if (!theme.bannerImage.isEmpty())
        ts << "  banner-image:    " << yq(theme.bannerImage)   << "\n";
        if (!theme.sidePanelImage.isEmpty())
        ts << "  sidepanel-image: " << yq(theme.sidePanelImage)<< "\n";
        if (!theme.accentColor.isEmpty())
        ts << "  accent-color:    " << yq(theme.accentColor)   << "\n";
        if (!theme.backgroundColor.isEmpty())
        ts << "  background:      " << yq(theme.backgroundColor)<< "\n";
        if (theme.darkMode)
        ts << "  dark-mode: true\n";
    }

    // ── signing ───────────────────────────────────────────────────────────────
    const bool hasSigningConfig =
        !signing.macosSigner.isEmpty() || !signing.winPfxPath.isEmpty()
        || !signing.linuxGpgKey.isEmpty() || signing.macosNotarize
        || signing.linuxSignDebs;
    if (hasSigningConfig) {
        ts << "\nsigning:\n";
        if (!signing.macosSigner.isEmpty())
        ts << "  macos-signer:   " << yq(signing.macosSigner)   << "\n";
        if (!signing.macosTeamId.isEmpty())
        ts << "  macos-team-id:  " << yq(signing.macosTeamId)   << "\n";
        ts << "  macos-hardened: " << (signing.macosHardened ? "true" : "false") << "\n";
        if (signing.macosNotarize)
        ts << "  macos-notarize: true\n";
        if (!signing.macosProfile.isEmpty())
        ts << "  macos-profile:  " << yq(signing.macosProfile)  << "\n";
        if (!signing.winPfxPath.isEmpty())
        ts << "  win-pfx-path:   " << yq(signing.winPfxPath)    << "\n";
        if (!signing.winTimestampUrl.isEmpty())
        ts << "  win-timestamp:  " << yq(signing.winTimestampUrl)<< "\n";
        if (!signing.linuxGpgKey.isEmpty())
        ts << "  linux-gpg-key:  " << yq(signing.linuxGpgKey)   << "\n";
        if (signing.linuxSignDebs)
        ts << "  linux-sign-debs: true\n";
    }

    // ── app-groups ────────────────────────────────────────────────────────────
    if (!appGroups.isEmpty()) {
        ts << "\napp-groups:\n";
        for (const auto &g : appGroups) {
            ts << "  - id:          " << yq(g.id)          << "\n";
            ts << "    name:        " << yq(g.name)         << "\n";
            if (!g.description.isEmpty())
            ts << "    description: " << yq(g.description)  << "\n";
        }
    }

    // ── prerequisites ─────────────────────────────────────────────────────────
    if (!prerequisites.isEmpty()) {
        ts << "\nprerequisites:\n";
        for (const auto &p : prerequisites) {
            ts << "  - id: " << yq(p.id) << "\n";
            ts << "    name: " << yq(p.name) << "\n";
            if (!p.checkCmd.isEmpty())
            ts << "    check: " << yq(p.checkCmd) << "\n";
            if (!p.installCmd.isEmpty())
            ts << "    install: " << yq(p.installCmd) << "\n";
            if (!p.platforms.isEmpty())
            ts << "    platforms: [" << p.platforms.join(", ") << "]\n";
        }
    }

    // ── components ────────────────────────────────────────────────────────────
    ts << "\ncomponents:\n";
    for (const auto &comp : components) {
        ts << "  - id:          " << yq(comp.id)          << "\n";
        ts << "    name:        " << yq(comp.name)         << "\n";
        ts << "    description: " << yq(comp.description)  << "\n";
        ts << "    required:    " << (comp.required  ? "true" : "false") << "\n";
        ts << "    selected:    " << (comp.selected  ? "true" : "false") << "\n";
        if (!comp.appGroup.isEmpty())
        ts << "    app-group:   " << yq(comp.appGroup)             << "\n";
        if (!comp.platforms.isEmpty())
        ts << "    platforms:   [" << comp.platforms.join(", ")    << "]\n";
        if (!comp.depends.isEmpty())
        ts << "    depends:     [" << comp.depends.join(", ")      << "]\n";
        if (!comp.files.isEmpty()) {
            ts << "    files:\n";
            for (const auto &fe : comp.files)
                ts << emitFileEntry(fe, 3);   // indent(3)=6sp → parser ind==6 ✓
        }
    }

    // ── shortcuts ─────────────────────────────────────────────────────────────
    if (!shortcuts.isEmpty()) {
        ts << "\nshortcuts:\n";
        for (const auto &sc : shortcuts) {
            ts << "  - name:   " << yq(sc.name)   << "\n";
            ts << "    target: " << yq(sc.target)  << "\n";
            if (!sc.type.isEmpty())
            ts << "    type:   " << yq(sc.type)    << "\n";
            if (!sc.icon.isEmpty())
            ts << "    icon:   " << yq(sc.icon)    << "\n";
        }
    }

    // ── registry (Windows) ────────────────────────────────────────────────────
    if (!registry.isEmpty()) {
        ts << "\nregistry:\n";
        for (const auto &r : registry) {
            ts << "  - hive:  " << yq(r.hive)      << "\n";
            ts << "    key:   " << yq(r.key)        << "\n";
            if (!r.valueName.isEmpty())
            ts << "    value: " << yq(r.valueName)  << "\n";
            if (!r.valueData.isEmpty())
            ts << "    data:  " << yq(r.valueData)  << "\n";
            if (!r.valueType.isEmpty())
            ts << "    type:  " << yq(r.valueType)  << "\n";
        }
    }

    // ── custom actions ────────────────────────────────────────────────────────
    if (!customActions.isEmpty()) {
        ts << "\ncustom-actions:\n";
        for (const auto &a : customActions) {
            ts << "  - id:      " << yq(a.id)      << "\n";
            ts << "    trigger: " << yq(a.trigger)  << "\n";
            ts << "    type:    " << yq(a.type)     << "\n";
            ts << "    command: " << yq(a.command)  << "\n";
            if (!a.platforms.isEmpty())
            ts << "    platforms: [" << a.platforms.join(", ") << "]\n";
        }
    }

    // ── targets ───────────────────────────────────────────────────────────────
    ts << "\ntargets:\n";
    for (const auto &t : targets)
        ts << "  - " << t << "\n";

    return s;
}

// ══════════════════════════════════════════════════════════════════════════════
// Lightweight YAML parser for .mis format
// ══════════════════════════════════════════════════════════════════════════════
namespace {

struct YNode {
    int         indent = 0;
    QString     key;
    QString     value;     // for scalar nodes
    bool        isList = false;
};

// Strip YAML comments and trailing whitespace
static QString stripLine(const QString &line)
{
    // Remove inline comments (# not inside quotes)
    bool inQ = false;
    int commentPos = -1;
    for (int i = 0; i < line.size(); ++i) {
        if (line[i] == '"') inQ = !inQ;
        if (!inQ && line[i] == '#') { commentPos = i; break; }
    }
    QString s = (commentPos >= 0) ? line.left(commentPos) : line;
    return s.trimmed();
}

static QString unquote(const QString &v)
{
    QString s = v.trimmed();
    if ((s.startsWith('"') && s.endsWith('"')) ||
        (s.startsWith('\'') && s.endsWith('\'')))
        s = s.mid(1, s.length() - 2).replace("\\\"", "\"");
    return s;
}

static bool toBool(const QString &v) { return (v == "true" || v == "yes" || v == "1"); }

} // namespace

// ══════════════════════════════════════════════════════════════════════════════
// fromYaml — parse the .mis YAML format
// ══════════════════════════════════════════════════════════════════════════════
bool Manifest::fromYaml(const QString &yaml, QString *errOut)
{
    QStringList lines = yaml.split('\n');

    // Helper lambdas
    auto key = [](const QString &line) -> QString {
        int colon = line.indexOf(':');
        return colon >= 0 ? line.left(colon).trimmed().toLower() : QString();
    };
    auto val = [](const QString &line) -> QString {
        int colon = line.indexOf(':');
        return colon >= 0 ? unquote(line.mid(colon + 1).trimmed()) : QString();
    };
    auto indOf = [](const QString &line) -> int {
        int i = 0;
        while (i < line.size() && line[i] == ' ') ++i;
        return i;
    };

    QString section;
    Component currentComp;
    bool inComp = false;
    bool inFiles = false;
    FileEntry currentFile;
    bool inFileItem = false;

    Shortcut currentSC;
    bool inSC = false;

    RegistryEntry currentReg;
    bool inReg = false;

    CustomAction currentCA;
    bool inCA = false;

    Prerequisite currentPrereq;
    bool inPrereq = false;

    AppGroup currentGroup;
    bool inGroup = false;

    // Reset all fields
    app = {}; defaults = {}; theme = {}; signing = {};
    appGroups.clear(); components.clear(); shortcuts.clear(); registry.clear();
    prerequisites.clear(); customActions.clear(); targets.clear();

    auto flushGroup = [&]() { if (inGroup) { appGroups.append(currentGroup); currentGroup = {}; inGroup = false; } };
    auto flushComp = [&]() {
        if (inComp) {
            if (inFileItem) { currentComp.files.append(currentFile); inFileItem = false; }
            components.append(currentComp);
            currentComp = {}; inComp = false; inFiles = false;
        }
    };
    auto flushSC  = [&]() { if (inSC)     { shortcuts.append(currentSC); currentSC = {}; inSC = false; } };
    auto flushReg = [&]() { if (inReg)    { registry.append(currentReg); currentReg = {}; inReg = false; } };
    auto flushCA  = [&]() { if (inCA)     { customActions.append(currentCA); currentCA = {}; inCA = false; } };
    auto flushPre = [&]() { if (inPrereq) { prerequisites.append(currentPrereq); currentPrereq = {}; inPrereq = false; } };

    for (const QString &rawLine : lines) {
        QString stripped = stripLine(rawLine);
        if (stripped.isEmpty()) continue;
        int ind = indOf(rawLine);
        QString k = key(rawLine);
        QString v = val(rawLine);

        // Top-level section detection (indent == 0, no dash)
        if (ind == 0 && !stripped.startsWith('-')) {
            // Flush any pending objects when leaving a section
            if (section == "app-groups")     flushGroup();
            if (section == "components")     flushComp();
            if (section == "shortcuts")      flushSC();
            if (section == "registry")       flushReg();
            if (section == "custom-actions") flushCA();
            if (section == "prerequisites")  flushPre();

            section = k;
            if (k == "format") {
                if (!v.startsWith("mis/") && errOut)
                    *errOut = QString("Unknown format: '%1'").arg(v);
            }
            continue;
        }

        // ── app ───────────────────────────────────────────────────────────────
        if (section == "app" && ind == 2) {
            if (k == "name")        app.name        = v;
            else if (k == "version")     app.version     = v;
            else if (k == "publisher")   app.publisher   = v;
            else if (k == "description") app.description = v;
            else if (k == "url")         app.url         = v;
            else if (k == "support-url") app.supportUrl  = v;
            else if (k == "identifier")  app.identifier  = v;
            else if (k == "icon")        app.iconPath    = v;
            else if (k == "license")     app.licenseFile = v;
            else if (k == "output-name") app.outputName  = v;
            continue;
        }

        // ── defaults ──────────────────────────────────────────────────────────
        if (section == "defaults") {
            if (ind == 4) {
                if (k == "macos")   defaults.installDirMacos   = v;
                else if (k == "windows") defaults.installDirWindows = v;
                else if (k == "linux")   defaults.installDirLinux   = v;
            } else if (ind == 2) {
                if (k == "require-admin")      defaults.requireAdmin      = toBool(v);
                else if (k == "allow-custom-dir")   defaults.allowCustomDir    = toBool(v);
                else if (k == "launch-after")       defaults.launchAfter       = toBool(v);
                else if (k == "create-uninstaller") defaults.createUninstaller = toBool(v);
            }
            continue;
        }

        // ── theme ─────────────────────────────────────────────────────────────
        if (section == "theme" && ind == 2) {
            if (k == "banner-image")    theme.bannerImage    = v;
            else if (k == "sidepanel-image") theme.sidePanelImage = v;
            else if (k == "accent-color")    theme.accentColor    = v;
            else if (k == "background")      theme.backgroundColor= v;
            else if (k == "dark-mode")       theme.darkMode       = toBool(v);
            continue;
        }

        // ── signing ───────────────────────────────────────────────────────────
        if (section == "signing" && ind == 2) {
            if      (k == "macos-signer")    signing.macosSigner    = v;
            else if (k == "macos-team-id")   signing.macosTeamId    = v;
            else if (k == "macos-hardened")  signing.macosHardened  = toBool(v);
            else if (k == "macos-notarize")  signing.macosNotarize  = toBool(v);
            else if (k == "macos-profile")   signing.macosProfile   = v;
            else if (k == "win-pfx-path")    signing.winPfxPath     = v;
            else if (k == "win-timestamp")   signing.winTimestampUrl= v;
            else if (k == "linux-gpg-key")   signing.linuxGpgKey    = v;
            else if (k == "linux-sign-debs") signing.linuxSignDebs  = toBool(v);
            continue;
        }

        // ── targets ───────────────────────────────────────────────────────────
        if (section == "targets" && stripped.startsWith("- ")) {
            targets << stripped.mid(2).trimmed();
            continue;
        }

        // ── app-groups ────────────────────────────────────────────────────────
        if (section == "app-groups") {
            if (ind == 2 && stripped.startsWith("- ")) {
                flushGroup();
                currentGroup = {}; inGroup = true;
                QString rest = stripped.mid(2).trimmed();
                if (rest.startsWith("id:"))
                    currentGroup.id = unquote(rest.mid(3).trimmed());
                else if (!rest.isEmpty())
                    currentGroup.id = unquote(rest);
                continue;
            }
            if (inGroup && ind == 4) {
                if      (k == "id")          currentGroup.id          = v;
                else if (k == "name")        currentGroup.name        = v;
                else if (k == "description") currentGroup.description = v;
                continue;
            }
        }

        // ── prerequisites ─────────────────────────────────────────────────────
        if (section == "prerequisites" && ind == 2) {
            if (stripped.startsWith("- ")) {
                flushPre();
                currentPrereq.id = unquote(stripped.mid(3).trimmed());
                if (currentPrereq.id.startsWith("id:"))
                    currentPrereq.id = unquote(currentPrereq.id.mid(3).trimmed());
                inPrereq = true;
            } else if (inPrereq) {
                if (k == "name")    currentPrereq.name       = v;
                else if (k == "check")   currentPrereq.checkCmd   = v;
                else if (k == "install") currentPrereq.installCmd = v;
            }
            if (inPrereq && k == "platforms" && v.startsWith('['))
                currentPrereq.platforms = v.remove('[').remove(']').split(',', Qt::SkipEmptyParts);
            continue;
        }

        // ── components ────────────────────────────────────────────────────────
        if (section == "components") {
            if (ind == 2 && stripped.startsWith("- ")) {
                // New component
                flushComp();
                currentComp.id = unquote(stripped.mid(2).trimmed());
                if (currentComp.id.startsWith("id:"))
                    currentComp.id = unquote(currentComp.id.mid(3).trimmed());
                inComp = true; inFiles = false;
                continue;
            }
            if (inComp && ind == 4 && !inFiles) {
                if (k == "id")          currentComp.id          = v;
                else if (k == "name")        currentComp.name        = v;
                else if (k == "description") currentComp.description = v;
                else if (k == "required")    currentComp.required    = toBool(v);
                else if (k == "selected")    currentComp.selected    = toBool(v);
                else if (k == "files")       inFiles = true;
                else if (k == "app-group")   currentComp.appGroup = v;
                else if (k == "depends" && v.startsWith('['))
                    currentComp.depends = v.remove('[').remove(']').split(',', Qt::SkipEmptyParts);
                else if (k == "platforms" && v.startsWith('['))
                    currentComp.platforms = v.remove('[').remove(']').split(',', Qt::SkipEmptyParts);
                continue;
            }
            if (inComp && inFiles) {
                if (ind == 6 && stripped.startsWith("- ")) {
                    if (inFileItem) currentComp.files.append(currentFile);
                    currentFile = {};
                    // may start with "src: ..."
                    QString rest = stripped.mid(2).trimmed();
                    if (rest.startsWith("src:"))
                        currentFile.src = unquote(rest.mid(4).trimmed());
                    inFileItem = true;
                    continue;
                }
                if (inFileItem && ind == 8) {
                    if      (k == "src")   currentFile.src   = v;
                    else if (k == "dst")   currentFile.dst   = v;
                    else if (k == "chmod") currentFile.chmod = v;
                    else if (k == "dir")   currentFile.isDir = toBool(v);
                    else if (k == "platforms" && v.startsWith('['))
                        currentFile.platforms = v.remove('[').remove(']').split(',', Qt::SkipEmptyParts);
                    continue;
                }
            }
            continue;
        }

        // ── shortcuts ─────────────────────────────────────────────────────────
        if (section == "shortcuts") {
            if (ind == 2 && stripped.startsWith("- ")) {
                flushSC();
                currentSC = {}; inSC = true;
                QString rest = stripped.mid(2).trimmed();
                if (rest.startsWith("name:"))
                    currentSC.name = unquote(rest.mid(5).trimmed());
                continue;
            }
            if (inSC && ind == 4) {
                if (k == "name")   currentSC.name   = v;
                else if (k == "target") currentSC.target = v;
                else if (k == "type")   currentSC.type   = v;
                else if (k == "icon")   currentSC.icon   = v;
                continue;
            }
        }

        // ── registry ──────────────────────────────────────────────────────────
        if (section == "registry") {
            if (ind == 2 && stripped.startsWith("- ")) {
                flushReg();
                currentReg = {}; inReg = true;
                continue;
            }
            if (inReg && ind == 4) {
                if (k == "hive")  currentReg.hive      = v;
                else if (k == "key")   currentReg.key       = v;
                else if (k == "value") currentReg.valueName = v;
                else if (k == "data")  currentReg.valueData = v;
                else if (k == "type")  currentReg.valueType = v;
            }
        }

        // ── custom-actions ────────────────────────────────────────────────────
        if (section == "custom-actions") {
            if (ind == 2 && stripped.startsWith("- ")) {
                flushCA();
                currentCA = {}; inCA = true;
                continue;
            }
            if (inCA && ind == 4) {
                if (k == "id")      currentCA.id      = v;
                else if (k == "trigger") currentCA.trigger  = v;
                else if (k == "type")    currentCA.type     = v;
                else if (k == "command") currentCA.command  = v;
                else if (k == "platforms" && v.startsWith('['))
                    currentCA.platforms = v.remove('[').remove(']').split(',', Qt::SkipEmptyParts);
            }
        }
    }

    // Flush last pending objects
    if (section == "app-groups")     flushGroup();
    if (section == "components")     flushComp();
    if (section == "shortcuts")      flushSC();
    if (section == "registry")       flushReg();
    if (section == "custom-actions") flushCA();
    if (section == "prerequisites")  flushPre();

    return true;
}

// ── File I/O ──────────────────────────────────────────────────────────────────
bool Manifest::save(const QString &path, QString *errOut) const
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errOut) *errOut = QString("Cannot write to: %1").arg(path);
        return false;
    }
    f.write(toYaml().toUtf8());
    return true;
}

bool Manifest::load(const QString &path, QString *errOut)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errOut) *errOut = QString("Cannot open: %1").arg(path);
        return false;
    }
    return fromYaml(QString::fromUtf8(f.readAll()), errOut);
}

// ── Token resolution ──────────────────────────────────────────────────────────
QString Manifest::defaultInstallDir(const QString &platform) const
{
    QString tpl;
    if      (platform == "macos")   tpl = defaults.installDirMacos;
    else if (platform == "windows") tpl = defaults.installDirWindows;
    else                            tpl = defaults.installDirLinux;
    tpl.replace("{name}",      app.name);
    tpl.replace("{publisher}", app.publisher);
    return tpl;
}

QString Manifest::resolve(const QString &templateStr, const QString &platform) const
{
    QString s = templateStr;
    s.replace("{install-dir}", defaultInstallDir(platform));
    s.replace("{name}",        app.name);
    s.replace("{publisher}",   app.publisher);
    s.replace("{version}",     app.version);
    return s;
}

// ── Validation ────────────────────────────────────────────────────────────────
QStringList Manifest::validate() const
{
    QStringList issues;
    if (app.name.trimmed().isEmpty())      issues << "App name is required.";
    if (app.version.trimmed().isEmpty())   issues << "App version is required.";
    if (app.publisher.trimmed().isEmpty()) issues << "Publisher is required.";
    if (components.isEmpty())              issues << "At least one component is required.";
    bool hasRequired = false;
    for (const auto &c : components) if (c.required) { hasRequired = true; break; }
    if (!hasRequired) issues << "At least one component must be marked required.";
    if (targets.isEmpty()) issues << "At least one target platform must be selected.";
    return issues;
}

// ── Factory ───────────────────────────────────────────────────────────────────
Manifest Manifest::newProject()
{
    Manifest m;
    m.app.name        = "My Application";
    m.app.version     = "1.0.0";
    m.app.publisher   = "My Company";
    m.app.description = "A brief description of My Application.";
    m.app.url         = "https://example.com";
    m.app.identifier  = "com.example.myapp";

    m.defaults.installDirMacos   = "/Applications/{publisher}";
    m.defaults.installDirWindows = "C:\\Program Files\\{publisher}\\{name}";
    m.defaults.installDirLinux   = "/opt/{publisher}";

    // Default app group
    AppGroup grp;
    grp.id   = "app1";
    grp.name = "My Application";
    grp.description = "Primary application deliverable.";
    m.appGroups.append(grp);

    Component core;
    core.id          = "core";
    core.name        = "Core Application";
    core.description = "Main application files (required).";
    core.required    = true;
    core.selected    = true;
    core.appGroup    = "app1";

    FileEntry fe;
    fe.src = "payload/MyApp.app";
    fe.dst = "{install-dir}/MyApp.app";
    core.files.append(fe);

    m.components.append(core);

    Shortcut sc;
    sc.name   = "Launch My Application";
    sc.target = "{install-dir}/MyApp.app";
    sc.type   = "app";
    m.shortcuts.append(sc);

    m.targets << "macos";
    return m;
}
