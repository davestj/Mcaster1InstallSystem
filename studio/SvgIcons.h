#pragma once
/*
 * SvgIcons.h — Inline SVG icon strings for the Install Studio UI.
 *
 * All icons are 24×24 viewBox unless otherwise noted.
 * Color variables:  --accent #00c9ff   --fg #e0e0e8   --bg2 #16213e
 */

namespace SvgIcons {

// ── App logo (48×48) — abstract "M1" package/wrench motif ────────────────────
static constexpr const char *kLogo = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <defs>
    <linearGradient id="lg" x1="0" y1="0" x2="1" y2="1">
      <stop offset="0%"   stop-color="#0f3460"/>
      <stop offset="100%" stop-color="#00c9ff"/>
    </linearGradient>
  </defs>
  <rect x="2" y="2" width="44" height="44" rx="10" ry="10" fill="url(#lg)" opacity="0.92"/>
  <!-- Box outline -->
  <path d="M10 30 L10 18 L24 11 L38 18 L38 30 L24 37 Z"
        fill="none" stroke="#00c9ff" stroke-width="1.8" opacity="0.85"/>
  <!-- Box lid -->
  <path d="M10 18 L24 25 L38 18" fill="none" stroke="#00c9ff" stroke-width="1.4" opacity="0.7"/>
  <!-- M1 text -->
  <text x="24" y="29" text-anchor="middle" font-family="system-ui,sans-serif"
        font-weight="700" font-size="9" fill="#ffffff" letter-spacing="0.5">M1</text>
  <!-- Wrench accent -->
  <path d="M33 10 Q36 7 39 10 L34.5 14.5 L36 16 L31.5 20.5 L30 19 L25.5 23.5
           Q22 26 19 23 Q16 20 19 17 Q22 14 25.5 17.5 L30 13 L31.5 14.5 Z"
        fill="#00c9ff" opacity="0.6" transform="scale(0.45) translate(24,2)"/>
</svg>
)svg";

// ── New project ───────────────────────────────────────────────────────────────
static constexpr const char *kNew = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <path d="M6 2h8l6 6v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2z"
        stroke="#e0e0e8" stroke-width="1.6" fill="#16213e"/>
  <path d="M14 2v6h6" stroke="#e0e0e8" stroke-width="1.6" fill="none"/>
  <line x1="12" y1="11" x2="12" y2="17" stroke="#00c9ff" stroke-width="2" stroke-linecap="round"/>
  <line x1="9"  y1="14" x2="15" y2="14" stroke="#00c9ff" stroke-width="2" stroke-linecap="round"/>
</svg>
)svg";

// ── Open project ──────────────────────────────────────────────────────────────
static constexpr const char *kOpen = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"
        stroke="#e0e0e8" stroke-width="1.6" fill="#16213e"/>
  <path d="M5 19l3-8h14l-3 8" stroke="#00c9ff" stroke-width="1.4" fill="#0f3460" opacity="0.8"/>
</svg>
)svg";

// ── Save ──────────────────────────────────────────────────────────────────────
static constexpr const char *kSave = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"
        stroke="#e0e0e8" stroke-width="1.6" fill="#16213e"/>
  <polyline points="17 21 17 13 7 13 7 21" stroke="#e0e0e8" stroke-width="1.4"/>
  <polyline points="7 3 7 8 15 8"          stroke="#e0e0e8" stroke-width="1.4"/>
</svg>
)svg";

// ── Build / hammer ────────────────────────────────────────────────────────────
static constexpr const char *kBuild = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <path d="M15.5 2.1l-5 5A3 3 0 0 0 14 12l5-5a3 3 0 0 0-3.5-4.9z"
        stroke="#00c9ff" stroke-width="1.6" fill="#0f3460"/>
  <line x1="4" y1="20" x2="13" y2="11" stroke="#e0e0e8" stroke-width="3"
        stroke-linecap="round"/>
  <line x1="19" y1="6" x2="22" y2="3" stroke="#00c9ff" stroke-width="1.6"
        stroke-linecap="round"/>
</svg>
)svg";

// ── macOS target (Apple silhouette) ───────────────────────────────────────────
static constexpr const char *kMacOs = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <path d="M17.05 20.28c-.98.95-2.05.8-3.08.35-1.09-.46-2.09-.48-3.24 0
           -1.44.62-2.2.44-3.06-.35C2.79 15.25 3.51 7.7 9.05 7.42
           c1.32.07 2.23.75 3.05.75.82 0 2.35-.93 3.96-.79
           1.62.13 2.84.75 3.64 1.88-3.16 1.86-2.65 5.96.35 7.02z"
        fill="#e0e0e8" opacity="0.85"/>
  <path d="M15.8 5.03c-2.12.26-3.88 2.27-3.6 4.26 2.07.16 3.9-1.82 3.6-4.26z"
        fill="#e0e0e8" opacity="0.65"/>
</svg>
)svg";

// ── Windows target ────────────────────────────────────────────────────────────
static constexpr const char *kWindows = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">
  <rect x="3"  y="3"  width="8.5" height="8.5" fill="#00a8e8" opacity="0.9"/>
  <rect x="12.5" y="3"  width="8.5" height="8.5" fill="#00a8e8" opacity="0.75"/>
  <rect x="3"  y="12.5" width="8.5" height="8.5" fill="#00a8e8" opacity="0.75"/>
  <rect x="12.5" y="12.5" width="8.5" height="8.5" fill="#00a8e8" opacity="0.6"/>
</svg>
)svg";

// ── Linux target (Tux-inspired penguin) ──────────────────────────────────────
static constexpr const char *kLinux = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <!-- body -->
  <ellipse cx="12" cy="14" rx="7" ry="8" fill="#e0e0e8" opacity="0.85"/>
  <!-- belly -->
  <ellipse cx="12" cy="15" rx="4" ry="5.5" fill="#f5c842" opacity="0.8"/>
  <!-- head -->
  <ellipse cx="12" cy="6" rx="5" ry="5" fill="#e0e0e8" opacity="0.85"/>
  <!-- eyes -->
  <circle cx="10" cy="5.5" r="1"   fill="#1a1a2e"/>
  <circle cx="14" cy="5.5" r="1"   fill="#1a1a2e"/>
  <circle cx="10.35" cy="5.15" r="0.3" fill="white"/>
  <circle cx="14.35" cy="5.15" r="0.3" fill="white"/>
  <!-- beak -->
  <ellipse cx="12" cy="8" rx="1.8" ry="1" fill="#f5c842"/>
</svg>
)svg";

// ── Package/box icon ──────────────────────────────────────────────────────────
static constexpr const char *kPackage = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8
           a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"
        stroke="#00c9ff" stroke-width="1.6" fill="#0f3460"/>
  <polyline points="3.27 6.96 12 12.01 20.73 6.96" stroke="#00c9ff" stroke-width="1.4"/>
  <line x1="12" y1="22.08" x2="12" y2="12" stroke="#00c9ff" stroke-width="1.4"/>
</svg>
)svg";

// ── Import script icon ────────────────────────────────────────────────────────
static constexpr const char *kImport = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"
        stroke="#e0e0e8" stroke-width="1.6" fill="#16213e"/>
  <polyline points="14 2 14 8 20 8" stroke="#e0e0e8" stroke-width="1.4"/>
  <line x1="16" y1="13" x2="8" y2="13" stroke="#00c9ff" stroke-width="1.4"/>
  <line x1="16" y1="17" x2="8" y2="17" stroke="#00c9ff" stroke-width="1.4"/>
  <polyline points="10 9 8 11 10 13" stroke="#f5c842" stroke-width="1.4"
            fill="none" stroke-linecap="round"/>
</svg>
)svg";

// ── Settings gear ─────────────────────────────────────────────────────────────
static constexpr const char *kSettings = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none">
  <circle cx="12" cy="12" r="3" stroke="#e0e0e8" stroke-width="1.6"/>
  <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06
           a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09
           A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83
           l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09
           A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83
           l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09
           a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83
           l-.06.06A1.65 1.65 0 0 0 19.4 9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09
           a1.65 1.65 0 0 0-1.51 1z"
        stroke="#e0e0e8" stroke-width="1.6"/>
</svg>
)svg";

} // namespace SvgIcons

// ── Helper: render an SVG string to a QPixmap at the given pixel size ─────────
// Must live OUTSIDE namespace SvgIcons because Qt headers cannot be included
// inside a user namespace (Q_DECLARE_TYPEINFO requires global scope).
// Include this only from .cpp files (needs Qt headers).
#ifdef QPIXMAP_H
#include <QSvgRenderer>
#include <QPainter>
inline QPixmap renderSvg(const char *svgStr, int size = 24)
{
    QByteArray data(svgStr);
    QSvgRenderer renderer(data);
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    renderer.render(&p);
    return px;
}
#endif
