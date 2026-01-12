/*
 * ComponentsEditor.cpp — Component dependency tree (Phase 1 stub)
 */
#include "ComponentsEditor.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>

ComponentsEditor::ComponentsEditor(QWidget *parent)
    : QWidget(parent)
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(24, 16, 24, 16);

    auto *lbl = new QLabel("Component Dependencies", this);
    lbl->setObjectName("titleLabel");
    vbox->addWidget(lbl);

    auto *hint = new QLabel(
        "Shows component dependency relationships.\n"
        "Phase 2: drag-connect components to set dependencies.", this);
    hint->setObjectName("hintLabel");
    hint->setWordWrap(true);
    vbox->addWidget(hint);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({"Component", "Depends On", "Required", "Platforms"});
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    vbox->addWidget(m_tree, 1);
}

void ComponentsEditor::load(const Manifest &m)
{
    m_tree->clear();
    for (const Component &c : m.components) {
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(0, c.name.isEmpty() ? c.id : c.name);
        item->setText(1, c.depends.join(", "));
        item->setText(2, c.required ? "Yes" : "No");
        item->setText(3, c.platforms.isEmpty() ? "All" : c.platforms.join(", "));
    }
    m_tree->expandAll();
}

void ComponentsEditor::save(Manifest &/*m*/) const
{
    // Phase 1: read-only view — no changes written back
}
