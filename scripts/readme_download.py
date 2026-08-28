"""Rewrite the download block at the top of both READMEs.

Called by scripts/update-readme-download.sh, which is the documented entry
point; this file is where the four claims - version, build stamp, link text and
size - are actually written, in one place, for both languages at once.

Two sources, one writer:

  a path      the artifact being released, read off disk, with VER.txt for the
              version. This is the release path: the file is measured before it
              is published, and the second build folds the number into About.
  --latest    whatever GitHub serves as the latest release right now. Refreshes
              the page without a build, and after publishing it is the check
              that the page and the download agree.
"""

import json
import os
import re
import sys
import time
import urllib.request

REPO = "rubidus-api/hgfloater"
DOWNLOAD_URL = "https://github.com/%s/releases/latest/download/hgfloater.exe" % REPO
RELEASES_URL = "https://github.com/%s/releases" % REPO


def rounded_kb(size_bytes):
    """Ten-kilobyte steps: the number is a scale, not a receipt, and a README
    that changes by three digits every build is noise in every diff."""
    kb = (size_bytes + 512) // 1024
    return int(round(kb / 10.0) * 10)


def from_disk(exe):
    with open("VER.txt", encoding="utf-8") as f:
        version = f.read().strip()
    stat = os.stat(exe)
    # The date, not the minute: the line is a claim about which release this is,
    # and a stamp that changed on every build would rewrite the top of both
    # READMEs - and the About window with them - for no news at all.
    stamp = time.strftime("%Y-%m-%d", time.localtime(stat.st_mtime))
    return version, stamp, rounded_kb(stat.st_size)


def from_latest_release():
    req = urllib.request.Request("https://api.github.com/repos/%s/releases/latest" % REPO)
    req.add_header("Accept", "application/vnd.github+json")
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        req.add_header("Authorization", "Bearer " + token)
    with urllib.request.urlopen(req, timeout=30) as response:
        release = json.loads(response.read())

    asset = next((a for a in release.get("assets", []) if a.get("name") == "hgfloater.exe"), None)
    if not asset:
        sys.exit("readme_download: the latest release (%s) has no hgfloater.exe" % release.get("tag_name"))

    # published_at is UTC ISO 8601; the date is all the top of the page claims.
    stamp = release.get("published_at", "")[:10]
    return release["tag_name"], stamp, rounded_kb(asset["size"])


# Every line the block owns, per language. A rule is a pattern and what replaces
# it; a pattern that matches nothing is an error rather than a silent skip,
# because a README that quietly stopped being updated is exactly the failure
# this script exists to prevent.
def rules(version, stamp, kb):
    return {
        "README.md": [
            (re.compile(r"^\*\*v[\d.]+\*\* — built .*$", re.M),
             "**%s** — built %s" % (version, stamp)),
            (re.compile(r"^\*\*\[Download hgfloater\.exe.*$", re.M),
             "**[Download hgfloater.exe — %s, %d KB](%s)** · [All releases](%s)"
             % (version, kb, DOWNLOAD_URL, RELEASES_URL)),
            (re.compile(r"(executable of about )\d+( KB)"), r"\g<1>%d\g<2>" % kb),
        ],
        "README.ko.md": [
            (re.compile(r"^\*\*v[\d.]+\*\* — 빌드 .*$", re.M),
             "**%s** — 빌드 %s" % (version, stamp)),
            (re.compile(r"^\*\*\[hgfloater\.exe 내려받기.*$", re.M),
             "**[hgfloater.exe 내려받기 — %s, %d KB](%s)** · [모든 릴리스](%s)"
             % (version, kb, DOWNLOAD_URL, RELEASES_URL)),
            (re.compile(r"(약 )\d+( KB짜리 실행 파일)"), r"\g<1>%d\g<2>" % kb),
        ],
    }


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: readme_download.py <exe> | --latest")

    if sys.argv[1] == "--latest":
        version, stamp, kb = from_latest_release()
        source = "the published latest release"
    else:
        version, stamp, kb = from_disk(sys.argv[1])
        source = sys.argv[1]

    changed = False
    for path, path_rules in rules(version, stamp, kb).items():
        with open(path, encoding="utf-8", newline="") as f:
            text = f.read()
        new_text = text
        for pattern, replacement in path_rules:
            new_text, count = pattern.subn(replacement, new_text)
            if count == 0:
                sys.exit("readme_download: %s no longer carries a line matching %s"
                         % (path, pattern.pattern))
        if new_text != text:
            with open(path, "w", encoding="utf-8", newline="") as f:
                f.write(new_text)
            changed = True
            print("download: %s now offers %s, %d KB" % (path, version, kb))
        else:
            print("download: %s already offers %s, %d KB" % (path, version, kb))

    print("download: from %s" % source)
    if not changed:
        print("download: nothing to change - the page already matches")
    return 0


if __name__ == "__main__":
    sys.exit(main())
