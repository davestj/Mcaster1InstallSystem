/*
 * ComponentsEditor.cpp — Component dependency and flags editor
 */
#include "ComponentsEditor.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>

ComponentsEditor::ComponentsEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void ComponentsEditor::load(const Manifest &m)
{
    m_currentRow  = -1;
    m_components  = m.components;

    m_compList->blockSignals(true);
    m_compList->clear();
    for (const Component &c : m_components) {
        auto *item = new QListWidgetItem(c.name.isEmpty() ? c.id : c.name, m_compList);
        item->setToolTip(c.description);
    }
    m_compList->blockSignals(false);

    // Clear detail panel
    m_detailGroup->setEnabled(false);
    m_nameLabel  ->clear();
    m_descLabel  ->clear();
    m_chkRequired->setChecked(false);
    m_chkSelected->setChecked(true);
    m_platforms  ->clear();

    // Clear any existing dependency checkboxes
    QLayoutItem *item;
    while ((item = m_depsGroup->layout()->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void ComponentsEditor::save(Manifest &m) const
{
    // Flush the currently open component before collecting
    const_cast<ComponentsEditor *>(this)->flushCurrentComponent();

    // Write back only flags and relations — file lists are owned by FilesEditor
    for (int i = 0; i < m_components.size() && i < m.components.size(); ++i) {
        m.components[i].required  = m_components[i].required;
        m.components[i].selected  = m_components[i].selected;
        m.components[i].platforms = m_components[i].platforms;
        m.components[i].depends   = m_components[i].depends;
    }
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void ComponentsEditor::onComponentSelected(int row)
{
    // Flush edits for previously selected component
    flushCurrentComponent();

    m_currentRow = row;
    if (row < 0 || row >= m_components.size()) {
        m_detailGroup->setEnabled(false);
        return;
    }

    const Component &c = m_components[row];
    m_detailGroup->setEnabled(true);
    m_nameLabel  ->setText("<b>" + (c.name.isEmpty() ? c.id : c.name) + "</b>");
    m_descLabel  ->setText(c.description);
    m_chkRequired->setChecked(c.required);
    m_chkSelected->setChecked(c.selected);
    m_platforms  ->setText(c.platforms.join(", "));

    populateDependsPanel(row);
}

// ── Private ───────────────────────────────────────────────────────────────────
void ComponentsEditor::flushCurrentComponent()
{
    if (m_currentRow < 0 || m_currentRow >= m_components.size()) return;

    Component &c  = m_components[m_currentRow];
    c.required    = m_chkRequired->isChecked();
    c.selected    = m_chkSelected->isChecked();

    // Parse platforms from comma-separated text
    QString pText = m_platforms->text().trimmed();
    c.platforms   = pText.isEmpty()
                    ? QStringList()
                    : pText.split(',', Qt::SkipEmptyParts);
    for (QString &p : c.platforms) p = p.trimmed();

    // Collect depends from checkbox states in deps group
    c.depends.clear();
    QLayout *lay = m_depsGroup->layout();
    for (int i = 0; i < lay->count(); ++i) {
        auto *chk = qobject_cast<QCheckBox *>(lay->itemAt(i)->widget());
        if (chk && chk->isChecked())
            c.depends << chk->property("compId").toString();
    }
}

void ComponentsEditor::populateDependsPanel(int compIndex)
{
    // Remove all existing checkboxes from the deps group box
    QLayout *lay = m_depsGroup->layout();
    QLayoutItem *li;
    while ((li = lay->takeAt(0)) != nullptr) {
        delete li->widget();
        delete li;
    }

    const Component &current = m_components[compIndex];

    for (int i = 0; i < m_components.size(); ++i) {
        if (i == compIndex) continue;  // can't depend on itself
        const Component &other = m_components[i];
        QString label = other.name.isEmpty() ? other.id : other.name;
        auto *chk = new QCheckBox(label, m_depsGroup);
        chk->setProperty("compId", other.id);
        chk->setChecked(current.depends.contains(other.id));
        lay->addWidget(chk);
    }

    if (m_components.size() <= 1)
        lay->addWidget(new QLabel("(No other components to depend on)", m_depsGroup));
}

void ComponentsEditor::buildUi()
{
    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    outer->addWidget(splitter);

    // ── Left: component list ──────────────────────────────────────────────
    auto *leftWidget = new QWidget(splitter);
    auto *leftVbox   = new QVBoxLayout(leftWidget);
    leftVbox->setContentsMargins(16, 16, 8, 16);
    leftVbox->setSpacing(6);

    auto *listTitle = new QLabel("Components", leftWidget);
    listTitle->setObjectName("titleLabel");
    leftVbox->addWidget(listTitle);

    m_compList = new QListWidget(leftWidget);
    m_compList->setAlternatingRowColors(true);
    m_compList->setSelectionMode(QAbstractItemView::SingleSelection);
    leftVbox->addWidget(m_compList, 1);

    splitter->addWidget(leftWidget);

    // ── Right: detail / dependency panel ─────────────────────────────────
    auto *rightScroll = new QScrollArea(splitter);
    rightScroll->setWidgetResizable(true);
    rightScroll->setFrameShape(QFrame::NoFrame);

    auto *rightWidget = new QWidget(rightScroll);
    rightScroll->setWidget(rightWidget);

    auto *rightVbox = new QVBoxLayout(rightWidget);
    rightVbox->setContentsMargins(8, 16, 16, 16);
    rightVbox->setSpacing(14);

    auto *detailTitle = new QLabel("Component Details", rightWidget);
    detailTitle->setObjectName("titleLabel");
    rightVbox->addWidget(detailTitle);

    m_detailGroup = new QGroupBox(rightWidget);
    m_detailGroup->setEnabled(false);
    auto *detailVbox = new QVBoxLayout(m_detailGroup);
    detailVbox->setSpacing(10);

    // Name + description (read-only display — editable in FilesEditor)
    m_nameLabel = new QLabel(m_detailGroup);
    m_nameLabel->setTextFormat(Qt::RichText);
    m_descLabel = new QLabel(m_detailGroup);
    m_descLabel->setObjectName("hintLabel");
    m_descLabel->setWordWrap(true);
    detailVbox->addWidget(m_nameLabel);
    detailVbox->addWidget(m_descLabel);

    // ── Flags ────────────────────────────────────────────────────────────
    {
        auto *flagsGrp = new QGroupBox("Install Flags", m_detailGroup);
        auto *fv       = new QVBoxLayout(flagsGrp);
        m_chkRequired  = new QCheckBox("Required (cannot be deselected by user)", flagsGrp);
        m_chkSelected  = new QCheckBox("Selected by default",                      flagsGrp);
        fv->addWidget(m_chkRequired);
        fv->addWidget(m_chkSelected);
        detailVbox->addWidget(flagsGrp);
    }

    // ── Platforms ────────────────────────────────────────────────────────
    {
        auto *platGrp  = new QGroupBox("Platforms", m_detailGroup);
        auto *platForm = new QFormLayout(platGrp);
        m_platforms    = new QLineEdit(platGrp);
        m_platforms->setPlaceholderText("macos, windows, linux  (empty = all)");
        platForm->addRow("Platforms:", m_platforms);

        auto *hint = new QLabel("Comma-separated. Leave empty to install on all platforms.", platGrp);
        hint->setObjectName("hintLabel");
        hint->setWordWrap(true);
        platForm->addRow("", hint);

        detailVbox->addWidget(platGrp);
    }

    // ── Dependencies ─────────────────────────────────────────────────────
    m_depsGroup = new QGroupBox("Depends On", m_detailGroup);
    m_depsGroup->setLayout(new QVBoxLayout(m_depsGroup));
    m_depsGroup->layout()->addWidget(
        new QLabel("(Select a component to see dependencies)", m_depsGroup));
    detailVbox->addWidget(m_depsGroup);

    detailVbox->addStretch();
    rightVbox->addWidget(m_detailGroup, 1);
    rightVbox->addStretch();

    splitter->addWidget(rightScroll);
    splitter->setSizes({240, 700});

    connect(m_compList, &QListWidget::currentRowChanged,
            this, &ComponentsEditor::onComponentSelected);
}
