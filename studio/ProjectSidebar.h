#pragma once
/*
 * ProjectSidebar.h — Left-side project tree widget.
 *
 * Shows a hierarchical view of the .mis project:
 *   My Application (root)
 *   ├── App Info
 *   ├── Components
 *   │   ├── core
 *   │   └── docs
 *   ├── Shortcuts
 *   ├── Registry
 *   └── Targets
 */

#include <QWidget>
#include "Manifest.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;

class ProjectSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectSidebar(QWidget *parent = nullptr);

    void populate(const Manifest &m);

signals:
    void navigateTo(const QString &section);  // e.g. "appinfo", "files", "build"

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);

private:
    void buildUi();

    QLabel      *m_appNameLabel = nullptr;
    QTreeWidget *m_tree         = nullptr;
};
