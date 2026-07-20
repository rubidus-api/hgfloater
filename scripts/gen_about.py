#!/usr/bin/env python3
"""Generate hg_about_text.h from README.md (mirror of scripts/gen_about.ps1).

Usage: python3 scripts/gen_about.py [repo-root]
Intended for hosts without PowerShell; the output is byte-compatible with the
PowerShell generator that build.bat invokes.
"""
import sys

root = sys.argv[1] if len(sys.argv) > 1 else "."
try:
    readme = open(root + "/README.md", encoding="utf-8").read()
except OSError:
    content = ('#ifndef HG_ABOUT_TEXT_H\r\n#define HG_ABOUT_TEXT_H\r\n'
               '#define HG_ABOUT_README_W L"(README.md not found)"\r\n#endif')
    open(root + "/src/hg_about_text.h", "w", encoding="utf-8", newline="").write(content)
    print("[Warning] README.md not found.")
    sys.exit(0)

out = []
skip = False
for raw in readme.split("\n"):
    line = raw.rstrip("\r")
    if "<!-- SKIP_START -->" in line:
        skip = True
        continue
    if "<!-- SKIP_END -->" in line:
        skip = False
        continue
    if skip or "<!-- SKIP -->" in line or line.strip().startswith("!["):
        continue
    escaped = line.replace("\\", "\\\\").replace('"', '\\"')
    out.append('L"' + escaped + '\\r\\n"')

content = ("#ifndef HG_ABOUT_TEXT_H\r\n#define HG_ABOUT_TEXT_H\r\n"
           "#define HG_ABOUT_README_W " + " ".join(out) + "\r\n#endif")
open(root + "/src/hg_about_text.h", "w", encoding="utf-8", newline="").write(content)
print("[Success] README.md processed successfully.")
