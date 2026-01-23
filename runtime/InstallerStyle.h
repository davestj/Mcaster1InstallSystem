#pragma once
/*
 * InstallerStyle.h — Generates a Qt stylesheet from the WizardTheme
 * stored in the manifest.  Falls back to sensible dark defaults.
 *
 * The installer wizard uses the app's own branding colors so each
 * installer feels like it belongs to the product being installed.
 */

#include <QString>
#include <QPalette>
#include <QApplication>
#include <QColor>
#include "Manifest.h"

namespace InstallerStyle {

inline QString sheet(const WizardTheme &t)
{
    // Resolve accent + bg with fallbacks matching the Studio defaults
    const QString accent = t.accentColor.isEmpty()     ? "#00c9ff" : t.accentColor;
    const QString bg     = t.backgroundColor.isEmpty() ? "#1a1a2e" : t.backgroundColor;

    // Derive slightly lighter/darker variants
    QColor bgC(bg);
    QColor bg2 = bgC.lighter(115);
    QColor bg3 = bgC.lighter(140);
    QColor acC(accent);

    QString bg2s = bg2.name();
    QString bg3s = bg3.name();
    QString fgs  = (bgC.lightness() < 100) ? "#e0e0e8" : "#111111";
    QString dims  = (bgC.lightness() < 100) ? "#888899" : "#444444";

    return QString(
    "QWizard, QWidget { background: %1; color: %2; "
                       "font-family: 'SF Pro Text','Segoe UI',system-ui,sans-serif; "
                       "font-size: 13px; }"

    "QWizard > QWidget#__qt__passive_wizardbutton0,"
    "QWizard > QWidget#__qt__passive_wizardbutton1,"
    "QWizard > QWidget#__qt__passive_wizardbutton2 { background: %3; }"

    "QLabel { color: %2; }"
    "QLabel#titleLabel   { color: %2; font-weight: 700; font-size: 18px; }"
    "QLabel#subtitleLabel { color: %5; font-size: 12px; }"
    "QLabel#hintLabel    { color: %5; font-size: 11px; }"
    "QLabel#sectionLabel { color: %4; font-weight: 600; font-size: 11px; "
                           "letter-spacing: 1px; text-transform: uppercase; }"

    "QLineEdit { background: %3; border: 1px solid #2d2d4a; border-radius: 5px; "
                 "padding: 5px 8px; color: %2; }"
    "QLineEdit:focus { border-color: %4; }"

    "QPushButton { background: %3; color: %2; border: none; "
                   "border-radius: 5px; padding: 7px 16px; font-weight: 500; }"
    "QPushButton:hover   { background: %4; color: %1; }"
    "QPushButton:pressed { background: %4; color: %1; opacity: 0.8; }"
    "QPushButton:disabled { background: #2d2d4a; color: #555566; }"
    "QPushButton#nextBtn, QPushButton#finishBtn "
    "{ background: %4; color: %1; font-weight: 700; }"
    "QPushButton#nextBtn:hover, QPushButton#finishBtn:hover { opacity: 0.85; }"

    "QCheckBox { spacing: 8px; color: %2; }"
    "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #2d2d4a; "
                             "border-radius: 3px; background: %3; }"
    "QCheckBox::indicator:checked { background: %4; border-color: %4; }"

    "QRadioButton { spacing: 8px; color: %2; }"
    "QRadioButton::indicator { width: 14px; height: 14px; border: 1px solid #2d2d4a; "
                                "border-radius: 7px; background: %3; }"
    "QRadioButton::indicator:checked { background: %4; border-color: %4; }"

    "QProgressBar { background: %3; border: 1px solid #2d2d4a; border-radius: 4px; "
                    "text-align: center; color: %2; }"
    "QProgressBar::chunk { background: %4; border-radius: 4px; }"

    "QScrollBar:vertical { background: %3; width: 8px; margin: 0; }"
    "QScrollBar::handle:vertical { background: #2d2d4a; border-radius: 4px; min-height: 24px; }"
    "QScrollBar::handle:vertical:hover { background: %4; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"

    "QTextEdit, QPlainTextEdit { background: %1; border: 1px solid #2d2d4a; "
                                  "border-radius: 5px; color: %2; padding: 4px; }"

    "QGroupBox { border: 1px solid #2d2d4a; border-radius: 6px; "
                  "margin-top: 1.5ex; color: %4; font-weight: 600; }"
    "QGroupBox::title { subcontrol-origin: margin; padding: 0 6px; }"
    )
    .arg(bg, fgs, bg2s, accent, dims);
}

inline void apply(const WizardTheme &t)
{
    qApp->setStyleSheet(sheet(t));

    if (t.darkMode || QColor(t.backgroundColor).lightness() < 100) {
        QPalette p;
        p.setColor(QPalette::Window,          QColor(t.backgroundColor.isEmpty() ? "#1a1a2e" : t.backgroundColor));
        p.setColor(QPalette::WindowText,      QColor("#e0e0e8"));
        p.setColor(QPalette::Base,            QColor("#16213e"));
        p.setColor(QPalette::AlternateBase,   QColor("#0d0d1a"));
        p.setColor(QPalette::Text,            QColor("#e0e0e8"));
        p.setColor(QPalette::Button,          QColor("#0f3460"));
        p.setColor(QPalette::ButtonText,      QColor("#e0e0e8"));
        p.setColor(QPalette::Highlight,       QColor(t.accentColor.isEmpty() ? "#00c9ff" : t.accentColor));
        p.setColor(QPalette::HighlightedText, QColor("#0d0d1a"));
        qApp->setPalette(p);
    }
}

} // namespace InstallerStyle
