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
    vbox->addWidget(m_table, 1);

    auto *btns = new QHBoxLayout;
    m_btnAdd = new QPushButton("+ Add Entry", this);
    m_btnDel = new QPushButton("Remove",      this);
    btns->addWidget(m_btnAdd);
    btns->addWidget(m_btnDel);
    btns->addStretch();
    vbox->addLayout(btns);

    connect(m_btnAdd, &QPushButton::clicked, this, &RegistryEditor::onAdd);
    connect(m_btnDel, &QPushButton::clicked, this, &RegistryEditor::onRemove);
}

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
        re.hive      = m_table->item(r,0) ? m_table->item(r,0)->text() : QString();
        re.key       = m_table->item(r,1) ? m_table->item(r,1)->text() : QString();
        re.valueName = m_table->item(r,2) ? m_table->item(r,2)->text() : QString();
        re.valueData = m_table->item(r,3) ? m_table->item(r,3)->text() : QString();
        re.valueType = m_table->item(r,4) ? m_table->item(r,4)->text() : QString();
        if (!re.key.isEmpty())
            m.registry.append(re);
    }
}

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
