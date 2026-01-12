/*
 * FilesEditor.cpp — Component and file list editor
 */

#include "FilesEditor.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

// ── Constructor ───────────────────────────────────────────────────────────────
FilesEditor::FilesEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void FilesEditor::load(const Manifest &m)
{
    m_components  = m.components;
    m_currentComp = -1;

    m_compTree->blockSignals(true);
    m_compTree->clear();
    for (const Component &c : m_components) {
        auto *item = new QTreeWidgetItem(m_compTree);
        item->setText(0, c.name.isEmpty() ? c.id : c.name);
        item->setData(0, Qt::UserRole, c.id);
    }
    m_compTree->blockSignals(false);

    // Clear editor panels
    m_compId       ->clear();
    m_compName     ->clear();
    m_compDesc     ->clear();
    m_compRequired ->setChecked(false);
    m_compSelected ->setChecked(true);
    m_fileTable    ->setRowCount(0);
    m_compProps    ->setEnabled(false);
    m_fileTable    ->setEnabled(false);
}

void FilesEditor::save(Manifest &m) const
{
    // Flush any pending edits for currently selected component
    const_cast<FilesEditor*>(this)->flushComponentEdits();
    m.components = m_components;
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void FilesEditor::onComponentSelected()
{
    flushComponentEdits();

    auto *sel = m_compTree->currentItem();
    if (!sel) { m_currentComp = -1; return; }

    QString id = sel->data(0, Qt::UserRole).toString();
    for (int i = 0; i < m_components.size(); ++i) {
        if (m_components[i].id == id) {
            m_currentComp = i;
            const Component &c = m_components[i];
            m_compId      ->setText(c.id);
            m_compName    ->setText(c.name);
            m_compDesc    ->setText(c.description);
            m_compRequired->setChecked(c.required);
            m_compSelected->setChecked(c.selected);
            m_compProps   ->setEnabled(true);
            m_fileTable   ->setEnabled(true);
            populateFileTable(i);
            break;
        }
    }
}

void FilesEditor::onAddComponent()
{
    Component c;
    c.id       = QString("component_%1").arg(m_components.size() + 1);
    c.name     = QString("Component %1").arg(m_components.size() + 1);
    c.required = false;
    c.selected = true;
    m_components.append(c);

    auto *item = new QTreeWidgetItem(m_compTree);
    item->setText(0, c.name);
    item->setData(0, Qt::UserRole, c.id);
    m_compTree->setCurrentItem(item);
    onComponentSelected();
}

void FilesEditor::onRemoveComponent()
{
    if (m_currentComp < 0) return;
    auto ans = QMessageBox::question(this, "Remove Component",
        QString("Remove component '%1'?").arg(m_components[m_currentComp].name));
    if (ans != QMessageBox::Yes) return;

    m_components.removeAt(m_currentComp);
    delete m_compTree->currentItem();
    m_currentComp = -1;
    m_compProps ->setEnabled(false);
    m_fileTable ->setEnabled(false);
    m_fileTable ->setRowCount(0);
}

void FilesEditor::onAddFile()
{
    if (m_currentComp < 0) return;

    // Open file picker — allow multiple files
    QStringList paths = QFileDialog::getOpenFileNames(
        this, "Add Files", QDir::homePath(), "All files (*)");

    for (const QString &p : paths) {
        FileEntry fe;
        fe.src = p;
        fe.dst = "{install-dir}/" + QFileInfo(p).fileName();
        m_components[m_currentComp].files.append(fe);

        int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);
        m_fileTable->setItem(row, 0, new QTableWidgetItem(fe.src));
        m_fileTable->setItem(row, 1, new QTableWidgetItem(fe.dst));
        m_fileTable->setItem(row, 2, new QTableWidgetItem(fe.chmod));
    }
}

void FilesEditor::onRemoveFile()
{
    if (m_currentComp < 0) return;
    int row = m_fileTable->currentRow();
    if (row < 0) return;
    m_fileTable->removeRow(row);
    m_components[m_currentComp].files.removeAt(row);
}

void FilesEditor::onMoveUp()
{
    if (m_currentComp < 0) return;
    int row = m_fileTable->currentRow();
    if (row < 1) return;
    m_components[m_currentComp].files.swapItemsAt(row - 1, row);
    populateFileTable(m_currentComp);
    m_fileTable->selectRow(row - 1);
}

void FilesEditor::onMoveDown()
{
    if (m_currentComp < 0) return;
    int row = m_fileTable->currentRow();
    if (row < 0 || row >= m_fileTable->rowCount() - 1) return;
    m_components[m_currentComp].files.swapItemsAt(row, row + 1);
    populateFileTable(m_currentComp);
    m_fileTable->selectRow(row + 1);
}

// ── Private ───────────────────────────────────────────────────────────────────
void FilesEditor::buildUi()
{
    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    outer->addWidget(m_splitter);

    // ── Left: component list ───────────────────────────────────────────────
    auto *leftWidget = new QWidget(m_splitter);
    auto *leftVbox   = new QVBoxLayout(leftWidget);
    leftVbox->setContentsMargins(8, 8, 4, 8);

    auto *compLbl = new QLabel("COMPONENTS", leftWidget);
    compLbl->setObjectName("sectionLabel");
    leftVbox->addWidget(compLbl);

    m_compTree = new QTreeWidget(leftWidget);
    m_compTree->setHeaderHidden(true);
    m_compTree->setRootIsDecorated(false);
    leftVbox->addWidget(m_compTree);

    auto *compBtns = new QHBoxLayout;
    m_btnAddComp = new QPushButton("+", leftWidget);
    m_btnDelComp = new QPushButton("-", leftWidget);
    m_btnAddComp->setFixedWidth(32);
    m_btnDelComp->setFixedWidth(32);
    compBtns->addWidget(m_btnAddComp);
    compBtns->addWidget(m_btnDelComp);
    compBtns->addStretch();
    leftVbox->addLayout(compBtns);

    m_splitter->addWidget(leftWidget);

    // ── Right: component props + file table ───────────────────────────────
    auto *rightWidget = new QWidget(m_splitter);
    auto *rightVbox   = new QVBoxLayout(rightWidget);
    rightVbox->setContentsMargins(4, 8, 8, 8);

    // Component properties
    m_compProps = new QGroupBox("Component Properties", rightWidget);
    m_compProps->setEnabled(false);
    auto *form = new QFormLayout(m_compProps);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_compId       = new QLineEdit(m_compProps);
    m_compName     = new QLineEdit(m_compProps);
    m_compDesc     = new QLineEdit(m_compProps);
    m_compRequired = new QCheckBox("Required (user cannot deselect)", m_compProps);
    m_compSelected = new QCheckBox("Selected by default",             m_compProps);

    form->addRow("ID:",          m_compId);
    form->addRow("Name:",        m_compName);
    form->addRow("Description:", m_compDesc);
    form->addRow("",             m_compRequired);
    form->addRow("",             m_compSelected);

    rightVbox->addWidget(m_compProps);

    // File list
    auto *fileLbl = new QLabel("FILES", rightWidget);
    fileLbl->setObjectName("sectionLabel");
    rightVbox->addWidget(fileLbl);

    m_fileTable = new QTableWidget(0, 3, rightWidget);
    m_fileTable->setHorizontalHeaderLabels({"Source Path", "Destination", "chmod"});
    m_fileTable->horizontalHeader()->setStretchLastSection(false);
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_fileTable->setColumnWidth(2, 60);
    m_fileTable->verticalHeader()->setVisible(false);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_fileTable->setEnabled(false);
    rightVbox->addWidget(m_fileTable, 1);

    // File buttons
    auto *fileBtns = new QHBoxLayout;
    m_btnAddFile = new QPushButton("Add Files...", rightWidget);
    m_btnDelFile = new QPushButton("Remove",       rightWidget);
    m_btnUp      = new QPushButton("↑",            rightWidget);
    m_btnDown    = new QPushButton("↓",            rightWidget);
    m_btnUp->setFixedWidth(32);
    m_btnDown->setFixedWidth(32);
    fileBtns->addWidget(m_btnAddFile);
    fileBtns->addWidget(m_btnDelFile);
    fileBtns->addStretch();
    fileBtns->addWidget(m_btnUp);
    fileBtns->addWidget(m_btnDown);
    rightVbox->addLayout(fileBtns);

    m_splitter->addWidget(rightWidget);
    m_splitter->setSizes({220, 780});

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_compTree,   &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem *, QTreeWidgetItem *) { onComponentSelected(); });
    connect(m_btnAddComp, &QPushButton::clicked, this, &FilesEditor::onAddComponent);
    connect(m_btnDelComp, &QPushButton::clicked, this, &FilesEditor::onRemoveComponent);
    connect(m_btnAddFile, &QPushButton::clicked, this, &FilesEditor::onAddFile);
    connect(m_btnDelFile, &QPushButton::clicked, this, &FilesEditor::onRemoveFile);
    connect(m_btnUp,      &QPushButton::clicked, this, &FilesEditor::onMoveUp);
    connect(m_btnDown,    &QPushButton::clicked, this, &FilesEditor::onMoveDown);
}

void FilesEditor::populateFileTable(int compIndex)
{
    m_fileTable->blockSignals(true);
    m_fileTable->setRowCount(0);
    if (compIndex < 0 || compIndex >= m_components.size()) {
        m_fileTable->blockSignals(false);
        return;
    }
    for (const FileEntry &fe : m_components[compIndex].files) {
        int row = m_fileTable->rowCount();
        m_fileTable->insertRow(row);
        m_fileTable->setItem(row, 0, new QTableWidgetItem(fe.src));
        m_fileTable->setItem(row, 1, new QTableWidgetItem(fe.dst));
        m_fileTable->setItem(row, 2, new QTableWidgetItem(fe.chmod));
    }
    m_fileTable->blockSignals(false);
}

void FilesEditor::flushComponentEdits()
{
    if (m_currentComp < 0 || m_currentComp >= m_components.size()) return;
    Component &c = m_components[m_currentComp];
    c.id          = m_compId      ->text().trimmed();
    c.name        = m_compName    ->text().trimmed();
    c.description = m_compDesc    ->text().trimmed();
    c.required    = m_compRequired->isChecked();
    c.selected    = m_compSelected->isChecked();

    // Sync file table back into component
    c.files.clear();
    for (int r = 0; r < m_fileTable->rowCount(); ++r) {
        FileEntry fe;
        fe.src   = m_fileTable->item(r, 0) ? m_fileTable->item(r, 0)->text() : QString();
        fe.dst   = m_fileTable->item(r, 1) ? m_fileTable->item(r, 1)->text() : QString();
        fe.chmod = m_fileTable->item(r, 2) ? m_fileTable->item(r, 2)->text() : QString();
        if (!fe.src.isEmpty())
            c.files.append(fe);
    }

    // Refresh tree label
    if (auto *item = m_compTree->currentItem())
        item->setText(0, c.name.isEmpty() ? c.id : c.name);
}
