<div align="center">

<img src="resources/icons/mcaster1.svg" alt="Mcaster1 Install Studio" width="96" height="96"/>

# Mcaster1 Install Studio

**Cross-platform installer authoring suite for macOS, Windows, and Linux.**

[![Version](https://img.shields.io/badge/version-1.0.0-blue?style=flat-square)](https://github.com/davestj/mcaster1-install-system/releases)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey?style=flat-square)](#build)
[![Qt6](https://img.shields.io/badge/Qt-6.x-41CD52?style=flat-square&logo=qt)](https://www.qt.io)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen?style=flat-square)](#build)

Create professional `.dmg`, `.exe (NSIS)`, and `.deb` / AppImage installers
from a single **YAML project file** — no scripting required.

---

</div>

## Overview

Mcaster1 Install Studio is a standalone, **InstallShield-style** GUI authoring tool that
compiles installer projects for all three major platforms from one place. It ships two
applications:

| App | Purpose |
|-----|---------|
| **Mcaster1InstallStudio** | Full IDE — create, edit, and build installer projects |
| **Mcaster1Installer** | Runtime wizard — the installer your end-users run |

---

## Screenshots

> _Coming soon_ — Studio IDE, Runtime Wizard, and Dark-Mode Builder Profiles.

---

## Features

- **Single YAML project file** (`.mis` — Mcaster1 Install Spec) defines everything
- **Multi-platform build queue** — build macOS + Windows + Linux in sequence from one click
- **Builder Profiles** — company identity, signing certificates, and contact info saved as reusable profiles
- **macOS DMG builder** — Homebrew artifact baking, `codesign`, optional `notarize` via `xcrun notarytool`
- **Windows NSIS generator** — produces a self-contained NSIS installer script + optional `makensis` call
- **Linux `.deb` + AppImage** — generates `control` file, `install` script, and AppDir skeleton
- **8-tab Studio IDE** — App Info, Files, Components, Shortcuts, Registry, Security, Prerequisites, Custom Actions
- **Dark-mode UI** — fully themed Qt6 dark palette with SVG icon set (32 icons)
- **EventLog dock** — timestamped, color-coded build output (Info / Warn / Error / Success)
- **BuildHistory dock** — persistent JSON log of every build with output path and sign status
- **Code Signing Manager** — macOS (ad-hoc / Developer ID / notarize), Windows (PFX Authenticode), Linux (GPG)
- **Certificate Generator** — creates self-signed certs and CSRs via OpenSSL
- **NSIS + Inno Setup importer** — import existing Windows installers as starting points
- **Test Installer button** — launch the runtime wizard against a temp manifest directly from Studio

---

## Project File Format (`.mis`)

```yaml
format: mis/1

app:
  name:        "My Application"
  version:     "2.0.0"
  publisher:   "ACME Corp"
  identifier:  "com.acme.myapp"

defaults:
  install-dir:
    macos:   "/Applications/MyApp"
    windows: "C:\\Program Files\\ACME\\MyApp"
    linux:   "/opt/acme/myapp"

targets: [macos, windows, linux]

components:
  - id:       core
    name:     "Core Application"
    required: true
    files:
      - src: "MyApp.app"
        dst: "{install-dir}/MyApp.app"
        isDir: true

custom-actions:
  - id:      fix-permissions
    trigger: after-install
    type:    shell
    command: "chmod +x \"{install-dir}/MyApp.app/Contents/MacOS/MyApp\""
```

---

## Build

### Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| Qt6 | 6.6+ | Widgets, Svg, SvgWidgets, Concurrent |
| CMake | 3.20+ | Or use Qt Creator |
| C++17 compiler | Clang/MSVC/GCC | |
| macOS build tools | Homebrew | See [README-MACOS-BUILD](windows/README-WINDOWS-BUILD.md) |

### macOS / Linux (CMake)

```bash
# Studio IDE
cmake -B studio/build -S studio \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt) \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build studio/build -j$(sysctl -n hw.logicalcpu)
codesign --force --sign - studio/build/Mcaster1InstallStudio.app
open studio/build/Mcaster1InstallStudio.app

# Runtime Installer
cmake -B runtime/build -S runtime \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt) \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build runtime/build -j$(sysctl -n hw.logicalcpu)
```

### Windows (Qt Creator — Recommended)

```
1. Open Qt Creator
2. File > Open File or Project > Mcaster1InstallSystem.pro
3. Select MSVC 2022 x64 kit  (Qt at C:\Qt)
4. Build > Build All Projects  (Ctrl+Shift+B)
```

Or use **Visual Studio 2022**:
```
windows/Mcaster1InstallSystem.sln
```
See [`windows/README-WINDOWS-BUILD.md`](windows/README-WINDOWS-BUILD.md) for full instructions.

---

## Project Structure

```
Mcaster1InstallSystem/
  studio/                   Studio IDE (Qt6 Widgets)
    CMakeLists.txt
    Mcaster1InstallStudio.pro  (Qt Creator)
    StudioMainWindow.*         Main window + menus + HUD
    AppInfoEditor.*            App Info, Defaults, Theme, Targets
    FilesEditor.*              Component + file tree
    BuildPanel.*               Build queue, platform checkboxes
    BuilderProfile.*           Per-user company/signing profiles
    EventLog.h                 Header-only event log dock widget
    BuildHistory.h             Header-only build history dock
    SvgIcons.h                 32 inline SVG icons

  runtime/                  Installer wizard (Qt6 Widgets)
    CMakeLists.txt
    Mcaster1Installer.pro      (Qt Creator)
    InstallerWizard.*          7-page QWizard
    InstallEngine.*            File copy, shortcuts, custom actions

  backends/                 Platform build backends
    MacOsBackend.*             DMG builder (Homebrew baking, codesign, hdiutil)
    WindowsBackend.*           NSIS script generator
    LinuxBackend.*             .deb + AppImage skeleton

  manifest/                 Data model + YAML serializer
    Manifest.h                 All structs: AppInfo, Component, FileEntry…
    Manifest.cpp               YAML parser/writer (no libyaml dependency)

  importers/                Importer plugins
    NsisImporter.*             .nsi → Manifest (regex-based)
    InnoSetupImporter.*        .iss → Manifest

  scripts/                  Payload assembly scripts
    mcaster1/Mcaster1DNAS/     Example: Mcaster1DNAS macOS installer spec
      mcaster1dnas-macos.mis
      assemble-payload.sh

  windows/                  Windows build files
    Mcaster1InstallSystem.sln
    Mcaster1InstallStudio.vcxproj
    Mcaster1Installer.vcxproj
    props/Qt6.props
    props/Common.props
    README-WINDOWS-BUILD.md
```

---

## License

MIT License — see [LICENSE](LICENSE).

---

<div align="center">

Built with Qt6 · Made by [Mcaster1](https://mcaster1.com)

</div>
