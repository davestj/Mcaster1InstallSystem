#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# assemble-payload.sh — Stage Mcaster1DNAS build artifacts into payload/
#
# Run from:  Mcaster1InstallSystem/scripts/mcaster1/Mcaster1DNAS/
# or set MCASTER1_SRC below to the full path of the mcaster1dnas repo.
#
# After this script completes, load mcaster1dnas-macos.mis in Mcaster1 Install
# Studio and click Build to produce the macOS DMG installer.
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MCASTER1_SRC="${MCASTER1_SRC:-/Users/dstjohn/dev/01_mcaster1.com/mcaster1dnas}"
PAYLOAD_DIR="${SCRIPT_DIR}/payload"

# ── Validate source ───────────────────────────────────────────────────────────
if [[ ! -d "${MCASTER1_SRC}" ]]; then
    echo "ERROR: mcaster1dnas source not found at: ${MCASTER1_SRC}"
    echo "  Set MCASTER1_SRC=/path/to/mcaster1dnas and re-run."
    exit 1
fi

APP_BUNDLE="${MCASTER1_SRC}/macos/build-qt/Mcaster1DNAS.app"
if [[ ! -d "${APP_BUNDLE}" ]]; then
    echo "ERROR: Mcaster1DNAS.app not found at: ${APP_BUNDLE}"
    echo ""
    echo "  Build it first:"
    echo "    cd ${MCASTER1_SRC}"
    echo "    make gui"
    echo ""
    exit 1
fi

# ── Version stamp ─────────────────────────────────────────────────────────────
VERSION="$(cat "${MCASTER1_SRC}/VERSION" 2>/dev/null || echo "2.5.3-beta")"
echo "Assembling payload for Mcaster1DNAS ${VERSION}"
echo "  Source: ${MCASTER1_SRC}"
echo "  Payload: ${PAYLOAD_DIR}"
echo ""

# ── Clean + recreate payload ──────────────────────────────────────────────────
rm -rf "${PAYLOAD_DIR}"
mkdir -p "${PAYLOAD_DIR}"
mkdir -p "${PAYLOAD_DIR}/ssl"
mkdir -p "${PAYLOAD_DIR}/logs"

# ── Copy application bundle ───────────────────────────────────────────────────
echo "[1/6] Copying Mcaster1DNAS.app…"
cp -R "${APP_BUNDLE}" "${PAYLOAD_DIR}/Mcaster1DNAS.app"
echo "      $(du -sh "${PAYLOAD_DIR}/Mcaster1DNAS.app" | cut -f1)  Mcaster1DNAS.app"

# ── Copy YAML configs ─────────────────────────────────────────────────────────
echo "[2/6] Copying configuration files…"
for cfg in mcaster1dnas.yaml mcaster1dnas-console.yaml; do
    if [[ -f "${MCASTER1_SRC}/${cfg}" ]]; then
        cp "${MCASTER1_SRC}/${cfg}" "${PAYLOAD_DIR}/${cfg}"
        echo "      ${cfg}"
    else
        echo "      WARN: ${cfg} not found — skipping"
    fi
done

# ── Copy SSL cert ─────────────────────────────────────────────────────────────
echo "[3/6] Copying SSL certificate…"
if [[ -f "${MCASTER1_SRC}/ssl/localhost.pem" ]]; then
    cp "${MCASTER1_SRC}/ssl/localhost.pem" "${PAYLOAD_DIR}/ssl/localhost.pem"
    echo "      ssl/localhost.pem"
else
    echo "      WARN: ssl/localhost.pem not found — HTTPS will not work without it"
fi

# ── Copy web + admin interfaces ───────────────────────────────────────────────
echo "[4/6] Copying web + admin interfaces…"
for dir in web admin; do
    if [[ -d "${MCASTER1_SRC}/${dir}" ]]; then
        cp -R "${MCASTER1_SRC}/${dir}" "${PAYLOAD_DIR}/${dir}"
        echo "      ${dir}/  ($(find "${PAYLOAD_DIR}/${dir}" -type f | wc -l | tr -d ' ') files)"
    else
        echo "      WARN: ${dir}/ not found — skipping"
    fi
done

# ── Copy documentation ────────────────────────────────────────────────────────
echo "[5/6] Copying documentation…"
if [[ -d "${MCASTER1_SRC}/docs" ]]; then
    cp -R "${MCASTER1_SRC}/docs" "${PAYLOAD_DIR}/docs"
    echo "      docs/  ($(find "${PAYLOAD_DIR}/docs" -type f | wc -l | tr -d ' ') files)"
else
    echo "      WARN: docs/ not found — skipping"
fi

# ── Create logs placeholder ───────────────────────────────────────────────────
echo "[6/6] Creating logs directory placeholder…"
touch "${PAYLOAD_DIR}/logs/.keep"
echo "      logs/.keep"

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "────────────────────────────────────────────────────────────"
echo "  Payload assembled: ${PAYLOAD_DIR}"
echo "  Total size:        $(du -sh "${PAYLOAD_DIR}" | cut -f1)"
echo ""
echo "  Next steps:"
echo "    1. Open Mcaster1 Install Studio:"
echo "       open /Users/dstjohn/dev/01_mcaster1.com/Mcaster1InstallSystem/studio/build/Mcaster1InstallStudio.app"
echo "    2. File > Open Project > select:"
echo "       ${SCRIPT_DIR}/mcaster1dnas-macos.mis"
echo "    3. Review App Info + Security tabs"
echo "    4. Click Build  (Ctrl+B)"
echo "────────────────────────────────────────────────────────────"
