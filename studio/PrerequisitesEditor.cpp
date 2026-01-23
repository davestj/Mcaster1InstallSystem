/*
 * PrerequisitesEditor.cpp — Studio tab for editing installer prerequisites.
 */

#include "PrerequisitesEditor.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include "SvgIcons.h"

static QIcon si(const char *svg, int sz = 16)
{
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}

PrerequisitesEditor::PrerequisitesEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void PrerequisitesEditor::load(const Manifest &m)
{
    m_currentRow = -1;
    m_prereqs    = m.prerequisites;

    m_list->blockSignals(true);
    m_list->clear();
    for (const Prerequisite &p : m_prereqs) {
        auto *item = new QListWidgetItem(p.name.isEmpty() ? p.id : p.name, m_list);
        item->setToolTip(p.checkCmd);
    }
    m_list->blockSignals(false);

    m_detail    ->setEnabled(false);
    m_id        ->clear();
    m_name      ->clear();
    m_checkCmd  ->clear();
    m_installCmd->clear();
    m_platforms ->clear();
}

void PrerequisitesEditor::save(Manifest &m) const
{
    const_cast<PrerequisitesEditor *>(this)->flushCurrentRow();
    m.prerequisites = m_prereqs;
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void PrerequisitesEditor::onRowSelected(int row)
{
    flushCurrentRow();

    m_currentRow = row;
    if (row < 0 || row >= m_prereqs.size()) {
        m_detail->setEnabled(false);
        return;
    }

    m_detail->setEnabled(true);
    populateRow(row);
}

void PrerequisitesEditor::onAdd()
{
    Prerequisite p;
    p.id   = QString("prereq_%1").arg(m_prereqs.size() + 1);
    p.name = "New Prerequisite";
    m_prereqs.append(p);

    m_list->blockSignals(true);
    auto *item = new QListWidgetItem(p.name, m_list);
    item->setToolTip(QString());
    m_list->blockSignals(false);

    m_list->setCurrentRow(m_prereqs.size() - 1);
}

void PrerequisitesEditor::onRemove()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_prereqs.size()) return;

    m_currentRow = -1;                    // prevent flush from writing to wrong index
    m_prereqs.removeAt(row);
    delete m_list->takeItem(row);

    if (!m_prereqs.isEmpty()) {
        m_list->setCurrentRow(qMin(row, m_prereqs.size() - 1));
    } else {
        m_detail->setEnabled(false);
        m_id->clear(); m_name->clear();
        m_checkCmd->clear(); m_installCmd->clear();
        m_platforms->clear();
    }
}

// ── Private ───────────────────────────────────────────────────────────────────
void PrerequisitesEditor::flushCurrentRow()
{
    if (m_currentRow < 0 || m_currentRow >= m_prereqs.size()) return;

    Prerequisite &p = m_prereqs[m_currentRow];
    p.id         = m_id        ->text().trimmed();
    p.name       = m_name      ->text().trimmed();
    p.checkCmd   = m_checkCmd  ->text().trimmed();
    p.installCmd = m_installCmd->text().trimmed();

    QString pText = m_platforms->text().trimmed();
    p.platforms = pText.isEmpty()
                  ? QStringList()
                  : pText.split(',', Qt::SkipEmptyParts);
    for (QString &s : p.platforms) s = s.trimmed();

    // Keep list label in sync with whatever name is in the form
    if (auto *item = m_list->item(m_currentRow))
        item->setText(p.name.isEmpty() ? p.id : p.name);
}

void PrerequisitesEditor::populateRow(int row)
{
    const Prerequisite &p = m_prereqs[row];
    m_id        ->setText(p.id);
    m_name      ->setText(p.name);
    m_checkCmd  ->setText(p.checkCmd);
    m_installCmd->setText(p.installCmd);
    m_platforms ->setText(p.platforms.join(", "));
}

void PrerequisitesEditor::buildUi()
{
    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    outer->addWidget(splitter);

    // ── Left: list + add/remove buttons ──────────────────────────────────
    auto *leftWidget = new QWidget(splitter);
    auto *leftVbox   = new QVBoxLayout(leftWidget);
    leftVbox->setContentsMargins(16, 16, 8, 16);
    leftVbox->setSpacing(6);

    auto *listTitle = new QLabel("Prerequisites", leftWidget);
    listTitle->setObjectName("titleLabel");
    leftVbox->addWidget(listTitle);

    m_list = new QListWidget(leftWidget);
    m_list->setAlternatingRowColors(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    leftVbox->addWidget(m_list, 1);

    {
        auto *btnRow = new QHBoxLayout;
        m_btnAdd    = new QPushButton(si(SvgIcons::kNew),   "Add",    leftWidget);
        m_btnRemove = new QPushButton(si(SvgIcons::kTrash), "Remove", leftWidget);
        btnRow->addWidget(m_btnAdd);
        btnRow->addWidget(m_btnRemove);
        leftVbox->addLayout(btnRow);
    }

    splitter->addWidget(leftWidget);

    // ── Right: detail form ────────────────────────────────────────────────
    auto *rightWidget = new QWidget(splitter);
    auto *rightVbox   = new QVBoxLayout(rightWidget);
    rightVbox->setContentsMargins(8, 16, 16, 16);
    rightVbox->setSpacing(14);

    auto *detailTitle = new QLabel("Prerequisite Details", rightWidget);
    detailTitle->setObjectName("titleLabel");
    rightVbox->addWidget(detailTitle);

    m_detail = new QGroupBox(rightWidget);
    m_detail->setEnabled(false);
    auto *form = new QFormLayout(m_detail);
    form->setSpacing(10);

    // ── ID ────────────────────────────────────────────────────────────────
    m_id = new QLineEdit(m_detail);
    m_id->setPlaceholderText("e.g. dotnet48, vcredist2022");
    form->addRow("ID:", m_id);

    // ── Name ──────────────────────────────────────────────────────────────
    m_name = new QLineEdit(m_detail);
    m_name->setPlaceholderText("Display name shown to the user");
    form->addRow("Name:", m_name);

    // ── Check command ─────────────────────────────────────────────────────
    m_checkCmd = new QLineEdit(m_detail);
    m_checkCmd->setPlaceholderText("Command that exits 0 if present, non-zero if missing");
    form->addRow("Check Command:", m_checkCmd);

    auto *chkHint = new QLabel(
        "Exit 0 = installed.  Exit non-zero = missing.", m_detail);
    chkHint->setObjectName("hintLabel");
    form->addRow("", chkHint);

    // ── Install command ───────────────────────────────────────────────────
    m_installCmd = new QLineEdit(m_detail);
    m_installCmd->setPlaceholderText("Install command or URL shown to the user if missing");
    form->addRow("Install Command:", m_installCmd);

    // ── Platforms ─────────────────────────────────────────────────────────
    m_platforms = new QLineEdit(m_detail);
    m_platforms->setPlaceholderText("macos, windows, linux  (empty = all)");
    form->addRow("Platforms:", m_platforms);

    auto *platHint = new QLabel(
        "Comma-separated.  Leave empty to check on all platforms.", m_detail);
    platHint->setObjectName("hintLabel");
    form->addRow("", platHint);

    rightVbox->addWidget(m_detail, 1);
    rightVbox->addStretch();

    splitter->addWidget(rightWidget);
    splitter->setSizes({240, 700});

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_list,     &QListWidget::currentRowChanged,
            this,       &PrerequisitesEditor::onRowSelected);
    connect(m_btnAdd,   &QPushButton::clicked,
            this,       &PrerequisitesEditor::onAdd);
    connect(m_btnRemove,&QPushButton::clicked,
            this,       &PrerequisitesEditor::onRemove);
}
