#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)

fail() {
  printf '%s\n' "project-check: $*" >&2
  exit 1
}

need() {
  command -v "$1" >/dev/null 2>&1 || fail "missing required tool: $1"
}

need git
need python3
need sh

cd "$root"

git status --short >/dev/null
git diff --check

if command -v find >/dev/null 2>&1; then
  for script in $(find scripts -type f -name '*.sh' 2>/dev/null | sort); do
    sh -n "$script"
  done
fi

python3 - <<'PY'
from pathlib import Path

for path in sorted(Path("scripts").glob("*.py")):
    compile(path.read_text(encoding="utf-8"), str(path), "exec")
PY

PYTHONDONTWRITEBYTECODE=1 python3 scripts/check-docs.py

printf '%s\n' "project-check: ok"
