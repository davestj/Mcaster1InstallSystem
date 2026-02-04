/*
 * AppInfoEditor.cpp — App metadata form editor implementation
 */

#include "AppInfoEditor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFileDialog>
#include <QColorDialog>
#include <QColor>
#include <QFrame>

// ── Helper implementations ────────────────────────────────────────────────────

/**
 * Wraps a QLineEdit in an [edit | …] row that opens a file or directory picker.
 * The returned QWidget can be passed directly to QFormLayout::addRow().
 */
QWidget *AppInfoEditor::makePathRow(QLineEdit *edit, QWidget *parent,
                                     bool isDir, const QString &filter)
{
    auto *row = new QWidget(parent);
    auto *h   = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(4);
    h->addWidget(edit);

    auto *btn = new QPushButton("…", row);
    btn->setFixedWidth(28);
    btn->setToolTip(isDir ? "Browse for directory…" : "Browse for file…");
    h->addWidget(btn);

    if (isDir) {
        connect(btn, &QPushButton::clicked, this, [this, edit] {
            QString path = QFileDialog::getExistingDirectory(
                this, "Select Directory",
                edit->text().isEmpty() ? QDir::homePath() : edit->text());
            if (!path.isEmpty()) edit->setText(path);
        });
    } else {
        connect(btn, &QPushButton::clicked, this, [this, edit, filter] {
            QString path = QFileDialog::getOpenFileName(
                this, "Select File",
                edit->text().isEmpty() ? QDir::homePath() : QFileInfo(edit->text()).absolutePath(),
                filter.isEmpty() ? "All files (*)" : filter);
            if (!path.isEmpty()) edit->setText(path);
        });
    }

    return row;
}

/**
 * Wraps a QLineEdit in a [edit | ■] row where ■ is a color swatch button
 * that opens QColorDialog and writes the hex value back to the edit.
 * btnOut receives the button pointer so load() can refresh the swatch.
 */
QWidget *AppInfoEditor::makeColorRow(QLineEdit *edit, QPushButton *&btnOut, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *h   = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(4);
    h->addWidget(edit);

    btnOut = new QPushButton(row);
    btnOut->setFixedSize(28, 22);
    btnOut->setToolTip("Pick color…");
    h->addWidget(btnOut);

    // Helper lambda: update swatch background from hex string
    auto updateSwatch = [btnOut](const QString &hex) {
        QColor c(hex);
        if (c.isValid())
            btnOut->setStyleSheet(
                QString("background:%1; border:1px solid #555; border-radius:3px;").arg(c.name()));
        else
            btnOut->setStyleSheet("border:1px solid #555; border-radius:3px;");
    };

    // Sync swatch when user types a hex value
    connect(edit, &QLineEdit::textChanged, this, [updateSwatch](const QString &t) {
        updateSwatch(t);
    });

    // Open color picker on button click
    connect(btnOut, &QPushButton::clicked, this, [this, edit, updateSwatch] {
        QColor initial(edit->text());
        QColor chosen = QColorDialog::getColor(
            initial.isValid() ? initial : QColor("#00c9ff"),
            this, "Pick Color");
        if (chosen.isValid()) {
            edit->setText(chosen.name().toLower());
            updateSwatch(chosen.name().toLower());
        }
    });

    return row;
}

// ── Constructor ───────────────────────────────────────────────────────────────
AppInfoEditor::AppInfoEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void AppInfoEditor::load(const Manifest &m)
{
    // Block signals during load so we don't emit modified() for programmatic updates
    QSignalBlocker blocker(this);

    m_name       ->setText(m.app.name);
    m_version    ->setText(m.app.version);
    m_publisher  ->setText(m.app.publisher);
    m_identifier ->setText(m.app.identifier);
    m_url        ->setText(m.app.url);
    m_supportUrl ->setText(m.app.supportUrl);
    m_description->setPlainText(m.app.description);
    m_iconPath   ->setText(m.app.iconPath);
    m_licenseFile->setText(m.app.licenseFile);

    m_dirMacos  ->setText(m.defaults.installDirMacos);
    m_dirWindows->setText(m.defaults.installDirWindows);
    m_dirLinux  ->setText(m.defaults.installDirLinux);
    m_requireAdmin   ->setChecked(m.defaults.requireAdmin);
    m_allowCustomDir ->setChecked(m.defaults.allowCustomDir);
    m_launchAfter    ->setChecked(m.defaults.launchAfter);
    m_createUninstall->setChecked(m.defaults.createUninstaller);

    m_tgtMacos  ->setChecked(m.targets.contains("macos"));
    m_tgtWindows->setChecked(m.targets.contains("windows"));
    m_tgtLinux  ->setChecked(m.targets.contains("linux"));

    m_accentColor   ->setText(m.theme.accentColor);
    m_bgColor       ->setText(m.theme.backgroundColor);
    m_bannerImage   ->setText(m.theme.bannerImage);
    m_sidePanelImage->setText(m.theme.sidePanelImage);
    m_darkMode      ->setChecked(m.theme.darkMode);

    // Refresh color swatches after setting text values
    auto refreshSwatch = [](QPushButton *btn, const QString &hex) {
        QColor c(hex);
        if (c.isValid())
            btn->setStyleSheet(
                QString("background:%1; border:1px solid #555; border-radius:3px;").arg(c.name()));
    };
    refreshSwatch(m_accentColorBtn, m.theme.accentColor);
    refreshSwatch(m_bgColorBtn,     m.theme.backgroundColor);
}

void AppInfoEditor::save(Manifest &m) const
{
    m.app.name        = m_name       ->text().trimmed();
    m.app.version     = m_version    ->text().trimmed();
    m.app.publisher   = m_publisher  ->text().trimmed();
    m.app.identifier  = m_identifier ->text().trimmed();
    m.app.url         = m_url        ->text().trimmed();
    m.app.supportUrl  = m_supportUrl ->text().trimmed();
    m.app.description = m_description->toPlainText().trimmed();
    m.app.iconPath    = m_iconPath   ->text().trimmed();
    m.app.licenseFile = m_licenseFile->text().trimmed();

    m.defaults.installDirMacos    = m_dirMacos  ->text().trimmed();
    m.defaults.installDirWindows  = m_dirWindows->text().trimmed();
    m.defaults.installDirLinux    = m_dirLinux  ->text().trimmed();
    m.defaults.requireAdmin       = m_requireAdmin   ->isChecked();
    m.defaults.allowCustomDir     = m_allowCustomDir ->isChecked();
    m.defaults.launchAfter        = m_launchAfter    ->isChecked();
    m.defaults.createUninstaller  = m_createUninstall->isChecked();

    m.targets.clear();
    if (m_tgtMacos  ->isChecked()) m.targets << "macos";
    if (m_tgtWindows->isChecked()) m.targets << "windows";
    if (m_tgtLinux  ->isChecked()) m.targets << "linux";

    m.theme.accentColor     = m_accentColor   ->text().trimmed();
    m.theme.backgroundColor = m_bgColor       ->text().trimmed();
    m.theme.bannerImage     = m_bannerImage   ->text().trimmed();
    m.theme.sidePanelImage  = m_sidePanelImage->text().trimmed();
    m.theme.darkMode        = m_darkMode      ->isChecked();
}

// ── Private ───────────────────────────────────────────────────────────────────
void AppInfoEditor::buildUi()
{
    // Outer scroll area so the form doesn't clip on small windows
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget(scroll);
    scroll->setWidget(container);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    auto *vbox = new QVBoxLayout(container);
    vbox->setContentsMargins(24, 16, 24, 24);
    vbox->setSpacing(16);

    // ── Title ─────────────────────────────────────────────────────────────
    auto *titleLbl = new QLabel("Application Information", container);
    titleLbl->setObjectName("titleLabel");
    vbox->addWidget(titleLbl);

    // ── App Info group ────────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("App Metadata", container);
        auto *form = new QFormLayout(grp);
        form->setRowWrapPolicy(QFormLayout::DontWrapRows);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_name        = new QLineEdit(grp); m_name->setPlaceholderText("My Application");
        m_version     = new QLineEdit(grp); m_version->setPlaceholderText("1.0.0");
        m_publisher   = new QLineEdit(grp); m_publisher->setPlaceholderText("ACME Corp");
        m_identifier  = new QLineEdit(grp); m_identifier->setPlaceholderText("com.example.myapp");
        m_url         = new QLineEdit(grp); m_url->setPlaceholderText("https://example.com");
        m_supportUrl  = new QLineEdit(grp); m_supportUrl->setPlaceholderText("https://support.example.com");
        m_description = new QTextEdit(grp); m_description->setFixedHeight(72);
                                            m_description->setPlaceholderText("Brief application description.");
        m_iconPath    = new QLineEdit(grp); m_iconPath->setPlaceholderText("resources/myapp.icns");
        m_licenseFile = new QLineEdit(grp); m_licenseFile->setPlaceholderText("LICENSE.txt");

        m_name       ->setToolTip("Display name of your application as shown to end-users.");
        m_version    ->setToolTip("Semantic version number (e.g. 1.2.3). Used in installer filenames and About dialogs.");
        m_publisher  ->setToolTip("Your company or developer name. Used in install paths and registry entries.");
        m_identifier ->setToolTip("Reverse-DNS bundle identifier, e.g. com.company.appname. Required for macOS app bundles.");
        m_url        ->setToolTip("Product homepage URL shown on the Welcome page.");
        m_supportUrl ->setToolTip("Support or help URL shown on the Finish page after installation.");
        m_description->setToolTip("Short description shown on the Welcome page of the installer wizard.");
        m_iconPath   ->setToolTip("Path to the app icon file relative to the project root.\nmacOS: .icns  Windows: .ico  Linux: .png (256x256)");
        m_licenseFile->setToolTip("Path to the license text file shown on the License page.\nSupports .txt and .rtf formats.");

        // Icon: file browse row (icns/ico/png)
        form->addRow("Name:",        m_name);
        form->addRow("Version:",     m_version);
        form->addRow("Publisher:",   m_publisher);
        form->addRow("Identifier:",  m_identifier);
        form->addRow("URL:",         m_url);
        form->addRow("Support URL:", m_supportUrl);
        form->addRow("Description:", m_description);
        form->addRow("Icon:",        makePathRow(m_iconPath, grp, false,
            "Icon files (*.icns *.ico *.png *.svg);;All files (*)"));
        form->addRow("License:",     makePathRow(m_licenseFile, grp, false,
            "License files (*.txt *.rtf *.md *.html);;All files (*)"));

        auto *hint = new QLabel("Identifier must be reverse-DNS notation, e.g. com.company.appname", grp);
        hint->setObjectName("hintLabel");
        form->addRow("", hint);

        vbox->addWidget(grp);
    }

    // ── Default install dirs ──────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("Default Install Directories", container);
        auto *form = new QFormLayout(grp);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_dirMacos   = new QLineEdit(grp);
        m_dirWindows = new QLineEdit(grp);
        m_dirLinux   = new QLineEdit(grp);

        m_dirMacos->setToolTip(
            "Default macOS install directory.\n\n"
            "Common choices:\n"
            "  /Applications/{Name}   — system-wide, requires admin\n"
            "  ~/Applications/{Name}  — current user only\n\n"
            "Tip: Use {name}, {publisher} tokens.");
        m_dirWindows->setToolTip(
            "Default Windows install directory.\n\n"
            "Common choices:\n"
            "  C:\\Program Files\\{Publisher}\\{Name}  — 64-bit, requires admin\n"
            "  %LOCALAPPDATA%\\{Publisher}\\{Name}     — current user, no admin\n\n"
            "Tip: Use LOCALAPPDATA for apps that write to their install dir at runtime.");
        m_dirLinux->setToolTip(
            "Default Linux install directory.\n\n"
            "Common choices:\n"
            "  /opt/{publisher}                — FHS vendor, requires root\n"
            "  ~/.local/share/{publisher}      — XDG user install, no root needed");

        // Directory browse buttons for each platform install dir
        form->addRow("macOS:",   makePathRow(m_dirMacos,   grp, true));
        form->addRow("Windows:", m_dirWindows);   // Windows dirs can't be browsed from macOS
        form->addRow("Linux:",   m_dirLinux);

        auto *hint = new QLabel(
            "Use {name}, {publisher}, {version} tokens — resolved at install time.", grp);
        hint->setObjectName("hintLabel");
        form->addRow("", hint);

        vbox->addWidget(grp);
    }

    // ── Targets ───────────────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("Target Platforms", container);
        auto *hbox = new QHBoxLayout(grp);

        m_tgtMacos   = new QCheckBox("macOS",   grp);
        m_tgtWindows = new QCheckBox("Windows", grp);
        m_tgtLinux   = new QCheckBox("Linux",   grp);

        m_tgtMacos  ->setToolTip("Build a macOS .dmg installer");
        m_tgtWindows->setToolTip("Build a Windows installer package (manifest + payload + Mcaster1Installer.exe)");
        m_tgtLinux  ->setToolTip("Build a Linux .deb / AppImage installer");

        hbox->addWidget(m_tgtMacos);
        hbox->addWidget(m_tgtWindows);
        hbox->addWidget(m_tgtLinux);
        hbox->addStretch();

        vbox->addWidget(grp);
    }

    // ── Install options ───────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox("Install Options", container);
        auto *vl  = new QVBoxLayout(grp);

        m_requireAdmin    = new QCheckBox("Require administrator / root privileges",    grp);
        m_allowCustomDir  = new QCheckBox("Allow user to change install directory",     grp);
        m_launchAfter     = new QCheckBox("Launch application after install completes", grp);
        m_createUninstall = new QCheckBox("Create uninstaller",                         grp);

        m_requireAdmin   ->setToolTip("Installer will request elevation (UAC on Windows, sudo on Linux).");
        m_allowCustomDir ->setToolTip("Enables the Directory page in the installer wizard.");
        m_launchAfter    ->setToolTip("Checked = 'Launch now' checkbox is pre-ticked on Finish page.");
        m_createUninstall->setToolTip("Writes an uninstall manifest and optionally registers in Add/Remove Programs.");

        vl->addWidget(m_requireAdmin);
        vl->addWidget(m_allowCustomDir);
        vl->addWidget(m_launchAfter);
        vl->addWidget(m_createUninstall);

        vbox->addWidget(grp);
    }

    // ── Wizard Theme ──────────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("Wizard Theme", container);
        auto *form = new QFormLayout(grp);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_accentColor    = new QLineEdit(grp); m_accentColor->setPlaceholderText("#00c9ff");
        m_bgColor        = new QLineEdit(grp); m_bgColor->setPlaceholderText("#1a1a2e");
        m_bannerImage    = new QLineEdit(grp); m_bannerImage->setPlaceholderText("resources/banner.png");
        m_sidePanelImage = new QLineEdit(grp); m_sidePanelImage->setPlaceholderText("resources/sidepanel.png");
        m_darkMode       = new QCheckBox("Dark mode wizard", grp);

        m_accentColor   ->setToolTip("Primary accent color as a hex value (#RRGGBB). Used for buttons and highlights.");
        m_bgColor       ->setToolTip("Wizard background color as a hex value (#RRGGBB).");
        m_bannerImage   ->setToolTip("Top banner image path (recommended: 700×90 px PNG).");
        m_sidePanelImage->setToolTip("Left side panel image path (recommended: 160×300 px PNG).");

        // Color picker rows for accent and bg
        form->addRow("Accent color:",     makeColorRow(m_accentColor, m_accentColorBtn, grp));
        form->addRow("Background color:", makeColorRow(m_bgColor,     m_bgColorBtn,     grp));
        form->addRow("Banner image:",     makePathRow(m_bannerImage,    grp, false,
            "Images (*.png *.jpg *.jpeg *.bmp);;All files (*)"));
        form->addRow("Side panel image:", makePathRow(m_sidePanelImage, grp, false,
            "Images (*.png *.jpg *.jpeg *.bmp);;All files (*)"));
        form->addRow("", m_darkMode);

        vbox->addWidget(grp);
    }

    vbox->addStretch();

    // ── Wire field-change signals ─────────────────────────────────────────
    // All editable fields → onAnyFieldChanged (emits modified())
    // Name + Version specifically also drive projectInfoChanged() for the status bar.
    const auto connectLine = [this](QLineEdit *le) {
        connect(le, &QLineEdit::textChanged, this, &AppInfoEditor::onAnyFieldChanged);
    };
    connectLine(m_name);         connectLine(m_version);
    connectLine(m_publisher);    connectLine(m_identifier);
    connectLine(m_url);          connectLine(m_supportUrl);
    connectLine(m_iconPath);     connectLine(m_licenseFile);
    connectLine(m_dirMacos);     connectLine(m_dirWindows);  connectLine(m_dirLinux);
    connectLine(m_accentColor);  connectLine(m_bgColor);
    connectLine(m_bannerImage);  connectLine(m_sidePanelImage);

    connect(m_description, &QTextEdit::textChanged, this, &AppInfoEditor::onAnyFieldChanged);

    const auto connectCheck = [this](QCheckBox *cb) {
        connect(cb, &QCheckBox::checkStateChanged, this, [this](auto) { onAnyFieldChanged(); });
    };
    connectCheck(m_tgtMacos);       connectCheck(m_tgtWindows);    connectCheck(m_tgtLinux);
    connectCheck(m_requireAdmin);   connectCheck(m_allowCustomDir);
    connectCheck(m_launchAfter);    connectCheck(m_createUninstall);
    connectCheck(m_darkMode);
}

// ── Sidebar cross-sync helper ─────────────────────────────────────────────────
void AppInfoEditor::setAppName(const QString &name)
{
    QSignalBlocker block(m_name);
    m_name->setText(name);
}

// ── Slot: any field changed ───────────────────────────────────────────────────
void AppInfoEditor::onAnyFieldChanged()
{
    emit modified();

    // Update status bar live when app name or version changes
    emit projectInfoChanged(
        m_name   ->text().trimmed(),
        m_version->text().trimmed());
}
