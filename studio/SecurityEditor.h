#pragma once
/*
 * SecurityEditor.h — Code signing and security settings editor.
 * macOS: codesign identity + notarization. Windows: Authenticode cert.
 */
#include <QWidget>
#include "Manifest.h"

class QLineEdit;
class QCheckBox;
class QGroupBox;

class SecurityEditor : public QWidget
{
    Q_OBJECT
public:
    explicit SecurityEditor(QWidget *parent = nullptr);
    void load(const Manifest &m);
    void save(Manifest &m) const;

private slots:
    void onManageCodeSigning();

private:
    void buildUi();
    Manifest m_manifest;  // retained to pass into CodeSignDialog

    // macOS signing
    QLineEdit *m_macosIdentity  = nullptr;
    QLineEdit *m_macosTeamId    = nullptr;
    QCheckBox *m_macosHarden    = nullptr;
    QCheckBox *m_macosNotarize  = nullptr;
    QLineEdit *m_macosProfile   = nullptr;

    // Windows signing
    QLineEdit *m_winCertFile    = nullptr;
    QLineEdit *m_winCertPass    = nullptr;
    QLineEdit *m_winTimestamp   = nullptr;

    // Linux signing
    QLineEdit *m_linuxGpgKey    = nullptr;
    QCheckBox *m_linuxSignDebs  = nullptr;
};
