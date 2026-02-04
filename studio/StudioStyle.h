#pragma once
/*
 * StudioStyle.h — Theme manager for Mcaster1 Install Studio.
 *
 * Two built-in themes:
 *   ThemeId::Dark        — Original deep-blue dark theme (Mcaster1 brand palette)
 *   ThemeId::Enterprise  — White background, nickel buttons, 3D chrome look
 *
 * Usage:
 *   // Apply on startup:
 *   ThemeManager::applyTheme(ThemeManager::loadTheme());
 *
 *   // Recolor an inline SVG icon for the current theme:
 *   QByteArray iconData = ThemeManager::recolorIcon(SvgIcons::kNew);
 *
 *   // Switch at runtime:
 *   ThemeManager::applyTheme(ThemeManager::Enterprise);
 *   ThemeManager::saveTheme(ThemeManager::Enterprise);
 */

#include <QString>
#include <QPalette>
#include <QColor>
#include <QApplication>
#include <QSettings>

// ─────────────────────────────────────────────────────────────────────────────
// Icon colour map entry (zero-terminated array)
// ─────────────────────────────────────────────────────────────────────────────
struct IconColorEntry { const char *from; const char *to; };

namespace StudioStyle {

// ─────────────────────────────────────────────────────────────────────────────
// ── Dark theme — Mcaster1 navy-blue palette (slightly lighter navy, Mar 2026) ─
// ─────────────────────────────────────────────────────────────────────────────
// Palette:
//   #1c2244 — main window background     (was #1a1a2e)
//   #1c2e52 — panel / secondary surface  (was #16213e)
//   #163968 — button / accent background (was #0f3460)
//   #111a2e — deepest (editor bg)        (was #0d0d1a)
//   #2d3a60 — borders / dividers         (was #2d2d4a)
inline QString darkSheet()
{
    return QStringLiteral(
    /* ── Global ─────────────────────────────────────────────────────────── */
    "QMainWindow, QDialog { background: #1c2244; color: #e0e0e8; }"
    "QWidget { background: #1c2244; color: #e0e0e8; "
              "font-family: 'SF Pro Text', 'Segoe UI', system-ui, sans-serif; "
              "font-size: 13px; }"
    /* ── Menu bar ─────────────────────────────────────────────────────────*/
    "QMenuBar { background: #1c2e52; color: #e0e0e8; border-bottom: 1px solid #2d3a60; }"
    "QMenuBar::item:selected { background: #163968; }"
    "QMenu { background: #1c2e52; color: #e0e0e8; border: 1px solid #2d3a60; }"
    "QMenu::item:selected { background: #163968; color: #00c9ff; }"
    "QMenu::separator { background: #2d3a60; height: 1px; margin: 4px 8px; }"
    /* ── Tool bar ─────────────────────────────────────────────────────────*/
    "QToolBar { background: #1c2e52; border: none; padding: 4px 8px; spacing: 4px; }"
    "QToolBar::separator { background: #2d3a60; width: 1px; margin: 6px 4px; }"
    "QToolButton { background: transparent; color: #e0e0e8; border: none; "
                   "border-radius: 5px; padding: 5px 8px; }"
    "QToolButton:hover   { background: #163968; color: #00c9ff; }"
    "QToolButton:pressed { background: #00c9ff; color: #111a2e; }"
    "QToolButton:checked { background: #163968; color: #00c9ff; border: 1px solid #00c9ff; }"
    /* ── Splitter ─────────────────────────────────────────────────────────*/
    "QSplitter::handle { background: #2d3a60; }"
    "QSplitter::handle:horizontal { width: 2px; }"
    "QSplitter::handle:vertical   { height: 2px; }"
    /* ── General tree widgets (editor panels) ────────────────────────────*/
    "QTreeWidget { background: #1c2e52; border: none; color: #e0e0e8; outline: none; }"
    "QTreeWidget::item { padding: 4px 6px; border-radius: 4px; }"
    "QTreeWidget::item:hover    { background: #2d3a60; }"
    "QTreeWidget::item:selected { background: #163968; color: #00c9ff; }"
    "QTreeWidget::branch { background: #1c2e52; }"
    /* ── Sidebar navigator tree ───────────────────────────────────────────*/
    "QTreeWidget#sidebarTree { background: #111a2e; border: none; color: #c8cce0; }"
    "QTreeWidget#sidebarTree::item { padding: 5px 4px; border-radius: 3px; }"
    "QTreeWidget#sidebarTree::item:hover    { background: #1c2e52; }"
    "QTreeWidget#sidebarTree::item:selected { background: #163968; color: #00c9ff; }"
    "QTreeWidget#sidebarTree::branch { background: #111a2e; }"
    /* ── Tab bar ──────────────────────────────────────────────────────────*/
    "QTabWidget::pane { border: none; border-top: 1px solid #2d3a60; }"
    "QTabBar::tab { background: #1c2e52; color: #888899; "
                    "padding: 8px 20px; border: none; "
                    "border-bottom: 2px solid transparent; "
                    "min-width: 90px; font-size: 12px; }"
    "QTabBar::tab:selected { background: #1c2244; color: #00c9ff; "
                              "border-bottom: 2px solid #00c9ff; }"
    "QTabBar::tab:hover:!selected { background: #2d3a60; color: #e0e0e8; }"
    /* ── Line edit / text edit ────────────────────────────────────────────*/
    "QLineEdit { background: #163968; border: 1px solid #2d3a60; "
                 "border-radius: 5px; padding: 5px 8px; color: #e0e0e8; }"
    "QLineEdit:focus { border-color: #00c9ff; }"
    "QLineEdit:hover { border-color: #7dd3fc; }"
    "QTextEdit, QPlainTextEdit { background: #111a2e; border: 1px solid #2d3a60; "
                                  "border-radius: 5px; color: #e0e0e8; padding: 4px; }"
    /* ── Push button ──────────────────────────────────────────────────────*/
    "QPushButton { background: #163968; color: #e0e0e8; border: none; "
                   "border-radius: 5px; padding: 7px 16px; font-weight: 500; }"
    "QPushButton:hover   { background: #00c9ff; color: #111a2e; }"
    "QPushButton:pressed { background: #7dd3fc; color: #111a2e; }"
    "QPushButton:disabled { background: #2d3a60; color: #555566; }"
    "QPushButton#buildBtn { background: #00c9ff; color: #111a2e; "
                             "font-weight: 700; font-size: 14px; "
                             "padding: 10px 24px; border-radius: 6px; }"
    "QPushButton#buildBtn:hover { background: #7dd3fc; }"
    /* ── Sidebar icon buttons (+, 📂) ─────────────────────────────────────*/
    "QPushButton#sidebarIconBtn { background: #163968; color: #c0c0cc; border: none; "
                                   "border-radius: 2px; font-size: 12px; padding: 0; }"
    "QPushButton#sidebarIconBtn:hover { background: #1e4d88; color: #ffffff; }"
    /* ── Combo box ────────────────────────────────────────────────────────*/
    "QComboBox { background: #163968; border: 1px solid #2d3a60; "
                 "border-radius: 5px; padding: 5px 8px; color: #e0e0e8; min-width: 80px; }"
    "QComboBox:hover { border-color: #7dd3fc; }"
    "QComboBox::drop-down { border: none; padding-right: 8px; }"
    "QComboBox QAbstractItemView { background: #1c2e52; color: #e0e0e8; "
                                    "border: 1px solid #2d3a60; }"
    /* ── Check box ────────────────────────────────────────────────────────*/
    "QCheckBox { spacing: 8px; }"
    "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #2d3a60; "
                             "border-radius: 3px; background: #163968; }"
    "QCheckBox::indicator:checked { background: #00c9ff; border-color: #00c9ff; }"
    /* ── Group box ────────────────────────────────────────────────────────*/
    "QGroupBox { border: 1px solid #2d3a60; border-radius: 6px; "
                  "margin-top: 1.5ex; color: #7dd3fc; font-weight: 600; }"
    "QGroupBox::title { subcontrol-origin: margin; padding: 0 6px; }"
    /* ── Labels ───────────────────────────────────────────────────────────*/
    "QLabel#sectionLabel { color: #7dd3fc; font-weight: 600; font-size: 11px; "
                            "letter-spacing: 1px; text-transform: uppercase; }"
    "QLabel#titleLabel   { color: #e0e0e8; font-weight: 700; font-size: 18px; }"
    "QLabel#hintLabel    { color: #888899; font-size: 11px; }"
    /* ── Sidebar section header bars ──────────────────────────────────────*/
    "QWidget#sidebarHeaderBar { background: #1c2e52; border-bottom: 1px solid #163968; }"
    "QLabel#sidebarSectionLabel { color: #5060a0; font-size: 9px; font-weight: 700; "
                                   "letter-spacing: 1px; background: transparent; }"
    /* ── Sidebar project list ─────────────────────────────────────────────*/
    "QListWidget#sidebarProjectList { background: #111a2e; border: none; }"
    "QListWidget#sidebarProjectList::item { border: none; padding: 0; }"
    "QListWidget#sidebarProjectList::item:selected { background: #163968; }"
    "QListWidget#sidebarProjectList::item:hover { background: #1c2e52; }"
    /* ── Sidebar inline name edit ─────────────────────────────────────────*/
    "QLineEdit#sidebarNameEdit { background: transparent; color: #d0d0e0; "
                                  "font-size: 13px; font-weight: bold; "
                                  "border: none; border-radius: 0; padding: 4px 10px; }"
    "QLineEdit#sidebarNameEdit:focus { background: #1c3462; "
                                        "border-bottom: 1px solid #00c9ff; }"
    /* ── Sidebar dividers ─────────────────────────────────────────────────*/
    "QFrame#sidebarDivider { border: none; background: #163968; max-height: 1px; }"
    /* ── Progress bar ─────────────────────────────────────────────────────*/
    "QProgressBar { background: #1c2e52; border: 1px solid #2d3a60; "
                    "border-radius: 4px; text-align: center; color: #e0e0e8; }"
    "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                            "stop:0 #163968, stop:1 #00c9ff); border-radius: 4px; }"
    /* ── Scroll bars ──────────────────────────────────────────────────────*/
    "QScrollBar:vertical { background: #1c2e52; width: 8px; margin: 0; }"
    "QScrollBar::handle:vertical { background: #2d3a60; border-radius: 4px; min-height: 24px; }"
    "QScrollBar::handle:vertical:hover { background: #00c9ff; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    "QScrollBar:horizontal { background: #1c2e52; height: 8px; margin: 0; }"
    "QScrollBar::handle:horizontal { background: #2d3a60; border-radius: 4px; min-width: 24px; }"
    "QScrollBar::handle:horizontal:hover { background: #00c9ff; }"
    "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
    /* ── Status bar ───────────────────────────────────────────────────────*/
    "QStatusBar { background: #1c2e52; color: #888899; border-top: 1px solid #2d3a60; }"
    /* ── Table / list ─────────────────────────────────────────────────────*/
    "QTableWidget, QListWidget { background: #1c2e52; border: none; "
                                   "color: #e0e0e8; gridline-color: #2d3a60; }"
    "QTableWidget::item:selected, QListWidget::item:selected { background: #163968; color: #00c9ff; }"
    "QHeaderView::section { background: #111a2e; color: #7dd3fc; "
                              "border: none; border-bottom: 1px solid #2d3a60; "
                              "padding: 4px 8px; font-weight: 600; }"
    /* ── Dock widget ──────────────────────────────────────────────────────*/
    "QDockWidget { color: #e0e0e8; titlebar-close-icon: none; }"
    "QDockWidget::title { background: #1c2e52; padding: 6px 10px; "
                           "border-bottom: 1px solid #2d3a60; "
                           "font-weight: 600; color: #7dd3fc; }"
    /* ── Tooltip ──────────────────────────────────────────────────────────*/
    "QToolTip { background: #163968; color: #e0e0e8; "
                 "border: 1px solid #00c9ff; border-radius: 4px; padding: 4px 8px; }"
    );
}

inline void applyDarkPalette()
{
    QPalette p;
    p.setColor(QPalette::Window,          QColor("#1c2244"));
    p.setColor(QPalette::WindowText,      QColor("#e0e0e8"));
    p.setColor(QPalette::Base,            QColor("#1c2e52"));
    p.setColor(QPalette::AlternateBase,   QColor("#111a2e"));
    p.setColor(QPalette::Text,            QColor("#e0e0e8"));
    p.setColor(QPalette::Button,          QColor("#163968"));
    p.setColor(QPalette::ButtonText,      QColor("#e0e0e8"));
    p.setColor(QPalette::Highlight,       QColor("#00c9ff"));
    p.setColor(QPalette::HighlightedText, QColor("#111a2e"));
    p.setColor(QPalette::Link,            QColor("#00c9ff"));
    p.setColor(QPalette::PlaceholderText, QColor("#555566"));
    qApp->setPalette(p);
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Enterprise Professional — white/nickel/3D chrome theme ───────────────────
// ─────────────────────────────────────────────────────────────────────────────
inline QString enterpriseSheet()
{
    return QStringLiteral(
    /* ── Global ─────────────────────────────────────────────────────────── */
    "QMainWindow, QDialog { background: #f0f0f0; color: #1a1a1a; }"
    "QWidget { background: #f0f0f0; color: #1a1a1a; "
              "font-family: 'Segoe UI', 'SF Pro Text', system-ui, sans-serif; "
              "font-size: 13px; }"
    /* ── Menu bar ─────────────────────────────────────────────────────────*/
    "QMenuBar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                "stop:0 #f8f8f8, stop:1 #e8e8e8); "
                "color: #1a1a1a; border-bottom: 1px solid #b8b8b8; }"
    "QMenuBar::item { padding: 5px 10px; }"
    "QMenuBar::item:selected { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                                "stop:0 #e8f3ff, stop:1 #cce0f8); "
                                "color: #0a2050; border: 1px solid #90b8e0; }"
    "QMenu { background: #ffffff; color: #1a1a1a; border: 1px solid #b0b0b0; }"
    "QMenu::item { padding: 5px 20px 5px 28px; }"
    "QMenu::item:selected { background: #0070d0; color: #ffffff; }"
    "QMenu::separator { background: #d0d0d0; height: 1px; margin: 4px 8px; }"
    "QMenu::indicator { width: 14px; height: 14px; }"
    /* ── Tool bar ─────────────────────────────────────────────────────────*/
    "QToolBar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                "stop:0 #f2f2f2, stop:0.5 #e8e8e8, stop:1 #d8d8d8); "
                "border: none; border-bottom: 1px solid #b8b8b8; "
                "padding: 3px 8px; spacing: 3px; }"
    "QToolBar::separator { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                           "stop:0 #c0c0c0, stop:0.5 #e0e0e0, stop:1 #c0c0c0); "
                           "width: 1px; margin: 4px 3px; }"
    "QToolButton { background: transparent; color: #1a1a1a; "
                   "border: 1px solid transparent; border-radius: 3px; padding: 4px 7px; }"
    "QToolButton:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                         "stop:0 #e8f3ff, stop:0.5 #d4e8ff, stop:1 #c0d8f8); "
                         "border: 1px solid #7ab8e8; color: #0a2050; }"
    "QToolButton:pressed { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                           "stop:0 #b8d0f0, stop:1 #d4e8ff); "
                           "border: 1px solid #5088c0; color: #0a2050; }"
    "QToolButton:checked { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                           "stop:0 #c8dff5, stop:1 #deeeff); "
                           "border: 1px solid #5898d0; color: #0a2050; }"
    /* ── Splitter ─────────────────────────────────────────────────────────*/
    "QSplitter::handle { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                         "stop:0 #d8d8d8, stop:0.5 #c8c8c8, stop:1 #d8d8d8); }"
    "QSplitter::handle:horizontal { width: 3px; }"
    "QSplitter::handle:vertical   { height: 3px; }"
    /* ── General tree widgets (editor panels) ────────────────────────────*/
    "QTreeWidget { background: #ffffff; border: 1px solid #c8c8c8; "
                   "color: #1a1a1a; outline: none; border-top: none; }"
    "QTreeWidget::item { padding: 4px 6px; }"
    "QTreeWidget::item:hover    { background: #e8f0fa; }"
    "QTreeWidget::item:selected { background: #0070d0; color: #ffffff; }"
    "QTreeWidget::branch { background: #ffffff; }"
    /* ── Sidebar navigator tree ───────────────────────────────────────────*/
    "QTreeWidget#sidebarTree { background: #f0f4fa; border: none; color: #1a2a4a; }"
    "QTreeWidget#sidebarTree::item { padding: 5px 4px; border-radius: 3px; }"
    "QTreeWidget#sidebarTree::item:hover    { background: #dce8f8; }"
    "QTreeWidget#sidebarTree::item:selected { background: #0070d0; color: #ffffff; }"
    "QTreeWidget#sidebarTree::branch { background: #f0f4fa; }"
    /* ── Tab bar ──────────────────────────────────────────────────────────*/
    "QTabWidget::pane { border: 1px solid #c0c0c0; border-top: none; "
                        "background: #f8f8f8; }"
    "QTabBar::tab { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                    "stop:0 #f5f5f5, stop:1 #e5e5e5); "
                    "color: #5a5a5a; padding: 7px 18px; "
                    "border: 1px solid #c0c0c0; border-bottom: none; "
                    "border-top-left-radius: 4px; border-top-right-radius: 4px; "
                    "margin-right: 2px; font-size: 12px; }"
    "QTabBar::tab:selected { background: #f8f8f8; color: #0a2050; "
                              "border-bottom: 2px solid #0070d0; "
                              "font-weight: 600; }"
    "QTabBar::tab:hover:!selected { background: #edf4fd; color: #1a1a1a; }"
    /* ── Line edit / text edit ────────────────────────────────────────────*/
    "QLineEdit { background: #ffffff; border: 1px solid #b0b0b0; "
                 "border-radius: 3px; padding: 5px 8px; color: #1a1a1a; }"
    "QLineEdit:focus { border: 2px solid #0070d0; padding: 4px 7px; }"
    "QLineEdit:hover { border-color: #7090b8; }"
    "QTextEdit, QPlainTextEdit { background: #ffffff; border: 1px solid #b0b0b0; "
                                  "border-radius: 3px; color: #1a1a1a; padding: 4px; }"
    /* ── Push button — 3D nickel/chrome ──────────────────────────────────*/
    "QPushButton { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0.00 #f8f8f8,"
            "stop:0.40 #eeeeee,"
            "stop:0.55 #e4e4e4,"
            "stop:1.00 #d8d8d8); "
        "border-top:    1px solid #c8c8c8; "
        "border-left:   1px solid #c0c0c0; "
        "border-bottom: 1px solid #909090; "
        "border-right:  1px solid #989898; "
        "border-radius: 3px; "
        "color: #1a1a1a; padding: 6px 16px; font-weight: 500; }"
    "QPushButton:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0.00 #eaf4ff,"
            "stop:0.40 #d4e9ff,"
            "stop:0.55 #c8e0ff,"
            "stop:1.00 #b8d4f8); "
        "border-top-color:    #90bcee; "
        "border-left-color:   #80b0e8; "
        "border-bottom-color: #4880c8; "
        "border-right-color:  #5090d0; "
        "color: #0a2050; }"
    "QPushButton:pressed { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #c0d8f0, stop:1 #d8eeff); "
        "border-top-color:    #5a80c0; "
        "border-left-color:   #6088c8; "
        "border-bottom-color: #9ab8d8; "
        "border-right-color:  #90b0d0; "
        "padding-top: 7px; padding-bottom: 5px; color: #0a2050; }"
    "QPushButton:disabled { background: #f0f0f0; "
                             "border: 1px solid #c8c8c8; color: #a0a0a0; }"
    /* Primary action button (Build) — enterprise blue */
    "QPushButton#buildBtn { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0.0 #2888f0,"
            "stop:0.5 #0068d8,"
            "stop:1.0 #0054c0); "
        "border-top:    1px solid #5898e0; "
        "border-left:   1px solid #4888d8; "
        "border-bottom: 1px solid #003ea0; "
        "border-right:  1px solid #0048a8; "
        "border-radius: 4px; color: #ffffff; "
        "font-weight: 700; font-size: 14px; padding: 10px 24px; }"
    "QPushButton#buildBtn:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #3898f8, stop:1 #1070e0); }"
    "QPushButton#buildBtn:pressed { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #0058c0, stop:1 #2078e8); "
        "padding-top: 11px; padding-bottom: 9px; }"
    /* ── Combo box ────────────────────────────────────────────────────────*/
    "QComboBox { background: #ffffff; border: 1px solid #b0b0b0; "
                 "border-radius: 3px; padding: 5px 8px; color: #1a1a1a; min-width: 80px; }"
    "QComboBox:hover { border-color: #6898c8; }"
    "QComboBox:focus { border: 2px solid #0070d0; padding: 4px 7px; }"
    "QComboBox::drop-down { border-left: 1px solid #c0c0c0; padding-right: 4px; "
                             "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                             "stop:0 #f0f0f0, stop:1 #e0e0e0); "
                             "width: 18px; }"
    "QComboBox QAbstractItemView { background: #ffffff; color: #1a1a1a; "
                                    "border: 1px solid #b0b0b0; "
                                    "selection-background-color: #0070d0; "
                                    "selection-color: #ffffff; }"
    /* ── Check box ────────────────────────────────────────────────────────*/
    "QCheckBox { spacing: 8px; color: #1a1a1a; }"
    "QCheckBox::indicator { width: 14px; height: 14px; "
                             "border: 1px solid #a0a0a0; border-radius: 2px; "
                             "background: #ffffff; }"
    "QCheckBox::indicator:hover { border-color: #0070d0; "
                                   "background: #edf4ff; }"
    "QCheckBox::indicator:checked { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #3090f0, stop:1 #0060c0); "
        "border-color: #0058b0; }"
    /* ── Group box ────────────────────────────────────────────────────────*/
    "QGroupBox { border: 1px solid #c0c0c0; border-radius: 4px; "
                  "margin-top: 1.5ex; color: #0a2050; font-weight: 600; "
                  "background: transparent; }"
    "QGroupBox::title { subcontrol-origin: margin; padding: 0 6px; "
                         "background: #f0f0f0; color: #0a2050; }"
    /* ── Labels ───────────────────────────────────────────────────────────*/
    "QLabel#sectionLabel { color: #0a2050; font-weight: 700; font-size: 11px; "
                            "letter-spacing: 1px; text-transform: uppercase; }"
    "QLabel#titleLabel   { color: #1a1a1a; font-weight: 700; font-size: 18px; }"
    "QLabel#hintLabel    { color: #5a5a5a; font-size: 11px; }"
    /* ── Progress bar ─────────────────────────────────────────────────────*/
    "QProgressBar { background: #e4e4e4; border: 1px solid #b8b8b8; "
                    "border-radius: 3px; text-align: center; color: #1a1a1a; }"
    "QProgressBar::chunk { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #0060c8, stop:0.5 #0090f0, stop:1 #0060c8); "
        "border-radius: 3px; }"
    /* ── Scroll bars — 3D nickel handles ─────────────────────────────────*/
    "QScrollBar:vertical { background: #f0f0f0; width: 14px; margin: 0; "
                            "border-left: 1px solid #d0d0d0; }"
    "QScrollBar::handle:vertical { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0.0 #e0e0e0, stop:0.3 #d0d0d0, stop:0.5 #c8c8c8,"
            "stop:0.7 #d0d0d0, stop:1.0 #e0e0e0); "
        "border: 1px solid #a8a8a8; border-radius: 5px; "
        "min-height: 28px; margin: 2px; }"
    "QScrollBar::handle:vertical:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0.0 #b0d0f0, stop:0.5 #88b8e8, stop:1.0 #b0d0f0); "
        "border-color: #5090c0; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    "QScrollBar:horizontal { background: #f0f0f0; height: 14px; margin: 0; "
                              "border-top: 1px solid #d0d0d0; }"
    "QScrollBar::handle:horizontal { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0.0 #e0e0e0, stop:0.3 #d0d0d0, stop:0.5 #c8c8c8,"
            "stop:0.7 #d0d0d0, stop:1.0 #e0e0e0); "
        "border: 1px solid #a8a8a8; border-radius: 5px; "
        "min-width: 28px; margin: 2px; }"
    "QScrollBar::handle:horizontal:hover { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0.0 #b0d0f0, stop:0.5 #88b8e8, stop:1.0 #b0d0f0); "
        "border-color: #5090c0; }"
    "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
    /* ── Status bar ───────────────────────────────────────────────────────*/
    "QStatusBar { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                   "stop:0 #e8e8e8, stop:1 #d8d8d8); "
                   "color: #3a3a3a; border-top: 1px solid #b8b8b8; }"
    "QStatusBar::item { border: none; }"
    /* ── Table / list ─────────────────────────────────────────────────────*/
    "QTableWidget, QListWidget { background: #ffffff; border: 1px solid #c0c0c0; "
                                   "color: #1a1a1a; gridline-color: #e0e0e0; }"
    "QTableWidget::item:selected, QListWidget::item:selected "
    "{ background: #0070d0; color: #ffffff; }"
    "QHeaderView::section { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #f0f0f0, stop:1 #e0e0e0); "
        "color: #1a1a1a; border: none; "
        "border-right: 1px solid #c8c8c8; border-bottom: 2px solid #b0b0b0; "
        "padding: 4px 8px; font-weight: 600; }"
    /* ── Dock widget ──────────────────────────────────────────────────────*/
    "QDockWidget { color: #1a1a1a; }"
    "QDockWidget::title { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #d8e8f8, stop:1 #c4d8f0); "
        "padding: 5px 10px; border-bottom: 1px solid #a8c0d8; "
        "color: #0a2050; font-weight: 600; }"
    /* ── Sidebar section header bars ──────────────────────────────────────*/
    "QWidget#sidebarHeaderBar { "
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #e8eef8, stop:1 #dce6f4); "
        "border-bottom: 1px solid #b4c4dc; }"
    "QLabel#sidebarSectionLabel { color: #3a5888; font-size: 9px; font-weight: 700; "
                                   "letter-spacing: 1px; background: transparent; }"
    /* ── Sidebar project list ─────────────────────────────────────────────*/
    "QListWidget#sidebarProjectList { background: #f8f8f8; border: none; }"
    "QListWidget#sidebarProjectList::item { border: none; padding: 0; }"
    "QListWidget#sidebarProjectList::item:selected { background: #d4e4f8; }"
    "QListWidget#sidebarProjectList::item:hover { background: #eaf0f8; }"
    /* ── Sidebar inline name edit ─────────────────────────────────────────*/
    "QLineEdit#sidebarNameEdit { background: #f4f4f4; color: #1a1a2e; "
                                  "font-size: 13px; font-weight: bold; "
                                  "border: none; border-radius: 0; padding: 4px 10px; }"
    "QLineEdit#sidebarNameEdit:focus { background: #edf4ff; "
                                        "border-bottom: 2px solid #0070d0; }"
    /* ── Sidebar dividers ─────────────────────────────────────────────────*/
    "QFrame#sidebarDivider { border: none; background: #c4d0e4; max-height: 1px; }"
    /* ── Sidebar icon buttons (+, 📂) ─────────────────────────────────────*/
    "QPushButton#sidebarIconBtn { background: transparent; color: #3a5888; border: none; "
                                   "border-radius: 2px; font-size: 12px; padding: 0; }"
    "QPushButton#sidebarIconBtn:hover { background: #c8d8f0; color: #0a2050; }"
    /* ── Tooltip — classic yellow ─────────────────────────────────────────*/
    "QToolTip { background: #fffce6; color: #1a1a1a; "
                 "border: 1px solid #b8a840; border-radius: 3px; padding: 4px 8px; }"
    );
}

inline void applyEnterprisePalette()
{
    QPalette p;
    p.setColor(QPalette::Window,          QColor("#f0f0f0"));
    p.setColor(QPalette::WindowText,      QColor("#1a1a1a"));
    p.setColor(QPalette::Base,            QColor("#ffffff"));
    p.setColor(QPalette::AlternateBase,   QColor("#f5f5f5"));
    p.setColor(QPalette::Text,            QColor("#1a1a1a"));
    p.setColor(QPalette::BrightText,      QColor("#ffffff"));
    p.setColor(QPalette::Button,          QColor("#e8e8e8"));
    p.setColor(QPalette::ButtonText,      QColor("#1a1a1a"));
    p.setColor(QPalette::Highlight,       QColor("#0070d0"));
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    p.setColor(QPalette::Link,            QColor("#0070d0"));
    p.setColor(QPalette::LinkVisited,     QColor("#5040a0"));
    p.setColor(QPalette::PlaceholderText, QColor("#a0a0a0"));
    p.setColor(QPalette::Mid,             QColor("#c0c0c0"));
    p.setColor(QPalette::Midlight,        QColor("#e0e0e0"));
    p.setColor(QPalette::Dark,            QColor("#a0a0a0"));
    p.setColor(QPalette::Shadow,          QColor("#808080"));
    qApp->setPalette(p);
}

// ─────────────────────────────────────────────────────────────────────────────
// ── ThemeManager — runtime theme switching + persistence ─────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class ThemeManager
{
public:
    enum ThemeId { Dark = 0, Enterprise = 1 };

    // Apply a theme to the running application
    static void applyTheme(ThemeId id, QApplication *app = qApp)
    {
        s_current = id;
        if (id == Enterprise) {
            app->setStyleSheet(enterpriseSheet());
            applyEnterprisePalette();
        } else {
            app->setStyleSheet(darkSheet());
            applyDarkPalette();
        }
    }

    static ThemeId currentTheme() { return s_current; }

    static QString themeName(ThemeId id)
    {
        return id == Enterprise
            ? QStringLiteral("Enterprise Professional")
            : QStringLiteral("Mcaster1 Dark");
    }

    // Persist the chosen theme via QSettings
    static void saveTheme(ThemeId id)
    {
        QSettings s("Mcaster1", "InstallStudio");
        s.setValue("theme", static_cast<int>(id));
    }

    // Load the persisted theme (defaults to Dark)
    static ThemeId loadTheme()
    {
        QSettings s("Mcaster1", "InstallStudio");
        const int v = s.value("theme", static_cast<int>(Dark)).toInt();
        return (v == static_cast<int>(Enterprise)) ? Enterprise : Dark;
    }

    // ── Icon recolouring ──────────────────────────────────────────────────
    // Dark theme SVG icons use light strokes / dark fills designed for a dark
    // background.  For the Enterprise (white) theme we swap those colours so
    // icons are readable and match the enterprise blue accent.
    //
    // Colour substitution table  (dark → enterprise):
    //   #e0e0e8  pale-grey stroke    →  #2a2a2a  near-black stroke
    //   #16213e  dark-navy fill      →  #d4e0f4  pale blue-grey fill
    //   #0f3460  deep-navy fill      →  #b8d0ec  medium blue fill
    //   #00c9ff  neon-cyan accent    →  #0070d0  enterprise blue
    //   #7dd3fc  sky-blue secondary  →  #3a8fcc  mid blue
    //   #ffffff  white text          →  #1a1a1a  near-black text

    static QByteArray recolorIcon(const char *svgStr)
    {
        if (s_current == Dark) return QByteArray(svgStr);
        QByteArray out(svgStr);
        static const IconColorEntry kMap[] = {
            { "#e0e0e8", "#2a2a2a" },
            { "#16213e", "#d4e0f4" },
            { "#0f3460", "#b8d0ec" },
            { "#00c9ff", "#0070d0" },
            { "#7dd3fc", "#3a8fcc" },
            { "#ffffff",  "#1a1a1a" },
            { nullptr, nullptr }
        };
        for (int i = 0; kMap[i].from; ++i)
            out.replace(kMap[i].from, kMap[i].to);
        return out;
    }

private:
    inline static ThemeId s_current = Dark;  // C++17 inline static
};

} // namespace StudioStyle
