/*
 * FinishPage.cpp — Completion page
 */
#include "FinishPage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>

FinishPage::FinishPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Installation Complete");
    setFinalPage(true);

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 24, 0, 0);
    vbox->setSpacing(16);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setObjectName("subtitleLabel");
    vbox->addWidget(m_statusLabel);

    m_launchCheck = new QCheckBox(this);
    m_launchCheck->setObjectName("launchCheck");
    vbox->addWidget(m_launchCheck);

    vbox->addStretch();

    auto *thanksLbl = new QLabel(
        "Thank you for installing. Click <b>Finish</b> to close the installer.", this);
    thanksLbl->setWordWrap(true);
    thanksLbl->setObjectName("hintLabel");
    vbox->addWidget(thanksLbl);
}

void FinishPage::initializePage()
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    const Manifest &m = wiz->manifest();

    if (wiz->installSuccess()) {
        m_statusLabel->setText(
            QString("%1 %2 was successfully installed to:\n%3")
                .arg(m.app.name, m.app.version, wiz->installDir()));
    } else {
        setTitle("Installation Failed");
        m_statusLabel->setText(
            QString("The installation did not complete successfully.\n\nError: %1\n\n"
                    "Please check the log above and try again.")
                .arg(wiz->installError()));
        m_launchCheck->hide();
        return;
    }

    // Launch checkbox — only shown if launchAfter is set in manifest defaults
    if (m.defaults.launchAfter) {
        m_launchCheck->setText(QString("Launch %1 after closing the installer").arg(m.app.name));
        m_launchCheck->setChecked(true);
        m_launchCheck->show();
    } else {
        m_launchCheck->hide();
    }
}

bool FinishPage::validatePage()
{
    // Called when user clicks Finish — launch app if requested
    if (!m_launchCheck->isChecked() || m_launchCheck->isHidden())
        return true;

    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz || !wiz->installSuccess()) return true;

    const QString installDir = wiz->installDir();
    const QString appName    = wiz->manifest().app.name;

#if defined(Q_OS_MAC)
    // On macOS open the .app bundle inside the install directory
    const QString appBundle = installDir + "/" + appName + ".app";
    QProcess::startDetached("open", {appBundle});

#elif defined(Q_OS_WIN)
    // On Windows run <name>.exe from the install directory
    const QString exe = installDir + "/" + appName + ".exe";
    QProcess::startDetached(exe, {}, installDir);

#else
    // Linux: try running the binary by identifier or name
    const QString bin = installDir + "/" + wiz->manifest().app.identifier.section('.', -1);
    if (!QProcess::startDetached(bin, {}, installDir))
        QDesktopServices::openUrl(QUrl::fromLocalFile(installDir));
#endif

    return true;
}
