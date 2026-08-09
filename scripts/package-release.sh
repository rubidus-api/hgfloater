#!/bin/sh
# Package a built dist/ for release: a .zip beside the .exe, and the checksums
# that belong in the release notes.
#
# Usage: sh scripts/package-release.sh [dist-dir]
#
# Why the zip exists at all: a bare .exe served over HTTPS is the shape browser
# download-protection reacts to most strongly, and this binary is unsigned, so
# it starts with no reputation to offset that. The same bytes inside a zip give
# a reader who hits a warning a second way through. The exe stays the primary
# download; the zip is the alternative, not a replacement.
#
# Why the checksums are printed: an unsigned binary that warns the user has to
# offer some way of verifying it is the file the developer built. Publishing
# the SHA-256 in the release notes is that way, and it is also the evidence a
# false-positive report is argued from.
set -e

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

DIST=${1:-dist}
EXE="$DIST/hgfloater.exe"
VERSION=$(cat VER.txt | tr -d '\r\n')

[ -f "$EXE" ] || { echo "package-release: no $EXE - build it first" >&2; exit 1; }

ZIP="$DIST/hgfloater-$VERSION.zip"
rm -f "$ZIP"

# Zipped from inside dist/ so the archive holds hgfloater.exe at its root and
# no directory to dig through. python3 is the fallback because the build host
# and the release host are not always the same machine, and only one of them
# is guaranteed to have zip(1).
if command -v zip >/dev/null 2>&1; then
    (cd "$DIST" && zip -q -X "$(basename "$ZIP")" hgfloater.exe)
elif command -v python3 >/dev/null 2>&1; then
    python3 - "$DIST" "$ZIP" <<'PY'
import sys, zipfile, os
dist, zip_path = sys.argv[1], sys.argv[2]
exe = os.path.join(dist, "hgfloater.exe")
with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
    z.write(exe, "hgfloater.exe")
PY
else
    echo "package-release: neither zip nor python3 found" >&2
    exit 1
fi

echo "package: OK $ZIP"
echo
echo "--- checksums for the release notes ---"
printf 'hgfloater.exe            SHA-256  %s\n' "$(sha256sum "$EXE" | cut -d' ' -f1)"
printf 'hgfloater-%s.zip  SHA-256  %s\n' "$VERSION" "$(sha256sum "$ZIP" | cut -d' ' -f1)"
