#!/usr/bin/env bash
#
# uninstall-mcaster1dnas.sh — Remove Mcaster1DNAS from your Mac
#
# Removes:
#   /Applications/Mcaster1DNAS.app        — the app bundle
#   ~/Library/Application Support/...     — cached state
#   ~/Library/Preferences/...             — QSettings .plist
#   ~/Library/Caches/...                  — Qt WebEngine cache
#   ~/Library/LaunchAgents/...            — launchd plist (if installed)
#
# Config and log files created by the SERVER (in the locations you chose
# during setup) are NOT automatically removed — they are listed at the end
# so you can review and remove them yourself.
#
# Usage:
#   ./uninstall-mcaster1dnas.sh           # interactive (asks before each step)
#   ./uninstall-mcaster1dnas.sh --force   # no prompts (for scripts/CI)
#
# Copyright 2025-2026, David St John <davestj@gmail.com>
# License: GPL-2.0

set -euo pipefail

# ── Config ────────────────────────────────────────────────────────────────────
APP_NAME="Mcaster1DNAS"
BUNDLE_ID="com.mcaster1.Mcaster1DNAS"
SERVICE_LABEL="com.mcaster1.mcaster1dnas"
ORG_NAME="MediaCast1"

# Installation group folder (/Applications/Mcaster1/ contains the .app + binaries)
GROUP_DIR="/Applications/Mcaster1"
APP_PATH="${GROUP_DIR}/${APP_NAME}.app"
SUPPORT_DIR="${HOME}/Library/Application Support/${ORG_NAME}"
PREFS_PLIST="${HOME}/Library/Preferences/${BUNDLE_ID}.plist"
CACHE_DIR="${HOME}/Library/Caches/${BUNDLE_ID}"
LAUNCH_AGENT="${HOME}/Library/LaunchAgents/${SERVICE_LABEL}.plist"

# ── Terminal colors ───────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'
ok()   { echo -e "  ${GREEN}✓${NC} $*"; }
skip() { echo -e "  ${CYAN}–${NC} $* (not found, skipping)"; }
warn() { echo -e "  ${YELLOW}!${NC} $*"; }
die()  { echo -e "\n${RED}ABORT: $*${NC}\n" >&2; exit 1; }

# ── Parse flags ───────────────────────────────────────────────────────────────
FORCE=0
for arg in "$@"; do
    case "$arg" in
        --force|-f) FORCE=1 ;;
        -h|--help)
            echo "Usage: $0 [--force]"
            echo "  --force   Skip confirmation prompts"
            exit 0 ;;
        *)
            echo "Unknown option: $arg" >&2; exit 1 ;;
    esac
done

# ── Confirmation helper ───────────────────────────────────────────────────────
confirm() {
    # confirm "message" → returns 0 (yes) or 1 (no)
    [[ "$FORCE" -eq 1 ]] && return 0
    local reply
    read -rp "    $1 [y/N] " reply
    [[ "${reply,,}" == "y" || "${reply,,}" == "yes" ]]
}

# ── Banner ────────────────────────────────────────────────────────────────────
echo -e "${BOLD}"
printf "╔══════════════════════════════════════════════════════════╗\n"
printf "║  Mcaster1DNAS Uninstaller                               ║\n"
printf "╚══════════════════════════════════════════════════════════╝\n"
echo -e "${NC}"
echo "This script will remove ${APP_NAME} from your Mac."
echo ""

if [[ "$FORCE" -eq 0 ]]; then
    echo -e "${YELLOW}The following will be removed:${NC}"
    [[ -d "$GROUP_DIR"      ]] && echo "  • ${GROUP_DIR}  (entire Mcaster1 group folder)"
    [[ -d "$SUPPORT_DIR"    ]] && echo "  • ${SUPPORT_DIR}"
    [[ -f "$PREFS_PLIST"    ]] && echo "  • ${PREFS_PLIST}"
    [[ -d "$CACHE_DIR"      ]] && echo "  • ${CACHE_DIR}"
    [[ -f "$LAUNCH_AGENT"   ]] && echo "  • ${LAUNCH_AGENT} (launchd service)"
    echo ""
    confirm "Proceed with uninstall?" || { echo "Cancelled."; exit 0; }
    echo ""
fi

# ── Step 1: Stop background service ──────────────────────────────────────────
echo -e "${BOLD}Step 1/5: Stopping background service...${NC}"
if launchctl list "${SERVICE_LABEL}" &>/dev/null 2>&1; then
    launchctl unload "${LAUNCH_AGENT}" 2>/dev/null || true
    ok "Service stopped."
else
    skip "Service not running"
fi

# ── Step 2: Quit the GUI app if running ───────────────────────────────────────
echo -e "${BOLD}Step 2/5: Stopping ${APP_NAME} GUI...${NC}"
if pgrep -x "${APP_NAME}" &>/dev/null; then
    warn "${APP_NAME} is running — attempting graceful quit..."
    osascript -e "tell application \"${APP_NAME}\" to quit" 2>/dev/null || true
    sleep 2
    if pgrep -x "${APP_NAME}" &>/dev/null; then
        warn "Graceful quit timed out — force killing..."
        pkill -9 -x "${APP_NAME}" 2>/dev/null || true
    fi
    ok "Stopped."
else
    skip "${APP_NAME} GUI not running"
fi

# ── Step 3: Remove the Mcaster1 group folder + all contents ───────────────────
echo -e "${BOLD}Step 3/5: Removing /Applications/Mcaster1/...${NC}"
if [[ -d "$GROUP_DIR" ]]; then
    rm -rf "$GROUP_DIR"
    ok "Removed: ${GROUP_DIR}"
elif [[ -d "$APP_PATH" ]]; then
    # Legacy: app installed without group folder
    rm -rf "$APP_PATH"
    ok "Removed: ${APP_PATH}"
else
    skip "Nothing found at ${GROUP_DIR}"
fi

# Remove launchd agent plist
if [[ -f "$LAUNCH_AGENT" ]]; then
    rm -f "$LAUNCH_AGENT"
    ok "Removed: ${LAUNCH_AGENT}"
fi

# ── Step 4: Remove Library files ──────────────────────────────────────────────
echo -e "${BOLD}Step 4/5: Removing Library files...${NC}"

# Application Support (Qt-created, org-level directory)
if [[ -d "$SUPPORT_DIR" ]]; then
    rm -rf "$SUPPORT_DIR"
    ok "Removed: ${SUPPORT_DIR}"
else
    skip "${SUPPORT_DIR}"
fi

# Preferences plist (QSettings-created)
if [[ -f "$PREFS_PLIST" ]]; then
    rm -f "$PREFS_PLIST"
    ok "Removed: ${PREFS_PLIST}"
else
    skip "${PREFS_PLIST}"
fi

# Qt WebEngine cache
if [[ -d "$CACHE_DIR" ]]; then
    rm -rf "$CACHE_DIR"
    ok "Removed: ${CACHE_DIR}"
else
    skip "${CACHE_DIR}"
fi

# ── Step 5: Locate server-created files (do NOT auto-remove) ─────────────────
echo -e "${BOLD}Step 5/5: Locating server data files (manual review)...${NC}"
echo ""
echo -e "${YELLOW}The following files were created by the Mcaster1DNAS SERVER${NC}"
echo "and are NOT automatically removed (they may contain your configuration,"
echo "recordings, or log history):"
echo ""

FOUND_ANY=0
for candidate in \
        "${HOME}/.config/mcaster1dnas" \
        "${HOME}/.mcaster1dnas" \
        "${HOME}/mcaster1dnas.yaml" \
        "${HOME}/mcaster1dnas-macos.yaml" \
        "${HOME}/Library/Logs/${APP_NAME}" \
        "/var/log/mcaster1dnas" \
        "/usr/local/var/log/mcaster1dnas" \
        "/opt/homebrew/var/log/mcaster1dnas"; do
    if [[ -e "$candidate" ]]; then
        echo "  • ${candidate}"
        FOUND_ANY=1
    fi
done

if [[ "$FOUND_ANY" -eq 0 ]]; then
    echo "  (none found in standard locations)"
fi
echo ""
echo "Remove these manually if you no longer need them:"
echo "  rm -rf <path>"
echo ""

# ── Done ─────────────────────────────────────────────────────────────────────
echo -e "${GREEN}${BOLD}"
printf "╔══════════════════════════════════════════════════════════╗\n"
printf "║  ✓  Mcaster1DNAS has been uninstalled.                  ║\n"
printf "╚══════════════════════════════════════════════════════════╝\n"
echo -e "${NC}"
echo "If you installed via Homebrew: brew uninstall --cask mcaster1dnas"
echo "For deep removal of all files: brew uninstall --zap --cask mcaster1dnas"
