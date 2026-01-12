/*
 * ProjectSidebar.cpp — Left-side project navigation tree
 */

#include "ProjectSidebar.h"

#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QFont>

ProjectSidebar::ProjectSidebar(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void ProjectSidebar::populate(const Manifest &m)
{
    m_tree->clear();

    // ── Root: application name ─────────────────────────────────────────────
    QString appName = m.app.name.isEmpty() ? "New Project" : m.app.name;
    m_appNameLabel->setText(appName);

    QTreeWidgetItem *root = new QTreeWidgetItem(m_tree);
    root->setText(0, appName);
    root->setData(0, Qt::UserRole, "root");
    root->setExpanded(true);

    // ── App Info ──────────────────────────────────────────────────────────
    QTreeWidgetItem *appInfo = new QTreeWidgetItem(root);
    appInfo->setText(0, "App Info");
    appInfo->setData(0, Qt::UserRole, "appinfo");

    // ── Files ─────────────────────────────────────────────────────────────
    QTreeWidgetItem *files = new QTreeWidgetItem(root);
    files->setText(0, "Files");
    files->setData(0, Qt::UserRole, "files");

    // ── Components ────────────────────────────────────────────────────────
    QTreeWidgetItem *comps = new QTreeWidgetItem(root);
    comps->setText(0, QString("Components (%1)").arg(m.components.size()));
    comps->setData(0, Qt::UserRole, "components");
    comps->setExpanded(true);

    for (const Component &c : m.components) {
        QTreeWidgetItem *ci = new QTreeWidgetItem(comps);
        ci->setText(0, c.name.isEmpty() ? c.id : c.name);
        ci->setData(0, Qt::UserRole, "component:" + c.id);
    }

    // ── Shortcuts ─────────────────────────────────────────────────────────
    QTreeWidgetItem *shortcuts = new QTreeWidgetItem(root);
    shortcuts->setText(0, QString("Shortcuts (%1)").arg(m.shortcuts.size()));
    shortcuts->setData(0, Qt::UserRole, "shortcuts");

    // ── Registry ─────────────────────────────────────────────────────────
    QTreeWidgetItem *registry = new QTreeWidgetItem(root);
    registry->setText(0, QString("Registry (%1)").arg(m.registry.size()));
    registry->setData(0, Qt::UserRole, "registry");

    // ── Security / Signing ────────────────────────────────────────────────
    QTreeWidgetItem *security = new QTreeWidgetItem(root);
    security->setText(0, "Security & Signing");
    security->setData(0, Qt::UserRole, "security");

    // ── Build ─────────────────────────────────────────────────────────────
    QTreeWidgetItem *buildNode = new QTreeWidgetItem(root);
    buildNode->setText(0, "Build");
    buildNode->setData(0, Qt::UserRole, "build");

    // ── Targets summary ───────────────────────────────────────────────────
    QTreeWidgetItem *targets = new QTreeWidgetItem(root);
    targets->setText(0, QString("Targets: %1").arg(
        m.targets.isEmpty() ? "none" : m.targets.join(", ")));
    targets->setData(0, Qt::UserRole, "");
    targets->setDisabled(true);

    m_tree->expandAll();
}

void ProjectSidebar::onItemClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (!item) return;
    QString key = item->data(0, Qt::UserRole).toString();
    if (!key.isEmpty())
        emit navigateTo(key);
}

void ProjectSidebar::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Section label ──────────────────────────────────────────────────────
    QLabel *sectionLbl = new QLabel("PROJECT", this);
    sectionLbl->setObjectName("sectionLabel");
    sectionLbl->setContentsMargins(10, 10, 10, 6);
    layout->addWidget(sectionLbl);

    // ── App name label ────────────────────────────────────────────────────
    m_appNameLabel = new QLabel("(no project)", this);
    m_appNameLabel->setObjectName("titleLabel");
    m_appNameLabel->setContentsMargins(10, 0, 10, 8);
    m_appNameLabel->setWordWrap(true);
    layout->addWidget(m_appNameLabel);

    // ── Tree ──────────────────────────────────────────────────────────────
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setAnimated(true);
    m_tree->setIndentation(14);
    m_tree->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_tree);

    connect(m_tree, &QTreeWidget::itemClicked, this, &ProjectSidebar::onItemClicked);

    setMinimumWidth(180);
    setMaximumWidth(320);
}
