#include "FinishPage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QProcess>

FinishPage::FinishPage(QWidget *parent)
    : QWizardPage(parent)
{
    // Empty title/subtitle — gives our custom banner the full top of the page.
    setTitle("");
    setSubTitle("");

    // ── Outer: vertical stack (banner on top, content row below) ──────────────
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Banner row — loaded in initializePage()
    m_bannerLabel = new QLabel(this);
    m_bannerLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_bannerLabel->setVisible(false);
    outer->addWidget(m_bannerLabel);

    // Content row: [side panel] [text area]
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_sidePanelLabel = new QLabel(this);
    m_sidePanelLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_sidePanelLabel->setVisible(false);
    row->addWidget(m_sidePanelLabel);

    // Text content
    auto *textBox = new QVBoxLayout;
    textBox->setContentsMargins(20, 14, 16, 12);
    textBox->setSpacing(8);

    // Headline — updated in initializePage() based on success/failure
    m_headline = new QLabel(this);
    QFont boldFont = m_headline->font();
    boldFont.setPointSize(boldFont.pointSize() + 2);
    boldFont.setBold(true);
    m_headline->setFont(boldFont);
    m_headline->setWordWrap(true);
    textBox->addWidget(m_headline);

    // Detail text — next-step instructions
    m_detail = new QLabel(this);
    m_detail->setTextFormat(Qt::RichText);
    m_detail->setWordWrap(true);
    m_detail->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    QFont smallFont = m_detail->font();
    smallFont.setPointSize(smallFont.pointSize() - 1);
    m_detail->setFont(smallFont);
    textBox->addWidget(m_detail);

    textBox->addStretch();

    // Launch checkbox — hidden on failure
    m_chkLaunch = new QCheckBox("Launch Mcaster1DNAS now", this);
    m_chkLaunch->setChecked(true);
    textBox->addWidget(m_chkLaunch);

    row->addLayout(textBox);
    outer->addLayout(row);
}

void FinishPage::initializePage()
{
    auto *wiz  = qobject_cast<InstallerWizard *>(wizard());
    m_success  = wiz ? wiz->installSuccess() : false;

    // Load banner + side panel from wizard (same images as WelcomePage)
    if (wiz) {
        QPixmap bpx = wiz->bannerPixmap();
        if (!bpx.isNull()) {
            m_bannerLabel->setPixmap(bpx);
            m_bannerLabel->setFixedSize(bpx.size());
            m_bannerLabel->setVisible(true);
        }

        QPixmap spx = wiz->sidePanelPixmap();
        if (!spx.isNull()) {
            m_sidePanelLabel->setPixmap(spx);
            m_sidePanelLabel->setFixedSize(spx.size());
            m_sidePanelLabel->setVisible(true);
        }
    }

    if (m_success) {
        m_headline->setText(
            "<span style='font-size:18px; font-weight:bold;'>"
            "Installation Complete!</span>");

        const QString installDir = wiz ? wiz->options().installDir
                                       : QStringLiteral("/Applications/Mcaster1");
        const bool svcInstalled  = wiz && wiz->options().installService;

        QString detail =
            "<b>What was installed:</b><br>"
            "&nbsp;&nbsp;&bull; <b>Mcaster1DNAS.app</b> &mdash; Qt6 GUI control panel<br>";
        if (svcInstalled)
            detail +=
                "&nbsp;&nbsp;&bull; <b>bin/mcaster1</b> &mdash; headless server binary "
                "(ports 9330 / 9443)<br>"
                "&nbsp;&nbsp;&bull; LaunchAgent service config &amp; install script<br>";
        detail +=
            "&nbsp;&nbsp;&bull; Web &amp; admin XSL templates<br>"
            "&nbsp;&nbsp;&bull; SSL setup guide<br>"
            "<br>"
            "<b>Before you start streaming:</b><br>"
            "&nbsp;&nbsp;1. Edit <tt>" + installDir + "/mcaster1dnas.yaml</tt><br>"
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Change every <tt>CHANGE_ME_*</tt> password.<br>"
            "&nbsp;&nbsp;2. In Mcaster1DNAS.app &rarr; <b>Config &rarr; SSL Cert</b> tab,<br>"
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;generate a self-signed cert, or drop your PEM<br>"
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;into <tt>" + installDir + "/ssl/</tt>.<br>";
        if (svcInstalled)
            detail +=
                "&nbsp;&nbsp;3. Double-click <tt>install-service.sh</tt><br>"
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;in the install folder to enable auto-start.<br>";
        detail +=
            "<br>"
            "GUI ports: <b>9033 HTTP / 9344 HTTPS</b>&nbsp;&nbsp;"
            "Service ports: <b>9330 HTTP / 9443 HTTPS</b>";

        m_detail->setText(detail);
        m_chkLaunch->setVisible(true);
        m_chkLaunch->setChecked(wiz ? wiz->options().launchAfter : true);
    } else {
        m_headline->setText(
            "<span style='font-size:18px; font-weight:bold; color:#cc2200;'>"
            "Installation Failed</span>");
        m_detail->setText(
            "Review the installation log on the previous page for details.<br><br>"
            "<b>Common causes:</b><br>"
            "&nbsp;&nbsp;&bull; Insufficient permissions to write to <tt>/Applications</tt>.<br>"
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Try right-clicking the installer and choosing<br>"
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<i>Open as Administrator</i>.<br>"
            "&nbsp;&nbsp;&bull; The installer payload may be incomplete or corrupted.<br>"
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Re-download the DMG and try again.<br><br>"
            "If the problem persists, open a support issue at:<br>"
            "github.com/davestj/mcaster1dnas/issues"
        );
        m_chkLaunch->setVisible(false);
    }
}

bool FinishPage::isComplete() const
{
    return true;
}

bool FinishPage::validatePage()
{
    if (m_success && m_chkLaunch->isVisible() && m_chkLaunch->isChecked()) {
        auto *wiz = qobject_cast<InstallerWizard *>(wizard());
        const QString appPath = wiz ? wiz->options().installDir + "/Mcaster1DNAS.app"
                                    : QStringLiteral("/Applications/Mcaster1/Mcaster1DNAS.app");
        QProcess::startDetached("open", {appPath});
    }
    return true;
}
