#pragma once
/*
 * LicensePage.h — License agreement page. Skipped if licenseFile is empty.
 */
#include <QWizardPage>

class QTextEdit;
class QRadioButton;

class LicensePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit LicensePage(QWidget *parent = nullptr);
    void initializePage() override;
    bool isComplete() const override;
private:
    QTextEdit    *m_licenseText = nullptr;
    QRadioButton *m_accept     = nullptr;
    QRadioButton *m_decline    = nullptr;
};
