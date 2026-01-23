/*
 * SecurityEditor.cpp — Code signing and security settings
 */
#include "SecurityEditor.h"
#include "CodeSignDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>

SecurityEditor::SecurityEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void SecurityEditor::load(const Manifest &m)
{
    m_manifest = m;  // retain for CodeSignDialog

    // macOS
    m_macosIdentity ->setText(m.signing.macosSigner);
    m_macosTeamId   ->setText(m.signing.macosTeamId);
    m_macosHarden   ->setChecked(m.signing.macosHardened);
    m_macosNotarize ->setChecked(m.signing.macosNotarize);
    m_macosProfile  ->setText(m.signing.macosProfile);

    // Windows
    m_winCertFile   ->setText(m.signing.winPfxPath);
    m_winCertPass   ->clear();          // never persist passwords
    m_winTimestamp  ->setText(m.signing.winTimestampUrl.isEmpty()
                              ? "http://timestamp.sectigo.com"
                              : m.signing.winTimestampUrl);

    // Linux
    m_linuxGpgKey   ->setText(m.signing.linuxGpgKey);
    m_linuxSignDebs ->setChecked(m.signing.linuxSignDebs);
}

void SecurityEditor::onManageCodeSigning()
{
    CodeSignDialog dlg(m_manifest, this);
    dlg.exec();
    // If user accepted, pull signing identity back into quick-view fields
    if (!dlg.macosIdentity().isEmpty() && !dlg.macosIdentity().startsWith("-  "))
        m_macosIdentity->setText(dlg.macosIdentity());
    if (!dlg.windowsPfxPath().isEmpty())
        m_winCertFile->setText(dlg.windowsPfxPath());
    if (!dlg.linuxGpgKeyId().isEmpty())
        m_linuxGpgKey->setText(dlg.linuxGpgKeyId());
}

void SecurityEditor::save(Manifest &m) const
{
    // macOS
    m.signing.macosSigner   = m_macosIdentity ->text().trimmed();
    m.signing.macosTeamId   = m_macosTeamId   ->text().trimmed();
    m.signing.macosHardened = m_macosHarden   ->isChecked();
    m.signing.macosNotarize = m_macosNotarize ->isChecked();
    m.signing.macosProfile  = m_macosProfile  ->text().trimmed();

    // Windows (never persist the PFX password — security risk)
    m.signing.winPfxPath      = m_winCertFile ->text().trimmed();
    m.signing.winTimestampUrl = m_winTimestamp->text().trimmed();

    // Linux
    m.signing.linuxGpgKey   = m_linuxGpgKey  ->text().trimmed();
    m.signing.linuxSignDebs = m_linuxSignDebs ->isChecked();
}

void SecurityEditor::buildUi()
{
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

    auto *titleLbl = new QLabel("Security & Code Signing", container);
    titleLbl->setObjectName("titleLabel");
    vbox->addWidget(titleLbl);

    // ── Full Code Signing Manager ──────────────────────────────────────────
    {
        auto *row     = new QHBoxLayout;
        auto *csMgrBtn = new QPushButton("⚙  Manage Code Signing…", container);
        csMgrBtn->setToolTip(
            "Open the full Code Signing Manager:\n"
            "  • Sign .app/.dmg/.exe/.deb/.rpm/.AppImage\n"
            "  • Generate self-signed certificates\n"
            "  • Create CSR for commercial CA submission\n"
            "  • Import PFX into macOS Keychain\n"
            "  • Certificate Authority guidance");
        connect(csMgrBtn, &QPushButton::clicked, this, &SecurityEditor::onManageCodeSigning);
        row->addWidget(csMgrBtn);
        row->addStretch();
        vbox->addLayout(row);

        auto *subHint = new QLabel(
            "Quick-view fields below are synced from the Code Signing Manager.", container);
        subHint->setObjectName("hintLabel");
        vbox->addWidget(subHint);
    }

    // ── macOS ─────────────────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("macOS Code Signing", container);
        auto *form = new QFormLayout(grp);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_macosIdentity = new QLineEdit(grp);
        m_macosIdentity->setPlaceholderText("Developer ID Application: ACME Corp (XXXXXXXXXX)");
        m_macosTeamId   = new QLineEdit(grp);
        m_macosTeamId  ->setPlaceholderText("XXXXXXXXXX");
        m_macosHarden   = new QCheckBox("Enable hardened runtime (required for notarization)", grp);
        m_macosNotarize = new QCheckBox("Notarize with Apple (requires Apple Developer account)", grp);
        m_macosProfile  = new QLineEdit(grp);
        m_macosProfile ->setPlaceholderText("notarytool profile name (xcrun notarytool store-credentials)");

        form->addRow("Signing Identity:", m_macosIdentity);
        form->addRow("Team ID:",          m_macosTeamId);
        form->addRow("",                  m_macosHarden);
        form->addRow("",                  m_macosNotarize);
        form->addRow("Notarytool Profile:", m_macosProfile);

        auto *hint = new QLabel(
            "Leave blank to use ad-hoc signing (codesign --sign -). "
            "Ad-hoc signing works on local machine but not for distribution.", grp);
        hint->setObjectName("hintLabel");
        hint->setWordWrap(true);
        form->addRow("", hint);

        vbox->addWidget(grp);
    }

    // ── Windows ───────────────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("Windows Authenticode Signing", container);
        auto *form = new QFormLayout(grp);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_winCertFile  = new QLineEdit(grp);
        m_winCertFile ->setPlaceholderText("path/to/certificate.pfx");
        m_winCertPass  = new QLineEdit(grp);
        m_winCertPass ->setEchoMode(QLineEdit::Password);
        m_winCertPass ->setPlaceholderText("PFX password");
        m_winTimestamp = new QLineEdit(grp);
        m_winTimestamp->setPlaceholderText("http://timestamp.sectigo.com");

        form->addRow("PFX Certificate:", m_winCertFile);
        form->addRow("PFX Password:",    m_winCertPass);
        form->addRow("Timestamp URL:",   m_winTimestamp);

        vbox->addWidget(grp);
    }

    // ── Linux ─────────────────────────────────────────────────────────────
    {
        auto *grp  = new QGroupBox("Linux GPG Signing", container);
        auto *form = new QFormLayout(grp);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_linuxGpgKey  = new QLineEdit(grp);
        m_linuxGpgKey ->setPlaceholderText("GPG Key ID or fingerprint");
        m_linuxSignDebs = new QCheckBox("Sign .deb packages with GPG", grp);

        form->addRow("GPG Key:", m_linuxGpgKey);
        form->addRow("",        m_linuxSignDebs);

        vbox->addWidget(grp);
    }

    vbox->addStretch();
}
