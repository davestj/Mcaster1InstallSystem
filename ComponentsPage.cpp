#include "ComponentsPage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFont>
#include <QDir>
#include <QFileInfo>

// ── helpers ──────────────────────────────────────────────────────────────────
static QString sizeStr(qint64 bytes)
{
    if (bytes >= 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 1) + " GB";
    return QString::number(bytes / (1024.0 * 1024), 'f', 0) + " MB";
}

static qint64 dirSize(const QString &path)
{
    qint64 total = 0;
    QDir d(path);
    for (const QFileInfo &fi : d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (fi.isDir()) total += dirSize(fi.absoluteFilePath());
        else            total += fi.size();
    }
    return total;
}

// ── ComponentsPage ────────────────────────────────────────────────────────────
ComponentsPage::ComponentsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Choose Components");
    setSubTitle("Select which parts of Mcaster1DNAS to install.");

    auto *root = new QVBoxLayout(this);
    root->setSpacing(6);

    // ── GUI Application ───────────────────────────────────────────────────────
    auto *grpGui = new QGroupBox(this);
    grpGui->setFlat(true);
    auto *guiLayout = new QVBoxLayout(grpGui);
    guiLayout->setContentsMargins(4, 2, 4, 2);

    m_chkGui = new QCheckBox("GUI Application  (ports 9033 / 9344)", this);
    m_chkGui->setChecked(true);
    m_chkGui->setEnabled(false);  // required — always installs
    QFont boldFont = m_chkGui->font();
    boldFont.setBold(true);
    m_chkGui->setFont(boldFont);

    m_lblGuiDetail = new QLabel(
        "  Native macOS Qt6 app with server controls, live stats, config manager,\n"
        "  SSL cert tools, and ICY 2.2 metadata support.\n"
        "  Runs on <b>localhost ports 9033 (HTTP) and 9344 (HTTPS)</b>.",
        this);
    m_lblGuiDetail->setTextFormat(Qt::RichText);
    m_lblGuiDetail->setWordWrap(true);
    QFont smallFont = m_lblGuiDetail->font();
    smallFont.setPointSize(smallFont.pointSize() - 1);
    m_lblGuiDetail->setFont(smallFont);

    guiLayout->addWidget(m_chkGui);
    guiLayout->addWidget(m_lblGuiDetail);
    root->addWidget(grpGui);

    // ── Background Service ────────────────────────────────────────────────────
    auto *grpSvc = new QGroupBox(this);
    grpSvc->setFlat(true);
    auto *svcLayout = new QVBoxLayout(grpSvc);
    svcLayout->setContentsMargins(4, 2, 4, 2);

    m_chkService = new QCheckBox("Background Service  (ports 9330 / 9443)", this);
    m_chkService->setChecked(false);
    m_chkService->setFont(boldFont);

    m_lblSvcDetail = new QLabel(
        "  Headless server binary registered as a macOS LaunchAgent. Starts automatically\n"
        "  at login and restarts on crash. Accessible from your network on\n"
        "  <b>ports 9330 (HTTP) and 9443 (HTTPS)</b>. Does not conflict with the GUI app.",
        this);
    m_lblSvcDetail->setTextFormat(Qt::RichText);
    m_lblSvcDetail->setWordWrap(true);
    m_lblSvcDetail->setFont(smallFont);

    svcLayout->addWidget(m_chkService);
    svcLayout->addWidget(m_lblSvcDetail);
    root->addWidget(grpSvc);

    // ── Shortcuts ─────────────────────────────────────────────────────────────
    m_chkShortcuts = new QCheckBox(
        "Shortcuts  (Launch.command, web bookmarks, uninstaller)", this);
    m_chkShortcuts->setChecked(true);
    root->addWidget(m_chkShortcuts);

    // ── Launch after install ──────────────────────────────────────────────────
    m_chkLaunch = new QCheckBox("Launch Mcaster1DNAS immediately after installation", this);
    m_chkLaunch->setChecked(true);
    root->addWidget(m_chkLaunch);

    root->addStretch();

    // ── Estimated size line ───────────────────────────────────────────────────
    m_lblSizeHint = new QLabel(this);
    m_lblSizeHint->setFont(smallFont);
    m_lblSizeHint->setAlignment(Qt::AlignRight);
    root->addWidget(m_lblSizeHint);

    // Wire up dependency logic
    connect(m_chkService,   &QCheckBox::checkStateChanged, this, &ComponentsPage::updateDependencies);
    connect(m_chkShortcuts, &QCheckBox::checkStateChanged, this, &ComponentsPage::updateDependencies);
    connect(m_chkLaunch,    &QCheckBox::checkStateChanged, this, &ComponentsPage::updateDependencies);
}

void ComponentsPage::initializePage()
{
    updateSizeHint();
}

void ComponentsPage::updateDependencies()
{
    updateSizeHint();
}

void ComponentsPage::updateSizeHint()
{
    // Estimate disk usage from payload directory
    qint64 guiSize = dirSize(InstallerWizard::payloadPath() + "/app");
    qint64 svcSize = dirSize(InstallerWizard::payloadPath() + "/service");

    qint64 total = guiSize;  // GUI is always installed
    if (m_chkService->isChecked()) total += svcSize;
    total += dirSize(InstallerWizard::payloadPath() + "/web");
    total += dirSize(InstallerWizard::payloadPath() + "/admin");

    m_lblSizeHint->setText(QString("Estimated install size:  %1").arg(sizeStr(total)));
}

bool ComponentsPage::validatePage()
{
    // Push the choices into the wizard's option struct
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return true;

    InstallOptions opts;
    opts.installGui       = true;                        // always
    opts.installService   = m_chkService->isChecked();
    opts.installShortcuts = m_chkShortcuts->isChecked();
    opts.launchAfter      = m_chkLaunch->isChecked();
    wiz->setOptions(opts);
    return true;
}

int ComponentsPage::nextId() const
{
    return PAGE_INSTALL;
}
