#pragma once
/*
 * CodeSignDialog.h — Code Signing Management Dialog
 *
 * Tabbed dialog providing:
 *   macOS tab  — identity picker (from Keychain), entitlements, notarize
 *   Windows tab — PFX/P12 cert path + password, timestamp server, osslsigncode
 *   Linux tab  — GPG key ID, signing tool selection
 *   Generate tab — self-signed cert generation, CSR export, CA guidance
 *
 * Usage:
 *   CodeSignDialog dlg(manifest, this);
 *   if (dlg.exec() == QDialog::Accepted)
 *       dlg.saveToManifest(manifest);
 */

#include <QDialog>
#include "../manifest/Manifest.h"

class QTabWidget;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QPlainTextEdit;
class QGroupBox;

class CodeSignDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CodeSignDialog(const Manifest &m, QWidget *parent = nullptr);

    // Write signing config back into the SecurityEditor fields (via manifest theme extension)
    void saveToManifest(Manifest &m) const;

    // ── Accessor: per-platform signing params (read by backends) ─────────────
    // macOS
    QString macosIdentity()          const;
    QString macosEntitlements()      const;
    bool    macosHardenedRuntime()   const;
    bool    macosNotarize()          const;
    QString macosAppleId()           const;
    QString macosTeamId()            const;
    QString macosAppPassword()       const;

    // Windows
    QString windowsPfxPath()         const;
    QString windowsPfxPassword()     const;
    QString windowsTimestampUrl()    const;
    QString windowsDescription()     const;

    // Linux
    QString linuxGpgKeyId()          const;
    QString linuxSigningTool()       const;

    // Generated cert output paths
    QString generatedKeyPath()       const;
    QString generatedCertPath()      const;
    QString generatedPfxPath()       const;
    QString generatedPemPath()       const;

private slots:
    // macOS
    void onRefreshIdentities();
    void onMacosSignNow();
    void onMacosNotarizeNow();

    // Windows
    void onBrowsePfx();
    void onWindowsSignNow();

    // Linux
    void onLinuxSignNow();

    // Generate
    void onGenerateSelfSigned();
    void onGenerateCsr();
    void onPemToPfx();
    void onImportToKeychain();
    void onBrowseOutputDir();

private:
    void buildMacosTab(QTabWidget *tabs);
    void buildWindowsTab(QTabWidget *tabs);
    void buildLinuxTab(QTabWidget *tabs);
    void buildGenerateTab(QTabWidget *tabs);

    void appendLog(const QString &msg, bool error = false);
    QString chooseFile(const QString &title, const QString &filter);

    // ── macOS widgets ─────────────────────────────────────────────────────────
    QComboBox    *m_macIdentity     = nullptr;
    QLineEdit    *m_macEntitlements = nullptr;
    QCheckBox    *m_macHardened     = nullptr;
    QCheckBox    *m_macNotarize     = nullptr;
    QLineEdit    *m_macAppleId      = nullptr;
    QLineEdit    *m_macTeamId       = nullptr;
    QLineEdit    *m_macAppPw        = nullptr;
    QLineEdit    *m_macTargetPath   = nullptr;

    // ── Windows widgets ───────────────────────────────────────────────────────
    QLineEdit    *m_winPfxPath      = nullptr;
    QLineEdit    *m_winPfxPw        = nullptr;
    QComboBox    *m_winTimestamp    = nullptr;
    QLineEdit    *m_winDesc         = nullptr;
    QLineEdit    *m_winTargetPath   = nullptr;

    // ── Linux widgets ─────────────────────────────────────────────────────────
    QLineEdit    *m_linGpgKey       = nullptr;
    QComboBox    *m_linTool         = nullptr;
    QLineEdit    *m_linTargetPath   = nullptr;

    // ── Generate widgets ──────────────────────────────────────────────────────
    QLineEdit    *m_genCN           = nullptr;
    QLineEdit    *m_genOrg          = nullptr;
    QLineEdit    *m_genOU           = nullptr;
    QLineEdit    *m_genCountry      = nullptr;
    QLineEdit    *m_genState        = nullptr;
    QLineEdit    *m_genCity         = nullptr;
    QLineEdit    *m_genEmail        = nullptr;
    QLineEdit    *m_genDays         = nullptr;
    QCheckBox    *m_genUseEc        = nullptr;
    QCheckBox    *m_genMakePfx      = nullptr;
    QCheckBox    *m_genMakePem      = nullptr;
    QLineEdit    *m_genPfxPw        = nullptr;
    QLineEdit    *m_genOutDir       = nullptr;
    QLabel       *m_genOutKey       = nullptr;
    QLabel       *m_genOutCert      = nullptr;

    // ── Shared log panel ──────────────────────────────────────────────────────
    QPlainTextEdit *m_log           = nullptr;
};
