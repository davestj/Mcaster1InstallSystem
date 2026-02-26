# Mcaster1 Install Studio — Windows Build Guide

## Prerequisites

| Tool | Where |
|------|-------|
| Visual Studio 2022 (any edition) | https://visualstudio.microsoft.com/ |
| "Desktop development with C++" workload | VS Installer |
| Qt 6.x for MSVC 2022 x64 | Already at `C:\Qt` |
| Qt Visual Studio Tools extension | VS Marketplace (recommended) |

---

## Option A — Qt Creator (Recommended for cross-platform workflow)

Qt Creator natively understands the `.pro` files. No extension needed.

```
1. Open Qt Creator
2. File > Open File or Project
3. Browse to: Mcaster1InstallSystem\Mcaster1InstallSystem.pro
4. Select "MSVC2022 x64" kit  (configured from C:\Qt)
5. Configure Project
6. Build > Build All Projects  (Ctrl+Shift+B)
```

Both `Mcaster1InstallStudio.exe` and `Mcaster1Installer.exe` will be placed in:
```
build\debug\   (Debug)
build\release\ (Release)
```

Qt DLLs are automatically copied by the `windeployqt` post-build step.

---

## Option B — Visual Studio 2022 with Qt VS Tools

### 1. Install Qt Visual Studio Tools
```
VS2022 > Extensions > Manage Extensions > Online
Search: "Qt Visual Studio Tools"
Install → Restart VS
```

### 2. Register Qt Version in VS
```
VS2022 > Extensions > Qt VS Tools > Qt Versions
Add: C:\Qt\6.x.x\msvc2022_64   (use your installed version dir)
Name it: msvc2022_64
```

### 3. Open the Solution
```
File > Open > Project/Solution
Browse to: Mcaster1InstallSystem\windows\Mcaster1InstallSystem.sln
```

### 4. Build
```
Build > Build Solution  (Ctrl+Shift+B)
```

Output: `windows\x64\Debug\` or `windows\x64\Release\`

### 5. Run windeployqt (first time or after Qt update)
Open Developer Command Prompt for VS2022:
```powershell
cd Mcaster1InstallSystem\windows\x64\Debug
C:\Qt\6.x.x\msvc2022_64\bin\windeployqt.exe Mcaster1InstallStudio.exe
C:\Qt\6.x.x\msvc2022_64\bin\windeployqt.exe Mcaster1Installer.exe
```

---

## Option C — Visual Studio WITHOUT Qt VS Tools

If you cannot install the Qt VS Tools extension, MOC must be run manually first.

### Generate moc_*.cpp files
Open PowerShell from the project root:
```powershell
$qt    = "C:\Qt\6.8.1\msvc2022_64"   # adjust to your version
$moc   = "$qt\bin\moc.exe"
$out   = "windows\moc_generated"
New-Item -ItemType Directory -Force -Path $out | Out-Null

$headers = @(
    "studio\StudioMainWindow.h",
    "studio\ProjectSidebar.h",
    "studio\AppInfoEditor.h",
    "studio\FilesEditor.h",
    "studio\ComponentsEditor.h",
    "studio\ShortcutsEditor.h",
    "studio\RegistryEditor.h",
    "studio\SecurityEditor.h",
    "studio\PrerequisitesEditor.h",
    "studio\CustomActionsEditor.h",
    "studio\BuildPanel.h",
    "studio\CodeSignDialog.h",
    "studio\BuilderProfileDialog.h",
    "studio\EventLog.h",
    "studio\BuildHistory.h",
    "runtime\InstallerWizard.h",
    "runtime\WelcomePage.h",
    "runtime\LicensePage.h",
    "runtime\PrerequisitesPage.h",
    "runtime\ComponentsPage.h",
    "runtime\DirectoryPage.h",
    "runtime\ReadyPage.h",
    "runtime\InstallPage.h",
    "runtime\FinishPage.h",
    "runtime\InstallEngine.h"
)

foreach ($h in $headers) {
    $base = [IO.Path]::GetFileNameWithoutExtension($h)
    & $moc $h -o "$out\moc_$base.cpp"
    Write-Host "moc: $h"
}
```

Then add `windows\moc_generated\` to each vcxproj's `AdditionalIncludeDirectories`
and add all `moc_*.cpp` files as `<ClCompile>` items.

---

## Directory Structure After Build

```
windows\
  Mcaster1InstallSystem.sln       ← VS2022 solution
  Mcaster1InstallStudio.vcxproj   ← Studio IDE project
  Mcaster1Installer.vcxproj       ← Runtime wizard project
  props\
    Qt6.props                     ← Qt6 include/lib paths (auto-detect C:\Qt)
    Common.props                  ← Shared compiler/linker settings
  res\
    Mcaster1InstallStudio.manifest ← UAC + DPI manifest
    Mcaster1InstallStudio.rc       ← Version info + icon
    Mcaster1Installer.manifest
    Mcaster1Installer.rc
  x64\
    Debug\                        ← Built executables + DLLs (VS build)
    Release\
```

---

## Qt Version Auto-detection

`props\Qt6.props` probes for Qt versions in this order:
1. `QT_MSVC_DIR` environment variable (explicit override)
2. `C:\Qt\6.10.2\msvc2022_64` through `C:\Qt\6.6.3\msvc2022_64` (newest first)
   - Installed versions confirmed: 6.10.2, 6.9.3, 6.9.1, 6.8.3, 6.7.3
3. vcpkg: `%VCPKG_ROOT%\installed\x64-windows`
4. Qt VS Tools: `$(QTDIR)` (set by qt.props extension import)

To force a specific version, set the environment variable before opening VS:
```cmd
set QT_MSVC_DIR=C:\Qt\6.8.1\msvc2022_64
start devenv windows\Mcaster1InstallSystem.sln
```

---

## Common Errors

| Error | Fix |
|-------|-----|
| `Qt6Cored.lib not found` | Qt path wrong — check `QT_MSVC_DIR` or Qt VS Tools version |
| `moc_*.h not found` | Install Qt VS Tools OR run the manual moc script above |
| `windeployqt: qt.conf not found` | Run from the output directory, not project root |
| `LINK: fatal error LNK1181: cannot open input file 'Qt6Concurrentd.lib'` | Add `qt6-concurrent` to your Qt installation |
