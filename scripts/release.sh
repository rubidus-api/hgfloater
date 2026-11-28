#!/bin/sh
# The whole release build, as one command.
#
# Usage: sh scripts/release.sh [dist-dir]
#
# There is exactly one way to produce what a release publishes, and it ends with
# the built executable sitting in dist/ as dist/hgfloater.exe - plain, the same
# bytes the compiler produced, nothing wrapped around it. A release build that
# left dist/ empty would still look finished, so this script is the step, and it
# fails rather than finishing without the exe.
#
# What it does, in order:
#
#   1. build   - warning-clean cross build, every test compiled, host tests run
#   2. package - stage the exe into dist/, write its version, date and size into
#                the download block of both READMEs, print its checksum, verify
#
# One pass is enough: About renders src/help.md, which names no size, so the
# READMEs written in 2 do not reach the binary. (Until the help was split from
# the README there were two passes, so the measured size could be compiled in.)
#
# Set HG_RELEASE_ZIP=1 to also drop a zip of the same exe beside it, for a
# browser that objects to a bare unsigned .exe. Off by default: the exe is the
# download.
#
# The checksum printed at the end is the one for the release notes.
set -e

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

DIST=${1:-dist}
VERSION=$(cat VER.txt | tr -d '\r\n')

rm -rf "$DIST"

echo "release: $VERSION - build and package"
sh scripts/build-mingw.sh build-mingw
sh scripts/package-release.sh "$DIST"

# package-release.sh verifies as it goes; asked again here because this script
# is what a release is cut from, and the last word on "is it complete" belongs
# to the thing that says the release is ready.
sh scripts/verify-release.sh "$DIST"

echo
echo "release: $VERSION is ready in $DIST/"
ls -l "$DIST"
echo "release: upload $DIST/hgfloater.exe with the checksum above"
