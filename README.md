# hgfloater

**English** | [한국어](README.ko.md)

hgfloater is a lightweight desktop utility for **Windows 11 and above**. A small
translucent widget floats on your desktop; hovering it opens a dashboard that
launches your shortcuts, switches between running windows, and puts volume,
brightness, opacity, and a command console one click away. It is written in pure
C against the Win32 API with zero external dependencies: the whole program is a
single **executable of about 450 KB** that needs no installer and no runtime, so
it starts instantly and stays out of your way.

<!-- SKIP_START -->
![screenshot](./screenshot/hgfloater-v26.07.29j-screenshot.png)
<!-- SKIP_END -->

---

## Table of Contents

1. [Overview](#1-overview)
2. [Install and First Run](#2-install-and-first-run)
3. [The Two Windows](#3-the-two-windows)
4. [The Floater](#4-the-floater)
5. [The Taskbox](#5-the-taskbox)
6. [The Toolbar](#6-the-toolbar)
7. [The Options Menu](#7-the-options-menu)
8. [The Command Box](#8-the-command-box)
9. [Monitor Thumbnails](#9-monitor-thumbnails)
10. [Notes](#10-notes)
11. [Keyboard Reference](#11-keyboard-reference)
12. [Mouse Reference](#12-mouse-reference)
13. [Configuration File](#13-configuration-file)
14. [Files and Directories](#14-files-and-directories)
15. [Building From Source](#15-building-from-source)
16. [Project Layout](#16-project-layout)
17. [Tests and Verification](#17-tests-and-verification)
18. [About the Developer](#18-about-the-developer)
19. [The HellGates Series](#19-the-hellgates-series)
20. [License](#20-license)

---

## 1. Overview

hgfloater aims at one thing: making everyday desktop control faster than the
stock taskbar allows. It works as a **quick launcher**, a **task switcher**, a
**system control panel** (volume, per-display brightness and scaling, window
opacity, screen lock), a **command box** for driving windows by name and number,
and a **note pad** - all reachable from a widget you can park anywhere on any
monitor.

Design principles worth knowing before you use it:

- **Nothing runs in the background but the app itself.** No services, no
  installer. A single `hgfloater.exe`. It writes exactly one registry value, and
  only if you switch **Start with Windows** on - the ordinary per-user `Run`
  entry, which you can see and delete yourself.
- **Plain files on disk, in one folder.** Settings live in a plain `config.ini`
  under your user profile, and notes are plain `.txt` files beside it. Nothing
  is a database, everything is editable by hand, and the program writes no logs,
  caches, or temporary files.
- **Everything is adjustable in place.** Size, opacity, font size, grid shape,
  and colors change live with the wheel or the keyboard, and persist by
  themselves.
- **Keyboard and mouse are equal citizens.** Every action has both a pointer
  gesture and a key.
- **It follows the system theme.** Switching Windows between light and dark mode
  re-colors the widgets immediately.

## 2. Install and First Run

1. **Download** the latest `hgfloater.exe` from the
   [Releases](https://github.com/rubidus-api/hgfloater/releases/tag/v26.08.05)
   page.
2. **Run it.** There is no installer. On first launch it creates
   `%USERPROFILE%\.HellGates\hgfloater\` with a `config.ini` and a `shortcuts`
   folder, then shows the floater.
3. **Add shortcuts.** Drop `.lnk` or `.url` files into
   `%USERPROFILE%\.HellGates\hgfloater\shortcuts`. They appear in the taskbox
   automatically; press `Esc` in the taskbox to re-scan the folder immediately.
4. **Summon it from anywhere** with `Win + Alt + Space` (configurable).

Only one instance runs at a time. Launching `hgfloater.exe` again simply
signals the running copy instead of starting a second one.

## 3. The Two Windows

hgfloater is built from two windows that trade places:

| Window | What it is | How you get it |
| :--- | :--- | :--- |
| **floater** | A small always-on-top widget: clock, date, host name, and system bars. | The default state. |
| **taskbox** | The dashboard: running windows, your shortcuts, and the toolbar. | Hover or click the floater, or press the global hotkey. |

Expanding centers the taskbox on the floater and then pushes it fully inside
that monitor's work area, so a floater parked at a screen edge never yields a
clipped dashboard. Collapsing returns the floater to where it was — and if you
moved the taskbox while it was open, the floater travels the same distance, so
the pair stays where you left it.

## 4. The Floater

The floater is the resting state: a compact translucent panel that stays on top
of other windows.

**What it shows**

- **Clock and date**, refreshed every second, sized proportionally to the
  widget itself.
- **Host name** in a thin line across the top.
- **Status bars** for CPU, temperature, GPU, memory, and battery: horizontal
  bars running behind everything - the host name included - with small labels
  down the left edge and each row's **reading printed down the right**. Full
  width means 100%; the two temperature rows use their own scale, below. A row
  whose value the machine does not report is absent entirely, which is why
  desktops have no battery row and machines without sensors have no `TMP` or
  `GPU`. Set `show_stats=0` in `config.ini` to hide the whole panel.
- **`TMP`** is drawn on a fixed **20 to 100 degree Celsius** scale rather than as
  a percentage, with the reading printed on its own bar, because the difference
  between 55 and 75 degrees matters more than a few pixels of bar length can
  say.

  **`GPU`** follows it on the same scale, and it gets a better answer than the
  CPU does: the adapter's own sensor, read through the same interface Task
  Manager uses, with no vendor SDK and no driver. Plenty of drivers report
  nothing, and those machines simply have no GPU row.

  The CPU reading is read from an **ACPI thermal zone** your firmware exposes. That is the
  only temperature reachable without installing a kernel driver and running as
  administrator, and it is worth knowing what it is: **a sensor on the board
  near the CPU, not the CPU die**. It tracks the processor without being it, and
  it lags under a sudden load. Many machines expose no zone at all - on those
  the row simply is not there, the same way the battery row is absent on a
  desktop. If you need true per-core readings, use a tool built around a
  hardware-monitoring driver; hgfloater will not become one.

  Firmware usually declares **several** zones, and often puts one it never
  updates in front of the live one, so a single reading taken blindly is a coin
  toss. hgfloater reads **every** zone, from both surfaces Windows offers - the
  WMI class and the thermal performance counters, which do not always agree
  about being available - and prefers a zone it has watched change over one that
  has only ever held the same number. Type **`list sensors`** in the command box
  to see all of them and which one is on the bar.

**What you can do to it**

- **Hover** — opens the taskbox in place. The floater hides itself while the
  taskbox is up.
- **Left click** — toggles the taskbox.
- **Left drag** — in **`F` adjust mode**, moves the floater anywhere on any
  monitor. Outside that mode the pointer arriving on the floater opens the
  taskbox before a drag can begin, so press `F` first.
- **Alt + Wheel** — opacity.
- **Ctrl + Wheel** or **Ctrl + Left drag** — font size, which scales the whole
  widget with it.
- **Alt + Arrows / WASD** — moves the window from the keyboard.
- **`T`** — opens the taskbox. **`C`** — opens the Command Box. **`F1`** — About.

When the cursor leaves the taskbox, it collapses back to the floater after a
half-second grace period, so brushing past the edge does not dismiss it.

### 4.1 The maximize button menu

While hgfloater is running, **right-click the maximize button** — the one left
of the X — on **any** window, and you get hgfloater's menu for that window:
**Move to (0, 0)**, the same size presets the task icons offer, and **Close**,
which shuts the menu without touching the window. Left-clicking the button still
maximizes; right-clicking a caption button does nothing in Windows, so nothing
was taken away.

Switch it off with **Menu on Maximize Button** in the `O` menu, or
`caption_menu=0` under `[etc]`. **Close hgfloater and the behaviour is gone** —
there is nothing installed and nothing to uninstall.

Three cases where the click does what it always did, none of them a failure:

- **Windows running as administrator.** Windows does not let an unelevated
  program touch input bound for an elevated one, and hgfloater runs unelevated
  on purpose.
- **Windows with no maximize button** — nothing to right-click.
- **Applications that report neither a maximize button nor caption-button
  bounds.** Rare, and they simply keep their ordinary behaviour.

This is the only thing hgfloater does outside its own windows, and it needs a
system-wide mouse hook to do it. The hook reads the button and the coordinates
and nothing else; it holds no keyboard hook and never has. Because a global
mouse hook is also a thing keyloggers do, an antivirus may take an interest —
which is worth knowing in advance rather than after. The design and its costs
are in `docs/RFC-2026-07-caption-button-menu.md`.

## 5. The Taskbox

The taskbox is a grid of icons with a status line on top and a toolbar of
built-in buttons.

### 5.1 Running windows and shortcuts

- **Task icons** come first: **one per window, not one per program**, in the
  order Windows reports them. A program with four windows open gets four icons,
  each going straight to its own window.

  That now includes **hgfloater's own windows** — every open note editor, the
  note list, the clipboard history, the command box. The clipboard history and
  the command box are tool windows, so Windows keeps them out of Alt-Tab, which
  is right for a widget's controls but wrong for a list whose whole job is
  reaching them — so this list carries them anyway. **The note windows are
  ordinary application windows**: a note is a document, so each editor and the
  note list get their own taskbar button and Alt-Tab entry, and they stay open
  regardless of what the floater and taskbox do, until you close them. The
  floater, the taskbox and the toolbar stay out: they are the thing you are
  looking at.
- **Shortcut icons** follow: one per `.lnk` or `.url` in your shortcuts folder.

> ### 📌 How to add a shortcut icon
>
> 1. **Right-click the `O` button** on the toolbar (or right-click the status
>    line) to open the options menu.
> 2. Choose **Open Shortcuts Folder**. Explorer opens on
>    `%USERPROFILE%\.HellGates\hgfloater\shortcuts\`.
> 3. **Copy a shortcut into it** — any `.lnk` or `.url`. Drag one out of the
>    Start menu, or right-click a program and *Send to → Desktop*, then move
>    that file here.
>
> The new icon appears within a second. No restart, no settings screen: the
> folder **is** the setting.
>
> **To change the order, rename the files.** Icons are sorted by file name, so
> putting a number in front controls where each one sits:
>
> ```
> 0001 Visual Studio Code.lnk
> 0002 Firefox.lnk
> 0010 Calculator.lnk
> ```
>
> Pad the numbers to the same width — `0001`, not `1`. The sort is a plain
> alphabetical one, so `10` would come before `2` otherwise. Leaving gaps
> (`0010`, `0020`, `0030`) means you can slip something in later without
> renaming everything after it.
- **Left click** activates a task or launches a shortcut.

**Every task icon carries a label** in its top-left corner — `0` to `9`, then
`A` to `Z`. **`Shift` + that character** activates the icon, exactly as clicking
it would. Digits come first because the first ten windows are the ones you reach
most, and a digit is one keystroke to find. Past 36 icons there is no label and
no key. `Shift` is what keeps this clear of the bare-letter grid movement
(`WASD`) and of the bare `C` that opens the command box.

**A tabbed application's tabs can have their own icons.** Off by default; turn
it on with **Show Tabs as Task Icons** in the `O` menu, or `show_tabs=1` under
`[taskbox]` in `config.ini`. With it on, a window with eight tabs contributes
eight icons; clicking one switches to that tab, and middle-clicking or
**Close Tab** closes that tab alone.

Nothing about this is specific to browsers. What limits it is cost: a tab is not
a window, so the only supported way to find one is **UI Automation** — a call
into the other application's own UI thread, which is slow and can block if that
thread is busy. Asking every window on the desktop would make that cost the cost
of running hgfloater, so it asks only windows whose **class** is on a list:

| Built in | |
| :--- | :--- |
| `Chrome_WidgetWin_1` | Chrome, Edge, Brave, Opera, and Electron apps |
| `CabinetWClass` | File Explorer |
| `MozillaWindowClass` | Firefox |
| `CASCADIA_HOSTING_WINDOW_CLASS` | Windows Terminal |
| `Notepad` | Notepad |

**Any other application can be added** with `tab_classes` under `[taskbox]`,
semicolon-separated, no rebuild needed:

```ini
[taskbox]
tab_classes=SomeApp_Frame;OtherApp
```

Run **`show windows class`** (`s w class`) in the command box to see the class of
every open window, which is where those names come from. An application whose
tabs UI Automation does not publish will simply keep its single icon.

It re-reads tabs every five seconds rather than every second, and anything that
fails falls back silently to one icon for the window. Some applications do not
publish off-screen tabs at all, and those tabs will not appear. The design and
what it costs are in `docs/RFC-2026-07-tabs-as-task-icons.md`.
- **Left drag** on a task icon reorders it within the grid.
- **Right click** (or `Enter` / `F2` on the focused icon) opens its menu:
  - **Run (&R)** — start a new instance, or launch the shortcut.
  - **Focus (&F)** — switch to the existing window.
  - **Close Window (&X)** — task icons only. On a **tab** icon this reads
    **Close Tab** and closes only that tab, by invoking the tab's own close
    button; if the tab has no such button nothing is closed and the status line
    says so. **Focus** on a tab switches to that tab rather than only raising
    its window.
  - **Open File Location (&O)** — shortcut icons only.

### 5.2 The status line

A single-line read-only field across the top of the taskbox.

- It shows **one message at a time** — the most recent one replaces its
  predecessor.
- Ten seconds after the last message it falls back to the **current time**,
  written as `2026. 11. 23.(Tue) 13:24`, and refreshes as the minute changes.
- **Right click** it to open the options menu (the same one the `O` button
  shows).
- **Left drag** it to move the whole taskbox.
- **Ctrl + Wheel** over it changes only its own font size.

### 5.3 Shape and size

- **Drag any border** to change the grid: the taskbox snaps to whole columns, so
  no half icon is ever left hanging.
- **Ctrl + Wheel** (or `Ctrl` + `+` / `-`) scales icons and text together and
  keeps the window proportional.
- **Alt + Wheel** (or `Alt` + `+` / `-`) changes opacity.
- **`Ctrl` + `R` / `0`, or `F5`** resets position, size, and opacity.

## 6. The Toolbar

Thirteen built-in buttons sit in the same grid as the icons. Their order is fixed.

| Button | Click | Drag / Wheel |
| :--- | :--- | :--- |
| **`R`** Resize | — | **Drag** resizes the taskbox grid. |
| **`M`** Move | **Click** moves the taskbox aside (see below). | **Drag** moves the window. |
| **`X`** Exit | Quits hgfloater. | — |
| **`D`** Desktop | Minimizes every window; click again to restore. | — |
| **`O`** Options | Opens the [options menu](#7-the-options-menu). | — |
| **`C`** Command | Opens the [Command Box](#8-the-command-box). | — |
| **`A`** Alpha | — | **Wheel** changes taskbox opacity. The red background brightens as opacity rises. |
| **`B`** Brightness | — | **Wheel** changes screen brightness in 5% steps. The green background brightens with it. |
| **`V`** Volume | Toggles mute. A thick accent border marks the muted state. | **Wheel** changes the volume. The blue background brightens with it. |
| **`F`** Floater | Collapses to the floater for tuning (see below). | — |
| **`P`** Pin | Pins the taskbox open. | — |
| **`N`** Note | Opens the [note list](#10-notes). | — |
| **`L`** Clipboard | Opens the clipboard history; press again to close it. | — |

**`M` — move aside.** Clicking the move handle without dragging nudges the pair
out of the way on its own, just far enough to stop covering the spot it was
sitting on. Clicks keep their heading, so pressing it repeatedly walks the
window across the screen; when that heading runs out of room it turns
counter-clockwise — north, west, south, east, and back to north. If no direction
has room, nothing moves.

**`B` — brightness.** The wheel moves brightness in **5% steps**, the same as
opacity and volume. The `O` menu offers quarter steps per display when you want
a specific level rather than a nudge.

hgfloater tries three things per display, in order, and remembers which one
answered. First the **low-level DDC/CI** path: it reads the monitor's
capabilities string and, if that advertises the luminance control, drives it on
whatever scale the monitor actually reports — which is often not 0 to 100. Then
the **high-level** brightness call, for monitors that support that but not the
first. Last a **gamma ramp**, which is worth being honest about: it dims the
picture, not the backlight, and costs contrast.

A monitor can refuse all of them — DDC/CI switched off in its own menu, a cable
or adapter that does not carry the channel, a KVM in the path, or a monitor that
advertises a control it does not honour. That display shows
**Brightness (unavailable)** rather than accepting a click that does nothing.

A **laptop's internal panel** is not a DDC/CI device at all, so none of that
would reach it. It gets its own path: hgfloater recognises the built-in display
by its connector and drives the real backlight through Windows' own brightness
service, the same one the system slider uses. That moves the lamp, not the
picture.

**`F` — floater adjust.** Collapses the dashboard and suspends hover-to-expand,
so you can tune the floater with `Ctrl + Wheel` (size) and `Alt + Wheel`
(opacity) without the taskbox reappearing under your cursor — and **drag it**
to put it where you want, which is only possible here for the same reason.
Click the floater without dragging to go back.

**`P` — pin.** While pinned, moving the mouse away no longer collapses the
taskbox, and the button carries an accent border. Explicit closes — `X`, `Esc`,
the global hotkey, a floater click — still work. Click again to unpin.


### The clipboard history

**`L`** opens a window listing what you have copied, newest first. Press `L`
again and it closes.

Capture runs whether or not that window is open — a history that only recorded
while you were looking at it would not be a history — so hgfloater keeps a
clipboard listener alive for as long as it is running. **Text only.** Copying an
image or a file leaves the history untouched rather than adding an empty row.

| What | What it does |
| :--- | :--- |
| **Click** a row, or `Enter` | Makes that clip the current one and closes the window. |
| **Search box**, top left | Shows only the clips containing what you type, ignoring case. Clear it to see them all again. |
| **Number box**, top right | The most clips to keep. Default 16, set with the spin buttons, the arrow keys, or the wheel over it. |
| `Esc` | Closes the window. |

**Choosing an old clip moves it to the top and pushes everything above it down
one.** Nothing is lost and nothing below your choice moves: the list ends up in
the order it would have been in had you simply copied that text again. A clip
identical to the one already at the top is never recorded twice, which is also
what keeps your own choice from coming straight back as a new entry.

**Lowering the maximum takes effect at once** — with 20 clips kept and the
number set to 16, the oldest 4 are dropped as you set it, because "at most 16"
would otherwise be false the moment you asked for it. **Raising it fills
forward**: the dropped clips are gone, and new ones accumulate up to the new
number.

**The history is bounded in bytes as well as in count.** Very large clips are
capped individually, and the whole history is capped in total; past the total,
the oldest clips go first and the newest always stays. Copying enormous text
blocks all day cannot make hgfloater's memory grow without limit.

**Nothing is written to disk.** The history lives in memory and dies with the
program; only the maximum is saved. This is deliberate, and it is the one place
hgfloater does less than the clipboard managers it was measured against: a
clipboard history on disk is a file containing every password and recovery code
that passed through the clipboard, and a floating clock widget should not be the
program that owns that file. The cost is stated rather than hidden — **restarting
hgfloater empties the history.**

---

## 7. The Options Menu

Open it with the `O` toolbar button or by right-clicking the status line.

- **Open Shortcuts Folder** — opens the shortcuts directory in Explorer. This is
  how shortcut icons are added; see [the box in 5.1](#51-running-windows-and-shortcuts).
- **Edit Configuration** — opens `config.ini` in Notepad.
- **Menu on Maximize Button** — checked while the maximize button of **every**
  window answers a right-click with hgfloater's move-and-resize menu. See
  [4.1](#41-the-maximize-button-menu).
- **Show Tabs as Task Icons** — checked when a tabbed application's tabs get
  their own task icons. See [5.1](#51-running-windows-and-shortcuts) for which
  applications, how to add one, and why it is off by default.
- **Start with Windows** — checked when hgfloater launches at sign-in. Toggling
  it writes or removes one value under
  `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, holding the quoted
  path of the running executable. No installer, no elevation, nothing else
  touched. Switching it on again after moving `hgfloater.exe` re-registers where
  the file actually is. **Reset Settings** leaves it alone.
- **About...** — this document, rendered inside the app.
- **Reset Settings** — restores default geometry, opacity, sizes, and colors.
- **Select Audio Device** — lists the output devices with the current one
  checked, and offers a **Mute** toggle.
- **_(one entry per display)_** — every connected monitor gets its own submenu,
  named the way you would name it: its number, the monitor name the driver reads
  out of the display's EDID, and the connector it hangs off, as in
  `2. DELL U2720Q (DP)`. Each submenu holds everything the app can do to that
  one display:
  - **Preview Window** — turns that display's
    [thumbnail](#9-monitor-thumbnails) on and off.
  - **Scale** — the Windows display-scaling percentages (100, 125, 150, 175,
    200, 225). The current scale is checked; percentages the monitor cannot
    reach are greyed out. Choosing one changes that monitor's scale
    system-wide, exactly as the Settings app would.
  - **Brightness** — the backlight in quarter steps (0, 25, 50, 75, 100). The
    step nearest the monitor's current level is checked. Displays that answer
    DDC/CI are driven directly; the rest fall back to a gamma curve applied to
    that display alone, so dimming one screen leaves the others as they were.

  The port is the socket on the graphics adapter, not the cable: a USB-C screen
  running DisplayPort alt mode reports as `DP` and cannot be told apart from a
  DisplayPort socket. `USB-C` appears when DisplayPort is tunnelled over USB4 or
  Thunderbolt, which is the only case Windows reports separately.
- **Lock Screen (Power Off)** — locks the workstation.
- **Exit** — quits.

## 8. The Command Box

A standalone console window, opened with the `C` toolbar button or the `C` key
while the floater or taskbox has focus.

**The keyboard goes to the input box** the moment the window opens, and again
whenever it is activated, so the first thing you type lands where you meant it.
**The window opens on its own key list** rather than a blank prompt — the same
list `help key` prints.

| Key | What it does |
| :--- | :--- |
| `Enter` | Run what is typed. |
| `Shift + Enter` | A new line. Several commands run in order, top to bottom. |
| `Ctrl + S` | **Scroll mode**: `Up`/`Down` move the transcript a line, `Left`/`Right` a page. |
| `Ctrl + H` | **History mode**: `Up`/`Down` walk through what you have run. |
| `Esc` | Leave the mode, or — with no mode on — close the box and go back to the taskbox. |
| `Ctrl + Space` | Jump to the input box. |

**The arrows stay the caret's** until you ask for a mode, so selecting and
editing text in the input box works the way it does in every other text box on
the machine. The title bar says which mode is on, and any key the mode has no
use for drops the mode and is then handled normally — typing never lands in a
hole.

The history keeps the last **64** lines by default, changed with
`write value history-max <n>` or the `history_max` key in `config.ini`. Running
the same line twice in a row stores it once. Nothing is written to disk.

- **`Ctrl + Wheel`** changes the text size, **`Alt + Wheel`** the opacity, and a
  plain wheel scrolls.
- Long lines **wrap** instead of running off the right edge, so a transcript of
  window titles and help text reads without a horizontal scrollbar.
- The transcript keeps a bounded tail: past its budget the **oldest lines are
  trimmed**, which also keeps output appearing forever — an EDIT control that
  hits its internal limit would otherwise drop new text silently.
- The window keeps its own position, size, opacity, font, and font size in
  `config.ini`, independent of the other widgets.

### Commands

Every command that names a window uses the number `show` prints beside it, and
that is the window's place in the same list the toolbar draws. Only the commands
that print numbers re-read the window list: if `go` refreshed first, windows
could come and go between the list you read and the number you typed.

| Command | Short | What it does |
| :--- | :--- | :--- |
| `help` | `h` | The list below, inside the box. |
| `help move` | `h m` | One command explained in full, with examples. |
| `help key` | `h k` | The keys above, not the commands. |
| `show` | `s` | Windows, numbered. |
| `show windows` | `s w` | The same list. |
| `show windows class` | `s w class` | The same list with each window's class, which is what `tab_classes` takes. |
| `show resize` | `s r` | The resize presets, numbered. |
| `show shortcut` | `s c` | The shortcut icons, numbered. |
| `show note` | `s n` | Every note, numbered for the `note` command. |
| `show monitor` | `s m` | Every display, numbered, with its size, its place, and whether its preview is up. |
| `show monitor 1` | `s m 1` | Turn display 1's [preview window](#9-monitor-thumbnails) on; run it again to close it. |
| `show sensors` | `s s` | Every temperature sensor found, numbered, and which one `TMP` and `GPU` show. |
| `show sensors 2` | `s s 2` | Just sensor 2, with its unit. |
| `show value` | `s v` | The settable values, numbered, with what each one is now. |
| `go 1` | — | Focus window 1, restoring it if it is minimised. |
| `resize 1 1` | `r 1 1` | Resize window 1 to preset 1. |
| `move 1 100 100` | `m 1 100 100` | Move window 1 to 100, 100 on the display it is already on. |
| `move 1 100 100 2` | `m 1 100 100 2` | Move window 1 to 100, 100 on display 2. |
| `find windows word` | `f w word` | Windows whose title contains `word`, listed under their `show` numbers rather than renumbered. |
| `note` | `n` | Open the [note list](#10-notes). |
| `note new` | `n n` | Write a new note and open it. |
| `note 3` | `n 3` | Open note 3 in its own editor. An archived note opens read-only. |
| `note 3 archive` | `n 3 a` | File note 3 away. |
| `note 3 restore` | `n 3 r` | Put it back among the active notes. |
| `note 3 delete` | `n 3 d` | Delete note 3, to the Recycle Bin. |
| `find note word` | `f n word` | Notes whose title **or body** contains `word`, under their `show note` numbers. |
| `clipboard` | `b` | The [clipboard history](#the-clipboard-history), numbered. |
| `clipboard 3` | `b 3` | Make entry 3 the current clipboard, pushing the ones above it down. |
| `write value 1 60` | `w v 1 60` | Set value 1 to 60. The name works too, shortened: `w v bright 60`. |
| `config` | `c` | Open `config.ini` in Notepad. |

`X` and `Y` are measured from the target display's own top-left corner, not from
the virtual desktop's, so the same pair of numbers means the same place on every
screen. The display number is the one the options menu shows beside that
monitor's name. A search term may contain spaces; everything after `windows` is
the term.

`help` on its own prints one line per command; **`help <command>`** prints that
command's arguments, what they mean, and worked examples - `help move`,
`h search`, and so on.

The input field is multi-line, so pasting several lines runs them in order, each
echoed behind a `>` prompt.

## 9. Monitor Thumbnails

**Preview Window** in a display's submenu opens a live thumbnail of that
display.

- **Click and drag inside a thumbnail** to drive the real monitor: mouse input
  is forwarded to it, so you can operate a screen you are not looking at.
- **Left drag the thumbnail's edit box** to move the thumbnail window.
- **Right click the edit box** to close it.
- Each thumbnail remembers its own position and size.
- The refresh rate follows attention: **10 frames a second while your pointer
  is on the thumbnail** (that is when you are driving the other monitor
  through it), 5 while it is merely open, and one when other windows cover it
  completely. Every frame is a capture of the whole source display, so a
  thumbnail nobody is looking at no longer costs what one being used does.

## 10. Notes

Open the note list with the **`N`** toolbar button or the `note` command.

**Note windows stand on their own.** The list and every open editor are
ordinary application windows: each has its taskbar button and Alt-Tab entry,
they appear as task icons in the taskbox like any other window, and collapsing
or closing the taskbox does not touch them — a note stays open until you close
it.

Each note is a plain UTF-8 `.txt` file, so it stays readable and editable
outside hgfloater. **The first line is the title**; everything after it is the
body. Notes live in `%USERPROFILE%\.HellGates\hgfloater\note\` and are named
`note-<id>-YYYYMMDD.txt`, where the date is the day the note was created.

**The list** starts with **`+Add Note`** and then shows one note per row:
creation date, last-modified date, and title.

`+Add Note` is the row the list opens on, so `N` then `Enter` writes a new note
without a key to remember. It answers a single click, unlike the note rows, and
it is there even when there are no notes yet.

**Active and archived notes are separate.** Notes you are still writing stay on
top; archived ones settle underneath. Headings appear once something has been
archived - until then the list is just notes, and a label would be noise.

| Key | What it does |
| :--- | :--- |
| `Enter` on `+Add Note`, or one click on it | Create a note and open it. |
| `Enter`, double click | Open the note in its own editor window. |
| `Insert` | Create a note and open it, from any row. |
| `K` | Archive the selected note, or restore it. |
| `Delete` | Delete the selected note. |
| `Esc` | Close the list. |

**Right-click a note** (or a heading) for that half of the list:

- **Open**, **Archive** or **Restore**, and **Delete**.
- **Sort by** created, modified, or title, each ascending or descending, with
  the one in force ticked.

**An archived note is read-only.** Archiving files a note away, and something
filed away has stopped being written: its editor opens read-only, and the
caption says so rather than leaving a window that silently ignores typing.
Copying and selecting still work - reading an archived note is the point of
keeping it - and **Restore** makes it writable again, so this is a state and not
a one-way door.

Deleting is the exception: any note can be deleted, archived or not. Deleted
notes go to the **Recycle Bin**, so a mistaken `Delete` is recoverable where
Windows would normally recover it.

The two halves sort independently: you can keep what you are writing by most
recently changed and your archive by title. Each half remembers its own order.

**The editors** are separate windows, and several can be open at once. Typing
updates the title as soon as the first line changes. `Esc` closes an editor.

**Right-click inside an editor** for the text and for the note itself: **Undo**,
**Redo**, **Cut**, **Copy**, **Paste**, **Delete** (the selection), **Select All**, then
**Archive** or **Restore**, and **Delete Note and Close**. Entries that would do
nothing are greyed, so the menu says what is possible rather than failing
quietly. It replaces the edit control's stock menu, which knows about the text
but nothing about the note it belongs to.

Undo goes back a hundred steps and **`Ctrl + Y`** or **Redo** comes forward
again, because the editor hosts a rich edit control rather than the plain one,
whose single undo level only toggles. If that control cannot be loaded the
editor still opens on the plain one, with Redo greyed out.

**`Ctrl + Wheel`** over the list or over any editor changes the note text size.
There is one size for all of them, so the list and every open editor follow the
same wheel. It is stored unscaled, so a change made on a 200% display reads the
same on a 100% one, and a note dragged between monitors is redrawn at that
display's scale. A plain wheel still scrolls.

Changing the size **re-fits what is on screen to the window you already have**:
the list re-measures its rows and scrolls the selected one back into view, and
every open editor re-wraps its text and keeps the caret on screen. No window
moves or changes size - only how much of it you can read at once does.

**Saving** happens on its own. Edits are held in memory and written a couple of
seconds after the typing settles, and only for the notes that actually changed,
so holding a key down does not turn into one write per character. Closing an
editor and quitting the app both flush whatever is still pending.

Everything a text file cannot carry lives in `note\note.ini` beside the notes:
which half of the list each note sits in, how each half is sorted, the shared
text size, the size and position of the list window, and **where each note's own
editor window was last left** - so a note reopens on the monitor and at the size you left it. The
creation time of day goes there too, since the file name only carries the day.

## 11. Keyboard Reference

### Global

| Key | Action |
| :--- | :--- |
| `Win + Alt + Space` | Show or hide the taskbox (configurable). If a window has drifted off screen, it is pulled back into the nearest monitor's work area. |

### Inside the taskbox

| Key | Action |
| :--- | :--- |
| `Arrow keys` / `WASD` | Move focus between icons |
| `Space` | Activate the focused item |
| `Enter` / `F2` | Open the focused item's context menu |
| `C` | Open the Command Box |
| `Esc` | Hide the taskbox and re-scan shortcuts |

### Inside the note list

| Key | Action |
| :--- | :--- |
| `Enter` on `+Add Note` | Create a note and open it |
| `Enter` on a note | Open it in its own editor |
| `Insert` | Create a note, from any row |
| `K` | Archive the selected note, or restore it |
| `Delete` | Delete the selected note (to the Recycle Bin) |
| `Esc` | Close the list |

### Inside a note editor

| Key | Action |
| :--- | :--- |
| `Ctrl + Z` / `Ctrl + Y` | Undo / redo, a hundred levels deep |
| `Ctrl + Wheel` | Note text size, shared with the list and every other editor |
| `Esc` | Close the editor (pending edits are written first) |

### Window manipulation (focused or hovered floater/taskbox)

| Key | Action |
| :--- | :--- |
| `Alt` + `Arrow keys` / `WASD` | Move the window |
| `Ctrl` + `+` / `-` | Font and icon size |
| `Alt` + `+` / `-` | Opacity |
| `Ctrl` + `R` / `0`, `F5` | Reset position, size, and opacity |

### System

| Key | Action |
| :--- | :--- |
| `F1` | About |
| `T` | Open the taskbox (from the floater) |
| `Ctrl` + `Q` / `X`, `Alt + F4` | Quit |
| `Enter` | Execute (inside the Command Box) |
| `Shift + Enter` | New line (inside the Command Box) |
| `Ctrl + S` / `Ctrl + H` | Scroll mode / history mode (inside the Command Box) |
| `Shift` + `0`–`9`, `A`–`Z` | Activate the task icon with that label |
| `Ctrl + Space` | Focus the input (inside the Command Box) |
| `Ctrl + Wheel` | Text size (inside the Command Box) |

## 12. Mouse Reference

| Action | Gesture |
| :--- | :--- |
| **Show / hide the taskbox** | Hover or left-click the floater, or press the global hotkey |
| **Activate an item** | Left-click an icon |
| **Reorder icons** | Left-drag a task icon |
| **Item context menu** | Right-click an icon |
| **Options menu** | Left-click `O`, or right-click the status line |
| **Move a window** | Left-drag empty space, the status line, or the `M` button |
| **Move the floater** | Left-click `F`, then drag the floater |
| **Move the taskbox aside** | Left-click the `M` button |
| **Resize the taskbox grid** | Drag a border, or drag the `R` button |
| **Font / icon size** | `Ctrl` + wheel |
| **Opacity** | `Alt` + wheel, or wheel over `A` |
| **Screen brightness** | Wheel over `B` |
| **Volume / mute** | Wheel over `V` / click `V` |
| **Pin the taskbox** | Left-click `P` |
| **Remote monitor control** | Click or drag inside a monitor thumbnail |
| **Open the notes** | Left-click `N` |
| **New note** | Left-click `+Add Note` (a single click, unlike a note row) |
| **Open a note** | Double-click it |
| **Sort, archive, or delete a note** | Right-click it, or right-click a section heading |
| **Note text size** | `Ctrl` + wheel over the list or an editor |
| **Edit a note's text** | Right-click inside an editor |
| **Command box text size** | `Ctrl` + wheel |
| **Quit** | Left-click `X`, or Exit in the options menu |

## 13. Configuration File

`%USERPROFILE%\.HellGates\hgfloater\config.ini`, plain INI, safe to edit by hand
while the program is closed. Missing keys are written back with their defaults on
startup, so deleting a key restores it.

Settings that change in bursts — opacity, font and icon size, window position —
are written once the change settles (about a second) or at exit, rather than on
every wheel notch.

### `[floater]` and `[taskbox]`

| Key | Meaning |
| :--- | :--- |
| `x`, `y` | Top-left screen coordinates |
| `w`, `h` | Width and height |
| `alpha` | Opacity, 76–255 |
| `font_size` | Text size |
| `icon_size` | Icon resolution (`[taskbox]` only) |
| `show_stats` | `0` hides the CPU/memory/battery line (`[floater]` only, default `1`) |
| `show_tabs` | `1` gives a tabbed application's tabs their own task icons (`[taskbox]` only, default `0`) |
| `tab_classes` | Extra window classes to look for tabs in, `;`-separated (`[taskbox]` only) |

### `[clipboard]`

| Key | Meaning |
| :--- | :--- |
| `max` | The most clips the history keeps, 1–64 (default `16`) |

Only the number lives here. The clips themselves are never written to disk.

### `[commandbox]`

Its own `x`, `y`, `w`, `h`, `alpha`, `font_size`, and `font_name`, plus:

| Key | Meaning |
| :--- | :--- |
| `history_max` | How many command lines `Shift + Left/Right` walk back through, 1–256 (default `64`) |

Both this and `[clipboard] max` have no control anywhere in the interface, so
hgfloater writes them into `config.ini` on first run with a comment block
explaining them. The lines and the clips themselves are never written to disk.

### `[etc]`

| Key | Meaning |
| :--- | :--- |
| `font_name` | Font for edit controls, tooltips, and the About dialog (default `Segoe UI`) |
| `caption_menu` | `0` stops right-clicking any window's maximize button from opening hgfloater's menu (default `1`) |

### `[colors]`

Every accent color as `RRGGBB` hex, for example `FFD228`:

- `scheme_bg`, `scheme_border`, `scheme_text`, `scheme_flash`, `scheme_selected`
  — the dark palette.
- `focus_bg` — the keyboard/mouse focus highlight.
- `stat_cpu`, `stat_temp`, `stat_gpu`, `stat_mem`, `stat_bat` — the floater's status bars.
- `value_alpha_low` / `value_alpha_high`, `value_brightness_low` /
  `value_brightness_high`, `value_volume_low` / `value_volume_high` — the
  gradients behind the `A`, `B`, and `V` buttons.

### `[hotkeys]`

| Key | Meaning |
| :--- | :--- |
| `global_focus_modifiers` | Modifier bitmask: `Alt=1`, `Ctrl=2`, `Shift=4`, `Win=8`. Default `9` (Win + Alt) |
| `global_focus_key` | Virtual key code of the trigger. Default `32` (Space) |

## 14. Files and Directories

| Path | Purpose |
| :--- | :--- |
| `%USERPROFILE%\.HellGates\hgfloater\` | Base directory, created on first run |
| `%USERPROFILE%\.HellGates\hgfloater\config.ini` | Every setting |
| `%USERPROFILE%\.HellGates\hgfloater\shortcuts\` | Your `.lnk` and `.url` files |
| `%USERPROFILE%\.HellGates\hgfloater\note\` | One `.txt` per [note](#10-notes), plus `note.ini` for everything the text files cannot carry |

That is the complete list on disk. The only thing written outside it is the
`Run` registry value behind [Start with Windows](#7-the-options-menu), and only
while that is switched on. hgfloater writes no log files, no caches, and no
temporary files, and nothing it writes grows without bound.

<!-- SKIP_START -->
## 15. Building From Source

The project builds with **MinGW-w64 GCC** and nothing else — no libraries, no
build system beyond `make` (or the batch script). The build is expected to stay
warning-clean under a strict warning set.

### 14.1 Toolchain

On Windows, install [MSYS2](https://www.msys2.org/), then:

```sh
pacman -S mingw-w64-x86_64-gcc make
```

Add `C:\msys64\mingw64\bin` to your `PATH` so `gcc` and `windres` are visible.

On Linux or macOS you need a mingw-w64 cross toolchain (`x86_64-w64-mingw32-gcc`
and `x86_64-w64-mingw32-windres`) plus GNU `make`.

### 14.2 With make

```sh
make                      # release build -> hgfloater.exe
make debug                # unoptimised build with symbols
make test                 # build and run the tests
make clean
```

Useful variables:

| Variable | Effect |
| :--- | :--- |
| `CROSS=x86_64-w64-mingw32-` | Build with a cross toolchain from Linux/macOS |
| `OUT=build` | Write the executable and objects to another directory |
| `VERSION_SUFFIX=b` | Mark a same-day re-release, e.g. `v26.07.20b` |

For example, a cross build into `build/`:

```sh
make CROSS=x86_64-w64-mingw32- OUT=build release
```

### 14.3 With build.bat

`build.bat` offers the same builds through a menu on Windows:

```bat
build.bat            REM interactive menu
build.bat release    REM non-interactive, exits with the build result
build.bat debug
build.bat test
```

### 14.4 What the build does

- The version string is the build date, `vYY.MM.DD`, optionally suffixed.
- `scripts/gen_about.py` (or `gen_about.ps1` on Windows) regenerates
  `src/hg_about_text.h` from this README, which is what the About dialog shows.
  Never edit that header by hand — edit the README and rebuild.
- The release build is `-O3 -flto` and stripped, linked statically, so the
  resulting `hgfloater.exe` needs no runtime DLLs.
<!-- SKIP_END -->

<!-- SKIP_START -->
## 16. Project Layout

```
hgfloater/
├── Makefile              build for POSIX hosts and MSYS2
├── build.bat             interactive Windows build menu
├── README.md             this file (English, the reference version)
├── README.ko.md          Korean translation
├── CHANGELOG.md          release history
├── src/                  all source and resources
│   ├── hgfloater.c       entry point, message loop, single-instance IPC
│   ├── hg_common.h       shared macros, ids, and constants
│   ├── hg_globals.*      global state
│   ├── hg_utils.*        theme, icons, toolbar descriptors, helpers
│   ├── hg_config.*       config.ini load/save, deferred writes
│   ├── hg_calc.*         pure math: layout, placement, clock formatting
│   ├── hg_command.*      the command box language
│   ├── hg_audio.c        volume and device selection
│   ├── hg_display.c      monitors, DPI, the brightness path ladder
│   ├── hg_wmi.c          root\WMI clients: panel backlight, thermal zones
│   ├── hg_shell.c        shortcuts, shell integration, start-with-Windows
│   ├── hg_sysinfo.c      CPU, memory, battery, GPU temperature
│   ├── hgfloater.rc      version info, icon, manifest
│   └── widgets/          one file per window
│       ├── hg_floater.c
│       ├── hg_taskbox.c, hg_taskbox_menus.c, hg_toolbar.c, hg_window_list.c
│       ├── hg_commandbox.c
│       ├── hg_note.c
│       ├── hg_clip.c         clipboard history
│       ├── hg_monitor.c
│       └── hg_about.c
├── test/                 console tests
├── scripts/              build and documentation helpers
└── docs/                 design notes and the test catalogue
```

`hg_calc.c` deliberately depends on neither Win32 nor the C runtime, so the
logic that is easy to get wrong — grid snapping, where a window moves to, how
the clock reads — can be tested on any host.
<!-- SKIP_END -->

<!-- SKIP_START -->
## 17. Tests and Verification

```sh
make test                     # compile every test, run the host-native ones
sh scripts/build-mingw.sh     # full cross-build verification
sh scripts/project-check.sh   # documentation and repository hygiene
```

Every file in `test/` is compiled with the full warning set. The units that
avoid Win32 also run natively on the build host, so their behaviour — not just
their compilation — is checked without a Windows machine. `docs/tests/` holds the
catalogue of what each check covers.
<!-- SKIP_END -->

## 18. About the Developer

- **Author**: rubidus-api (rubidus@gmail.com)
- **Method**: developed with AI assistance, in the "vibe coding" style.
- **Note**: the author is a hobbyist, not a career programmer. This project is
  the result of creative experimentation and collaboration with AI tools.

## 19. The HellGates Series

"HellGates" is a playful parody of Bill Gates and Windows: a collection of
utilities meant to make the desktop lighter and more responsive. The series
started from wishing Windows were friendlier, quicker, and less in the way, and
it exists to try UX ideas that the stock shell will not.

## 20. License

Released under the MIT License. See [LICENSE](LICENSE).
