#!/bin/sh
#
# autogen.sh — Bootstrap the autotools build system
# Run this once after a fresh clone, then: ./configure && make
#

set -e

# Detect glibtoolize (macOS Homebrew) vs libtoolize (Linux)
if command -v glibtoolize >/dev/null 2>&1; then
    LIBTOOLIZE=glibtoolize
elif command -v libtoolize >/dev/null 2>&1; then
    LIBTOOLIZE=libtoolize
else
    echo "ERROR: libtoolize (or glibtoolize) not found."
    echo "  macOS:  brew install libtool"
    echo "  Debian: apt-get install libtool"
    exit 1
fi

echo "==> autogen.sh: Bootstrapping Mcaster1InstallSystem..."
echo "    libtoolize: $LIBTOOLIZE"

$LIBTOOLIZE --force --copy
aclocal -I m4
autoheader
automake --add-missing --foreign --copy
autoconf

echo ""
echo "==> Bootstrap complete."
echo "    Run: ./configure && make studio"
echo ""
