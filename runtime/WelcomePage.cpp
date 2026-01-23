/*
 * WelcomePage.cpp — Splash/welcome page
 */
#include "WelcomePage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFont>

WelcomePage::WelcomePage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("");  // We draw our own title in the layout

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    // ── Banner image ──────────────────────────────────────────────────────
    m_bannerLabel = new QLabel(this);
    m_bannerLabel->setAlignment(Qt::AlignCenter);
    m_bannerLabel->setFixedHeight(72);
    m_bannerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    vbox->addWidget(m_bannerLabel);

    // ── Content ───────────────────────────────────────────────────────────
    auto *content = new QWidget(this);
    auto *cl      = new QVBoxLayout(content);
    cl->setContentsMargins(40, 32, 40, 24);
    cl->setSpacing(8);

    m_appName = new QLabel(content);
    m_appName->setObjectName("titleLabel");
    cl->addWidget(m_appName);

    m_appVersion = new QLabel(content);
    m_appVersion->setObjectName("subtitleLabel");
    cl->addWidget(m_appVersion);

    cl->addSpacing(16);

    m_description = new QLabel(content);
    m_description->setWordWrap(true);
    m_description->setObjectName("hintLabel");
    cl->addWidget(m_description);

    cl->addSpacing(16);

    m_publisherLbl = new QLabel(content);
    m_publisherLbl->setObjectName("hintLabel");
    cl->addWidget(m_publisherLbl);

    cl->addStretch();

    auto *readyLbl = new QLabel("Click <b>Next</b> to begin the installation.", content);
    readyLbl->setWordWrap(true);
    cl->addWidget(readyLbl);

    vbox->addWidget(content, 1);
}

void WelcomePage::initializePage()
{
    auto *wiz = qobject_cast<InstallerWizard*>(wizard());
    if (!wiz) return;

    const Manifest &m = wiz->manifest();

    // Banner
    const QPixmap &px = wiz->bannerPixmap();
    m_bannerLabel->setPixmap(px.scaled(m_bannerLabel->width(), 72,
                                       Qt::KeepAspectRatioByExpanding,
                                       Qt::SmoothTransformation));

    m_appName    ->setText(m.app.name);
    m_appVersion ->setText(QString("Version %1").arg(m.app.version));
    m_description->setText(m.app.description);
    m_publisherLbl->setText(QString("Publisher: %1").arg(m.app.publisher));
}
