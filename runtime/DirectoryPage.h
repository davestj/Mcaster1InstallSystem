#pragma once
/*
 * DirectoryPage.h — Install directory selection page.
 *
 * Pre-populates from manifest defaults for the current platform.
 * Registers wizard field "installDir" so all other pages can read it via
 * wizard()->field("installDir").toString() — or via InstallerWizard::installDir().
 */
#include <QWizardPage>

class QLineEdit;
class QLabel;

class DirectoryPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit DirectoryPage(QWidget *parent = nullptr);
    void initializePage() override;

private slots:
    void onBrowse();

private:
    QLineEdit *m_dirEdit    = nullptr;
    QLabel    *m_spaceLabel = nullptr;
};
