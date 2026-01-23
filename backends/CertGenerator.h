#pragma once
/*
 * CertGenerator.h — Self-signed code-signing certificate generator
 *
 * Uses the openssl CLI (Homebrew, system, or bundled) to generate:
 *   - RSA or EC private keys
 *   - Self-signed X.509 certificates configured for code signing
 *   - Certificate Signing Requests (CSRs) for submission to a CA
 *   - PFX/P12 bundles (for Windows Authenticode)
 *   - Combined PEM (key + cert, for osslsigncode / macOS codesign)
 *
 * ── Certificate Authority (CA) Guidance ─────────────────────────────────────
 *
 *  Self-signed (this class)
 *    • Free, instant, zero trust outside your machine.
 *    • Useful for internal distribution, testing, CI pipelines.
 *    • Windows will show SmartScreen warning; macOS will show "unverified developer".
 *
 *  Free / low-cost CAs
 *    • Certum Open Source Code Signing     — https://www.certum.eu
 *      Free for open-source projects (EV requires paid).
 *    • SignPath Foundation                  — https://signpath.io/foundation
 *      Free code signing for open-source via CI integration.
 *    • Actalis Free S/MIME                 — not code-signing but email certs.
 *    • Let's Encrypt                       — TLS only, NOT code-signing.
 *
 *  Commercial CAs (produce trusted, no-warning signatures)
 *    • Sectigo (formerly Comodo)  — https://sectigo.com    ~$100-$400/year
 *    • DigiCert                   — https://digicert.com   ~$400-$800/year
 *    • GlobalSign                 — https://globalsign.com
 *    • SSL.com                    — https://ssl.com        ~$80/year
 *
 *  Platform-specific
 *    • Apple Developer ID         — $99/year Apple Developer Program
 *      Required for macOS Gatekeeper bypass + notarization.
 *    • Microsoft Trusted Publisher — acquired through any commercial CA above.
 *      Eliminates Windows SmartScreen warning.
 *    • Google Play                — separate Android signing key, not CA-based.
 *
 * ── Key types ────────────────────────────────────────────────────────────────
 *  RSA 4096  — widest compatibility, larger files; use for Windows Authenticode
 *  EC P-256  — smaller, faster; preferred for macOS, Linux, modern Windows
 */

#include <QString>
#include <QStringList>
#include <functional>

struct CertParams {
    QString commonName;         // CN=  e.g. "ACME Corp Code Signing"
    QString organization;       // O=   e.g. "ACME Corp"
    QString organizationUnit;   // OU=  e.g. "Software Release"
    QString country;            // C=   2-letter, e.g. "US"
    QString state;              // ST=  e.g. "Texas"
    QString city;               // L=   e.g. "Dallas"
    QString email;              // email in Subject / SAN

    int  validityDays  = 3650;  // ~10 years (self-signed); CAs limit to 1-3 years
    bool useEc         = false; // true = EC P-256, false = RSA 4096
    bool forCodeSign   = true;  // true = adds extendedKeyUsage=codeSigning OID

    // Output paths (directory must exist)
    QString keyPath;     // private key PEM output
    QString certPath;    // certificate PEM output
    QString csrPath;     // CSR PEM output (for CA submission); empty = skip
    QString pfxPath;     // PFX/P12 output (for Windows); empty = skip
    QString pfxPassword; // PFX password (may be empty)
    QString pemPath;     // combined key+cert PEM (for macOS/Linux); empty = skip
};

class CertGenerator
{
public:
    using ProgressFn = std::function<void(int, const QString &)>;

    /*  Generate a self-signed certificate.
     *  Returns true on success; any error is written to errOut.              */
    static bool generateSelfSigned(const CertParams &p,
                                   QString *errOut = nullptr,
                                   const ProgressFn &progress = {});

    /*  Generate only a CSR (for submission to a commercial CA).
     *  The private key is also written to p.keyPath.                         */
    static bool generateCsr(const CertParams &p,
                             QString *errOut = nullptr,
                             const ProgressFn &progress = {});

    /*  Convert an existing PEM key+cert pair into a PFX/P12 bundle.          */
    static bool pemToPfx(const QString &keyPemPath,
                          const QString &certPemPath,
                          const QString &pfxPath,
                          const QString &pfxPassword,
                          QString *errOut = nullptr,
                          const ProgressFn &progress = {});

    /*  Import a PFX into the macOS Keychain (for use with codesign).         */
    static bool importPfxToKeychain(const QString &pfxPath,
                                     const QString &pfxPassword,
                                     QString *errOut = nullptr,
                                     const ProgressFn &progress = {});

    /*  Returns a list of code-signing identities visible to codesign.        */
    static QStringList listMacosIdentities();

    /*  Returns the path to a usable openssl binary, or empty if none found.  */
    static QString opensslPath();

    /*  Guidance text for the current platform.                               */
    static QString caGuidanceText();

private:
    static QString buildSubject(const CertParams &p);
    static bool runOpenSsl(const QStringList &args, QString *errOut,
                            const ProgressFn &progress = {});
};
