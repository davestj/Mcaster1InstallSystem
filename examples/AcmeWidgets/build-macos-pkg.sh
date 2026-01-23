#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────────
# build-macos-pkg.sh  —  Build the ACME Widgets 2.1.0 macOS .pkg installer
#
# Produces:  dist/AcmeWidgets-2.1.0-macos.pkg
#
# Requirements:
#   - macOS 12+ with Xcode Command Line Tools (pkgbuild + productbuild)
#   - No code-signing certificate required (runs --sign flag is optional)
#
# Usage:
#   ./build-macos-pkg.sh              # unsigned (dev/testing)
#   ./build-macos-pkg.sh --sign "Developer ID Installer: Your Name (TEAMID)"
#   ./build-macos-pkg.sh --clean      # remove dist/ and tmp/ then build
# ──────────────────────────────────────────────────────────────────────────────
set -euo pipefail

# ── Configuration ─────────────────────────────────────────────────────────────
APP_NAME="Acme Widgets"
APP_ID="com.acme-corp.acme-widgets"
APP_VERSION="2.1.0"
PKG_NAME="AcmeWidgets-${APP_VERSION}-macos"
INSTALL_LOCATION="/Applications/ACME Widgets"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PAYLOAD_DIR="${SCRIPT_DIR}/payload"
RESOURCES_DIR="${SCRIPT_DIR}/resources"
TMP_DIR="${SCRIPT_DIR}/tmp/macos"
DIST_DIR="${SCRIPT_DIR}/dist"
PKG_OUT="${DIST_DIR}/${PKG_NAME}.pkg"
COMPONENT_PKG="${TMP_DIR}/AcmeWidgets-component.pkg"

SIGN_IDENTITY=""

# ── Parse arguments ────────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --clean)
            echo "Cleaning dist/ and tmp/ ..."
            rm -rf "${DIST_DIR}" "${TMP_DIR}"
            ;;
        --sign)
            shift
            SIGN_IDENTITY="${1:-}"
            ;;
        --sign=*)
            SIGN_IDENTITY="${arg#--sign=}"
            ;;
    esac
done

# ── Helpers ───────────────────────────────────────────────────────────────────
info()  { printf "\033[36m[INFO]\033[0m  %s\n" "$*"; }
ok()    { printf "\033[32m[ OK ]\033[0m  %s\n" "$*"; }
warn()  { printf "\033[33m[WARN]\033[0m  %s\n" "$*"; }
die()   { printf "\033[31m[FAIL]\033[0m  %s\n" "$*" >&2; exit 1; }

# ── Preflight ─────────────────────────────────────────────────────────────────
info "Checking prerequisites..."
command -v pkgbuild     >/dev/null 2>&1 || die "pkgbuild not found (install Xcode CLT)"
command -v productbuild >/dev/null 2>&1 || die "productbuild not found (install Xcode CLT)"
ok "pkgbuild and productbuild are available"

# ── Create stage tree ─────────────────────────────────────────────────────────
info "Staging payload to ${TMP_DIR}/stage ..."
STAGE_DIR="${TMP_DIR}/stage"
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}/bin"
mkdir -p "${STAGE_DIR}/etc"
mkdir -p "${STAGE_DIR}/share/acme-widgets"

# Copy payload files
cp "${PAYLOAD_DIR}/bin/acme-widgets"             "${STAGE_DIR}/bin/"
cp "${PAYLOAD_DIR}/etc/acme-widgets.conf"         "${STAGE_DIR}/etc/"
cp "${PAYLOAD_DIR}/share/acme-widgets/README.txt"   "${STAGE_DIR}/share/acme-widgets/"
cp "${PAYLOAD_DIR}/share/acme-widgets/CHANGELOG.txt" "${STAGE_DIR}/share/acme-widgets/"
cp "${PAYLOAD_DIR}/share/acme-widgets/sample.dat"   "${STAGE_DIR}/share/acme-widgets/"

# Ensure executable bits
chmod +x "${STAGE_DIR}/bin/acme-widgets"
ok "Payload staged ($(find "${STAGE_DIR}" -type f | wc -l | tr -d ' ') files)"

# ── Build scripts (pre/post install) ──────────────────────────────────────────
info "Creating installer scripts..."
SCRIPTS_DIR="${TMP_DIR}/scripts"
mkdir -p "${SCRIPTS_DIR}"

cat > "${SCRIPTS_DIR}/preinstall" <<'EOF'
#!/bin/bash
# Pre-install: stop any running widget service
if [ -f "/Applications/ACME Widgets/bin/acme-widgets" ]; then
    /Applications/ACME\ Widgets/bin/acme-widgets stop 2>/dev/null || true
fi
exit 0
EOF

cat > "${SCRIPTS_DIR}/postinstall" <<'POSTEOF'
#!/bin/bash
# Post-install: set correct ownership and permissions
INSTALL_DIR="/Applications/ACME Widgets"
chown -R root:wheel   "${INSTALL_DIR}" 2>/dev/null || true
chmod -R 755          "${INSTALL_DIR}" 2>/dev/null || true
chmod +x              "${INSTALL_DIR}/bin/acme-widgets" 2>/dev/null || true

# Create log directory
mkdir -p /var/log/acme-widgets
chmod 755 /var/log/acme-widgets

# macOS notification
osascript -e 'display notification "Acme Widgets 2.1.0 installed successfully." with title "Installation Complete"' 2>/dev/null || true

exit 0
POSTEOF

chmod +x "${SCRIPTS_DIR}/preinstall" "${SCRIPTS_DIR}/postinstall"
ok "Installer scripts created"

# ── Build component .pkg ───────────────────────────────────────────────────────
info "Building component package with pkgbuild..."
mkdir -p "${TMP_DIR}"
pkgbuild \
    --root        "${STAGE_DIR}" \
    --identifier  "${APP_ID}" \
    --version     "${APP_VERSION}" \
    --install-location "${INSTALL_LOCATION}" \
    --scripts     "${SCRIPTS_DIR}" \
    "${COMPONENT_PKG}"
ok "Component package: ${COMPONENT_PKG}"

# ── Create distribution resources ─────────────────────────────────────────────
info "Preparing distribution resources..."
RES_TMP="${TMP_DIR}/distresources"
mkdir -p "${RES_TMP}"

# Welcome page
cat > "${RES_TMP}/Welcome.html" <<WELCOME_EOF
<!DOCTYPE html>
<html>
<head><meta charset="utf-8">
<style>
  body { font-family: -apple-system, Helvetica, Arial, sans-serif;
         font-size: 13px; color: #1a1a2e; padding: 16px; }
  h1   { font-size: 18px; color: #0078d4; margin-bottom: 8px; }
  p    { line-height: 1.6; }
  .note { color: #555; font-size: 11px; margin-top: 12px; }
</style>
</head>
<body>
<h1>Welcome to Acme Widgets 2.1.0</h1>
<p>This installer will guide you through installing <strong>Acme Widgets</strong>
from ACME Corporation on your Mac.</p>
<p>Acme Widgets is a cross-platform widget toolkit for managing and deploying
ACME products. This release includes the widget runtime, default configuration,
and documentation.</p>
<p class="note">This is an example installer created with the Mcaster1 Install System.
The application binaries are demonstration stubs — no real services are started.</p>
</body>
</html>
WELCOME_EOF

# Conclusion page
cat > "${RES_TMP}/Conclusion.html" <<CONCL_EOF
<!DOCTYPE html>
<html>
<head><meta charset="utf-8">
<style>
  body { font-family: -apple-system, Helvetica, Arial, sans-serif;
         font-size: 13px; color: #1a1a2e; padding: 16px; }
  h1   { font-size: 18px; color: #0078d4; margin-bottom: 8px; }
  p    { line-height: 1.6; }
  code { background: #f0f4ff; padding: 2px 6px; border-radius: 3px; }
</style>
</head>
<body>
<h1>Installation Complete</h1>
<p><strong>Acme Widgets 2.1.0</strong> has been installed to
<code>/Applications/ACME Widgets</code>.</p>
<p>To get started, open Terminal and run:</p>
<p><code>"/Applications/ACME Widgets/bin/acme-widgets" --help</code></p>
<p>Documentation is available at:<br>
<code>/Applications/ACME Widgets/share/acme-widgets/README.txt</code></p>
</body>
</html>
CONCL_EOF

# Copy license
cp "${SCRIPT_DIR}/LICENSE.txt" "${RES_TMP}/LICENSE.txt"

# Copy banner image if available
if [ -f "${RESOURCES_DIR}/banner.png" ]; then
    cp "${RESOURCES_DIR}/banner.png" "${RES_TMP}/banner.png"
fi

ok "Distribution resources prepared"

# ── Build distribution XML ─────────────────────────────────────────────────────
info "Generating Distribution.xml..."
DIST_XML="${TMP_DIR}/Distribution.xml"

cat > "${DIST_XML}" <<DISTEOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">

    <!-- Application metadata -->
    <title>Acme Widgets 2.1.0</title>
    <organization>com.acme-corp</organization>
    <domains enable_anywhere="false"
             enable_currentUserHome="false"
             enable_localSystem="true"/>
    <options customize="allow"
             allow-external-scripts="false"
             rootVolumeOnly="false"
             hostArchitectures="arm64 x86_64"/>

    <!-- Wizard pages -->
    <welcome    file="Welcome.html"    mime-type="text/html"/>
    <license    file="LICENSE.txt"     mime-type="text/plain"/>
    <conclusion file="Conclusion.html" mime-type="text/html"/>

    <!-- Component choices -->
    <choice id="com.acme-corp.acme-widgets.core"
            title="Acme Widgets Application"
            description="The main acme-widgets binary, default configuration and shared data files. Required."
            enabled="false"
            selected="true">
        <pkg-ref id="${APP_ID}"/>
    </choice>

    <!-- Choices outline (installer sidebar) -->
    <choices-outline>
        <line choice="com.acme-corp.acme-widgets.core"/>
    </choices-outline>

    <!-- Package reference -->
    <pkg-ref id="${APP_ID}"
             version="${APP_VERSION}"
             onConclusion="none">AcmeWidgets-component.pkg</pkg-ref>

</installer-gui-script>
DISTEOF

ok "Distribution.xml created"

# ── Build final .pkg ───────────────────────────────────────────────────────────
info "Building final distribution package with productbuild..."
mkdir -p "${DIST_DIR}"

PRODUCTBUILD_ARGS=(
    --distribution  "${DIST_XML}"
    --resources     "${RES_TMP}"
    --package-path  "${TMP_DIR}"
)

if [ -n "${SIGN_IDENTITY}" ]; then
    PRODUCTBUILD_ARGS+=(--sign "${SIGN_IDENTITY}")
    info "Signing with: ${SIGN_IDENTITY}"
fi

PRODUCTBUILD_ARGS+=("${PKG_OUT}")

productbuild "${PRODUCTBUILD_ARGS[@]}"
ok "Package built: ${PKG_OUT}"

# ── Summary ───────────────────────────────────────────────────────────────────
PKG_SIZE=$(du -sh "${PKG_OUT}" | cut -f1)
echo ""
echo "  ╔═══════════════════════════════════════════════════╗"
echo "  ║  Build Complete                                   ║"
echo "  ╠═══════════════════════════════════════════════════╣"
printf "  ║  Package:  %-38s ║\n" "$(basename "${PKG_OUT}")"
printf "  ║  Size:     %-38s ║\n" "${PKG_SIZE}"
printf "  ║  Location: %-38s ║\n" "dist/"
echo "  ╚═══════════════════════════════════════════════════╝"
echo ""
echo "  To install (double-click or):"
echo "    open \"${PKG_OUT}\""
echo ""
echo "  To inspect the package contents:"
echo "    pkgutil --expand \"${PKG_OUT}\" /tmp/acme-expanded"
echo ""
