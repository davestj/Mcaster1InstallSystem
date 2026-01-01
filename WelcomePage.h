#pragma once
#include <QWizardPage>
#include <QLabel>

class WelcomePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit WelcomePage(QWidget *parent = nullptr);
    void initializePage() override;   // sets banner image once wizard is available
    int  nextId() const override;

private:
    QLabel *m_bannerLabel    = nullptr;
    QLabel *m_sidePanelLabel = nullptr;
};
