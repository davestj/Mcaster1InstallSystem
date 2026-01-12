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
        "Define shortcuts created during installation (desktop icons, Start Menu entries, Dock tiles).", this);
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
    vbox->addWidget(m_table, 1);

    auto *btns = new QHBoxLayout;
    m_btnAdd = new QPushButton("+ Add Shortcut", this);
    m_btnDel = new QPushButton("Remove",         this);
    btns->addWidget(m_btnAdd);
    btns->addWidget(m_btnDel);
    btns->addStretch();
    vbox->addLayout(btns);

    connect(m_btnAdd, &QPushButton::clicked, this, &ShortcutsEditor::onAdd);
    connect(m_btnDel, &QPushButton::clicked, this, &ShortcutsEditor::onRemove);
}

void ShortcutsEditor::load(const Manifest &m)
{
    m_table->setRowCount(0);
    for (const Shortcut &sc : m.shortcuts) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(sc.name));
        m_table->setItem(row, 1, new QTableWidgetItem(sc.target));
        m_table->setItem(row, 2, new QTableWidgetItem(sc.icon));
        m_table->setItem(row, 3, new QTableWidgetItem(sc.type));
    }
}

void ShortcutsEditor::save(Manifest &m) const
{
    m.shortcuts.clear();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        Shortcut sc;
        sc.name   = m_table->item(r,0) ? m_table->item(r,0)->text() : QString();
        sc.target = m_table->item(r,1) ? m_table->item(r,1)->text() : QString();
        sc.icon   = m_table->item(r,2) ? m_table->item(r,2)->text() : QString();
        sc.type   = m_table->item(r,3) ? m_table->item(r,3)->text() : QString();
        if (!sc.name.isEmpty())
            m.shortcuts.append(sc);
    }
}

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
