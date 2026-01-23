/*
 * BuilderProfileDialog.cpp — Create / edit a BuilderProfile
 */

#include "BuilderProfileDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QUuid>

// ── Constructor ───────────────────────────────────────────────────────────────
BuilderProfileDialog::BuilderProfileDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Builder Profile");
    setMinimumWidth(520);
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void BuilderProfileDialog::setProfile(const BuilderProfile &p)
{
    m_id = p.id;
    m_displayName   ->setText(p.displayName);
    m_company       ->setText(p.company);
    m_publisherName ->setText(p.publisherName);
    m_email         ->setText(p.email);
    m_website       ->setText(p.website);
    m_supportUrl    ->setText(p.supportUrl);
    m_chkSkipSigning->setChecked(p.skipSigning);
    m_chkDevSign    ->setChecked(p.devSignMode);
    m_macosSigningId->setText(p.macosSigningId);
    m_winCert       ->setText(p.windowsSigningCert);
    m_gpgKey        ->setText(p.linuxGpgKeyId);

    QStringList tok;
    for (auto it = p.customTokens.constBegin(); it != p.customTokens.constEnd(); ++it)
        tok << it.key() + "=" + it.value();
    m_customTokens->setPlainText(tok.join('\n'));
}

BuilderProfile BuilderProfileDialog::profile() const
{
    BuilderProfile p;
    p.id             = m_id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : m_id;
    p.displayName    = m_displayName   ->text().trimmed();
    p.company        = m_company       ->text().trimmed();
    p.publisherName  = m_publisherName ->text().trimmed();
    p.email          = m_email         ->text().trimmed();
    p.website        = m_website       ->text().trimmed();
    p.supportUrl     = m_supportUrl    ->text().trimmed();
    p.skipSigning    = m_chkSkipSigning->isChecked();
    p.devSignMode    = m_chkDevSign    ->isChecked();
    p.macosSigningId = m_macosSigningId->text().trimmed();
    p.windowsSigningCert = m_winCert   ->text().trimmed();
    p.linuxGpgKeyId  = m_gpgKey        ->text().trimmed();

    for (const QString &line : m_customTokens->toPlainText().split('\n', Qt::SkipEmptyParts)) {
        const int eq = line.indexOf('=');
        if (eq > 0)
            p.customTokens[line.left(eq).trimmed()] = line.mid(eq + 1).trimmed();
    }
    return p;
}

// ── Private ───────────────────────────────────────────────────────────────────
void BuilderProfileDialog::buildUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setSpacing(12);

    auto *tabs = new QTabWidget(this);

    // ── Identity tab ─────────────────────────────────────────────────────────
    {
        auto *w    = new QWidget(tabs);
        auto *form = new QFormLayout(w);
        form->setContentsMargins(16, 16, 16, 16);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_displayName  = new QLineEdit(w); m_displayName->setPlaceholderText("My Company (personal)");
        m_company      = new QLineEdit(w); m_company->setPlaceholderText("ACME Corp");
        m_publisherName= new QLineEdit(w); m_publisherName->setPlaceholderText("AcmeCorp");
        m_email        = new QLineEdit(w); m_email->setPlaceholderText("contact@example.com");
        m_website      = new QLineEdit(w); m_website->setPlaceholderText("https://example.com");
        m_supportUrl   = new QLineEdit(w); m_supportUrl->setPlaceholderText("https://support.example.com");

        m_displayName  ->setToolTip("Label shown in the profile selector dropdown.");
        m_company      ->setToolTip("Full company or organization name.");
        m_publisherName->setToolTip("Short publisher name used in install paths (no spaces).");
        m_email        ->setToolTip("Contact e-mail address.");
        m_website      ->setToolTip("Company / product homepage URL.");
        m_supportUrl   ->setToolTip("Support page URL.");

        form->addRow("Profile name:", m_displayName);
        form->addRow("Company:",      m_company);
        form->addRow("Publisher:",    m_publisherName);
        form->addRow("E-mail:",       m_email);
        form->addRow("Website:",      m_website);
        form->addRow("Support URL:",  m_supportUrl);

        tabs->addTab(w, "Identity");
    }

    // ── Code Signing tab ─────────────────────────────────────────────────────
    {
        auto *w    = new QWidget(tabs);
        auto *vbox = new QVBoxLayout(w);
        vbox->setContentsMargins(16, 16, 16, 16);
        vbox->setSpacing(12);

        // ── Signing mode ─────────────────────────────────────────────────────
        auto *modeGrp  = new QGroupBox("Signing Mode", w);
        auto *modeVbox = new QVBoxLayout(modeGrp);
        modeVbox->setSpacing(6);

        m_chkSkipSigning = new QCheckBox(
            "Skip code signing entirely  (unsigned — internal testing only)", modeGrp);
        m_chkSkipSigning->setToolTip(
            "No signing will be attempted on any platform.\n"
            "Do not distribute unsigned release builds publicly.");

        m_chkDevSign = new QCheckBox(
            "Use development (ad-hoc) signing  — dev / testing mode", modeGrp);
        m_chkDevSign->setToolTip(
            "macOS:   codesign --force --sign -   (ad-hoc, no Apple certificate needed)\n"
            "Windows: skip signtool.exe\n"
            "Linux:   skip GPG package signing\n\n"
            "Builds run locally but cannot be notarized or publicly distributed.");

        modeVbox->addWidget(m_chkSkipSigning);
        modeVbox->addWidget(m_chkDevSign);
        vbox->addWidget(modeGrp);

        // skipSigning disables all other signing controls
        connect(m_chkSkipSigning, &QCheckBox::checkStateChanged, this,
                [this](Qt::CheckState s) {
            const bool skip = (s == Qt::Checked);
            m_chkDevSign    ->setEnabled(!skip);
            m_macosSigningId->setEnabled(!skip);
            m_winCert       ->setEnabled(!skip);
            m_gpgKey        ->setEnabled(!skip);
        });
        // devSign disables identity fields (no cert needed for ad-hoc)
        connect(m_chkDevSign, &QCheckBox::checkStateChanged, this,
                [this](Qt::CheckState s) {
            const bool dev = (s == Qt::Checked);
            m_macosSigningId->setEnabled(!dev);
            m_winCert       ->setEnabled(!dev);
            m_gpgKey        ->setEnabled(!dev);
        });

        // macOS
        auto *macGrp  = new QGroupBox("macOS Code Signing", w);
        auto *macForm = new QFormLayout(macGrp);
        macForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        macForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_macosSigningId = new QLineEdit(macGrp);
        m_macosSigningId->setPlaceholderText("Developer ID Application: ACME Corp (XXXXXXXXXX)");
        m_macosSigningId->setToolTip(
            "macOS signing identity string.\n"
            "Run: security find-identity -v -p codesigning\n"
            "to list valid identities in your keychain.");
        macForm->addRow("Signing identity:", m_macosSigningId);
        vbox->addWidget(macGrp);

        // Windows
        auto *winGrp  = new QGroupBox("Windows Code Signing", w);
        auto *winForm = new QFormLayout(winGrp);
        winForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        winForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_winCert = new QLineEdit(winGrp);
        m_winCert->setPlaceholderText("path/to/cert.pfx  or  SHA1:AABBCCDD...");
        m_winCert->setToolTip(
            "Path to a .pfx / .p12 certificate, or the SHA-1 thumbprint of a\n"
            "certificate already installed in the Windows certificate store.");
        winForm->addRow("Certificate:", m_winCert);
        vbox->addWidget(winGrp);

        // Linux
        auto *linGrp  = new QGroupBox("Linux GPG Signing", w);
        auto *linForm = new QFormLayout(linGrp);
        linForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        linForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_gpgKey = new QLineEdit(linGrp);
        m_gpgKey->setPlaceholderText("0xDEADBEEF or full fingerprint");
        m_gpgKey->setToolTip("GPG key ID or full fingerprint used to sign packages.");
        linForm->addRow("GPG key:", m_gpgKey);
        vbox->addWidget(linGrp);

        vbox->addStretch();
        tabs->addTab(w, "Code Signing");
    }

    // ── Custom Tokens tab ────────────────────────────────────────────────────
    {
        auto *w    = new QWidget(tabs);
        auto *vbox = new QVBoxLayout(w);
        vbox->setContentsMargins(16, 16, 16, 16);
        vbox->setSpacing(8);

        auto *hint = new QLabel(
            "Define custom tokens that can be used in installer fields.\n"
            "Format: one  <b>key=value</b>  pair per line.\n"
            "Example: <b>department=Engineering</b>", w);
        hint->setTextFormat(Qt::RichText);
        hint->setWordWrap(true);
        hint->setObjectName("hintLabel");
        vbox->addWidget(hint);

        m_customTokens = new QPlainTextEdit(w);
        m_customTokens->setPlaceholderText("department=Engineering\ncostCenter=CC-1234");
        m_customTokens->setToolTip("Custom key=value tokens. Use {key} in installer fields to substitute.");
        vbox->addWidget(m_customTokens);

        tabs->addTab(w, "Custom Tokens");
    }

    outer->addWidget(tabs);

    // ── Buttons ───────────────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(btns);
}
