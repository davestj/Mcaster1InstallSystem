#pragma once
/*
 * ProjectSidebar.h — Unified Project + Application navigator
 *
 * Layout (top to bottom):
 *   [PROJECTS]  [＋ New]  [📂 Open]      ← header toolbar
 *   ──────────────────────────────────────
 *   QTreeWidget (col 0: name | col 1: [✕])
 *     ▼ My Custom Project Name    [✕]    ← active project (bold, expanded)
 *         My Custom Application 1        ← app node
 *         My Application 2               ← app node
 *         ＋ Add Application…            ← addapp node (italic)
 *     ▶ Project Name No.2         [✕]    ← inactive project (collapsed)
 *
 * Signals:
 *   newProjectRequested()              — toolbar [＋ New]
 *   openProjectRequested()             — toolbar [📂 Open]
 *   switchProjectRequested(path)       — click an inactive project node
 *   removeProjectFromProfile(path)     — [✕] button
 *   addApplicationRequested()          — click ＋ Add Application…
 *   applicationSelected(appGroupId)    — click an application node
 *   projectNameChanged(name)           — debounced 1200ms after inline rename
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
        QString     path;           // absolute path to .mis; empty = unsaved new project
        QString     name;           // display name (app.name from manifest)
        QStringList appGroupIds;    // only populated for the active project
        QStringList appGroupNames;  // parallel to appGroupIds
        bool        isActive = false;
    };

    explicit ProjectSidebar(QWidget *parent = nullptr);

    // Full tree rebuild — called by StudioMainWindow on project load/switch/profile change
    void refresh(const QList<ProjectEntry> &projects);

    // Lightweight: update the active project's display name without full rebuild
    void setActiveProjectName(const QString &name);

signals:
    void newProjectRequested();
    void openProjectRequested();
    void switchProjectRequested(const QString &path);
    void removeProjectFromProfile(const QString &path);
    void addApplicationRequested();
    void applicationSelected(const QString &appGroupId);
    void projectNameChanged(const QString &name);    // debounced 1200ms

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onItemChanged(QTreeWidgetItem *item, int column);

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
    QString      m_activeProjectPath;
};
