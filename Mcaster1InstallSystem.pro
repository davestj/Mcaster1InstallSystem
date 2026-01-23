# ─────────────────────────────────────────────────────────────────────────────
# Mcaster1InstallSystem.pro — Top-level Qt Creator workspace
# ─────────────────────────────────────────────────────────────────────────────
# Open this file in Qt Creator (File > Open File or Project…).
# It references both the Studio IDE and the Runtime Installer as sub-projects.
#
# Qt version: Qt6 (6.x MSVC2022 kit recommended on Windows)
# Platform:   Windows (x64), macOS (arm64/x86_64), Linux (x86_64)
#
# Build:
#   Qt Creator:  Open > select kit > Build All (Ctrl+Shift+B)
#   Command line:
#     qmake Mcaster1InstallSystem.pro && nmake      (Windows / MSVC)
#     qmake Mcaster1InstallSystem.pro && make -j8   (macOS / Linux)
# ─────────────────────────────────────────────────────────────────────────────

TEMPLATE = subdirs
CONFIG  += ordered

SUBDIRS  = studio runtime cli

studio.file   = studio/Mcaster1InstallStudio.pro
runtime.file  = runtime/Mcaster1Installer.pro
cli.file      = cli/miscc.pro

# CLI depends on nothing (manifest + backends compiled inline)
# runtime depends on nothing (manifest is compiled inline in both)
