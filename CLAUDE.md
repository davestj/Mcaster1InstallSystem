# CLAUDE.md — Mcaster1 Install System

> **Last updated:** 2026-03-03
> **Version:** 1.0.0
> **Active branch:** `main`
> **Next phase:** Phase 10 — Windows & Linux native builds + CLI miscc cross-platform

This is the authoritative context document for Claude Code sessions working on this project.
Read it at the start of every session. Full details are in `memory/installsystem.md`.

---

## What This Project Is

**Mcaster1 Install System** is a standalone cross-platform installer authoring suite — an
InstallShield / InstallAnywhere equivalent built with Qt6. It has two binaries:

| Binary | Purpose |
|--------|---------|
| `Mcaster1InstallStudio.app` | IDE for creating `.mis` installer spec files |
| `Mcaster1Installer.app`     | Runtime wizard that reads a `.mis` file and installs an application |

**NOT** an installer for Mcaster1DNAS — it is a general-purpose tool.

---

## Repository Structure

```
Mcaster1InstallSystem/
  autogen.sh / configure.ac / Makefile.am   # Autotools pipeline
  VERSION                                    # 1.0.0
  CLAUDE.md                                  # This file
  README.md                                  # End-user documentation
  docs/index.html                            # In-app help (opened from Help menu)

  manifest/          Manifest.h + Manifest.cpp  (.mis YAML data model)
  backends/          MacOsBackend / WindowsBackend / LinuxBackend / CodeSigner / CertGenerator
  importers/         NsisImporter / InnoSetupImporter

  studio/            Qt6 IDE (Mcaster1InstallStudio.app)
    CMakeLists.txt   Qt6 Widgets+Svg+SvgWidgets+Concurrent
    main.cpp         QApplication + buildAppIcon() + dark theme
    StudioMainWindow.h/cpp   9-tab IDE + sidebar + dock log + status bar
    AppInfoEditor            App name/version/publisher/icon/license/defaults/theme
    FilesEditor              Component + file assignment table
    ComponentsEditor         Dependency flags
    ShortcutsEditor          Desktop / Start Menu shortcuts
    RegistryEditor           Windows registry entries
    SecurityEditor           Signing credentials (macOS / Windows / Linux)
    PrerequisitesEditor      Prerequisite check commands
    CustomActionsEditor      Before/after install/uninstall custom commands
    BuildPanel               Platform checkboxes + build queue + Test Installer
    CodeSignDialog           Full code-signing dialog (sign + notarize + certs)
    SvgIcons.h               25 inline SVG action icons (all 24×24 dark theme)
    StudioStyle.h            Qt dark stylesheet

  runtime/           Qt6 Installer Wizard (Mcaster1Installer.app)
    CMakeLists.txt   Qt6 Widgets+Svg+SvgWidgets
    InstallerWizard  QWizard hub (PageId enum 0-7)
    WelcomePage / LicensePage / PrerequisitesPage / ComponentsPage /
    DirectoryPage / ReadyPage / InstallPage / FinishPage
    InstallEngine    QThread: file copy, registry, shortcuts, custom actions, uninstall manifest

  resources/
    icons/mcaster1.svg    Canonical app icon (SVG, 512 viewBox 48×48)
    icons/mcaster1.icns   Built from SVG by rsvg-convert+iconutil at cmake time
```

---

## Build Commands

### Prerequisites (macOS)
```bash
brew install qt librsvg
```

### Studio (IDE)
```bash
cmake -B studio/build -S studio \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt) \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build studio/build -j$(sysctl -n hw.logicalcpu)
codesign --force --sign - studio/build/Mcaster1InstallStudio.app
open studio/build/Mcaster1InstallStudio.app
```

### Runtime Installer
```bash
cmake -B runtime/build -S runtime \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt) \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build runtime/build -j$(sysctl -n hw.logicalcpu)
codesign --force --sign - runtime/build/Mcaster1Installer.app
```

### miscc CLI (headless compiler — macOS/Linux)
```bash
cmake -B cli/build -S cli -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Release
cmake --build cli/build -j$(sysctl -n hw.logicalcpu)
# Binary: cli/build/miscc

# Usage:
cli/build/miscc --build-file=examples/AcmeWidgets/AcmeWidgets.mis --platform=macos
cli/build/miscc --build-file=examples/AcmeWidgets/AcmeWidgets.mis --platform=linux
cli/build/miscc --build-file=examples/AcmeWidgets/AcmeWidgets.mis --platform=windows
cli/build/miscc --build-file=examples/AcmeWidgets/AcmeWidgets.mis --dry-run --verbose
```

### miscc CLI (Windows — Phase 10)
```powershell
cmake -B cli\build-win -S cli -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build cli\build-win --config Release
# Binary: cli\build-win\Release\miscc.exe
```

### miscc CLI (Linux — Phase 10)
```bash
cmake -B cli/build-linux -S cli -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake --build cli/build-linux -j$(nproc)
# Binary: cli/build-linux/miscc
```

### One-liner rebuild (incremental, macOS)
```bash
cmake --build studio/build -j$(sysctl -n hw.logicalcpu) && \
codesign --force --sign - studio/build/Mcaster1InstallStudio.app && \
open studio/build/Mcaster1InstallStudio.app
```

---

## .mis File Format (YAML)

```yaml
format: mis/1
app:
  name: "My Application"
  version: "1.0.0"
  publisher: "ACME Corp"
  identifier: "com.example.myapp"
  licenseFile: "payload/LICENSE.txt"

defaults:
  install-dir:
    macos:   "/Applications/ACME"
    windows: "C:\\Program Files\\ACME\\MyApp"
    linux:   "/opt/acme"
  require-admin: true
  allow-custom-dir: true
  launch-after: true

components:
  - id: core
    name: "Core Application"
    required: true
    files:
      - src: "payload/MyApp.app"
        dst: "{install-dir}/MyApp.app"

shortcuts:
  - name: "Launch My App"
    target: "{install-dir}/MyApp.app"
    type: app

prerequisites:
  - id: homebrew
    name: "Homebrew"
    checkCmd: "which brew"
    installCmd: "https://brew.sh"
    platforms: [macos]

custom-actions:
  - id: postinstall
    trigger: after-install
    type: shell
    command: "chmod +x {install-dir}/MyApp.app/Contents/MacOS/myapp"
    platforms: [macos]

targets:
  - macos
  - windows
  - linux
```

---

## StudioMainWindow Tab Order
```
0=AppInfo | 1=Files | 2=Components | 3=Shortcuts | 4=Registry
5=Security | 6=Prerequisites | 7=Actions | 8=Build
```

Navigation keys for `onNavigateTo()`: `appinfo`, `files`, `components`, `shortcuts`, `registry`, `security`, `prerequisites`, `actions`, `build`

---

## InstallerWizard Page IDs
```cpp
enum PageId { Welcome=0, License=1, Prerequisites=2, Components=3,
              Directory=4, Ready=5, Installing=6, Finish=7 };
// Prerequisites is only registered when manifest.prerequisites.isEmpty() == false
```

---

## Key Patterns

### si() helper (file-local SVG→QIcon)
```cpp
static QIcon si(const char *svg, int sz = 16) {
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}
```
Used in: FilesEditor, ShortcutsEditor, RegistryEditor, CodeSignDialog, BuildPanel,
         PrerequisitesEditor, CustomActionsEditor.
StudioMainWindow uses `svgIcon()` member instead (same logic).

### Flush-on-navigate (split-panel editors)
`flushCurrentRow()` is called at the START of `onRowSelected()` before switching data.
`save() const` calls `const_cast<T*>(this)->flushCurrentRow()` before collecting.

### BuildPanel backend queue
`onBuildClicked()` assembles `QList<BuildBackend*>` for checked platforms.
`launchNextBackend()` starts `BuildWorker` on `QThread`.
`onWorkerFinished()` advances queue index; `cleanupQueue()` deletes all backends.

### SVG → icns at cmake time
`find_program(RSVG_CONVERT rsvg-convert)` + `find_program(ICONUTIL iconutil)`.
`add_custom_command(OUTPUT mcaster1.icns)` renders 10 sizes and assembles .icns.
Falls back to pre-built `.icns` if `rsvg-convert` absent.

---

## Phase Progress Summary

| Phase | Feature | Status |
|-------|---------|--------|
| 1 | Build system, Studio core, Runtime QWizard base | COMPLETE |
| 2 | NSIS + Inno Setup importers | PARTIAL (stubs wired) |
| 3 | Full Runtime installer wizard (8 pages + InstallEngine) | COMPLETE |
| 4 | CodeSignDialog (macOS / Windows / Linux signing) | COMPLETE |
| 5 | Build backends (MacOs / Windows / Linux) + BuildPanel | COMPLETE |
| 6 | SVG icon system (app icns + 25 action icons) + Test Installer | COMPLETE |
| 7 | PrerequisitesEditor, CustomActionsEditor, PrerequisitesPage | COMPLETE |
| 8 | EventLog dock, Build History, Tooltips, Status Bar Clock | COMPLETE |
| 9 | Install directory hinting, docs/index.html, HelpPanel dock | COMPLETE |
| Pre-9 | Multi-project sidebar, inline rename, dev signing | COMPLETE |
| **10** | **Windows & Linux native builds + miscc CLI** | **PLANNED** |

**Example projects:** `examples/AcmeWidgets/` — full cross-platform example (.mis + bash/bat stubs +
macOS .pkg + Linux .deb + Windows NSIS). Built and verified on macOS 2026-03-03.

---

## Phase 10 — Windows & Linux Builds (PLANNED NEXT)

### Windows build environment
- Visual Studio 2022 (MSVC) + Qt6 6.x MSVC build
- `cmake -B studio/build-win -S studio -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64`
- `cmake --build studio/build-win --config Release`
- Output: `studio/build-win/Release/Mcaster1InstallStudio.exe`
- miscc CLI: `cmake -B cli/build-win -S cli` → `cli/build-win/Release/miscc.exe`

### Linux build environment
- Ubuntu 22.04 LTS (primary CI target)
- `sudo apt install qtbase5-dev qtsvg5-dev` OR aqtinstall Qt6
- `cmake -B studio/build-linux -S studio -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6`
- miscc CLI: `cmake -B cli/build-linux -S cli` → `cli/build-linux/miscc`

### Platform-specific runtime items
- **Windows**: UAC manifest (`requireAdministrator`), registry write via Win32 API, HKCU/HKLM support
- **Linux**: pkexec/sudo elevation detection, .desktop file creation, ldconfig, /opt install
- **macOS**: Already works — `pkgbuild` + `productbuild` backends functional

### CI/CD (GitHub Actions matrix)
- `jobs.build.strategy.matrix.os: [macos-latest, ubuntu-22.04, windows-2022]`
- Artifact upload per platform
- ACME Widgets example built on each OS in CI

---

## Common Gotchas

- **clangd false positives**: cross-project LSP confusion causes phantom "unused include" warnings.
  All were confirmed spurious — builds succeed cleanly.
- **`_encode/_decode: command not found`** in cmake output — benign shell completion artifact from
  the user's shell environment; does not affect build.
- **`WrapVulkanHeaders not found`** — harmless cmake miss; no Vulkan dependency.
- **codesign required on macOS after every build** — macOS caches the code directory; stale cache
  causes SIGKILL on launch. Always run `codesign --force --sign -` before `open`.
