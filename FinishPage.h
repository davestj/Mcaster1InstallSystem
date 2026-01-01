#pragma once
#include <QWizardPage>
#include <QLabel>
#include <QCheckBox>

class FinishPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit FinishPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool isComplete()     const override;
    bool validatePage()   override;   // fires on Done click — launches app if requested
    int  nextId()         const override { return -1; }

private:
    QLabel    *m_bannerLabel    = nullptr;
    QLabel    *m_sidePanelLabel = nullptr;
    QLabel    *m_headline       = nullptr;
    QLabel    *m_detail         = nullptr;
    QCheckBox *m_chkLaunch      = nullptr;
    bool       m_success        = false;
};
