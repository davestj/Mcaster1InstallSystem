/*
 * InstallerWizard.cpp — The top-level QWizard container
 */

#include "InstallerWizard.h"
#include "WelcomePage.h"
#include "LicensePage.h"
#include "PrerequisitesPage.h"
#include "ComponentsPage.h"
#include "DirectoryPage.h"
#include "ReadyPage.h"
#include "InstallPage.h"
#include "FinishPage.h"

#include <QPixmap>
#include <QDir>
#include <QCoreApplication>
#include <QAbstractButton>

InstallerWizard::InstallerWizard(const Manifest &m, const QString &payloadDir, QWidget *parent)
    : QWizard(parent)
    , m_manifest(m)
    , m_payloadDir(payloadDir)
{
    setWindowTitle(QString("Install %1 %2").arg(m.app.name, m.app.version));
    setWizardStyle(QWizard::ModernStyle);
    setMinimumSize(740, 520);
    setOption(QWizard::NoBackButtonOnStartPage, true);
    setOption(QWizard::NoCancelButtonOnLastPage, true);

    loadBanner();
    buildPages();

    // Style the wizard buttons
    if (auto *btn = button(QWizard::NextButton))
        btn->setObjectName("nextBtn");
    if (auto *btn = button(QWizard::FinishButton))
        btn->setObjectName("finishBtn");
}

QString InstallerWizard::installDir() const
{
    // DirectoryPage registers the field "installDir"
    return field("installDir").toString();
}

void InstallerWizard::buildPages()
{
    setPage(Welcome,    new WelcomePage(this));
    setPage(License,    new LicensePage(this));

    // Prerequisites page is only registered when the manifest has prerequisites.
    // QWizard skips unregistered page IDs seamlessly when nextId() returns them,
    // so the numeric gaps in the enum don't cause any issues.
    if (!m_manifest.prerequisites.isEmpty())
        setPage(Prerequisites, new PrerequisitesPage(this));

    setPage(Components, new ComponentsPage(this));
    setPage(Directory,  new DirectoryPage(this));
    setPage(Ready,      new ReadyPage(this));
    setPage(Installing, new InstallPage(this));
    setPage(Finish,     new FinishPage(this));

    setStartId(Welcome);
}

void InstallerWizard::loadBanner()
{
    // Try bannerImage from manifest, then bundle Resources/, then fallback placeholder
    QStringList candidates;

    if (!m_manifest.theme.bannerImage.isEmpty())
        candidates << m_manifest.theme.bannerImage
                   << m_payloadDir + "/../" + m_manifest.theme.bannerImage;

    // Standard bundle resource path
    candidates << QCoreApplication::applicationDirPath() + "/../Resources/banner.png"
               << QCoreApplication::applicationDirPath() + "/banner.png";

    for (const QString &p : candidates) {
        if (QFile::exists(p) && m_banner.load(p)) {
            m_banner = m_banner.scaledToHeight(72, Qt::SmoothTransformation);
            return;
        }
    }

    // No banner found — generate a simple gradient placeholder
    m_banner = QPixmap(740, 72);
    m_banner.fill(QColor("#0f3460"));
}
