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
#include <QMessageBox>
#include <QDir>
#include <QFont>
#include <QDateTime>
#include <QProcess>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include "SvgIcons.h"

static QIcon si(const char *svg, int sz = 20)
{
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}

// ── BuildWorker ───────────────────────────────────────────────────────────────
void BuildWorker::run()
{
    emit progress(0, QString("Starting %1 build...").arg(m_backend->displayName()));

    auto progressFn = [this](int pct, const QString &msg) {
        emit logLine(QString("[%1%] %2").arg(pct, 3).arg(msg));
        emit progress(pct, msg);
    };

    QString err;
    bool ok = m_backend->build(m_manifest, m_projectDir, m_outDir, progressFn, &err);
    if (!ok)
        emit logLine(QString("\n[ERROR] %1").arg(err));

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

BuildPanel::~BuildPanel()
{
    cleanupQueue();
}

void BuildPanel::load(const Manifest &m)
{
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
    m_manifest    = m;
    m_projectDir  = projectDir;
    onBuildClicked();
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
    // ── Cancel running build ───────────────────────────────────────────────
    if (m_buildThread && m_buildThread->isRunning()) {
        m_buildThread->requestInterruption();
        m_statusLbl->setText("Cancelling...");
        return;
    }

    // ── Validate user inputs ───────────────────────────────────────────────
    QString outDir = m_outDir->text().trimmed();
    if (outDir.isEmpty()) {
        m_statusLbl->setText("Please select an output directory.");
        return;
    }

    if (!m_chkMacos->isChecked() && !m_chkWindows->isChecked() && !m_chkLinux->isChecked()) {
        m_statusLbl->setText("Select at least one target platform.");
        return;
    }

    // ── Validate manifest itself ───────────────────────────────────────────
    QStringList manifestErrors = m_manifest.validate();
    if (!manifestErrors.isEmpty()) {
        QMessageBox::warning(this, "Manifest Validation Failed",
            "Fix these issues before building:\n\n" + manifestErrors.join('\n'));
        return;
    }

    // ── Assemble backend queue ─────────────────────────────────────────────
    cleanupQueue();
    m_queueIndex = 0;
    m_failCount  = 0;

    struct PlatformEntry { bool checked; BuildBackend *backend; };
    QList<PlatformEntry> candidates = {
        { m_chkMacos  ->isChecked(), new MacOsBackend()   },
        { m_chkWindows->isChecked(), new WindowsBackend() },
        { m_chkLinux  ->isChecked(), new LinuxBackend()   },
    };

    // Validate each selected backend before queuing
    QStringList allValidationIssues;
    for (auto &pe : candidates) {
        if (!pe.checked) { delete pe.backend; continue; }

        QStringList issues = pe.backend->validate(m_manifest, m_projectDir);
        if (!issues.isEmpty())
            allValidationIssues << QString("── %1 ──").arg(pe.backend->displayName())
                                << issues;

        m_backendQueue << pe.backend;
    }

    if (!allValidationIssues.isEmpty()) {
        auto answer = QMessageBox::warning(this, "Pre-Build Validation",
            "Some checks did not pass:\n\n" + allValidationIssues.join('\n')
            + "\n\nBuild anyway?",
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            cleanupQueue();
            return;
        }
    }

    if (m_backendQueue.isEmpty()) {
        m_statusLbl->setText("No platforms selected.");
        return;
    }

    // ── Prepare output directory ───────────────────────────────────────────
    QDir().mkpath(outDir);

    // ── Start ──────────────────────────────────────────────────────────────
    m_log->clear();
    m_log->appendPlainText(QString("=== Build started: %1 ===")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    m_log->appendPlainText(QString("App:      %1 %2")
        .arg(m_manifest.app.name, m_manifest.app.version));
    m_log->appendPlainText(QString("Output:   %1").arg(outDir));
    m_log->appendPlainText(QString("Targets:  %1")
        .arg(m_backendQueue.size() == 1
             ? m_backendQueue[0]->displayName()
             : QString("%1 platforms").arg(m_backendQueue.size())));
    m_log->appendPlainText("");

    setBuildRunning(true);
    m_progress->setValue(0);

    launchNextBackend();
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
    // Clean up the thread that just finished
    if (m_buildThread) {
        m_buildThread->quit();
        m_buildThread->wait();
        m_buildThread->deleteLater();
        m_buildThread = nullptr;
    }

    if (!ok) ++m_failCount;

    // Emit per-platform result to the dock log
    QString resultLine = ok
        ? QString("\n✓ %1 build OK: %2")
            .arg(m_backendQueue[m_queueIndex]->displayName(), outputPath)
        : QString("\n✗ %1 build FAILED")
            .arg(m_backendQueue[m_queueIndex]->displayName());
    onWorkerLog(resultLine);
    emit buildFinished(ok, outputPath);

    // ── Advance queue ──────────────────────────────────────────────────────
    ++m_queueIndex;
    if (m_queueIndex < m_backendQueue.size()) {
        // More platforms to build
        launchNextBackend();
    } else {
        // All platforms done
        setBuildRunning(false);
        m_progress->setValue(m_failCount == 0 ? 100 : m_progress->value());

        QString summary = m_failCount == 0
            ? QString("All %1 build(s) complete.").arg(m_backendQueue.size())
            : QString("%1 build(s) succeeded, %2 failed.")
                .arg(m_backendQueue.size() - m_failCount).arg(m_failCount);
        m_statusLbl->setText(summary);

        onWorkerLog(QString("\n=== Build finished: %1 ===")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        onWorkerLog(summary);

        cleanupQueue();
    }
}

void BuildPanel::onTestInstaller()
{
    if (m_manifest.app.name.isEmpty()) {
        m_statusLbl->setText("No project loaded — open or create a project first.");
        return;
    }

    // ── Write manifest to a temp directory ────────────────────────────────
    QString tmpDir = QDir::temp().filePath("mcaster1_test_installer");
    QDir().mkpath(tmpDir);
    QString tmpMis = tmpDir + "/manifest.mis";

    QString err;
    if (!m_manifest.save(tmpMis, &err)) {
        QMessageBox::critical(this, "Test Installer",
            "Could not write temporary manifest:\n" + err);
        return;
    }

    // ── Locate the Runtime installer — platform-specific probe ────────────
    QString exeDir = QCoreApplication::applicationDirPath();
    bool launched  = false;

#if defined(Q_OS_MAC)
    // macOS: look for Mcaster1Installer.app bundle
    QStringList candidates = {
        exeDir + "/../../../../runtime/build/Mcaster1Installer.app",
        exeDir + "/../../../runtime/build/Mcaster1Installer.app",
        exeDir + "/../../runtime/build/Mcaster1Installer.app",
        exeDir + "/Mcaster1Installer.app",
        QDir::homePath() + "/Applications/Mcaster1Installer.app",
        "/Applications/Mcaster1Installer.app",
    };
    QString installerPath;
    for (const QString &c : candidates)
        if (QDir(c).exists()) { installerPath = c; break; }

    if (installerPath.isEmpty()) {
        installerPath = QFileDialog::getExistingDirectory(this,
            "Locate Mcaster1Installer.app",
            QDir::homePath(), QFileDialog::ShowDirsOnly);
        if (installerPath.isEmpty()) return;
    }
    // `open` passes --args to the app bundle
    launched = QProcess::startDetached("open", {installerPath, "--args", tmpMis});

#elif defined(Q_OS_WIN)
    // Windows: look for Mcaster1Installer.exe
    QStringList candidates = {
        exeDir + "\\..\\..\\..\\..\\runtime\\build\\Mcaster1Installer.exe",
        exeDir + "\\..\\..\\..\\runtime\\build\\Mcaster1Installer.exe",
        exeDir + "\\..\\runtime\\build\\Mcaster1Installer.exe",
        exeDir + "\\runtime\\build\\Mcaster1Installer.exe",
        exeDir + "\\Mcaster1Installer.exe",
        // Visual Studio Debug/Release layout
        exeDir + "\\..\\..\\windows\\x64\\Debug\\Mcaster1Installer.exe",
        exeDir + "\\..\\..\\windows\\x64\\Release\\Mcaster1Installer.exe",
    };
    QString installerExe;
    for (const QString &c : candidates)
        if (QFile::exists(c)) { installerExe = QDir::toNativeSeparators(c); break; }

    if (installerExe.isEmpty()) {
        installerExe = QFileDialog::getOpenFileName(this,
            "Locate Mcaster1Installer.exe",
            QDir::homePath(),
            "Installer executable (Mcaster1Installer.exe);;All files (*)");
        if (installerExe.isEmpty()) return;
    }
    launched = QProcess::startDetached(installerExe, {tmpMis});

#else
    // Linux: look for Mcaster1Installer binary or AppImage
    QStringList candidates = {
        exeDir + "/../../../../runtime/build/Mcaster1Installer",
        exeDir + "/../../../runtime/build/Mcaster1Installer",
        exeDir + "/../../runtime/build/Mcaster1Installer",
        exeDir + "/Mcaster1Installer",
        exeDir + "/Mcaster1Installer.AppImage",
    };
    QString installerExe;
    for (const QString &c : candidates)
        if (QFile::exists(c)) { installerExe = c; break; }

    if (installerExe.isEmpty()) {
        installerExe = QFileDialog::getOpenFileName(this,
            "Locate Mcaster1Installer",
            QDir::homePath(),
            "Installer (Mcaster1Installer Mcaster1Installer.AppImage);;All files (*)");
        if (installerExe.isEmpty()) return;
    }
    // Ensure the binary is executable
    QFile(installerExe).setPermissions(
        QFile(installerExe).permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup);
    launched = QProcess::startDetached(installerExe, {tmpMis});
#endif

    if (launched) {
        m_statusLbl->setText(QString("Launched test installer with: %1").arg(tmpMis));
        m_log->appendPlainText(QString("[Test] Launched installer — manifest: %1").arg(tmpMis));
    } else {
        QMessageBox::critical(this, "Test Installer",
            "Failed to launch the installer runtime.\n\n"
            "Make sure Mcaster1Installer is built and accessible from the Studio directory.");
    }
}

// ── Private ───────────────────────────────────────────────────────────────────
void BuildPanel::launchNextBackend()
{
    if (m_queueIndex >= m_backendQueue.size()) return;

    BuildBackend *backend = m_backendQueue[m_queueIndex];
    QString outDir        = m_outDir->text().trimmed();

    m_statusLbl->setText(QString("Building %1...").arg(backend->displayName()));
    m_log->appendPlainText(QString("\n─── Platform %1/%2: %3 ───")
        .arg(m_queueIndex + 1).arg(m_backendQueue.size())
        .arg(backend->displayName()));

    // Create the worker (no parent — will be moved to thread)
    auto *worker = new BuildWorker(backend, m_manifest, m_projectDir, outDir);
    m_buildThread = new QThread(this);

    worker->moveToThread(m_buildThread);

    connect(m_buildThread, &QThread::started,      worker, &BuildWorker::run);
    connect(worker, &BuildWorker::logLine,          this,   &BuildPanel::onWorkerLog);
    connect(worker, &BuildWorker::progress,         this,   &BuildPanel::onWorkerProgress);
    connect(worker, &BuildWorker::finished,         this,   &BuildPanel::onWorkerFinished);
    connect(worker, &BuildWorker::finished,         m_buildThread, &QThread::quit);
    connect(m_buildThread, &QThread::finished,      worker, &QObject::deleteLater);

    m_buildThread->start();
}

void BuildPanel::cleanupQueue()
{
    qDeleteAll(m_backendQueue);
    m_backendQueue.clear();
    m_queueIndex = 0;
}

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
        m_btnBrowse = new QPushButton(si(SvgIcons::kFolder), "Browse...", grp);

        hbox->addWidget(m_outDir, 1);
        hbox->addWidget(m_btnBrowse);
        vbox->addWidget(grp);
    }

    // ── Build button + Test button + progress ─────────────────────────────
    {
        auto *row  = new QHBoxLayout;
        m_btnBuild = new QPushButton(si(SvgIcons::kBuild), "Build Installer", this);
        m_btnBuild->setObjectName("buildBtn");
        m_btnBuild->setMinimumHeight(42);
        m_btnTest  = new QPushButton(si(SvgIcons::kPlay), "Test Installer", this);
        m_btnTest->setMinimumHeight(42);
        m_btnTest->setToolTip("Write manifest to temp dir and launch Mcaster1Installer");
        row->addWidget(m_btnBuild, 3);
        row->addWidget(m_btnTest,  1);
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
        m_log->setMaximumBlockCount(8000);
        QFont mono("Menlo");
        mono.setStyleHint(QFont::Monospace);
        mono.setPointSize(11);
        m_log->setFont(mono);
        vbox->addWidget(m_log, 1);
    }

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_btnBrowse, &QPushButton::clicked, this, &BuildPanel::onBrowseOutput);
    connect(m_btnBuild,  &QPushButton::clicked, this, &BuildPanel::onBuildClicked);
    connect(m_btnTest,   &QPushButton::clicked, this, &BuildPanel::onTestInstaller);
}

void BuildPanel::setBuildRunning(bool running)
{
    m_btnBuild  ->setText(running ? "Cancel Build" : "Build Installer");
    m_btnTest   ->setEnabled(!running);
    m_chkMacos  ->setEnabled(!running);
    m_chkWindows->setEnabled(!running);
    m_chkLinux  ->setEnabled(!running);
    m_outDir    ->setEnabled(!running);
    m_btnBrowse ->setEnabled(!running);
}
