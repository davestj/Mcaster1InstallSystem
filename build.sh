#!/usr/bin/env bash
#
# build.sh — Build Mcaster1InstallSystem and (optionally) create a DMG
#
# USAGE
#   ./build.sh                                     # Debug build only
#   ./build.sh --release                           # Release build only
#   ./build.sh --dmg                               # Debug build + create DMG
#   ./build.sh --release --dmg                     # Release build + DMG
#   ./build.sh --skip-build --dmg                  # Repackage existing build
#   ./build.sh --payload-dir /path/to/payload      # Inject external payload
#   ./build.sh -h                                  # Help
#
# PAYLOAD DIRECTORY
#   The installer wizard bundles a payload/ tree into its app bundle at
#   Contents/Resources/payload/.  If --payload-dir is NOT given, the script
#   auto-builds the payload from:
#     - ./payload/app/Mcaster1DNAS.app       (put a macdeployqt'd app here)
#     - ./templates/                          (service config, XSL, shortcuts)
#   You can also pre-populate payload/ manually and pass --skip-payload-build.
#
# REQUIREMENTS
#   brew install qt cmake
#   Optional for polished DMG: brew install create-dmg
#
# Copyright 2025-2026, David St John <davestj@gmail.com>
# License: GPL-2.0

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Configuration ──────────────────────────────────────────────────────────────
APP_NAME="Mcaster1InstallSystem"
ORG_NAME="Mcaster1"
VERSION="2.5.3-beta"
ARCH="$(uname -m)"
BUILD_TYPE="Debug"

BUILD_DIR="${SCRIPT_DIR}/build"
DIST_DIR="${SCRIPT_DIR}/dist"
DMG_NAME="${ORG_NAME}-DNAS-${VERSION}-macOS-${ARCH}-installer.dmg"
DMG_PATH="${DIST_DIR}/${DMG_NAME}"
ICNS_PATH="${SCRIPT_DIR}/resources/icons/mcaster1.icns"

DMG_WIN_W=600
DMG_WIN_H=420

# ── Parse CLI flags ────────────────────────────────────────────────────────────
SKIP_BUILD=0
SKIP_PAYLOAD=0
MAKE_DMG=0
EXTERNAL_PAYLOAD=""

for arg in "$@"; do
    case "$arg" in
        --release)          BUILD_TYPE="Release" ;;
        --debug)            BUILD_TYPE="Debug" ;;
        --dmg)              MAKE_DMG=1 ;;
        --skip-build)       SKIP_BUILD=1 ;;
        --skip-payload-build) SKIP_PAYLOAD=1 ;;
        --payload-dir=*)    EXTERNAL_PAYLOAD="${arg#--payload-dir=}" ;;
        -h|--help)
            sed -n '3,18p' "$0" | sed 's/^# \{0,2\}//'
            exit 0 ;;
        *) echo "Unknown option: $arg  (use -h for help)" >&2; exit 1 ;;
    esac
done

# ── Terminal colors ────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'
step() { echo -e "\n${GREEN}${BOLD}==> $*${NC}"; }
info() { echo -e "    ${CYAN}$*${NC}"; }
warn() { echo -e "    ${YELLOW}WARN: $*${NC}"; }
die()  { echo -e "\n${RED}ERROR: $*${NC}\n" >&2; exit 1; }

# Helper: write a .webloc (macOS web bookmark) file
webloc() {
    local path="$1" url="$2"
    cat > "$path" << WEBLOC
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict><key>URL</key><string>${url}</string></dict>
</plist>
WEBLOC
}

# ── Banner ─────────────────────────────────────────────────────────────────────
echo -e "${BOLD}"
printf "╔══════════════════════════════════════════════════════════╗\n"
printf "║  Mcaster1InstallSystem  Builder                         ║\n"
printf "║  Version: %-12s  Arch: %-10s  Build: %-7s  ║\n" "$VERSION" "$ARCH" "$BUILD_TYPE"
printf "╚══════════════════════════════════════════════════════════╝\n"
echo -e "${NC}"

# ── Prerequisites ──────────────────────────────────────────────────────────────
step "Checking prerequisites..."
BREW="$(brew --prefix 2>/dev/null)" || die "Homebrew not found. Install from https://brew.sh"
QT_PREFIX="${BREW}/opt/qt"
MACDEPLOYQT="${QT_PREFIX}/bin/macdeployqt"
CMAKE="$(command -v cmake 2>/dev/null)" || die "cmake not found. Run: brew install cmake"

[[ -d "${QT_PREFIX}" ]]  || die "Qt not found at ${QT_PREFIX}. Run: brew install qt"
[[ -x "${MACDEPLOYQT}" ]] || die "macdeployqt not found at ${MACDEPLOYQT}"

info "Homebrew:     ${BREW}"
info "Qt6:          ${QT_PREFIX}"
info "Build type:   ${BUILD_TYPE}"
info "Output dir:   ${BUILD_DIR}"

# ── Step 1: Build Mcaster1InstallSystem.app ────────────────────────────────────
if [[ "$SKIP_BUILD" -eq 0 ]]; then
    step "Step 1/4: Building ${APP_NAME}.app (${BUILD_TYPE})..."
    cmake -B "${BUILD_DIR}" -S "${SCRIPT_DIR}" \
        -DCMAKE_PREFIX_PATH="${QT_PREFIX}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -Wno-dev 2>&1 | grep -E "^(-- |CMake |Error)" | head -30 || true
    cmake --build "${BUILD_DIR}" -j"$(sysctl -n hw.logicalcpu)"
    info "Built: ${BUILD_DIR}/${APP_NAME}.app"
else
    step "Step 1/4: Skipping build (--skip-build)"
    [[ -d "${BUILD_DIR}/${APP_NAME}.app" ]] || die "No build at ${BUILD_DIR}/${APP_NAME}.app"
    info "Reusing: ${BUILD_DIR}/${APP_NAME}.app"
fi

APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
[[ -f "${APP_PATH}/Contents/MacOS/${APP_NAME}" ]] || die "Binary not found in ${APP_PATH}"
INSTALLER_RESOURCES="${APP_PATH}/Contents/Resources"

# ── Step 2: Build / inject payload ────────────────────────────────────────────
step "Step 2/4: Preparing payload..."

if [[ -n "${EXTERNAL_PAYLOAD}" ]]; then
    # Use caller-supplied payload directory
    PAYLOAD_SRC="${EXTERNAL_PAYLOAD}"
    [[ -d "${PAYLOAD_SRC}" ]] || die "External payload not found: ${PAYLOAD_SRC}"
    info "Using external payload: ${PAYLOAD_SRC}"
elif [[ "$SKIP_PAYLOAD" -eq 0 ]]; then
    # Build payload from templates + whatever is in payload/app/
    PAYLOAD_SRC="${SCRIPT_DIR}/payload"
    mkdir -p "${PAYLOAD_SRC}/service" "${PAYLOAD_SRC}/web" "${PAYLOAD_SRC}/admin"
    mkdir -p "${PAYLOAD_SRC}/ssl"     "${PAYLOAD_SRC}/configs" "${PAYLOAD_SRC}/shortcuts"

    # Service templates
    TMPL="${SCRIPT_DIR}/templates"
    [[ -f "${TMPL}/mcaster1dnas-service.yaml" ]]       && cp "${TMPL}/mcaster1dnas-service.yaml"       "${PAYLOAD_SRC}/service/"
    [[ -f "${TMPL}/com.mcaster1.mcaster1dnas.plist" ]] && cp "${TMPL}/com.mcaster1.mcaster1dnas.plist" "${PAYLOAD_SRC}/service/"
    [[ -f "${TMPL}/install-service.sh" ]]              && cp "${TMPL}/install-service.sh"              "${PAYLOAD_SRC}/service/" && chmod +x "${PAYLOAD_SRC}/service/install-service.sh"
    [[ -f "${TMPL}/ssl/SSL-SETUP.txt" ]]               && cp "${TMPL}/ssl/SSL-SETUP.txt"               "${PAYLOAD_SRC}/ssl/"

    # Config templates
    [[ -f "${TMPL}/mcaster1dnas-install.yaml" ]] && cp "${TMPL}/mcaster1dnas-install.yaml" "${PAYLOAD_SRC}/configs/mcaster1dnas.yaml"
    [[ -f "${TMPL}/mcaster1dnas-install.xml" ]]  && cp "${TMPL}/mcaster1dnas-install.xml"  "${PAYLOAD_SRC}/configs/mcaster1dnas.xml"

    # Shortcuts
    SHORTCUTS="${PAYLOAD_SRC}/shortcuts"
    cat > "${SHORTCUTS}/Launch Mcaster1DNAS.command" << 'CMDEOF'
#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP="${SCRIPT_DIR}/Mcaster1DNAS.app"
if [[ -d "$APP" ]]; then
    open "$APP"
else
    osascript -e 'display dialog "Mcaster1DNAS.app was not found.\n\nRun from the install directory." buttons {"OK"} default button "OK" with icon stop'
fi
CMDEOF
    chmod +x "${SHORTCUTS}/Launch Mcaster1DNAS.command"

    if [[ -f "${TMPL}/uninstall-mcaster1dnas.sh" ]]; then
        cp "${TMPL}/uninstall-mcaster1dnas.sh" "${SHORTCUTS}/Uninstall Mcaster1DNAS.command"
        chmod +x "${SHORTCUTS}/Uninstall Mcaster1DNAS.command"
    fi

    webloc "${SHORTCUTS}/Documentation.webloc"    "https://mcaster1.com/mcaster1dnas/"
    webloc "${SHORTCUTS}/Mcaster1 Website.webloc" "https://mcaster1.com/mcaster1_dnas.php"
    webloc "${SHORTCUTS}/Support & Issues.webloc"  "https://github.com/davestj/mcaster1dnas/issues"

    if [[ ! -d "${PAYLOAD_SRC}/app" ]]; then
        warn "payload/app/ is empty — no Mcaster1DNAS.app bundled."
        warn "To bundle it: run the mcaster1dnas make gui + macdeployqt, then"
        warn "  cp -R macos/build-qt/Mcaster1DNAS.app ${PAYLOAD_SRC}/app/"
    fi
    info "Payload prepared at: ${PAYLOAD_SRC}"
else
    PAYLOAD_SRC="${SCRIPT_DIR}/payload"
    [[ -d "${PAYLOAD_SRC}" ]] || die "payload/ directory missing and --skip-payload-build given."
    info "Reusing existing payload: ${PAYLOAD_SRC}"
fi

# Inject payload into installer bundle
info "Injecting payload into ${APP_NAME}.app..."
rm -rf "${INSTALLER_RESOURCES}/payload"
cp -R "${PAYLOAD_SRC}" "${INSTALLER_RESOURCES}/payload"
PAYLOAD_SIZE="$(du -sh "${INSTALLER_RESOURCES}/payload" 2>/dev/null | cut -f1)"
info "Payload injected: ${PAYLOAD_SIZE}"

# ── Step 3: macdeployqt + codesign ────────────────────────────────────────────
step "Step 3/4: Bundling Qt6 frameworks + codesigning..."

# macdeployqt must run AFTER payload injection is complete so it doesn't
# try to scan the bundled Mcaster1DNAS.app recursively.
# (If macdeployqt was already run, re-running with -always-overwrite is safe.)
"${MACDEPLOYQT}" "${APP_PATH}" \
    -verbose=1 \
    -hardened-runtime \
    -always-overwrite \
    2>&1 | grep -E "^(Copying|WARNING|ERROR)" | head -60 || true

# Two-step codesign: deep first (signs all nested dylibs + platform plugin),
# then re-sign QtWebEngineProcess with its JIT entitlements if present.
HELPER="${APP_PATH}/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
ENTITLEMENTS="${HELPER}/Contents/Resources/QtWebEngineProcess.entitlements"
codesign --force --deep --sign - "${APP_PATH}"
info "[1/2] Deep-signed all nested code"
if [[ -f "${ENTITLEMENTS}" ]]; then
    codesign --force --sign - --entitlements "${ENTITLEMENTS}" "${HELPER}" 2>/dev/null && \
    info "[2/2] QtWebEngineProcess re-signed with JIT entitlements" || true
fi

APP_SIZE="$(du -sh "${APP_PATH}" 2>/dev/null | cut -f1)"
info "Installer app: ${APP_SIZE}"
info "Location: ${APP_PATH}"
info "Run: open ${APP_PATH}"

# ── Step 4 (optional): Create DMG ─────────────────────────────────────────────
if [[ "$MAKE_DMG" -eq 0 ]]; then
    echo ""
    echo -e "${GREEN}${BOLD}Build complete.${NC}"
    echo -e "  App: ${APP_PATH}"
    echo -e "  Run: open ${APP_PATH}"
    echo ""
    echo -e "  To create a distributable DMG:"
    echo -e "    ./build.sh --dmg"
    exit 0
fi

step "Step 4/4: Creating DMG..."
mkdir -p "${DIST_DIR}"
[[ -f "${DMG_PATH}" ]] && { warn "Removing existing: ${DMG_PATH}"; rm -f "${DMG_PATH}"; }

VOLNAME="${ORG_NAME} DNAS ${VERSION} Installer"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

STAGING="${WORK_DIR}/dmg_staging"
mkdir -p "${STAGING}"
cp -R "${APP_PATH}" "${STAGING}/"

# DMG background (splash.png scaled via sips)
BG_DIR="${STAGING}/.background"
mkdir -p "${BG_DIR}"
SPLASH_SRC="${SCRIPT_DIR}/resources/splash.png"
if [[ -f "${SPLASH_SRC}" ]]; then
    sips -z ${DMG_WIN_H} ${DMG_WIN_W} "${SPLASH_SRC}" --out "${BG_DIR}/background.png" 2>/dev/null
    info "DMG background: splash.png scaled to ${DMG_WIN_W}x${DMG_WIN_H}"
else
    warn "resources/splash.png not found — DMG will have plain background"
fi

if command -v create-dmg &>/dev/null; then
    info "Using create-dmg..."
    create-dmg \
        --volname          "${VOLNAME}" \
        --background       "${BG_DIR}/background.png" \
        --window-pos       "150" "100" \
        --window-size      "${DMG_WIN_W}" "${DMG_WIN_H}" \
        --icon-size        "128" \
        --text-size        "13" \
        --icon             "${APP_NAME}.app" "300" "280" \
        --hide-extension   "${APP_NAME}.app" \
        --no-internet-enable \
        "${DMG_PATH}" \
        "${STAGING}" 2>&1 | tail -8
else
    info "Using hdiutil..."
    TREE_SIZE_KB="$(du -sk "${STAGING}" | cut -f1)"
    RW_DMG="${WORK_DIR}/installer-rw.dmg"
    RW_SIZE_MB=$(( (TREE_SIZE_KB / 1024) + 64 ))

    hdiutil create -megabytes "${RW_SIZE_MB}" -fs HFS+ -volname "${VOLNAME}" -o "${RW_DMG}"
    hdiutil detach "/Volumes/${VOLNAME}"   -quiet 2>/dev/null || true
    hdiutil detach "/Volumes/${VOLNAME} 1" -quiet 2>/dev/null || true

    MOUNT_OUTPUT="$(hdiutil attach "${RW_DMG}" -readwrite -noverify -noautoopen)"
    MOUNT_DEV="$(  echo "${MOUNT_OUTPUT}" | grep Apple_HFS | cut -f1 | tr -d '[:space:]')"
    MOUNT_PATH="$( echo "${MOUNT_OUTPUT}" | grep Apple_HFS | cut -f3 | sed 's/^[[:space:]]*//')"
    [[ -n "${MOUNT_PATH}" ]] || die "Failed to get mount path from hdiutil"

    ACTUAL_VOLNAME="$(basename "${MOUNT_PATH}")"
    cp -R "${STAGING}"/. "${MOUNT_PATH}/"
    chflags hidden "${MOUNT_PATH}/.background" 2>/dev/null || true

    /usr/bin/osascript << ASEOF || warn "Finder layout script failed — DMG is valid but icons may not be pre-positioned"
tell application "Finder"
    tell disk "${ACTUAL_VOLNAME}"
        open
        delay 3
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {150, 100, 750, 520}
        set vw to the icon view options of container window
        set arrangement of vw to not arranged
        set icon size of vw to 128
        set text size of vw to 13
        set background picture of vw to POSIX file "${MOUNT_PATH}/.background/background.png"
        set position of item "${APP_NAME}.app" of container window to {300, 280}
        update without registering applications
        delay 2
        close
    end tell
end tell
ASEOF

    sync
    hdiutil detach "${MOUNT_DEV}" -quiet
    hdiutil convert "${RW_DMG}" -format UDZO -imagekey zlib-level=9 -o "${DMG_PATH}"
fi

DMG_SIZE="$(du -sh "${DMG_PATH}" 2>/dev/null | cut -f1)"
echo ""
echo -e "${GREEN}${BOLD}"
printf "╔══════════════════════════════════════════════════════════╗\n"
printf "║  ✓  Installer DMG ready!                                ║\n"
printf "╚══════════════════════════════════════════════════════════╝\n"
echo -e "${NC}"
info "DMG:  ${DMG_PATH}"
info "Size: ${DMG_SIZE}"
echo ""
echo -e "${BOLD}To install (end users):${NC}"
echo "  1. Double-click ${DMG_NAME}"
echo "  2. Double-click ${APP_NAME}.app"
echo "  3. Follow the installer wizard"
echo ""
echo -e "${BOLD}For Developer ID + notarization:${NC}"
echo "  Replace '--sign -' with your certificate name, then:"
echo "  xcrun notarytool submit ${DMG_PATH} --apple-id you@example.com ..."
echo "  xcrun stapler staple ${DMG_PATH}"
echo ""
