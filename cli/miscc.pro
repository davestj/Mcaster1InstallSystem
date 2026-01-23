# ─────────────────────────────────────────────────────────────────────────────
# miscc.pro — Qt Creator project for the miscc CLI compiler
# ─────────────────────────────────────────────────────────────────────────────
# Builds a headless CLI binary (no GUI, no .app bundle).
# QCoreApplication only — links Qt6::Core + Qt6::Concurrent.
# ─────────────────────────────────────────────────────────────────────────────

QT       -= gui
QT       += core concurrent

CONFIG   += c++17 console
CONFIG   -= app_bundle

TARGET   = miscc

SOURCES += miscc.cpp

# Manifest library (compiled inline)
SOURCES += $$PWD/../manifest/Manifest.cpp

# Platform backends
SOURCES += $$PWD/../backends/MacOsBackend.cpp
SOURCES += $$PWD/../backends/WindowsBackend.cpp
SOURCES += $$PWD/../backends/LinuxBackend.cpp
SOURCES += $$PWD/../backends/CodeSigner.cpp
SOURCES += $$PWD/../backends/CertGenerator.cpp

HEADERS += $$PWD/../manifest/Manifest.h
HEADERS += $$PWD/../studio/BuilderProfile.h
HEADERS += $$PWD/../backends/BuildBackend.h
HEADERS += $$PWD/../backends/MacOsBackend.h
HEADERS += $$PWD/../backends/WindowsBackend.h
HEADERS += $$PWD/../backends/LinuxBackend.h
HEADERS += $$PWD/../backends/CodeSigner.h
HEADERS += $$PWD/../backends/CertGenerator.h

INCLUDEPATH += $$PWD/../manifest
INCLUDEPATH += $$PWD/../studio
INCLUDEPATH += $$PWD/../backends

# ── Platform-specific install ─────────────────────────────────────────────────
unix {
    target.path = /usr/local/bin
    INSTALLS += target
}

# ── Windows: deploy Qt DLLs after build ──────────────────────────────────────
win32 {
    QMAKE_POST_LINK += $$[QT_INSTALL_BINS]/windeployqt.exe \
        --no-translations \
        --no-system-d3d-compiler \
        --no-quick-import \
        $$OUT_PWD/release/$${TARGET}.exe $$escape_expand(\\n\\t)
}
