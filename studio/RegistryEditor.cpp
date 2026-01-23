/*
 * RegistryEditor.cpp — Windows registry entries table
 */
#include "RegistryEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QApplication>
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

// ── Reusable QComboBox delegate ───────────────────────────────────────────────
class ComboDelegate : public QStyledItemDelegate
{
public:
    explicit ComboDelegate(QStringList items, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), m_items(std::move(items)) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &) const override
    {
        auto *cb = new QComboBox(parent);
        cb->addItems(m_items);
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

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &opt,
                              const QModelIndex &) const override
    {
        editor->setGeometry(opt.rect);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        QStyledItemDelegate::paint(painter, opt, idx);
        QRect ar(opt.rect.right() - 14, opt.rect.top() + 2, 12, opt.rect.height() - 4);
        QStyleOptionComboBox cbOpt;
        cbOpt.rect  = ar;
        cbOpt.state = opt.state;
        QApplication::style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &cbOpt, painter);
    }

private:
    QStringList m_items;
};

// ── Constructor ───────────────────────────────────────────────────────────────
RegistryEditor::RegistryEditor(QWidget *parent)
    : QWidget(parent)
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(24, 16, 24, 16);
    vbox->setSpacing(10);

    auto *lbl = new QLabel("Windows Registry Entries", this);
    lbl->setObjectName("titleLabel");
    vbox->addWidget(lbl);

    auto *hint = new QLabel(
        "Registry keys written during Windows installation (HKLM/HKCU). "
        "These have no effect on macOS or Linux builds.", this);
    hint->setObjectName("hintLabel");
    hint->setWordWrap(true);
    vbox->addWidget(hint);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({"Hive", "Key", "Value Name", "Data", "Type"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setToolTip(
        "Hive       — HKLM (system-wide) or HKCU (current user)\n"
        "Key        — registry path, e.g. SOFTWARE\\MyCompany\\MyApp\n"
        "Value Name — leave blank for (Default) value\n"
        "Data       — value data (use {install-dir} token for paths)\n"
        "Type       — REG_SZ, REG_DWORD, REG_EXPAND_SZ, REG_BINARY, REG_MULTI_SZ");

    // Hive column (0): HKLM / HKCU / HKCU_ALL
    m_table->setItemDelegateForColumn(0,
        new ComboDelegate({"HKLM", "HKCU", "HKCU_ALL"}, m_table));

    // Type column (4): common registry value types
    m_table->setItemDelegateForColumn(4,
        new ComboDelegate({"REG_SZ", "REG_DWORD", "REG_EXPAND_SZ",
                           "REG_BINARY", "REG_MULTI_SZ"}, m_table));

    vbox->addWidget(m_table, 1);

    auto *btns = new QHBoxLayout;
    m_btnAdd = new QPushButton(si(SvgIcons::kNew),   "Add Entry", this);
    m_btnDel = new QPushButton(si(SvgIcons::kTrash), "Remove",    this);
    m_btnAdd->setToolTip("Add a new registry entry");
    m_btnDel->setToolTip("Remove the selected registry entry");
    btns->addWidget(m_btnAdd);
    btns->addWidget(m_btnDel);
    btns->addStretch();
    vbox->addLayout(btns);

    connect(m_btnAdd, &QPushButton::clicked, this, &RegistryEditor::onAdd);
    connect(m_btnDel, &QPushButton::clicked, this, &RegistryEditor::onRemove);
}

// ── Public ────────────────────────────────────────────────────────────────────
void RegistryEditor::load(const Manifest &m)
{
    m_table->setRowCount(0);
    for (const RegistryEntry &re : m.registry) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(re.hive));
        m_table->setItem(row, 1, new QTableWidgetItem(re.key));
        m_table->setItem(row, 2, new QTableWidgetItem(re.valueName));
        m_table->setItem(row, 3, new QTableWidgetItem(re.valueData));
        m_table->setItem(row, 4, new QTableWidgetItem(re.valueType));
    }
}

void RegistryEditor::save(Manifest &m) const
{
    m.registry.clear();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        RegistryEntry re;
        re.hive      = m_table->item(r, 0) ? m_table->item(r, 0)->text() : "HKLM";
        re.key       = m_table->item(r, 1) ? m_table->item(r, 1)->text() : QString();
        re.valueName = m_table->item(r, 2) ? m_table->item(r, 2)->text() : QString();
        re.valueData = m_table->item(r, 3) ? m_table->item(r, 3)->text() : QString();
        re.valueType = m_table->item(r, 4) ? m_table->item(r, 4)->text() : "REG_SZ";
        if (!re.key.isEmpty())
            m.registry.append(re);
    }
}

// ── Private slots ─────────────────────────────────────────────────────────────
void RegistryEditor::onAdd()
{
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, new QTableWidgetItem("HKLM"));
    m_table->setItem(row, 1, new QTableWidgetItem("SOFTWARE\\MyCompany\\MyApp"));
    m_table->setItem(row, 2, new QTableWidgetItem("InstallDir"));
    m_table->setItem(row, 3, new QTableWidgetItem("{install-dir}"));
    m_table->setItem(row, 4, new QTableWidgetItem("REG_SZ"));
    m_table->editItem(m_table->item(row, 1));
}

void RegistryEditor::onRemove()
{
    int row = m_table->currentRow();
    if (row >= 0) m_table->removeRow(row);
}
