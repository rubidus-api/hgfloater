#!/bin/sh
# Refuse to call a release ready unless the built executable is in dist/.
#
# Usage: sh scripts/verify-release.sh [dist-dir]
#
# The release artifact is dist/hgfloater.exe - the build output itself, plain,
# not wrapped in anything. That is what a reader downloads and runs, so that is
# what this checks: present, non-empty, and carrying the version on the label.
#
# The zip is a convenience beside it, for a browser that objects to a bare
# unsigned .exe. It is packaged when it can be, and its absence is worth a line
# of output, but the exe is the file a release cannot go out without.
#
# package-release.sh ends by running this, so packaging cannot report success
# while dist/ is missing the executable.
set -e

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

DIST=${1:-dist}
VERSION=$(cat VER.txt | tr -d '\r\n')
EXE="$DIST/hgfloater.exe"
ZIP="$DIST/hgfloater-$VERSION.zip"

fail() {
    echo "verify-release: $1" >&2
    exit 1
}

[ -f "$EXE" ] || fail "no $EXE - the built exe is what a release publishes"
[ -s "$EXE" ] || fail "$EXE is empty"

# The version in the file has to be the version on the label; a binary left over
# from the previous release is the failure this catches. HG_VERSION_W is a wide
# string, so the bytes to look for are UTF-16.
if command -v python3 >/dev/null 2>&1; then
    python3 -c 'import sys
exe, version = sys.argv[1], sys.argv[2]
with open(exe, "rb") as f:
    if version.encode("utf-16-le") not in f.read():
        sys.exit("verify-release: %s does not carry %s - rebuild it" % (exe, version))' "$EXE" "$VERSION" || exit 1
fi

# wc -c, not du: du reports blocks on disk, and a compressing filesystem
# cheerfully answered "1 KB" for a 600 KB binary.
echo "verify: OK $EXE ($(( ($(wc -c < "$EXE") + 1023) / 1024 )) KB, $VERSION)"

if [ -f "$ZIP" ] && [ -s "$ZIP" ]; then
    echo "verify: zip alongside it ($(basename "$ZIP"))"
else
    echo "verify: no zip this time - the exe is the release artifact"
fi
