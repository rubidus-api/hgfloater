#!/usr/bin/env python3
"""Generate hg_about_text.h from README.md (mirror of scripts/gen_about.ps1).

Usage: python3 scripts/gen_about.py [repo-root]
Intended for hosts without PowerShell; the output is byte-compatible with the
PowerShell generator that build.bat invokes.

The About window (F1) is a plain text box, so the README is not copied into it
as Markdown: it is turned into text a person can read there. Both generators
apply the same rules, in the same order:

  - what is between SKIP_START and SKIP_END, a line carrying SKIP, an image
    line, the language line at the very top and the download line are left out;
  - a paragraph's hard-wrapped lines are joined into one, so the text box wraps
    it to whatever width the window has;
  - # Title, ## Section, ### Part become  Title,  == Section ==,  -- Part --;
  - a list item keeps its indent and takes a bullet;
  - a table row becomes "first - second"; with more than two columns the first
    cell is a line of its own and each further cell follows as "Header: cell";
  - a fenced block is kept as written, indented;
  - `code`, **bold**, *italic* lose their marks, [text](#anchor) keeps its text
    and [text](https://...) becomes "text (https://...)".
"""
import re
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

RE_CODE = re.compile(r"`([^`]*)`")
RE_LINK = re.compile(r"\[([^\]]*)\]\(([^)]*)\)")
RE_BOLD = re.compile(r"\*\*(.+?)\*\*")
RE_ITALIC = re.compile(r"(?<![\w*])\*([^*\s](?:[^*]*[^*\s])?)\*(?![\w*])")
RE_ESCAPE = re.compile(r"\\([|*_`])")
RE_STASH = re.compile("\x00(\\d+)\x00")
RE_HEADING = re.compile(r"^(#{1,6})\s+(.*)$")
RE_ITEM = re.compile(r"^([-*+]|\d+\.)\s+(.*)$")
RE_RULE = re.compile(r"^(-{3,}|\*{3,}|_{3,})$")
RE_CELL_SPLIT = re.compile(r"(?<!\\)\|")
RE_SEPARATOR = re.compile(r"^:?-+:?$")


def inline(text):
    codes = []

    def stash(m):
        codes.append(m.group(1))
        return "\x00%d\x00" % (len(codes) - 1)

    def link(m):
        label, url = m.group(1), m.group(2)
        if url.startswith("http") and url != label:
            return label + " (" + url + ")"
        return label

    text = RE_CODE.sub(stash, text)
    text = RE_LINK.sub(link, text)
    text = RE_BOLD.sub(lambda m: m.group(1), text)
    text = RE_ITALIC.sub(lambda m: m.group(1), text)
    text = RE_ESCAPE.sub(lambda m: m.group(1), text)
    text = RE_STASH.sub(lambda m: codes[int(m.group(1))], text)
    return text


out = []          # the text, one entry per line of the window; "" is a blank line
marked = []       # per entry: still Markdown, to be converted once the paragraph is whole
skip = False
fence = False
header = None     # the header cells of the table being read, or None
para = False      # the last entry is a paragraph or list item that a plain line continues


def add(text, still_markdown=False):
    out.append(text)
    marked.append(still_markdown)


def blank():
    if out and out[-1] != "":
        add("")


first = True
for raw in readme.split("\n"):
    line = raw.rstrip("\r")
    was_first, first = first, False
    if "<!-- SKIP_START -->" in line:
        skip = True
        continue
    if "<!-- SKIP_END -->" in line:
        skip = False
        continue
    if skip or "<!-- SKIP -->" in line or line.strip().startswith("!["):
        continue
    if was_first and not line.startswith("#"):
        continue                      # the language / version / download line
    if line.startswith("**[Download"):
        continue                      # a link to the file the reader is running

    # A quoted line is read as what it quotes: the > and the space after it go.
    body = line
    quoted = False
    while body.lstrip(" ").startswith(">"):
        body = body.lstrip(" ")[1:]
        if body.startswith(" "):
            body = body[1:]
        quoted = True

    if body.strip().startswith("```"):
        fence = not fence
        para = False
        header = None
        continue
    if fence:
        add("    " + body)
        continue

    s = body.strip()
    indent = 0 if quoted else len(body) - len(body.lstrip(" "))

    if s == "" or RE_RULE.match(s):
        blank()
        para = False
        header = None
        continue

    m = RE_HEADING.match(s)
    if m:
        level, title = len(m.group(1)), inline(m.group(2))
        blank()
        if level == 1:
            add(title)
        elif level == 2:
            add("== " + title + " ==")
        else:
            add("-- " + title + " --")
        add("")
        para = False
        header = None
        continue

    if s.startswith("|"):
        cells = [c.strip() for c in RE_CELL_SPLIT.split(s)]
        cells = cells[1:-1] if len(cells) >= 2 else cells
        para = False
        if cells and all(RE_SEPARATOR.match(c) for c in cells):
            continue
        cells = [inline(c) for c in cells]
        if header is None:
            header = cells
            continue
        if not cells:
            continue
        if len(cells) <= 2:
            text = "  " + cells[0]
            if len(cells) == 2 and cells[1] not in ("", "—"):
                text += " — " + cells[1]
            add(text)
        else:
            add("  " + cells[0])
            for j in range(1, len(cells)):
                if cells[j] in ("", "—"):
                    continue
                name = header[j] if j < len(header) else ""
                add("      " + (name + ": " if name else "") + cells[j])
        continue
    header = None

    m = RE_ITEM.match(s)
    if m:
        marker = "•" if m.group(1) in ("-", "*", "+") else m.group(1)
        add(" " * indent + marker + " " + m.group(2), True)
        para = True
        continue

    # A paragraph or a list item is converted once all its lines are joined:
    # bold and links run across the README's hard line breaks.
    if para and out and out[-1] != "":
        out[-1] += " " + s
    else:
        add(s, True)
        para = True

while out and out[-1] == "":
    out.pop()
    marked.pop()

parts = []
for text, still_markdown in zip(out, marked):
    if still_markdown:
        text = inline(text)
    escaped = text.replace("\\", "\\\\").replace('"', '\\"')
    parts.append('L"' + escaped + '\\r\\n"')

content = ("#ifndef HG_ABOUT_TEXT_H\r\n#define HG_ABOUT_TEXT_H\r\n"
           "#define HG_ABOUT_README_W " + " ".join(parts) + "\r\n#endif")
open(root + "/src/hg_about_text.h", "w", encoding="utf-8", newline="").write(content)
print("[Success] README.md processed successfully.")
