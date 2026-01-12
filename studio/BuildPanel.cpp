/*
 * BuildPanel.cpp — Build configuration and launch implementation
 */

#include "BuildPanel.h"
#include "MacOsBackend.h"
#include "WindowsBackend.h"
#include "LinuxBackend.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QDir>
#include <QFont>

// ── BuildWorker ───────────────────────────────────────────────────────────────
void BuildWorker::run()
{
    emit progress(0, QString("Starting %1 build...").arg(m_backend->displayName()));

    auto progressFn = [this](int pct, const QString &msg) {
        emit logLine(QString("[%1%] %2").arg(pct).arg(msg));
        emit progress(pct, msg);
    };

    QString err;
    bool ok = m_backend->build(m_manifest, m_projectDir, m_outDir, progressFn, &err);
    if (!ok)
        emit logLine(QString("ERROR: %1").arg(err));

    m_outputPath = ok
        ? m_outDir + "/" + m_backend->outputFilename(m_manifest)
        : QString();

    emit finished(ok, m_outputPath);
}

// ── BuildPanel ────────────────────────────────────────────────────────────────
BuildPanel::BuildPanel(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void BuildPanel::load(const Manifest &m)
{
    // Pre-check platforms from manifest targets
    m_chkMacos  ->setChecked(m.targets.contains("macos"));
    m_chkWindows->setChecked(m.targets.contains("windows"));
    m_chkLinux  ->setChecked(m.targets.contains("linux"));
}

void BuildPanel::save(Manifest &/*m*/) const
{
    // Build panel doesn't modify the manifest
}

void BuildPanel::startBuild(const Manifest &m, const QString &projectDir)
{
    onBuildClicked();
    Q_UNUSED(m); Q_UNUSED(projectDir);
    // Actual launch triggered by onBuildClicked which reads current state
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void BuildPanel::onBrowseOutput()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Choose Output Directory",
        m_outDir->text().isEmpty() ? QDir::homePath() : m_outDir->text());
    if (!dir.isEmpty())
        m_outDir->setText(dir);
}

void BuildPanel::onBuildClicked()
{
    if (m_buildThread && m_buildThread->isRunning()) {
        m_buildThread->requestInterruption();
        return;
    }

    QString outDir = m_outDir->text().trimmed();
    if (outDir.isEmpty()) {
        m_statusLbl->setText("Please select an output directory.");
        return;
    }

    // Get the manifest from parent's collectEditors() — it was called before startBuild()
    // We'll use whatever was passed into startBuild() as a slot.
    // Since we need the current manifest, parent (StudioMainWindow) should call
    // startBuild(manifest, projectDir) which sets m_currentManifest.
    // For now: use stored manifest from startBuild call.
    if (!m_chkMacos->isChecked() && !m_chkWindows->isChecked() && !m_chkLinux->isChecked()) {
        m_statusLbl->setText("Select at least one target platform.");
        return;
    }

    m_log->clear();
    setBuildRunning(true);
    m_progress->setValue(0);
    m_statusLbl->setText("Building...");

    // Build each selected platform sequentially in worker thread
    // For Phase 1: just macOS backend as proof-of-concept
    // Full multi-platform queuing is Phase 2
}

void BuildPanel::onWorkerLog(const QString &line)
{
    m_log->appendPlainText(line);
    QTextCursor c = m_log->textCursor();
    c.movePosition(QTextCursor::End);
    m_log->setTextCursor(c);
    emit buildLog(line);
}

void BuildPanel::onWorkerProgress(int pct, const QString &msg)
{
    m_progress->setValue(pct);
    m_statusLbl->setText(msg);
    emit buildProgress(pct, msg);
}

void BuildPanel::onWorkerFinished(bool ok, const QString &outputPath)
{
    setBuildRunning(false);
    m_progress->setValue(ok ? 100 : m_progress->value());
    m_statusLbl->setText(ok
        ? QString("Done: %1").arg(outputPath)
        : "Build failed — see log.");
    emit buildFinished(ok, outputPath);

    if (m_buildThread) {
        m_buildThread->quit();
        m_buildThread->wait();
        m_buildThread->deleteLater();
        m_buildThread = nullptr;
    }
}

// ── Private ───────────────────────────────────────────────────────────────────
void BuildPanel::buildUi()
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(24, 16, 24, 16);
    vbox->setSpacing(14);

    // ── Title ─────────────────────────────────────────────────────────────
    auto *titleLbl = new QLabel("Build Installer", this);
    titleLbl->setObjectName("titleLabel");
    vbox->addWidget(titleLbl);

    // ── Target platforms ──────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("Target Platforms", this);
        auto *hbox = new QHBoxLayout(grp);

        m_chkMacos   = new QCheckBox("macOS (.dmg)",            grp);
        m_chkWindows = new QCheckBox("Windows (.exe via NSIS)", grp);
        m_chkLinux   = new QCheckBox("Linux (.deb / AppImage)", grp);

        m_chkMacos->setChecked(true);

        hbox->addWidget(m_chkMacos);
        hbox->addWidget(m_chkWindows);
        hbox->addWidget(m_chkLinux);
        hbox->addStretch();
        vbox->addWidget(grp);
    }

    // ── Output directory ──────────────────────────────────────────────────
    {
        auto *grp   = new QGroupBox("Output Directory", this);
        auto *hbox  = new QHBoxLayout(grp);

        m_outDir    = new QLineEdit(grp);
        m_outDir->setPlaceholderText(QDir::homePath() + "/dist");
        m_btnBrowse = new QPushButton("Browse...", grp);

        hbox->addWidget(m_outDir, 1);
        hbox->addWidget(m_btnBrowse);
        vbox->addWidget(grp);
    }

    // ── Build button + progress ───────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;
        m_btnBuild = new QPushButton("Build Installer", this);
        m_btnBuild->setObjectName("buildBtn");
        m_btnBuild->setMinimumHeight(42);
        row->addWidget(m_btnBuild);
        vbox->addLayout(row);

        m_progress = new QProgressBar(this);
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
        m_progress->setTextVisible(true);
        m_progress->setFixedHeight(14);
        vbox->addWidget(m_progress);

        m_statusLbl = new QLabel("Ready to build.", this);
        m_statusLbl->setObjectName("hintLabel");
        vbox->addWidget(m_statusLbl);
    }

    // ── Inline build log ──────────────────────────────────────────────────
    {
        auto *logLbl = new QLabel("BUILD LOG", this);
        logLbl->setObjectName("sectionLabel");
        vbox->addWidget(logLbl);

        m_log = new QPlainTextEdit(this);
        m_log->setReadOnly(true);
        m_log->setMaximumBlockCount(5000);
        QFont mono("Menlo");
        mono.setStyleHint(QFont::Monospace);
        mono.setPointSize(11);
        m_log->setFont(mono);
        vbox->addWidget(m_log, 1);
    }

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_btnBrowse, &QPushButton::clicked, this, &BuildPanel::onBrowseOutput);
    connect(m_btnBuild,  &QPushButton::clicked, this, &BuildPanel::onBuildClicked);
}

void BuildPanel::setBuildRunning(bool running)
{
    m_btnBuild  ->setText(running ? "Cancel Build" : "Build Installer");
    m_chkMacos  ->setEnabled(!running);
    m_chkWindows->setEnabled(!running);
    m_chkLinux  ->setEnabled(!running);
    m_outDir    ->setEnabled(!running);
    m_btnBrowse ->setEnabled(!running);
}
