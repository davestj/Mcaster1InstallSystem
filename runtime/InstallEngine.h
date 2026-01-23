#pragma once
/*
 * InstallEngine.h — File copy, shortcut creation, custom action runner.
 *
 * Runs on a background QThread. Emits progress/log/finished signals.
 * The wizard's InstallPage connects these to its progress bar and log view.
 *
 * Install sequence:
 *   1. Expand tokens in all paths ({install-dir}, {name}, {publisher}, {version})
 *   2. Run "before-install" custom actions
 *   3. For each selected component → copy files into installDir
 *   4. Create shortcuts
 *   5. Write uninstall manifest to installDir/.uninstall/manifest.json
 *   6. Run "after-install" custom actions
 */

#include <QObject>
#include <QThread>
#include <QString>
#include <QStringList>
#include "Manifest.h"

class InstallEngine : public QObject
{
    Q_OBJECT

public:
    explicit InstallEngine(QObject *parent = nullptr);

    // Called before moving to the worker thread
    void setManifest(const Manifest &m);
    void setInstallDir(const QString &dir);
    void setSelectedComponents(const QStringList &componentIds);
    void setPayloadDir(const QString &payloadDir);   // absolute path to payload/

    // Utility: resolve tokens in a path given installDir + manifest metadata
    static QString resolveToken(const QString &tpl, const Manifest &m, const QString &installDir);

public slots:
    void run();   // Slot — call via QMetaObject::invokeMethod or QThread::start + moveToThread

signals:
    void progress(int pct, const QString &message);
    void logLine(const QString &line);
    void finished(bool success, const QString &errorMessage);

private:
    bool copyFile(const QString &src, const QString &dst);
    bool copyDir(const QString &src, const QString &dst);
    bool createShortcut(const Shortcut &sc);
    bool runCustomAction(const CustomAction &ca);
    bool writeUninstallManifest(const QStringList &installedFiles);

    Manifest     m_manifest;
    QString      m_installDir;
    QString      m_payloadDir;
    QStringList  m_selectedComponents;

    QStringList  m_installedFiles;   // tracks everything copied for uninstall manifest
};
