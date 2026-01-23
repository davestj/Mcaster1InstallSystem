#pragma once
/*
 * FinishPage.h — Final wizard page.
 *
 * Shows "Installation complete" (or error message if install failed).
 * Optional "Launch <app>" checkbox, obeying manifest.defaults.launchAfter.
 * validatePage() launches the app if the checkbox is checked.
 */
#include <QWizardPage>

class QLabel;
class QCheckBox;

class FinishPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit FinishPage(QWidget *parent = nullptr);
    void initializePage() override;
    bool validatePage() override;

private:
    QLabel    *m_statusLabel = nullptr;
    QCheckBox *m_launchCheck = nullptr;
};
