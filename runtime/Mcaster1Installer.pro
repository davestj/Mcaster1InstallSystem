# ─────────────────────────────────────────────────────────────────────────────
# Mcaster1Installer.pro — Qt Creator project for the Runtime Installer Wizard
# ─────────────────────────────────────────────────────────────────────────────
# Requires Qt6 with: Widgets Svg SvgWidgets
# ─────────────────────────────────────────────────────────────────────────────

QT      += widgets svg svgwidgets
CONFIG  += c++17
TARGET   = Mcaster1Installer
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

win32: CONFIG += windows
macx:  QMAKE_INFO_PLIST = $$PWD/../resources/Info.plist

# ── Include search paths ──────────────────────────────────────────────────────
INCLUDEPATH += \
    $$PWD \
    $$PWD/../manifest

# ── Source files ──────────────────────────────────────────────────────────────
SOURCES += \
    main.cpp \
    InstallerWizard.cpp \
    WelcomePage.cpp \
    LicensePage.cpp \
    PrerequisitesPage.cpp \
    ComponentsPage.cpp \
    DirectoryPage.cpp \
    ReadyPage.cpp \
    InstallPage.cpp \
    FinishPage.cpp \
    InstallEngine.cpp \
    ../manifest/Manifest.cpp

# ── Header files ──────────────────────────────────────────────────────────────
HEADERS += \
    InstallerWizard.h \
    WelcomePage.h \
    LicensePage.h \
    PrerequisitesPage.h \
    ComponentsPage.h \
    DirectoryPage.h \
    ReadyPage.h \
    InstallPage.h \
    FinishPage.h \
    InstallEngine.h \
    InstallerStyle.h \
    ../manifest/Manifest.h

# ── Windows: resource + UAC elevation (installer requires admin) ──────────────
win32 {
    RC_FILE = ../windows/res/Mcaster1Installer.rc
    DEFINES += _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN
    # Note: UAC elevation is declared in the .manifest embedded in the .rc file
}

# ── macOS: bundle setup ───────────────────────────────────────────────────────
macx {
    DOCS_DIR.files = $$PWD/../docs
    DOCS_DIR.path  = Contents/Resources/docs
    QMAKE_BUNDLE_DATA += DOCS_DIR
    ICON = $$PWD/../resources/icons/mcaster1.icns
}

# ── Output directory ──────────────────────────────────────────────────────────
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../build/debug
} else {
    DESTDIR = $$PWD/../build/release
}

# ── Post-build: windeployqt (Windows) ────────────────────────────────────────
win32 {
    QMAKE_POST_LINK += $$[QT_INSTALL_BINS]/windeployqt.exe $$shell_quote($$DESTDIR/$$TARGET.exe) $$escape_expand(\\n\\t)
}
