/*
 * HelpPanel.cpp — Dockable help viewer (QTextBrowser-based)
 */

#include "HelpPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextBrowser>
#include <QLabel>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

// ── Constructor ───────────────────────────────────────────────────────────────
HelpPanel::HelpPanel(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

// ── Public ────────────────────────────────────────────────────────────────────
void HelpPanel::scrollToAnchor(const QString &anchor)
{
    if (!anchor.isEmpty())
        m_browser->scrollToAnchor(anchor);
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void HelpPanel::onOpenExternal()
{
    const QUrl url = m_browser->source();
    if (url.isValid() && !url.isEmpty())
        QDesktopServices::openUrl(url);
}

// ── Private helpers ───────────────────────────────────────────────────────────
void HelpPanel::buildUi()
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(0);

    // ── Toolbar ───────────────────────────────────────────────────────────
    auto *bar = new QWidget(this);
    bar->setFixedHeight(28);
    bar->setStyleSheet(
        "background:#0f3460; border-bottom:1px solid #1a4a8a;");
    auto *barHb = new QHBoxLayout(bar);
    barHb->setContentsMargins(4, 2, 4, 2);
    barHb->setSpacing(2);

    const QString btnStyle =
        "QPushButton { background:transparent; color:#9090b8; border:none;"
        "  font-size:14px; padding:0 5px; border-radius:2px; min-width:22px; }"
        "QPushButton:hover { color:#ffffff; background:#1a4a8a; }"
        "QPushButton:disabled { color:#444466; }";

    m_btnBack = new QPushButton("‹", bar);
    m_btnBack->setToolTip("Back");
    m_btnBack->setStyleSheet(btnStyle);
    m_btnBack->setEnabled(false);

    m_btnFwd = new QPushButton("›", bar);
    m_btnFwd->setToolTip("Forward");
    m_btnFwd->setStyleSheet(btnStyle);
    m_btnFwd->setEnabled(false);

    m_titleLbl = new QLabel("Help & Documentation", bar);
    m_titleLbl->setStyleSheet(
        "color:#6060a0; font-size:10px; letter-spacing:0.5px; padding-left:6px;");

    auto *extBtn = new QPushButton("↗ Browser", bar);
    extBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#6060a0; border:none;"
        "  font-size:10px; padding:0 6px; border-radius:2px; }"
        "QPushButton:hover { color:#00c9ff; }");
    extBtn->setToolTip("Open full documentation in system browser");
    connect(extBtn, &QPushButton::clicked, this, &HelpPanel::onOpenExternal);

    barHb->addWidget(m_btnBack);
    barHb->addWidget(m_btnFwd);
    barHb->addWidget(m_titleLbl, 1);
    barHb->addWidget(extBtn);
    vbox->addWidget(bar);

    // ── Browser ───────────────────────────────────────────────────────────
    m_browser = new QTextBrowser(this);
    m_browser->setOpenLinks(false);
    m_browser->setOpenExternalLinks(false);
    m_browser->setFrameShape(QFrame::NoFrame);
    m_browser->setStyleSheet(
        "QTextBrowser { background:#12122a; color:#d0d0e8; border:none;"
        "  font-family: system-ui, -apple-system, sans-serif;"
        "  font-size: 13px; padding: 8px; }"
        "QScrollBar:vertical { background:#0a0a1a; width:8px; border:none; }"
        "QScrollBar::handle:vertical { background:#1a3060; border-radius:4px; min-height:20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }");

    vbox->addWidget(m_browser, 1);

    // ── Navigation wiring ─────────────────────────────────────────────────
    connect(m_btnBack, &QPushButton::clicked, m_browser, &QTextBrowser::backward);
    connect(m_btnFwd,  &QPushButton::clicked, m_browser, &QTextBrowser::forward);
    connect(m_browser, &QTextBrowser::backwardAvailable,
            m_btnBack, &QPushButton::setEnabled);
    connect(m_browser, &QTextBrowser::forwardAvailable,
            m_btnFwd,  &QPushButton::setEnabled);

    // External http/https links → system browser; internal file links → browser
    connect(m_browser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        const QString scheme = url.scheme();
        if (scheme == "http" || scheme == "https") {
            QDesktopServices::openUrl(url);
        } else {
            m_browser->setSource(url);
        }
    });

    // Update toolbar title when source changes
    connect(m_browser, &QTextBrowser::sourceChanged, this, [this](const QUrl &url) {
        const QString fname = url.fileName();
        if (!fname.isEmpty())
            m_titleLbl->setText(fname == "index.html" ? "Help & Documentation" : fname);
    });

    // ── Load docs ─────────────────────────────────────────────────────────
    const QString docsDir = locateDocsDir();
    if (!docsDir.isEmpty()) {
        m_browser->setSearchPaths({docsDir});
        m_browser->setSource(QUrl::fromLocalFile(docsDir + "/index.html"));
    } else {
        m_browser->setHtml(
            "<html><body style='background:#12122a; color:#d0d0e8;"
            "  font-family:system-ui,sans-serif; padding:24px;'>"
            "<h2 style='color:#00c9ff;'>Documentation Not Found</h2>"
            "<p>Could not locate <code>docs/index.html</code>.</p>"
            "<p style='color:#888;'>Rebuild the project to copy docs into the app bundle:</p>"
            "<pre style='background:#0f3460; padding:10px; border-radius:4px;'>"
            "cmake --build studio/build</pre>"
            "</body></html>");
    }
}

QString HelpPanel::locateDocsDir()
{
    const QStringList candidates = {
        QCoreApplication::applicationDirPath() + "/../Resources/docs",
        QCoreApplication::applicationDirPath() + "/docs",
        QDir::current().filePath("docs"),
    };
    for (const QString &dir : candidates) {
        if (QFile::exists(dir + "/index.html"))
            return QDir::cleanPath(dir);
    }
    return {};
}
