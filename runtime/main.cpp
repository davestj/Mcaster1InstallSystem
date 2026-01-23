/*
 * main.cpp — Mcaster1 Runtime Installer entry point
 *
 * Discovers manifest.mis from:
 *   1. Command-line argument (argv[1])
 *   2. App bundle Contents/Resources/manifest.mis  (macOS)
 *   3. <exedir>/manifest.mis                       (Linux/Windows)
 *   4. ./manifest.mis                              (dev/fallback)
 *
 * After loading the manifest, applies the WizardTheme, builds the SVG
 * app icon (dock + wizard title bar), and shows the installer wizard.
 */

#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>
#include <QDir>
#include <QFileInfo>

#include "InstallerWizard.h"
#include "InstallerStyle.h"
#include "Manifest.h"

// ── App logo SVG (same design as SvgIcons::kLogo in the Studio) ──────────────
// Inlined here so the runtime installer has zero dependency on studio headers.
static constexpr const char *kAppLogoSvg = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <defs>
    <linearGradient id="lg" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%"   stop-color="#0f3460"/>
      <stop offset="100%" stop-color="#00c9ff"/>
    </linearGradient>
  </defs>
  <rect x="2" y="2" width="44" height="44" rx="10" ry="10" fill="url(#lg)" opacity="0.92"/>
  <path d="M10 30 L10 18 L24 11 L38 18 L38 30 L24 37 Z"
        fill="none" stroke="#00c9ff" stroke-width="1.8" opacity="0.85"/>
  <path d="M10 18 L24 25 L38 18" fill="none" stroke="#00c9ff" stroke-width="1.4" opacity="0.7"/>
  <text x="24" y="29" text-anchor="middle" font-family="system-ui,sans-serif"
        font-weight="700" font-size="9" fill="#ffffff" letter-spacing="0.5">M1</text>
  <path d="M33 10 Q36 7 39 10 L34.5 14.5 L36 16 L31.5 20.5 L30 19 L25.5 23.5
           Q22 26 19 23 Q16 20 19 17 Q22 14 25.5 17.5 L30 13 L31.5 14.5 Z"
        fill="#00c9ff" opacity="0.6" transform="scale(0.45) translate(24,2)"/>
</svg>
)svg";

// ── Build a multi-resolution QIcon from an inline SVG string ─────────────────
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

// ── Manifest discovery ────────────────────────────────────────────────────────
static QString findManifest(int argc, char *argv[])
{
    // 1. Explicit argument
    if (argc > 1) {
        QString p = QString::fromLocal8Bit(argv[1]);
        if (QFile::exists(p)) return p;
    }

    QString exeDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_MAC
    // 2. macOS bundle: .app/Contents/MacOS/../Resources/manifest.mis
    QString bundleRes = exeDir + "/../Resources/manifest.mis";
    if (QFile::exists(bundleRes)) return bundleRes;
#endif

    // 3. Alongside the executable
    QString beside = exeDir + "/manifest.mis";
    if (QFile::exists(beside)) return beside;

    // 4. Current working directory
    QString cwd = QDir::currentPath() + "/manifest.mis";
    if (QFile::exists(cwd)) return cwd;

    return QString();
}

static QString findPayloadDir(const QString &manifestPath)
{
    // Payload sits alongside the manifest
    QString dir = QFileInfo(manifestPath).absolutePath();
    QString payload = dir + "/payload";
    if (QDir(payload).exists()) return payload;

    // macOS bundle: Resources/payload/
    QString exeDir = QCoreApplication::applicationDirPath();
    QString bundlePayload = exeDir + "/../Resources/payload";
    if (QDir(bundlePayload).exists()) return bundlePayload;

    return dir;  // fallback: manifest directory
}

int main(int argc, char *argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Mcaster1 Installer");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Mcaster1");
    app.setOrganizationDomain("mcaster1.com");

    // ── SVG app icon (dock + wizard title bar) ──────────────────────────────
    QIcon appIcon = buildAppIcon(kAppLogoSvg);
    app.setWindowIcon(appIcon);

    // ── Locate manifest ────────────────────────────────────────────────────
    QString manifestPath = findManifest(argc, argv);
    if (manifestPath.isEmpty()) {
        QMessageBox::critical(nullptr, "Installer Error",
            "Could not find manifest.mis.\n\n"
            "This installer appears to be incomplete.\n"
            "Please re-download the installer package.");
        return 1;
    }

    // ── Parse manifest ─────────────────────────────────────────────────────
    Manifest m;
    QString err;
    if (!m.load(manifestPath, &err)) {
        QMessageBox::critical(nullptr, "Installer Error",
            QString("Could not read installer manifest:\n%1\n\nError: %2")
                .arg(manifestPath, err));
        return 1;
    }

    // ── Apply theme ────────────────────────────────────────────────────────
    InstallerStyle::apply(m.theme);

    // Update app name to match the product
    if (!m.app.name.isEmpty())
        app.setApplicationName(QString("Install %1").arg(m.app.name));

    // ── Launch wizard ──────────────────────────────────────────────────────
    QString payloadDir = findPayloadDir(manifestPath);
    InstallerWizard wizard(m, payloadDir);
    wizard.setWindowIcon(appIcon);   // title-bar icon on the wizard window
    wizard.show();

    return app.exec();
}
