/*
 * SecurityEditor.cpp — Code signing and security settings
 */
#include "SecurityEditor.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QScrollArea>

SecurityEditor::SecurityEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void SecurityEditor::load(const Manifest &/*m*/)
{
    // Phase 1: security settings stored in manifest theme/app extension (Phase 2)
    // For now populate with sensible defaults
    m_macosHarden ->setChecked(true);
    m_macosNotarize->setChecked(false);
    m_linuxSignDebs->setChecked(false);
}

void SecurityEditor::save(Manifest &/*m*/) const
{
    // Phase 2: serialize to manifest extended fields
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
