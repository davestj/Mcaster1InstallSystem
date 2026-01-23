/*
 * CustomActionsEditor.cpp — Studio tab for editing custom install/uninstall actions.
 */

#include "CustomActionsEditor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include "SvgIcons.h"

// Column indices
enum Col { ColId = 0, ColTrigger, ColType, ColCommand, ColPlatforms, ColCount };

static const QStringList kTriggers = {
    "before-install", "after-install",
    "before-uninstall", "after-uninstall"
};

static const QStringList kTypes = { "shell", "script" };

static QIcon si(const char *svg, int sz = 16)
{
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}

CustomActionsEditor::CustomActionsEditor(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void CustomActionsEditor::load(const Manifest &m)
{
    m_actions = m.customActions;
    rebuildTable();
}

void CustomActionsEditor::save(Manifest &m) const
{
    // Flush all rows from table back into m_actions, then copy to manifest
    auto *self = const_cast<CustomActionsEditor *>(this);
    self->m_actions.resize(m_table->rowCount());
    for (int row = 0; row < m_table->rowCount(); ++row)
        collectRow(row, self->m_actions[row]);
    m.customActions = m_actions;
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void CustomActionsEditor::onAdd()
{
    CustomAction a;
    a.id      = QString("action_%1").arg(m_actions.size() + 1);
    a.trigger = "after-install";
    a.type    = "shell";
    m_actions.append(a);

    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_blockCellSignal = true;
    addTableRow(row, a);
    m_blockCellSignal = false;

    m_table->selectRow(row);
    m_table->scrollToBottom();
}

void CustomActionsEditor::onRemove()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount()) return;

    m_actions.removeAt(row);
    m_table->removeRow(row);
}

void CustomActionsEditor::onCellChanged(int row, int col)
{
    if (m_blockCellSignal) return;
    if (row < 0 || row >= m_actions.size()) return;

    // Only the text-item columns fire cellChanged (combos use currentIndexChanged)
    if (col == ColId || col == ColCommand || col == ColPlatforms) {
        collectRow(row, m_actions[row]);
    }
}

// ── Private ───────────────────────────────────────────────────────────────────
void CustomActionsEditor::rebuildTable()
{
    m_blockCellSignal = true;
    m_table->setRowCount(0);
    m_table->setRowCount(m_actions.size());

    for (int row = 0; row < m_actions.size(); ++row)
        addTableRow(row, m_actions[row]);

    m_blockCellSignal = false;
}

void CustomActionsEditor::addTableRow(int row, const CustomAction &a)
{
    m_table->setItem(row, ColId,       new QTableWidgetItem(a.id));
    m_table->setItem(row, ColCommand,  new QTableWidgetItem(a.command));
    m_table->setItem(row, ColPlatforms,new QTableWidgetItem(a.platforms.join(", ")));

    // Trigger combo
    auto *trigCombo = new QComboBox(m_table);
    trigCombo->addItems(kTriggers);
    int ti = kTriggers.indexOf(a.trigger);
    trigCombo->setCurrentIndex(ti >= 0 ? ti : 0);
    m_table->setCellWidget(row, ColTrigger, trigCombo);

    // Type combo
    auto *typeCombo = new QComboBox(m_table);
    typeCombo->addItems(kTypes);
    int yi = kTypes.indexOf(a.type);
    typeCombo->setCurrentIndex(yi >= 0 ? yi : 0);
    m_table->setCellWidget(row, ColType, typeCombo);

    // Connect combo changes so m_actions stays in sync
    connect(trigCombo, &QComboBox::currentIndexChanged, this, [this, row](int) {
        if (!m_blockCellSignal && row < m_actions.size())
            collectRow(row, m_actions[row]);
    });
    connect(typeCombo, &QComboBox::currentIndexChanged, this, [this, row](int) {
        if (!m_blockCellSignal && row < m_actions.size())
            collectRow(row, m_actions[row]);
    });
}

void CustomActionsEditor::collectRow(int row, CustomAction &a) const
{
    if (auto *item = m_table->item(row, ColId))
        a.id = item->text().trimmed();
    if (auto *item = m_table->item(row, ColCommand))
        a.command = item->text().trimmed();
    if (auto *item = m_table->item(row, ColPlatforms)) {
        QString t = item->text().trimmed();
        a.platforms = t.isEmpty()
                      ? QStringList()
                      : t.split(',', Qt::SkipEmptyParts);
        for (QString &s : a.platforms) s = s.trimmed();
    }

    if (auto *cb = qobject_cast<QComboBox *>(m_table->cellWidget(row, ColTrigger)))
        a.trigger = cb->currentText();
    if (auto *cb = qobject_cast<QComboBox *>(m_table->cellWidget(row, ColType)))
        a.type = cb->currentText();
}

void CustomActionsEditor::buildUi()
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(24, 16, 24, 16);
    vbox->setSpacing(10);

    // ── Title ─────────────────────────────────────────────────────────────
    auto *title = new QLabel("Custom Actions", this);
    title->setObjectName("titleLabel");
    vbox->addWidget(title);

    auto *subtitle = new QLabel(
        "Define shell commands or scripts that run at specific points during install/uninstall.",
        this);
    subtitle->setObjectName("hintLabel");
    subtitle->setWordWrap(true);
    vbox->addWidget(subtitle);

    // ── Toolbar ───────────────────────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;
        m_btnAdd    = new QPushButton(si(SvgIcons::kNew),   "Add Action",    this);
        m_btnRemove = new QPushButton(si(SvgIcons::kTrash), "Remove Action", this);
        row->addWidget(m_btnAdd);
        row->addWidget(m_btnRemove);
        row->addStretch();
        vbox->addLayout(row);
    }

    // ── Table ─────────────────────────────────────────────────────────────
    m_table = new QTableWidget(0, ColCount, this);
    m_table->setHorizontalHeaderLabels(
        {"ID", "Trigger", "Type", "Command", "Platforms"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->resizeSection(ColId,       120);
    m_table->horizontalHeader()->resizeSection(ColTrigger,  140);
    m_table->horizontalHeader()->resizeSection(ColType,      80);
    m_table->horizontalHeader()->resizeSection(ColPlatforms, 110);
    m_table->horizontalHeader()->setSectionResizeMode(ColCommand, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked |
                             QAbstractItemView::SelectedClicked |
                             QAbstractItemView::AnyKeyPressed);
    vbox->addWidget(m_table, 1);

    // ── Hint ──────────────────────────────────────────────────────────────
    auto *hint = new QLabel(
        "Platforms: comma-separated (macos, windows, linux).  Leave empty to run on all.",
        this);
    hint->setObjectName("hintLabel");
    vbox->addWidget(hint);

    // ── Connections ───────────────────────────────────────────────────────
    connect(m_btnAdd,    &QPushButton::clicked,
            this,        &CustomActionsEditor::onAdd);
    connect(m_btnRemove, &QPushButton::clicked,
            this,        &CustomActionsEditor::onRemove);
    connect(m_table,     &QTableWidget::cellChanged,
            this,        &CustomActionsEditor::onCellChanged);
}
