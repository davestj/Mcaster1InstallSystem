/*
 * ShortcutsEditor.cpp — Shortcuts table editor
 */
#include "ShortcutsEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QApplication>
#include "SvgIcons.h"

static QIcon si(const char *svg, int sz = 16)
{
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}

// ── ComboBox delegate for the "Type" column ───────────────────────────────────
// Shows a dropdown when the cell enters edit mode; saves the chosen string back.
class ShortcutTypeDelegate : public QStyledItemDelegate
{
public:
    static const QStringList &types() {
        static const QStringList t = {"app", "url", "webloc", "command"};
        return t;
    }

    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &,
                          const QModelIndex &) const override
    {
        auto *cb = new QComboBox(parent);
        cb->addItems(types());
        return cb;
    }

    void setEditorData(QWidget *editor, const QModelIndex &idx) const override
    {
        auto *cb = qobject_cast<QComboBox*>(editor);
        if (!cb) return;
        int i = cb->findText(idx.data(Qt::EditRole).toString());
        cb->setCurrentIndex(i < 0 ? 0 : i);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &idx) const override
    {
        auto *cb = qobject_cast<QComboBox*>(editor);
        if (cb) model->setData(idx, cb->currentText(), Qt::EditRole);
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &opt,
                              const QModelIndex &) const override
    {
        editor->setGeometry(opt.rect);
    }

    // Paint the cell with a subtle indicator so users know it's a combo
    void paint(QPainter *painter, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        QStyledItemDelegate::paint(painter, opt, idx);
        // Small down-arrow hint at the right edge
        QRect ar(opt.rect.right() - 14, opt.rect.top() + 2,
                 12, opt.rect.height() - 4);
        QStyleOptionComboBox cbOpt;
        cbOpt.rect = ar;
        cbOpt.state = opt.state;
        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorArrowDown,
                                             &cbOpt, painter);
    }
};

// ── Constructor ───────────────────────────────────────────────────────────────
ShortcutsEditor::ShortcutsEditor(QWidget *parent)
    : QWidget(parent)
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(24, 16, 24, 16);
    vbox->setSpacing(10);

    auto *lbl = new QLabel("Shortcuts", this);
    lbl->setObjectName("titleLabel");
    vbox->addWidget(lbl);

    auto *hint = new QLabel(
        "Define shortcuts created during installation (desktop icons, Start Menu entries, Dock tiles).\n"
        "Type: app = launch the app bundle/exe  |  url/webloc = browser URL  |  command = shell command", this);
    hint->setObjectName("hintLabel");
    hint->setWordWrap(true);
    vbox->addWidget(hint);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"Name", "Target", "Icon", "Type"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setToolTip(
        "Name    — label shown for the shortcut\n"
        "Target  — path to app/exe/URL (use {install-dir} token)\n"
        "Icon    — optional icon path (leave blank to use app default)\n"
        "Type    — app | url | webloc | command");

    // Install the combo delegate on the Type column (column 3)
    m_table->setItemDelegateForColumn(3, new ShortcutTypeDelegate(m_table));

    vbox->addWidget(m_table, 1);

    auto *btns = new QHBoxLayout;
    m_btnAdd = new QPushButton(si(SvgIcons::kNew),   "Add Shortcut", this);
    m_btnDel = new QPushButton(si(SvgIcons::kTrash), "Remove",       this);
    m_btnAdd->setToolTip("Add a new shortcut entry");
    m_btnDel->setToolTip("Remove the selected shortcut");
    btns->addWidget(m_btnAdd);
    btns->addWidget(m_btnDel);
    btns->addStretch();
    vbox->addLayout(btns);

    connect(m_btnAdd, &QPushButton::clicked, this, &ShortcutsEditor::onAdd);
    connect(m_btnDel, &QPushButton::clicked, this, &ShortcutsEditor::onRemove);
}

// ── Public ────────────────────────────────────────────────────────────────────
void ShortcutsEditor::load(const Manifest &m)
{
    m_table->setRowCount(0);
    for (const Shortcut &sc : m.shortcuts) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(sc.name));
        m_table->setItem(row, 1, new QTableWidgetItem(sc.target));
        m_table->setItem(row, 2, new QTableWidgetItem(sc.icon));
        // Validate type — fall back to "app" if value is not in the known set
        QString type = sc.type;
        if (!ShortcutTypeDelegate::types().contains(type)) type = "app";
        m_table->setItem(row, 3, new QTableWidgetItem(type));
    }
}

void ShortcutsEditor::save(Manifest &m) const
{
    m.shortcuts.clear();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        Shortcut sc;
        sc.name   = m_table->item(r, 0) ? m_table->item(r, 0)->text() : QString();
        sc.target = m_table->item(r, 1) ? m_table->item(r, 1)->text() : QString();
        sc.icon   = m_table->item(r, 2) ? m_table->item(r, 2)->text() : QString();
        sc.type   = m_table->item(r, 3) ? m_table->item(r, 3)->text() : "app";
        if (!sc.name.isEmpty())
            m.shortcuts.append(sc);
    }
}

// ── Private slots ─────────────────────────────────────────────────────────────
void ShortcutsEditor::onAdd()
{
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem("Launch My Application"));
    m_table->setItem(row, 1, new QTableWidgetItem("{install-dir}/MyApp.app"));
    m_table->setItem(row, 2, new QTableWidgetItem(""));
    m_table->setItem(row, 3, new QTableWidgetItem("app"));
    m_table->editItem(m_table->item(row, 0));
}

void ShortcutsEditor::onRemove()
{
    int row = m_table->currentRow();
    if (row >= 0) m_table->removeRow(row);
}
