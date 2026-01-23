/*
 * ProjectSidebar.cpp — Unified project + application navigator
 */

#include "ProjectSidebar.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QHeaderView>
#include <QSignalBlocker>
#include <QFont>
#include <QFileInfo>

static const int kRoleData = Qt::UserRole;      // path (project) | appGroupId (app) | "" (addapp)
static const int kRoleType = Qt::UserRole + 1;  // "project" | "app" | "addapp"

// ── Constructor ───────────────────────────────────────────────────────────────
ProjectSidebar::ProjectSidebar(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────

void ProjectSidebar::refresh(const QList<ProjectEntry> &projects)
{
    QSignalBlocker block(m_tree);
    m_tree->clear();

    for (const ProjectEntry &e : projects) {
        if (e.isActive)
            m_activeProjectPath = e.path;

        QTreeWidgetItem *projNode = addProjectNode(e);

        if (e.isActive) {
            populateApps(projNode, e);
            projNode->setExpanded(true);
        }
    }
}

void ProjectSidebar::setActiveProjectName(const QString &name)
{
    QSignalBlocker block(m_tree);
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *item = m_tree->topLevelItem(i);
        if (item->data(0, kRoleData).toString() == m_activeProjectPath) {
            item->setText(0, name.isEmpty() ? "New Project" : name);
            return;
        }
    }
}

// ── Private slots ─────────────────────────────────────────────────────────────

void ProjectSidebar::onItemClicked(QTreeWidgetItem *item, int /*col*/)
{
    if (!item) return;
    const QString type = item->data(0, kRoleType).toString();

    if (type == "project") {
        const QString path = item->data(0, kRoleData).toString();
        if (path != m_activeProjectPath && !path.isEmpty())
            emit switchProjectRequested(path);
        // expand/collapse is handled automatically by QTreeWidget

    } else if (type == "app") {
        emit applicationSelected(item->data(0, kRoleData).toString());

    } else if (type == "addapp") {
        emit addApplicationRequested();
    }
}

void ProjectSidebar::onItemChanged(QTreeWidgetItem *item, int col)
{
    if (!item || col != 0) return;
    if (item->data(0, kRoleType).toString() != "project") return;

    // Only allow rename of the currently active project
    if (item->data(0, kRoleData).toString() != m_activeProjectPath) {
        // Revert non-active item to its stored filename
        QSignalBlocker block(m_tree);
        const QString path = item->data(0, kRoleData).toString();
        item->setText(0, path.isEmpty() ? "New Project" : QFileInfo(path).baseName());
        return;
    }

    m_pendingRename = item->text(0).trimmed();
    if (!m_pendingRename.isEmpty())
        m_renameTimer->start();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void ProjectSidebar::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Toolbar: header label + New + Open ────────────────────────────────
    auto *toolbar = new QWidget(this);
    toolbar->setObjectName("sidebarHeaderBar");
    toolbar->setFixedHeight(32);

    auto *hb = new QHBoxLayout(toolbar);
    hb->setContentsMargins(8, 4, 6, 4);
    hb->setSpacing(4);

    auto *lbl = new QLabel("PROJECTS", toolbar);
    lbl->setObjectName("sidebarSectionLabel");
    hb->addWidget(lbl);
    hb->addStretch();

    m_btnNew = new QPushButton("＋ New", toolbar);
    m_btnNew->setObjectName("sidebarIconBtn");
    m_btnNew->setFixedHeight(22);
    m_btnNew->setToolTip("Create a new installer project");
    connect(m_btnNew, &QPushButton::clicked, this, &ProjectSidebar::newProjectRequested);
    hb->addWidget(m_btnNew);

    m_btnOpen = new QPushButton("📂 Open", toolbar);
    m_btnOpen->setObjectName("sidebarIconBtn");
    m_btnOpen->setFixedHeight(22);
    m_btnOpen->setToolTip("Open an existing .mis project file");
    connect(m_btnOpen, &QPushButton::clicked, this, &ProjectSidebar::openProjectRequested);
    hb->addWidget(m_btnOpen);

    layout->addWidget(toolbar);

    // ── Unified two-column tree ───────────────────────────────────────────
    // Col 0: expand arrow + name (stretch)
    // Col 1: [✕] close button (fixed 26 px, project rows only)
    m_tree = new QTreeWidget(this);
    m_tree->setObjectName("sidebarTree");
    m_tree->setColumnCount(2);
    m_tree->header()->hide();
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tree->header()->resizeSection(1, 26);
    m_tree->setRootIsDecorated(true);
    m_tree->setAnimated(true);
    m_tree->setIndentation(16);
    m_tree->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(m_tree, 1);

    connect(m_tree, &QTreeWidget::itemClicked,
            this, &ProjectSidebar::onItemClicked);
    connect(m_tree, &QTreeWidget::itemChanged,
            this, &ProjectSidebar::onItemChanged);

    // ── Rename debounce timer ─────────────────────────────────────────────
    m_renameTimer = new QTimer(this);
    m_renameTimer->setSingleShot(true);
    m_renameTimer->setInterval(1200);
    connect(m_renameTimer, &QTimer::timeout, this, [this]() {
        if (!m_pendingRename.isEmpty())
            emit projectNameChanged(m_pendingRename);
    });

    setMinimumWidth(180);
    setMaximumWidth(320);
}

QTreeWidgetItem *ProjectSidebar::addProjectNode(const ProjectEntry &entry)
{
    const QString displayName = entry.name.isEmpty()
        ? (entry.path.isEmpty() ? "New Project" : QFileInfo(entry.path).baseName())
        : entry.name;

    auto *item = new QTreeWidgetItem(m_tree);
    item->setText(0, displayName);
    item->setData(0, kRoleData, entry.path);
    item->setData(0, kRoleType, "project");
    item->setToolTip(0, entry.path.isEmpty() ? "(unsaved — use File > Save to save)" : entry.path);

    if (entry.isActive) {
        // Bold + editable (double-click or F2 to rename inline)
        QFont f = item->font(0);
        f.setBold(true);
        item->setFont(0, f);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    } else {
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    }

    // [✕] in column 1 — only for projects with a known path (can't remove an unsaved project)
    if (!entry.path.isEmpty()) {
        auto *btn = new QPushButton("✕");
        btn->setObjectName("sidebarIconBtn");
        btn->setFixedSize(20, 20);
        btn->setFlat(true);
        btn->setToolTip("Remove from profile (file is NOT deleted from disk)");
        const QString path = entry.path;
        connect(btn, &QPushButton::clicked, this, [this, path]() {
            emit removeProjectFromProfile(path);
        });
        m_tree->setItemWidget(item, 1, btn);
    }

    return item;
}

void ProjectSidebar::populateApps(QTreeWidgetItem *projectNode,
                                  const ProjectEntry &entry)
{
    // One child item per AppGroup
    for (int i = 0; i < entry.appGroupIds.size(); ++i) {
        const QString name = (i < entry.appGroupNames.size() && !entry.appGroupNames[i].isEmpty())
                             ? entry.appGroupNames[i]
                             : entry.appGroupIds[i];

        auto *appItem = new QTreeWidgetItem(projectNode);
        appItem->setText(0, "  " + name);
        appItem->setData(0, kRoleData, entry.appGroupIds[i]);
        appItem->setData(0, kRoleType, "app");
        appItem->setFlags(appItem->flags() & ~Qt::ItemIsEditable);
        appItem->setToolTip(0, QString("Application: %1\nClick to navigate to Files tab").arg(name));
    }

    // "＋ Add Application…" pseudo-item always at bottom
    auto *addItem = new QTreeWidgetItem(projectNode);
    addItem->setText(0, "  ＋ Add Application\u2026");
    addItem->setData(0, kRoleData, QString());
    addItem->setData(0, kRoleType, "addapp");
    addItem->setFlags(addItem->flags() & ~Qt::ItemIsEditable);
    addItem->setToolTip(0, "Add a new application to this project");
    QFont f = addItem->font(0);
    f.setItalic(true);
    addItem->setFont(0, f);
}
