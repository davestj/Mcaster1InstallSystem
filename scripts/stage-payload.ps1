# stage-payload.ps1 — Stage all Windows build outputs into scripts/payload/
# Run from the project root: powershell -File scripts\stage-payload.ps1
# Requires: studio/build-win, runtime/build-win, cli/build-win all built (Debug config)

$ProjectRoot = Split-Path $PSScriptRoot -Parent
$PayloadDir  = "$PSScriptRoot\payload"
$StudioSrc   = "$ProjectRoot\studio\build-win\Debug"
$RuntimeSrc  = "$ProjectRoot\runtime\build-win\Debug"
$CliSrc      = "$ProjectRoot\cli\build-win\Debug"

Write-Host "Mcaster1 Install Studio — Payload Staging"
Write-Host "  Project root : $ProjectRoot"
Write-Host "  Payload dir  : $PayloadDir"
Write-Host ""

# ── Clean and recreate payload/ ───────────────────────────────────────────────
if (Test-Path $PayloadDir) {
    Write-Host "  Removing existing payload/ ..."
    Remove-Item -Recurse -Force $PayloadDir
}
New-Item -ItemType Directory -Force $PayloadDir | Out-Null

# ── Helper: copy with progress ─────────────────────────────────────────────────
function Stage-File($src, $dst) {
    $dstDir = Split-Path $dst -Parent
    if (-not (Test-Path $dstDir)) { New-Item -ItemType Directory -Force $dstDir | Out-Null }
    if (Test-Path $src) {
        Copy-Item -Force $src $dst
        Write-Host "  + $(Split-Path $dst -Leaf)"
    } else {
        Write-Host "  ! MISSING: $src" -ForegroundColor Yellow
    }
}

function Stage-Dir($src, $dst) {
    if (Test-Path $src) {
        Copy-Item -Recurse -Force $src $dst
        Write-Host "  + $(Split-Path $dst -Leaf)/ (dir)"
    } else {
        Write-Host "  ! MISSING DIR: $src" -ForegroundColor Yellow
    }
}

# ── Studio IDE + Qt DLLs ──────────────────────────────────────────────────────
Write-Host "[1/4] Staging Mcaster1InstallStudio.exe + Qt6 DLLs ..."

Stage-File "$StudioSrc\Mcaster1InstallStudio.exe" "$PayloadDir\Mcaster1InstallStudio.exe"

# Core Qt DLLs (debug build)
foreach ($dll in @(
    "Qt6Cored.dll", "Qt6Guid.dll", "Qt6Widgetsd.dll",
    "Qt6Svgd.dll", "Qt6Concurrentd.dll",
    "Qt6Networkd.dll", "Qt6Pdfd.dll",
    "opengl32sw.dll", "D3Dcompiler_47.dll",
    "dxcompiler.dll", "dxil.dll"
)) {
    $src = "$StudioSrc\$dll"
    if (Test-Path $src) { Stage-File $src "$PayloadDir\$dll" }
}

# Qt plugin subdirectories (required for GUI to start)
foreach ($dir in @("platforms", "iconengines", "imageformats", "styles",
                    "networkinformation", "generic", "tls")) {
    $src = "$StudioSrc\$dir"
    if (Test-Path $src) { Stage-Dir $src "$PayloadDir\$dir" }
}

# ── Runtime Installer ─────────────────────────────────────────────────────────
# The runtime installer needs windeployqt run against it so it gets its own Qt DLLs.
# These DLLs live in runtime/build-win/Debug/ after cmake --build runtime/build-win.
# We stage them into payload/ so the installed Mcaster1Installer.exe can find them.
Write-Host ""
Write-Host "[2/4] Staging Mcaster1Installer.exe + runtime Qt DLLs ..."

# Run windeployqt on runtime if not already done
$WinDeployQt = "C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe"
$RuntimeExe  = "$RuntimeSrc\Mcaster1Installer.exe"
if (Test-Path $RuntimeExe) {
    if (Test-Path $WinDeployQt) {
        Write-Host "  Running windeployqt on Mcaster1Installer.exe ..."
        & $WinDeployQt --no-translations $RuntimeExe 2>&1 | Where-Object { $_ -match "Updating|error" } | ForEach-Object { Write-Host "  $_" }
    }
    Stage-File $RuntimeExe "$PayloadDir\Mcaster1Installer.exe"
    # Copy any runtime-specific DLLs (Qt6SvgWidgets etc.) that may differ from Studio
    foreach ($dll in @("Qt6SvgWidgetsd.dll")) {
        if (Test-Path "$RuntimeSrc\$dll") { Stage-File "$RuntimeSrc\$dll" "$PayloadDir\$dll" }
    }
} else {
    Write-Host "  ! Mcaster1Installer.exe not found at: $RuntimeExe" -ForegroundColor Yellow
    Write-Host "  ! Run: cmake --build runtime/build-win --config Debug" -ForegroundColor Yellow
}

# ── miscc CLI ─────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "[3/4] Staging miscc.exe ..."
Stage-File "$CliSrc\miscc.exe"            "$PayloadDir\miscc.exe"
# miscc needs its own Qt Core + Concurrent DLLs alongside it
Stage-File "$CliSrc\Qt6Cored.dll"         "$PayloadDir\Qt6Cored.dll"       # already there (same ver)
Stage-File "$CliSrc\Qt6Concurrentd.dll"   "$PayloadDir\Qt6Concurrentd.dll" # already there

# ── Docs + Examples + License ─────────────────────────────────────────────────
Write-Host ""
Write-Host "[4/4] Staging docs/, examples/, LICENSE.md ..."
Stage-Dir  "$ProjectRoot\docs"        "$PayloadDir\docs"
Stage-Dir  "$ProjectRoot\examples"    "$PayloadDir\examples"
Stage-File "$ProjectRoot\LICENSE.md"  "$PayloadDir\LICENSE.md"

# ── Summary ──────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "Payload staged to: $PayloadDir"
$fileCount = (Get-ChildItem -Recurse -File $PayloadDir).Count
$sizeKB    = [math]::Round((Get-ChildItem -Recurse -File $PayloadDir |
              Measure-Object -Property Length -Sum).Sum / 1KB, 0)
Write-Host "  Files  : $fileCount"
Write-Host "  Size   : $sizeKB KB  ($([math]::Round($sizeKB/1024,1)) MB)"
Write-Host ""
Write-Host "Next: run  miscc.exe --build-file scripts\Mcaster1InstallStudio.mis --platform=windows"
