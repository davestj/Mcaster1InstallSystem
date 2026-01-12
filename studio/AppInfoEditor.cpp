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
#include <QScrollArea>

// ── Constructor ───────────────────────────────────────────────────────────────
AppInfoEditor::AppInfoEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void AppInfoEditor::load(const Manifest &m)
{
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
    // Outer scroll area so form doesn't clip on small windows
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
        auto *grp = new QGroupBox("App Metadata", container);
        auto *form = new QFormLayout(grp);
        form->setRowWrapPolicy(QFormLayout::DontWrapRows);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_name        = new QLineEdit(grp);  m_name->setPlaceholderText("My Application");
        m_version     = new QLineEdit(grp);  m_version->setPlaceholderText("1.0.0");
        m_publisher   = new QLineEdit(grp);  m_publisher->setPlaceholderText("ACME Corp");
        m_identifier  = new QLineEdit(grp);  m_identifier->setPlaceholderText("com.example.myapp");
        m_url         = new QLineEdit(grp);  m_url->setPlaceholderText("https://example.com");
        m_supportUrl  = new QLineEdit(grp);  m_supportUrl->setPlaceholderText("https://support.example.com");
        m_description = new QTextEdit(grp);  m_description->setFixedHeight(72);
                                             m_description->setPlaceholderText("Brief application description.");
        m_iconPath    = new QLineEdit(grp);  m_iconPath->setPlaceholderText("resources/myapp.icns");
        m_licenseFile = new QLineEdit(grp);  m_licenseFile->setPlaceholderText("LICENSE.txt");

        form->addRow("Name:",        m_name);
        form->addRow("Version:",     m_version);
        form->addRow("Publisher:",   m_publisher);
        form->addRow("Identifier:",  m_identifier);
        form->addRow("URL:",         m_url);
        form->addRow("Support URL:", m_supportUrl);
        form->addRow("Description:", m_description);
        form->addRow("Icon:",        m_iconPath);
        form->addRow("License:",     m_licenseFile);

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

        form->addRow("macOS:",   m_dirMacos);
        form->addRow("Windows:", m_dirWindows);
        form->addRow("Linux:",   m_dirLinux);

        auto *hint = new QLabel("Use {publisher} and {name} tokens.", grp);
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

        hbox->addWidget(m_tgtMacos);
        hbox->addWidget(m_tgtWindows);
        hbox->addWidget(m_tgtLinux);
        hbox->addStretch();

        vbox->addWidget(grp);
    }

    // ── Install options ───────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("Install Options", container);
        auto *vl   = new QVBoxLayout(grp);

        m_requireAdmin    = new QCheckBox("Require administrator / root privileges",    grp);
        m_allowCustomDir  = new QCheckBox("Allow user to change install directory",     grp);
        m_launchAfter     = new QCheckBox("Launch application after install completes", grp);
        m_createUninstall = new QCheckBox("Create uninstaller",                         grp);

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

        form->addRow("Accent color:",     m_accentColor);
        form->addRow("Background color:", m_bgColor);
        form->addRow("Banner image:",     m_bannerImage);
        form->addRow("Side panel image:", m_sidePanelImage);
        form->addRow("",                  m_darkMode);

        vbox->addWidget(grp);
    }

    vbox->addStretch();
}
