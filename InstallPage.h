#pragma once
#include <QWizardPage>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QLabel>
#include <QThread>

class InstallWorker;

class InstallPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit InstallPage(QWidget *parent = nullptr);
    ~InstallPage() override;

    void initializePage() override;
    bool isComplete() const override;
    int  nextId() const override;

private slots:
    void onLogLine(const QString &line);
    void onProgress(int percent);
    void onFinished(bool success, const QString &message);

private:
    QProgressBar   *m_progress;
    QPlainTextEdit *m_log;
    QLabel         *m_statusLabel;
    QThread        *m_thread  = nullptr;
    InstallWorker  *m_worker  = nullptr;
    bool            m_done    = false;
    bool            m_success = false;
};
