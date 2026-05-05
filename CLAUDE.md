# CLAUDE.md — Mcaster1 Install System

> **Last updated:** 2026-03-04
> **Version:** 1.0.0
> **Active branch:** `windows-dev` (master = macOS only)
> **Next phase:** Phase 10 — Linux native builds + CI/CD GitHub Actions matrix

This is the authoritative context document for Claude Code sessions working on this project.
Read it at the start of every session.

---

## What This Project Is

**Mcaster1 Install System** is a standalone cross-platform installer authoring suite —
an InstallShield / InstallAnywhere equivalent built with Qt6. It has three binaries:

| Binary | Purpose |
|--------|---------|
| `Mcaster1InstallStudio.exe/.app` | IDE for creating and building `.mis` installer projects |
| `Mcaster1Installer.exe/.app` | Runtime wizard end-users run to install an application |
| `miscc` / `miscc.exe` | Headless CLI compiler — builds installer packages from `.mis` files |

**NOT** an installer for Mcaster1DNAS — this is a general-purpose tool for any application.

---

## Repository Structure

```
Mcaster1InstallSystem/
  VERSION                                    # 1.0.0
  CLAUDE.md                                  # This file
  CHANGELOG.md                               # All releases
  README.md                                  # End-user documentation
  LICENSE.md                                 # MIT license
  docs/index.html                            # In-app help (opened from Help menu)

  manifest/          Manifest.h + Manifest.cpp  (.mis YAML data model — no libyaml)
  backends/          MacOsBackend / WindowsBackend / LinuxBackend / CodeSigner / CertGenerator
  importers/         NsisImporter / InnoSetupImporter  (import only — never invokes compiler)

  studio/            Qt6 IDE
    CMakeLists.txt   Qt6 Widgets+Svg+SvgWidgets+Concurrent; WIN32 RC + /MANIFESTINPUT: on Windows
    StudioMainWindow.h/cpp   Multi-project IDE + 9-tab editor + sidebar + dock log + status bar
    ProjectSidebar           VS-style tree: projects + app groups; right-click context menus
    AppInfoEditor / FilesEditor / ComponentsEditor / ShortcutsEditor / RegistryEditor
    SecurityEditor / PrerequisitesEditor / CustomActionsEditor / BuildPanel
    BuilderProfile.h / BuilderProfileDialog / CodeSignDialog / HelpPanel
    EventLog.h / BuildHistory.h / SvgIcons.h / StudioStyle.h

  runtime/           Qt6 Installer Wizard
    CMakeLists.txt   WIN32 RC + /MANIFESTUAC:requireAdministrator + /MANIFESTINPUT: on Windows
    InstallerWizard  QWizard hub (8 pages; PageId 0-7)
    InstallEngine    QThread: file copy, registry, shortcuts, custom actions, uninstall manifest

  cli/               Headless CLI compiler (miscc / miscc.exe)
    miscc.cpp        QCoreApplication only (no GUI); 12 flags; ANSI colour output
    CMakeLists.txt   Qt6 Core + Concurrent only; windeployqt post-build on Windows

  resources/
    icons/mcaster1.svg    Canonical app icon (SVG)
    icons/mcaster1.icns   macOS icon (rsvg-convert+iconutil at cmake time)
    icons/mcaster1.ico    Windows icon (7-size ICO: 16,24,32,48,64,128,256 — Pillow-generated)
    icons/png/            Individual PNGs at all sizes

  windows/
    Mcaster1InstallSystem.sln      VS2022 solution (3 projects — CMake preferred)
    res/Mcaster1InstallStudio.rc   Version info + icon; manifest via /MANIFESTINPUT:
    res/Mcaster1InstallStudio.manifest  Windows 11 compat + PerMonitorV2 DPI + asInvoker UAC
    res/Mcaster1Installer.rc       Version info + icon; manifest via /MANIFESTINPUT: + /MANIFESTUAC:
    res/Mcaster1Installer.manifest Windows 11 compat + PerMonitorV2 DPI (UAC via linker flag)
    res/mcaster1.ico               Copy of resources/icons/mcaster1.ico for RC compiler
    props/Qt6.props                Qt6 auto-detection (6.10.2, 6.9.3, 6.9.1, 6.8.3, 6.7.3)
    props/Common.props             MSVC flags: C++17, _WIN32_WINNT=0x0A00 (Windows 11 target)

  scripts/
    Mcaster1InstallStudio.mis      Self-installer manifest (dogfood build)
    stage-payload.ps1              Stages all build outputs into scripts/payload/ with windeployqt
    generate-icons.py              Renders mcaster1.svg to ICO+PNGs using Pillow (no Cairo needed)
    payload/                       Staged payload dir (gitignored — regenerate with stage-payload.ps1)

  examples/
    AcmeWidgets/      Full cross-platform example with real payload structure (min OS: Windows 11)
    SimpleNotepad/    Minimal 2-platform starter example
    DevSuite/         Enterprise 3-app-group example (full signing, all platforms)
    StreamServer/     Media server daemon + GUI pattern (macOS + Linux)
```

---

## Build Commands — Windows

```bash
# Prerequisites: Qt 6.9.3 at C:\Qt\6.9.3\msvc2022_64, VS2022 Professional, CMake 4.2.3

# CLI (miscc.exe)
cmake -B cli/build-win -S cli -DCMAKE_PREFIX_PATH="C:/Qt/6.9.3/msvc2022_64" -G "Visual Studio 17 2022" -A x64
cmake --build cli/build-win --config Debug

# Studio IDE
cmake -B studio/build-win -S studio -DCMAKE_PREFIX_PATH="C:/Qt/6.9.3/msvc2022_64" -G "Visual Studio 17 2022" -A x64
cmake --build studio/build-win --config Debug

# Runtime Installer
cmake -B runtime/build-win -S runtime -DCMAKE_PREFIX_PATH="C:/Qt/6.9.3/msvc2022_64" -G "Visual Studio 17 2022" -A x64
cmake --build runtime/build-win --config Debug
```

Outputs: `*/build-win/Debug/*.exe` — windeployqt runs automatically post-build.

### Build Self-Installer (Windows)
```bash
# Stage payload (run from project root in PowerShell or Git Bash)
powershell -File scripts/stage-payload.ps1

# Build installer package
cli/build-win/Debug/miscc.exe -f scripts/Mcaster1InstallStudio.mis --platform=windows --no-sign --output-dir=dist

# Sign all four EXEs with self-signed PFX
SIGNTOOL="/c/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/signtool.exe"
PFX="C:/Users/dstjohn/codesigning/mcaster1/installstudio/mcaster1-installstudio.pfx"
PASS="Mcaster1Dev2026!"
for exe in dist/*/Mcaster1Installer.exe dist/*/payload/{Mcaster1InstallStudio,Mcaster1Installer,miscc}.exe; do
  "$SIGNTOOL" sign -fd sha256 -f "$PFX" -p "$PASS" -tr "http://timestamp.sectigo.com" -td sha256 "$exe"
done
```

---

## Build Commands — macOS

```bash
brew install qt librsvg

cmake -B studio/build -S studio -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Debug
cmake --build studio/build -j$(sysctl -n hw.logicalcpu)
codesign --force --sign - studio/build/Mcaster1InstallStudio.app
open studio/build/Mcaster1InstallStudio.app

cmake -B runtime/build -S runtime -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Debug
cmake --build runtime/build -j$(sysctl -n hw.logicalcpu)
codesign --force --sign - runtime/build/Mcaster1Installer.app

cmake -B cli/build -S cli -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Release
cmake --build cli/build -j$(sysctl -n hw.logicalcpu)
```

---

## .mis File Format (YAML)

See `examples/` for full annotated examples. Key sections:

```yaml
format: mis/1
app:            # name, version, publisher, identifier, icon, license, output-name, url, support-url
defaults:       # install-dir (per platform), require-admin, allow-custom-dir, launch-after
theme:          # accent-color, background, dark-mode, banner-image, sidepanel-image
signing:        # macos-signer, macos-team-id, macos-notarize, win-pfx-path, win-timestamp, linux-gpg-key
app-groups:     # id + name groupings for multi-app projects
prerequisites:  # id, name, check (shell cmd → "ok"), install (URL or cmd), platforms
components:     # id, name, required, selected, app-group, depends, files[]
shortcuts:      # name, target, type (app|url)
registry:       # hive, key, value, data, type — Windows only
custom-actions: # id, trigger (before/after-install/uninstall), type (shell), command, platforms
targets:        # block-style list only — see gotchas
```

Token substitution in paths: `{install-dir}`, `{name}`, `{version}`, `{publisher}`

### output-name field (AppInfo)
Optional override for the generated installer filename stem:
```yaml
app:
  output-name: "MyApp-Setup"   # → MyApp-Setup-win64.zip / MyApp-Setup-macOS-arm64.dmg
```
Omit to use the default: `<Publisher>-<Name>-<Version>-<platform>`.

### Windows paths in .mis — use single-quoted YAML
Single-quoted YAML strings treat `\` as literal — no escaping needed:
```yaml
defaults:
  install-dir:
    windows: 'C:\Program Files\Mcaster1\MyApp'   # correct
    # NOT: "C:\\Program Files\\Mcaster1\\MyApp"   # ugly and error-prone
registry:
  - key: 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MyApp'
```

---

## YAML Parser Notes (IMPORTANT)

- **`targets:`** section ONLY parses block-style lists — `- windows` on separate lines.
  `targets: [windows]` inline format is **silently ignored**.
- **`win-timestamp`** not `win-timestamp-url` for the signing section key.
- **Double-quoted strings**: parser now correctly handles `\\` → `\`, `\"` → `"`, `\n` → newline.
- **Single-quoted strings**: `\` is literal, `''` → `'`. Preferred for Windows paths.
- **Serializer (toYaml/yq())**: values containing `\` are auto-emitted as single-quoted YAML.
- Validate with: `miscc.exe -f X.mis --dry-run --verbose --no-sign`
- Signing key name: `win-pfx-password` (not `win-pfx-pass`), `win-timestamp` (not `win-timestamp-url`)

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

## WindowsBackend

Native packager — does NOT invoke `makensis.exe`, `iscc.exe`, or any third-party tool.

Output structure in `<output-dir>/<name>-win64-setup/`:
- `manifest.mis` — project manifest (serialized with single-quoted Windows paths)
- `payload/` — application files copied from project `payload/`
- `Mcaster1Installer.exe` — bundled runtime (found via `findRuntimeExe()`)
- Qt DLLs at package root (deployed by `findWinDeployQt()` → windeployqt on runtime exe)
- `LICENSE.md` — if declared in manifest

**Runtime exe search order** (`findRuntimeExe()`): flat layout alongside studio/miscc →
`runtime/build-win/Debug/` → `runtime/build-win/Release/` → VS `windows/x64/` paths.

**Zip step**: calls `powershell.exe` directly (NOT via `cmd.exe /C`) to avoid nested-quote
mangling with `Compress-Archive`. Runs ~15s for a 150 MB payload.

---

## Windows App Manifests

Both `.manifest` files declare:
- Windows 11 compatibility GUID `{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}`
- PerMonitorV2 DPI awareness
- UAC level is set via linker flags (NOT in the manifest XML) to avoid mt.exe conflict:
  - Studio: `/MANIFESTUAC:` not set (defaults to asInvoker)
  - Installer: `/MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"`
- Both use `/MANIFESTINPUT:` to merge the custom manifest into the linker-generated one.

---

## Windows Code Signing

**Tool:** `signtool.exe` at `C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\`

**Self-signed cert:** `~/codesigning/mcaster1/installstudio/mcaster1-installstudio.pfx`
Password: `Mcaster1Dev2026!` · Valid to: 2036-03-01 · Thumbprint: `3BB088045454DC2B7A57B7F46789091E54459A54`

**Sign command:**
```bash
signtool sign -fd sha256 -f path/to.pfx -p <password> \
  -tr "http://timestamp.sectigo.com" -td sha256 Target.exe
```

**After install — fix Windows SmartScreen block:**
1. Import cert to Trusted Root CAs (one-time per machine):
   ```powershell
   $cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2
   $cert.Import('~/codesigning/mcaster1/installstudio/mcaster1-installstudio.crt')
   $store = New-Object System.Security.Cryptography.X509Certificates.X509Store('Root','LocalMachine')
   $store.Open('ReadWrite'); $store.Add($cert); $store.Close()
   ```
2. Remove Mark-of-the-Web from installed files:
   ```powershell
   # Save as C:\Temp\unblock.ps1 and run:
   Get-ChildItem -Recurse 'C:\Program Files\Mcaster1\Mcaster1InstallStudio' -Include '*.exe','*.dll' |
       ForEach-Object { Unblock-File $_.FullName }
   ```

**Note:** SmartScreen will always block self-signed certs on machines without the cert in Trusted Root.
For public distribution, purchase a commercial EV code signing cert (DigiCert, Sectigo, etc.).

**vcredist dependency:** Debug builds require Visual C++ 2022 Redistributable because
`vcruntime140d.dll` is not redistributable and windeployqt doesn't bundle debug CRT DLLs.
Release builds: windeployqt bundles `vcruntime140.dll` automatically.

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
| 10a | Windows builds (miscc.exe + Studio + Installer) — Debug confirmed | ✅ COMPLETE |
| 10a | Windows ICO icon, RC resource, app manifests (Win11 + DPI + UAC) | ✅ COMPLETE |
| 10a | Self-installer manifest + stage-payload.ps1 + dogfood build | ✅ COMPLETE |
| 10a | YAML parser fix (backslash escaping, single-quoted output) | ✅ COMPLETE |
| 10a | output-name field for custom installer filename | ✅ COMPLETE |
| 10a | Self-signed PFX cert + signtool signing + SmartScreen fix | ✅ COMPLETE |
| 10a | LICENSE.md (MIT) | ✅ COMPLETE |
| **10b** | **Linux native builds** | **PLANNED** |
| **10c** | **CI/CD GitHub Actions matrix** | **PLANNED** |

---

## Common Gotchas

- **clangd false positives** — cross-project LSP confusion causes phantom "unused include"
  warnings in backend files. All confirmed spurious; builds succeed cleanly.
- **codesign required on macOS after every build** — macOS caches the code directory;
  stale cache causes SIGKILL on launch. Always run `codesign --force --sign -` before `open`.
- **Build button must go through `onBuildStart()`** — do NOT connect the Build button
  directly to `onBuildClicked()`. The `buildRequested()` signal → `onBuildStart()` path
  is the only one that calls `collectEditors()` first.
- **WindowsBackend never calls makensis** — the `.nsi` importer is read-only. Any code
  that shells out to `makensis`, `iscc`, or similar must be removed immediately.
- **`targets:` must use block-style** — `targets: [windows]` is silently ignored by the parser.
- **Windows manifest conflict** — do NOT embed RT_MANIFEST (type 24) in the .rc file AND
  use `/MANIFESTINPUT:`. The linker auto-generates a manifest; use `/MANIFESTINPUT:` to
  merge your custom one, and `/MANIFESTUAC:` for UAC level. Mixing both causes
  "duplicate resource" or mt.exe "level attribute mismatch" errors.
- **SmartScreen blocks self-signed EXEs** — on new machines, import the cert to
  Local Machine Trusted Root CAs and run `Unblock-File` on all installed EXEs/DLLs.
- **MOTW (Mark of the Web)** — zip files and their extracted contents get a Zone.Identifier
  ADS that Windows uses to block execution. `Unblock-File` removes it.
- **Debug CRT not redistributable** — windeployqt does NOT bundle `vcruntime140d.dll`.
  Debug builds require VS2022 or VC++ Redist on the target machine.
