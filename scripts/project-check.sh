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

# src/hg_about_text.h is generated from README.md and committed, so it can be
# committed stale: edit the README after a build and the tree carries an About
# window describing the previous version. Regenerating into a scratch copy and
# comparing is the only way to see that.
PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
import pathlib, subprocess, shutil, sys, tempfile

root = pathlib.Path(".").resolve()
header = root / "src" / "hg_about_text.h"
current = header.read_bytes() if header.exists() else b""

with tempfile.TemporaryDirectory() as tmp:
    scratch = pathlib.Path(tmp)
    (scratch / "src").mkdir()
    shutil.copy(root / "README.md", scratch / "README.md")
    subprocess.run([sys.executable, str(root / "scripts" / "gen_about.py"), str(scratch)],
                   check=True, stdout=subprocess.DEVNULL)
    fresh = (scratch / "src" / "hg_about_text.h").read_bytes()

if fresh != current:
    sys.exit("project-check: src/hg_about_text.h is stale - "
             "run python3 scripts/gen_about.py and commit the result")
PY


# 이 기계의 절대 경로·인증서가 저장소에 들어갔는가 (작업공간 공용 검사).
# 도구는 저장소 밖(usr/bin)에 있다 --- 없으면 조용히 건너뛴다. 그 자리에서 옳은
# 문자열은 저장소 뿌리의 .privacy-allow 에 적는다.
privacy="$(cd "$(dirname "$0")/.." && pwd)/../usr/bin/check-privacy"
if [ -x "$privacy" ]; then
  "$privacy" || fail "local paths or credentials in the repository"
fi

printf '%s\n' "project-check: ok"
