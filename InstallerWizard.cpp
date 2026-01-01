#include "InstallerWizard.h"
#include "WelcomePage.h"
#include "ComponentsPage.h"
#include "InstallPage.h"
#include "FinishPage.h"

#include <QApplication>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QScreen>

// ── Layout constants ──────────────────────────────────────────────────────────
static constexpr int kWizW         = 760;  // wizard window width
static constexpr int kMacPageMarginH = 20; // Qt MacStyle left+right page margin (each side)
static constexpr int kPageW        = kWizW - 2 * kMacPageMarginH; // usable page content width
static constexpr int kContentH     = 250;  // content area (below banner on welcome page)
static constexpr int kButtonH      = 60;   // wizard button row
static constexpr int kBannerPad    = 18;   // padding around content-crop region

// ── Content-bounds scanner ────────────────────────────────────────────────────
// Scan row brightness to find where the actual image content (text, graphics)
// lives, ignoring the dark padding canvas surrounding it.
static std::pair<int,int> contentRows(const QImage &img, int threshold = 22)
{
    const int step = qMax(1, img.width() / 24);
    int top = 0, bottom = img.height() - 1;

    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); x += step) {
            QRgb px = img.pixel(x, y);
            if ((qRed(px) + qGreen(px) + qBlue(px)) / 3 > threshold) {
                top = y;
                goto found_top;
            }
        }
    }
    found_top:

    for (int y = img.height() - 1; y >= 0; --y) {
        for (int x = 0; x < img.width(); x += step) {
            QRgb px = img.pixel(x, y);
            if ((qRed(px) + qGreen(px) + qBlue(px)) / 3 > threshold) {
                bottom = y;
                goto found_bottom;
            }
        }
    }
    found_bottom:

    return { top, bottom };
}

InstallerWizard::InstallerWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle("Mcaster1DNAS Installer");

    // MacStyle: native macOS look — NO built-in left sidebar slot.
    // This gives the full wizard width to the page content, so our images
    // are displayed at their correct proportions without any sidebar eating
    // into the available horizontal space.
    setWizardStyle(QWizard::MacStyle);

    // ── Banner: detect content bounds, crop, scale to wizard width ────────────
    // The banner image has significant dark padding around the actual text/logo.
    // We scan row brightness to find the real content bounding box, then crop
    // to just those rows (+kBannerPad), and scale to the full wizard width.
    // The resulting pixmap is stored in m_bannerPixmap for WelcomePage.
    int wizH = 540;

    QImage bannerImg(resourcePath() + "/header-banner.png");
    if (!bannerImg.isNull()) {
        auto [cTop, cBot] = contentRows(bannerImg);
        cTop = qMax(0,                    cTop - kBannerPad);
        cBot = qMin(bannerImg.height()-1, cBot + kBannerPad);

        // Crop to content region
        QImage cropped = bannerImg.copy(0, cTop, bannerImg.width(), cBot - cTop + 1);

        // Scale to page content width (kWizW minus Qt's 20px MacStyle margins each side)
        // — no cropping, no squishing, full aspect ratio
        m_bannerPixmap = QPixmap::fromImage(
            cropped.scaledToWidth(kPageW, Qt::SmoothTransformation));

        // Wizard height adapts: banner height + content area + buttons
        wizH = m_bannerPixmap.height() + kContentH + kButtonH;

        // Safety: never exceed 90 % of screen height
        if (QScreen *scr = QGuiApplication::primaryScreen()) {
            int maxH = qRound(scr->availableGeometry().height() * 0.90);
            if (wizH > maxH) {
                wizH = maxH;
                int clampedBH = wizH - kContentH - kButtonH;
                // Clamp to height then re-scale to fit within page content width
                QImage tmp = cropped.scaledToHeight(clampedBH, Qt::SmoothTransformation);
                if (tmp.width() > kPageW)
                    tmp = tmp.scaledToWidth(kPageW, Qt::SmoothTransformation);
                m_bannerPixmap = QPixmap::fromImage(tmp);
            }
        }
    }

    // ── Side panel: stored for WelcomePage to embed in its content area ───────
    // We don't use WatermarkPixmap (which would eat into page width in MacStyle)
    // Instead, WelcomePage places the side panel image in a manual column layout.
    QImage sidePanelImg(resourcePath() + "/sidepanel.png");
    if (!sidePanelImg.isNull()) {
        // Scale to the wizard height so it fills a full-height column
        m_sidePanelPixmap = QPixmap::fromImage(
            sidePanelImg.scaledToHeight(kContentH, Qt::SmoothTransformation));
    }

    setFixedSize(kWizW, wizH);

    // Center on screen
    if (QScreen *scr = QGuiApplication::primaryScreen()) {
        QRect sg = scr->availableGeometry();
        move((sg.width()  - kWizW) / 2,
             (sg.height() - wizH)  / 2);
    }

    setButtonText(QWizard::CancelButton, "Quit");
    setButtonText(QWizard::FinishButton, "Done");
    setOption(QWizard::NoBackButtonOnStartPage,       true);
    setOption(QWizard::NoBackButtonOnLastPage,        true);
    setOption(QWizard::DisabledBackButtonOnLastPage,  true);

    setPage(PAGE_WELCOME,    new WelcomePage(this));
    setPage(PAGE_COMPONENTS, new ComponentsPage(this));
    setPage(PAGE_INSTALL,    new InstallPage(this));
    setPage(PAGE_FINISH,     new FinishPage(this));

    setStartId(PAGE_WELCOME);
}

QString InstallerWizard::resourcePath()
{
    return QApplication::applicationDirPath() + "/../Resources";
}

QString InstallerWizard::payloadPath()
{
    return resourcePath() + "/payload";
}
