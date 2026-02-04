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
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QEvent>
#include <QContextMenuEvent>

static const int kRoleData     = Qt::UserRole;      // path (project) | appGroupId (app)
static const int kRoleType     = Qt::UserRole + 1;  // "project" | "app" | "addapp"
static const int kRoleIsActive = Qt::UserRole + 2;  // bool — active project flag

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
        if (e.isActive) {
            m_activeProjectPath = e.path;
            m_activeAppGroupId  = e.activeAppGroupId;
        }

        QTreeWidgetItem *projNode = addProjectNode(e);
        populateApps(projNode, e);
        projNode->setExpanded(e.isActive);  // active project starts expanded
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

void ProjectSidebar::setActiveAppGroup(const QString &id)
{
    m_activeAppGroupId = id;

    // Walk all app items and toggle bold + bullet prefix
    QSignalBlocker block(m_tree);
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *proj = m_tree->topLevelItem(i);
        for (int j = 0; j < proj->childCount(); ++j) {
            auto *child = proj->child(j);
            if (child->data(0, kRoleType).toString() != "app") continue;

            const bool active = (child->data(0, kRoleData).toString() == id);
            QFont f = child->font(0);
            f.setBold(active);
            child->setFont(0, f);

            // Bullet prefix indicates the active app
            const QString rawName = child->text(0).mid(2).trimmed();  // strip old prefix
            child->setText(0, (active ? "  \u25cf " : "  ") + rawName);
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
        // Only switch if clicking a *different* project
        if (path != m_activeProjectPath)
            emit switchProjectRequested(path);

    } else if (type == "app") {
        const QString id = item->data(0, kRoleData).toString();
        setActiveAppGroup(id);
        emit applicationSelected(id);

    } else if (type == "addapp") {
        emit addApplicationRequested();
    }
}

void ProjectSidebar::onItemChanged(QTreeWidgetItem *item, int col)
{
    if (!item || col != 0) return;
    if (item->data(0, kRoleType).toString() != "project") return;

    const QString path = item->data(0, kRoleData).toString();
    m_pendingRename     = item->text(0).trimmed();
    m_pendingRenamePath = path;
    if (!m_pendingRename.isEmpty())
        m_renameTimer->start();
}

// ── Event filter — intercepts right-click on the viewport ─────────────────────

bool ProjectSidebar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_tree->viewport() && event->type() == QEvent::ContextMenu) {
        auto *ce   = static_cast<QContextMenuEvent *>(event);
        // ce->pos() is in viewport coordinates — exactly what itemAt() expects
        QTreeWidgetItem *item = m_tree->itemAt(ce->pos());
        if (item)
            showContextMenu(item, ce->globalPos());
        return true;  // always consume so Qt doesn't open a default menu
    }
    return QWidget::eventFilter(obj, event);
}

void ProjectSidebar::showContextMenu(QTreeWidgetItem *item, const QPoint &globalPos)
{
    const QString type = item->data(0, kRoleType).toString();

    // ── Project context menu ──────────────────────────────────────────────
    if (type == "project") {
        const QString path = item->data(0, kRoleData).toString();
        const QString name = item->text(0);

        QMenu menu(this);

        auto *actRename = menu.addAction("Rename Project");
        actRename->setToolTip("Rename this project (F2 or double-click also works)");

        menu.addSeparator();
        auto *actCopy = menu.addAction("Copy Project\u2026");
        actCopy->setToolTip("Duplicate this project with a new name");

        menu.addSeparator();
        auto *actClose = menu.addAction("Close Project");
        actClose->setToolTip("Remove from session \u2014 file is NOT deleted from disk");

        menu.addSeparator();
        auto *actDelete = menu.addAction("Delete Project");
        actDelete->setToolTip("Close and permanently delete the .mis file from disk");

        connect(actRename, &QAction::triggered, this, [this, item]() {
            m_tree->editItem(item, 0);
        });
        connect(actCopy, &QAction::triggered, this, [this, path]() {
            emit copyProjectRequested(path);
        });
        connect(actClose, &QAction::triggered, this, [this, path]() {
            emit closeProjectRequested(path);
        });
        connect(actDelete, &QAction::triggered, this, [this, path, name]() {
            const int ret = QMessageBox::warning(
                this, "Delete Project",
                QString("Permanently delete \"%1\" and remove its .mis file from disk?\n\n"
                        "This cannot be undone.").arg(name),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
            if (ret == QMessageBox::Yes)
                emit deleteProjectRequested(path);
        });

        menu.exec(globalPos);

    // ── Application context menu ──────────────────────────────────────────
    } else if (type == "app") {
        const QString appId   = item->data(0, kRoleData).toString();
        const QString appName = item->text(0).trimmed().remove(QChar(0x25cf)).trimmed();

        QMenu menu(this);

        auto *actSetActive = menu.addAction("Set as Active Application");
        actSetActive->setToolTip("Switch editors to show this application's files and settings");

        menu.addSeparator();
        auto *actDelete = menu.addAction("Delete Application");
        actDelete->setToolTip(QString("Remove \"%1\" from this project").arg(appName));

        connect(actSetActive, &QAction::triggered, this, [this, appId]() {
            setActiveAppGroup(appId);
            emit applicationSelected(appId);
        });
        connect(actDelete, &QAction::triggered, this, [this, appId, appName]() {
            const int ret = QMessageBox::warning(
                this, "Delete Application",
                QString("Remove application \"%1\" from this project?\n\n"
                        "All files, components, and settings for this application will be lost.")
                    .arg(appName),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
            if (ret == QMessageBox::Yes)
                emit deleteApplicationRequested(appId);
        });

        menu.exec(globalPos);
    }
    // "addapp" node: no context menu
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

    m_btnNew = new QPushButton("\uff0b New", toolbar);
    m_btnNew->setObjectName("sidebarIconBtn");
    m_btnNew->setFixedHeight(22);
    m_btnNew->setToolTip("Create a new installer project");
    connect(m_btnNew, &QPushButton::clicked, this, &ProjectSidebar::newProjectRequested);
    hb->addWidget(m_btnNew);

    m_btnOpen = new QPushButton("\U0001f4c2 Open", toolbar);
    m_btnOpen->setObjectName("sidebarIconBtn");
    m_btnOpen->setFixedHeight(22);
    m_btnOpen->setToolTip("Open an existing .mis project file");
    connect(m_btnOpen, &QPushButton::clicked, this, &ProjectSidebar::openProjectRequested);
    hb->addWidget(m_btnOpen);

    layout->addWidget(toolbar);

    // ── Single-column tree — right-click for all project/app actions ──────
    m_tree = new QTreeWidget(this);
    m_tree->setObjectName("sidebarTree");
    m_tree->setColumnCount(1);
    m_tree->header()->hide();
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->setRootIsDecorated(true);
    m_tree->setAnimated(true);
    m_tree->setIndentation(16);
    m_tree->setFocusPolicy(Qt::StrongFocus);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_tree, 1);

    connect(m_tree, &QTreeWidget::itemClicked,
            this, &ProjectSidebar::onItemClicked);
    connect(m_tree, &QTreeWidget::itemChanged,
            this, &ProjectSidebar::onItemChanged);

    // Install event filter on viewport — the reliable way to intercept right-click
    // context menu events on a QTreeWidget regardless of coordinate system nuances.
    m_tree->viewport()->installEventFilter(this);

    // ── Rename debounce timer ─────────────────────────────────────────────
    m_renameTimer = new QTimer(this);
    m_renameTimer->setSingleShot(true);
    m_renameTimer->setInterval(1200);
    connect(m_renameTimer, &QTimer::timeout, this, [this]() {
        if (!m_pendingRename.isEmpty())
            emit renameProjectRequested(m_pendingRenamePath, m_pendingRename);
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
    item->setData(0, kRoleData,     entry.path);
    item->setData(0, kRoleType,     QString("project"));
    item->setData(0, kRoleIsActive, entry.isActive);
    item->setToolTip(0, entry.path.isEmpty()
                     ? "(unsaved \u2014 use File \u25b8 Save to save)"
                     : entry.path);

    // All project nodes are editable — F2 or double-click to rename inline
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    if (entry.isActive) {
        QFont f = item->font(0);
        f.setBold(true);
        item->setFont(0, f);
    }

    return item;
}

void ProjectSidebar::populateApps(QTreeWidgetItem *projectNode,
                                  const ProjectEntry &entry)
{
    for (int i = 0; i < entry.appGroupIds.size(); ++i) {
        const QString &id   = entry.appGroupIds[i];
        const QString  name = (i < entry.appGroupNames.size() && !entry.appGroupNames[i].isEmpty())
                              ? entry.appGroupNames[i]
                              : id;

        const bool isActiveApp = (!entry.activeAppGroupId.isEmpty() &&
                                  id == entry.activeAppGroupId);

        auto *appItem = new QTreeWidgetItem(projectNode);
        // Bullet prefix for the active app
        appItem->setText(0, (isActiveApp ? "  \u25cf " : "  ") + name);
        appItem->setData(0, kRoleData, id);
        appItem->setData(0, kRoleType, QString("app"));
        appItem->setFlags(appItem->flags() & ~Qt::ItemIsEditable);
        appItem->setToolTip(0, QString("Application: %1\n"
                                       "Left-click to navigate \u2014 Right-click for options")
                                .arg(name));

        if (isActiveApp) {
            QFont f = appItem->font(0);
            f.setBold(true);
            appItem->setFont(0, f);
        }
    }

    // "＋ Add Application…" pseudo-item always at bottom
    auto *addItem = new QTreeWidgetItem(projectNode);
    addItem->setText(0, "  \uff0b Add Application\u2026");
    addItem->setData(0, kRoleData, QString());
    addItem->setData(0, kRoleType, QString("addapp"));
    addItem->setFlags(addItem->flags() & ~Qt::ItemIsEditable);
    addItem->setToolTip(0, "Add a new application group to this project");
    QFont f = addItem->font(0);
    f.setItalic(true);
    addItem->setFont(0, f);
}
