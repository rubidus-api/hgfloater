#!/bin/sh
# Write the built executable's real size into both READMEs.
#
# Usage: sh scripts/update-exe-size.sh [exe]   (default: dist/hgfloater.exe)
#
# The size is a claim the README makes about the download, so it has to come
# from the download rather than from memory: it drifted from "about 450 KB" to
# nearly 600 without anyone noticing, because nothing ever measured it. This
# runs from package-release.sh, so every release re-measures.
#
# The About window renders README.md, so a size written now reaches About at the
# next build - which is why the release recipe builds, packages (here), and
# builds again before the artifacts are published.
set -e

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

EXE=${1:-dist/hgfloater.exe}
[ -f "$EXE" ] || { echo "update-exe-size: no $EXE - build it first" >&2; exit 1; }

command -v python3 >/dev/null 2>&1 || {
    echo "update-exe-size: python3 not found; leaving the READMEs alone" >&2
    exit 0
}

python3 - "$EXE" <<'PY'
import os, re, sys

exe = sys.argv[1]
kb = (os.path.getsize(exe) + 512) // 1024

# Rounded to ten kilobytes: the number is a scale, not a receipt, and a README
# that changes by three digits every build is noise in every diff.
kb = int(round(kb / 10.0) * 10)

edits = [
    ("README.md", re.compile(r"(executable of about )\d+( KB)"), r"\g<1>%d\g<2>" % kb),
    ("README.ko.md", re.compile(r"(약 )\d+( KB짜리 실행 파일)"), r"\g<1>%d\g<2>" % kb),
]

for path, pattern, replacement in edits:
    with open(path, encoding="utf-8", newline="") as f:
        text = f.read()
    new_text, count = pattern.subn(replacement, text)
    if count == 0:
        sys.exit("update-exe-size: %s no longer states a size the way this script looks for" % path)
    if new_text != text:
        with open(path, "w", encoding="utf-8", newline="") as f:
            f.write(new_text)
        print("size: %s now says %d KB" % (path, kb))
    else:
        print("size: %s already says %d KB" % (path, kb))
PY
