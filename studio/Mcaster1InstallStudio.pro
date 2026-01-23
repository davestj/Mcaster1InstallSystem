# ─────────────────────────────────────────────────────────────────────────────
# Mcaster1InstallStudio.pro — Qt Creator project for the Studio IDE
# ─────────────────────────────────────────────────────────────────────────────
# Requires Qt6 with: Widgets Svg SvgWidgets Concurrent
# Tested with: Qt 6.7+ / MSVC2022 x64 (Windows), Clang/Apple Silicon (macOS)
# ─────────────────────────────────────────────────────────────────────────────

QT      += widgets svg svgwidgets concurrent
CONFIG  += c++17
TARGET   = Mcaster1InstallStudio
TEMPLATE = app

# Suppress deprecation warnings from Qt internals
DEFINES += QT_DEPRECATED_WARNINGS

# Windows: GUI application (no console window)
win32: CONFIG += windows

# macOS: generate a proper .app bundle
macx: QMAKE_INFO_PLIST = $$PWD/../resources/Info.plist

# ── Include search paths ──────────────────────────────────────────────────────
INCLUDEPATH += \
    $$PWD \
    $$PWD/../manifest \
    $$PWD/../backends \
    $$PWD/../importers

# ── Source files ──────────────────────────────────────────────────────────────
SOURCES += \
    main.cpp \
    StudioMainWindow.cpp \
    ProjectSidebar.cpp \
    AppInfoEditor.cpp \
    FilesEditor.cpp \
    ComponentsEditor.cpp \
    ShortcutsEditor.cpp \
    RegistryEditor.cpp \
    SecurityEditor.cpp \
    PrerequisitesEditor.cpp \
    CustomActionsEditor.cpp \
    BuildPanel.cpp \
    CodeSignDialog.cpp \
    BuilderProfileDialog.cpp \
    ../manifest/Manifest.cpp \
    ../importers/NsisImporter.cpp \
    ../importers/InnoSetupImporter.cpp \
    ../backends/CodeSigner.cpp \
    ../backends/CertGenerator.cpp

# ── Platform-specific backend sources ────────────────────────────────────────
win32 {
    SOURCES += ../backends/WindowsBackend.cpp
    DEFINES += _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN
}
macx {
    SOURCES += ../backends/MacOsBackend.cpp
    QT += concurrent
}
linux {
    SOURCES += ../backends/LinuxBackend.cpp
}

# ── Header files (HEADERS triggers AUTOMOC for Q_OBJECT classes) ──────────────
HEADERS += \
    StudioMainWindow.h \
    ProjectSidebar.h \
    AppInfoEditor.h \
    FilesEditor.h \
    ComponentsEditor.h \
    ShortcutsEditor.h \
    RegistryEditor.h \
    SecurityEditor.h \
    PrerequisitesEditor.h \
    CustomActionsEditor.h \
    BuildPanel.h \
    CodeSignDialog.h \
    BuilderProfileDialog.h \
    BuilderProfile.h \
    EventLog.h \
    BuildHistory.h \
    StudioStyle.h \
    SvgIcons.h \
    ../manifest/Manifest.h \
    ../backends/BuildBackend.h \
    ../backends/CodeSigner.h \
    ../backends/CertGenerator.h \
    ../importers/NsisImporter.h \
    ../importers/InnoSetupImporter.h

# ── Windows resource file (icon + version info) ───────────────────────────────
win32: RC_FILE = ../windows/res/Mcaster1InstallStudio.rc

# ── macOS: copy docs into bundle Resources/ ───────────────────────────────────
macx {
    DOCS_DIR.files = $$PWD/../docs
    DOCS_DIR.path  = Contents/Resources/docs
    QMAKE_BUNDLE_DATA += DOCS_DIR

    # Copy installer icon
    ICON = $$PWD/../resources/icons/mcaster1.icns
}

# ── Output directory ──────────────────────────────────────────────────────────
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../build/debug
} else {
    DESTDIR = $$PWD/../build/release
}

# ── Post-build: windeployqt (Windows only) ────────────────────────────────────
win32 {
    # Run windeployqt automatically after build
    # Finds Qt DLLs and copies them next to the executable
    QMAKE_POST_LINK += $$[QT_INSTALL_BINS]/windeployqt.exe $$shell_quote($$DESTDIR/$$TARGET.exe) $$escape_expand(\\n\\t)
}
