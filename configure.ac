dnl configure.ac — Mcaster1InstallSystem autoconf script
dnl Process with: autoconf (or via autogen.sh)

AC_PREREQ([2.69])
AC_INIT([Mcaster1InstallSystem], [1.0.0], [bugs@mcaster1.com], [mcaster1installsystem])
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

# ── C++ compiler ──────────────────────────────────────────────────────────────
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
AC_PROG_RANLIB

# Require C++17
AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])

# ── Platform detection ────────────────────────────────────────────────────────
AC_CANONICAL_HOST

IS_MACOS=no
IS_WINDOWS=no
IS_LINUX=no

case "$host_os" in
  darwin*)
    IS_MACOS=yes
    AC_DEFINE([PLATFORM_MACOS], [1], [Building on macOS])
    ;;
  mingw* | cygwin* | msys*)
    IS_WINDOWS=yes
    AC_DEFINE([PLATFORM_WINDOWS], [1], [Building on Windows])
    ;;
  linux*)
    IS_LINUX=yes
    AC_DEFINE([PLATFORM_LINUX], [1], [Building on Linux])
    ;;
  *)
    AC_MSG_WARN([Unknown platform: $host_os — treating as Linux])
    IS_LINUX=yes
    AC_DEFINE([PLATFORM_LINUX], [1], [Building on Linux])
    ;;
esac

AM_CONDITIONAL([MACOS],   [test "$IS_MACOS"   = yes])
AM_CONDITIONAL([WINDOWS], [test "$IS_WINDOWS" = yes])
AM_CONDITIONAL([LINUX],   [test "$IS_LINUX"   = yes])

# ── CMake (required for Qt6 sub-builds) ───────────────────────────────────────
AC_PATH_PROG([CMAKE], [cmake], [no])
if test "$CMAKE" = no; then
  AC_MSG_ERROR([cmake is required to build the Qt6 studio and runtime.
  macOS:  brew install cmake
  Linux:  apt-get install cmake  (or package manager of choice)])
fi
AC_MSG_CHECKING([cmake version])
CMAKE_VERSION=$("$CMAKE" --version 2>/dev/null | head -1 | awk '{print $3}')
AC_MSG_RESULT([$CMAKE_VERSION])

# ── Qt6 (via pkg-config — validate environment at configure time) ─────────────
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([QT6WIDGETS],    [Qt6Widgets    >= 6.0],
  [AC_MSG_RESULT([Qt6Widgets: OK])],
  [AC_MSG_WARN([Qt6Widgets not found via pkg-config — cmake will verify at build time])])

PKG_CHECK_MODULES([QT6SVG],        [Qt6Svg        >= 6.0],
  [AC_MSG_RESULT([Qt6Svg: OK])],
  [AC_MSG_WARN([Qt6Svg not found — install qt6-svg or brew install qt])])

PKG_CHECK_MODULES([QT6CONCURRENT], [Qt6Concurrent >= 6.0],
  [AC_MSG_RESULT([Qt6Concurrent: OK])],
  [AC_MSG_WARN([Qt6Concurrent not found])])

# Locate moc (used to verify Qt toolchain is functional)
AC_PATH_PROG([MOC], [moc], [no])
if test "$MOC" = no; then
  AC_MSG_WARN([moc not found in PATH — Qt6 from Homebrew: add \$(brew --prefix qt)/bin to PATH])
fi

# ── macOS-specific tools ──────────────────────────────────────────────────────
if test "$IS_MACOS" = yes; then
  AC_PATH_PROG([MACDEPLOYQT], [macdeployqt], [no])
  if test "$MACDEPLOYQT" = no; then
    AC_MSG_WARN([macdeployqt not found — needed for DMG creation. Install Homebrew Qt.])
  fi
  AC_PATH_PROG([HDIUTIL],     [hdiutil],     [no])
  AC_PATH_PROG([CODESIGN],    [codesign],    [no])
  AC_SUBST([MACDEPLOYQT])
  AC_SUBST([HDIUTIL])
fi

# ── Linux packaging tools (optional) ─────────────────────────────────────────
if test "$IS_LINUX" = yes; then
  AC_PATH_PROG([DPKG_DEB], [dpkg-deb], [no])
  AC_PATH_PROG([RPMBUILD],  [rpmbuild],  [no])
  AC_SUBST([DPKG_DEB])
  AC_SUBST([RPMBUILD])
fi

# ── Version info ──────────────────────────────────────────────────────────────
PACKAGE_VERSION_FULL="$(cat VERSION 2>/dev/null | head -1 | tr -d '[:space:]')"
test -z "$PACKAGE_VERSION_FULL" && PACKAGE_VERSION_FULL="1.0.0"
AC_SUBST([PACKAGE_VERSION_FULL])
AC_DEFINE_UNQUOTED([PACKAGE_VERSION_FULL], ["$PACKAGE_VERSION_FULL"],
  [Full version string from VERSION file])

# ── Output ────────────────────────────────────────────────────────────────────
AC_CONFIG_FILES([
  Makefile
  manifest/Makefile
  backends/Makefile
  importers/Makefile
])
AC_OUTPUT

echo ""
echo "=== Mcaster1InstallSystem Configuration ==="
echo ""
echo "  Version:    $PACKAGE_VERSION_FULL"
echo "  Platform:   $host_os"
echo "  C++:        $CXX"
echo "  CMake:      $CMAKE_VERSION"
echo "  Qt6 MOC:    $MOC"
if test "$IS_MACOS" = yes; then
echo "  macdeployqt:$MACDEPLOYQT"
fi
echo ""
echo "  Build targets:"
echo "    make studio          — build the Install Studio app"
echo "    make studio-release  — optimised release build"
echo "    make runtime         — build the generic runtime installer wizard"
echo "    make all-apps        — build both studio and runtime"
if test "$IS_MACOS" = yes; then
echo "    make dmg             — create distributable DMG (requires --dmg)"
fi
echo ""
echo "  Run: make studio && open studio/build/Mcaster1InstallStudio.app"
echo ""
