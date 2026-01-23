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
 *
 * Build execution model:
 *   startBuild() stores the manifest + projectDir, then calls onBuildClicked().
 *   onBuildClicked() assembles a m_backendQueue of selected BuildBackend
 *   instances, validates each upfront, then launches them sequentially via
 *   BuildWorker + QThread.  onWorkerFinished() starts the next backend until
 *   the queue is exhausted.
 */

#include <QWidget>
#include <QThread>
#include <QList>
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
    ~BuildPanel() override;

    void load(const Manifest &m);
    void save(Manifest &m) const;  // no-op — build panel doesn't modify manifest

    // Called by StudioMainWindow toolbar / menu Build action
    void startBuild(const Manifest &m, const QString &projectDir);

signals:
    void buildLog(const QString &line);
    void buildProgress(int pct, const QString &msg);
    void buildFinished(bool ok, const QString &outputPath);

private slots:
    void onBrowseOutput();
    void onBuildClicked();
    void onTestInstaller();
    void onWorkerLog(const QString &line);
    void onWorkerProgress(int pct, const QString &msg);
    void onWorkerFinished(bool ok, const QString &outputPath);

private:
    void buildUi();
    void setBuildRunning(bool running);
    void launchNextBackend();    // starts m_backendQueue[m_queueIndex] on a new thread
    void cleanupQueue();         // delete owned backends and clear queue

    // ── Widgets ──────────────────────────────────────────────────────────
    QCheckBox    *m_chkMacos   = nullptr;
    QCheckBox    *m_chkWindows = nullptr;
    QCheckBox    *m_chkLinux   = nullptr;
    QLineEdit    *m_outDir     = nullptr;
    QPushButton  *m_btnBrowse  = nullptr;
    QPushButton  *m_btnBuild   = nullptr;
    QPushButton  *m_btnTest    = nullptr;
    QProgressBar *m_progress   = nullptr;
    QLabel       *m_statusLbl  = nullptr;
    QPlainTextEdit *m_log      = nullptr;

    // ── Build state ───────────────────────────────────────────────────────
    QThread *m_buildThread = nullptr;

    Manifest m_manifest;
    QString  m_projectDir;

    QList<BuildBackend *> m_backendQueue;   // owned; deleted in cleanupQueue()
    int                   m_queueIndex = 0;
    int                   m_failCount  = 0;
};
