[한국어](README.ko.md) | **English** — **HGFloater v0.18.3** — [Pages](https://rubidus-api.github.io/hgfloater/) — [EXE(x64)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater.exe) · [PDF(en)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-en.pdf) · [PDF(ko)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-ko.pdf) · [HTML(en)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-en-html.zip) · [HTML(ko)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-ko-html.zip)

# HGFloater

A floating clock that opens into a task switcher, launcher and control panel
for **Windows 11**: one portable executable of about 700 KB, written in pure C
against the Win32 API, with no installer and no runtime.

**v0.18.3** — built 2026-11-28

[Homepage](https://rubidus-api.github.io/hgfloater/) · [User manual](https://rubidus-api.github.io/hgfloater/en/) · [사용 설명서](https://rubidus-api.github.io/hgfloater/ko/)

<img src="https://rubidus-api.github.io/hgfloater/screenshots/taskbox.png" alt="The taskbox: running windows and shortcuts as icons, the ten buttons ending in the bottom-right corner, and the status line across the top" width="480">

## Why HGFloater

The stock taskbar puts windows, programs, volume, brightness and settings in
different places, each a few clicks deep. HGFloater keeps them in one small box
that floats wherever you park it, on any monitor or on the taskbar itself:

- **One window per icon, one key per icon.** Every open window, every shortcut
  you keep and every control sits in one grid; `Shift + 3` is always the fourth
  window and `Shift + K` always the same program.
- **Light by construction.** No service, no background helper, no network
  access, no logs. Settings and notes are plain files in one folder.
- **Keyboard and mouse are equal.** Everything has a gesture and a key, and
  every key can be changed.

## Download and install

**[Download hgfloater.exe — v0.18.3, 700 KB](https://github.com/rubidus-api/hgfloater/releases/latest/download/hgfloater.exe)** · [All releases](https://github.com/rubidus-api/hgfloater/releases)

Windows 11 or later, 64-bit. There is no installer: put `hgfloater.exe`
anywhere and run it. The file is unsigned, so each release lists its SHA-256;
check it in PowerShell with `Get-FileHash .\hgfloater.exe -Algorithm SHA256`.

## Quick start

1. Run `hgfloater.exe`. The floater, a small box with the clock, appears on the
   monitor under the pointer.
2. Click it, or press `Win + Alt + Space` from anywhere: the taskbox opens with
   your windows and the buttons.
3. Point at `Set`, choose **Open Shortcuts Folder**, and copy any `.lnk` into
   it. The icon appears in the taskbox within a second.
4. Press `F1` for the built-in help, `Ctrl + Q` to quit.

## Features

- **Task switcher**: an icon per window, with a key badge; pointing at one
  outlines its window on the desktop, and browser and Explorer tabs can be
  icons of their own.
- **Launcher**: the shortcuts folder is the setting; folder shortcuts go to a
  `Dir` list.
- **Sound and picture**: `Vol`, `Mon` and `Alp` show volume, brightness and
  opacity as gauges and change with the wheel. The A/V Control Center switches
  audio devices, screen arrangement, scaling, resolution and DDC/CI brightness.
- **Notes**: plain `.txt` files, with search, archive and drag-to-reorder.
- **Clipboard history**: text only, kept in memory and never written to disk.
- **Command box**: drive windows by number (`go 1`, `move 1 100 100 2`), open
  notes, change any setting.
- **A clock you format**: both floater lines take `strftime` codes, beside CPU,
  temperature, GPU, memory and battery bars.

Every feature, key and setting is described in the
[user manual](https://rubidus-api.github.io/hgfloater/en/).

## What it does not do

It does not replace the taskbar or the Start menu, install a service, run as
administrator, or reach the network: the binary imports no socket or HTTP
function. Two features are switched off in published builds until the
executable is code-signed: the maximize-button menu (it needs a system-wide
mouse hook) and **Start with Windows**.

## Status

Each release is built warning-clean and its behaviour is checked automatically
on a Windows 11 virtual machine before it is published. That machine has one
virtual monitor, no real backlight and no USB voice recorder, so three things
have not been verified yet: brightness of physical monitors over DDC/CI, use
with two or more monitors, and the recorder's clock command. The recorder's
settings form was checked by the author on a real recorder.

## Documentation

- [Homepage](https://rubidus-api.github.io/hgfloater/), with screenshots.
- User manual: [web (English)](https://rubidus-api.github.io/hgfloater/en/) ·
  [web (Korean)](https://rubidus-api.github.io/hgfloater/ko/); PDF and offline
  HTML editions are attached to each release.
- [CHANGELOG.md](CHANGELOG.md) for every release.

## Build

MinGW-w64 GCC and GNU `make`, nothing else:

```sh
make CROSS=x86_64-w64-mingw32- OUT=build release    # from Linux or macOS
build.bat release                                    # on Windows with MSYS2
```

The [manual](https://rubidus-api.github.io/hgfloater/en/reference-build.html)
has the toolchain setup and the release recipe.

## License

MIT, see [LICENSE](LICENSE). HGFloater is part of the HellGates series of
small Windows utilities by rubidus-api, developed with AI assistance.
