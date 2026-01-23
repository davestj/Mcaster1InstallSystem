/*
 * CodeSignDialog.cpp — Code Signing Management Dialog
 */
#include "CodeSignDialog.h"
#include "../backends/CodeSigner.h"
#include "../backends/CertGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include "SvgIcons.h"

static QIcon si(const char *svg, int sz = 16)
{
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}

CodeSignDialog::CodeSignDialog(const Manifest &m, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Code Signing Manager");
    setMinimumSize(680, 600);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(6);

    // ── Tab widget ────────────────────────────────────────────────────────────
    auto *tabs = new QTabWidget(this);
    buildMacosTab(tabs);
    buildWindowsTab(tabs);
    buildLinuxTab(tabs);
    buildGenerateTab(tabs);
    root->addWidget(tabs, 1);

    // ── Shared log panel ──────────────────────────────────────────────────────
    auto *logGrp = new QGroupBox("Operation Log", this);
    auto *logL   = new QVBoxLayout(logGrp);
    m_log = new QPlainTextEdit(logGrp);
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(120);
    m_log->setFont(QFont("Menlo", 10));
    logL->addWidget(m_log);
    root->addWidget(logGrp);

    // ── OK / Cancel ───────────────────────────────────────────────────────────
    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btnBox);

    // Pre-fill from manifest app info
    if (m_genCN)  m_genCN->setText(m.app.publisher + " Code Signing");
    if (m_genOrg) m_genOrg->setText(m.app.publisher);
    if (m_winDesc) m_winDesc->setText(m.app.name);
}

// ── macOS Tab ─────────────────────────────────────────────────────────────────

void CodeSignDialog::buildMacosTab(QTabWidget *tabs)
{
    auto *w    = new QWidget;
    auto *vbox = new QVBoxLayout(w);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);

    // Identity picker
    m_macIdentity = new QComboBox(w);
    auto *refreshBtn = new QPushButton(si(SvgIcons::kRefresh), "Refresh", w);
    connect(refreshBtn, &QPushButton::clicked, this, &CodeSignDialog::onRefreshIdentities);
    auto *idRow = new QHBoxLayout;
    idRow->addWidget(m_macIdentity, 1);
    idRow->addWidget(refreshBtn);
    form->addRow("Signing Identity:", idRow);
    onRefreshIdentities();  // populate on open

    // Entitlements
    m_macEntitlements = new QLineEdit(w);
    m_macEntitlements->setPlaceholderText("path/to/app.entitlements (optional)");
    auto *entBtn = new QPushButton(si(SvgIcons::kFolder), "", w);
    entBtn->setFixedWidth(28);
    connect(entBtn, &QPushButton::clicked, this, [this]() {
        const QString f = QFileDialog::getOpenFileName(this, "Entitlements File",
            QDir::homePath(), "Plist (*.entitlements *.plist);;All (*)");
        if (!f.isEmpty()) m_macEntitlements->setText(f);
    });
    auto *entRow = new QHBoxLayout;
    entRow->addWidget(m_macEntitlements, 1);
    entRow->addWidget(entBtn);
    form->addRow("Entitlements:", entRow);

    m_macHardened = new QCheckBox("Hardened Runtime (required for notarization)", w);
    m_macHardened->setChecked(true);
    form->addRow("", m_macHardened);

    // Target path
    m_macTargetPath = new QLineEdit(w);
    m_macTargetPath->setPlaceholderText("path/to/App.app  or  path/to/Installer.dmg");
    form->addRow("Target File:", m_macTargetPath);

    auto *signBtn = new QPushButton(si(SvgIcons::kSign), "Sign Now", w);
    connect(signBtn, &QPushButton::clicked, this, &CodeSignDialog::onMacosSignNow);
    form->addRow("", signBtn);

    vbox->addLayout(form);

    // Notarization group
    auto *notGrp = new QGroupBox("Apple Notarization", w);
    notGrp->setCheckable(true);
    notGrp->setChecked(false);
    auto *notForm = new QFormLayout(notGrp);

    m_macNotarize  = new QCheckBox("Run notarytool + stapler after signing", notGrp);
    notForm->addRow("", m_macNotarize);

    m_macAppleId   = new QLineEdit(notGrp);
    m_macAppleId->setPlaceholderText("your@apple.id");
    notForm->addRow("Apple ID:", m_macAppleId);

    m_macTeamId    = new QLineEdit(notGrp);
    m_macTeamId->setPlaceholderText("ABCD1234EF");
    notForm->addRow("Team ID:", m_macTeamId);

    m_macAppPw     = new QLineEdit(notGrp);
    m_macAppPw->setPlaceholderText("xxxx-xxxx-xxxx-xxxx (app-specific password)");
    m_macAppPw->setEchoMode(QLineEdit::Password);
    notForm->addRow("App Password:", m_macAppPw);

    auto *notBtn = new QPushButton(si(SvgIcons::kCheck), "Notarize + Staple", notGrp);
    connect(notBtn, &QPushButton::clicked, this, &CodeSignDialog::onMacosNotarizeNow);
    notForm->addRow("", notBtn);

    auto *notHint = new QLabel(
        "<small>Requires an Apple Developer Program membership ($99/yr).<br>"
        "Create an app-specific password at <a href='https://appleid.apple.com'>appleid.apple.com</a>.</small>", notGrp);
    notHint->setOpenExternalLinks(true);
    notHint->setWordWrap(true);
    notForm->addRow("", notHint);

    vbox->addWidget(notGrp);
    vbox->addStretch();
    tabs->addTab(w, "macOS");
}

// ── Windows Tab ───────────────────────────────────────────────────────────────

void CodeSignDialog::buildWindowsTab(QTabWidget *tabs)
{
    auto *w    = new QWidget;
    auto *vbox = new QVBoxLayout(w);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);

    // PFX cert
    m_winPfxPath = new QLineEdit(w);
    m_winPfxPath->setPlaceholderText("path/to/certificate.pfx");
    auto *pfxBtn = new QPushButton(si(SvgIcons::kFolder), "", w);
    pfxBtn->setFixedWidth(28);
    connect(pfxBtn, &QPushButton::clicked, this, &CodeSignDialog::onBrowsePfx);
    auto *pfxRow = new QHBoxLayout;
    pfxRow->addWidget(m_winPfxPath, 1);
    pfxRow->addWidget(pfxBtn);
    form->addRow("PFX / P12 Cert:", pfxRow);

    m_winPfxPw = new QLineEdit(w);
    m_winPfxPw->setEchoMode(QLineEdit::Password);
    m_winPfxPw->setPlaceholderText("certificate password");
    form->addRow("Password:", m_winPfxPw);

    m_winTimestamp = new QComboBox(w);
    m_winTimestamp->setEditable(true);
    m_winTimestamp->addItems({
        "http://timestamp.digicert.com",
        "http://timestamp.sectigo.com",
        "http://ts.ssl.com",
        "http://timestamp.globalsign.com/scripts/timestamp.dll",
        "http://rfc3161timestamp.globalsign.com/advanced"
    });
    form->addRow("Timestamp Server:", m_winTimestamp);

    m_winDesc = new QLineEdit(w);
    form->addRow("Description:", m_winDesc);

    m_winTargetPath = new QLineEdit(w);
    m_winTargetPath->setPlaceholderText("path/to/Installer.exe");
    form->addRow("Target File:", m_winTargetPath);

    auto *signBtn = new QPushButton(si(SvgIcons::kSign), "Sign Now", w);
    connect(signBtn, &QPushButton::clicked, this, &CodeSignDialog::onWindowsSignNow);
    form->addRow("", signBtn);

    vbox->addLayout(form);

    // Tool status
    const QString tool = CodeSigner::windowsSigningTool();
    auto *toolLbl = new QLabel(
        tool.isEmpty()
            ? "<b style='color:orange'>No Windows signing tool found.</b><br>"
              "Install: <code>brew install osslsigncode</code> (macOS/Linux)<br>"
              "or install the Windows SDK on Windows."
            : QString("<b style='color:green'>Tool: %1</b>").arg(tool), w);
    toolLbl->setWordWrap(true);
    vbox->addWidget(toolLbl);

    vbox->addStretch();
    tabs->addTab(w, "Windows");
}

// ── Linux Tab ─────────────────────────────────────────────────────────────────

void CodeSignDialog::buildLinuxTab(QTabWidget *tabs)
{
    auto *w    = new QWidget;
    auto *vbox = new QVBoxLayout(w);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);

    m_linGpgKey = new QLineEdit(w);
    m_linGpgKey->setPlaceholderText("GPG key fingerprint or email");
    form->addRow("GPG Key ID:", m_linGpgKey);

    m_linTool = new QComboBox(w);
    m_linTool->addItems({"Auto (dpkg-sig → debsigs → gpg)", "dpkg-sig", "debsigs", "gpg"});
    form->addRow("Signing Tool:", m_linTool);

    m_linTargetPath = new QLineEdit(w);
    m_linTargetPath->setPlaceholderText("path/to/package.deb  or  .rpm  or  .AppImage");
    form->addRow("Target File:", m_linTargetPath);

    auto *signBtn = new QPushButton(si(SvgIcons::kSign), "Sign Now", w);
    connect(signBtn, &QPushButton::clicked, this, &CodeSignDialog::onLinuxSignNow);
    form->addRow("", signBtn);

    vbox->addLayout(form);

    auto *hint = new QLabel(
        "<small>Generate a GPG key: <code>gpg --full-gen-key</code><br>"
        "List keys: <code>gpg --list-keys --keyid-format=long</code><br>"
        "For .rpm signing, ensure <code>~/.rpmmacros</code> has <code>%_gpg_name</code>.</small>", w);
    hint->setWordWrap(true);
    vbox->addWidget(hint);
    vbox->addStretch();
    tabs->addTab(w, "Linux");
}

// ── Generate Tab ──────────────────────────────────────────────────────────────

void CodeSignDialog::buildGenerateTab(QTabWidget *tabs)
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto *w    = new QWidget;
    auto *vbox = new QVBoxLayout(w);
    scroll->setWidget(w);

    // ── Certificate Details ──────────────────────────────────────────────────
    auto *detGrp = new QGroupBox("Certificate Details", w);
    auto *form   = new QFormLayout(detGrp);
    form->setLabelAlignment(Qt::AlignRight);

    m_genCN      = new QLineEdit(w); m_genCN->setPlaceholderText("My Company Code Signing");
    m_genOrg     = new QLineEdit(w); m_genOrg->setPlaceholderText("My Company LLC");
    m_genOU      = new QLineEdit(w); m_genOU->setPlaceholderText("Software Release (optional)");
    m_genCountry = new QLineEdit(w); m_genCountry->setPlaceholderText("US"); m_genCountry->setMaxLength(2);
    m_genState   = new QLineEdit(w); m_genState->setPlaceholderText("Texas");
    m_genCity    = new QLineEdit(w); m_genCity->setPlaceholderText("Dallas");
    m_genEmail   = new QLineEdit(w); m_genEmail->setPlaceholderText("release@company.com (optional)");
    m_genDays    = new QLineEdit(w); m_genDays->setText("3650"); m_genDays->setMaxLength(5);

    form->addRow("Common Name (CN):",  m_genCN);
    form->addRow("Organization:",      m_genOrg);
    form->addRow("Unit (OU):",         m_genOU);
    form->addRow("Country:",           m_genCountry);
    form->addRow("State:",             m_genState);
    form->addRow("City:",              m_genCity);
    form->addRow("Email:",             m_genEmail);
    form->addRow("Validity (days):",   m_genDays);

    m_genUseEc   = new QCheckBox("Use EC P-256 key (smaller; RSA 4096 if unchecked)", w);
    form->addRow("", m_genUseEc);

    vbox->addWidget(detGrp);

    // ── Output ───────────────────────────────────────────────────────────────
    auto *outGrp = new QGroupBox("Output", w);
    auto *outForm = new QFormLayout(outGrp);
    outForm->setLabelAlignment(Qt::AlignRight);

    m_genOutDir  = new QLineEdit(w);
    m_genOutDir->setText(QDir::homePath() + "/certs");
    auto *dirBtn = new QPushButton(si(SvgIcons::kFolder), "", w); dirBtn->setFixedWidth(28);
    connect(dirBtn, &QPushButton::clicked, this, &CodeSignDialog::onBrowseOutputDir);
    auto *dirRow = new QHBoxLayout;
    dirRow->addWidget(m_genOutDir, 1);
    dirRow->addWidget(dirBtn);
    outForm->addRow("Output Directory:", dirRow);

    m_genMakePfx = new QCheckBox("Also export PFX/P12 (for Windows signing)", w);
    m_genMakePfx->setChecked(true);
    outForm->addRow("", m_genMakePfx);

    m_genPfxPw   = new QLineEdit(w);
    m_genPfxPw->setEchoMode(QLineEdit::Password);
    m_genPfxPw->setPlaceholderText("PFX password (leave empty for no password)");
    outForm->addRow("PFX Password:", m_genPfxPw);

    m_genMakePem = new QCheckBox("Also export combined PEM (key+cert, for codesign/osslsigncode)", w);
    m_genMakePem->setChecked(true);
    outForm->addRow("", m_genMakePem);

    m_genOutKey  = new QLabel("(generated)", w); m_genOutKey->setObjectName("hintLabel");
    m_genOutCert = new QLabel("(generated)", w); m_genOutCert->setObjectName("hintLabel");
    outForm->addRow("Key file:", m_genOutKey);
    outForm->addRow("Cert file:", m_genOutCert);

    vbox->addWidget(outGrp);

    // ── Action buttons ────────────────────────────────────────────────────────
    auto *btnRow = new QHBoxLayout;
    auto *genBtn = new QPushButton(si(SvgIcons::kKey), "Generate Self-Signed Certificate", w);
    genBtn->setDefault(false);
    connect(genBtn, &QPushButton::clicked, this, &CodeSignDialog::onGenerateSelfSigned);
    btnRow->addWidget(genBtn);

    auto *csrBtn = new QPushButton(si(SvgIcons::kKey), "Generate Key + CSR for CA", w);
    connect(csrBtn, &QPushButton::clicked, this, &CodeSignDialog::onGenerateCsr);
    btnRow->addWidget(csrBtn);

    auto *pfxBtn = new QPushButton(si(SvgIcons::kConvert), "PEM → PFX Converter", w);
    connect(pfxBtn, &QPushButton::clicked, this, &CodeSignDialog::onPemToPfx);
    btnRow->addWidget(pfxBtn);

    auto *kcBtn = new QPushButton(si(SvgIcons::kKey), "Import to macOS Keychain", w);
    connect(kcBtn, &QPushButton::clicked, this, &CodeSignDialog::onImportToKeychain);
    btnRow->addWidget(kcBtn);

    vbox->addLayout(btnRow);

    // ── CA Guidance ───────────────────────────────────────────────────────────
    auto *guidGrp = new QGroupBox("Certificate Authority Guidance", w);
    auto *guidL   = new QVBoxLayout(guidGrp);
    auto *guidTxt = new QPlainTextEdit(CertGenerator::caGuidanceText(), guidGrp);
    guidTxt->setReadOnly(true);
    guidTxt->setMinimumHeight(160);
    guidTxt->setFont(QFont("Menlo", 10));
    guidL->addWidget(guidTxt);
    vbox->addWidget(guidGrp);

    vbox->addStretch();
    tabs->addTab(scroll, "Generate / CA");
}

// ── Slot implementations ──────────────────────────────────────────────────────

void CodeSignDialog::onRefreshIdentities()
{
    if (!m_macIdentity) return;
    const QString current = m_macIdentity->currentText();
    m_macIdentity->clear();
    m_macIdentity->addItem("-  (ad-hoc — no trust, for testing only)");

    for (const QString &id : CertGenerator::listMacosIdentities())
        m_macIdentity->addItem(id);

    // Restore previous selection
    const int idx = m_macIdentity->findText(current);
    if (idx >= 0) m_macIdentity->setCurrentIndex(idx);
}

void CodeSignDialog::onMacosSignNow()
{
    const QString path = m_macTargetPath->text().trimmed();
    if (path.isEmpty()) { appendLog("Set a target file path first.", true); return; }

    const QString id = m_macIdentity->currentText();
    // Strip the leading "-  " prefix from the ad-hoc entry
    const QString identity = id.startsWith("-  ") ? "-" : id;

    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };

    if (CodeSigner::signMacosApp(path, identity, m_macEntitlements->text().trimmed(),
                                  m_macHardened->isChecked(), &err, prog)) {
        appendLog("Sign succeeded: " + path);
    } else {
        appendLog("Sign failed: " + err, true);
    }
}

void CodeSignDialog::onMacosNotarizeNow()
{
    const QString path = m_macTargetPath->text().trimmed();
    if (path.isEmpty() || m_macAppleId->text().isEmpty()) {
        appendLog("Set target file + Apple ID before notarizing.", true);
        return;
    }
    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };
    if (!CodeSigner::notarizeMacos(path, m_macAppleId->text(), m_macTeamId->text(),
                                    m_macAppPw->text(), &err, prog))
        appendLog("Notarize failed: " + err, true);
}

void CodeSignDialog::onBrowsePfx()
{
    const QString f = QFileDialog::getOpenFileName(this, "PFX/P12 Certificate",
        QDir::homePath(), "PFX/P12 (*.pfx *.p12);;All (*)");
    if (!f.isEmpty()) m_winPfxPath->setText(f);
}

void CodeSignDialog::onWindowsSignNow()
{
    const QString path = m_winTargetPath->text().trimmed();
    const QString pfx  = m_winPfxPath->text().trimmed();
    if (path.isEmpty() || pfx.isEmpty()) {
        appendLog("Set target file and PFX path first.", true); return;
    }
    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };
    if (!CodeSigner::signWindowsExe(path, pfx, m_winPfxPw->text(),
                                     m_winTimestamp->currentText(),
                                     m_winDesc->text(), &err, prog))
        appendLog("Sign failed: " + err, true);
}

void CodeSignDialog::onLinuxSignNow()
{
    const QString path = m_linTargetPath->text().trimmed();
    const QString key  = m_linGpgKey->text().trimmed();
    if (path.isEmpty() || key.isEmpty()) {
        appendLog("Set target file and GPG key ID first.", true); return;
    }

    const QString lower = path.toLower();
    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };
    bool ok = false;

    if (lower.endsWith(".deb"))
        ok = CodeSigner::signDebPackage(path, key, &err, prog);
    else if (lower.endsWith(".rpm"))
        ok = CodeSigner::signRpmPackage(path, key, &err, prog);
    else
        ok = CodeSigner::signAppImage(path, key, &err, prog);

    if (!ok) appendLog("Sign failed: " + err, true);
}

void CodeSignDialog::onGenerateSelfSigned()
{
    if (m_genCN->text().trimmed().isEmpty() || m_genOutDir->text().isEmpty()) {
        appendLog("Fill in Common Name and output directory first.", true); return;
    }
    QDir().mkpath(m_genOutDir->text());

    CertParams p;
    p.commonName        = m_genCN->text().trimmed();
    p.organization      = m_genOrg->text().trimmed();
    p.organizationUnit  = m_genOU->text().trimmed();
    p.country           = m_genCountry->text().trimmed().toUpper();
    p.state             = m_genState->text().trimmed();
    p.city              = m_genCity->text().trimmed();
    p.email             = m_genEmail->text().trimmed();
    p.validityDays      = m_genDays->text().toInt();
    p.useEc             = m_genUseEc->isChecked();
    p.forCodeSign       = true;

    const QString base  = m_genOutDir->text() + "/" + p.commonName.replace(' ', '_');
    p.keyPath           = base + ".key.pem";
    p.certPath          = base + ".cert.pem";
    if (m_genMakePfx->isChecked()) {
        p.pfxPath       = base + ".pfx";
        p.pfxPassword   = m_genPfxPw->text();
    }
    if (m_genMakePem->isChecked())
        p.pemPath       = base + ".combined.pem";

    m_genOutKey ->setText(p.keyPath);
    m_genOutCert->setText(p.certPath);

    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };
    if (!CertGenerator::generateSelfSigned(p, &err, prog))
        appendLog("Generation failed: " + err, true);

    // If PFX was generated, pre-fill the Windows tab
    if (!p.pfxPath.isEmpty() && QFile::exists(p.pfxPath)) {
        m_winPfxPath->setText(p.pfxPath);
        m_winPfxPw->setText(p.pfxPassword);
    }
}

void CodeSignDialog::onGenerateCsr()
{
    if (m_genCN->text().trimmed().isEmpty() || m_genOutDir->text().isEmpty()) {
        appendLog("Fill in Common Name and output directory first.", true); return;
    }
    QDir().mkpath(m_genOutDir->text());

    CertParams p;
    p.commonName        = m_genCN->text().trimmed();
    p.organization      = m_genOrg->text().trimmed();
    p.organizationUnit  = m_genOU->text().trimmed();
    p.country           = m_genCountry->text().trimmed().toUpper();
    p.state             = m_genState->text().trimmed();
    p.city              = m_genCity->text().trimmed();
    p.email             = m_genEmail->text().trimmed();
    p.useEc             = m_genUseEc->isChecked();
    p.forCodeSign       = true;

    const QString base  = m_genOutDir->text() + "/" + p.commonName.replace(' ', '_');
    p.keyPath           = base + ".key.pem";
    p.csrPath           = base + ".csr";

    m_genOutKey->setText(p.keyPath);

    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };
    if (CertGenerator::generateCsr(p, &err, prog))
        appendLog("CSR ready for CA submission: " + p.csrPath);
    else
        appendLog("CSR generation failed: " + err, true);
}

void CodeSignDialog::onPemToPfx()
{
    const QString key  = QFileDialog::getOpenFileName(this, "Select Private Key PEM",
        m_genOutDir->text(), "PEM (*.pem *.key);;All (*)");
    if (key.isEmpty()) return;
    const QString cert = QFileDialog::getOpenFileName(this, "Select Certificate PEM",
        QFileInfo(key).dir().path(), "PEM (*.pem *.crt);;All (*)");
    if (cert.isEmpty()) return;
    const QString pfx  = QFileDialog::getSaveFileName(this, "Save PFX As",
        QFileInfo(key).dir().path() + "/cert.pfx", "PFX (*.pfx *.p12)");
    if (pfx.isEmpty()) return;

    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };
    if (!CertGenerator::pemToPfx(key, cert, pfx, m_winPfxPw->text(), &err, prog))
        appendLog("PFX conversion failed: " + err, true);
    else
        m_winPfxPath->setText(pfx);
}

void CodeSignDialog::onImportToKeychain()
{
    const QString pfx = m_winPfxPath->text().trimmed().isEmpty()
        ? QFileDialog::getOpenFileName(this, "Select PFX/P12",
            m_genOutDir->text(), "PFX/P12 (*.pfx *.p12)")
        : m_winPfxPath->text().trimmed();
    if (pfx.isEmpty()) return;

    QString err;
    auto prog = [this](int, const QString &msg) { appendLog(msg); };
    if (!CertGenerator::importPfxToKeychain(pfx, m_winPfxPw->text(), &err, prog))
        appendLog("Keychain import failed: " + err, true);
    else
        onRefreshIdentities();
}

void CodeSignDialog::onBrowseOutputDir()
{
    const QString d = QFileDialog::getExistingDirectory(this, "Output Directory",
        m_genOutDir->text());
    if (!d.isEmpty()) m_genOutDir->setText(d);
}

// ── Shared log ────────────────────────────────────────────────────────────────

void CodeSignDialog::appendLog(const QString &msg, bool error)
{
    const QString prefix = error ? "[ERR] " : "[OK]  ";
    m_log->appendPlainText(prefix + msg);
}

QString CodeSignDialog::chooseFile(const QString &title, const QString &filter)
{
    return QFileDialog::getOpenFileName(this, title, QDir::homePath(), filter);
}

// ── Accessors ─────────────────────────────────────────────────────────────────

QString CodeSignDialog::macosIdentity()   const { return m_macIdentity  ? m_macIdentity->currentText() : QString(); }
QString CodeSignDialog::macosEntitlements() const { return m_macEntitlements ? m_macEntitlements->text() : QString(); }
bool    CodeSignDialog::macosHardenedRuntime() const { return m_macHardened && m_macHardened->isChecked(); }
bool    CodeSignDialog::macosNotarize()   const { return m_macNotarize  && m_macNotarize->isChecked(); }
QString CodeSignDialog::macosAppleId()    const { return m_macAppleId   ? m_macAppleId->text() : QString(); }
QString CodeSignDialog::macosTeamId()     const { return m_macTeamId    ? m_macTeamId->text()  : QString(); }
QString CodeSignDialog::macosAppPassword() const { return m_macAppPw    ? m_macAppPw->text()   : QString(); }
QString CodeSignDialog::windowsPfxPath()  const { return m_winPfxPath  ? m_winPfxPath->text()  : QString(); }
QString CodeSignDialog::windowsPfxPassword() const { return m_winPfxPw ? m_winPfxPw->text()   : QString(); }
QString CodeSignDialog::windowsTimestampUrl() const { return m_winTimestamp ? m_winTimestamp->currentText() : QString(); }
QString CodeSignDialog::windowsDescription() const { return m_winDesc   ? m_winDesc->text()    : QString(); }
QString CodeSignDialog::linuxGpgKeyId()   const { return m_linGpgKey   ? m_linGpgKey->text()  : QString(); }
QString CodeSignDialog::linuxSigningTool() const { return m_linTool    ? m_linTool->currentText() : QString(); }
QString CodeSignDialog::generatedKeyPath()  const { return m_genOutKey  ? m_genOutKey->text()  : QString(); }
QString CodeSignDialog::generatedCertPath() const { return m_genOutCert ? m_genOutCert->text() : QString(); }
QString CodeSignDialog::generatedPfxPath()  const { return m_winPfxPath ? m_winPfxPath->text() : QString(); }
QString CodeSignDialog::generatedPemPath()  const { return m_genMakePem && m_genMakePem->isChecked() && m_genOutDir
    ? m_genOutDir->text() + "/" + m_genCN->text().replace(' ', '_') + ".combined.pem" : QString(); }

void CodeSignDialog::saveToManifest(Manifest &m) const
{
    // Persist the signing identity as the macOS codesign identity
    // (WizardTheme fields could be extended — for now we use description as a carrier)
    Q_UNUSED(m)
}
