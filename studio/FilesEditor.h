#pragma once
/*
 * FilesEditor.h — Multi-app group + component tree with per-file OS targeting.
 *
 * Left panel:  2-level collapsible QTreeWidget
 *              Level 0: App Group nodes (folder icon), keyed by AppGroup::id
 *                       "(No Group)" virtual node for ungrouped components
 *              Level 1: Component nodes (child of their group)
 *
 * Right panel: QStackedWidget — 3 pages
 *              Page 0 (empty):     hint label — "Select a group or component"
 *              Page 1 (group):     id / name / description fields for an AppGroup
 *              Page 2 (component): component props + 7-column file table
 *                                  File columns: Source | Destination | Dir | chmod |
 *                                                macOS | Windows | Linux
 */

#include <QWidget>
#include "Manifest.h"

class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QStackedWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QPushButton;
class QLabel;

class FilesEditor : public QWidget
{
    Q_OBJECT

public:
    explicit FilesEditor(QWidget *parent = nullptr);

    void load(const Manifest &m);
    void save(Manifest &m) const;

signals:
    void modified();

private slots:
    void onTreeSelectionChanged();
    void onAddGroup();
    void onAddComponent();
    void onRemoveSelected();
    void onAddFile();
    void onRemoveFile();
    void onMoveUp();
    void onMoveDown();

private:
    enum class SelType { None, Group, Component };

    void buildUi();
    void rebuildTree();

    // Tree item lookup helpers (searches all top-level + children)
    QTreeWidgetItem *groupItem(const QString &groupId) const;
    QTreeWidgetItem *compItem(const QString &compId)   const;

    void showEmptyPage();
    void showGroupEditor(const QString &groupId);
    void showComponentEditor(const QString &compId);

    void flushGroupEdits();
    void flushComponentEdits();

    void populateFileTable(const QString &compId);
    void refreshGroupCombo();  // repopulate m_compGroup from m_groups

    // ── UI ────────────────────────────────────────────────────────────────────
    QSplitter      *m_splitter    = nullptr;
    QTreeWidget    *m_tree        = nullptr;

    // Left toolbar
    QPushButton    *m_btnAddGroup = nullptr;
    QPushButton    *m_btnAddComp  = nullptr;
    QPushButton    *m_btnRemove   = nullptr;

    // Right: stacked pages
    QStackedWidget *m_stack       = nullptr;

    // Page 1 — Group editor widgets
    QLineEdit      *m_grpId       = nullptr;
    QLineEdit      *m_grpName     = nullptr;
    QLineEdit      *m_grpDesc     = nullptr;

    // Page 2 — Component editor widgets
    QLineEdit      *m_compId      = nullptr;
    QLineEdit      *m_compName    = nullptr;
    QLineEdit      *m_compDesc    = nullptr;
    QCheckBox      *m_compReq     = nullptr;
    QCheckBox      *m_compSel     = nullptr;
    QCheckBox      *m_compMacos   = nullptr;  // component-level platform support
    QCheckBox      *m_compWindows = nullptr;
    QCheckBox      *m_compLinux   = nullptr;
    QComboBox      *m_compGroup   = nullptr;  // group assignment combo

    // Page 2 — File table
    QTableWidget   *m_fileTable   = nullptr;
    QPushButton    *m_btnAddFile  = nullptr;
    QPushButton    *m_btnDelFile  = nullptr;
    QPushButton    *m_btnUp       = nullptr;
    QPushButton    *m_btnDown     = nullptr;

    // ── Data ──────────────────────────────────────────────────────────────────
    QList<AppGroup>  m_groups;
    QList<Component> m_components;
    SelType          m_selType    = SelType::None;
    QString          m_selGroupId;
    QString          m_selCompId;
};
