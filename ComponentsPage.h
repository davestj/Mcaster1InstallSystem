#pragma once
#include <QWizardPage>
#include <QCheckBox>
#include <QLabel>

class ComponentsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ComponentsPage(QWidget *parent = nullptr);
    int  nextId() const override;
    void initializePage() override;
    bool validatePage() override;

private slots:
    void updateDependencies();

private:
    QCheckBox *m_chkGui;
    QCheckBox *m_chkService;
    QCheckBox *m_chkShortcuts;
    QCheckBox *m_chkLaunch;
    QLabel    *m_lblGuiDetail;
    QLabel    *m_lblSvcDetail;
    QLabel    *m_lblSizeHint;

    void updateSizeHint();
};
