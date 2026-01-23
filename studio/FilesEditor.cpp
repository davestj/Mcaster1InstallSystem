/*
 * FilesEditor.cpp — Multi-app group + component tree with per-file OS targeting.
 */

#include "FilesEditor.h"

#include <algorithm>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include "SvgIcons.h"

// ── File table column indices ─────────────────────────────────────────────────
static constexpr int COL_SRC     = 0;
static constexpr int COL_DST     = 1;
static constexpr int COL_DIR     = 2;
static constexpr int COL_CHMOD   = 3;
static constexpr int COL_MACOS   = 4;
static constexpr int COL_WINDOWS = 5;
static constexpr int COL_LINUX   = 6;
static constexpr int NUM_COLS    = 7;

// ── Item type sentinel in Qt::UserRole + 1 ───────────────────────────────────
// Qt::UserRole holds the id string (group id or comp id)
// Qt::UserRole+1 holds the type tag (0=group, 1=component)
static constexpr int TAG_GROUP = 0;
static constexpr int TAG_COMP  = 1;

// ── Helpers ──────────────────────────────────────────────────────────────────

// Build a small icon from an inline SVG
static QIcon si(const char *svg, int sz = 16)
{
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}

// Create a QTableWidgetItem that is purely a checkbox (no text)
static QTableWidgetItem *checkCell(bool checked)
{
    auto *item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

// ── Constructor ───────────────────────────────────────────────────────────────
FilesEditor::FilesEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void FilesEditor::load(const Manifest &m)
{
    m_groups     = m.appGroups;
    m_components = m.components;
    m_selType    = SelType::None;
    m_selGroupId.clear();
    m_selCompId.clear();
    rebuildTree();
    showEmptyPage();
}

void FilesEditor::save(Manifest &m) const
{
    // Flush any unsaved edits for the currently active page
    auto *self = const_cast<FilesEditor *>(this);
    if (m_selType == SelType::Group)     self->flushGroupEdits();
    if (m_selType == SelType::Component) self->flushComponentEdits();

    m.appGroups  = m_groups;
    m.components = m_components;
}

// ── Build UI ──────────────────────────────────────────────────────────────────
void FilesEditor::buildUi()
{
    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    outer->addWidget(m_splitter);

    // ── Left: tree + toolbar ──────────────────────────────────────────────────
    auto *leftW = new QWidget(m_splitter);
    auto *leftV = new QVBoxLayout(leftW);
    leftV->setContentsMargins(8, 8, 4, 8);
    leftV->setSpacing(4);

    auto *treeLbl = new QLabel("APP GROUPS & COMPONENTS", leftW);
    treeLbl->setObjectName("sectionLabel");
    leftV->addWidget(treeLbl);

    m_tree = new QTreeWidget(leftW);
    m_tree->setHeaderHidden(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setAnimated(true);
    m_tree->setIndentation(16);
    leftV->addWidget(m_tree, 1);

    // Toolbar under tree
    auto *treeBtns = new QHBoxLayout;
    treeBtns->setSpacing(2);
    m_btnAddGroup = new QPushButton(leftW);
    m_btnAddComp  = new QPushButton(leftW);
    m_btnRemove   = new QPushButton(leftW);
    m_btnAddGroup->setIcon(si(SvgIcons::kFolder));
    m_btnAddComp ->setIcon(si(SvgIcons::kNew));
    m_btnRemove  ->setIcon(si(SvgIcons::kTrash));
    m_btnAddGroup->setToolTip("Add App Group");
    m_btnAddComp ->setToolTip("Add Component");
    m_btnRemove  ->setToolTip("Remove Selected");
    m_btnAddGroup->setFixedWidth(32);
    m_btnAddComp ->setFixedWidth(32);
    m_btnRemove  ->setFixedWidth(32);
    treeBtns->addWidget(m_btnAddGroup);
    treeBtns->addWidget(m_btnAddComp);
    treeBtns->addWidget(m_btnRemove);
    treeBtns->addStretch();
    leftV->addLayout(treeBtns);

    m_splitter->addWidget(leftW);

    // ── Right: stacked widget ─────────────────────────────────────────────────
    m_stack = new QStackedWidget(m_splitter);

    // ── Page 0: empty ─────────────────────────────────────────────────────────
    auto *emptyPage = new QWidget();
    auto *emptyV    = new QVBoxLayout(emptyPage);
    emptyV->setAlignment(Qt::AlignCenter);
    auto *hint = new QLabel("Select a group or component to edit.", emptyPage);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("color: #666; font-style: italic;");
    emptyV->addWidget(hint);
    m_stack->addWidget(emptyPage);   // index 0

    // ── Page 1: group editor ──────────────────────────────────────────────────
    auto *groupPage = new QWidget();
    auto *groupV    = new QVBoxLayout(groupPage);
    groupV->setContentsMargins(8, 8, 8, 8);
    groupV->setSpacing(8);

    auto *grpLbl = new QLabel("APP GROUP", groupPage);
    grpLbl->setObjectName("sectionLabel");
    groupV->addWidget(grpLbl);

    auto *grpBox  = new QGroupBox("Group Properties", groupPage);
    auto *grpForm = new QFormLayout(grpBox);
    grpForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    grpForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_grpId   = new QLineEdit(grpBox);
    m_grpName = new QLineEdit(grpBox);
    m_grpDesc = new QLineEdit(grpBox);
    grpForm->addRow("ID:",          m_grpId);
    grpForm->addRow("Name:",        m_grpName);
    grpForm->addRow("Description:", m_grpDesc);
    groupV->addWidget(grpBox);
    groupV->addStretch();
    m_stack->addWidget(groupPage);   // index 1

    // ── Page 2: component editor ──────────────────────────────────────────────
    auto *compPage = new QWidget();
    auto *compV    = new QVBoxLayout(compPage);
    compV->setContentsMargins(8, 8, 8, 8);
    compV->setSpacing(6);

    auto *compLbl = new QLabel("COMPONENT", compPage);
    compLbl->setObjectName("sectionLabel");
    compV->addWidget(compLbl);

    auto *compBox  = new QGroupBox("Component Properties", compPage);
    auto *compForm = new QFormLayout(compBox);
    compForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    compForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_compId      = new QLineEdit(compBox);
    m_compName    = new QLineEdit(compBox);
    m_compDesc    = new QLineEdit(compBox);
    m_compGroup   = new QComboBox(compBox);
    m_compReq     = new QCheckBox("Required (user cannot deselect)", compBox);
    m_compSel     = new QCheckBox("Selected by default", compBox);

    // Component-level platform support row
    auto *platRow  = new QWidget(compBox);
    auto *platHbox = new QHBoxLayout(platRow);
    platHbox->setContentsMargins(0, 0, 0, 0);
    platHbox->setSpacing(16);
    m_compMacos   = new QCheckBox("macOS",   platRow);
    m_compWindows = new QCheckBox("Windows", platRow);
    m_compLinux   = new QCheckBox("Linux",   platRow);
    m_compMacos  ->setChecked(true);
    m_compWindows->setChecked(true);
    m_compLinux  ->setChecked(true);
    platHbox->addWidget(m_compMacos);
    platHbox->addWidget(m_compWindows);
    platHbox->addWidget(m_compLinux);
    platHbox->addStretch();

    compForm->addRow("ID:",          m_compId);
    compForm->addRow("Name:",        m_compName);
    compForm->addRow("Description:", m_compDesc);
    compForm->addRow("Group:",       m_compGroup);
    compForm->addRow("",             m_compReq);
    compForm->addRow("",             m_compSel);
    compForm->addRow("Platforms:",   platRow);
    compV->addWidget(compBox);

    // File list section
    auto *fileLbl = new QLabel("FILES", compPage);
    fileLbl->setObjectName("sectionLabel");
    compV->addWidget(fileLbl);

    m_fileTable = new QTableWidget(0, NUM_COLS, compPage);
    m_fileTable->setHorizontalHeaderLabels(
        {"Source Path", "Destination", "Dir", "chmod", "macOS", "Win", "Linux"});
    auto *hdr = m_fileTable->horizontalHeader();
    hdr->setSectionResizeMode(COL_SRC,     QHeaderView::Stretch);
    hdr->setSectionResizeMode(COL_DST,     QHeaderView::Stretch);
    hdr->setSectionResizeMode(COL_DIR,     QHeaderView::Fixed);
    hdr->setSectionResizeMode(COL_CHMOD,   QHeaderView::Fixed);
    hdr->setSectionResizeMode(COL_MACOS,   QHeaderView::Fixed);
    hdr->setSectionResizeMode(COL_WINDOWS, QHeaderView::Fixed);
    hdr->setSectionResizeMode(COL_LINUX,   QHeaderView::Fixed);
    m_fileTable->setColumnWidth(COL_DIR,     40);
    m_fileTable->setColumnWidth(COL_CHMOD,   60);
    m_fileTable->setColumnWidth(COL_MACOS,   55);
    m_fileTable->setColumnWidth(COL_WINDOWS, 45);
    m_fileTable->setColumnWidth(COL_LINUX,   52);
    m_fileTable->verticalHeader()->setVisible(false);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                | QAbstractItemView::EditKeyPressed);
    compV->addWidget(m_fileTable, 1);

    // File buttons
    auto *fileBtns = new QHBoxLayout;
    fileBtns->setSpacing(4);
    m_btnAddFile = new QPushButton(si(SvgIcons::kFolder), "Add Files...", compPage);
    m_btnDelFile = new QPushButton(si(SvgIcons::kTrash),  "Remove",       compPage);
    m_btnUp      = new QPushButton(compPage);
    m_btnDown    = new QPushButton(compPage);
    m_btnUp  ->setIcon(si(SvgIcons::kArrowUp));
    m_btnDown->setIcon(si(SvgIcons::kArrowDown));
    m_btnUp  ->setToolTip("Move Up");
    m_btnDown->setToolTip("Move Down");
    m_btnUp  ->setFixedWidth(32);
    m_btnDown->setFixedWidth(32);
    fileBtns->addWidget(m_btnAddFile);
    fileBtns->addWidget(m_btnDelFile);
    fileBtns->addStretch();
    fileBtns->addWidget(m_btnUp);
    fileBtns->addWidget(m_btnDown);
    compV->addLayout(fileBtns);

    m_stack->addWidget(compPage);   // index 2

    m_splitter->addWidget(m_stack);
    m_splitter->setSizes({220, 780});

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_tree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *, QTreeWidgetItem *) { onTreeSelectionChanged(); });

    connect(m_btnAddGroup, &QPushButton::clicked, this, &FilesEditor::onAddGroup);
    connect(m_btnAddComp,  &QPushButton::clicked, this, &FilesEditor::onAddComponent);
    connect(m_btnRemove,   &QPushButton::clicked, this, &FilesEditor::onRemoveSelected);

    connect(m_btnAddFile,  &QPushButton::clicked, this, &FilesEditor::onAddFile);
    connect(m_btnDelFile,  &QPushButton::clicked, this, &FilesEditor::onRemoveFile);
    connect(m_btnUp,       &QPushButton::clicked, this, &FilesEditor::onMoveUp);
    connect(m_btnDown,     &QPushButton::clicked, this, &FilesEditor::onMoveDown);

    // When the user reassigns a component's group via the combo, rebuild the tree
    connect(m_compGroup, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (m_selType != SelType::Component) return;
                flushComponentEdits();
                rebuildTree();
                // Re-select the component (ID may have been updated by flush)
                if (auto *item = compItem(m_selCompId))
                    m_tree->setCurrentItem(item);
            });
}

// ── rebuildTree ───────────────────────────────────────────────────────────────
void FilesEditor::rebuildTree()
{
    m_tree->blockSignals(true);
    m_tree->clear();

    // ── Create named group items ───────────────────────────────────────────
    for (const AppGroup &g : m_groups) {
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(0, g.name.isEmpty() ? g.id : g.name);
        item->setData(0, Qt::UserRole,     g.id);
        item->setData(0, Qt::UserRole + 1, TAG_GROUP);
        item->setIcon(0, si(SvgIcons::kFolder));
        item->setExpanded(true);
    }

    // ── Determine if any ungrouped components exist ────────────────────────
    bool hasUngrouped = std::any_of(m_components.cbegin(), m_components.cend(),
        [&](const Component &c) {
            if (c.appGroup.isEmpty()) return true;
            return !std::any_of(m_groups.cbegin(), m_groups.cend(),
                [&](const AppGroup &g) { return g.id == c.appGroup; });
        });

    QTreeWidgetItem *noGroupItem = nullptr;
    if (hasUngrouped) {
        noGroupItem = new QTreeWidgetItem(m_tree);
        noGroupItem->setText(0, "(No Group)");
        noGroupItem->setData(0, Qt::UserRole,     QString());
        noGroupItem->setData(0, Qt::UserRole + 1, TAG_GROUP);
        noGroupItem->setIcon(0, si(SvgIcons::kFolder));
        noGroupItem->setExpanded(true);
    }

    // ── Add components under their group ──────────────────────────────────
    for (const Component &c : m_components) {
        QTreeWidgetItem *parent = groupItem(c.appGroup);
        if (!parent) parent = noGroupItem;
        if (!parent) continue;

        auto *item = new QTreeWidgetItem(parent);
        item->setText(0, c.name.isEmpty() ? c.id : c.name);
        item->setData(0, Qt::UserRole,     c.id);
        item->setData(0, Qt::UserRole + 1, TAG_COMP);
        item->setIcon(0, si(SvgIcons::kPackage));
    }

    m_tree->blockSignals(false);

    // Restore previous selection
    if (m_selType == SelType::Group && !m_selGroupId.isEmpty()) {
        if (auto *item = groupItem(m_selGroupId))
            m_tree->setCurrentItem(item);
    } else if (m_selType == SelType::Component && !m_selCompId.isEmpty()) {
        if (auto *item = compItem(m_selCompId))
            m_tree->setCurrentItem(item);
    }
}

// ── Tree item lookup ──────────────────────────────────────────────────────────
QTreeWidgetItem *FilesEditor::groupItem(const QString &groupId) const
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *item = m_tree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == groupId)
            return item;
    }
    return nullptr;
}

QTreeWidgetItem *FilesEditor::compItem(const QString &compId) const
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *grp = m_tree->topLevelItem(i);
        for (int j = 0; j < grp->childCount(); ++j) {
            auto *child = grp->child(j);
            if (child->data(0, Qt::UserRole).toString() == compId)
                return child;
        }
    }
    return nullptr;
}

// ── Page switching ────────────────────────────────────────────────────────────
void FilesEditor::showEmptyPage()
{
    m_stack->setCurrentIndex(0);
}

void FilesEditor::showGroupEditor(const QString &groupId)
{
    auto it = std::find_if(m_groups.cbegin(), m_groups.cend(),
        [&](const AppGroup &g) { return g.id == groupId; });
    if (it == m_groups.cend()) { showEmptyPage(); return; }

    m_grpId  ->setText(it->id);
    m_grpName->setText(it->name);
    m_grpDesc->setText(it->description);
    m_stack->setCurrentIndex(1);
}

void FilesEditor::showComponentEditor(const QString &compId)
{
    auto it = std::find_if(m_components.cbegin(), m_components.cend(),
        [&](const Component &c) { return c.id == compId; });
    if (it == m_components.cend()) { showEmptyPage(); return; }

    m_compId  ->setText(it->id);
    m_compName->setText(it->name);
    m_compDesc->setText(it->description);
    m_compReq ->setChecked(it->required);
    m_compSel ->setChecked(it->selected);

    // Component-level platform checkboxes
    if (it->platforms.isEmpty()) {
        m_compMacos  ->setChecked(true);
        m_compWindows->setChecked(true);
        m_compLinux  ->setChecked(true);
    } else {
        m_compMacos  ->setChecked(it->platforms.contains("macos"));
        m_compWindows->setChecked(it->platforms.contains("windows"));
        m_compLinux  ->setChecked(it->platforms.contains("linux"));
    }

    // Populate group combo, then set current group
    refreshGroupCombo();
    int gIdx = m_compGroup->findData(it->appGroup);
    m_compGroup->blockSignals(true);
    m_compGroup->setCurrentIndex(gIdx >= 0 ? gIdx : 0);
    m_compGroup->blockSignals(false);

    populateFileTable(compId);
    m_stack->setCurrentIndex(2);
}

// ── flushGroupEdits ───────────────────────────────────────────────────────────
void FilesEditor::flushGroupEdits()
{
    if (m_selType != SelType::Group || m_selGroupId.isEmpty()) return;

    auto it = std::find_if(m_groups.begin(), m_groups.end(),
        [&](const AppGroup &g) { return g.id == m_selGroupId; });
    if (it == m_groups.end()) return;

    const QString oldId = it->id;
    it->id          = m_grpId  ->text().trimmed();
    it->name        = m_grpName->text().trimmed();
    it->description = m_grpDesc->text().trimmed();
    m_selGroupId    = it->id;

    // Propagate ID rename to all components that reference this group
    if (oldId != it->id) {
        for (auto &c : m_components)
            if (c.appGroup == oldId)
                c.appGroup = it->id;
    }

    // Update tree item label/id
    if (auto *item = groupItem(oldId)) {
        item->setText(0, it->name.isEmpty() ? it->id : it->name);
        item->setData(0, Qt::UserRole, it->id);
    }
}

// ── flushComponentEdits ───────────────────────────────────────────────────────
void FilesEditor::flushComponentEdits()
{
    if (m_selType != SelType::Component || m_selCompId.isEmpty()) return;

    auto it = std::find_if(m_components.begin(), m_components.end(),
        [&](const Component &c) { return c.id == m_selCompId; });
    if (it == m_components.end()) return;

    const QString oldId = it->id;
    it->id          = m_compId  ->text().trimmed();
    it->name        = m_compName->text().trimmed();
    it->description = m_compDesc->text().trimmed();
    it->required    = m_compReq ->isChecked();
    it->selected    = m_compSel ->isChecked();
    it->appGroup    = m_compGroup->currentData().toString();

    // Component-level platform list
    bool mac = m_compMacos  ->isChecked();
    bool win = m_compWindows->isChecked();
    bool lnx = m_compLinux  ->isChecked();
    if (mac && win && lnx) {
        it->platforms.clear();
    } else {
        it->platforms.clear();
        if (mac) it->platforms << "macos";
        if (win) it->platforms << "windows";
        if (lnx) it->platforms << "linux";
    }

    // Sync file rows back into the component
    it->files.clear();
    for (int r = 0; r < m_fileTable->rowCount(); ++r) {
        FileEntry fe;
        auto *srcItem = m_fileTable->item(r, COL_SRC);
        auto *dstItem = m_fileTable->item(r, COL_DST);
        fe.src   = srcItem ? srcItem->text() : QString();
        fe.dst   = dstItem ? dstItem->text() : QString();
        if (fe.src.isEmpty()) continue;

        auto *dirItem = m_fileTable->item(r, COL_DIR);
        auto *chItem  = m_fileTable->item(r, COL_CHMOD);
        auto *macItem = m_fileTable->item(r, COL_MACOS);
        auto *winItem = m_fileTable->item(r, COL_WINDOWS);
        auto *lnxItem = m_fileTable->item(r, COL_LINUX);

        fe.isDir = dirItem && dirItem->checkState() == Qt::Checked;
        fe.chmod = chItem  ? chItem->text() : QString();

        bool fMac = macItem && macItem->checkState() == Qt::Checked;
        bool fWin = winItem && winItem->checkState() == Qt::Checked;
        bool fLnx = lnxItem && lnxItem->checkState() == Qt::Checked;
        if (fMac && fWin && fLnx) {
            fe.platforms.clear();   // all platforms
        } else {
            if (fMac) fe.platforms << "macos";
            if (fWin) fe.platforms << "windows";
            if (fLnx) fe.platforms << "linux";
        }

        it->files.append(fe);
    }

    // Track possible ID rename
    m_selCompId = it->id;

    // Refresh tree item label
    if (auto *item = compItem(oldId)) {
        item->setText(0, it->name.isEmpty() ? it->id : it->name);
        item->setData(0, Qt::UserRole, it->id);
    }
}

// ── populateFileTable ─────────────────────────────────────────────────────────
void FilesEditor::populateFileTable(const QString &compId)
{
    m_fileTable->blockSignals(true);
    m_fileTable->setRowCount(0);

    auto it = std::find_if(m_components.cbegin(), m_components.cend(),
        [&](const Component &c) { return c.id == compId; });
    if (it == m_components.cend()) { m_fileTable->blockSignals(false); return; }

    for (const FileEntry &fe : it->files) {
        const int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);

        m_fileTable->setItem(row, COL_SRC,  new QTableWidgetItem(fe.src));
        m_fileTable->setItem(row, COL_DST,  new QTableWidgetItem(fe.dst));
        m_fileTable->setItem(row, COL_DIR,  checkCell(fe.isDir));
        m_fileTable->setItem(row, COL_CHMOD, new QTableWidgetItem(fe.chmod));

        // Per-file platform columns: empty = all checked
        bool allPlats = fe.platforms.isEmpty();
        m_fileTable->setItem(row, COL_MACOS,   checkCell(allPlats || fe.platforms.contains("macos")));
        m_fileTable->setItem(row, COL_WINDOWS, checkCell(allPlats || fe.platforms.contains("windows")));
        m_fileTable->setItem(row, COL_LINUX,   checkCell(allPlats || fe.platforms.contains("linux")));
    }
    m_fileTable->blockSignals(false);
}

// ── refreshGroupCombo ─────────────────────────────────────────────────────────
void FilesEditor::refreshGroupCombo()
{
    m_compGroup->blockSignals(true);
    m_compGroup->clear();
    m_compGroup->addItem("(No Group)", QString());
    for (const AppGroup &g : m_groups)
        m_compGroup->addItem(g.name.isEmpty() ? g.id : g.name, g.id);
    m_compGroup->blockSignals(false);
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void FilesEditor::onTreeSelectionChanged()
{
    // Flush whatever was being edited
    if (m_selType == SelType::Group)     flushGroupEdits();
    if (m_selType == SelType::Component) flushComponentEdits();

    auto *item = m_tree->currentItem();
    if (!item) {
        m_selType = SelType::None;
        showEmptyPage();
        return;
    }

    int tag = item->data(0, Qt::UserRole + 1).toInt();

    if (item->parent() == nullptr && tag == TAG_GROUP) {
        // Top-level: group node (including "(No Group)" with empty id)
        const QString gId = item->data(0, Qt::UserRole).toString();
        if (gId.isEmpty()) {
            // "(No Group)" virtual node — nothing to edit
            m_selType = SelType::None;
            m_selGroupId.clear();
            m_selCompId.clear();
            showEmptyPage();
        } else {
            m_selType    = SelType::Group;
            m_selGroupId = gId;
            m_selCompId.clear();
            showGroupEditor(gId);
        }
    } else {
        // Child item: component
        m_selType    = SelType::Component;
        m_selCompId  = item->data(0, Qt::UserRole).toString();
        m_selGroupId.clear();
        showComponentEditor(m_selCompId);
    }
}

void FilesEditor::onAddGroup()
{
    // Flush current edits before adding
    if (m_selType == SelType::Group)     flushGroupEdits();
    if (m_selType == SelType::Component) flushComponentEdits();

    AppGroup g;
    g.id   = QString("group_%1").arg(m_groups.size() + 1);
    g.name = QString("App Group %1").arg(m_groups.size() + 1);
    m_groups.append(g);

    m_selType    = SelType::Group;
    m_selGroupId = g.id;
    m_selCompId.clear();

    rebuildTree();
    if (auto *item = groupItem(g.id))
        m_tree->setCurrentItem(item);

    emit modified();
}

void FilesEditor::onAddComponent()
{
    // Flush current edits
    if (m_selType == SelType::Group)     flushGroupEdits();
    if (m_selType == SelType::Component) flushComponentEdits();

    Component c;
    c.id       = QString("component_%1").arg(m_components.size() + 1);
    c.name     = QString("Component %1").arg(m_components.size() + 1);
    c.selected = true;

    // Assign to currently selected group (if a real group is selected)
    if (m_selType == SelType::Group && !m_selGroupId.isEmpty())
        c.appGroup = m_selGroupId;
    else if (m_selType == SelType::Component && !m_selCompId.isEmpty()) {
        // Find the group of the currently selected component
        auto it = std::find_if(m_components.cbegin(), m_components.cend(),
            [&](const Component &cx) { return cx.id == m_selCompId; });
        if (it != m_components.cend())
            c.appGroup = it->appGroup;
    }

    m_components.append(c);

    m_selType   = SelType::Component;
    m_selCompId = c.id;
    m_selGroupId.clear();

    rebuildTree();
    if (auto *item = compItem(c.id))
        m_tree->setCurrentItem(item);

    emit modified();
}

void FilesEditor::onRemoveSelected()
{
    if (m_selType == SelType::Group && !m_selGroupId.isEmpty()) {
        // Find group name for confirmation message
        auto it = std::find_if(m_groups.cbegin(), m_groups.cend(),
            [&](const AppGroup &g) { return g.id == m_selGroupId; });
        QString gname = (it != m_groups.cend()) ? it->name : m_selGroupId;

        auto ans = QMessageBox::question(this, "Remove App Group",
            QString("Remove group '%1'?\n\n"
                    "Components in this group will become ungrouped.").arg(gname),
            QMessageBox::Yes | QMessageBox::No);
        if (ans != QMessageBox::Yes) return;

        // Ungrouped affected components
        for (auto &c : m_components)
            if (c.appGroup == m_selGroupId)
                c.appGroup.clear();

        m_groups.erase(std::remove_if(m_groups.begin(), m_groups.end(),
            [&](const AppGroup &g) { return g.id == m_selGroupId; }), m_groups.end());

        m_selType = SelType::None;
        m_selGroupId.clear();
        rebuildTree();
        showEmptyPage();

    } else if (m_selType == SelType::Component && !m_selCompId.isEmpty()) {
        auto it = std::find_if(m_components.cbegin(), m_components.cend(),
            [&](const Component &c) { return c.id == m_selCompId; });
        QString cname = (it != m_components.cend()) ? it->name : m_selCompId;

        auto ans = QMessageBox::question(this, "Remove Component",
            QString("Remove component '%1'?").arg(cname),
            QMessageBox::Yes | QMessageBox::No);
        if (ans != QMessageBox::Yes) return;

        m_components.erase(std::remove_if(m_components.begin(), m_components.end(),
            [&](const Component &c) { return c.id == m_selCompId; }), m_components.end());

        m_selType = SelType::None;
        m_selCompId.clear();
        rebuildTree();
        showEmptyPage();
    }
    emit modified();
}

void FilesEditor::onAddFile()
{
    if (m_selType != SelType::Component || m_selCompId.isEmpty()) return;

    QStringList paths = QFileDialog::getOpenFileNames(
        this, "Add Files", QDir::homePath(), "All files (*)");

    for (const QString &p : paths) {
        const int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);
        m_fileTable->setItem(row, COL_SRC,  new QTableWidgetItem(p));
        m_fileTable->setItem(row, COL_DST,  new QTableWidgetItem(
            "{install-dir}/" + QFileInfo(p).fileName()));
        m_fileTable->setItem(row, COL_DIR,     checkCell(false));
        m_fileTable->setItem(row, COL_CHMOD,   new QTableWidgetItem(QString()));
        m_fileTable->setItem(row, COL_MACOS,   checkCell(true));
        m_fileTable->setItem(row, COL_WINDOWS, checkCell(true));
        m_fileTable->setItem(row, COL_LINUX,   checkCell(true));
    }
    emit modified();
}

void FilesEditor::onRemoveFile()
{
    if (m_selType != SelType::Component) return;
    int row = m_fileTable->currentRow();
    if (row < 0) return;
    m_fileTable->removeRow(row);
    emit modified();
}

void FilesEditor::onMoveUp()
{
    if (m_selType != SelType::Component) return;
    int row = m_fileTable->currentRow();
    if (row < 1) return;

    // Swap all cells between row and row-1
    for (int col = 0; col < NUM_COLS; ++col) {
        auto *a = m_fileTable->takeItem(row - 1, col);
        auto *b = m_fileTable->takeItem(row,     col);
        m_fileTable->setItem(row - 1, col, b);
        m_fileTable->setItem(row,     col, a);
    }
    m_fileTable->selectRow(row - 1);
    emit modified();
}

void FilesEditor::onMoveDown()
{
    if (m_selType != SelType::Component) return;
    int row = m_fileTable->currentRow();
    if (row < 0 || row >= m_fileTable->rowCount() - 1) return;

    for (int col = 0; col < NUM_COLS; ++col) {
        auto *a = m_fileTable->takeItem(row,     col);
        auto *b = m_fileTable->takeItem(row + 1, col);
        m_fileTable->setItem(row,     col, b);
        m_fileTable->setItem(row + 1, col, a);
    }
    m_fileTable->selectRow(row + 1);
    emit modified();
}
