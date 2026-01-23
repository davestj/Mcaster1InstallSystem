/*
 * CertGenerator.cpp — Self-signed certificate + CSR generator
 *
 * Delegates all crypto operations to the openssl CLI.  Requires openssl 1.1+
 * (openssl 3.x preferred for EC P-256 support).
 *
 * Homebrew: brew install openssl@3
 */

#include "CertGenerator.h"

#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>

// ── Utility ───────────────────────────────────────────────────────────────────

QString CertGenerator::opensslPath()
{
    // Search in priority order: Homebrew > system
    const QStringList candidates = {
        "/opt/homebrew/opt/openssl@3/bin/openssl",   // Apple Silicon Homebrew
        "/usr/local/opt/openssl@3/bin/openssl",       // Intel Homebrew
        "/opt/homebrew/opt/openssl/bin/openssl",
        "/usr/local/opt/openssl/bin/openssl",
        "openssl",                                     // PATH fallback
    };
    for (const QString &c : candidates) {
        QProcess p;
        p.start(c, {"version"});
        if (p.waitForFinished(3000) && p.exitCode() == 0)
            return c;
    }
    return {};
}

bool CertGenerator::runOpenSsl(const QStringList &args, QString *errOut,
                                const ProgressFn &progress)
{
    const QString ssl = opensslPath();
    if (ssl.isEmpty()) {
        if (errOut) *errOut = "openssl not found.\n"
                              "macOS: brew install openssl@3\n"
                              "Linux: sudo apt install openssl  (or dnf/pacman)\n"
                              "Windows: https://slproweb.com/products/Win32OpenSSL.html";
        return false;
    }

    if (progress) progress(0, "  openssl " + args.join(' '));

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(ssl, args);
    if (!p.waitForStarted(5000)) {
        if (errOut) *errOut = "Could not start openssl: " + p.errorString();
        return false;
    }
    p.waitForFinished(60000);

    const QString out = QString::fromLocal8Bit(p.readAll()).trimmed();
    if (p.exitCode() != 0) {
        if (errOut) *errOut = QString("openssl failed (exit %1):\n%2").arg(p.exitCode()).arg(out);
        return false;
    }
    return true;
}

QString CertGenerator::buildSubject(const CertParams &p)
{
    QStringList parts;
    if (!p.country.isEmpty())          parts << "C="  + p.country;
    if (!p.state.isEmpty())            parts << "ST=" + p.state;
    if (!p.city.isEmpty())             parts << "L="  + p.city;
    if (!p.organization.isEmpty())     parts << "O="  + p.organization;
    if (!p.organizationUnit.isEmpty()) parts << "OU=" + p.organizationUnit;
    parts << "CN=" + p.commonName;
    if (!p.email.isEmpty())            parts << "emailAddress=" + p.email;
    return "/" + parts.join('/');
}

// ── Extensions config for code-signing ───────────────────────────────────────

static QString buildExtensionsConfig(const CertParams &p)
{
    QString s;
    QTextStream ts(&s);
    ts << "[req]\n";
    ts << "distinguished_name = dn\n";
    ts << "x509_extensions    = v3_req\n";
    ts << "prompt             = no\n\n";

    ts << "[dn]\n";
    if (!p.country.isEmpty())          ts << "C  = " << p.country << "\n";
    if (!p.state.isEmpty())            ts << "ST = " << p.state   << "\n";
    if (!p.city.isEmpty())             ts << "L  = " << p.city    << "\n";
    if (!p.organization.isEmpty())     ts << "O  = " << p.organization << "\n";
    if (!p.organizationUnit.isEmpty()) ts << "OU = " << p.organizationUnit << "\n";
    ts << "CN = " << p.commonName << "\n";
    if (!p.email.isEmpty())            ts << "emailAddress = " << p.email << "\n";

    ts << "\n[v3_req]\n";
    ts << "basicConstraints  = CA:FALSE\n";
    ts << "keyUsage          = digitalSignature\n";
    if (p.forCodeSign)
        ts << "extendedKeyUsage  = codeSigning\n";
    if (!p.email.isEmpty())
        ts << "subjectAltName    = email:" << p.email << "\n";

    return s;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool CertGenerator::generateSelfSigned(const CertParams &p, QString *errOut,
                                        const ProgressFn &progress)
{
    if (p.keyPath.isEmpty() || p.certPath.isEmpty()) {
        if (errOut) *errOut = "keyPath and certPath must be set.";
        return false;
    }

    // Write extensions config to a temp file
    const QString cfgPath = p.keyPath + ".cnf";
    {
        QFile cfgFile(cfgPath);
        if (!cfgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (errOut) *errOut = "Cannot write config: " + cfgPath;
            return false;
        }
        cfgFile.write(buildExtensionsConfig(p).toUtf8());
    }

    if (progress) progress(10, "Generating private key…");

    // Step 1: Generate private key
    QStringList keyArgs;
    if (p.useEc) {
        keyArgs = {"ecparam", "-name", "prime256v1", "-genkey", "-noout",
                   "-out", p.keyPath};
    } else {
        keyArgs = {"genpkey", "-algorithm", "RSA",
                   "-pkeyopt", "rsa_keygen_bits:4096",
                   "-out", p.keyPath};
    }
    if (!runOpenSsl(keyArgs, errOut, progress)) {
        QFile::remove(cfgPath);
        return false;
    }

    if (progress) progress(50, "Generating self-signed certificate…");

    // Step 2: Self-sign
    QStringList certArgs = {
        "req", "-new", "-x509", "-nodes",
        "-key",     p.keyPath,
        "-out",     p.certPath,
        "-days",    QString::number(p.validityDays),
        "-config",  cfgPath,
        "-extensions", "v3_req"
    };
    if (!runOpenSsl(certArgs, errOut, progress)) {
        QFile::remove(cfgPath);
        return false;
    }

    if (progress) progress(75, "Certificate written: " + p.certPath);

    // Optional CSR generation (for CA submission alongside self-signed cert)
    if (!p.csrPath.isEmpty()) {
        if (progress) progress(80, "Generating CSR for CA submission…");
        QStringList csrArgs = {
            "req", "-new", "-key", p.keyPath,
            "-out", p.csrPath, "-config", cfgPath
        };
        runOpenSsl(csrArgs, errOut, progress);  // non-fatal
    }

    QFile::remove(cfgPath);

    // Optional: PFX/P12 bundle
    if (!p.pfxPath.isEmpty()) {
        if (progress) progress(85, "Creating PFX/P12 bundle…");
        if (!pemToPfx(p.keyPath, p.certPath, p.pfxPath, p.pfxPassword, errOut, progress))
            return false;
        if (progress) progress(93, "PFX written: " + p.pfxPath);
    }

    // Optional: combined PEM (key + cert)
    if (!p.pemPath.isEmpty()) {
        if (progress) progress(96, "Writing combined PEM…");
        QFile combined(p.pemPath);
        if (combined.open(QIODevice::WriteOnly)) {
            QFile keyFile(p.keyPath);
            QFile crtFile(p.certPath);
            if (keyFile.open(QIODevice::ReadOnly))
                combined.write(keyFile.readAll());
            if (crtFile.open(QIODevice::ReadOnly))
                combined.write(crtFile.readAll());
        }
    }

    if (progress) progress(100, "Certificate generation complete.");
    return true;
}

bool CertGenerator::generateCsr(const CertParams &p, QString *errOut,
                                  const ProgressFn &progress)
{
    if (p.keyPath.isEmpty() || p.csrPath.isEmpty()) {
        if (errOut) *errOut = "keyPath and csrPath must be set.";
        return false;
    }

    const QString cfgPath = p.keyPath + ".cnf";
    {
        QFile cfgFile(cfgPath);
        if (!cfgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (errOut) *errOut = "Cannot write config: " + cfgPath;
            return false;
        }
        cfgFile.write(buildExtensionsConfig(p).toUtf8());
    }

    if (progress) progress(10, "Generating private key…");
    QStringList keyArgs = p.useEc
        ? QStringList{"ecparam", "-name", "prime256v1", "-genkey", "-noout", "-out", p.keyPath}
        : QStringList{"genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:4096",
                      "-out", p.keyPath};

    if (!runOpenSsl(keyArgs, errOut, progress)) { QFile::remove(cfgPath); return false; }

    if (progress) progress(60, "Generating CSR…");
    QStringList csrArgs = {
        "req", "-new", "-key", p.keyPath,
        "-out", p.csrPath, "-config", cfgPath
    };
    const bool ok = runOpenSsl(csrArgs, errOut, progress);
    QFile::remove(cfgPath);

    if (ok && progress)
        progress(100, "CSR ready for CA submission: " + p.csrPath);
    return ok;
}

bool CertGenerator::pemToPfx(const QString &keyPemPath,
                               const QString &certPemPath,
                               const QString &pfxPath,
                               const QString &pfxPassword,
                               QString *errOut,
                               const ProgressFn &progress)
{
    QStringList args = {
        "pkcs12", "-export",
        "-inkey", keyPemPath,
        "-in",    certPemPath,
        "-out",   pfxPath,
    };
    if (pfxPassword.isEmpty()) {
        args << "-passout" << "pass:";
    } else {
        args << "-passout" << "pass:" + pfxPassword;
    }
    return runOpenSsl(args, errOut, progress);
}

bool CertGenerator::importPfxToKeychain(const QString &pfxPath,
                                         const QString &pfxPassword,
                                         QString *errOut,
                                         const ProgressFn &progress)
{
#ifndef Q_OS_MAC
    if (errOut) *errOut = "Keychain import is only supported on macOS.";
    return false;
#else
    if (progress) progress(10, "Importing PFX into login keychain…");

    QStringList args = {
        "import", pfxPath,
        "-k", QDir::homePath() + "/Library/Keychains/login.keychain-db",
        "-f", "pkcs12",
        "-T", "/usr/bin/codesign"
    };
    if (!pfxPassword.isEmpty()) args << "-P" << pfxPassword;

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start("security", args);
    p.waitForFinished(30000);

    if (p.exitCode() != 0) {
        const QString out = QString::fromLocal8Bit(p.readAll());
        if (errOut) *errOut = "security import failed:\n" + out;
        return false;
    }
    if (progress) progress(100, "Certificate imported to keychain.");
    return true;
#endif
}

QStringList CertGenerator::listMacosIdentities()
{
#ifndef Q_OS_MAC
    return {"(macOS only)"};
#else
    QProcess p;
    p.start("security", {"find-identity", "-v", "-p", "codesigning"});
    p.waitForFinished(5000);
    const QString out = QString::fromLocal8Bit(p.readAll());

    QStringList identities;
    // Each line looks like:   1) HASH "Identity Name (TEAMID)"
    static const QRegularExpression re(R"re(\d+\)\s+\w+\s+"([^"]+)")re");
    auto it = re.globalMatch(out);
    while (it.hasNext())
        identities << it.next().captured(1);

    if (identities.isEmpty())
        identities << "(no code-signing identities in keychain)";
    return identities;
#endif
}

QString CertGenerator::caGuidanceText()
{
    return
        "── Code Signing Certificate Guide ─────────────────────────────────\n"
        "\n"
        "SELF-SIGNED (generated here)\n"
        "  • Free, instant. No CA involved.\n"
        "  • macOS: shows 'unverified developer' without Apple Developer ID.\n"
        "  • Windows: SmartScreen warning until cert builds reputation.\n"
        "  • Best for: internal testing, CI pipelines, open-source tools.\n"
        "\n"
        "FREE / LOW-COST CAs\n"
        "  • Certum Open Source Code Signing  — https://certum.eu\n"
        "    Free for open-source projects. Requires org verification.\n"
        "  • SignPath Foundation               — https://signpath.io/foundation\n"
        "    Free CI-integrated signing for open-source.\n"
        "\n"
        "COMMERCIAL CAs  (eliminate SmartScreen / Gatekeeper warnings)\n"
        "  • SSL.com          — ~$80/yr   https://ssl.com\n"
        "  • Sectigo          — ~$200/yr  https://sectigo.com\n"
        "  • DigiCert         — ~$500/yr  https://digicert.com\n"
        "  • GlobalSign       — ~$300/yr  https://globalsign.com\n"
        "\n"
        "PLATFORM-SPECIFIC\n"
        "  • Apple Developer ID   — $99/yr Apple Developer Program\n"
        "    Mandatory for macOS Gatekeeper bypass + notarization.\n"
        "    Enroll: https://developer.apple.com/programs/\n"
        "  • Microsoft Trusted Publisher — via any commercial CA above.\n"
        "    Eliminates Windows SmartScreen after sufficient reputation.\n"
        "\n"
        "CSR SUBMISSION WORKFLOW\n"
        "  1. Click 'Generate Key + CSR' — creates private key and .csr file.\n"
        "  2. Submit the .csr to your chosen CA.\n"
        "  3. Download the issued certificate (.crt/.pem).\n"
        "  4. Use 'PEM → PFX' to package key + cert for Windows signing.\n"
        "  5. Use 'Import to Keychain' for macOS codesign.\n"
        "─────────────────────────────────────────────────────────────────────\n";
}
