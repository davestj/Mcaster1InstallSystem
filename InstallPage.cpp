#include "InstallPage.h"
#include "InstallerWizard.h"
#include "InstallWorker.h"

#include <QVBoxLayout>
#include <QFont>
#include <QScrollBar>
#include <QTimer>

InstallPage::InstallPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Installing Mcaster1DNAS...");
    setSubTitle("Please wait while the files are being copied.");

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    m_statusLabel = new QLabel("Preparing...", this);
    layout->addWidget(m_statusLabel);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    layout->addWidget(m_progress);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(240);
    QFont mono("Menlo", 11);
    mono.setStyleHint(QFont::Monospace);
    m_log->setFont(mono);
    layout->addWidget(m_log);
}

InstallPage::~InstallPage()
{
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait(3000);
    }
}

void InstallPage::initializePage()
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    m_done = false;
    m_success = false;
    m_log->clear();
    m_progress->setValue(0);
    m_statusLabel->setText("Installing...");

    // Create worker + thread
    m_worker = new InstallWorker(wiz->options(), InstallerWizard::payloadPath());
    m_thread = new QThread(this);
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started,      m_worker, &InstallWorker::run);
    connect(m_worker, &InstallWorker::logLine, this,    &InstallPage::onLogLine);
    connect(m_worker, &InstallWorker::progress,this,    &InstallPage::onProgress);
    connect(m_worker, &InstallWorker::finished,this,    &InstallPage::onFinished);
    connect(m_worker, &InstallWorker::finished,m_thread,&QThread::quit);
    connect(m_thread, &QThread::finished,     m_worker, &QObject::deleteLater);

    m_thread->start();
}

bool InstallPage::isComplete() const
{
    return m_done;
}

int InstallPage::nextId() const
{
    return PAGE_FINISH;
}

void InstallPage::onLogLine(const QString &line)
{
    m_log->appendPlainText(line);
    // Scroll to bottom
    auto *sb = m_log->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}

void InstallPage::onProgress(int percent)
{
    m_progress->setValue(percent);
    if (percent < 100)
        m_statusLabel->setText(QString("Installing... (%1%)").arg(percent));
}

void InstallPage::onFinished(bool success, const QString &message)
{
    m_done    = success;
    m_success = success;

    // Propagate result to the wizard so FinishPage can read it
    if (auto *wiz = qobject_cast<InstallerWizard *>(wizard()))
        wiz->setInstallSuccess(success);

    if (success) {
        m_progress->setValue(100);
        m_statusLabel->setText("Installation complete!");
    } else {
        m_statusLabel->setText("Installation failed.");
        if (!message.isEmpty())
            m_log->appendPlainText("\nERROR: " + message);
    }

    // Unblock the Next button
    emit completeChanged();
    if (success) {
        // Auto-advance to finish page after a brief pause
        QTimer::singleShot(600, this, [this]() {
            if (wizard()) wizard()->next();
        });
    }
}
