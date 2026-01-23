/*
 * DirectoryPage.cpp — Install directory picker
 */
#include "DirectoryPage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QStorageInfo>

DirectoryPage::DirectoryPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Installation Folder");
    setSubTitle("Select where the application will be installed.");

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 16, 0, 0);
    vbox->setSpacing(8);

    auto *folderLbl = new QLabel("Install to:", this);
    vbox->addWidget(folderLbl);

    auto *row = new QHBoxLayout;
    m_dirEdit = new QLineEdit(this);
    row->addWidget(m_dirEdit, 1);

    auto *browseBtn = new QPushButton("Browse…", this);
    browseBtn->setFixedWidth(80);
    connect(browseBtn, &QPushButton::clicked, this, &DirectoryPage::onBrowse);
    row->addWidget(browseBtn);

    vbox->addLayout(row);

    m_spaceLabel = new QLabel(this);
    m_spaceLabel->setObjectName("hintLabel");
    vbox->addWidget(m_spaceLabel);

    // ── Per-platform install directory guidance ───────────────────────────
    vbox->addSpacing(16);

#if defined(Q_OS_MACOS)
    auto *guideTitle = new QLabel("macOS Install Directory Guide", this);
    guideTitle->setObjectName("sectionLabel");
    guideTitle->setStyleSheet("font-weight:600; color:#00c9ff; margin-top:4px;");
    vbox->addWidget(guideTitle);
    auto *guide = new QLabel(
        "<b>/Applications/{Name}</b> — system-wide, requires admin (recommended)<br>"
        "<b>~/Applications/{Name}</b> — current user only, no admin required<br>"
        "<b>/Applications/{Publisher}/{Name}</b> — grouped under publisher folder",
        this);
#elif defined(Q_OS_WIN)
    auto *guideTitle = new QLabel("Windows Install Directory Guide", this);
    guideTitle->setObjectName("sectionLabel");
    guideTitle->setStyleSheet("font-weight:600; color:#00c9ff; margin-top:4px;");
    vbox->addWidget(guideTitle);
    auto *guide = new QLabel(
        "<b>C:\\Program Files\\{Publisher}\\{Name}</b> — 64-bit apps, requires admin<br>"
        "<b>C:\\Program Files (x86)\\{Publisher}\\{Name}</b> — 32-bit apps, requires admin<br>"
        "<b>%LOCALAPPDATA%\\{Publisher}\\{Name}</b> — current user, no admin required<br>"
        "<b>%APPDATA%\\{Publisher}\\{Name}</b> — roaming profile (syncs across machines)",
        this);
#else
    auto *guideTitle = new QLabel("Linux Install Directory Guide", this);
    guideTitle->setObjectName("sectionLabel");
    guideTitle->setStyleSheet("font-weight:600; color:#00c9ff; margin-top:4px;");
    vbox->addWidget(guideTitle);
    auto *guide = new QLabel(
        "<b>/opt/{publisher}</b> — FHS-compliant vendor software, requires root (recommended)<br>"
        "<b>/usr/local</b> — traditional manual install area, requires root<br>"
        "<b>~/.local/share/{publisher}/{name}</b> — XDG user install, no root required",
        this);
#endif
    guide->setObjectName("hintLabel");
    guide->setWordWrap(true);
    guide->setTextFormat(Qt::RichText);
    vbox->addWidget(guide);

    vbox->addStretch();

    // registerField with * makes the field mandatory (non-empty) — QWizard reads
    // the QLineEdit::text property automatically
    registerField("installDir*", m_dirEdit);

    connect(m_dirEdit, &QLineEdit::textChanged, this, [this]() {
        // Update available space hint whenever path changes
        QStorageInfo si(m_dirEdit->text());
        if (si.isValid()) {
            const qint64 gb = si.bytesFree() / (1024LL * 1024 * 1024);
            const qint64 mb = (si.bytesFree() % (1024LL * 1024 * 1024)) / (1024LL * 1024);
            m_spaceLabel->setText(QString("%1 GB (%2 MB) available on this volume")
                                      .arg(gb).arg(mb));
        } else {
            m_spaceLabel->setText("(Path does not exist yet — it will be created.)");
        }
    });
}

void DirectoryPage::initializePage()
{
    if (!m_dirEdit->text().isEmpty()) return;  // user already typed something

    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    // Resolve the per-platform default, expanding {name} and {publisher}
    const Manifest &m = wiz->manifest();

#if defined(Q_OS_MAC)
    const QString platform = "macos";
    QString dir = m.defaults.installDirMacos;
#elif defined(Q_OS_WIN)
    const QString platform = "windows";
    QString dir = m.defaults.installDirWindows;
#else
    const QString platform = "linux";
    QString dir = m.defaults.installDirLinux;
    Q_UNUSED(platform)
#endif

    // Simple token expansion (no installDir token here — it IS the installDir)
    dir.replace("{name}",      m.app.name);
    dir.replace("{publisher}", m.app.publisher);
    dir.replace("{version}",   m.app.version);

    m_dirEdit->setText(QDir::toNativeSeparators(dir));
}

void DirectoryPage::onBrowse()
{
    const QString current = m_dirEdit->text().trimmed();
    const QString chosen  = QFileDialog::getExistingDirectory(
        this, "Choose Installation Folder",
        current.isEmpty() ? QDir::homePath() : current,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!chosen.isEmpty())
        m_dirEdit->setText(QDir::toNativeSeparators(chosen));
}
