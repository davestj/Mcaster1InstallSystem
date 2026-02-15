# Changelog

All notable changes to Mcaster1 Install Studio are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [1.0.0] — 2026-03-03

### Added

#### Studio IDE
- **Multi-project sidebar** — VS Solution Explorer-style tree; holds multiple `.mis` projects
  open simultaneously with one-click switching; active project is bold-highlighted
- **Inline project rename** — type a new name directly in the sidebar; 1 200 ms debounce
  auto-saves and renames the `.mis` file on disk; AppInfoEditor stays in sync via
  `QSignalBlocker` (no feedback loop)
- **Add Application Group button** — "＋" in the Applications section header; pops a dialog,
  creates a new `AppGroup` with a UUID-based id, reloads editors
- **Right-click context menus** on sidebar tree — rename, delete, open-in-finder, add file, add dir,
  add shortcut; all with keyboard accelerators
- **Auto-save timer** — 1 500 ms single-shot fires after any name or field change; saves to disk,
  renames file if app name changed, refreshes sidebar
- **9-tab editor** — App Info, Files, Components, Shortcuts, Registry, Security, Prerequisites,
  Custom Actions, Build
- **Status bar clock** — live HH:MM:SS clock at bottom-right
- **EventLog dock** — timestamped color-coded messages (Info / Build / Warn / Error / Success)
- **BuildHistory dock** — persistent JSON log of every build with output path and signing status
- **HelpPanel dock** — in-app `QTextBrowser` opens `docs/index.html`
- **Test Installer button** — launches runtime wizard against a temp manifest

#### Builder Profiles
- **Signing Mode group** at the top of the Code Signing tab in Builder Profile Dialog:
  - *Skip code signing entirely* — no signing on any platform; for internal testing only
  - *Use development (ad-hoc) signing* — `codesign --force --sign -` on macOS; skips
    `signtool.exe` on Windows; skips GPG signing on Linux
- Interlock logic: Skip disables all other signing controls; Dev Sign disables identity fields
- Profile JSON now persists `skipSigning`, `devSignMode`, `projectPaths` fields
- **Projects list** per profile — sidebar shows project paths saved to the active profile;
  `[✕]` removes from profile (file not deleted); click any row to switch

#### Build System — Native Windows Packager
- **`WindowsBackend` fully rewritten** as a standalone native packager; zero dependency on
  `makensis.exe`, `iscc.exe`, or any third-party installer compiler
- Output: `<Publisher>-<Name>-<Version>-win64-setup/` staged directory + `.zip` archive
- Bundles `manifest.mis`, `payload/`, `Mcaster1Installer.exe` (if found), and `LICENSE.txt`
- Cross-platform zip: POSIX `zip -r` on macOS / Linux; PowerShell `Compress-Archive` on Windows
- Runtime probe: 7 candidate paths for `Mcaster1Installer.exe`; missing is a non-fatal warning

#### Build System — macOS Backend
- DMG builder: `hdiutil create`, `codesign`, optional `xcrun notarytool` notarization
- Signing identity filled from Builder Profile if manifest `signing:` block is empty

#### Build System — Linux Backend
- `.deb` control file + install script generator
- AppImage skeleton
- GPG package signing (skipped in dev-sign mode)

#### miscc CLI Compiler
- Headless `QCoreApplication` CLI — builds installer packages without launching Studio
- 12 flags: `--help`, `--version`, `-f <file>`, `--platform`, `--output-dir`, `--dry-run`,
  `--verbose`, `--no-sign`, `--validate`, `--list-platforms`, `--list-profiles`, `--list-files`
- `--signing-id`, `--pfx`, `--notarize`, `--profile` override manifest signing at build time
- ANSI color output (auto-detected via `isatty()`); exit codes 0/1/2/3
- Dry-run validates manifest and prints file list without writing any output

#### Runtime Installer Wizard
- 8-page `QWizard`: Welcome → License → Prerequisites → Components → Directory →
  Ready → Installing → Finish
- `InstallEngine` QThread: file copy, registry writes, shortcuts, custom actions,
  uninstall manifest generation
- Prerequisites page registered only when manifest declares prerequisites (no empty page)

#### Importers (read-only converters)
- **NsisImporter** — converts `.nsi` NSIS scripts into `.mis` format for editing in Studio
- **InnoSetupImporter** — converts `.iss` Inno Setup scripts into `.mis` format
- Both importers are read-only; no compiler is ever invoked on the original script

#### Icon System
- 32 inline SVG action icons (24×24, dark-theme palette) embedded in `SvgIcons.h`
- `si()` helper: `QSvgRenderer` → `QPixmap` → `QIcon` (no external resource files)

#### Documentation
- `docs/index.html` — in-app help (opens from Help menu and HelpPanel dock)
- `README-MACOS-BUILD.html` — macOS build guide with Homebrew prerequisites

#### Example Manifests
- **`examples/AcmeWidgets/`** — full cross-platform example with real `payload/` directory structure
- **`examples/SimpleNotepad/`** — minimal two-platform starter example
- **`examples/DevSuite/`** — enterprise three-application-group example (full signing, all platforms)
- **`examples/StreamServer/`** — media server daemon + GUI pattern (macOS + Linux)

### Architecture Notes

- `.mis` YAML parsed by hand-rolled state machine in `Manifest.cpp` — no libyaml dependency
- Build button in BuildPanel emits `buildRequested()` signal → routes through
  `StudioMainWindow::onBuildStart()` → calls `collectEditors()` before validation;
  the Build button must NEVER call `onBuildClicked()` directly
- Signing overrides (from `BuilderProfile`) applied to the manifest **copy** inside
  `onBuildClicked()` at build-time; the `.mis` file on disk is never mutated by signing settings
- `WindowsBackend` NEVER calls `makensis.exe` — the `.nsi` importer is read-only

---

[1.0.0]: https://github.com/davestj/mcaster1-install-system/releases/tag/v1.0.0
