/*
 * ComponentsPage.cpp — Component selection
 */
#include "ComponentsPage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>

ComponentsPage::ComponentsPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Select Components");
    setSubTitle("Choose which features to install. Required components cannot be deselected.");

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 8, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);

    auto *inner  = new QWidget(m_scroll);
    m_checkLayout = new QVBoxLayout(inner);
    m_checkLayout->setContentsMargins(4, 4, 4, 4);
    m_checkLayout->setSpacing(12);

    m_scroll->setWidget(inner);
    outerLayout->addWidget(m_scroll);
}

void ComponentsPage::initializePage()
{
    // Build rows only once (manifest doesn't change between visits)
    if (m_populated) return;
    m_populated = true;

    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    const auto &components = wiz->manifest().components;

    if (components.isEmpty()) {
        auto *lbl = new QLabel("No optional components are available for this package.", this);
        lbl->setWordWrap(true);
        m_checkLayout->addWidget(lbl);
        m_checkLayout->addStretch();
        return;
    }

    for (const Component &comp : components) {
        // Each component gets a small card: checkbox + indented description
        auto *row = new QWidget(m_scroll->widget());
        auto *rl  = new QVBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->setSpacing(2);

        auto *cb = new QCheckBox(comp.name, row);
        cb->setChecked(comp.selected || comp.required);
        if (comp.required) {
            cb->setEnabled(false);
            cb->setToolTip("This component is required.");
        }
        cb->setObjectName("compCheck");
        rl->addWidget(cb);

        if (!comp.description.isEmpty()) {
            auto *desc = new QLabel(comp.description, row);
            desc->setWordWrap(true);
            desc->setIndent(20);
            desc->setObjectName("hintLabel");
            rl->addWidget(desc);
        }

        m_checkLayout->addWidget(row);
        m_checks.append(cb);

        connect(cb, &QCheckBox::toggled, this, &ComponentsPage::completeChanged);
    }

    m_checkLayout->addStretch();
}

bool ComponentsPage::isComplete() const
{
    // Satisfied if at least one component is checked, or there are none at all
    for (auto *cb : m_checks)
        if (cb->isChecked()) return true;
    return m_checks.isEmpty();
}

bool ComponentsPage::validatePage()
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (wiz)
        wiz->setSelectedComponents(collectSelectedIds());
    return true;
}

QStringList ComponentsPage::collectSelectedIds() const
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return {};

    const auto &components = wiz->manifest().components;
    QStringList ids;
    const int n = qMin(m_checks.size(), components.size());
    for (int i = 0; i < n; ++i)
        if (m_checks[i]->isChecked())
            ids << components[i].id;
    return ids;
}
