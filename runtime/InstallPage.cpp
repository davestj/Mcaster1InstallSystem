/*
 * InstallPage.cpp — Live installation progress
 */
#include "InstallPage.h"
#include "InstallerWizard.h"
#include "InstallEngine.h"

#include <QVBoxLayout>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QLabel>
#include <QThread>

InstallPage::InstallPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Installing");
    setSubTitle("Please wait while the installation completes.");

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 16, 0, 0);
    vbox->setSpacing(8);

    m_status = new QLabel("Preparing…", this);
    vbox->addWidget(m_status);

    m_bar = new QProgressBar(this);
    m_bar->setRange(0, 100);
    m_bar->setValue(0);
    vbox->addWidget(m_bar);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(160);
    m_log->setMaximumBlockCount(2000);  // keep memory bounded
    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    m_log->setFont(mono);
    vbox->addWidget(m_log, 1);
}

void InstallPage::initializePage()
{
    if (m_started) return;  // guard against re-entry if wizard revisits this page
    m_started = true;
    m_done    = false;

    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    // Propagate selected components from ComponentsPage if not yet set
    QStringList selIds = wiz->selectedComponents();
    if (selIds.isEmpty()) {
        // If ComponentsPage was skipped or had no components, select all
        for (const Component &c : wiz->manifest().components)
            selIds << c.id;
        wiz->setSelectedComponents(selIds);
    }

    // Build engine
    auto *engine = new InstallEngine();
    engine->setManifest(wiz->manifest());
    engine->setInstallDir(wiz->installDir());
    engine->setSelectedComponents(selIds);
    engine->setPayloadDir(wiz->payloadDir());

    // Move engine to a worker thread
    auto *thread = new QThread(this);
    engine->moveToThread(thread);

    // Wire signals — note: cross-thread connection uses Qt::QueuedConnection by default
    connect(thread, &QThread::started,       engine, &InstallEngine::run);
    connect(engine, &InstallEngine::progress, this,  &InstallPage::onProgress);
    connect(engine, &InstallEngine::logLine,  this,  &InstallPage::onLogLine);
    connect(engine, &InstallEngine::finished, this,  &InstallPage::onFinished);

    // Clean up thread and engine after install completes
    connect(engine, &InstallEngine::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished,       engine, &QObject::deleteLater);
    connect(thread, &QThread::finished,       thread, &QObject::deleteLater);

    thread->start();
}

bool InstallPage::isComplete() const
{
    return m_done;
}

void InstallPage::onProgress(int pct, const QString &msg)
{
    m_bar->setValue(pct);
    m_status->setText(msg);
}

void InstallPage::onLogLine(const QString &line)
{
    m_log->appendPlainText(line);
}

void InstallPage::onFinished(bool success, const QString &error)
{
    m_done = true;

    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (wiz)
        wiz->setInstallResult(success, error);

    if (success) {
        m_bar->setValue(100);
        m_status->setText("Installation complete.");
    } else {
        m_status->setText(QString("Installation failed: %1").arg(error));
        m_log->appendPlainText(QString("\n[ERROR] %1").arg(error));
    }

    // Notify QWizard that isComplete() has changed so it enables "Next"
    emit completeChanged();
}
