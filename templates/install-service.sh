#!/usr/bin/env bash
#
# install-service.sh — Register Mcaster1DNAS as a macOS background service
# =========================================================================
#
# This script installs the LaunchAgent plist so the mcaster1 server binary
# starts automatically when you log in and restarts if it crashes.
#
# PORTS:  HTTP 9330  •  HTTPS 9443
# CONFIG: /Applications/Mcaster1/mcaster1dnas-service.yaml
#
# The GUI app (Mcaster1DNAS.app) runs on ports 9033/9344 — both can run
# simultaneously without conflict.
#
# Usage:
#   double-click  install-service.sh   in Finder, OR
#   cd /Applications/Mcaster1 && sudo ./install-service.sh
#

set -euo pipefail

INSTALL_DIR="/Applications/Mcaster1"
BINARY="${INSTALL_DIR}/bin/mcaster1"
CONFIG="${INSTALL_DIR}/mcaster1dnas-service.yaml"
PLIST_SRC="${INSTALL_DIR}/com.mcaster1.mcaster1dnas.plist"
LAUNCHD_DIR="${HOME}/Library/LaunchAgents"
PLIST_DEST="${LAUNCHD_DIR}/com.mcaster1.mcaster1dnas.plist"
LABEL="com.mcaster1.mcaster1dnas"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'
ok()   { echo -e "${GREEN}  ✓  $*${NC}"; }
info() { echo -e "${CYAN}  →  $*${NC}"; }
warn() { echo -e "${YELLOW}  !  $*${NC}"; }
die()  { echo -e "\n${RED}ERROR: $*${NC}\n" >&2; exit 1; }

echo ""
echo -e "${BOLD}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║  Mcaster1DNAS — Service Installer                      ║${NC}"
echo -e "${BOLD}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""

# ── Preflight checks ──────────────────────────────────────────────────────────
[[ -d "${INSTALL_DIR}" ]] || die "Mcaster1 is not installed at ${INSTALL_DIR}
       Please drag the Mcaster1 folder from the DMG to /Applications first."

[[ -x "${BINARY}" ]] || die "Server binary not found: ${BINARY}"

[[ -f "${CONFIG}" ]] || die "Service config not found: ${CONFIG}
       Edit /Applications/Mcaster1/mcaster1dnas-service.yaml first."

[[ -f "${PLIST_SRC}" ]] || die "LaunchAgent plist missing: ${PLIST_SRC}"

# ── Check passwords not still set to defaults ─────────────────────────────────
if grep -q "CHANGE_ME" "${CONFIG}" 2>/dev/null; then
    warn "IMPORTANT: mcaster1dnas-service.yaml still has CHANGE_ME passwords!"
    warn "Edit the file before running a public-facing server:"
    warn "  open /Applications/Mcaster1/mcaster1dnas-service.yaml"
    echo ""
fi

# ── Create LaunchAgents dir if needed ─────────────────────────────────────────
mkdir -p "${LAUNCHD_DIR}"

# ── Stop existing service if running ──────────────────────────────────────────
if launchctl list "${LABEL}" &>/dev/null 2>&1; then
    info "Stopping existing service..."
    launchctl unload "${PLIST_DEST}" 2>/dev/null || true
fi

# ── Install the plist ─────────────────────────────────────────────────────────
info "Installing LaunchAgent plist..."
cp "${PLIST_SRC}" "${PLIST_DEST}"
chmod 644 "${PLIST_DEST}"
ok "Installed: ${PLIST_DEST}"

# ── Create logs dir ───────────────────────────────────────────────────────────
mkdir -p "${INSTALL_DIR}/logs"
ok "Log directory ready: ${INSTALL_DIR}/logs/"

# ── Load (start) the service ──────────────────────────────────────────────────
info "Starting service..."
launchctl load -w "${PLIST_DEST}" 2>/dev/null || die "launchctl load failed"
sleep 1

if launchctl list "${LABEL}" 2>/dev/null | grep -q '"Label"'; then
    ok "Service is running  (label: ${LABEL})"
else
    warn "Service may not have started. Check the log:"
    warn "  tail -f ${INSTALL_DIR}/logs/mcaster1-service.log"
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
ok "Mcaster1DNAS background service installed!"
echo ""
echo -e "${BOLD}Listening on:${NC}"
echo "   HTTP  → http://127.0.0.1:9330/status.xsl"
echo "   HTTPS → https://127.0.0.1:9443/status.xsl"
echo ""
echo -e "${BOLD}Config:${NC}  ${CONFIG}"
echo -e "${BOLD}Logs:${NC}    ${INSTALL_DIR}/logs/"
echo ""
echo -e "${BOLD}Manage the service:${NC}"
echo "   Stop    : launchctl unload ${PLIST_DEST}"
echo "   Start   : launchctl load   ${PLIST_DEST}"
echo "   Status  : launchctl list ${LABEL}"
echo "   Live log: tail -f ${INSTALL_DIR}/logs/mcaster1-service.log"
echo ""
echo -e "${BOLD}Uninstall:${NC}"
echo "   Open /Applications/Mcaster1/'Uninstall Mcaster1DNAS.command'"
echo ""
