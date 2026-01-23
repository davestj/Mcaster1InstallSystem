#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
# build-linux-deb.sh  —  Build the ACME Widgets 2.1.0 Debian/Ubuntu .deb package
#
# Produces:  dist/acme-widgets_2.1.0-1_amd64.deb
#
# Requirements:
#   - Linux (Debian/Ubuntu) with dpkg-deb
#   - OR: macOS with 'brew install dpkg' (builds the stage tree at minimum)
#
# Usage:
#   ./build-linux-deb.sh              # build .deb
#   ./build-linux-deb.sh --stage-only # only create staging tree (for inspection)
#   ./build-linux-deb.sh --clean      # clean dist/ and tmp/ then build
# ──────────────────────────────────────────────────────────────────────────────
set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
PACKAGE="acme-widgets"
VERSION="2.1.0"
REVISION="1"
ARCH="amd64"
PKG_FULL="${PACKAGE}_${VERSION}-${REVISION}_${ARCH}"
MAINTAINER="ACME Corporation <support@acme-corp.example.com>"
DESCRIPTION="A cross-platform widget toolkit for managing and deploying ACME products."
INSTALL_PREFIX="/opt/acme-widgets"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PAYLOAD_DIR="${SCRIPT_DIR}/payload"
TMP_DIR="${SCRIPT_DIR}/tmp/linux"
DIST_DIR="${SCRIPT_DIR}/dist"
STAGE_DIR="${TMP_DIR}/${PKG_FULL}"

STAGE_ONLY=false

# ── Parse arguments ────────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --clean)
            echo "Cleaning dist/ and tmp/ ..."
            rm -rf "${DIST_DIR}" "${TMP_DIR}"
            ;;
        --stage-only)
            STAGE_ONLY=true
            ;;
    esac
done

# ── Helpers ───────────────────────────────────────────────────────────────────
info()  { printf "\033[36m[INFO]\033[0m  %s\n" "$*"; }
ok()    { printf "\033[32m[ OK ]\033[0m  %s\n" "$*"; }
warn()  { printf "\033[33m[WARN]\033[0m  %s\n" "$*"; }
die()   { printf "\033[31m[FAIL]\033[0m  %s\n" "$*" >&2; exit 1; }

# ── Create staging tree ────────────────────────────────────────────────────────
info "Creating Debian package staging tree..."
rm -rf "${STAGE_DIR}"

INST="${STAGE_DIR}${INSTALL_PREFIX}"
mkdir -p "${INST}/bin"
mkdir -p "${INST}/etc"
mkdir -p "${INST}/share/acme-widgets"
mkdir -p "${INST}/share/doc/acme-widgets"

# ── Copy payload files ─────────────────────────────────────────────────────────
cp "${PAYLOAD_DIR}/bin/acme-widgets"                  "${INST}/bin/"
cp "${PAYLOAD_DIR}/etc/acme-widgets.conf"              "${INST}/etc/"
cp "${PAYLOAD_DIR}/share/acme-widgets/README.txt"       "${INST}/share/acme-widgets/"
cp "${PAYLOAD_DIR}/share/acme-widgets/CHANGELOG.txt"    "${INST}/share/acme-widgets/"
cp "${PAYLOAD_DIR}/share/acme-widgets/sample.dat"       "${INST}/share/acme-widgets/"
cp "${SCRIPT_DIR}/LICENSE.txt"                         "${INST}/share/doc/acme-widgets/copyright"

chmod +x "${INST}/bin/acme-widgets"

# ── Create symlink in /usr/local/bin ──────────────────────────────────────────
mkdir -p "${STAGE_DIR}/usr/local/bin"
# Use relative symlink for portability
ln -sf "${INSTALL_PREFIX}/bin/acme-widgets" "${STAGE_DIR}/usr/local/bin/acme-widgets"

# ── Calculate installed size (in KB) ──────────────────────────────────────────
INSTALLED_SIZE=$(du -sk "${STAGE_DIR}" 2>/dev/null | cut -f1)

# ── Write DEBIAN/control ──────────────────────────────────────────────────────
info "Writing DEBIAN/control..."
mkdir -p "${STAGE_DIR}/DEBIAN"

cat > "${STAGE_DIR}/DEBIAN/control" <<CTRL_EOF
Package: ${PACKAGE}
Version: ${VERSION}-${REVISION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Installed-Size: ${INSTALLED_SIZE}
Maintainer: ${MAINTAINER}
Homepage: https://www.acme-corp.example.com/widgets
Description: ${DESCRIPTION}
 Acme Widgets provides a cross-platform widget runtime for ACME products.
 .
 This package installs the acme-widgets CLI and default configuration.
 .
 Supported widgets:
  - acme-clock-widget: Desktop clock integration
  - acme-weather-widget: Live weather data
  - acme-stock-widget: Market data (optional, disabled by default)
CTRL_EOF

# ── Write DEBIAN/conffiles (config files, not clobbered on upgrade) ───────────
cat > "${STAGE_DIR}/DEBIAN/conffiles" <<EOF
${INSTALL_PREFIX}/etc/acme-widgets.conf
EOF

# ── Write DEBIAN/postinst ─────────────────────────────────────────────────────
cat > "${STAGE_DIR}/DEBIAN/postinst" <<'POSTINST_EOF'
#!/bin/sh
set -e

INSTALL_DIR="/opt/acme-widgets"

# Create log directory
mkdir -p /var/log/acme-widgets
chmod 755 /var/log/acme-widgets

# Set permissions
chown root:root   "${INSTALL_DIR}/bin/acme-widgets"
chmod 755         "${INSTALL_DIR}/bin/acme-widgets"

# Update ldconfig so any future .so files are found
ldconfig 2>/dev/null || true

echo "Acme Widgets 2.1.0 installed to ${INSTALL_DIR}"
echo "Run: acme-widgets --help"

exit 0
POSTINST_EOF
chmod 755 "${STAGE_DIR}/DEBIAN/postinst"

# ── Write DEBIAN/prerm ────────────────────────────────────────────────────────
cat > "${STAGE_DIR}/DEBIAN/prerm" <<'PRERM_EOF'
#!/bin/sh
set -e
# Stop service before removing
if [ -f "/opt/acme-widgets/bin/acme-widgets" ]; then
    /opt/acme-widgets/bin/acme-widgets stop 2>/dev/null || true
fi
exit 0
PRERM_EOF
chmod 755 "${STAGE_DIR}/DEBIAN/prerm"

# ── Write DEBIAN/postrm ───────────────────────────────────────────────────────
cat > "${STAGE_DIR}/DEBIAN/postrm" <<'POSTRM_EOF'
#!/bin/sh
set -e
case "$1" in
    purge)
        rm -rf /var/log/acme-widgets
        rm -rf /opt/acme-widgets
        ;;
esac
exit 0
POSTRM_EOF
chmod 755 "${STAGE_DIR}/DEBIAN/postrm"

ok "Staging tree created: ${STAGE_DIR}"

if $STAGE_ONLY; then
    echo ""
    info "Stage-only mode — skipping dpkg-deb build."
    echo "  Stage tree: ${STAGE_DIR}"
    echo "  To build manually: dpkg-deb --build \"${STAGE_DIR}\""
    exit 0
fi

# ── Build .deb ────────────────────────────────────────────────────────────────
mkdir -p "${DIST_DIR}"
DEB_OUT="${DIST_DIR}/${PKG_FULL}.deb"

if ! command -v dpkg-deb >/dev/null 2>&1; then
    warn "dpkg-deb not found — staging tree is ready at:"
    warn "  ${STAGE_DIR}"
    warn ""
    warn "To build on macOS:    brew install dpkg && dpkg-deb --build \"${STAGE_DIR}\""
    warn "To build on Linux:    dpkg-deb --build \"${STAGE_DIR}\""
    warn "Or use the miscc CLI: miscc --build-file=AcmeWidgets.mis --platform=linux"
    exit 0
fi

info "Building .deb package with dpkg-deb..."
dpkg-deb --build "${STAGE_DIR}" "${DEB_OUT}"
ok "Package built: ${DEB_OUT}"

# ── Verify ────────────────────────────────────────────────────────────────────
info "Verifying package..."
dpkg-deb --info "${DEB_OUT}"

# ── Summary ───────────────────────────────────────────────────────────────────
PKG_SIZE=$(du -sh "${DEB_OUT}" | cut -f1)
echo ""
echo "  ╔═══════════════════════════════════════════════════════════╗"
echo "  ║  Build Complete                                           ║"
echo "  ╠═══════════════════════════════════════════════════════════╣"
printf "  ║  Package:  %-46s ║\n" "$(basename "${DEB_OUT}")"
printf "  ║  Size:     %-46s ║\n" "${PKG_SIZE}"
printf "  ║  Location: %-46s ║\n" "dist/"
echo "  ╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "  To install (requires sudo):"
echo "    sudo dpkg -i \"${DEB_OUT}\""
echo ""
echo "  To inspect:"
echo "    dpkg-deb --contents \"${DEB_OUT}\""
echo "    dpkg-deb --info    \"${DEB_OUT}\""
echo ""
