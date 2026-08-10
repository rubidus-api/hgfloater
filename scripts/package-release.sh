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

# The build writes build-mingw/hgfloater.exe and nothing else; staging it here
# is the packaging step's job. Doing it any other way once put a stale binary
# from the previous release inside a correctly named zip, with a checksum that
# was perfectly accurate about the wrong file.
BUILT=build-mingw/hgfloater.exe
if [ -f "$BUILT" ]; then
    mkdir -p "$DIST"
    cp -f "$BUILT" "$EXE"
fi

[ -f "$EXE" ] || { echo "package-release: no $EXE and no $BUILT - build it first" >&2; exit 1; }

# The version in the file has to be the version on the label. A binary built
# before the last version bump is the failure this catches. HG_VERSION_W is a
# wide string, so the bytes to look for are UTF-16.
if command -v python3 >/dev/null 2>&1; then
    python3 -c 'import sys
exe, version = sys.argv[1], sys.argv[2]
with open(exe, "rb") as f:
    if version.encode("utf-16-le") not in f.read():
        sys.exit("package-release: %s does not carry %s - rebuild it" % (exe, version))' "$EXE" "$VERSION" || exit 1
fi

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
