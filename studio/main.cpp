/*
 * main.cpp — Mcaster1 Install Studio entry point
 *
 * Applies dark theme, builds the SVG app icon (dock + title bar),
 * creates the main window and hands control to Qt.
 */

#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QSvgRenderer>
#include <QSurfaceFormat>
#include "StudioMainWindow.h"
#include "StudioStyle.h"
#include "SvgIcons.h"

// ── Build a multi-resolution QIcon from an inline SVG string ─────────────────
// Qt picks the best resolution for each use (dock at 64/128/256, title bar at
// 16/32). Providing all standard sizes avoids blurry up-scaling.
static QIcon buildAppIcon(const char *svgStr)
{
    QByteArray data(svgStr);
    QSvgRenderer renderer(data);

    QIcon icon;
    for (int sz : {16, 32, 64, 128, 256, 512}) {
        QPixmap px(sz, sz);
        px.fill(Qt::transparent);
        QPainter p(&px);
        renderer.render(&p);
        p.end();
        icon.addPixmap(px);
    }
    return icon;
}

int main(int argc, char *argv[])
{
    // ── High-DPI support ───────────────────────────────────────────────────
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    // ── App identity ────────────────────────────────────────────────────────
    app.setApplicationName("Mcaster1 Install Studio");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Mcaster1");
    app.setOrganizationDomain("mcaster1.com");

    // ── SVG app icon (dock + taskbar/title bar) ─────────────────────────────
    // Rendered from the inline kLogo SVG at multiple sizes so Qt can pick the
    // best resolution for each context (dock, title bar, Alt+Tab, Launchpad).
    QIcon appIcon = buildAppIcon(SvgIcons::kLogo);
    app.setWindowIcon(appIcon);

    // ── Apply persisted theme (defaults to Dark on first launch) ────────────
    const auto theme = StudioStyle::ThemeManager::loadTheme();
    StudioStyle::ThemeManager::applyTheme(theme, &app);

    // ── Main window ─────────────────────────────────────────────────────────
    StudioMainWindow w;
    w.setWindowIcon(appIcon);   // title-bar icon (macOS: proxy icon on Retina)
    w.show();

    // Open project from command-line if supplied
    if (argc > 1) {
        const QString path = QString::fromLocal8Bit(argv[1]);
        if (path.endsWith(".mis", Qt::CaseInsensitive))
            w.openProject(path);
    }

    return app.exec();
}
