#pragma once
/*
 * BuilderProfileDialog.h — Create / edit a BuilderProfile
 */

#include <QDialog>
#include "BuilderProfile.h"

class QCheckBox;
class QLineEdit;
class QTabWidget;
class QPlainTextEdit;

class BuilderProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BuilderProfileDialog(QWidget *parent = nullptr);

    // Pre-fill the form for editing an existing profile
    void setProfile(const BuilderProfile &p);

    // Collect the form into a BuilderProfile (call after exec() == Accepted)
    BuilderProfile profile() const;

private:
    void buildUi();

    QLineEdit *m_displayName    = nullptr;
    QLineEdit *m_company        = nullptr;
    QLineEdit *m_publisherName  = nullptr;
    QLineEdit *m_email          = nullptr;
    QLineEdit *m_website        = nullptr;
    QLineEdit *m_supportUrl     = nullptr;
    QCheckBox *m_chkSkipSigning = nullptr;
    QCheckBox *m_chkDevSign     = nullptr;
    QLineEdit *m_macosSigningId = nullptr;
    QLineEdit *m_winCert        = nullptr;
    QLineEdit *m_gpgKey         = nullptr;
    QPlainTextEdit *m_customTokens = nullptr; // "key=value\nkey=value" format
    QString    m_id;   // preserved when editing; generated when new
};
