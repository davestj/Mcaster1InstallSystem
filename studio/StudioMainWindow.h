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
 */

#include <QMainWindow>
#include <QString>
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
    void onOpenHelp();
    void onOpenBuildHistory();
    void onToggleHelpPanel();

    // Builder profile slots
    void onProfileChanged(int index);
    void onManageProfiles();

    // Clock
    void onClockTick();

    // Emitted by BuildPanel → routed here for the log dock + HUD
    void onBuildLog(const QString &line);
    void onBuildProgress(int pct, const QString &msg);
    void onBuildFinished(bool ok, const QString &outputPath);

    // Emitted by ProjectSidebar → navigate to that application's Files tab
    void onApplicationSelected(const QString &appGroupId);

    // Mark project dirty when any editor signals a change
    void onProjectModified();

    // Update project info label in status bar when AppInfo changes
    void onProjectInfoChanged();

    // Inline sidebar name edit + auto-save
    void onProjectNameChanged(const QString &name);
    void onAutoSave();

    // Sidebar action slots
    void onAddApplication();
    void onSwitchProject(const QString &path);
    void onRemoveProjectFromProfile(const QString &path);

    // Theme switching
    void onThemeChanged(StudioStyle::ThemeManager::ThemeId id);

private:
    void buildUi();
    void buildMenuBar();
    void buildToolBar();
    void updateWindowTitle();
    void updateProjectInfoLabel();
    void updateHud(const char *iconSvg, const QString &tip);
    void loadEditors();      // push m_manifest into all editor tabs
    void collectEditors();   // pull from all editor tabs into m_manifest
    bool confirmDiscard();   // returns true if safe to discard current project
    void refreshProfileCombo();
    void rebuildIcons();     // re-render all toolbar + tab icons for current theme
    void associateProjectWithProfile(const QString &path);
    void refreshSidebar();
    static QString sanitizeFilename(const QString &name);

    QIcon svgIcon(const char *svgStr, int size = 24);

    // ── Data ──────────────────────────────────────────────────────────────
    Manifest             m_manifest;
    QString              m_projectPath;
    bool                 m_dirty       = false;
    bool                 m_initialized = false;
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

    // Auto-save timer (1500ms debounce after inline name edit)
    QTimer     *m_autoSaveTimer      = nullptr;

    // Toolbar / HUD widgets
    QComboBox  *m_profileCombo       = nullptr;
    QLabel     *m_hudIcon            = nullptr;  // SVG status icon pixmap
    QLabel     *m_clockLabel         = nullptr;
    QTimer     *m_clockTimer         = nullptr;

    // Status bar labels
    QLabel     *m_projectInfoLabel   = nullptr;  // "Project: AppName v1.0"
    QLabel     *m_studioVersionLabel = nullptr;  // "Studio v1.0.0"

    // ── Actions ───────────────────────────────────────────────────────────
    QAction *m_actNew        = nullptr;
    QAction *m_actOpen       = nullptr;
    QAction *m_actSave       = nullptr;
    QAction *m_actSaveAs     = nullptr;
    QAction *m_actBuild      = nullptr;
    QAction *m_actImportNsis = nullptr;
    QAction *m_actImportInno = nullptr;

    // Theme toggle actions (checked = active theme)
    QAction *m_actThemeDark       = nullptr;
    QAction *m_actThemeEnterprise = nullptr;
};
