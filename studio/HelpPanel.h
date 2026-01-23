#pragma once
/*
 * HelpPanel.h — Dockable in-app help & documentation viewer
 *
 * Uses QTextBrowser to render docs/index.html from the app bundle.
 * Provides Back / Forward navigation and an "Open in Browser" button
 * that falls back to QDesktopServices for full CSS rendering.
 *
 * Loaded into StudioMainWindow as a right-side QDockWidget, toggled
 * via Help ▶ Show Help Panel  (Ctrl+Shift+H).
 */

#include <QWidget>
#include <QUrl>

class QTextBrowser;
class QPushButton;
class QLabel;

class HelpPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HelpPanel(QWidget *parent = nullptr);

    // Scroll to a named anchor — pass the bare id, e.g. "quick-start"
    void scrollToAnchor(const QString &anchor);

private slots:
    void onOpenExternal();

private:
    void    buildUi();
    QString locateDocsDir();

    QTextBrowser *m_browser  = nullptr;
    QPushButton  *m_btnBack  = nullptr;
    QPushButton  *m_btnFwd   = nullptr;
    QLabel       *m_titleLbl = nullptr;
};
