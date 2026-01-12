#pragma once
/*
 * StudioMainWindow.h — Mcaster1 Install Studio main window
 *
 * Layout:
 *   [Toolbar]
 *   [ProjectSidebar | [TabWidget: AppInfo | Files | Components |
 *                               Shortcuts | Registry | Security | Build]]
 *   [Build Log — QDockWidget (bottom)]
 *   [Status Bar]
 */

#include <QMainWindow>
#include <QString>
#include "Manifest.h"

class ProjectSidebar;
class AppInfoEditor;
class FilesEditor;
class ComponentsEditor;
class ShortcutsEditor;
class RegistryEditor;
class SecurityEditor;
class BuildPanel;

class QTabWidget;
class QSplitter;
class QDockWidget;
class QPlainTextEdit;
class QLabel;
class QAction;

class StudioMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit StudioMainWindow(QWidget *parent = nullptr);
    ~StudioMainWindow() override = default;

    // Open a .mis project file
    void openProject(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onBuildStart();
    void onImportNsis();
    void onImportInno();
    void onAbout();
    void onTabChanged(int index);

    // Emitted by BuildPanel → routed here for the dock log
    void onBuildLog(const QString &line);
    void onBuildProgress(int pct, const QString &msg);
    void onBuildFinished(bool ok, const QString &outputPath);

private:
    void buildUi();
    void buildMenuBar();
    void buildToolBar();
    void updateWindowTitle();
    void loadEditors();      // push m_manifest into all editor tabs
    void collectEditors();   // pull from all editor tabs into m_manifest
    bool confirmDiscard();   // returns true if safe to discard current project

    QIcon svgIcon(const char *svgStr, int size = 24);

    // ── Data ──────────────────────────────────────────────────────────────
    Manifest m_manifest;
    QString  m_projectPath;
    bool     m_dirty = false;

    // ── Widgets ───────────────────────────────────────────────────────────
    QSplitter      *m_splitter     = nullptr;
    ProjectSidebar *m_sidebar      = nullptr;
    QTabWidget     *m_editorTabs   = nullptr;

    AppInfoEditor    *m_appInfo    = nullptr;
    FilesEditor      *m_files      = nullptr;
    ComponentsEditor *m_components = nullptr;
    ShortcutsEditor  *m_shortcuts  = nullptr;
    RegistryEditor   *m_registry   = nullptr;
    SecurityEditor   *m_security   = nullptr;
    BuildPanel       *m_build      = nullptr;

    QDockWidget    *m_logDock      = nullptr;
    QPlainTextEdit *m_logView      = nullptr;

    QLabel  *m_statusLabel   = nullptr;

    // ── Actions ───────────────────────────────────────────────────────────
    QAction *m_actNew        = nullptr;
    QAction *m_actOpen       = nullptr;
    QAction *m_actSave       = nullptr;
    QAction *m_actSaveAs     = nullptr;
    QAction *m_actBuild      = nullptr;
    QAction *m_actImportNsis = nullptr;
    QAction *m_actImportInno = nullptr;
};
