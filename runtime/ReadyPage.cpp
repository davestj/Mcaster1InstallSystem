/*
 * ReadyPage.cpp — Pre-installation summary / commit page
 */
#include "ReadyPage.h"
#include "InstallerWizard.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>

ReadyPage::ReadyPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle("Ready to Install");
    setSubTitle("Review your choices before installation begins.");

    // Mark as commit page — Qt replaces "Next" with "Install" (or "Commit" on some styles)
    setCommitPage(true);
    setButtonText(QWizard::CommitButton, "Install");

    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 16, 0, 0);
    vbox->setSpacing(8);

    auto *grp = new QGroupBox("Installation Summary", this);
    auto *gl  = new QVBoxLayout(grp);
    gl->setSpacing(6);

    m_appLabel  = new QLabel(grp);
    m_appLabel->setWordWrap(true);
    gl->addWidget(m_appLabel);

    m_dirLabel  = new QLabel(grp);
    m_dirLabel->setWordWrap(true);
    gl->addWidget(m_dirLabel);

    m_compLabel = new QLabel(grp);
    m_compLabel->setWordWrap(true);
    gl->addWidget(m_compLabel);

    vbox->addWidget(grp);

    auto *note = new QLabel(
        "Click <b>Install</b> to begin the installation. "
        "Click <b>Back</b> to review or change settings.", this);
    note->setWordWrap(true);
    note->setObjectName("hintLabel");
    vbox->addWidget(note);

    vbox->addStretch();
}

void ReadyPage::initializePage()
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    const Manifest &m = wiz->manifest();

    m_appLabel->setText(QString("<b>Application:</b> %1  %2")
                            .arg(m.app.name.toHtmlEscaped(),
                                 m.app.version.toHtmlEscaped()));

    m_dirLabel->setText(QString("<b>Install folder:</b> %1")
                            .arg(wiz->installDir().toHtmlEscaped()));

    // List selected component names
    const QStringList &selIds = wiz->selectedComponents();
    QStringList selNames;
    for (const Component &c : m.components)
        if (selIds.contains(c.id))
            selNames << c.name;

    if (selNames.isEmpty() && !m.components.isEmpty()) {
        // Fall back to all components if ComponentsPage was skipped
        for (const Component &c : m.components)
            selNames << c.name;
    }

    if (selNames.isEmpty())
        m_compLabel->setText("<b>Components:</b> (all files)");
    else
        m_compLabel->setText(QString("<b>Components:</b> %1")
                                 .arg(selNames.join(", ").toHtmlEscaped()));
}
