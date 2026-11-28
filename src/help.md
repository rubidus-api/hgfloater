# HGFloater help

This is the short help: what each window is, and the keys and gestures that
drive it. The full manual, with pictures, is on the web:
https://rubidus-api.github.io/hgfloater/

## Start here

- **The floater** is the small box with the clock that stays on top. Click it,
  or press `Win + Alt + Space` from anywhere, to open the taskbox.
- **The taskbox** is the dashboard: your running windows, your shortcuts and
  ten buttons, in one grid. Move the pointer away and it folds back into the
  floater after half a second.
- **To add a shortcut**, point at `Set`, choose **Open Shortcuts Folder**, and
  copy any `.lnk` or `.url` into that folder. The icon appears within a second.
  A shortcut to a folder goes to the `Dir` list instead. Icons are sorted by
  file name: `0001 Editor.lnk`, `0002 Browser.lnk`.
- **To quit**, press `Ctrl + Q`, or choose **Exit hgfloater** in the `Set` list.

Only one copy runs at a time. HGFloater has no installer and no service, never
reaches the network, and keeps everything in
`%USERPROFILE%\.HellGates\hgfloater\`.

## The floater

| Gesture or key | What it does |
| :--- | :--- |
| Click | Opens or closes the taskbox. |
| Drag, or `Alt` + drag | Moves it anywhere, the taskbar included; on the taskbar it stays on top. |
| `Alt` + wheel | Opacity. |
| `Ctrl` + wheel, or `Ctrl` + drag | Size of the whole floater. |
| `Alt` + arrows or `WASD` | Moves it from the keyboard. |
| `T` | Opens the taskbox. |
| `C`, `Ctrl + E` | Opens the command box. |
| `F1` | This help. |

The lines under the clock are bars for CPU, temperature (`TMP`), `GPU`,
memory and battery, with the reading at the right. A row the machine does not
report is left out. `show_stats=0` in `config.ini` hides them.

Both clock lines are formats you write, in the `strftime` notation: the clock
is `%H:%M` and the date `%a, %b %-d` by default. Change them in the settings
window under **Floater Clock**.

| Code | Shows | Code | Shows |
| :--- | :--- | :--- | :--- |
| `%H` | hour, 00-23 | `%I` | hour, 01-12 |
| `%M` | minute, 00-59 | `%S` | second, 00-59 |
| `%p` | AM or PM | `%j` | day of the year |
| `%Y` | year, 2026 | `%y` | year, 26 |
| `%m` | month, 01-12 | `%d` | day, 01-31 |
| `%b` | Jan | `%B` | January |
| `%a` | Sun | `%A` | Sunday |
| `%e` | day, space-padded | `%%` | a percent sign |

A minus drops the padding: `%-d` is `6` where `%d` is `06`.

## The taskbox

| Gesture or key | What it does |
| :--- | :--- |
| Click an icon, or `Space` on it | Brings the window forward, or launches the shortcut. |
| Right-click, or `Enter` / `F2` | The icon's menu: Run, Focus, Close Window, Open File Location. |
| Drag a task icon | Moves it in the grid. |
| Arrows or `WASD` | Move between icons. |
| `Shift` + `0`-`9` | The window with that badge (`S0`-`S9`). |
| `Shift` + `A`-`Z` | The shortcut with that badge (`SA`-`SZ`). |
| Drag a border | Changes the grid; it snaps to whole columns. |
| `Ctrl` + wheel, `Ctrl` + `+` / `-` | Icon and text size. |
| `Alt` + wheel, `Alt` + `+` / `-` | Opacity. |
| `Ctrl` + `R` / `0`, `F5` | Resets position, size and opacity. |
| `Esc` | Hides the taskbox and re-reads the shortcuts folder. |

Badges show on the icon under the pointer, on the one with the keyboard focus,
and on every icon while `Shift`, `Ctrl` or `Alt` is held.

Pointing at a task icon outlines its window on the desktop without raising it.
With **Show Tabs as Task Icons** on, pointing at a browser, Explorer, Terminal
or Notepad icon lists its tabs beside it.

The status line at the top shows the last message, then the time. Drag it to
move the taskbox; right-click it for the A/V Control Center.

## The buttons

| Button | Click | Wheel | Right-click |
| :--- | :--- | :--- | :--- |
| `A/V` | The A/V Control Center | — | — |
| `Vol` | Mute and unmute | Volume, 5% a notch | A/V Control Center, Audio |
| `Mon` | Scaling of this screen | Brightness, 10% a notch | A/V Control Center, Display |
| `Alp` | — | Taskbox opacity | — |
| `Dir` | Folder shortcuts, as a list | — | — |
| `Set` | Every switch, the settings window, Exit | — | — |
| `Clip` | Clipboard history | — | — |
| `Note` | The note list | — | — |
| `Cmd` | The command box | — | — |
| `D` | Shows the desktop; again to restore | — | — |

`Dir` and `Set` open when pointed at; `A/V` waits for a click. On `Vol`,
`Mon` and `Alp`, `PageUp` / `PageDown` or `E` / `Q` turn the value from the
keyboard.

In the `Dir`, `Set` and tab lists: `1`-`9` and `0` jump to a row, `Up` / `Down`
or `Tab` step in, `Enter` or `Space` takes the row, `Left` / `Right` leave the
list (on a number row they turn the value), `Esc` steps out.

## The A/V Control Center

`A/V`, `Ctrl + O` in the taskbox, or a right-click on the status line, `Vol` or
`Mon`. The left pane lists what can be set; the right pane holds its controls:

- **Output & Volume**: the master volume, Mute Output, and the output devices.
- **Screen Topology**: the four `Win + P` arrangements.
- **A display**: Preview Window (a live thumbnail you can click through),
  Brightness, Scale and Resolution.

`Esc`, `Ctrl + W` or a click outside closes it.

## The command box

`Cmd`, `C` or `Ctrl + E`. Type a command and press `Enter`.

| Key | What it does |
| :--- | :--- |
| `Enter` | Runs what is typed. |
| `Shift + Enter` | A new line; several lines run in order. |
| `Ctrl + H` | History mode: pick an earlier line with `Up` / `Down`, `Enter`. |
| `Ctrl + S` | Scroll mode: `Up` / `Down` a line, `Left` / `Right` a page. |
| `Ctrl + Space` | Back to the input box. |
| `Esc` | Leaves the mode, or closes the box and returns to the taskbox. |
| `Ctrl + W` | Closes the box. |

| Command | Short | What it does |
| :--- | :--- | :--- |
| `help` | `h` | Every command. `help move` explains one. |
| `help key` | `h k` | The keys of this box. |
| `show` | `s` | Windows, numbered. |
| `go 1` | — | Focus window 1. |
| `move 1 100 100 2` | `m 1 100 100 2` | Move window 1 to 100, 100 on display 2. |
| `resize 1 1` | `r 1 1` | Resize window 1 to preset 1 (`show resize` lists them). |
| `find windows word` | `f w word` | Windows whose title holds `word`. |
| `show tabs`, `go tab 4` | `s t`, `g t 4` | Every tab, and switch to one. |
| `show monitor 1` | `s m 1` | Display 1's preview, on and off. |
| `note`, `note new`, `note 3` | `n`, `n n`, `n 3` | The note list, a new note, note 3. |
| `find note word` | `f n word` | Notes that hold `word`. |
| `clipboard 3` | `b 3` | Make clip 3 the current one. |
| `show option`, `write option 1 on` | `s o`, `w o 1 on` | The switches, and set one. |
| `show value`, `write value 1 60` | `s v`, `w v 1 60` | The values, and set one. |
| `show key`, `bind`, `unbind` | `s k` | The keys, and change them. |
| `show sensors` | `s s` | Every temperature sensor found. |
| `settings` | `set` | The settings window. |
| `config` | `c` | `config.ini` in Notepad. |
| `clear` | `cls` | Empties the transcript. |

## Notes

`Note`, `N` or `Ctrl + N` in the taskbox, or `note` in the command box. Each
note is a plain `.txt` file; its first line is the title. Saving is automatic.

| Key or gesture | What it does |
| :--- | :--- |
| `Enter`, double-click | Opens the note in its own editor. |
| `Insert`, the `Add` button | A new note. |
| `Space`, a click on the box | Checks the note. |
| `K`, the `Arc` button | Archives the checked notes, or restores them. |
| `Delete`, the `Del` button | Deletes the checked notes, to the Recycle Bin. |
| The `Search` button | Shows only notes that hold every word typed. |
| Drag a row by its grip | Puts it elsewhere; that half turns to the Manual sort. |
| Right-click | Open, Archive or Restore, Delete, and Sort by. |
| `Ctrl` + wheel | Note text size, shared by the list and every editor. |
| `Alt` + arrows, `Ctrl` + arrows | Move, resize the list window. |
| `Esc`, `Ctrl + W` | Closes the list or the editor. |

An archived note opens read-only until it is restored. In an editor, `Ctrl + Z`
and `Ctrl + Y` undo and redo a hundred steps.

## The clipboard history

`Clip` or `Ctrl + L`. It lists the text you copied, newest first, kept in
memory only: nothing is written to disk, and quitting empties it.

| Key or gesture | What it does |
| :--- | :--- |
| Click, `Enter` | Makes that clip the current one; the window stays open. |
| Right-click | Copy to Clipboard, Delete, Delete All. |
| `Del` | Deletes the selected clip. |
| The search box | Shows only clips that contain the text. |
| The number box | The most clips kept, 16 by default. |

## The settings window

`Ctrl + ,`, **Settings Window...** in the `Set` list, or `settings`. One list,
with a filter box at the top (`Ctrl + F`): Options, Floater Clock, Fonts,
Values, Window Colours and Keys. Every change takes effect and is saved at
once.

- `Enter` or `Space` flips a switch, opens a font or colour chooser, or edits a
  clock format.
- `Left` / `Right` change a value.
- `R` puts a row back to its default.
- Under **Keys**, `Enter` on a function adds the next chord you press, and `Del`
  removes one.

## Keys anywhere in HGFloater

| Key | What it does |
| :--- | :--- |
| `Win + Alt + Space` | Shows or hides the taskbox, from anywhere. |
| `Ctrl + ,` | The settings window. |
| `F1` | This help. |
| `Ctrl + W` | Closes a note, the note list, the clipboard history, the command box or this window. |
| `Ctrl + Q` | Quits. |

The note windows, the clipboard history and the command box keep the usual
editing keys (`Ctrl + X`, `C`, `V`, `Z`, `Y`, `A`) for the text.

## Files

| Path | What it holds |
| :--- | :--- |
| `%USERPROFILE%\.HellGates\hgfloater\config.ini` | Every setting, plain INI. |
| `%USERPROFILE%\.HellGates\hgfloater\shortcuts\` | Your `.lnk` and `.url` files. |
| `%USERPROFILE%\.HellGates\hgfloater\note\` | One `.txt` per note, and `note.ini`. |

Edit `config.ini` while HGFloater is closed; a deleted key comes back with its
default at the next start. The only thing written outside this folder is the
`Run` value of **Start with Windows**, and only while it is on.

The configuration file, every command and every key are described in full in
the manual: https://rubidus-api.github.io/hgfloater/
