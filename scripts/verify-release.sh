#!/bin/sh
# Refuse to call a release ready unless both downloads are there.
#
# Usage: sh scripts/verify-release.sh [dist-dir]
#
# Every release publishes two files: the .exe, and the same executable inside a
# .zip. The zip is not decoration - it is the second way in for a reader whose
# browser objects to a bare unsigned .exe, and the release notes list a checksum
# for it either way. A release that quietly went out with only one of them would
# still look finished, which is exactly why this is a check and not a habit.
#
# package-release.sh ends by running this, so the packaging step cannot report
# success while the pair is incomplete.
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

[ -f "$EXE" ] || fail "no $EXE"
[ -f "$ZIP" ] || fail "no $ZIP - the zip is published beside the exe, always"

[ -s "$EXE" ] || fail "$EXE is empty"
[ -s "$ZIP" ] || fail "$ZIP is empty"

# The zip has to hold the executable, not merely exist: a zip built from an
# empty staging directory is the failure this catches.
if command -v python3 >/dev/null 2>&1; then
    python3 - "$ZIP" <<'PY'
import sys, zipfile
zip_path = sys.argv[1]
with zipfile.ZipFile(zip_path) as z:
    names = z.namelist()
    if "hgfloater.exe" not in names:
        sys.exit("verify-release: %s does not contain hgfloater.exe (%s)" % (zip_path, names))
    if z.getinfo("hgfloater.exe").file_size <= 0:
        sys.exit("verify-release: hgfloater.exe inside %s is empty" % zip_path)
PY
fi

echo "verify: OK both downloads are present ($(basename "$EXE"), $(basename "$ZIP"))"
echo "verify: publish BOTH - a release with only the exe is an incomplete release"
