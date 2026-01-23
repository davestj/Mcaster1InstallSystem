# Mcaster1 Install System — Example Projects

This directory contains fully-worked example installer projects you can use as
templates and to verify that the build pipeline works end-to-end.

---

## AcmeWidgets

A fictional "Acme Widgets 2.1.0" application from ACME Corporation.
Demonstrates every major feature of the Mcaster1 Install System manifest format:
cross-platform targeting (macOS / Windows / Linux), component selection,
prerequisites, shortcuts, Windows registry entries, and custom install actions.

```
examples/AcmeWidgets/
  AcmeWidgets.mis               ← the installer manifest
  LICENSE.txt                   ← MIT license
  resources/
    acme-widgets.icns            ← macOS app icon
    banner.png                   ← installer header image
    sidepanel.png                ← installer sidebar image
  payload/
    bin/
      acme-widgets               ← bash CLI (macOS / Linux demo binary)
      acme-widgets.bat           ← Windows batch equivalent
    etc/
      acme-widgets.conf          ← default INI configuration
    share/acme-widgets/
      README.txt
      CHANGELOG.txt
      sample.dat                 ← sample component data
  windows/
    AcmeWidgets.nsi              ← NSIS installer script (Windows)
    build-windows-installer.ps1  ← PowerShell build helper
  build-macos-pkg.sh             ← macOS .pkg builder
  build-linux-deb.sh             ← Debian/Ubuntu .deb builder
  dist/                          ← built installers land here (git-ignored)
  tmp/                           ← build temporaries (git-ignored)
```

---

## Building the Example Installers

### Option A — Platform-native build scripts (no miscc needed)

These scripts use only OS-provided tools and need no Qt or miscc binary.

#### macOS  →  `.pkg`

```bash
cd examples/AcmeWidgets
chmod +x build-macos-pkg.sh
./build-macos-pkg.sh
# Output: dist/AcmeWidgets-2.1.0-macos.pkg
```

Open the `.pkg` in Finder or run `open dist/AcmeWidgets-2.1.0-macos.pkg`
to see the full native macOS installer wizard (Welcome → License → Location → Install → Done).

To build with a Developer ID signature:
```bash
./build-macos-pkg.sh --sign "Developer ID Installer: Your Name (TEAMID)"
```

#### Linux  →  `.deb` (Debian/Ubuntu)

```bash
cd examples/AcmeWidgets
chmod +x build-linux-deb.sh
./build-linux-deb.sh
# Output: dist/acme-widgets_2.1.0-1_amd64.deb

# Install (Debian/Ubuntu, requires sudo):
sudo dpkg -i dist/acme-widgets_2.1.0-1_amd64.deb
acme-widgets --help
```

If `dpkg-deb` is not available (e.g. on macOS), pass `--stage-only` to produce
the staging tree you can inspect or copy to a Linux machine:
```bash
./build-linux-deb.sh --stage-only
```

#### Windows  →  NSIS `.exe`

```powershell
cd examples\AcmeWidgets\windows
.\build-windows-installer.ps1
# Output: dist\AcmeWidgets-2.1.0-win64-setup.exe
```

Requirements: [NSIS](https://nsis.sourceforge.io/) installed (adds `makensis.exe` to PATH).
If NSIS is not installed, the script prints instructions and exits cleanly.

---

### Option B — Using the `miscc` command-line compiler

`miscc` is the headless CLI tool that reads a `.mis` manifest and invokes the
appropriate platform backend.  Build it first if you have not already:

```bash
# From the Mcaster1InstallSystem root:
cmake -B cli/build -S cli -DCMAKE_BUILD_TYPE=Release
cmake --build cli/build --config Release
# Binary: cli/build/miscc  (or cli/build/Release/miscc.exe on Windows)
```

Then build each platform target from the manifest:

```bash
cd examples/AcmeWidgets

# macOS .pkg
../../cli/build/miscc --build-file=AcmeWidgets.mis --platform=macos --output-dir=dist

# Linux .deb
../../cli/build/miscc --build-file=AcmeWidgets.mis --platform=linux --output-dir=dist

# Windows NSIS installer (generates .nsi; requires makensis to compile)
../../cli/build/miscc --build-file=AcmeWidgets.mis --platform=windows --output-dir=dist

# All platforms at once
../../cli/build/miscc --build-file=AcmeWidgets.mis --no-sign

# Dry run — validate the manifest without building anything
../../cli/build/miscc --build-file=AcmeWidgets.mis --dry-run --verbose

# List platforms supported by this build
../../cli/build/miscc --list-platforms

# List available builder profiles
../../cli/build/miscc --list-profiles
```

**Common flags:**

| Flag | Description |
|------|-------------|
| `--build-file=FILE` | Path to the `.mis` manifest |
| `--platform=PLAT`   | `macos`, `windows`, or `linux` |
| `--output-dir=DIR`  | Where to write the built installer |
| `--no-sign`         | Skip all code signing |
| `--dry-run`         | Parse + validate only; no files written |
| `--verbose`         | Enable verbose/debug output |
| `--list-platforms`  | List compiled-in platform support |
| `--list-profiles`   | List saved builder profiles |

---

## Manifest Format Quick Reference

```yaml
format: mis/1

app:
  name:        "My Application"
  version:     "1.0.0"
  publisher:   "My Company"
  identifier:  "com.mycompany.myapp"
  icon:        "resources/myapp.icns"
  license:     "LICENSE.txt"

defaults:
  install-dir:
    macos:   "/Applications/My Application"
    windows: "C:\\Program Files\\My Company\\My Application"
    linux:   "/opt/myapp"
  require-admin:      true
  allow-custom-dir:   true
  create-uninstaller: true

components:
  - id:       core
    name:     "Core Application (required)"
    required: true
    selected: true
    files:
      - src: bin/myapp
        dst: "{install-dir}/bin/myapp"
        chmod: "+x"
        platforms: [macos, linux]
      - src: bin/myapp.bat
        dst: "{install-dir}/bin/myapp.bat"
        platforms: [windows]

shortcuts:
  - name:   "My Application"
    target: "{install-dir}/bin/myapp"
    type:   app

targets:
  - macos
  - windows
  - linux
```

See `AcmeWidgets.mis` for a complete example with all features including
prerequisites, application groups, Windows registry, and custom actions.

---

## What the Example Installers Do

These are **demonstration installers** — the payload binaries are shell/batch
script stubs that print colourful output but start no real service.

| Installer | What it does |
|-----------|-------------|
| macOS `.pkg` | Native Apple installer wizard; installs to `/Applications/ACME Widgets`; sets permissions; shows macOS notification on completion |
| Linux `.deb` | Installs to `/opt/acme-widgets`; creates `/usr/local/bin/acme-widgets` symlink; creates log directory; configures ldconfig |
| Windows NSIS `.exe` | Full MUI2 wizard; installs to `C:\Program Files\ACME Corporation\Acme Widgets`; writes registry; creates Start Menu shortcuts; optional Desktop shortcut |

Try running the installed binary:
```bash
# macOS / Linux
"/Applications/ACME Widgets/bin/acme-widgets" status
"/Applications/ACME Widgets/bin/acme-widgets" list

# Linux
/opt/acme-widgets/bin/acme-widgets --help

# Windows (Command Prompt)
"C:\Program Files\ACME Corporation\Acme Widgets\bin\acme-widgets.bat" status
```
