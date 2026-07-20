#!/bin/sh
# Cross-build verification for hgfloater on a Linux host with mingw-w64.
# Usage: sh scripts/build-mingw.sh [output-dir]
# Builds a release-flag, warning-clean executable, compiles every test with the
# same warning set, and runs the host-native tests with the system compiler.
#
# The source list, flags, and version string all live in the Makefile; this
# script only picks the cross toolchain and the output directory, so the two
# cannot drift apart.
set -e

CROSS=${CROSS:-x86_64-w64-mingw32}
OUT=${1:-build-mingw}

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

make CROSS="$CROSS-" OUT="$OUT" release
make CROSS="$CROSS-" OUT="$OUT" test-compile

# Windows binaries only run here when wine is installed; the Win32-free units
# always run through the Makefile's host-test target below.
if command -v wine >/dev/null 2>&1; then
    for t in test/*.c; do
        n=$(basename "$t" .c)
        wine "$OUT/$n.exe" >/dev/null && echo "test run (wine): OK $n"
    done
fi

make OUT="$OUT" test-host
