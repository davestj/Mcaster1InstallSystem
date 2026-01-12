#pragma once
/*
 * StudioStyle.h — Dark theme stylesheet + palette for the Install Studio.
 * Apply once via: qApp->setStyleSheet(StudioStyle::darkSheet());
 */

#include <QString>
#include <QPalette>
#include <QColor>
#include <QApplication>

namespace StudioStyle {

// ── Color tokens ──────────────────────────────────────────────────────────────
static constexpr const char *kBg0      = "#0d0d1a";   // deepest background
static constexpr const char *kBg1      = "#1a1a2e";   // main window bg
static constexpr const char *kBg2      = "#16213e";   // panel / toolbar bg
static constexpr const char *kBg3      = "#0f3460";   // selected / active bg
static constexpr const char *kAccent   = "#00c9ff";   // highlight / accent
static constexpr const char *kAccent2  = "#7dd3fc";   // secondary accent
static constexpr const char *kFg       = "#e0e0e8";   // primary text
static constexpr const char *kFgDim    = "#888899";   // secondary text
static constexpr const char *kBorder   = "#2d2d4a";   // dividers / borders
static constexpr const char *kSuccess  = "#22c55e";   // build OK
static constexpr const char *kWarn     = "#f5c842";   // warnings
static constexpr const char *kError    = "#ef4444";   // errors

inline QString darkSheet()
{
    return QStringLiteral(
    /* ── Global ────────────────────────────────────────────────────────── */
    "QMainWindow, QDialog { background: #1a1a2e; color: #e0e0e8; }"
    "QWidget { background: #1a1a2e; color: #e0e0e8; "
              "font-family: 'SF Pro Text', 'Segoe UI', system-ui, sans-serif; "
              "font-size: 13px; }"

    /* ── Menu bar ───────────────────────────────────────────────────────── */
    "QMenuBar { background: #16213e; color: #e0e0e8; border-bottom: 1px solid #2d2d4a; }"
    "QMenuBar::item:selected { background: #0f3460; }"
    "QMenu { background: #16213e; color: #e0e0e8; border: 1px solid #2d2d4a; }"
    "QMenu::item:selected { background: #0f3460; color: #00c9ff; }"
    "QMenu::separator { background: #2d2d4a; height: 1px; margin: 4px 8px; }"

    /* ── Tool bar ───────────────────────────────────────────────────────── */
    "QToolBar { background: #16213e; border: none; padding: 4px 8px; spacing: 4px; }"
    "QToolBar::separator { background: #2d2d4a; width: 1px; margin: 6px 4px; }"
    "QToolButton { background: transparent; color: #e0e0e8; border: none; "
                   "border-radius: 5px; padding: 5px 8px; }"
    "QToolButton:hover   { background: #0f3460; color: #00c9ff; }"
    "QToolButton:pressed { background: #00c9ff; color: #0d0d1a; }"
    "QToolButton:checked { background: #0f3460; color: #00c9ff; "
                           "border: 1px solid #00c9ff; }"

    /* ── Splitter ───────────────────────────────────────────────────────── */
    "QSplitter::handle { background: #2d2d4a; }"
    "QSplitter::handle:horizontal { width: 2px; }"
    "QSplitter::handle:vertical   { height: 2px; }"

    /* ── Sidebar tree ───────────────────────────────────────────────────── */
    "QTreeWidget { background: #16213e; border: none; color: #e0e0e8; "
                   "outline: none; }"
    "QTreeWidget::item { padding: 4px 6px; border-radius: 4px; }"
    "QTreeWidget::item:hover    { background: #2d2d4a; }"
    "QTreeWidget::item:selected { background: #0f3460; color: #00c9ff; }"
    "QTreeWidget::branch { background: #16213e; }"

    /* ── Tab bar ────────────────────────────────────────────────────────── */
    "QTabWidget::pane { border: none; border-top: 1px solid #2d2d4a; }"
    "QTabBar::tab { background: #16213e; color: #888899; "
                    "padding: 8px 20px; border: none; "
                    "border-bottom: 2px solid transparent; "
                    "min-width: 90px; font-size: 12px; }"
    "QTabBar::tab:selected { background: #1a1a2e; color: #00c9ff; "
                              "border-bottom: 2px solid #00c9ff; }"
    "QTabBar::tab:hover:!selected { background: #2d2d4a; color: #e0e0e8; }"

    /* ── Line edit / text edit ──────────────────────────────────────────── */
    "QLineEdit { background: #0f3460; border: 1px solid #2d2d4a; "
                 "border-radius: 5px; padding: 5px 8px; color: #e0e0e8; }"
    "QLineEdit:focus { border-color: #00c9ff; }"
    "QLineEdit:hover { border-color: #7dd3fc; }"
    "QTextEdit, QPlainTextEdit { background: #0d0d1a; border: 1px solid #2d2d4a; "
                                  "border-radius: 5px; color: #e0e0e8; "
                                  "padding: 4px; }"

    /* ── Push button ────────────────────────────────────────────────────── */
    "QPushButton { background: #0f3460; color: #e0e0e8; border: none; "
                   "border-radius: 5px; padding: 7px 16px; font-weight: 500; }"
    "QPushButton:hover   { background: #00c9ff; color: #0d0d1a; }"
    "QPushButton:pressed { background: #7dd3fc; color: #0d0d1a; }"
    "QPushButton:disabled { background: #2d2d4a; color: #555566; }"
    "QPushButton#buildBtn { background: #00c9ff; color: #0d0d1a; "
                             "font-weight: 700; font-size: 14px; "
                             "padding: 10px 24px; border-radius: 6px; }"
    "QPushButton#buildBtn:hover { background: #7dd3fc; }"

    /* ── Combo box ──────────────────────────────────────────────────────── */
    "QComboBox { background: #0f3460; border: 1px solid #2d2d4a; "
                 "border-radius: 5px; padding: 5px 8px; color: #e0e0e8; "
                 "min-width: 80px; }"
    "QComboBox:hover { border-color: #7dd3fc; }"
    "QComboBox::drop-down { border: none; padding-right: 8px; }"
    "QComboBox QAbstractItemView { background: #16213e; color: #e0e0e8; "
                                    "border: 1px solid #2d2d4a; }"

    /* ── Check box ──────────────────────────────────────────────────────── */
    "QCheckBox { spacing: 8px; }"
    "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #2d2d4a; "
                             "border-radius: 3px; background: #0f3460; }"
    "QCheckBox::indicator:checked { background: #00c9ff; border-color: #00c9ff; }"

    /* ── Group box ──────────────────────────────────────────────────────── */
    "QGroupBox { border: 1px solid #2d2d4a; border-radius: 6px; "
                  "margin-top: 1.5ex; color: #7dd3fc; font-weight: 600; }"
    "QGroupBox::title { subcontrol-origin: margin; padding: 0 6px; }"

    /* ── Labels ─────────────────────────────────────────────────────────── */
    "QLabel#sectionLabel { color: #7dd3fc; font-weight: 600; font-size: 11px; "
                            "letter-spacing: 1px; text-transform: uppercase; }"
    "QLabel#titleLabel   { color: #e0e0e8; font-weight: 700; font-size: 18px; }"
    "QLabel#hintLabel    { color: #888899; font-size: 11px; }"

    /* ── Progress bar ───────────────────────────────────────────────────── */
    "QProgressBar { background: #16213e; border: 1px solid #2d2d4a; "
                    "border-radius: 4px; text-align: center; color: #e0e0e8; }"
    "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                            "stop:0 #0f3460, stop:1 #00c9ff); "
                            "border-radius: 4px; }"

    /* ── Scroll bars ────────────────────────────────────────────────────── */
    "QScrollBar:vertical { background: #16213e; width: 8px; margin: 0; }"
    "QScrollBar::handle:vertical { background: #2d2d4a; border-radius: 4px; "
                                    "min-height: 24px; }"
    "QScrollBar::handle:vertical:hover { background: #00c9ff; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    "QScrollBar:horizontal { background: #16213e; height: 8px; margin: 0; }"
    "QScrollBar::handle:horizontal { background: #2d2d4a; border-radius: 4px; "
                                      "min-width: 24px; }"
    "QScrollBar::handle:horizontal:hover { background: #00c9ff; }"
    "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

    /* ── Status bar ─────────────────────────────────────────────────────── */
    "QStatusBar { background: #16213e; color: #888899; "
                   "border-top: 1px solid #2d2d4a; }"

    /* ── Table / list ───────────────────────────────────────────────────── */
    "QTableWidget, QListWidget { background: #16213e; border: none; "
                                   "color: #e0e0e8; gridline-color: #2d2d4a; }"
    "QTableWidget::item:selected, QListWidget::item:selected "
    "{ background: #0f3460; color: #00c9ff; }"
    "QHeaderView::section { background: #0d0d1a; color: #7dd3fc; "
                              "border: none; border-bottom: 1px solid #2d2d4a; "
                              "padding: 4px 8px; font-weight: 600; }"

    /* ── Dock widget ────────────────────────────────────────────────────── */
    "QDockWidget { color: #e0e0e8; titlebar-close-icon: none; }"
    "QDockWidget::title { background: #16213e; padding: 6px 10px; "
                           "border-bottom: 1px solid #2d2d4a; "
                           "font-weight: 600; color: #7dd3fc; }"

    /* ── Tooltip ────────────────────────────────────────────────────────── */
    "QToolTip { background: #0f3460; color: #e0e0e8; "
                 "border: 1px solid #00c9ff; border-radius: 4px; "
                 "padding: 4px 8px; }"
    );
}

// Apply dark Qt palette (for native controls that ignore stylesheet)
inline void applyDarkPalette()
{
    QPalette p;
    p.setColor(QPalette::Window,          QColor("#1a1a2e"));
    p.setColor(QPalette::WindowText,      QColor("#e0e0e8"));
    p.setColor(QPalette::Base,            QColor("#16213e"));
    p.setColor(QPalette::AlternateBase,   QColor("#0d0d1a"));
    p.setColor(QPalette::Text,            QColor("#e0e0e8"));
    p.setColor(QPalette::Button,          QColor("#0f3460"));
    p.setColor(QPalette::ButtonText,      QColor("#e0e0e8"));
    p.setColor(QPalette::Highlight,       QColor("#00c9ff"));
    p.setColor(QPalette::HighlightedText, QColor("#0d0d1a"));
    p.setColor(QPalette::Link,            QColor("#00c9ff"));
    p.setColor(QPalette::PlaceholderText, QColor("#555566"));
    qApp->setPalette(p);
}

} // namespace StudioStyle
