#include "WelcomePage.h"
#include "InstallerWizard.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QPixmap>

WelcomePage::WelcomePage(QWidget *parent)
    : QWizardPage(parent)
{
    // Empty title/subtitle — MacStyle hides the header bar when both are empty,
    // giving our banner label the full page height from the top edge.
    setTitle("");
    setSubTitle("");

    // ── Outer: vertical stack (banner on top, content row below) ──────────────
    // Zero margins so the banner and side panel reach the page edges cleanly.
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Banner row — full wizard width, hidden until initializePage() loads it
    m_bannerLabel = new QLabel(this);
    m_bannerLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_bannerLabel->setVisible(false);
    outer->addWidget(m_bannerLabel);

    // Content row: [side panel column] [text area]
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    // Side panel — portrait column, hidden until initializePage() loads it
    m_sidePanelLabel = new QLabel(this);
    m_sidePanelLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_sidePanelLabel->setVisible(false);
    row->addWidget(m_sidePanelLabel);

    // Text area — intro text, padded so it doesn't touch the side panel edge
    auto *textBox = new QVBoxLayout;
    textBox->setContentsMargins(20, 14, 16, 12);
    textBox->setSpacing(8);

    auto *heading = new QLabel(
        "<span style='font-size:18px; font-weight:bold;'>"
        "Welcome to the Mcaster1DNAS Installer</span>",
        this);
    heading->setTextFormat(Qt::RichText);
    heading->setWordWrap(true);
    textBox->addWidget(heading);

    auto *version = new QLabel("Version 2.5.3-beta", this);
    QFont vFont = version->font();
    vFont.setPointSize(10);
    version->setFont(vFont);
    version->setStyleSheet("color: #555555;");
    textBox->addWidget(version);

    textBox->addSpacing(8);

    auto *intro = new QLabel(
        "<p><b>Mcaster1DNAS</b> is a next-generation Digital Network Audio "
        "Server &mdash; a fork of Icecast&nbsp;2 extended with ICY&nbsp;2.2 metadata, "
        "YAML configuration, and a native macOS control panel.</p>"
        "<p>Click <b>Next</b> to choose which components to install.</p>",
        this);
    intro->setTextFormat(Qt::RichText);
    intro->setWordWrap(true);
    textBox->addWidget(intro);

    textBox->addStretch();

    auto *note = new QLabel(
        "<span style='color:#888888; font-size:10px;'>"
        "Default install location:&nbsp;&nbsp;<tt>/Applications/Mcaster1/</tt>"
        "</span>",
        this);
    note->setTextFormat(Qt::RichText);
    textBox->addWidget(note);

    row->addLayout(textBox);
    outer->addLayout(row);
}

void WelcomePage::initializePage()
{
    auto *wiz = qobject_cast<InstallerWizard *>(wizard());
    if (!wiz) return;

    // Banner: full wizard width, computed content-crop
    QPixmap bpx = wiz->bannerPixmap();
    if (!bpx.isNull()) {
        m_bannerLabel->setPixmap(bpx);
        m_bannerLabel->setFixedSize(bpx.size());
        m_bannerLabel->setVisible(true);
    }

    // Side panel: portrait column, sized to match content area height
    QPixmap spx = wiz->sidePanelPixmap();
    if (!spx.isNull()) {
        m_sidePanelLabel->setPixmap(spx);
        m_sidePanelLabel->setFixedSize(spx.size());
        m_sidePanelLabel->setVisible(true);
    }
}

int WelcomePage::nextId() const
{
    return PAGE_COMPONENTS;
}
