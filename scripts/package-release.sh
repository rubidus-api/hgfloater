#!/bin/sh
# Put the built executable in dist/ for release, and print the checksum that
# belongs in the release notes.
#
# Usage: sh scripts/package-release.sh [dist-dir]
#
# The release artifact is the exe itself: dist/hgfloater.exe, the same bytes the
# build produced, plain. That is what the release publishes and what a reader
# downloads and runs.
#
# A zip of the same exe can be made alongside it by setting HG_RELEASE_ZIP=1 -
# it exists only as a second way in for a browser that objects to a bare
# unsigned .exe. It is off by default: the exe is the download.
#
# Why the checksum is printed: an unsigned binary that warns the user has to
# offer some way of verifying it is the file the developer built. Publishing the
# SHA-256 in the release notes is that way, and it is also the evidence a
# false-positive report is argued from.
set -e

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

DIST=${1:-dist}
EXE="$DIST/hgfloater.exe"
VERSION=$(cat VER.txt | tr -d '\r\n')

# The build writes build-mingw/hgfloater.exe and nothing else; staging it here
# is the packaging step's job. Doing it any other way once put a stale binary
# from the previous release under a correctly named label, with a checksum that
# was perfectly accurate about the wrong file.
BUILT=build-mingw/hgfloater.exe
if [ -f "$BUILT" ]; then
    mkdir -p "$DIST"
    cp -f "$BUILT" "$EXE"
fi

[ -f "$EXE" ] || { echo "package-release: no $EXE and no $BUILT - build it first" >&2; exit 1; }

# The README claims a download size; the download is right here, so measure it
# rather than trusting a number someone typed once. About renders the README, so
# a change here reaches it at the next build.
sh scripts/update-exe-size.sh "$EXE"

echo "package: OK $EXE"

ZIP="$DIST/hgfloater-$VERSION.zip"
rm -f "$ZIP"

if [ "${HG_RELEASE_ZIP:-0}" = "1" ]; then
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
        echo "package-release: HG_RELEASE_ZIP=1 but neither zip nor python3 found" >&2
        exit 1
    fi
    echo "package: OK $ZIP (extra, HG_RELEASE_ZIP=1)"
fi

# The exe has to be there, non-empty, and carrying this version. Checked rather
# than remembered.
sh scripts/verify-release.sh "$DIST"

echo
echo "--- checksums for the release notes ---"
printf 'hgfloater.exe            SHA-256  %s\n' "$(sha256sum "$EXE" | cut -d' ' -f1)"
if [ -f "$ZIP" ]; then
    printf 'hgfloater-%s.zip  SHA-256  %s\n' "$VERSION" "$(sha256sum "$ZIP" | cut -d' ' -f1)"
fi
