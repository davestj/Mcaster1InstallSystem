#pragma once
/*
 * PrerequisitesPage.h — Wizard page that checks and reports software prerequisites.
 *
 * Displays a table of Name | Status | Action rows.
 * Each check command is run via QProcess::execute(); status is updated live.
 * Users may proceed even if some prerequisites are missing (advisory, not blocking).
 * A "Recheck All" button allows re-running all checks after the user installs something.
 */

#include <QWizardPage>
#include <QList>

class QTableWidget;
class QPushButton;
class QLabel;

class PrerequisitesPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PrerequisitesPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool isComplete()    const override;  // always true — non-blocking

private slots:
    void onRecheckAll();
    void onActionClicked(int row);

private:
    void runChecks();
    void updateStatusRow(int row, bool ok);
    QString currentPlatform() const;

    QTableWidget *m_table    = nullptr;
    QPushButton  *m_btnCheck = nullptr;
    QLabel       *m_summary  = nullptr;

    bool m_populated = false;
};
