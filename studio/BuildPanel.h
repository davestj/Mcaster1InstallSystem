#pragma once
/*
 * BuildPanel.h — Build configuration and progress panel (the "Build" tab).
 *
 * Shows:
 *   - Platform checkboxes (macOS / Windows / Linux)
 *   - Output directory picker
 *   - Build button
 *   - Progress bar + status label
 *   - Inline build log (QPlainTextEdit)
 */

#include <QWidget>
#include <QThread>
#include "Manifest.h"
#include "BuildBackend.h"

class QCheckBox;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QLabel;
class QPlainTextEdit;

// ── Background worker ─────────────────────────────────────────────────────────
class BuildWorker : public QObject
{
    Q_OBJECT
public:
    BuildWorker(BuildBackend *backend, Manifest manifest, QString projectDir, QString outDir)
        : m_backend(backend), m_manifest(std::move(manifest))
        , m_projectDir(std::move(projectDir)), m_outDir(std::move(outDir)) {}

public slots:
    void run();

signals:
    void logLine(const QString &line);
    void progress(int pct, const QString &msg);
    void finished(bool ok, const QString &outputPath);

private:
    BuildBackend *m_backend;
    Manifest      m_manifest;
    QString       m_projectDir;
    QString       m_outDir;
    QString       m_outputPath;
};

// ── BuildPanel ────────────────────────────────────────────────────────────────
class BuildPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BuildPanel(QWidget *parent = nullptr);

    void load(const Manifest &m);
    void save(Manifest &m) const;  // no-op — build panel doesn't modify manifest

    // Called by StudioMainWindow to start a build
    void startBuild(const Manifest &m, const QString &projectDir);

signals:
    void buildLog(const QString &line);
    void buildProgress(int pct, const QString &msg);
    void buildFinished(bool ok, const QString &outputPath);

private slots:
    void onBrowseOutput();
    void onBuildClicked();
    void onWorkerLog(const QString &line);
    void onWorkerProgress(int pct, const QString &msg);
    void onWorkerFinished(bool ok, const QString &outputPath);

private:
    void buildUi();
    void setBuildRunning(bool running);

    QCheckBox    *m_chkMacos   = nullptr;
    QCheckBox    *m_chkWindows = nullptr;
    QCheckBox    *m_chkLinux   = nullptr;
    QLineEdit    *m_outDir     = nullptr;
    QPushButton  *m_btnBrowse  = nullptr;
    QPushButton  *m_btnBuild   = nullptr;
    QProgressBar *m_progress   = nullptr;
    QLabel       *m_statusLbl  = nullptr;
    QPlainTextEdit *m_log      = nullptr;

    QThread *m_buildThread = nullptr;
};
