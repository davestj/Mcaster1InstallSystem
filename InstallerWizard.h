#pragma once
#include <QWizard>
#include <QString>

// ── Page IDs ─────────────────────────────────────────────────────────────────
enum PageId {
    PAGE_WELCOME    = 0,
    PAGE_COMPONENTS = 1,
    PAGE_READY      = 2,
    PAGE_INSTALL    = 3,
    PAGE_FINISH     = 4,
};

// ── Shared install options ────────────────────────────────────────────────────
struct InstallOptions {
    bool installGui       = true;
    bool installService   = false;
    bool installShortcuts = true;
    bool launchAfter      = true;
    QString installDir    = "/Applications/Mcaster1";
};

// ── InstallerWizard ───────────────────────────────────────────────────────────
class InstallerWizard : public QWizard
{
    Q_OBJECT
public:
    explicit InstallerWizard(QWidget *parent = nullptr);

    InstallOptions options() const { return m_opts; }
    void           setOptions(const InstallOptions &o) { m_opts = o; }

    bool installSuccess() const    { return m_success; }
    void setInstallSuccess(bool s) { m_success = s; }

    // Pre-computed content-cropped banner for WelcomePage / FinishPage to embed
    QPixmap bannerPixmap()     const { return m_bannerPixmap; }

    // Side panel column image, scaled to the content area height
    QPixmap sidePanelPixmap()  const { return m_sidePanelPixmap; }

    // Path to payload inside our own app bundle's Resources/
    static QString payloadPath();
    static QString resourcePath();

private:
    InstallOptions m_opts;
    bool           m_success        = false;
    QPixmap        m_bannerPixmap;      // content-cropped, scaled to wizard width
    QPixmap        m_sidePanelPixmap;   // portrait column, scaled to content area height
};
