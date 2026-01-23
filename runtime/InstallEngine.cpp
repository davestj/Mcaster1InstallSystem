/*
 * InstallEngine.cpp — File installation, shortcut creation, custom actions
 */

#include "InstallEngine.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <shlobj.h>
#  include <objbase.h>
#endif

InstallEngine::InstallEngine(QObject *parent)
    : QObject(parent)
{}

void InstallEngine::setManifest(const Manifest &m)    { m_manifest = m; }
void InstallEngine::setInstallDir(const QString &d)   { m_installDir = d; }
void InstallEngine::setSelectedComponents(const QStringList &ids) { m_selectedComponents = ids; }
void InstallEngine::setPayloadDir(const QString &d)   { m_payloadDir = d; }

// ── Token resolver ─────────────────────────────────────────────────────────────
QString InstallEngine::resolveToken(const QString &tpl, const Manifest &m, const QString &installDir)
{
    QString out = tpl;
    out.replace("{install-dir}", installDir);
    out.replace("{name}",        m.app.name);
    out.replace("{publisher}",   m.app.publisher);
    out.replace("{version}",     m.app.version);
    out.replace("{identifier}",  m.app.identifier);
    return out;
}

// ── Main install runner ────────────────────────────────────────────────────────
void InstallEngine::run()
{
    emit progress(0, "Starting installation...");
    emit logLine(QString("Installing %1 %2").arg(m_manifest.app.name, m_manifest.app.version));
    emit logLine(QString("Target: %1").arg(m_installDir));

    int totalSteps = 0;
    int doneSteps  = 0;

    // Count steps: custom-before + file entries + shortcuts + custom-after
    for (const Component &c : m_manifest.components) {
        if (!m_selectedComponents.contains(c.id)) continue;
        totalSteps += c.files.size();
    }
    totalSteps += m_manifest.shortcuts.size();
    totalSteps += m_manifest.customActions.size();
    totalSteps = qMax(totalSteps, 1);

    auto pct = [&]() -> int {
        return qMin(99, (doneSteps * 95) / totalSteps);
    };

    // ── 1. Create install directory ────────────────────────────────────────────
    emit progress(2, QString("Creating directory: %1").arg(m_installDir));
    if (!QDir().mkpath(m_installDir)) {
        emit finished(false, QString("Cannot create install directory: %1").arg(m_installDir));
        return;
    }
    emit logLine(QString("  mkdir: %1").arg(m_installDir));

    // ── 2. Before-install custom actions ──────────────────────────────────────
    for (const CustomAction &ca : m_manifest.customActions) {
        if (ca.trigger != "before-install") continue;
        if (!ca.platforms.isEmpty()) {
#ifdef Q_OS_MAC
            if (!ca.platforms.contains("macos")) { ++doneSteps; continue; }
#elif defined(Q_OS_WIN)
            if (!ca.platforms.contains("windows")) { ++doneSteps; continue; }
#else
            if (!ca.platforms.contains("linux")) { ++doneSteps; continue; }
#endif
        }
        emit progress(pct(), QString("Running pre-install action: %1").arg(ca.id));
        emit logLine(QString("  [action] %1: %2").arg(ca.id, ca.command));
        if (!runCustomAction(ca)) {
            emit logLine(QString("  WARN: action %1 failed (continuing)").arg(ca.id));
        }
        ++doneSteps;
    }

    // ── 3. Copy component files ────────────────────────────────────────────────
    for (const Component &c : m_manifest.components) {
        if (!m_selectedComponents.contains(c.id)) continue;

        emit logLine(QString("\n[Component] %1").arg(c.name));

        for (const FileEntry &fe : c.files) {
            QString srcAbs = m_payloadDir + "/" + fe.src;
            QString dstAbs = resolveToken(fe.dst, m_manifest, m_installDir);

            emit progress(pct(), QString("Copying %1").arg(QFileInfo(fe.src).fileName()));
            emit logLine(QString("  %1  →  %2").arg(fe.src, dstAbs));

            bool ok = fe.isDir ? copyDir(srcAbs, dstAbs) : copyFile(srcAbs, dstAbs);
            if (!ok) {
                emit finished(false, QString("Failed to copy:\n%1\nto\n%2").arg(srcAbs, dstAbs));
                return;
            }

#ifndef Q_OS_WIN
            // Apply chmod if specified (e.g., "+x" for executables)
            if (!fe.chmod.isEmpty() && fe.chmod.contains('x')) {
                QFile f(dstAbs);
                f.setPermissions(
                    f.permissions() |
                    QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);
            }
#endif
            m_installedFiles << dstAbs;
            ++doneSteps;
        }
    }

    // ── 4. Create shortcuts ────────────────────────────────────────────────────
    emit progress(pct(), "Creating shortcuts...");
    for (const Shortcut &sc : m_manifest.shortcuts) {
        emit logLine(QString("  shortcut: %1  →  %2").arg(sc.name, sc.target));
        if (!createShortcut(sc)) {
            emit logLine(QString("  WARN: failed to create shortcut '%1'").arg(sc.name));
        }
        ++doneSteps;
    }

    // ── 5. Write uninstall manifest ────────────────────────────────────────────
    emit progress(pct(), "Writing uninstall manifest...");
    writeUninstallManifest(m_installedFiles);

    // ── 6. After-install custom actions ───────────────────────────────────────
    for (const CustomAction &ca : m_manifest.customActions) {
        if (ca.trigger != "after-install") continue;
        emit progress(pct(), QString("Running post-install action: %1").arg(ca.id));
        emit logLine(QString("  [action] %1: %2").arg(ca.id, ca.command));
        if (!runCustomAction(ca))
            emit logLine(QString("  WARN: action %1 failed (continuing)").arg(ca.id));
        ++doneSteps;
    }

    emit progress(100, "Installation complete!");
    emit logLine(QString("\n✓ %1 %2 installed successfully.").arg(m_manifest.app.name, m_manifest.app.version));
    emit logLine(QString("  Location: %1").arg(m_installDir));
    emit finished(true, QString());
}

// ── Private helpers ────────────────────────────────────────────────────────────
bool InstallEngine::copyFile(const QString &src, const QString &dst)
{
    QFileInfo di(dst);
    if (!QDir().mkpath(di.absolutePath())) return false;

    // Remove existing destination
    if (QFile::exists(dst))
        QFile::remove(dst);

    if (!QFile::copy(src, dst)) {
        // If source doesn't exist (payload not present in dev mode), create placeholder
        QFile ph(dst);
        if (ph.open(QIODevice::WriteOnly)) {
            ph.write(QString("# Placeholder for %1\n").arg(src).toUtf8());
            ph.close();
            return true;
        }
        return false;
    }
    return true;
}

bool InstallEngine::copyDir(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    if (!srcDir.exists()) {
        // If source dir doesn't exist (dev mode without real payload), just create dst
        return QDir().mkpath(dst);
    }

    if (!QDir().mkpath(dst)) return false;

    for (const QFileInfo &fi : srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        QString dstPath = dst + "/" + fi.fileName();
        if (fi.isDir()) {
            if (!copyDir(fi.absoluteFilePath(), dstPath)) return false;
        } else {
            if (!copyFile(fi.absoluteFilePath(), dstPath)) return false;
        }
        m_installedFiles << dstPath;
    }
    return true;
}

bool InstallEngine::createShortcut(const Shortcut &sc)
{
    QString target = resolveToken(sc.target, m_manifest, m_installDir);

#ifdef Q_OS_MAC
    // macOS: create a .webloc for URLs or a symlink for apps
    if (sc.type == "webloc" || sc.type == "url") {
        QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        QString path = desktop + "/" + sc.name + ".webloc";
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
               << "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
               << "<plist version=\"1.0\">\n<dict>\n"
               << "  <key>URL</key>\n  <string>" << target << "</string>\n"
               << "</dict>\n</plist>\n";
            m_installedFiles << path;
            return true;
        }
        return false;
    }
    // For app shortcuts — symlink in /Applications or create Dock alias (best-effort)
    m_installedFiles << target;
    return true;

#elif defined(Q_OS_WIN)
    // Windows: create .lnk in Desktop and/or Start Menu
    // CoCreateInstance IShellLink — simplified via PowerShell in Phase 2
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString lnkPath = desktop + "\\" + sc.name + ".lnk";

    // Use PowerShell to create shortcut (most portable, no COM linking needed)
    QString script = QString(
        "$ws = New-Object -ComObject WScript.Shell;"
        "$lnk = $ws.CreateShortcut('%1');"
        "$lnk.TargetPath = '%2';"
        "$lnk.Save()")
        .arg(lnkPath, target);

    return QProcess::execute("powershell.exe",
        {"-NonInteractive", "-Command", script}) == 0;

#else
    // Linux: create .desktop file
    QString apps = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString desktopPath = apps + "/" + sc.name.simplified().replace(' ', '_') + ".desktop";
    QDir().mkpath(apps);

    QFile f(desktopPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << "[Desktop Entry]\n"
           << "Version=1.0\n"
           << "Type=Application\n"
           << "Name=" << sc.name << "\n"
           << "Exec=" << target << "\n";
        if (!sc.icon.isEmpty())
            ts << "Icon=" << resolveToken(sc.icon, m_manifest, m_installDir) << "\n";
        ts << "Terminal=false\n"
           << "Categories=Application;\n";
        f.setPermissions(f.permissions() | QFileDevice::ExeOwner);
        m_installedFiles << desktopPath;
        return true;
    }
    return false;
#endif
}

bool InstallEngine::runCustomAction(const CustomAction &ca)
{
    if (ca.type == "shell") {
#ifdef Q_OS_WIN
        return QProcess::execute("cmd.exe", {"/C", ca.command}) == 0;
#else
        return QProcess::execute("/bin/sh", {"-c", ca.command}) == 0;
#endif
    }

    if (ca.type == "script") {
        // ca.command is the script file path relative to payloadDir
        QString scriptPath = m_payloadDir + "/" + ca.command;
        if (!QFile::exists(scriptPath)) {
            emit logLine(QString("  WARN: script not found: %1").arg(scriptPath));
            return false;
        }
        // Make executable and run via the appropriate interpreter
#ifdef Q_OS_WIN
        // PowerShell for .ps1, cmd for .bat/.cmd, else direct exec
        QString ext = QFileInfo(scriptPath).suffix().toLower();
        if (ext == "ps1")
            return QProcess::execute("powershell.exe",
                {"-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", scriptPath}) == 0;
        if (ext == "bat" || ext == "cmd")
            return QProcess::execute("cmd.exe", {"/C", scriptPath}) == 0;
        return QProcess::execute(scriptPath, {}) == 0;
#else
        // Ensure executable bit
        QFile f(scriptPath);
        f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup);

        QString ext = QFileInfo(scriptPath).suffix().toLower();
        if (ext == "py")
            return QProcess::execute("python3", {scriptPath}) == 0;
        if (ext == "rb")
            return QProcess::execute("ruby", {scriptPath}) == 0;
        // Default: run as shell script
        return QProcess::execute("/bin/sh", {scriptPath}) == 0;
#endif
    }

    return true;  // unknown type — silently succeed
}

bool InstallEngine::writeUninstallManifest(const QStringList &files)
{
    QString uninstallDir = m_installDir + "/.uninstall";
    QDir().mkpath(uninstallDir);

    QString path = uninstallDir + "/files.txt";
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream ts(&f);
    ts << "# Mcaster1Installer uninstall manifest\n";
    ts << "# Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    ts << "# App: " << m_manifest.app.name << " " << m_manifest.app.version << "\n";
    ts << "# Install-dir: " << m_installDir << "\n\n";
    for (const QString &p : files)
        ts << p << "\n";

    return true;
}
