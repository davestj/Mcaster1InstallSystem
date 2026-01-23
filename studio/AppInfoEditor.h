#pragma once
/*
 * AppInfoEditor.h — App metadata + defaults + theme editor tab.
 *
 * Covers:  AppInfo, InstallDefaults, WizardTheme, targets list.
 */

#include <QWidget>
#include "Manifest.h"

class QLineEdit;
class QCheckBox;
class QTextEdit;
class QGroupBox;
class QPushButton;

class AppInfoEditor : public QWidget
{
    Q_OBJECT

public:
    explicit AppInfoEditor(QWidget *parent = nullptr);

    void load(const Manifest &m);
    void save(Manifest &m) const;

    // Update the app name field without emitting modified() (called by sidebar inline edit)
    void setAppName(const QString &name);

signals:
    // Emitted whenever any field changes (wired to StudioMainWindow::onProjectModified)
    void modified();

    // Emitted specifically when app name or version changes (updates the status bar live)
    void projectInfoChanged(const QString &name, const QString &version);

private slots:
    void onAnyFieldChanged();

private:
    void buildUi();

    // Helper: wrap a QLineEdit in a [edit | …] browse row.
    // isDir=true opens a directory picker; isDir=false opens file picker.
    QWidget *makePathRow(QLineEdit *edit, QWidget *parent, bool isDir = false,
                         const QString &filter = QString());

    // Helper: wrap a QLineEdit + color swatch button in one row.
    QWidget *makeColorRow(QLineEdit *edit, QPushButton *&btnOut, QWidget *parent);

    // AppInfo
    QLineEdit *m_name        = nullptr;
    QLineEdit *m_version     = nullptr;
    QLineEdit *m_publisher   = nullptr;
    QLineEdit *m_identifier  = nullptr;
    QLineEdit *m_url         = nullptr;
    QLineEdit *m_supportUrl  = nullptr;
    QTextEdit *m_description = nullptr;
    QLineEdit *m_iconPath    = nullptr;
    QLineEdit *m_licenseFile = nullptr;

    // InstallDefaults
    QLineEdit *m_dirMacos    = nullptr;
    QLineEdit *m_dirWindows  = nullptr;
    QLineEdit *m_dirLinux    = nullptr;
    QCheckBox *m_requireAdmin    = nullptr;
    QCheckBox *m_allowCustomDir  = nullptr;
    QCheckBox *m_launchAfter     = nullptr;
    QCheckBox *m_createUninstall = nullptr;

    // Targets
    QCheckBox *m_tgtMacos   = nullptr;
    QCheckBox *m_tgtWindows = nullptr;
    QCheckBox *m_tgtLinux   = nullptr;

    // WizardTheme
    QLineEdit  *m_accentColor    = nullptr;
    QLineEdit  *m_bgColor        = nullptr;
    QLineEdit  *m_bannerImage    = nullptr;
    QLineEdit  *m_sidePanelImage = nullptr;
    QCheckBox  *m_darkMode       = nullptr;
    QPushButton *m_accentColorBtn = nullptr;  // color swatch opener
    QPushButton *m_bgColorBtn     = nullptr;
};
