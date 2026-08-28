#!/bin/sh
# Point the first page at the newest release, and say what it weighs.
#
# Usage:
#   sh scripts/update-readme-download.sh [exe]   the release being cut (default dist/hgfloater.exe)
#   sh scripts/update-readme-download.sh --latest   whatever GitHub currently serves as latest
#
# Four things at the top of both READMEs are claims about the download: the
# version, when it was built, the link, and the size. The link never went stale -
# it is /releases/latest/download, which always serves the newest - but the three
# beside it did, and a page that says v0.17.2 above a button that hands you
# v0.17.4 is worse than one that says nothing. So they are rewritten from the
# artifact rather than typed.
#
# The size comes from the file, never from memory: it drifted from "about 450 KB"
# to nearly 600 without anyone noticing, because nothing measured it.
#
# --latest asks GitHub instead of the working tree. That is how the page is
# refreshed without a build, and how a release is checked after publishing:
# same numbers means the page and the download agree.
#
# The About window renders README.md, so anything written here reaches About at
# the next build - which is why the release recipe builds, packages (this runs),
# and builds again before the artifact is published.
set -e

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

command -v python3 >/dev/null 2>&1 || {
    echo "update-readme-download: python3 not found; leaving the READMEs alone" >&2
    exit 0
}

if [ "$1" = "--latest" ]; then
    python3 scripts/readme_download.py --latest
else
    EXE=${1:-dist/hgfloater.exe}
    [ -f "$EXE" ] || { echo "update-readme-download: no $EXE - build it first" >&2; exit 1; }
    python3 scripts/readme_download.py "$EXE"
fi
