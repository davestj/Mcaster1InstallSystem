#include "InstallWorker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

InstallWorker::InstallWorker(const InstallOptions &opts, const QString &payloadDir,
                             QObject *parent)
    : QObject(parent), m_opts(opts), m_payload(payloadDir),
      m_dest(opts.installDir)
{}

// ── public slot: called from QThread::start() ────────────────────────────────
void InstallWorker::run()
{
    emit logLine("=== Mcaster1DNAS Installation ===");
    emit logLine(QString("Destination: %1").arg(m_dest));
    emit progress(2);

    // ── 1. Create install directory ───────────────────────────────────────────
    emit logLine("\n[1/5] Creating install directory...");
    if (!makeDir(m_dest)) {
        emit finished(false, QString("Could not create %1\n\nMake sure you have "
                                     "write permission to /Applications.\n"
                                     "You may need to run the installer as an "
                                     "administrator.").arg(m_dest));
        return;
    }
    emit logLine(QString("  Created: %1").arg(m_dest));
    emit progress(10);

    // ── 2. Install GUI app bundle ─────────────────────────────────────────────
    emit logLine("\n[2/5] Installing Mcaster1DNAS.app...");
    if (!copyDir(m_payload + "/app/Mcaster1DNAS.app",
                 m_dest   + "/Mcaster1DNAS.app")) {
        emit finished(false, "Failed to copy Mcaster1DNAS.app.\n"
                              "Check that the installer is not corrupted.");
        return;
    }
    // Clear Gatekeeper quarantine
    runShell(QString("xattr -rc \"%1/Mcaster1DNAS.app\"").arg(m_dest));
    emit logLine("  Installed: Mcaster1DNAS.app");
    emit progress(55);

    // ── 3. Install web/admin XSL templates ────────────────────────────────────
    emit logLine("\n[3/5] Installing web & admin templates...");
    copyDir(m_payload + "/web",   m_dest + "/web");
    copyDir(m_payload + "/admin", m_dest + "/admin");
    makeDir(m_dest + "/ssl");
    copyFile(m_payload + "/ssl/SSL-SETUP.txt", m_dest + "/ssl/SSL-SETUP.txt");
    makeDir(m_dest + "/logs");
    copyFile(m_payload + "/configs/mcaster1dnas.yaml",
             m_dest     + "/mcaster1dnas.yaml");
    copyFile(m_payload + "/configs/mcaster1dnas.xml",
             m_dest     + "/mcaster1dnas.xml");
    emit logLine("  Installed: web/, admin/, ssl/, logs/, configs");
    emit progress(65);

    // ── 4. Background service (optional) ─────────────────────────────────────
    if (m_opts.installService) {
        emit logLine("\n[4/5] Installing background service...");
        makeDir(m_dest + "/bin");
        copyFile(m_payload + "/service/mcaster1", m_dest + "/bin/mcaster1");
        runShell(QString("chmod +x \"%1/bin/mcaster1\"").arg(m_dest));
        copyFile(m_payload + "/service/mcaster1dnas-service.yaml",
                 m_dest     + "/mcaster1dnas-service.yaml");
        copyFile(m_payload + "/service/com.mcaster1.mcaster1dnas.plist",
                 m_dest     + "/com.mcaster1.mcaster1dnas.plist");
        copyFile(m_payload + "/service/install-service.sh",
                 m_dest     + "/install-service.sh");
        runShell(QString("chmod +x \"%1/install-service.sh\"").arg(m_dest));
        emit logLine("  Installed: bin/mcaster1, service config, LaunchAgent plist");
        emit logLine("  To start service: open " + m_dest + "/install-service.sh");
    } else {
        emit logLine("\n[4/5] Background service: skipped");
    }
    emit progress(78);

    // ── 5. Shortcuts ──────────────────────────────────────────────────────────
    if (m_opts.installShortcuts) {
        emit logLine("\n[5/5] Installing shortcuts...");
        copyFile(m_payload + "/shortcuts/Launch Mcaster1DNAS.command",
                 m_dest     + "/Launch Mcaster1DNAS.command");
        runShell(QString("chmod +x \"%1/Launch Mcaster1DNAS.command\"").arg(m_dest));
        copyFile(m_payload + "/shortcuts/Uninstall Mcaster1DNAS.command",
                 m_dest     + "/Uninstall Mcaster1DNAS.command");
        runShell(QString("chmod +x \"%1/Uninstall Mcaster1DNAS.command\"").arg(m_dest));
        copyFile(m_payload + "/shortcuts/Documentation.webloc",
                 m_dest     + "/Documentation.webloc");
        copyFile(m_payload + "/shortcuts/Mcaster1 Website.webloc",
                 m_dest     + "/Mcaster1 Website.webloc");
        copyFile(m_payload + "/shortcuts/Support & Issues.webloc",
                 m_dest     + "/Support & Issues.webloc");
        emit logLine("  Installed: shortcuts & bookmarks");
    } else {
        emit logLine("\n[5/5] Shortcuts: skipped");
    }
    emit progress(98);

    emit logLine("\n=== Installation complete! ===");
    emit progress(100);
    emit finished(true, QString());
}

// ── Private helpers ───────────────────────────────────────────────────────────
bool InstallWorker::makeDir(const QString &path)
{
    if (QDir(path).exists()) return true;
    if (!QDir().mkpath(path)) {
        emit logLine("  ERROR: Cannot create directory: " + path);
        return false;
    }
    return true;
}

bool InstallWorker::copyFile(const QString &src, const QString &dst)
{
    if (!QFile::exists(src)) {
        emit logLine("  WARN: Source not found: " + src);
        return true;  // non-fatal — some payload items may be optional
    }
    // Ensure destination directory exists
    makeDir(QFileInfo(dst).absolutePath());
    if (QFile::exists(dst)) QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        emit logLine("  ERROR: Could not copy: " + QFileInfo(src).fileName());
        return false;
    }
    // Preserve executable bit for .command / binary files
    QFile dstFile(dst);
    QFile srcFile(src);
    dstFile.setPermissions(srcFile.permissions());
    return true;
}

bool InstallWorker::copyDir(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    if (!srcDir.exists()) {
        emit logLine("  ERROR: Payload directory missing: " + src);
        return false;
    }
    if (!makeDir(dst)) return false;

    for (const QFileInfo &fi :
         srcDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden)) {
        const QString srcPath = fi.absoluteFilePath();
        const QString dstPath = dst + "/" + fi.fileName();

        if (fi.isSymLink()) {
            // Preserve symlinks (Qt framework bundles use them)
            QString target = fi.symLinkTarget();
            QFile::link(target, dstPath);
        } else if (fi.isDir()) {
            if (!copyDir(srcPath, dstPath)) return false;
        } else {
            if (!copyFile(srcPath, dstPath)) return false;
        }
    }
    return true;
}

bool InstallWorker::runShell(const QString &cmd)
{
    QProcess p;
    p.start("/bin/bash", {"-c", cmd});
    p.waitForFinished(10000);
    return p.exitCode() == 0;
}
