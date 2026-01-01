#pragma once
#include "InstallerWizard.h"
#include <QObject>
#include <QString>
#include <QStringList>

// Runs the actual file-copy installation on a background thread.
// Emits log lines + progress percentage + finished signal.
class InstallWorker : public QObject
{
    Q_OBJECT
public:
    explicit InstallWorker(const InstallOptions &opts, const QString &payloadDir,
                           QObject *parent = nullptr);

public slots:
    void run();

signals:
    void logLine(const QString &line);
    void progress(int percent);
    void finished(bool success, const QString &message);

private:
    bool copyDir(const QString &src, const QString &dst);
    bool copyFile(const QString &src, const QString &dst);
    bool makeDir(const QString &path);
    bool runShell(const QString &cmd);  // for chmod / xattr

    InstallOptions m_opts;
    QString        m_payload;
    QString        m_dest;
};
