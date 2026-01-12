/*
 * main.cpp — Mcaster1 Install Studio entry point
 *
 * Applies dark theme, creates the main window and hands control to Qt.
 */

#include <QApplication>
#include <QSurfaceFormat>
#include "StudioMainWindow.h"
#include "StudioStyle.h"

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

    // ── Apply dark theme ────────────────────────────────────────────────────
    app.setStyleSheet(StudioStyle::darkSheet());
    StudioStyle::applyDarkPalette();

    // ── Main window ─────────────────────────────────────────────────────────
    StudioMainWindow w;
    w.show();

    // Open project from command-line if supplied
    if (argc > 1) {
        const QString path = QString::fromLocal8Bit(argv[1]);
        if (path.endsWith(".mis", Qt::CaseInsensitive))
            w.openProject(path);
    }

    return app.exec();
}
