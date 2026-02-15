# CLAUDE.md — Mcaster1 Install System

> **Last updated:** 2026-03-03
> **Version:** 1.0.0
> **Active branch:** `main`
> **Next phase:** Phase 10 — Windows & Linux native builds + CI/CD matrix

This is the authoritative context document for Claude Code sessions working on this project.
Read it at the start of every session.

---

## What This Project Is

**Mcaster1 Install System** is a standalone cross-platform installer authoring suite —
an InstallShield / InstallAnywhere equivalent built with Qt6. It has three binaries:

| Binary | Purpose |
|--------|---------|
| `Mcaster1InstallStudio.app` | IDE for creating and building `.mis` installer projects |
| `Mcaster1Installer.app/.exe` | Runtime wizard end-users run to install an application |
| `miscc` | Headless CLI compiler — builds installer packages from `.mis` files |

**NOT** an installer for Mcaster1DNAS — this is a general-purpose tool for any application.

---

## Repository Structure

```
Mcaster1InstallSystem/
  VERSION                                    # 1.0.0
  CLAUDE.md                                  # This file
  CHANGELOG.md                               # All releases
  README.md                                  # End-user documentation
  docs/index.html                            # In-app help (opened from Help menu)

  manifest/          Manifest.h + Manifest.cpp  (.mis YAML data model — no libyaml)
  backends/          MacOsBackend / WindowsBackend / LinuxBackend / CodeSigner / CertGenerator
  importers/         NsisImporter / InnoSetupImporter  (import only — never invokes compiler)

  studio/            Qt6 IDE (Mcaster1InstallStudio.app)
    CMakeLists.txt   Qt6 Widgets+Svg+SvgWidgets+Concurrent
    StudioMainWindow.h/cpp   Multi-project IDE + 9-tab editor + sidebar + dock log + status bar
    ProjectSidebar           VS-style tree: projects + app groups; right-click context menus
    AppInfoEditor            App name/version/publisher/icon/license/defaults/theme/targets
    FilesEditor              Component + file assignment table
    ComponentsEditor         Dependency flags, required/optional
    ShortcutsEditor          Desktop / Start Menu shortcuts
    RegistryEditor           Windows registry entries (HKLM/HKCU, REG_SZ/DWORD/EXPAND_SZ)
    SecurityEditor           Signing credentials (macOS / Windows / Linux)
    PrerequisitesEditor      Prerequisite check commands per platform
    CustomActionsEditor      Before/after install/uninstall custom commands
    BuildPanel               Platform checkboxes + signing status + build queue + Test Installer
    BuilderProfile.h         Per-user company/signing profiles (JSON, AppConfigLocation)
    BuilderProfileDialog     Edit profile: identity + Signing Mode (skip / ad-hoc / identity)
    CodeSignDialog           Full code-signing dialog (sign + notarize + certs)
    HelpPanel                In-app docs viewer (QTextBrowser)
    EventLog.h               Timestamped color-coded dock log (Info/Build/Warn/Error/Success)
    BuildHistory.h           Persistent JSON dock log of every build
    SvgIcons.h               32 inline SVG action icons (24×24 dark theme)
    StudioStyle.h            Qt dark + enterprise stylesheets + ThemeManager

  runtime/           Qt6 Installer Wizard (Mcaster1Installer.app)
    InstallerWizard  QWizard hub (8 pages; PageId 0-7)
    WelcomePage / LicensePage / PrerequisitesPage / ComponentsPage /
    DirectoryPage / ReadyPage / InstallPage / FinishPage
    InstallEngine    QThread: file copy, registry, shortcuts, custom actions, uninstall manifest

  cli/               Headless CLI compiler (miscc)
    miscc.cpp        QCoreApplication only (no GUI); 12 flags; ANSI colour output
    CMakeLists.txt   Qt6 Core + Concurrent only
    build/miscc      Compiled binary (macOS arm64)

  resources/
    icons/mcaster1.svg    Canonical app icon (SVG)
    icons/mcaster1.icns   Built from SVG by rsvg-convert+iconutil at cmake time

  examples/
    AcmeWidgets/          Full cross-platform example with real payload structure
    SimpleNotepad/        Minimal 2-platform starter example
    DevSuite/             Enterprise 3-app-group example (full signing, all platforms)
    StreamServer/         Media server daemon + GUI pattern (macOS + Linux)
```

---

## Build Commands

### Prerequisites (macOS)
```bash
brew install qt librsvg
```

### Studio IDE
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

### miscc CLI
```bash
cmake -B cli/build -S cli -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Release
cmake --build cli/build -j$(sysctl -n hw.logicalcpu)

# All flags:
cli/build/miscc --help
cli/build/miscc --version
cli/build/miscc --list-profiles
cli/build/miscc -f <file.mis> --list-platforms
cli/build/miscc -f <file.mis> --dry-run --verbose --no-sign
cli/build/miscc -f <file.mis> --platform=macos --output-dir=~/dist
cli/build/miscc -f <file.mis> --platform=windows --no-sign
cli/build/miscc -f <file.mis> --profile="My Profile" --notarize
cli/build/miscc -f <file.mis> --signing-id="Developer ID Application: ACME (XXXXX)"
cli/build/miscc -f <file.mis> --pfx=path/to/cert.pfx
```

### One-liner rebuild + launch
```bash
cmake --build studio/build -j$(sysctl -n hw.logicalcpu) && \
codesign --force --sign - studio/build/Mcaster1InstallStudio.app && \
open studio/build/Mcaster1InstallStudio.app
```

---

## .mis File Format (YAML)

See `examples/` for full annotated examples. Key sections:

```yaml
format: mis/1
app:            # name, version, publisher, identifier, icon, license, url, support-url
defaults:       # install-dir (per platform), require-admin, allow-custom-dir, launch-after
theme:          # accent-color, background, dark-mode, banner-image, sidepanel-image
signing:        # macos-signer, macos-team-id, macos-notarize, win-pfx-path, linux-gpg-key
app-groups:     # id + name groupings for multi-app projects
prerequisites:  # id, name, check (shell cmd → "ok"), install (URL or cmd), platforms
components:     # id, name, required, selected, app-group, depends, files[]
shortcuts:      # name, target, type (app|url)
registry:       # hive, key, value, data, type — Windows only
custom-actions: # id, trigger (before/after-install/uninstall), type (shell), command, platforms
targets:        # [macos, windows, linux]
```

Token substitution in paths: `{install-dir}`, `{name}`, `{version}`, `{publisher}`

---

## StudioMainWindow Tab Order
```
0=AppInfo | 1=Files | 2=Components | 3=Shortcuts | 4=Registry
5=Security | 6=Prerequisites | 7=Actions | 8=Build
```

---

## Multi-Project Session Model

```
m_openProjects  QList<OpenProject>  — all projects held in memory simultaneously
m_activeIdx     int                 — which is shown in editors
active()        OpenProject&        — returns m_openProjects[m_activeIdx]
autoSaveActive()                    — saves dirty active project silently
switchToProject(int)                — auto-saves then changes active index
```

`OpenProject` fields: `manifest`, `path`, `dirty`, `tempId`, `activeAppGroupId`

---

## BuildPanel Wiring (IMPORTANT)

The Build button inside the panel emits `buildRequested()` — **never** calls
`onBuildClicked()` directly. `StudioMainWindow` connects `buildRequested()` to
`onBuildStart()`, which calls `collectEditors()` before `startBuild()`. This
ensures editor values are always flushed into the manifest before validation.

Signing overrides (from `BuilderProfile`) are applied to the manifest **copy**
inside `onBuildClicked()` at build-time — the `.mis` file on disk is never mutated.

---

## WindowsBackend — No NSIS, No External Compiler

`WindowsBackend` is our own native packager. It does NOT invoke `makensis.exe`,
`iscc.exe`, or any third-party tool. Output: `<name>-win64-setup.zip` containing:
- `manifest.mis` — project manifest
- `payload/` — application files
- `Mcaster1Installer.exe` — bundled runtime (if found in runtime/build/)
- `LICENSE.txt` — if declared in manifest

The importer (`NsisImporter`, `InnoSetupImporter`) converts `.nsi`/`.iss` scripts
**into our format** for editing. No import path ever invokes the original compiler.

---

## InstallerWizard Page IDs
```cpp
enum PageId { Welcome=0, License=1, Prerequisites=2, Components=3,
              Directory=4, Ready=5, Installing=6, Finish=7 };
// Prerequisites page only registered when manifest.prerequisites is non-empty
```

---

## Key Patterns

### si() helper (file-local SVG → QIcon)
```cpp
static QIcon si(const char *svg, int sz = 16) {
    QByteArray d(svg); QSvgRenderer r(d);
    QPixmap px(sz, sz); px.fill(Qt::transparent);
    QPainter p(&px); r.render(&p); return QIcon(px);
}
```

### Flush-on-navigate (split-panel editors)
`flushCurrentRow()` is called at the START of `onRowSelected()` before switching data.
`save() const` calls `const_cast<T*>(this)->flushCurrentRow()` before collecting.

### BuildPanel backend queue
`startBuild()` sets `m_manifest` + calls `onBuildClicked()`.
`onBuildClicked()` applies profile signing overrides → assembles `QList<BuildBackend*>`.
`launchNextBackend()` starts `BuildWorker` on `QThread`.
`onWorkerFinished()` advances queue; `cleanupQueue()` deletes all backends.

---

## Phase Progress Summary

| Phase | Feature | Status |
|-------|---------|--------|
| 1 | Build system, Studio core, Runtime QWizard base | ✅ COMPLETE |
| 2 | NSIS + Inno Setup importers | ✅ COMPLETE (stubs wired) |
| 3 | Full Runtime installer wizard (8 pages + InstallEngine) | ✅ COMPLETE |
| 4 | CodeSignDialog (macOS / Windows / Linux signing) | ✅ COMPLETE |
| 5 | Build backends (MacOs / Windows / Linux) + BuildPanel | ✅ COMPLETE |
| 6 | SVG icon system (32 action icons) + Test Installer | ✅ COMPLETE |
| 7 | PrerequisitesEditor, CustomActionsEditor, PrerequisitesPage | ✅ COMPLETE |
| 8 | EventLog dock, Build History, Tooltips, Status Bar Clock | ✅ COMPLETE |
| 9 | HelpPanel dock, docs/index.html, install dir hinting | ✅ COMPLETE |
| Pre-9 | Multi-project sidebar, inline rename, dev signing | ✅ COMPLETE |
| Pre-9 | WindowsBackend rewrite — native packager (no NSIS) | ✅ COMPLETE |
| Pre-9 | miscc CLI — all 12 flags, dry-run, profile support | ✅ COMPLETE |
| Pre-9 | 3 new example manifests (SimpleNotepad, DevSuite, StreamServer) | ✅ COMPLETE |
| **10** | **Windows & Linux native builds + CI/CD GitHub Actions** | **PLANNED** |

---

## Phase 10 — Windows & Linux Builds (PLANNED NEXT)

### Windows build environment
- Visual Studio 2022 (MSVC) + Qt6 MSVC build
- `cmake -B studio/build-win -S studio -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64`
- Output: `studio/build-win/Release/Mcaster1InstallStudio.exe`
- miscc CLI: `cmake -B cli/build-win -S cli` → `cli/build-win/Release/miscc.exe`

### Linux build environment
- Ubuntu 22.04 LTS (primary CI target)
- `cmake -B studio/build-linux -S studio -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6`
- miscc CLI: `cmake -B cli/build-linux -S cli` → `cli/build-linux/miscc`

### CI/CD (GitHub Actions matrix)
- `jobs.build.strategy.matrix.os: [macos-latest, ubuntu-22.04, windows-2022]`
- Artifact upload per platform
- AcmeWidgets example dry-run validated on each OS

---

## Common Gotchas

- **clangd false positives** — cross-project LSP confusion causes phantom "unused include"
  warnings in backend files. All confirmed spurious; builds succeed cleanly.
- **`_encode/_decode: command not found`** in cmake output — benign shell completion
  artifact from the user's shell environment; does not affect build.
- **codesign required on macOS after every build** — macOS caches the code directory;
  stale cache causes SIGKILL on launch. Always run `codesign --force --sign -` before `open`.
- **Build button must go through `onBuildStart()`** — do NOT connect the Build button
  directly to `onBuildClicked()`. The `buildRequested()` signal → `onBuildStart()` path
  is the only one that calls `collectEditors()` first.
- **WindowsBackend never calls makensis** — the `.nsi` importer is read-only. Any code
  that shells out to `makensis`, `iscc`, or similar must be removed immediately.
