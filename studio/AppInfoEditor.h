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

class AppInfoEditor : public QWidget
{
    Q_OBJECT

public:
    explicit AppInfoEditor(QWidget *parent = nullptr);

    void load(const Manifest &m);
    void save(Manifest &m) const;

private:
    void buildUi();

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
    QLineEdit *m_accentColor    = nullptr;
    QLineEdit *m_bgColor        = nullptr;
    QLineEdit *m_bannerImage    = nullptr;
    QLineEdit *m_sidePanelImage = nullptr;
    QCheckBox *m_darkMode       = nullptr;
};
