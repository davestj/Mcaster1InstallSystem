#pragma once
/*
 * WelcomePage.h — First wizard page.
 * Shows the app banner, name, version, publisher, and description.
 */
#include <QWizardPage>

class QLabel;

class WelcomePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit WelcomePage(QWidget *parent = nullptr);
    void initializePage() override;
private:
    QLabel *m_bannerLabel  = nullptr;
    QLabel *m_appName      = nullptr;
    QLabel *m_appVersion   = nullptr;
    QLabel *m_description  = nullptr;
    QLabel *m_publisherLbl = nullptr;
};
