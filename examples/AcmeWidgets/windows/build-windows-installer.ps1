# ──────────────────────────────────────────────────────────────────────────────
# build-windows-installer.ps1  -  Build the ACME Widgets Windows NSIS installer
#
# Usage:
#   .\build-windows-installer.ps1
#   .\build-windows-installer.ps1 -SkipNSIS     # just check, don't run makensis
#   .\build-windows-installer.ps1 -MakeNsisPath "C:\Program Files (x86)\NSIS\makensis.exe"
# ──────────────────────────────────────────────────────────────────────────────
param(
    [switch]$SkipNSIS,
    [string]$MakeNsisPath = ""
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ExampleDir = Split-Path -Parent $ScriptDir
$DistDir = Join-Path $ExampleDir "dist"

$AppName    = "Acme Widgets"
$AppVersion = "2.1.0"
$NsiScript  = Join-Path $ScriptDir "AcmeWidgets.nsi"

function Write-Info  { param($msg) Write-Host "[INFO]  $msg" -ForegroundColor Cyan }
function Write-Ok    { param($msg) Write-Host "[ OK ]  $msg" -ForegroundColor Green }
function Write-Warn  { param($msg) Write-Host "[WARN]  $msg" -ForegroundColor Yellow }
function Write-Fail  { param($msg) Write-Host "[FAIL]  $msg" -ForegroundColor Red }

Write-Info "ACME Widgets ${AppVersion} - Windows Installer Build"
Write-Info "Script: ${NsiScript}"

# Create output directory
if (-not (Test-Path $DistDir)) {
    New-Item -ItemType Directory -Path $DistDir | Out-Null
}

# Check payload files exist
Write-Info "Checking payload files..."
$requiredFiles = @(
    (Join-Path $ExampleDir "payload\bin\acme-widgets.bat"),
    (Join-Path $ExampleDir "payload\etc\acme-widgets.conf"),
    (Join-Path $ExampleDir "payload\share\acme-widgets\README.txt"),
    (Join-Path $ExampleDir "payload\share\acme-widgets\CHANGELOG.txt"),
    (Join-Path $ExampleDir "LICENSE.txt")
)
foreach ($f in $requiredFiles) {
    if (-not (Test-Path $f)) {
        Write-Fail "Missing required file: $f"
        exit 1
    }
}
Write-Ok "All payload files present"

if ($SkipNSIS) {
    Write-Warn "SkipNSIS flag set - skipping makensis build."
    Write-Info "NSI script is ready at: $NsiScript"
    Write-Info "To build manually: makensis `"$NsiScript`""
    exit 0
}

# Locate makensis
if ($MakeNsisPath -eq "") {
    $candidates = @(
        "C:\Program Files (x86)\NSIS\makensis.exe",
        "C:\Program Files\NSIS\makensis.exe",
        (Get-Command makensis -ErrorAction SilentlyContinue)?.Source
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path $c)) {
            $MakeNsisPath = $c
            break
        }
    }
}

if ($MakeNsisPath -eq "" -or -not (Test-Path $MakeNsisPath)) {
    Write-Warn "makensis.exe not found."
    Write-Warn ""
    Write-Warn "To build the Windows installer:"
    Write-Warn "  1. Install NSIS from https://nsis.sourceforge.io/"
    Write-Warn "  2. Run: .\build-windows-installer.ps1"
    Write-Warn ""
    Write-Warn "The NSI script is ready to use: $NsiScript"
    Write-Info "You can also use miscc CLI: miscc --build-file=AcmeWidgets.mis --platform=windows"
    exit 0
}

Write-Info "Using makensis: $MakeNsisPath"
Write-Info "Building installer..."

Push-Location $ScriptDir
try {
    & $MakeNsisPath $NsiScript
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "makensis exited with code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
} finally {
    Pop-Location
}

$outExe = Join-Path $DistDir "AcmeWidgets-${AppVersion}-win64-setup.exe"
if (Test-Path $outExe) {
    $size = (Get-Item $outExe).Length / 1KB
    Write-Ok "Installer built: $(Split-Path -Leaf $outExe) ($([math]::Round($size)) KB)"
    Write-Host ""
    Write-Host "  To install (run as Administrator):" -ForegroundColor White
    Write-Host "    Start-Process `"$outExe`" -Verb RunAs" -ForegroundColor Gray
    Write-Host ""
} else {
    Write-Fail "Expected output not found: $outExe"
    exit 1
}
