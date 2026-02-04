#pragma once
/*
 * StudioMainWindow.h — Mcaster1 Install Studio main window
 *
 * Layout:
 *   [Toolbar 1: Logo | File actions | Build | ───spacer─── | HUD status | Clock]
 *   [Toolbar 2: 👤 Profile: [ComboBox▼] [Manage…]  │  info label]
 *   [ProjectSidebar | [TabWidget: AppInfo | Files | Components |
 *                               Shortcuts | Registry | Security |
 *                               Prerequisites | Actions | Build]]
 *   [EventLog dock (bottom)]   [BuildHistory dock (right)]   [Help dock (right)]
 *   [Status Bar: [Project: name v1.0] ─── [Studio v1.0.0] ]
 *
 * Multi-project model
 * ───────────────────
 *   m_openProjects  — all currently-open projects held in memory simultaneously
 *   m_activeIdx     — which project is shown in the editor tabs
 *
 *   Opening a new project adds it to the list (never closes others).
 *   Switching projects in the sidebar auto-saves the current one silently.
 *   No "discard changes?" prompts — changes auto-save after each edit.
 *   Ctrl+S / Save button saves the active project in-place (or asks for a
 *   path if it has never been saved).
 */

#include <QMainWindow>
#include <QString>
#include <QList>
#include "Manifest.h"
#include "BuilderProfile.h"
#include "StudioStyle.h"

class ProjectSidebar;
class AppInfoEditor;
class FilesEditor;
class ComponentsEditor;
class ShortcutsEditor;
class RegistryEditor;
class SecurityEditor;
class PrerequisitesEditor;
class CustomActionsEditor;
class BuildPanel;
class EventLog;
class BuildHistory;
class HelpPanel;

class QTabWidget;
class QSplitter;
class QDockWidget;
class QPlainTextEdit;
class QLabel;
class QAction;
class QComboBox;
class QTimer;

class StudioMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit StudioMainWindow(QWidget *parent = nullptr);
    ~StudioMainWindow() override = default;

    // Open a .mis file — adds to the open project list (or switches if already open)
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
    void onOpenHelp();
    void onOpenBuildHistory();
    void onToggleHelpPanel();

    // Builder profile slots
    void onProfileChanged(int index);
    void onManageProfiles();

    // Clock
    void onClockTick();

    // BuildPanel signals
    void onBuildLog(const QString &line);
    void onBuildProgress(int pct, const QString &msg);
    void onBuildFinished(bool ok, const QString &outputPath);

    // ProjectSidebar signals
    void onApplicationSelected(const QString &appGroupId);
    void onSwitchProject(const QString &path);
    void onCloseProject(const QString &path);    // removes from session, not disk
    void onCopyProject(const QString &path);     // duplicate project with new name
    void onDeleteProject(const QString &path);   // remove from session + delete .mis file
    void onAddApplication();
    void onDeleteApplication(const QString &appGroupId);
    void onRenameProject(const QString &path, const QString &newName);  // any project
    void onProjectNameChanged(const QString &name);  // AppInfoEditor→active project only

    // Editor change signals
    void onProjectModified();
    void onProjectInfoChanged();
    void onAutoSave();

    // Theme switching
    void onThemeChanged(StudioStyle::ThemeManager::ThemeId id);

private:
    // ── Per-project record (all open simultaneously in memory) ────────────
    struct OpenProject {
        Manifest manifest;
        QString  path;              // abs path to .mis; empty = never saved
        bool     dirty  = false;
        QString  tempId;            // UUID used as key before first save
        QString  activeAppGroupId;  // which AppGroup is shown in editors
        QString  key()  const { return path.isEmpty() ? tempId : path; }
    };

    bool               hasActive() const {
        return m_activeIdx >= 0 && m_activeIdx < m_openProjects.size();
    }
    OpenProject       &active()       { return m_openProjects[m_activeIdx]; }
    const OpenProject &active() const { return m_openProjects[m_activeIdx]; }

    // Switch the editor panel to a different open project (auto-saves current)
    void switchToProject(int index);

    // Silently save the active project if dirty and it has a path
    void autoSaveActive();

    void buildUi();
    void buildMenuBar();
    void buildToolBar();
    void updateWindowTitle();
    void updateProjectInfoLabel();
    void updateHud(const char *iconSvg, const QString &tip);
    void loadEditors();      // push active().manifest into all editor tabs
    void collectEditors();   // pull from all editor tabs into active().manifest
    void refreshProfileCombo();
    void rebuildIcons();
    void associateProjectWithProfile(const QString &path);
    void refreshSidebar();
    static QString sanitizeFilename(const QString &name);

    QIcon svgIcon(const char *svgStr, int size = 24);

    // ── Data ──────────────────────────────────────────────────────────────
    QList<OpenProject>    m_openProjects;
    int                   m_activeIdx      = -1;
    bool                  m_loading        = false;  // guard during loadEditors()
    BuilderProfileManager m_profileManager;

    // ── Widgets ───────────────────────────────────────────────────────────
    QSplitter      *m_splitter     = nullptr;
    ProjectSidebar *m_sidebar      = nullptr;
    QTabWidget     *m_editorTabs   = nullptr;

    AppInfoEditor       *m_appInfo    = nullptr;
    FilesEditor         *m_files      = nullptr;
    ComponentsEditor    *m_components = nullptr;
    ShortcutsEditor     *m_shortcuts  = nullptr;
    RegistryEditor      *m_registry   = nullptr;
    SecurityEditor      *m_security   = nullptr;
    PrerequisitesEditor *m_prereqs    = nullptr;
    CustomActionsEditor *m_actions    = nullptr;
    BuildPanel          *m_build      = nullptr;

    // Docks
    QDockWidget    *m_logDock        = nullptr;
    QDockWidget    *m_historyDock    = nullptr;
    QDockWidget    *m_helpDock       = nullptr;
    EventLog       *m_eventLog       = nullptr;
    BuildHistory   *m_buildHistory   = nullptr;
    HelpPanel      *m_helpPanel      = nullptr;

    // Auto-save timer (fires 1500ms after last edit)
    QTimer     *m_autoSaveTimer      = nullptr;

    // Toolbar / HUD
    QComboBox  *m_profileCombo       = nullptr;
    QLabel     *m_hudIcon            = nullptr;
    QLabel     *m_clockLabel         = nullptr;
    QTimer     *m_clockTimer         = nullptr;

    // Status bar
    QLabel     *m_projectInfoLabel   = nullptr;
    QLabel     *m_studioVersionLabel = nullptr;

    // ── Actions ───────────────────────────────────────────────────────────
    QAction *m_actNew        = nullptr;
    QAction *m_actOpen       = nullptr;
    QAction *m_actSave       = nullptr;
    QAction *m_actSaveAs     = nullptr;
    QAction *m_actBuild      = nullptr;
    QAction *m_actImportNsis = nullptr;
    QAction *m_actImportInno = nullptr;

    QAction *m_actThemeDark       = nullptr;
    QAction *m_actThemeEnterprise = nullptr;
};
