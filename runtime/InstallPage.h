#pragma once
/*
 * InstallPage.h — Live installation progress page.
 *
 * Creates InstallEngine, moves it to a background QThread, starts the
 * install when the page is shown.  isComplete() returns true only after
 * the engine emits finished().
 */
#include <QWizardPage>

class QProgressBar;
class QPlainTextEdit;
class QLabel;
class QThread;
class InstallEngine;

class InstallPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit InstallPage(QWidget *parent = nullptr);
    void initializePage() override;
    bool isComplete() const override;

private slots:
    void onProgress(int pct, const QString &msg);
    void onLogLine(const QString &line);
    void onFinished(bool success, const QString &error);

private:
    QProgressBar   *m_bar     = nullptr;
    QLabel         *m_status  = nullptr;
    QPlainTextEdit *m_log     = nullptr;
    bool            m_done    = false;
    bool            m_started = false;
};
