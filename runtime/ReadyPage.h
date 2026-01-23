#pragma once
/*
 * ReadyPage.h — Pre-installation summary page.
 *
 * Commit page: Next button says "Install". Shows a summary of what will be
 * installed where, and which components are selected.
 */
#include <QWizardPage>

class QLabel;

class ReadyPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ReadyPage(QWidget *parent = nullptr);
    void initializePage() override;

private:
    QLabel *m_appLabel  = nullptr;
    QLabel *m_dirLabel  = nullptr;
    QLabel *m_compLabel = nullptr;
};
