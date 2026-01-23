#pragma once
/*
 * InstallerWizard.h — QWizard subclass for the manifest-driven installer.
 *
 * Page IDs (fixed enum so pages can navigate with setCurrentId):
 *   0 Welcome
 *   1 License
 *   2 Prerequisites  (only registered when manifest.prerequisites is non-empty)
 *   3 Components
 *   4 Directory
 *   5 Ready
 *   6 Installing   (progress)
 *   7 Finish
 */

#include <QWizard>
#include "Manifest.h"

class InstallerWizard : public QWizard
{
    Q_OBJECT

public:
    enum PageId { Welcome, License, Prerequisites, Components, Directory, Ready, Installing, Finish };

    explicit InstallerWizard(const Manifest &m, const QString &payloadDir,
                              QWidget *parent = nullptr);

    // Accessors used by pages
    const Manifest &manifest() const  { return m_manifest; }
    QString payloadDir()        const  { return m_payloadDir; }
    QString installDir()        const;  // reads the Directory page field

    // Banner pixmap (loaded once from bannerImage path or default)
    const QPixmap &bannerPixmap() const { return m_banner; }

    // Component selection — written by ComponentsPage, read by InstallPage
    void        setSelectedComponents(const QStringList &ids) { m_selectedComponents = ids; }
    QStringList selectedComponents()  const  { return m_selectedComponents; }

    // Install result — written by InstallPage, read by FinishPage
    void    setInstallResult(bool success, const QString &error) {
        m_installSuccess = success;
        m_installError   = error;
    }
    bool    installSuccess() const { return m_installSuccess; }
    QString installError()   const { return m_installError; }

private:
    void buildPages();
    void loadBanner();

    Manifest    m_manifest;
    QString     m_payloadDir;
    QPixmap     m_banner;
    QStringList m_selectedComponents;
    bool        m_installSuccess = false;
    QString     m_installError;
};
