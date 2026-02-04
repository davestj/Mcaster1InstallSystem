#pragma once
/*
 * ProjectSidebar.h — Unified Project + Application navigator
 *
 * Layout (top to bottom):
 *   [PROJECTS]  [＋ New]  [📂 Open]            ← header toolbar
 *   ────────────────────────────────────────────
 *   QTreeWidget (single column — right-click for all actions)
 *     ▼ My Custom Project Name                  ← active project (bold, expanded)
 *         ● My Application 1                    ← active app (bold + bullet)
 *           My Application 2                    ← inactive app
 *           ＋ Add Application…                 ← addapp node (italic)
 *     ▶ Project Name No.2                       ← inactive project (collapsed)
 *
 * Right-click on project node:
 *   Rename Project
 *   ───────────────────
 *   Copy Project…
 *   ───────────────────
 *   Close Project        (remove from session, file stays on disk)
 *   ───────────────────
 *   Delete Project       (remove from session + delete file — with confirmation)
 *
 * Right-click on application node:
 *   Set as Active Application
 *   ───────────────────
 *   Delete Application   (with confirmation)
 *
 * Signals:
 *   newProjectRequested()                    — toolbar [＋ New]
 *   openProjectRequested()                   — toolbar [📂 Open]
 *   switchProjectRequested(path)             — click an inactive project node
 *   closeProjectRequested(path)              — "Close Project" context menu
 *   copyProjectRequested(path)               — "Copy Project…" context menu
 *   deleteProjectRequested(path)             — "Delete Project" context menu (after confirm)
 *   addApplicationRequested()                — click ＋ Add Application…
 *   applicationSelected(appGroupId)          — click or "Set as Active" on an app node
 *   deleteApplicationRequested(appGroupId)   — "Delete Application" context menu (after confirm)
 *   renameProjectRequested(path, newName)    — debounced 1200ms after inline rename (any project)
 */

#include <QWidget>
#include <QList>
#include <QString>
#include <QStringList>

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QTimer;
class QLabel;

class ProjectSidebar : public QWidget
{
    Q_OBJECT

public:
    // Describes one project for the sidebar tree
    struct ProjectEntry {
        QString     path;               // absolute path to .mis; empty = unsaved new project
        QString     name;               // display name (app.name from manifest)
        QStringList appGroupIds;        // populated for all open projects
        QStringList appGroupNames;      // parallel to appGroupIds
        bool        isActive          = false;
        QString     activeAppGroupId; // which AppGroup is currently selected in editors
    };

    explicit ProjectSidebar(QWidget *parent = nullptr);

    // Full tree rebuild — called by StudioMainWindow on project load/switch/profile change
    void refresh(const QList<ProjectEntry> &projects);

    // Lightweight: update the active project's display name without full rebuild
    void setActiveProjectName(const QString &name);

    // Mark an app group as visually active (bold + bullet) in the tree
    void setActiveAppGroup(const QString &id);

signals:
    void newProjectRequested();
    void openProjectRequested();
    void switchProjectRequested(const QString &path);
    void closeProjectRequested(const QString &path);            // remove from session (keep file)
    void copyProjectRequested(const QString &path);             // duplicate project
    void deleteProjectRequested(const QString &path);           // remove + delete file (after confirm)
    void addApplicationRequested();
    void applicationSelected(const QString &appGroupId);        // left-click or "Set as Active"
    void deleteApplicationRequested(const QString &appGroupId); // after confirmation
    void renameProjectRequested(const QString &path,
                                const QString &newName);        // debounced 1200ms

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemChanged(QTreeWidgetItem *item, int column);

private:
    void showContextMenu(QTreeWidgetItem *item, const QPoint &globalPos);

private:
    void buildUi();
    QTreeWidgetItem *addProjectNode(const ProjectEntry &entry);
    void             populateApps(QTreeWidgetItem *projectNode,
                                  const ProjectEntry &entry);

    QTreeWidget *m_tree              = nullptr;
    QPushButton *m_btnNew            = nullptr;
    QPushButton *m_btnOpen           = nullptr;
    QTimer      *m_renameTimer       = nullptr;
    QString      m_pendingRename;
    QString      m_pendingRenamePath; // path of item being renamed (may be empty for unsaved)
    QString      m_activeProjectPath;
    QString      m_activeAppGroupId;
};
