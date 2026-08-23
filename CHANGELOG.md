# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [v0.17.1] - 2026-08-23

### Changed
- **The `Set` list says whole words.** It is text, and text has room:
  `Volume (ScrollWheel)`, `Brightness (ScrollWheel)`, `Alpha (ScrollWheel)`,
  `Pin`, `Options` - each still carrying its reading. A wheel over a row is not
  something anyone guesses, so the row says so.
- **Every switch moved from the options menu into the `Set` list.** They are the
  same kind of thing as the volume and the opacity - something to set, right
  here - and keeping the list in two places meant knowing which of the two a
  setting had been filed under. What stayed in the options menu is what is not a
  setting: the shortcuts folder, the displays, the audio devices, About, Reset,
  Exit. The settings window still lists them beside the numbers and the keys.
- **A button's word is fitted to the button in two dimensions.** Up to three
  letters it stays on one line; four to six are stacked on two, at a size the
  button can hold. Shrinking a long word until it fits across ends at a size
  nobody can read.
- **The box is a keyboard place now.** `Space` on `Dir` or `Set` opens the list
  *and* hands it the keyboard; on any icon that has a box, **`Up`/`Down` step
  into it** - `Down` at the top row, `Up` at the bottom - rather than stepping
  over it to the next row of icons, since the box is what sits above and below
  that icon. Inside, `Up`/`Down` walk the rows with the selected one filled and
  marked, `Enter` takes it, `Esc` leaves. **`Left`/`Right` leave the
  box and move to the icon beside it** - a list is a column, and sideways is
  what those keys mean everywhere else in the grid; the icon they land on opens
  its own box. The same keys in all three lists, because it is the same box.

## [v0.17.0] - 2026-08-23

### Added
- **A `Dir` button, and the folder shortcuts behind it.** Point at it, or click
  it, and the folders open in a list beside the button; pick one and it opens in
  Explorer. The list is not something new to maintain: any `.lnk` in the
  shortcuts folder whose **target is a directory** goes here instead of taking a
  slot in the icon grid. A folder is a place to go rather than a program to run,
  and a dozen of them crowd out the programs. `show folder` lists them in the
  command box; a shortcut that does not resolve to a real directory stays an
  icon, which is what keeps anything unusual working.
- **`Set` gathers the five controls that were five buttons.** Volume, brightness,
  opacity, the pin and the options menu are rows of a list now: the wheel over a
  row moves that value in the same 5% steps, a click does what clicking the
  button did, and each row carries its own reading so a value can be set without
  looking anywhere else.

  It is **the same box the tabs use** - the same window, the same placement
  rules (above or below, clear of the button, inside the work area), the same
  keys and the same painting. Only what fills the rows and what picking one does
  are per list. Three lists, one box.

### Changed
- **The buttons carry words rather than single capitals:** `ReS`, `Mv`, `Ext`,
  `Dsk`, `Com`, `Nt`, `Clp`, `Dir`, `Set`, and `Vol`, `Bri`, `Alp`, `Pin`, `Opt`
  in the Set list. A capital is legible only to someone who already knows the
  program. The text is **measured and fitted to the button** rather than the
  button to the text, so the fit holds across DPI, icon sizes and font names.
- **Nine buttons on the row instead of twelve.** What left the row did not leave
  the program: the five are in the Set list, and the folder shortcuts are behind
  Dir.

## [v0.16.3] - 2026-08-18

### Changed
- **A note's row in the list follows its first line as it is typed.** The first
  line is the title, but the list only heard about a new one when the editor
  closed, so a note could sit there called `(untitled)` while its name was
  already on screen in the window next to it.

  Only the first line is read - `EM_LINELENGTH` and `EM_GETLINE` rather than the
  whole document - so this runs on a keystroke without undoing the reason
  `EN_CHANGE` stopped copying the text at all. The list is told only when the
  title actually changes, which is rarely, and the rule that turns a first line
  into a title now lives in one function instead of two, so the caption and the
  row cannot disagree about what a note is called.

## [v0.16.2] - 2026-08-18

### Fixed
- **The widgets stop smearing as they get more transparent.** The floater, the
  taskbox and the toolbar drew their text with ClearType - two of them by asking
  for it, the floater by taking the default that becomes it. All three are
  layered windows, so everything they draw is blended with the desktop
  afterwards, and ClearType colours the sub-pixels of a glyph edge for the
  background it was drawn on: blend that with a desktop it knows nothing about
  and the fringes line up with nothing. The text picks up colour casts, the
  whole widget reads as washed out, and it gets worse the more transparent the
  window is - which is exactly the shape this was reported in, and why turning
  the opacity up made it look right again.

  They ask for greyscale antialiasing now, which survives a blend. On an opaque
  window ClearType is the sharper of the two; on a translucent one it is the
  wrong tool, and these three windows are translucent by design.

## [v0.16.1] - 2026-08-17

### Added
- **`show theme`,** a diagnostic for the floater looking faint. Colour, opacity
  and a paint that did not happen are indistinguishable from across the room, so
  it prints all three: the light/dark and high-contrast flags, the scheme
  actually in use and the two it is chosen from, the opacity the program
  believes in **and** the one the window reports back, the GDI object count, and
  whether the clock and date fonts exist. Run it while the widget looks wrong
  and the answer is read rather than guessed.

### Fixed
- **A theme change read the colours too early.** Windows announces the change
  before its own colours have finished changing, and `GetSysColor` answered
  during that window returns the values on their way out - so the widgets could
  be left painted from a palette nobody chose, pale against the desktop, until
  something else forced another refresh. The theme is read once more 400 ms
  after the broadcast, which is the shape of this bug rather than a guess at it:
  a light/dark switch is exactly the moment it was reported at.

## [v0.16.0] - 2026-08-16

### Added
- **1024x768 joins the 4:3 sizes,** above 1280x960, so that group now reads
  640x480, 800x600, 1024x768, 1280x960. It is in both places the presets appear:
  the task icon's menu and the maximize-button menu, and `show resize` in the
  command box.

  Two consequences worth saying out loud. **The preset numbers after the second
  one shift by one** - `resize <window> 3` is 1024x768 now, and what was 3 is 4 -
  because the numbers are positions in one shared list rather than names.
  And **1280x960 keeps its `D` access key**: the new size takes `G` rather than
  the `D` that would follow `A`, `S`, since a hand that learned `D` for
  1280x960 should not be re-taught for the sake of an alphabet.

## [v0.15.4] - 2026-08-16

### Fixed
- **The floater no longer goes faint grey.** Two things could leave it looking
  like a ghost of itself, and both are the same mistake underneath: a layered
  window that does not paint is not blank on screen, it is whatever the
  compositor still holds.
  - **A paint that gave up drew nothing at all.** The whole panel - plate,
    border, clock, bars - sat inside one `if` that needed a back buffer *and*
    both fonts. Any of the three failing, which GDI pressure or a font handle
    dropped by a DPI or theme change can do, meant the window kept the pixels it
    had. The plate is painted first and separately now, from a cached brush, so
    the widget always looks like itself; the content draws over it when it can,
    and a missing back buffer paints straight to the window instead of skipping.
  - **The opacity was set once and never asserted again.** It is a window
    attribute the compositor holds rather than something a repaint restores, and
    the floater is hidden, moved and shown again every time the taskbox opens
    and closes. It is set again where that happens, on a display or DPI change,
    and once in the five-second revalidation - so a widget that does come back
    wrong heals itself within seconds instead of staying that way.
- **`WM_ERASEBKGND` is answered.** The paint covers the client area completely,
  so the erase could only ever show the shared widget colour in the gap before
  it - one more way to catch the floater mid-repaint looking wrong.

## [v0.15.3] - 2026-08-15

### Removed
- **The false-positive notice is gone from both READMEs.** It was written for
  the v0.8.0 downloads that browsers and scanners warned about; the builds since
  then have not reproduced that, and a banner about a warning nobody is getting
  is itself misleading. Releases from here on carry no such block either. The
  notes already published are left exactly as they are - they were true when
  they were written, and rewriting release history to look tidier is not a thing
  this project does.

### Changed
- **Section 2 is about checking a download rather than about scanners.** What is
  worth keeping is what a reader can act on: the SHA-256 of both files, the zip,
  the source, and the fact that the binary imports nothing that could reach the
  network. The two features switched off pending code signing are still said
  plainly, since the `O` menu shows them greyed and a reader deserves the reason.

## [v0.15.2] - 2026-08-15

### Changed
- **Hover-to-open answers over the clock's minutes alone.** The whole clock was
  the target in v0.15.0, which is still most of the width of a small widget. The
  two minute digits are a target you have to mean, and they sit at the end of
  the clock, so the pointer reaches them from outside the widget rather than by
  crossing it. As before, the behaviour is off by default and lives under
  **Options → Open the Taskbox on Hover** (`hoveropen` on the command line), and
  **a click still opens the taskbox from anywhere on the floater**.

## [v0.15.1] - 2026-08-15

### Fixed
- **The settings window paints the page the other document windows paint.** It
  had no background of its own, so the class brush showed: the widget colour,
  which is the system theme inverted, standing behind and around a page that was
  not inverted. It now fills its client area the way the notes, the clipboard
  and the command box do, and follows a light/dark switch while open.

## [v0.15.0] - 2026-08-15

### Added
- **Two more switches under `Options`.** **Outline the Window Under the
  Pointer** and **Show a Window's Tabs on Hover** - the frame a task icon draws
  around the window it stands for, and the list of tabs that opens beside a
  tabbed window's icon. Both were unconditional; both are on by default and now
  answer to the menu, `write option`, and the settings window. The gate is
  inside each feature rather than at its call sites, so the mouse and the
  keyboard focus obey it alike.

### Changed
- **Every name on the command line is one word.** `hoveropen`, `windowoutline`,
  `tabbox`, `captionmenu`, `statbars`; `showtaskbox`, `opentaskbox`,
  `commandbox`, `fontup`, `fontdown`, `scrollmode`, `historymode`, `focusinput`,
  `newnote`. Hyphens were a keystroke to remember in the middle of a name, and a
  unique leading piece of any name is enough anyway - `w o hover on` still
  works. A `config.ini` written before this reads correctly: the old spelling is
  looked up when the new one is not in the file, and the next change writes the
  new one.
- **The settings window wears the command box's colours and font.** The two are
  the same kind of surface - a fixed-pitch page of rows that have to line up in
  columns - and a settings window in a different face from the console that sets
  the same settings reads as a different program. The size is shared too, so
  `Ctrl + Wheel` in either moves both, and `[settings] font_size` is gone.
- **`Floater Shows System Bars` is now `Stat Bars on the Floater`.** Every other
  row in that menu is a name for what you get rather than a sentence about what
  the program does, and "stat bar" is what a reader of English already calls a
  small bar standing for a number. On the command line it is one word - `write
  option statbars off`, or `w o stat off`, since a unique leading piece of any
  name is enough. The settings file key is untouched, so nothing anyone had set
  is lost.
- **Hover-to-open answers over the clock only.** With the option on, the whole
  floater used to be the target, so a hand crossing the system bars on its way
  somewhere else brought the dashboard up. The clock is a small target in the
  middle: resting on it is a decision. A **click still counts anywhere on the
  floater**, clock included.
- **The taskbox only flashes when a key asked for it.** Three blinks exist to
  find a window that appeared where the eye was not looking; a click is aimed at
  the thing it opens, so it does not need them. The global hotkey and `T` keep
  the flash.
- **Every setting is in `config.ini`.** The note font size, the two list sort
  orders and the list window's geometry were in `note/note.ini`; they are in
  `[note]` with everything else now, and the old file is read once as the
  default so nothing is lost. What stays beside the notes is what is about a
  note rather than about the program - archived, created, and each note window's
  own place.

- **WASD walks the settings window,** as it walks the taskbox grid, so a hand on
  the home row does not have to leave it. `Ctrl+W` still closes and `Alt+A` still
  moves the window: only the bare letters navigate.
- **The bare arrows and WASD cannot be bound to anything.** They are how a
  window is walked rather than a function - in the grid, in the settings list and
  in the tab box alike - and a window whose navigation had been bound elsewhere
  could not be walked at all. Held with Ctrl or Alt they bind like any chord.

### Fixed
- **A key being bound no longer runs while it is being bound.** Pressing `F1` in
  the settings window recorded `F1` and opened About, because an accelerator is
  translated before any window sees the key. The message loop stops translating
  while the settings window is listening.
- **Any one key can be removed, not just the last.** Each chord is its own row
  under its function now, so `Del` takes away the one under the cursor and
  `Enter` replaces it with the next key pressed - and the old chord goes only
  once the new one is in, so `Esc` halfway through leaves the binding as it was.
  On the function row itself, `Del` clears every key and `R` restores the
  default.

## [v0.14.0] - 2026-08-14

### Added
- **A settings window** — `Settings...` in the `O` menu, `Ctrl + ,`, or
  `settings` in the command box. One list in three parts: the switches, the
  numbers, and every key as **window, function, chords**. Nothing in it has an
  OK button; a change reaches `config.ini` and the running program the moment it
  is made, the way the wheel and the menus always have. It is a view of the same
  three tables the command box writes through, so the two can never disagree.
- **Keys are configurable, per window.** They are kept as three levels - the
  window the key is pressed in, the function it runs, and the chords that reach
  it, from none up to four. A window is part of the identity rather than
  decoration: `notes` is one row in the floater and a different row in the
  taskbox, and the same chord has always meant different things in each. Both
  Win32 shapes a binding must take - the registered system hotkeys and the
  accelerator tables - are built from that table, so a rebind needs no restart.
  `show key`, `bind` and `unbind` do it from the command box; the settings
  window does it by waiting for the chord itself. Seven windows are covered:
  `system`, `widget`, `floater`, `taskbox`, `commandbox`, `note` and
  `clipboard`. Navigation and editing keys are deliberately left out - the
  arrows and the grid keys are the taskbox's own language, and the keys inside a
  note editor belong to the text under the caret.
- **`Open the Taskbox on Hover`,** off by default. v0.13.0 made opening a click;
  this brings the old behaviour back for anyone who wants it, as a switch rather
  than a build.
- **`show option` and `write option`,** so every switch can be read and set from
  the command box as the numbers already could be.
- **Ctrl+N opens the note list from the floater.** It answered Ctrl+E and Ctrl+L
  already, and a key that works in one of the two surfaces and not the other
  reads as a key that sometimes works.

### Removed
- **The `F` toolbar button is gone,** and with it floater-adjust mode. It existed
  because the pointer arriving on the floater expanded it, so tuning or moving
  the floater needed a state where that did not happen. Opening is a click now,
  and the mode had nothing left to protect: **the floater can simply be
  dragged** - a press and release is still a click, a few pixels of travel is a
  drag - and `Ctrl`/`Alt + Wheel` tune its size and opacity where it stands. The
  toolbar is twelve buttons instead of thirteen.

### Changed
- **Every switch is under `Options` in the `O` menu.** They used to sit loose at
  the top of that menu, three of them, each written out three times - a menu
  item with its own command id, a branch in the floater's WM_COMMAND, and
  nothing at all on the command line. One table (`hg_options.c`) holds them now,
  and the submenu, the listing and the setter are each a loop over it.
- **`Floater Shows System Bars` has a control.** It was in `config.ini` and
  nowhere else.
- The `[hotkeys]` pair of numbers is read once, to carry a hotkey chosen before
  this version into `[keys.system]`, and is then no longer used.

## [v0.13.0] - 2026-08-14

### Changed
- **The taskbox opens on a click, not on hover.** Passing the pointer over the
  floater now does nothing at all: a left click toggles the dashboard, exactly
  as the global hotkey and `T` do. Hover-to-expand made the floater a trap on
  the way to anything behind it - a cursor crossing the widget on its way
  somewhere else expanded the whole dashboard over the desktop, unasked. Opening
  is now something you say, not something the pointer says for you.
- **The floater drag rule reads the same, for a new reason.** Dragging it still
  needs `F` adjust mode, because outside that mode a press and release on the
  floater is a click and a click toggles the taskbox.

## [v0.12.1] - 2026-08-13

### Changed
- **The function buttons and shortcuts have no background plate.** The desktop
  shows through behind them, the same as everywhere else in that window since
  v0.11.0. **`A`, `B` and `V` keep theirs**, because for those three the
  background colour is not decoration: it is the reading, brightening with the
  opacity, brightness and volume they set.

  This also corrects an accident of making the toolbar transparent. Those plates
  were painted as the *inverse* of the toolbar's background, and the inverse of
  the colour key is very nearly white - so making the toolbar see-through had
  left a grid of near-white squares on it.
- **One white line around each lettered function button.** With the plate gone
  the letters would float loose over the desktop, and a button needs an edge to
  be a button. The line is drawn after the hover fill, so the yellow still fills
  the button and the line still frames it.
- **The window outline is thicker** - six pixels rather than three. It is read
  from the corner of the eye, while the eye itself is on the taskbox, so it
  wants to be a signal rather than a hairline.

## [v0.12.0] - 2026-08-13

### Added
- **Pointing at a task icon outlines its window on the desktop.** A frame is
  drawn around the window itself, so "which one is that?" is answered by where
  it is and not only by its name. Nothing else happens: the window is not
  raised, not focused, and the outline takes no clicks - pointing at something
  is not choosing it. The keyboard focus does the same as the pointer, and every
  task icon gets it, not only the ones with tabs.

  A **minimized** window is left alone; there is no place on the desktop to
  point at, and a frame around where it used to be would be a lie. A window
  **behind** others is outlined all the same - the frame says where it is, not
  what is in front of it.

  It is one layered popup with a see-through middle, marked transparent to the
  mouse so it cannot eat the hover it exists to serve, and no-activate so it
  cannot take the keyboard. The rectangle comes from the window's extended frame
  bounds rather than `GetWindowRect`, which since Windows 10 includes an
  invisible resize border several pixels wide.

## [v0.11.0] - 2026-08-13

### Changed
- **The taskbox has no background of its own.** The desktop shows through
  between the icons; only the icons, their labels and the border are drawn. The
  window was already layered with a colour key for exactly this, so it costs
  nothing. One consequence worth knowing: colour-keyed pixels are click-through,
  so a click in the gaps lands on whatever is behind - drag the box by its `M`
  button rather than by the space between icons. The collapse-on-leave test is
  unaffected, because it compares the cursor position against the window
  rectangle rather than waiting for mouse messages.
- **`help key` is grouped by where you press the keys.** One flat list was the
  wrong shape: the same key means different things in the taskbox and in a note,
  and a reader is always standing in one of those places. `h k` prints an index
  and then every group; **`h k <topic>`** prints one, and a topic needs only
  enough letters to be the only match - `h k f`, `h k g`, `h k n`, `h k t`, and
  `cl` or `co` for the two that start with c. The topics are global, floater,
  taskbox, commandbox, note and clipboard.
- **The tab hover box never covers the icon.** It sits flush above or below it,
  never beside it, running right from the icon's left edge or left from its
  right edge so the icon's whole width falls inside the box's. Of those four
  placements it takes the first that fits the display whole, which is what stops
  it running off an edge near a corner.

### Added
- **The clipboard history answers the same four adjustments the command box
  does**: `Ctrl + Wheel` for text size, `Alt + Wheel` for opacity, `Alt` +
  arrows to move it and `Ctrl` + arrows to resize it. Both settings are this
  window's own, kept under `[clipboard]`, rather than the shared ones the
  taskbox uses. `Ctrl` + left/right stops meaning "by word" in its search box as
  a result - the same trade the command box's input box already makes.

## [v0.10.6] - 2026-08-13

### Added
- **`Ctrl + L` calls up the clipboard history** from the floater or the taskbox.
  It is the letter the toolbar button already carries, so the key and the button
  say the same thing, and it toggles the way that button does: already in front
  means hide, minimized or buried means come back.

## [v0.10.5] - 2026-08-13

### Added
- **`Ctrl + E` opens the command box** from the floater or the taskbox, beside
  the bare `C` that already did. `E` for Execute, which is what the box's own
  button says, and it is reachable with the left hand while the right one is on
  the mouse. The letter was free of every Ctrl binding in the program and of
  every toolbar letter.

  `Ctrl + R` was the first thought and is not available: **Reset Settings**
  holds it. Handing it over would have meant taking the key from a destructive
  action and leaving it beside the one a hand reaches for when it wants a
  command line - so reset keeps `Ctrl + R`, `Ctrl + Shift + R`, `F5`, `Ctrl + 0`
  and its menu entry, unchanged.

## [v0.10.4] - 2026-08-13

### Added
- **`Ctrl + W` closes the About window** too. All five document windows - note
  editors, the note list, the clipboard history, the command box and About -
  answer it now, which is the same set that takes the document colours and the
  document accelerator table.

## [v0.10.3] - 2026-08-13

### Fixed
- **`Del` in the clipboard history's search box deleted the newest clip instead
  of a character**, and swallowed the key so the character stayed. The `Del`
  handler added in v0.9.0 sat in a subclass shared by the list and the two edit
  boxes, without checking which one it was on: an EDIT answers 0 to
  `LB_GETCURSEL` and `LB_GETITEMDATA` the way it answers any message it does not
  know, and 0 reads as "row 0, history entry 0". It now runs only on the list.

### Added
- **`Ctrl + W` closes the clipboard history and the command box** too, the way
  it already closed a note. In the command box it differs from `Esc`, which goes
  back to the taskbox; `Ctrl + W` just closes. The Execute button answers it as
  well - a button keeps the focus once clicked and forwards no keys to its
  parent, so without that the key would have gone quiet after one click.

## [v0.10.2] - 2026-08-13

### Added
- **`Ctrl + N` opens the note list** from the taskbox, alongside the bare `N`
  that already did. It was bound to nothing at all - the only two places `'N'`
  appeared in the source were that key and the letter drawn on the toolbar
  button - so it cost nothing to answer the key a hand reaches for out of
  habit. The list opens with `+Add Note` selected and the arrows already
  meaning what they should, so the shortcut needs no second half.
- **`Ctrl + W` closes a note editor or the note list**, the way it closes a tab
  or a document elsewhere. `Esc` still does the same. A note's text is written
  on the way out, as it is for every other closing path.

## [v0.10.1] - 2026-08-12

### Fixed
- **`Ctrl + X` cuts again.** It was a second binding for quitting the program,
  which meant a note, a command line and a search box all lost their cut to it.
  **Quit is `Ctrl + Q`** now, and nothing else.
- **The program's keys no longer reach into the text you are typing.** The
  accelerator table was translated in the message loop, and
  `TranslateAccelerator` matches on the message rather than on the window it was
  headed for - so every one of those keys applied to every keystroke in the
  process. `Ctrl + X` was the one that got noticed, but it was never alone:
  `Ctrl + R` and `F5` reset every setting mid sentence, `Ctrl + 0` and
  `Ctrl + +/-` resized the widget font instead of the text, and `Alt + F4`
  closed the whole program rather than the note it was pressed in. The document
  windows - notes, the note list, the clipboard history, the command box, About
  - now keep the standard Windows editing keys, and only `Ctrl + Q` and `F1`
  reach past them.
- `Alt + F4` closes the focused document window. From the floater or the
  taskbox it still quits.

### Changed
- The note's context menu names the key beside each editing command, and the
  command box's `help key` lists the editing and quit keys.
- The Korean key reference no longer lists `Ctrl + Enter` in the command box;
  that binding was removed some releases ago and `Enter` runs what is typed.

## [v0.10.0] - 2026-08-12

### Changed
- **The document windows follow the system theme directly.** Notes, the note
  list, the clipboard history, the command box and About are pages of text, and
  a page should look like the theme says a page looks: **light grey with black
  text on a light theme, black with white text on a dark one**. The widgets -
  the floater, the taskbox, the toolbar - still invert the theme on purpose,
  because a control has to stand out against the desktop behind it. That
  inversion was never right for a note, which on a dark desktop came out as a
  white sheet. Under high contrast both defer to the colors the user chose
  there.
- **The sunken edge is gone from the text controls in those windows**, and each
  window's background is now the same color as the control filling it. The
  border was drawn to separate two surfaces that are now one color, so it was a
  line around nothing.
- **Small input fields keep one step of separation**: the clipboard's search and
  count boxes and the command line are painted white on a light theme and near
  black on a dark one, a shade off the page behind them. Without an outline
  *and* without a shade, a box you are meant to type in gives no sign of being
  one.
- **A document window's title bar is coloured like its page.** Left on the
  widget scheme it inverted against its own client area - a light grey note
  under a black caption - which read as two windows stuck together. The frame
  colour stays with the widgets: it is the one edge that has to be findable
  against whatever is behind the window.

### Fixed
- A theme change now repaints these windows regardless of the order
  `WM_SETTINGCHANGE` reaches them in. Each one refreshes the theme globals
  itself rather than assuming the window that owns them was served first.

## [v0.9.0] - 2026-08-10

### Added
- **A menu on each row of the clipboard history**, opened by a right-click or
  by the keyboard's context-menu key: **Copy to Clipboard**, **Delete**,
  **Delete All**. The row under the pointer is selected before the menu opens,
  because a menu that acts on something other than what was aimed at is a menu
  that deletes the wrong clip.
- **`Del` deletes the selected clip.** The selection stays at the same row
  afterwards, so clearing out a run of stale clips is one key pressed
  repeatedly rather than a re-aim between each.

### Changed
- **The clipboard history is an ordinary application window now**, like the
  notes and the command box: its own taskbar button, its own Alt-Tab entry, and
  all three caption buttons - minimize, maximize, close. It was a tool window,
  which Windows keeps out of both; that is right for a widget's controls and
  wrong for a window you work in.
- **Taking a clip no longer closes the window.** Copying three things out of
  the history in a row is exactly what a history is for, and the window
  vanishing after each one meant reopening it in between. Closing is `Esc`, the
  X, or the `L` button, and nothing else - clicking elsewhere leaves it alone.
- The `L` button only hides the history when it is already the window in front.
  A minimized or buried history is raised instead, since that is what asking
  for it means.

## [v0.8.2] - 2026-08-09

### Changed
- The two suspended menu entries now read **`(off in this build)`** rather than
  `(temporarily disabled)`. The greyed state already says the entry cannot be
  used, so repeating it spent the only words there was room for; what the grey
  cannot say is that this was a decision about this version rather than a
  fault - and "in this build" carries the temporariness without claiming it.
  No behaviour changed.

## [v0.8.1] - 2026-08-09

### Changed
- **Two features are temporarily disabled: Start with Windows, and the menu on
  the maximize button.** Some browsers and antivirus products warned users away
  from the v0.8.0 download; v0.8.1 does not reproduce that.
  - **Why they were flagged.** A scanner cannot judge what a program does with
    a given Windows facility, so it classifies by the facility: programs using
    a particular API are statistically more often malicious, and a harmless
    feature joins that category simply for using it.
    - **Menu on Maximize Button** needed a **global mouse hook** to reach other
      applications' windows. The hook read the mouse button and the cursor
      position, to decide whether that point was a maximize button — **no
      keyboard hook exists anywhere in this program** — but a global input hook
      is the same family of call a keylogger uses, and static analysis cannot
      easily separate the two.
    - **Start with Windows** wrote one value named `hgfloater` under the
      ordinary per-user `Run` key and removed it when switched off. That is
      also where malware registers itself to survive a reboot, so writing there
      raises the score by itself.
    - Neither was doing anything harmful. Both were classified by the company
      their APIs keep, and an unsigned binary with few downloads has no
      reputation to offset it.
  - **What this build does instead:** it installs **no hook** and writes
    **nothing to the registry**. Both menu entries stay in the `O` menu, greyed,
    reading `(off in this build)`, because a feature that vanishes entirely
    leaves you unsure it was ever there.
  - **Nothing else changed.** Task switching, the tab hover list, notes,
    clipboard history, monitor previews, the command box and per-display
    brightness and scaling all behave exactly as in v0.8.0.
  - Both features come back once the underlying issue is addressed properly -
    code signing being the real fix. In the source this is one compile-time
    switch (`HG_TEMP_DISABLE_FLAGGED_FEATURES` in `hg_common.h`); the code is
    intact and turning it off restores them unchanged.

## [v0.8.0] - 2026-08-08

### Fixed
- **`Space` on a focused window now leaves the dashboard, the way clicking
  one does.** Activating raised the window but left the taskbox standing;
  with the mouse it collapsed anyway, because the pointer had wandered off it
  and the hover timer noticed. The keyboard had no such accident to rely on.
  Activating any task icon now closes the tab box, collapses the taskbox, and
  then brings the window forward - in that order, so the window is the one
  that ends up in front.
- **The tab box opens for the keyboard too.** It appeared on hover only, so
  arrowing onto a browser icon showed nothing. Arriving at an icon is
  arriving at an icon, whichever way you travelled.

### Changed
- **The tab box has two states, and the keys say which.** Open, it answers
  the digits (`1`-`9`, `0`) and `Tab`. `Tab` steps into it - a frame appears -
  and then the arrows, `Home`/`End`, the letter labels, `Enter`/`Space` and
  `Esc` are all the box's. The letters wait for `Tab` on purpose: outside the
  box `WASD` moves the grid and `C` and `N` open other windows, and a hover
  box that silently took those would break navigation every time the focus
  passed a browser.
- **`show tabs` (`s t`) now lists tabs, not diagnostics.** Every tab of every
  tabbed window, numbered across all of them - and **`go tab <n>`
  (`g t <n>`)** switches to one, whichever window is holding it. The old
  counters moved to **`show tabsinfo`**, which has no shorthand because it is
  a diagnostic.

### Added
- **`clear` (`cls`)** empties the command box transcript. The command history
  is a different thing and is left alone.
- `help` lists `clear`, and points at `help key`, which is not a command and
  so had no row of its own to be found by. The key help itself now describes
  the history list, the mode frames, and the three-line input.

## [v0.7.0] - 2026-08-07

### Changed
- **Tabs left the grid. They live in a hover box now.** A tabbed window is one
  icon again, like every other window — orderable, uncrowded — and hovering
  that icon opens a small list beside it: one row per tab, titled, labelled
  `1`-`9` then `a`-`z` then `A`-`Z`.
  - **Press a label and you are there.** `0` goes to the last row. `Up`/`Down`
    (and `Home`/`End`) move the selection, `Enter` switches, **`Esc` closes
    the box and leaves the keyboard in the taskbox** where it was. Clicking a
    row switches to it; right-clicking closes that tab and keeps the list up,
    because closing several is why anyone right-clicks a tab list.
  - **Nothing is read until somebody looks.** Opening the box asks for that
    one window's tabs, once. The background cadence that kept every tabbed
    window's titles fresh — the five-second re-asks, the per-window title
    gate, the circuit breaker — is gone, along with the work it did whether
    or not anyone cared.
  - The box never takes the focus (hovering must not be a destructive act),
    and the taskbox counts it as inside itself, so moving the pointer from
    the icon onto the list does not collapse anything.
  - Design and the two field studies behind it:
    `docs/RFC-2026-07-tabs-as-task-icons.md`, section D8.

## [v0.6.1] - 2026-08-07

### Fixed
- **`N` in the taskbox opens the note list now.** The N toolbar button had the
  mouse's path and nothing had the keyboard's; bare `N` joins bare `C` (the
  command box) as a keyboard shortcut. `Shift+N` stays the task badge it was.
- **Ctrl+Wheel font sizing in the command box works with every mouse.** The
  handler existed but trusted the modifier flags packed into the wheel
  message, which some pointing drivers do not set; the key state is now asked
  directly as well. The history list follows the font too.

### Changed
- RFC-2026-07 D8 (the agreed tab hover sub-box, not yet implemented) now
  specifies the interaction in full: title rows labelled `1-9`, `a-z`, `A-Z`,
  a label key jumps to its row and `0` to the last, Up/Down move, Enter
  switches, Esc closes the box and hands the keyboard back to the taskbox.

## [v0.6.0] - 2026-08-07

### Added
- **Modes you can see.** The command box's scroll mode frames the transcript
  and history mode frames the input, in the system accent color - the pane
  with the border is the pane the arrow keys now belong to. The title bar
  keeps saying it in words.
- **History mode is a list now.** `Ctrl+H` shows every remembered command over
  the transcript, numbered from `1:` at the oldest so a number never changes
  meaning. Up/Down move the selection, `Enter` or a double click puts the line
  into the input - not runs it - and leaves the mode.

### Changed
- **The command box input is always exactly three lines tall.** It tracks the
  font, not the window: resizing the box resizes the transcript, and typing or
  pasting past three lines scrolls the input.
- **The maximize-button menu reaches more windows.** hgfloater's own document
  windows - notes, the note list, the command box, About - now get it (only
  the floater, taskbox and toolbar stay out), and so do windows that answer
  title-bar hit-tests their own way, PuTTY-style: when a window will not name
  its caption buttons, the DWM-computed button bounds decide which third the
  click landed on (windows without a real maximize button stay excluded).

### Fixed
- **The maximize-button menu now dismisses when you click elsewhere.** The
  menu's owner needs the foreground for an outside click to reach it, and a
  plain SetForegroundWindow is refused while another application holds it -
  which it always does here, the click having just landed on that
  application's title bar. The menu now takes the foreground the way the rest
  of hgfloater does, and an outside click closes it like any other popup.

## [v0.5.0] - 2026-08-07

### Changed
- **The command box is an ordinary application window now**, like the note
  windows before it: its own taskbar button, its own Alt-Tab entry, no owner,
  and no always-on-top. Its opacity keeps working.
- **All three caption buttons — minimize, maximize, close — on the command
  box, the note list, and every note editor.** Minimize parks them on the
  taskbar; Alt-Tab (with Shift for the reverse walk), the taskbar, and the
  taskbox's own task icons all reach them.
- **The tab breaker's thresholds caught up with the scoped read.** Field
  numbers confirmed B3 working (an Explorer window's refresh fell from
  1018 ms at discovery to 112 ms scoped; Chrome to 64 ms) - and showed the
  50 ms slow threshold, tuned in the full-walk era, putting those healthy
  refreshes on a 30-second leash. Slow now starts at 150 ms, the two-minute
  circuit at 300 ms; three straight failures still trip it.

## [v0.4.0] - 2026-08-07

### Changed
- **Tab reading now remembers where the strip lives, and asks only there.**
  The full-window walk made the browser consider its entire tree - Explorer
  its whole folder view - to find a strip that sits in one small container.
  Discovery now records the container's address (the property chain from the
  window root down to it) and every later refresh descends that chain and
  queries just the container's own subtree. A provider that rebuilds its tree
  breaks the chain harmlessly: that one ask falls back to a full discovery
  and re-records it. In `show tabs`, `s` marks the scoped reads (the expected
  steady state, at a fraction of the old cost), `f` the full discoveries,
  `!` a failed ask.

### Removed
- **The MSAA fast path, retired by its own exit criteria.** Two instrumented
  field runs settled it: Chrome still exhausted a tripled budget with a
  top-first walk - MSAA's one-round-trip-per-element cost model cannot beat
  a one-call UIA query at any budget - and Explorer's XAML bridge never
  exposes a tab-strip role at all. Meanwhile the failed attempt was adding up
  to 60 ms to every ask. The RFC keeps the full story, including the numbers.

## [v0.3.2] - 2026-08-06

### Changed
- **The MSAA verdict came back `b` - the budget, not the browser - so the
  budget moved.** The first instrumented field run showed Chrome serving a
  real MSAA tree and the walk dying on its 20 ms clock, which at a few
  milliseconds per cross-process element access bought only a handful of
  nodes. The budget is now 60 ms - still far under the 106-649 ms UIA walks
  it replaces - and the walk visits each level's children **top-first**,
  because on a clock budget the order of the walk is the walk, and a tab
  strip is by definition at the top of the window.
- **Every tab-class window now gets the MSAA attempt**, not just Chromium:
  the same field run showed the priciest walks were Explorer's (644 and
  649 ms over UIA), which the old gate never even offered to MSAA. Adoption
  stays per attempt, on evidence, with UIA in the same ask when the walk
  finds no real strip - and `show tabs` still says how far each attempt got.

## [v0.3.1] - 2026-08-06

### Added
- **`show tabs` now says why the MSAA fast path did not answer**, with a
  letter beside the provider: `r` no root object, `e` a stub tree with
  nothing enumerable, `b` budget exhausted, `t` an empty strip, `x` a full
  walk that found no strip, `-` not tried. First field evidence showed every
  Chromium window answering over UIA; these letters decide between the two
  possible verdicts - Chromium withholding its real MSAA tree from clients
  that have not tripped its assistive-technology detection (`e`, which would
  retire the fast path for stock browsers), or budgets in need of tuning
  (`b`/`x`). The RFC records both roads.

## [v0.3.0] - 2026-08-06

### Added
- **A faster way to read Chromium tabs, and a sturdier worker to do it with**
  (RFC-2026-08). For Chrome, Edge and other Chromium windows the worker now
  asks through MSAA first - the accessibility interface Chromium itself calls
  complete - with a strictly budgeted walk (depth 8, 256 nodes, 20 ms) that
  never enters web content, so answering does not wake the browser's page
  accessibility machinery at all. The result is adopted only when a real tab
  strip answered; anything else falls back to UI Automation in the same ask.
- **`show tabs`** (`s t`) in the command box: what was queued, answered,
  failed, served by which provider, and how long each window's ask took -
  the numbers the whole tab pipeline is judged by, kept in memory only.
- **A per-window circuit breaker.** A window whose asks run slow (over 50 ms)
  or keep failing earns a quiet period - 30 seconds, or two minutes past
  200 ms or three straight failures - no matter what its title does.

### Fixed
- **A worker stuck in a cross-process call could come back from the dead.**
  Toggling tabs off waited two seconds, then reset the stop flag whatever
  happened - which is exactly what let a stuck worker resume when its call
  finally returned, and a re-enable then started a second one. The lifecycle
  is now an explicit state machine: a stuck worker stays marked STOPPING with
  its stop flag set, no new worker starts until the old one is reaped, and
  there is only ever one.
- **A queued window could be silently unqueued.** New request batches replaced
  the pending set, so a window waiting its turn could vanish - while being
  stamped as asked, leaving its tabs stale for up to 30 seconds. Requests now
  merge; nothing queued is dropped without being counted.
- **A transient read failure no longer folds a browser's tabs into one icon.**
  "The question failed" and "there are no tabs" were both a zero; they are now
  different answers, and a failure keeps showing what was known.
- A recycled window handle can no longer briefly wear another window's tabs:
  answers carry the process id they were read from, and a mismatch resets the
  slot instead of applying.

## [v0.2.1] - 2026-08-06

### Fixed
- **The moment after expansion could still hitch the whole machine with many
  browser tabs open.** v0.2.0 moved the tab walk off hgfloater's UI thread,
  but the walk's real cost lands in the browser: answering it makes Chrome or
  Edge materialize their accessibility trees, across their processes - and
  current Chromium drops that machinery when nobody asks for a while, so the
  first ask after an idle stretch pays for a full rebuild, of every browser
  window at once. Three changes (RFC D7):
  - A window is **re-asked only when its own title changed** - switching or
    navigating a tab retitles the window - with a 30-second backstop for what
    a title does not carry. Windows you never touched now cost nothing.
  - The walk's answer (names and positions) **rides back in a single call**
    via a UIA cache request, instead of two extra cross-process round trips
    per tab.
  - Batches are **staggered 150 ms apart** and the worker runs below normal
    priority, so no two browsers rebuild their trees in the same instant.
  - The trade, stated: a strip change that does not change the window title
    (a background tab quietly opened) can show up to 30 seconds late.

## [v0.2.0] - 2026-08-05

### Fixed
- **Title changes no longer cost anything while the taskbox is hidden.** A
  window retitling itself - a browser does it on every page - made hgfloater
  re-fetch that window's icon at once, through a pipeline that blocks on the
  target window and reads disk, whether or not the taskbox was even visible.
  The re-fetch now happens only while the list is on screen; an icon that
  changed meanwhile shows its old face until the window's next retitle.
  (Found by a full re-review of everything changed since the last audit; the
  rest of that delta - the worker thread, the caches, the caps - survived the
  re-read without a defect.)
- **The floater-to-taskbox expansion stuttered once Chrome had been running a
  while, with tabs shown as icons.** Reading another application's tabs is a
  UI Automation walk into that application, and Chromium's accessibility trees
  grow for as long as the browser runs - so a walk that cost nothing at
  startup cost a hundred milliseconds an hour later, and it ran on the UI
  thread, on the expand path. Enumeration now runs on a **background worker
  thread**: the dashboard draws from its cache immediately, files a request,
  and folds the answer in when it arrives. Expanding is instant no matter what
  the browser is doing; a brand-new browser window shows one icon for the
  fraction of a second its first answer takes, then fans out into tabs.
  - Requests keep their five-second cadence, now on a wall clock. Switching a
    tab or closing one from the icon menu still acts immediately.
  - Design notes: `docs/RFC-2026-07-tabs-as-task-icons.md`, section D6.

## [v0.1.0] - 2026-08-05

### Changed
- **The version scheme is now semantic, starting at v0.1.0.** Date-style
  versions (v26.08.05 and earlier) said *when* a build was cut but nothing
  about *how much* changed; from here the version says what kind of change a
  release carries, and the date moves to where a date belongs: the **About
  window (`F1`) now shows the exact build timestamp**, and the README opens
  with the version and build time of the release it describes. The version
  lives in `VER.txt`, which both build scripts read, so a release is one edit.
  (A `v26.08.05` release published earlier the same day carried the note-window
  change below; it was withdrawn in favour of this one, which includes it.)
- **Note windows stand on their own.** The note list and every note editor are
  now ordinary application windows (WS_EX_APPWINDOW) instead of tool windows:
  each gets its own taskbar button and Alt-Tab entry, and they appear in the
  taskbox's task icons like any other window. They were already unowned, so the
  taskbox never actually closed them - but as tool windows they fell behind
  whatever was clicked next with no button anywhere to bring them back, which
  read as "my note closed when the taskbox did". Now they stay reachable, and
  open, until closed.

## [v26.08.04] - 2026-08-04

A max-effort review of the whole codebase, aimed at the reported high idle CPU
occupancy and at bounding memory growth, confirmed fifteen root causes; all are
addressed here. The standing allocation policy applies throughout: fixed static
storage first, then arenas, ad-hoc heap last, and every cache carries a hard cap.

### Fixed
- **Idle CPU: the floater re-measured the same text every second.** Every paint
  asked for about eighteen ink extents, and each ask created a scratch DC and a
  32-bit DIB, rendered the text, and scanned every pixel row. Measurements are
  now cached per font and string in fixed storage; releasing any font retires
  the whole cache through a global generation counter, so a recycled handle
  value can never serve another font's numbers.
- **Idle CPU: the monitor preview captured the whole screen ten times a second
  no matter who was looking.** The rate now follows attention: 10 fps with the
  pointer on the preview (that is the remote-driving case), 5 fps while merely
  open, and one frame a second when other windows cover it completely - which
  the old visibility check could not see, because a covered window still has its
  visible bit set. The floater's clock tick also invalidated every preview a
  second time each second; that duplicate is gone.
- **Idle CPU: the temperature poll rebuilt its machinery every five seconds.**
  The PDH query - whose wildcard add parses the entire performance-counter
  registry - is now opened once and kept, each poll doing only a collect into a
  fixed buffer instead of open, parse, allocate, close. The WMI zone query
  drops its connection on failure (the module's own recovery contract, which
  the thermal path had been skipping) and a machine that keeps answering
  "no zones" earns geometrically longer pauses, up to about ten minutes,
  instead of a doomed cross-process query every five seconds forever. The GPU
  temperature poll's per-call heap allocation is now a fixed array.
- **Idle CPU: Explorer paths were re-resolved over DCOM every second.** The
  resolved path is now cached per window and revalidated only when the window's
  title changes - navigating changes the title, so nothing is missed, and both
  hgfloater's and explorer.exe's steady-state CPU drop while the taskbox is
  open. A window whose icon extraction keeps failing also now earns a growing
  retry pause (three quick tries, then doubling waits capped near half a
  minute) instead of re-running the full blocking pipeline every refresh.
- **Idle CPU: extra browser tabs cost an icon copy per tab per second.** Tab
  items now share the window's icon handle without owning it; the fan-out is
  rebuilt and released as one unit, so the share cannot outlive the owner.
- **Typing lag: the note editor extracted the whole document on every
  keystroke.** A keystroke now just marks the note stale; the text is pulled
  once, when the save timer, focus loss, or closing actually consumes it, and
  the caption redraws only when the title really changed. Typing cost no longer
  grows with the note.
- **UI stalls: brightness was read over DDC/CI every five seconds.** A DDC read
  blocks the UI thread for up to hundreds of milliseconds on slow monitor
  firmware. The cache now refreshes once a minute - the app's own setters stamp
  it immediately, so only outside changes (the Windows slider, monitor buttons)
  wait that long.
- **Unbounded memory: the clipboard history had no total cap.** Entry count and
  per-clip size were bounded, but 64 clips near the ceiling could hold over a
  hundred megabytes for the life of the process. A 4 Mi-character total cap now
  evicts oldest-first; the newest clip always stays.
- **Unbounded memory: the command box transcript grew forever, then went
  mute.** Once the EDIT control's internal limit filled, appends silently
  inserted nothing - commands appeared to run with no output. The transcript
  now trims its oldest lines past a fixed budget, so both the memory and the
  output keep working.
- **Leak: closing a monitor preview left its tooltip registered.** Destroying a
  tool's window does not remove the tool, so every open/close cycle grew the
  shared tooltip control's table. The preview now deletes its tool in
  WM_DESTROY, the one path every teardown crosses.
- **Leak: a full window table dropped icon handles.** When windows plus
  expanded tabs reached the item cap, both the expansion's copy-back and the
  refresh's early bail discarded items whose icons they owned without releasing
  them. Every drop path now honors the release contract.
- **Crash-adjacent: a failed note-editor creation could act on the wrong
  note.** The window learns which note it belongs to only after creation
  returns, so a WM_DESTROY from a creation that failed midway read the default
  slot of 0 and destroyed note 0's font under its live editor. The window now
  starts marked as belonging to no note.
- **Undefined behavior: a long number in a command overflowed.** More than ten
  digits drove the accumulating multiply past INT_MAX - undefined in C, and a
  wrapped value could name a real window. The parse now rejects the overflow
  before it can happen.

## [v26.08.03c] - 2026-08-03

### Fixed
- **The maximize-button menu appeared and vanished, and sometimes moved the window on its way out.** The menu opened on the press, while the right button was still held down — so the release that followed landed straight in a menu that had just appeared under the cursor. Sometimes that release only dismissed it, which read as a menu that flashed; sometimes it picked the entry sitting under the cursor, which is the first one, **Move to (0, 0)**, and the window jumped to the corner. The menu now waits for the release: the press arms it, the release opens it, and releasing somewhere else cancels it the way dragging off any button in Windows does.
  - The release is swallowed along with the press it belongs to. Only the press was taken before, which left the target application an up with no down in front of it — and an application that draws its own title bar may act on that.
  - The menu no longer opens on top of itself. The message that opens it is posted, and posted messages are dispatched inside a menu's own modal loop too, so a second right-click could stack a second menu; now that click dismisses the first one instead, which is what a click outside a menu should do.
  - The taskbox's own collapse timer no longer cancels the menu. The taskbox owns this menu, the cursor is out on somebody else's title bar the whole time it is open, and hiding a window cancels the menu it owns — so the flag that already holds that timer off for the taskbox's own menus now covers this one.

## [v26.08.03b] - 2026-08-03

### Added
- **A watchdog for the maximize-button hook.** Windows stops calling a low-level hook that has been too slow and tells nobody — the handle stays valid and the feature simply stops working. There is no API that answers "am I still hooked", so it is inferred every 30 seconds: the hook counts its own callbacks, and if the pointer has moved since the last check while the counter has not, the hook was dropped and is put back.
  - The inference has **no false positives**, which is the point. Pointer moved means mouse events happened, and a live hook is called for them. Pointer unmoved concludes nothing and does nothing — silence is not evidence of death, and re-installing on a hunch would churn the input path for no reason.
  - It costs one increment per mouse event, which is the only work the hook does for events this feature does not care about, plus one `GetCursorPos` every 30 seconds.
  - It rides the floater's clock, not the taskbox's refresh: that one only ticks while the taskbox is visible, and the taskbox is hidden most of the time.

## [v26.08.03] - 2026-08-03

### Changed
- The maximize-button hook is now taken out on **every exit the process can still run code on**: the ordinary path as before, plus `WM_ENDSESSION` when logging off or shutting down, plus an unhandled exception through a top-level filter that chains to whatever was there before, so a crash still crashes and still gets reported.
  - A force-kill was already safe and still is: a hook is a process-owned resource, and Windows removes the registration when the process ends however it ends. The deeper reason is that this feature never wrote anything into another process to begin with — there is nothing left behind to undo.
  - These handlers are not load-bearing. They exist because relying on a cleanup you never perform is how you find out it was not doing what you assumed.

### Fixed
- The hook could swallow a right-click and show nothing, if posting the message failed — a full queue, or the taskbox window going away between the check and the post. The click now goes through untouched in that case. Taking an input event and giving nothing back is worse than not having the feature.

## [v26.08.02] - 2026-08-02

### Added
- The maximize-button menu ends with **Close**, which shuts the menu and leaves the window alone. Escape and a click outside already dismissed it, but this menu opens on a button in someone else's title bar, where a stray click is likelier than usual to land on something that acts — so the harmless way out is on the menu, at the bottom, where anyone looking for "never mind" looks.

## [v26.07.31k] - 2026-07-31

### Added
- **Right-click the maximize button on any window** — the one left of the X — and hgfloater offers that window **Move to (0, 0)** and its size presets. The same entries the task icons offer, from the same preset table. Left-click still maximizes, and right-clicking a caption button does nothing in Windows, so nothing was taken away.
  - **Close hgfloater and the behaviour is gone.** Nothing is installed and nothing needs uninstalling; the hook goes in when the setting is on and comes out when it is off or the program exits.
  - Switched with **Menu on Maximize Button** in the `O` menu or `[etc] caption_menu`, defaulting on.
  - Windows running as administrator keep their ordinary behaviour: Windows does not let an unelevated program touch input bound for an elevated one, and hgfloater runs unelevated on purpose. Same for windows with no maximize button.
  - This is the only thing hgfloater does outside its own windows, and it needs a system-wide mouse hook. The hook reads the button and the coordinates and nothing else, there is no keyboard hook, and the README says so — a global mouse hook is also what a keylogger uses, so an antivirus may take an interest.
  - Design, costs and limits: `docs/RFC-2026-07-caption-button-menu.md`.

### Fixed
- `build.bat` was missing `hg_tabs.c`, so the Windows-side build script had been unable to build the tab feature since v26.07.31e. Both source lists are now checked against each other.

## [v26.07.31j] - 2026-07-31

### Changed
- **Tabs are not a browser feature.** Nothing in the mechanism ever was — the same UI Automation tree answers for any tabbed application — so the built-in list now covers **Firefox** (`MozillaWindowClass`), **Windows Terminal** (`CASCADIA_HOSTING_WINDOW_CLASS`) and **Notepad** alongside the Chromium family and Explorer. The `O` menu entry reads **Show Tabs as Task Icons** rather than naming browsers, and the `config.ini` comments say the same.

### Added
- `[taskbox] tab_classes` adds window classes to that list from `config.ini`, semicolon-separated, with no rebuild. A list compiled into the program can only ever be out of date; this is what makes the answer "what you tell us about" instead of "these five".
- `show windows class` (`s w class`) prints the window class beside each title, which is where the names for `tab_classes` come from. A setting whose value nobody can discover is a setting nobody can use.

## [v26.07.31i] - 2026-07-31

### Fixed
- **Middle-clicking a tab icon closed the whole application.** v26.07.31h taught the context menu about tabs and left this path behind, so the menu and the middle click disagreed about what Close means. A middle click on a tab now closes that tab, or nothing at all — never the window.
- **Explorer's tabs stopped appearing entirely.** The v26.07.31g fix required the tabs to sit under a `TabControl` parent, and what a XAML tab strip publishes as its container is not something this program gets to decide. Depending on another application's choice of control type was the mistake.
  - The strip is now found by **where it is** — tab items within the upper quarter of the window — with nothing assumed about their parent. That still excludes Explorer's Home page sections, which sit below the toolbar, and it no longer excludes Explorer's real tabs.
  - Tabs are ordered by their left edge rather than by tree order, so the number on an icon means the tab in that position on screen.

## [v26.07.31h] - 2026-07-31

### Fixed
- **Right-clicking a tab icon and choosing Close closed the whole window**, taking every other tab with it. The menu entry acted on the window because that is all a task item used to be; once a tab is an item in that menu, the menu has to mean what it says.
- Close on a tab now invokes **that tab's own close button**, so it closes exactly the tab that was right-clicked. Not `Ctrl+W`: that needs the window focused and acts on whichever tab is current rather than the one asked for.
- The entry is labelled **Close Tab** on a tab and **Close Window** on a window, and **Focus** on a tab switches to that tab rather than only raising its window.
- If the tab has no close button to invoke, **nothing happens** and the status line says so. The only other thing the code could do is close the window, which is what the reader was not asking for and cannot be undone.

## [v26.07.31g] - 2026-07-31

### Fixed
- **Explorer contributed Favourites, Recent and Shared as if they were tabs.** They are tabs — of Explorer's Home *page*, not of the window — and the query was asking for every tab item anywhere below the window, which is the obvious question and the wrong one.
- The search is now two steps: find the tab *controls*, take the one at the top of the window, and read that one's immediate children. A window's tab strip is above everything by definition of what it is; a tab control further down belongs to what is being displayed. An Explorer window showing only its Home page now contributes no tab items at all, which is right — it has one tab, and its Home page's sections are not tabs of the window.

## [v26.07.31f] - 2026-07-31

### Fixed
- **v26.07.31e crashed at startup.** The tab expansion added in that release declared its output array as a local: a `WindowItem` holds two 1024-character strings and is about 4 KB, so a thousand of them is a four-megabyte stack frame on a one-megabyte stack. The frame is reserved when the function is entered, so the early return for "tabs are switched off" never got a chance to run — the crash happened the first time the window list refreshed, which is during startup, on every machine, whether or not the feature was on.
- The array is now static, like every other item array in that file. The measured frame for `refresh_window_list` went from four megabytes to 160 bytes.
- The tab title cache is bounded at 16 windows rather than sized off the monitor count, which was both arbitrary and larger than it needed to be.

## [v26.07.31e] - 2026-07-31

### Added
- **Every task icon carries a label** in its top-left corner — `0`–`9`, then `A`–`Z` — and **`Shift` + that character** activates it, exactly as clicking would. Digits first because the first ten windows are the ones reached most often. `Shift` is what keeps this clear of the bare-letter grid movement and the bare `C` that opens the command box, which is what sank an earlier bare-letter scheme.
- **Browser and Explorer tabs can have their own task icons**, off by default, switched with **Show Browser & Explorer Tabs** in the `O` menu or `[taskbox] show_tabs` in `config.ini`. Clicking a tab icon switches to that tab without a synthesised click.
  - A tab is not a window, so no amount of window enumeration finds one; the only supported way to ask is UI Automation, which is a call into the other application's own UI thread. That is slow and can block if the thread is busy, which is why this is off by default, asks only windows whose class can have tabs, re-reads them every five seconds rather than every second, and falls back silently to one icon per window on any failure.
  - Design, costs, and what was turned down: `docs/RFC-2026-07-tabs-as-task-icons.md`.

## [v26.07.31d] - 2026-07-31

### Fixed
- **Task icons that only appeared when the pointer crossed them.** A window whose icon would not load left the cell empty, and an empty cell reads as no window at all — until you hovered and the highlight box appeared out of nowhere. The window is there either way, so the title's first character now stands in for a missing icon.
- **An icon that failed to load once stayed missing for the life of the window.** A window asked for its icon while it is still starting up can answer nothing, and that nothing was cached. Each refresh now retries the ones that have no icon, which costs one call for the few that failed.
- **hgfloater's monitor preview windows** were still missing from the task icons. v26.07.31 added the note editors, the note list, the clipboard history and the command box; the preview windows belong there too.

### Documentation
- The README now says plainly **how to add a shortcut icon** — right-click `O`, *Open Shortcuts Folder*, drop a `.lnk` or `.url` in — in a highlighted box rather than one line in a list, and explains that the order comes from the file name, so a `0001` prefix controls it. Pad the numbers: the sort is plain alphabetical, so `10` comes before `2` otherwise.

## [v26.07.31c] - 2026-07-31

### Added
- `show monitor` (`s m`) lists the displays with their number, size, position, and whether their preview window is up. `show monitor 1` (`s m 1`) turns display 1's preview on, and running it again closes it — the same switch the display's own submenu in the `O` menu holds, called once rather than reimplemented.
- The number it prints is the one `move` already takes: Windows' own display number where the system gives us one, so it matches the Settings page, falling back to position in the list only when nothing is labelled.

## [v26.07.31b] - 2026-07-31

### Changed
- **Scrolling and the history are modes now, not modifier combinations.** `Ctrl + S` enters scroll mode, where `Up`/`Down` move the transcript a line and `Left`/`Right` a page; `Ctrl + H` enters history mode, where `Up`/`Down` walk through what you have run. The `Shift + arrow` and `Shift + PgUp/PgDn` bindings added a few hours earlier are gone.
  - The reason is what those bindings cost: `Shift` with the arrows is text selection in every other text box on the machine, and taking it away from the one box you type commands into was the wrong trade. A mode is something you enter on purpose, so it costs nothing you had.
  - The title bar says which mode is on, because a window that has silently changed what its keys do is a window that looks broken. Any key the mode has no use for drops the mode and is then handled normally, so typing never lands in a hole.
- **`Esc` leaves the mode, or closes the box and hands the keyboard back to the taskbox** — expanding it from the floater first if it was not already open. Closing the box and leaving focus nowhere in particular was never useful.
- **`Ctrl + Enter` is gone.** `Enter` runs the line and `Shift + Enter` is the newline; there is no longer a second way to do the first thing.

## [v26.07.31] - 2026-07-31

### Added
- The command box opens on **its own key list** instead of a blank prompt, and `help key` (`h k`) prints it again. A box with a cursor and nothing else is a box you have to be told about somewhere other than the box.
- **Command history**: `Shift + Left` and `Shift + Right` walk back and forward through what you have run. It keeps 64 lines by default, settable with `write value history-max <n>` or `history_max` in `config.ini`, and holds nothing on disk. A line is recorded before it runs, so a command that failed is still one you can bring back and correct.
- `Shift + Up/Down` and `Shift + PgUp/PgDn` **scroll the transcript**. It is read-only and never takes focus, so until now the only way to look back at what a command printed was the mouse.
- New commands: `note new` (`n n`) writes a note; `clipboard` (`b`) prints the clipboard history numbered and `b 3` makes entry 3 current; `config` (`c`) opens `config.ini` in Notepad; `show value` (`s v`) lists every settable value with what it is now, and `write value <number|name> <value>` (`w v`) sets one; `show sensors 2` prints one sensor rather than all of them.
- `config.ini` gains `[commandbox] history_max`, written on first run together with a comment block explaining it and `[clipboard] max` — the two keys that have no control anywhere in the interface.

### Changed
- **The keyboard goes to the input box** when the command box opens and whenever it is activated. Opening a window whose entire purpose is being typed into, and then having the first keystroke land nowhere, was the complaint this answers.
- **`Enter` runs the line; `Shift + Enter` is the newline.** `Ctrl + Enter` still works — it was the only way to run a line for a long time. The consequence is that `Shift` with the arrows now belongs to the transcript and the history rather than to text selection; `Ctrl + Shift + Left/Right` still selects a word at a time.
- `list` is now **`show`** (`s`) and `search` is now **`find`** (`f`). Because sensors took `s` as a kind, `show shortcut` is `s c`.
- **Task icons are one per window, and that now includes hgfloater's own windows** — every open note editor, the note list, the clipboard history, the command box. They were being filtered out for being tool windows, which is right for Alt-Tab and wrong for a list whose job is reaching windows. The floater, taskbox and toolbar stay out: they are the thing you are looking at.

## [v26.07.30] - 2026-07-30

### Changed
- The wheel over `B` moves brightness in 5% steps again, the same as opacity and volume. It was put on 1% in v26.07.29d because driving each monitor on its own real scale made fine steps possible — which turned out not to be the same thing as wanting them. A hundred notches to cross the range makes the wheel a chore, and the `O` menu already covers wanting a specific level rather than a nudge.
- `VER.txt` is current again; it had been left at v26.07.29l through three releases.

## [v26.07.29o] - 2026-07-29

### Added
- **Clipboard history**, behind a new `L` toolbar button that toggles its window. It lists what you have copied, newest first; clicking a clip makes it the current one and pushes everything above it down a place, so the list ends up in the order it would have been in had you copied that text again. A search box filters the list, and a number box beside it sets how many clips to keep — 16 by default, 1 to 64.
- Lowering that number takes effect immediately: 20 clips kept and a maximum of 16 drops the oldest 4 as you set it, because "at most 16" would otherwise be false the moment you asked for it. Raising it fills forward, since the dropped clips are gone and inventing history would be worse than not having it.

### Notes
- Capture runs whether or not the window is open — a history that only recorded while you were looking at it would not be one — so a message-only window holds a clipboard listener for the life of the process. Text only.
- **Nothing is written to disk**, only the maximum. This is the one place the feature deliberately does less than the clipboard managers it was measured against: a clipboard history on disk is a file of every password and recovery code that passed through the clipboard, and a floating clock widget should not be the program that owns that file. Restarting hgfloater empties the history, and the README says so.

## [v26.07.29n] - 2026-07-29

### Added
- The command box can work the notes without the list window: `list note` (`l n`) numbers every note, `note 3` opens one, `note 3 archive` / `restore` / `delete` (`n 3 a` / `r` / `d`) act on it, and `search note <text>` (`s n`) finds notes by their **title or their body**. Numbers come from `list note` and are identifier order, not the window's — the window sorts however you left it, so a number taken from it would mean a different note after the next sort.

### Changed
- **An archived note is now read-only.** Archiving files a note away, and something filed away has stopped being written; its editor opens read-only and the caption says so, rather than being a window that ignores typing without explanation. Copy and Select All stay — reading an archived note is the point of keeping it — and Restore makes it writable again. Deleting is unchanged: any note can be deleted, archived or not.
- Changing the note text size now re-fits what is on screen to the window you already have. The list re-measures its rows and scrolls the selected one back into view; every open editor re-wraps and keeps the caret on screen. Before, the new font was applied and the layout was left where the old one had put it.

## [v26.07.29m] - 2026-07-29

### Added
- `list sensors` (`l t`) in the command box prints every thermal zone the machine exposes, with its source, its name, and its reading, followed by the two numbers actually on the `TMP` and `GPU` bars. The zone shown is picked by a heuristic over data the firmware controls, and a heuristic nobody can inspect is one nobody can report a bug against.

### Changed
- The CPU temperature now comes from **every** zone the machine exposes rather than the first one WMI happened to hand over. Firmware routinely declares several and puts one it never updates in front of the live one, so the old reading was as likely to be filler as a sensor on any machine with more than one.
- Zones are now read from **two** surfaces, the WMI class and the `Thermal Zone Information` performance counters, because the two do not agree about being available: some machines answer one and not the other. PDH is part of Windows, so this costs no dependency — only the care to open the counter through `PdhAddEnglishCounterW`, since the object's name is localised and the English path would not resolve on a Korean or any other non-English Windows.
- Which zone gets shown is now decided rather than assumed: a zone that has been watched change beats one named after the CPU, which beats one that is neither, and only an outright tie is settled by the hotter reading. Taking the hottest outright — the obvious rule — would have picked the filler zone on every machine whose filler is a high constant.

## [v26.07.29l] - 2026-07-29

### Changed
- The README opens with what the program actually costs to have: a single executable of about 360 KB, no installer and no runtime. That was the point of writing it in C against Win32 with nothing else linked in, and it was nowhere on the page.
- The screenshot is current again, taken on v26.07.29j. The one it replaces was from v26.04.30 and predates the status bars, the note system, the command language, and every per-display menu.

## [v26.07.29k] - 2026-07-29

### Fixed
- The manuals had fallen behind the code in three places, all found by checking them against the tree rather than against memory. The project layout never listed `hg_wmi.c`, which has held the internal panel's backlight since v26.07.29e and the thermal zone since v26.07.29f; `hg_sysinfo.c` was still described as CPU, memory, and battery after it grew a GPU temperature reader; and the mouse reference had no entry for moving the floater at all, before or after v26.07.29j gave that gesture something to do.

## [v26.07.29j] - 2026-07-29

### Added
- **Dragging the floater moves it**, while `F` adjust mode is on. That mode is the one state where the floater stays put under the pointer instead of opening the taskbox, so it is the only state where dragging it anywhere was possible - and it did nothing until now. A drag neither toggles the taskbox nor drops out of adjust mode when the button comes up, so the floater can be positioned and tuned in one visit; a click that merely wobbles is still a click.

### Fixed
- The README claimed a left drag moved the floater. It did not, in any mode. It does now, in the mode where it can, and both manuals say which.

## [v26.07.29i] - 2026-07-29

### Fixed
- The clock no longer sits on top of the computer name. Running the bars up behind the host name in v26.07.29h moved the top of the bar area to the top of the floater, and the clock was taking its own starting point from that same number - so it rose onto the host line. The bars and the text stack now have separate origins: the bars still span the whole inner height, and the clock and date sit below the host name where they belong.

## [v26.07.29h] - 2026-07-29

### Changed
- **Every status row now prints its reading**, in a strip of its own down the right side of the floater: `NN%` for CPU, memory, and battery, `NN°` for the two temperatures. The strip is sized for the widest reading any row can produce, so the column does not shift as the numbers change, and the readings no longer sit on top of their own bars.
- **The bars run behind the host name too.** Keeping that line clear of them cost a row's worth of height for nothing - the bars are background, and text reads perfectly well over them - so the panel now uses the floater's whole inner height and the rows are correspondingly less cramped.

## [v26.07.29g] - 2026-07-29

### Added
- **A GPU temperature bar**, after the CPU one and on the same 20 to 100 degree scale, with its reading printed on it and its colour configurable as `stat_gpu`. It gets a better answer than the CPU row does: the adapter's own sensor, read through the same WDDM interface Task Manager uses, with no vendor SDK, no driver, and no elevation - so NVIDIA, AMD, and Intel all answer the same call. Drivers that report nothing leave the row absent, the way a missing thermal zone does.

## [v26.07.29f] - 2026-07-29

### Added
- **A temperature bar on the floater**, directly after CPU. It is drawn on a fixed 20 to 100 degree Celsius scale rather than as a percentage, with the reading printed on its own bar, and its colour is configurable as `stat_temp`.
- `docs/RFC-2026-07-temperature.md` records where the number comes from and, more to the point, where it does not. The accurate way to read a CPU die needs a kernel driver and administrator rights - the reference project named in the request uses a third-party .NET library for exactly that, and its own README reports crashes and hangs from the feature. hgfloater declines all three and reads the **ACPI thermal zone** the firmware exposes instead: a sensor on the board near the CPU, not the die. It tracks the processor without being it, and it lags under a sudden load.
- Machines whose firmware exposes no thermal zone simply have no row, the way desktops have no battery row. The zone is read once every fifth refresh rather than every second, and once a run of reads has failed the asking stops for the session.

## [v26.07.29e] - 2026-07-29

### Added
- **A laptop's internal panel now gets its real backlight changed**, not just its picture dimmed. A built-in display is not a DDC/CI device, so every path hgfloater had missed it and it fell through to the gamma ramp. It is now recognised by its connector and driven through Windows' own brightness service - the same one the system slider uses - which moves the lamp. Machines with two integrated panels are a limitation: the first one answers.

## [v26.07.29d] - 2026-07-29

### Changed
- Brightness now tries three paths per display and remembers which one answered. The **low-level DDC/CI** path comes first: it reads the monitor's capabilities string, checks that the luminance control is advertised, and drives it on the scale the monitor actually reports rather than assuming 0 to 100. The **high-level** call it used to use exclusively is now the second rung, and the gamma ramp the last. This reaches monitors that answer DDC/CI but not the high-level API, and it reports their level correctly.
- The wheel over `B` moves brightness in **1% steps**, where opacity and volume keep their coarse 5%. A monitor's real scale is known now, so the finer step means something.
- A display that has been asked and answered nothing shows **Brightness (unavailable)** in its submenu, the way an unreachable Scale already did, instead of accepting a click that does nothing.
- Writing a brightness no longer costs an extra read: the scale comes from the one-time probe. A monitor that advertises the control and then refuses to set it falls through to the next path rather than being taken at its word.

### Added
- `docs/RFC-2026-07-brightness-control.md` records how brightness control works on Windows - the high-level, low-level, and WMI paths, what each reaches, and why gamma is not brightness - and what remains: the WMI path for internal laptop panels, which is the one case still landing on the gamma fallback.

## [v26.07.29c] - 2026-07-29

### Fixed
- Dimming a display that does not answer DDC/CI no longer loses track of itself. The refresh timer was stamping "unknown" over the brightness the app had just set through the gamma fallback, so the menu ticked the step you chose and then, seconds later, ticked nothing.
- A display whose original gamma ramp was never captured is no longer dimmed at all. Past the backup table's capacity it was being dimmed with no way back, and stayed dimmed after hgfloater exited; the table is also four times larger.
- A note's modification time is the time it was saved. It was read from the file handle before Windows had committed the stamps, so every note showed - and sorted by - its previous save until a restart.
- A command line of 1024 characters or more says it is too long instead of vanishing without a word.
- The command box leaves the keyboard where a command put it. `note` and `go` hand the foreground to another window on purpose, and the box was pulling focus back over it immediately.
- `move` with a display number nothing answers to is an error rather than a guess. Windows leaves gaps in display numbering after a monitor is unplugged, and the lookup was falling back to array position, so `move 1 0 0 2` could move a window to the display labelled 3.
- The note list keeps a heading over an empty half, so the sort order of a half everything has been archived out of can still be reached by right-clicking it.

### Changed
- The note table no longer reserves about 16 MiB of memory that nothing used: it held each note's full path in a fixed 32768-character field when the directory is known and the name is short. Static memory drops from 29.6 MiB to 13.6 MiB, whether or not you ever open a note.

## [v26.07.29b] - 2026-07-29

### Added
- **Start with Windows** in the options menu, checked when hgfloater launches at sign-in. It writes one value under the per-user `Run` key holding the quoted path of the running executable - no installer, no elevation, nothing else touched - and switching it on again after moving the executable re-registers where the file actually is. It is the only registry value the app writes, and `Reset Settings` leaves it alone.

## [v26.07.29] - 2026-07-29

### Added

- **Notes.** A new `N` toolbar button (and the `note` command) opens a note list.
  - Each note is a plain UTF-8 `.txt` file under `note\` whose **first line is the title**, so Notepad or any other editor reads and writes them without help. Edits are held in memory and written a couple of seconds after the typing settles, for the changed notes only; closing an editor or quitting flushes the rest.
  - The list starts with a **`+Add Note`** row and opens with it selected, so `N` then `Enter` writes a new note without a key to remember. It answers a single click, unlike the note rows, and it is there even when there are no notes yet.
  - Notes being written stay above **archived** ones, under headings that appear once something has been archived. Right-clicking a note - or a heading - opens it, archives or restores it, deletes it, and sorts that half of the list by creation time, modification time, or title, ascending or descending. The two halves sort independently and each remembers its own order.
  - Every note can be deleted, archived or not: archiving files a note away, it does not lock it. Deleted notes go to the **Recycle Bin**.
  - Each note opens in its own editor window, several at a time. Right-clicking one offers **Undo**, **Redo**, **Cut**, **Copy**, **Paste**, **Delete** the selection, **Select All**, **Archive** or **Restore**, and **Delete Note and Close**, with anything that would do nothing greyed out. Undo runs a hundred levels deep and `Ctrl + Y` redoes.
  - **`Ctrl + Wheel`** sets one text size for the list and every editor at once. It is stored unscaled, so a size chosen on a 200% display reads the same on a 100% one and a note dragged between monitors is redrawn at that display's scale.
  - Everything a text file cannot carry lives in `note\note.ini`: each note's section and creation time, both sort orders, the list window's geometry, the shared text size, and **each note's own editor position and size**, so a note reopens on the monitor and at the size it was left.

- **A command language in the command box**, which until now echoed back whatever was typed. `help`, `list` (windows, resize presets, or shortcuts), `go`, `resize`, `move`, `search windows`, and `note`, each with a one-letter short form. **`help <command>`** explains one of them in full, with its arguments and worked examples.
  - Every command that names a window uses the number `list` prints beside it, and only the commands that print numbers re-read the window list, so a number stays valid between reading it and typing it.
  - `move`'s X and Y are measured from a display's own top-left corner rather than the virtual desktop's, so the same pair means the same place on every screen; an optional display number moves a window to another monitor.
  - The input field is multi-line, so pasting several lines runs them in order, each echoed behind a `>` prompt.

- **Per-display control in the options menu.** Every connected monitor gets its own submenu holding everything the app can do to that one display: its **Preview Window** thumbnail, **Scale** (100, 125, 150, 175, 200, 225), and **Brightness** in quarter steps. The current scale and the nearest brightness step are checked, and scaling percentages the monitor cannot reach stay greyed out.
- Brightness is now per display. Monitors that answer DDC/CI are driven directly; the rest fall back to a gamma curve applied to that display's own device context, so dimming one screen leaves its neighbours alone. Every backed-up ramp is handed back at exit, not just the desktop's.

### Changed

- Displays are named the way their owner would name them - display number, the monitor name the driver reads out of the EDID, and the connector it hangs off, as in `2. DELL U2720Q (DP)` - in the options menu, the thumbnail window caption, and its tooltip. Reported connectors are RGB, DVI, HDMI, DP, eDP, USB-C, Internal, and the rest of the DisplayConfig set. The port is the socket on the graphics adapter rather than the cable, so a USB-C screen running DisplayPort alt mode reports as `DP`; `USB-C` appears when DisplayPort is tunnelled over USB4 or Thunderbolt.
- `Arrange Monitors` is gone as a top-level entry; the thumbnail toggle it held now sits in each display's own submenu as **Preview Window**.
- The resize presets the task context menu offers come from one shared table, which is also what `list resize` prints and `resize` indexes, so the menu and the command box cannot drift apart.
- A DDC/CI brightness reading is normalised against the range the monitor reports instead of being read as a percentage outright, so displays that do not use 0..100 no longer show a misleading level.

### Fixed

- Long lines in the command box wrap instead of running off the right edge. A multiline edit control treats `ES_AUTOHSCROLL` as the switch that turns word wrap off, and both fields were created with it.
- A plain wheel scrolls the command box. Every wheel message was being forwarded to the window, which acts only on `Ctrl` and `Alt`, so the transcript could not be scrolled at all.

## [v26.07.22] - 2026-07-22

### Added
- The options menu (`O` button, or right-click the status line) gains a submenu named after the display the taskbox is on, listing Windows scaling percentages 100, 125, 150, 175, 200, and 225. The current scale is checked, values the monitor cannot reach are greyed out, and choosing one changes that monitor's scale system-wide through the same DisplayConfig interface the Settings app uses.

## [v26.07.20b] - 2026-07-20

### Changed
- The sources moved into `src/`, leaving the repository root for the build files and documentation. `build.bat`, the cross-build script, the tests, and the About-text generator all follow the new layout.
- `README.md` is now an English-only reference manual, rewritten as numbered chapters with a table of contents; the Korean translation lives beside it in `README.ko.md` and the two link to each other.

### Added
- A `Makefile` builds the project on any host with a MinGW-w64 toolchain: `make`, `make debug`, `make test`, `make clean`, with `CROSS=`, `OUT=`, and `VERSION_SUFFIX=` variables. `build.bat` still offers the same builds on Windows, and `scripts/build-mingw.sh` is now a thin wrapper over the Makefile so the source list cannot drift between them.

## [v26.07.20] - 2026-07-20

### Changed
- The Move-handle click keeps the heading it used last time instead of restarting its search at north every click, so repeated clicks walk the window across the screen rather than bouncing between two spots. It turns counter-clockwise - north, west, south, east - only when the current heading has no room left.

## [v26.07.19e] - 2026-07-19

### Changed
- Right-clicking the taskbox status line now opens the main options menu, the same one the `O` toolbar button shows, instead of a copy-only context menu.
- Settings that change in bursts - opacity, font and icon size, and window position - are written to `config.ini` once the change settles (or at exit) instead of on every wheel notch or arrow-key repeat. Each notch used to rewrite the whole INI file, twice for opacity.

## [v26.07.19d] - 2026-07-19

### Added
- A `P` (Pin) toolbar button holds the taskbox open: while pinned, moving the mouse away no longer collapses it back to the floater, and the button carries an accent border. Explicit closes still work.

### Changed
- The taskbox status line shows one message at a time instead of a scrolling log, and falls back to the current time (`2026. 11. 23.(Tue) 13:24`) once a message has sat there for ten seconds, refreshing as the minute changes.

## [v26.07.19c] - 2026-07-19

### Fixed
- Collapsing the taskbox no longer drops the floater back at its pre-expand spot after the taskbox has been moved. The floater now travels the same distance the taskbox did, whether it was dragged by the Move handle, nudged with `Alt` + arrows, resized, or sent aside by a Move-handle click, and the result is kept inside the work area.

## [v26.07.19b] - 2026-07-19

### Changed
- The Move handle's click now takes the smallest step that clears the spot it was on, landing flush against it instead of travelling all the way to the edge of the work area.

## [v26.07.19] - 2026-07-19

### Added
- Clicking the toolbar's Move handle (rather than dragging it) moves the floater and taskbox aside on their own. The search tries north, then west, then south, then east, and takes the first work-area edge where the pair no longer overlaps the area it occupies right now; if no direction is clear, the pair stays where it is.

## [v26.07.14] - 2026-07-14

### Changed
- The windows report themselves as `HGFloater` (and `HGFloater Taskbox`), so the taskbar and Task Manager no longer show a bare `floater`.
- Summoning the taskbox from a floater parked at a screen edge no longer clips it: the taskbox is centered on the floater but pushed fully inside that monitor's work area.
- Collapsing the taskbox returns the floater to exactly where it was before the expand, instead of re-centering it on the taskbox.

## [v26.07.13g] - 2026-07-13

### Changed
- The floater's clock gets a small breathing margin above and below (a tenth of its own height), so the ink-flush lines no longer read as cramped.

## [v26.07.13f] - 2026-07-13

### Fixed
- The floater's text lines now sit truly flush and are vertically centered on what you actually see: each line is measured by its real ink bounds instead of the font's reported cell, which carried invisible slack above the caps and below the baseline and made the glyphs look bottom-heavy.

## [v26.07.13e] - 2026-07-13

### Changed
- The floater is more compact: the clock is the reference size and the other text scales off it (labels 30%, computer name 40%, date 70%), and the name, clock, and date stack flush with no vertical gaps between them. The status bars split the area below the name into equal rows.

## [v26.07.13d] - 2026-07-13

### Changed
- The floater's layout is now fully proportional: every padding and gap derives from the floater font size (scaled for DPI) or from measured text, instead of fixed pixel values, so the design keeps its proportions at any font size and on any monitor. The window height is exactly padding + host name + gap + clock + date + padding, the clock and date center in the column right of the CPU/MEM/BAT labels, and the labels center vertically against their bars.

## [v26.07.13c] - 2026-07-13

### Changed
- The floater's computer-name line is smaller (80 percent of the previous size) and sits nearly flush with the content below it, and the clock and date now anchor directly under the name instead of centering in the remaining space.

## [v26.07.13b] - 2026-07-13

### Changed
- The floater's computer-name line is larger (1.5x), sits below a small top padding matching the side edges, and keeps only a tight gap above the clock.

## [v26.07.13] - 2026-07-13

### Added
- The floater shows the computer name in a thin line across its top, slightly larger than the status bar labels and scaling with the floater font.

## [v26.07.11d] - 2026-07-11

### Added
- Every accent color is now centrally managed and configurable from the new `[colors]` section of `config.ini` (RRGGBB hex): the custom dark palette, the focus highlight, the floater status bar colors, and the `A`/`B`/`V` value-button gradients. Defaults are written back on first run and Reset Settings restores them.

## [v26.07.11c] - 2026-07-11

### Fixed
- The floater and taskbox occasionally lingered with washed-out, grayish content (stale layered-window bitmaps after display sleep or compositor hiccups) until a mouse interaction repainted them; visible widgets now force a full repaint every few seconds and immediately after display changes, so the state heals itself.

## [v26.07.11b] - 2026-07-11

### Fixed
- Arrow keys navigate between taskbox icons again after a hover-summon: the taskbox could become active without keyboard focus, so plain keys arrived as system keys and were misread as Alt+Arrow window moves. Hover-summon now assigns focus and the Alt detection no longer trusts the system-key flag.
- The yellow focus highlight now follows the mouse as well: hovering an icon moves the focus to it.

## [v26.07.11] - 2026-07-11

### Changed
- The keyboard/click focus on taskbox items is now marked by a yellow background fill (task icons and toolbar buttons alike, including the value buttons) instead of an accent ring, which proved too subtle at small icon sizes; plain mouse hover keeps its lighter highlight.

## [v26.07.10e] - 2026-07-10

### Changed
- The toolbar focus ring is now unmistakable: a bold accent ring at least three pixels thick that starts just outside the focused item (task icons and toolbar buttons alike), absorbed by the inter-item gap so the layout is unchanged. Note that the ring marks the keyboard/click focus; plain mouse hover keeps its lighter highlight.

## [v26.07.10d] - 2026-07-10

### Fixed
- The floater status bars no longer draw a filled track behind them; the floater background shows through, so the blue MEM bar stays visible in dark mode where the system highlight color used to fill the track with the same blue.

### Changed
- The keyboard/mouse focus outline around toolbar icons is drawn a few pixels thick, making the focused item clearly visible without changing the icon layout.

## [v26.07.10c] - 2026-07-10

### Changed
- The taskbox `X` button now exits hgfloater entirely instead of hiding the taskbox (hover-away, a floater click, and `Esc` still hide it), and the `P` (Popup Menu) button is renamed to `O` (Options); tooltips and the edit-console hint line follow.

## [v26.07.10b] - 2026-07-10

### Added
- The floater gains status bars: CPU, MEM, and BAT horizontal bar graphs (red/blue/green, full width = 100%) run underneath the clock and date with tiny labels on the left edge, refreshed once per second inside the clock height so the floater stays small; the battery row hides on systems without one, a `+` on the label marks charging, and `[floater] show_stats=0` turns them off.

### Changed
- The Command Box, About window, and monitor previews now scale to the DPI of the monitor they are on instead of inheriting the floater's scale, completing mixed-DPI support.

## [v26.07.10] - 2026-07-10

### Added
- Moving between monitors with different display scaling (or changing scale) now resizes and re-renders the floater and taskbox for the new DPI, and the startup size matches the monitor the floater appears on.

### Fixed
- Hover-summoning the taskbox reliably takes keyboard focus again, so pressing `C` (Command Box), `Esc`, and the navigation keys works right after a focus-preserving auto-collapse; Windows refused the plain foreground request once another application held focus.
- Off-screen windows are clamped into the nearest monitor work area instead of jumping to the primary monitor origin.
- Removing or rearranging a display immediately moves stranded widgets back into view instead of waiting for the next hotkey press.
- Monitor preview positions are remembered per display device, so reordered monitor enumeration no longer applies a saved position to the wrong display.
- Theme changes no longer leave the floater, taskbox, and command box window classes holding a deleted background brush.
- Task context menu commands (Focus, Close, Move, Resize) act on the window captured when the menu opened, so a background list refresh can no longer redirect them to a different window; the list also pauses refreshing while a menu is open.
- The taskbox auto-collapse no longer steals foreground focus from the application being used.
- Hiding the taskbox stops the hover-check timer on every path (Esc, X button, floater click), not only on timer-driven collapse.
- Releasing a Ctrl+drag floater font-resize gesture no longer toggles the taskbox.
- The About window text now follows Ctrl+Plus/Minus font size changes instead of keeping a destroyed font handle.
- Command lines forwarded from a second instance are copied with an explicit size bound.
- Persisted alpha values below 128 are honored on startup, matching the 30 percent minimum the runtime controls already allow.
- Right and bottom taskbox resize bands are now the same width as left and top.
- Hiding the taskbox rescans the shortcuts folder only when the folder actually changed.

### Changed
- The executable now carries standard Windows version information (file properties show the vYY.MM.DD build version), and the build derives its date without the removed wmic tool.
- build.bat can run unattended (`build.bat debug|release|test`), and the About-text generator is a standalone script usable on non-Windows hosts too.
- Removed the unreachable monitor preview deferred-drop timer branch and sized global array declarations with the shared macros.
- Toolbar painting and the volume/mute/brightness readouts no longer create COM audio devices, perform monitor DDC/CI reads, or churn GDI brushes on every frame; the values come from caches kept current by timers, setters, and menu opens.
- Dragging the command box no longer writes its position to the configuration file on every pixel; it saves once when the drag ends.
- Reduced static memory use by several megabytes and cut the deepest icon-resolution stack usage.
- Internal cleanup with no behavior change: edit-height measurement, column-snap width, control coloring, alpha stepping, and read-only edit IME handling each have a single shared implementation, and the toolbar window class registers alongside the other window classes.
- Internal restructuring with no behavior change: the taskbox source now consists of four units (window proc/layout, toolbar controller, menus, window-list refresh) instead of one 2,400-line file, and the utility grab-bag is split into audio, display, and shell-icon modules.

### 2026-06-24 - Floater Adjust Mode & Taskbox UX

#### Added
- Added a toolbar **F (Floater)** button: clicking it collapses the dashboard to
  the floater in an adjust mode where **Ctrl+Wheel resizes** and **Alt+Wheel
  changes opacity**, and a click on the floater returns to the taskbox. Hover-to-
  expand is suppressed while adjusting so the wheel tuning isn't interrupted.

#### Changed
- Renamed the **P** toolbar button's label/tooltip from "Menu" to **"Popup Menu"**.
- The taskbox now collapses to the floater after the cursor has stayed outside for
  **0.5s** (was 1s), re-checked each tick so a brief exit doesn't collapse it.

### 2026-06-24 - Menu Construction Refactor

#### Added
- Added taskbox main popup menu builder helpers for the main menu, audio submenu, monitor submenu, and selected command forwarding.
- Added taskbox task and shortcut context menu builder/dispatch helpers, and reused the audio submenu builder for the volume context menu.
- Added floater command dispatch helpers for monitor, audio device, and fixed-volume menu commands.
- Added taskbox command dispatch helpers for forwarded floater commands and taskbox font commands.
- Added a single toolbar controller state context to replace scattered callback-local transient state.
- Added a toolbar-local taskbox reorder drag context to replace taskbox-only process-wide drag globals.
- Added taskbox-local focus state and a reset helper so other widgets no longer write toolbar focus globals directly.
- Added a taskbox layout state context for interactive resize start bounds and centralized toolbar icon-size calculation.
- Added a taskbox owned-popup-menu tracker so popup display, menu-active state, and `DestroyMenu()` cleanup share one path.
- Added a shared default audio endpoint-volume acquisition helper to centralize COM release ownership for volume and mute operations.
- Added a shared `WindowItem` icon release helper so owned window icons are destroyed and reset consistently.
- Added a Command Box line-height helper to centralize font metric DC acquisition and release.
- Added shared global font and brush release helpers to preserve stock fonts and reset owned GDI handles consistently.
- Added a shared offscreen paint buffer helper to centralize memory DC, bitmap selection, restore, and cleanup paths.
- Added a shortcut icon release helper so shortcut-owned icons are destroyed and reset consistently.
- Added shared COM and BSTR release helpers for audio, UWP icon, shortcut, and Explorer shell paths.
- Added shared heap and COM task-memory release macros for UWP icon and audio device string cleanup paths.
- Added named timer IDs for floater clock, taskbox refresh, and monitor refresh/deferred-drop timers.
- Added a repeatable verification checklist covering warning-clean builds, smoke tests, and focused manual runtime checks.
- Added a toolbar contract smoke test for built-in toolbar count and index invariants.

#### Fixed
- Update the stored toolbar focus index on mouse press so keyboard-triggered context menus follow the clicked item.
- Guard monitor preview painting against failed memory DC, bitmap, screen DC, and pen allocation paths.
- Guard Command Box font metric calculation against failed `GetDC()` and remove the temporary dummy window.
- Fix the `build.bat` test runner so `main()`-based console smoke tests link correctly, and silence existing cast smoke-test warnings.

### 2026-06-23 - Stabilization and Refactor Planning

#### Added
- Added `docs/RFC-2026-06-staged-refactor.md` with a staged refactor plan covering configuration persistence, toolbar modeling, menu helpers, widget state boundaries, resource lifetime audit, and verification expansion.
- Added named geometry persistence helpers for floater, taskbox, and command box windows as the first Phase 1 configuration-boundary refactor step.
- Added named command box font config helpers and a shared font-name save helper for Phase 1 persistence cleanup.
- Added centralized global hotkey register/unregister helpers.
- Added a built-in toolbar descriptor table for labels and static focus/tooltip text as the first Phase 2 toolbar-model extraction step.
- Added toolbar descriptor value roles for alpha, brightness, and volume dynamic focus/tooltip text and wheel dispatch.
- Added toolbar descriptor click and drag roles for built-in toolbar button dispatch.

#### Fixed
- Persist floater/taskbox alpha changes made through `Alt + Mouse Wheel` and `Alt +/-` paths by routing both runtime alpha update helpers through `save_alpha_config()`.
- Route command box alpha persistence through a named helper and skip redundant updates when the value is already clamped.
- Clamp command box font size loaded from `config.ini` to the supported 8-72 range and persist normalized defaults.
- Normalize invalid hotkey modifier bits and invalid virtual-key values loaded from `config.ini` before registration.
- Normalize floater/taskbox font settings before saving and skip redundant taskbox font/icon refreshes at clamp boundaries.
- Avoid redundant layered-window updates when alpha is already clamped at its minimum or maximum.

### 2026-06-22 - Floater to Taskbox Hover UX & UI Adjustments

#### Changed
- Converted the interaction model from clicking the Floater to simply **hovering** over it to spawn the Taskbox in place instantly (`WM_MOUSEMOVE` triggering `ShowWindow(SW_HIDE)` for floater and `SW_SHOW` for taskbox). Left-click on the floater still toggles the taskbox.
- Added an automatic `HG_TIMER_HOVER_CHECK` timer in Taskbox to automatically close and return to the Floater UI when the mouse leaves the bounds of the Taskbox, with a ~1 second (10 tick) grace delay that aborts the hide if the cursor briefly leaves and re-enters.
- **Removed the Floater's own right-click context menu** to avoid redundancy. The main context menu (Open Shortcuts Folder, Edit Configuration, About, Reset Settings, Select Audio Device, Arrange Monitors, Lock Screen, Exit) now lives solely on the Taskbox toolbar button.
- **Renamed the Taskbox `M` (Menu) toolbar button to `P` (PopupMenu)**; left-clicking it opens the main context menu.
- Unified the Time and Date font sizes in the Floater by aligning their multiplier ratio to `1:1` (while preserving variables separately for future independent scaling).

#### Build / Internal
- Converted `build.bat` to compile the modular split sources (`hg_globals.c`, `hg_utils.c`, `hg_config.c`, `widgets/*.c`) together with `hgfloater.c`, replacing the previous single-translation-unit build. Release builds now use `-O3 -flto=auto -DNDEBUG`.
- Replaced `dxva2.dll` function-pointer casts with a `union` loader to silence `-Wcast-function-type`, and routed toolbar tooltip indexing through named `HG_TOOL_ICON_*` constants.

## [v26.05.31] - 2026-05-31

### Added
- **Hybrid Brightness Control**: Integrated a robust fallback mechanism for laptop displays and monitors lacking DDC/CI hardware support. Implemented software-based brightness adjustment using GDI `SetDeviceGammaRamp` while preserving the user's original desktop color profile via a persistent backup-and-restore process at application exit.
- **Command Box (C Button)**: Added an independent customizable CLI utility window featuring unique dimensions, positions, fonts, and alpha channels stored separately in `config.ini`. Enabled live layout adjustment via keyboard (`Ctrl/Alt + Arrows`) and font scaling (`Ctrl/Alt + Wheel`).
- **Focus Hotkey (`Ctrl + Space`)**: Implemented a global message routing system to instantly focus the Command Box input field and move the caret dynamically to the rightmost index.
- **Rich Taskbox Toolbar Icons**:
  - `B` Icon: Added mouse-wheel 5% step brightness adjustment.
  - `A` Icon: Added mouse-wheel transparency control.
  - `V` Icon: Left-click toggles system-wide volume Mute/Unmute state with context tooltip.
  - Removed redundant `F` icon and adjusted icon indexes cleanly.
- **Dynamic Context Audio Menu**: Integrated active local audio device scanning natively into the Floater Context sub-menus alongside standard `MF_CHECKED` checkmarks for the system Mute state.

### Changed
- **Architectural Modularity Refactoring (Phase 1 ~ Phase 3)**: Extracted all widgets and subsystem logic from `hgfloater.c` (formerly ~6,200 lines) into highly modular source files:
  - Global Configuration: `hg_config.c` / `hg_config.h`
  - Global State: `hg_globals.c` / `hg_globals.h`
  - Core Utilities: `hg_utils.c` / `hg_utils.h`
  - Custom Widgets (`widgets/`): `hg_floater.c`, `hg_taskbox.c`, `hg_controlbox.c`, `hg_monitor.c`, `hg_commandbox.c`, `hg_about.c`
- **Legacy HJKL Bindings Removal**: Completely removed Vi-style HJKL movement bindings from all window navigations in favor of standard `WASD` / `Arrow Keys` as per specifications.

### Fixed
- **Monitor Loading Crash**: Resolved GDI object leaks causing layout loading failures of secondary monitor thumbnails by centralizing resource lifetimes inside the `wWinMain` teardown scope.
- **VK_F5 Reset Invariant**: Forced acceleration table routing to correctly deliver layout reset notifications to the Controlbox even when sub-trackbars retain keyboard focus.
- **NCRBUTTONUP Caption Destruction**: Added `WM_NCRBUTTONUP` caption hit-test listeners allowing title bar right-clicks to instantly destroy Controlbox dialogs.

## [v26.05.21] - 2026-05-21

### Added
- Implemented the **Controlbox** interactive system utility window class (`hgcontrolbox_class`), enabling real-time customizable volume adjustment via a horizontal layered trackbar slider control.
- Designed dynamic size, geometry, and coordinates persistence configurations under the `[controlbox]` section in the standard `.ini` configuration.
- Configured native tooltip dynamic text updates over the slider matching current system volume percentages precisely without delay.
- Restructured event listeners and subclasses to support window dragging using its top status bar, right-click destruction, and mouse-wheel suppression of controlbox.
- Added a dedicated TDD test scenario `tests/test_controlbox.c` verifying standard class registration and trackbar instantiation.

## [v26.05.18b] - 2026-05-18

### Changed
- Replaced the circular monitor thumbnail cursor with a high-contrast crosshair pointer (centered at the target pixel) for improved location identification. The crosshair retains dynamic coloring based on mouse button state (red, green, blue, yellow) with a black background outline for visibility.

## [v26.05.18a] - 2026-05-18

### Fixed
- Fixed critical startup font alignment issue where `hg_g_tooltip_wnd` (affecting all tooltip sizes dynamically) was receiving uninitialized null structural initialization states because `update_monitor_enum` ran natively before fully initializing `taskbox` tooltips configurations sequentially. Tooltips and fonts now perfectly scale in tandem from internal `.ini` files synchronously at application launch natively.
- Fixed an issue causing monitor thumbnail edit controls (taskbox items) to render implicitly fully black, incorrectly blending out their contents completely securely, by eliminating hard-coded structural fallback class lookups targeting ID 104 incorrectly, dynamically distributing `.bg` opaque rendering commands across all integrated read-only textual controls properly seamlessly securely natively structurally. 

## [v26.05.17e] - 2026-05-17

### Changed
- Disabled visual styling on taskbox tooltips by bypassing the `Explorer` native theme via `CCM_SETWINDOWTHEME` to force identical font scaling and synchronization with the edit control font settings exactly.
- Applied exact Taskbox color scheme matching capabilities dynamically onto tooltips directly assigning foreground and background definitions utilizing `TTM_SETTIPBKCOLOR` and `TTM_SETTIPTEXTCOLOR`.

### Fixed
- Fixed readability issue in the monitor window edit control by assigning the exact `hg_g_edit_bg_brush` background brush in the `WM_CTLCOLORSTATIC` callback matching the exact `hg_g_color_scheme_selected.bg` rather than preserving `BLACK_BRUSH` class definitions ensuring black text isn't lost on black spaces.

## [v26.05.17d] - 2026-05-17

### Fixed
- Addressed `-Wsign-conversion` compilation warnings by completely clearing `HGDI_ERROR` sign validation operations inside critical GDI `SelectObject` bounds handling logic exclusively testing for `NULL` ensuring safer memory allocations intrinsically safely.

## [v26.05.17c] - 2026-05-17

### Changed
- Replaced incorrect `IPolicyConfig` GUID and `IPolicyConfigVtbl` definitions to accurately match the widely supported `CPolicyConfigClient` implementation for default audio endpoint assignment.
- Restructured `get_window_icon()` to securely prioritize AUMID/Package icon extraction for UWP `ApplicationFrameHost` proxies before blindly falling back to legacy `WM_GETICON` results.

### Fixed
- Fixed `set_default_audio_device()` logic to properly process multiple independent role assignments gracefully and strictly return a valid boolean success state. 
- Prevented potential crash in `update_audio_device_list()` by adding explicit validation for `VT_LPWSTR` variant type and non-null pointers during property store value retrieval.
- Ensured stringent memory failure handling and correct release flows inside `get_aumid_from_hwnd()` resolving string copy vulnerabilities.
- Applied rigorous GDI handle validation checks inside `WM_EXITSIZEMOVE` and `WM_PAINT` handlers to protect against silent context initialization failures causing structural visual regressions.
- Included `<wctype.h>` explicitly securing correct macro linkage for wide character classification within `normalize_path_for_api()`.
- Updated `build.bat` adding the missing `-lpropsys` linker flag specifically into the testing suite compilation pipeline avoiding implicit property system linkage errors on GCC.

## [v26.05.17b] - 2026-05-17

### Changed
- Aligned `monitor_wnd_proc` edit and static control coloring to match the taskbox explicitly, utilizing `hg_g_color_scheme_selected` structurally instead of static native values.
- Re-enabled `WM_LBUTTONDOWN` inside `edit_subclass_proc` to properly restore the legacy "drag to move" functionality of the taskbox via its edit control and deliberately suppressed native text selection behaviors cleanly securely.

## [v26.05.17a] - 2026-05-17

### Fixed
- Fixed critical infinite recursion crash in `WM_MOUSEWHEEL` handling caused by cyclic propagation between `window_proc` and `toolbar_proc`.
- Restored missing `WM_LBUTTONUP`, `WM_RBUTTONUP`, and `WM_MBUTTONUP` handler injections in `monitor_wnd_proc` to ensure proper remote interaction lifting.
- Significantly reduced `monitor_wnd_proc` rendering overhead by increasing its frame polling interval and silencing UI updates when minimized or invisible.
- Removed arbitrary row ceiling limits in `get_toolbar_item_rect` logic so that taskbox icons accurately correspond to keyboard shortcuts map.
- Fixed stale theming in dark mode by triggering `init_color_scheme` again whenever `WM_SYSCOLORCHANGE` is dispatched.
- Prevented `edit_subclass_proc` from blindly dispatching `SC_MOVE` on `WM_LBUTTONDOWN` to restore standard text selection capability inside message boxes.
- Enforced native path querying using `get_explorer_path` when reusing intact Explorer processes during `refresh_window_list` fast updates.
- Refactored `set_default_audio_device` endpoint assignment to execute linearly without masking potential independent `HRESULT` configuration failures.
- Dropped unused redundant system parameter override of `hg_g_current_font_size` prior to `load_taskbox_font_config`.

## [v26.05.14d] - 2026-05-14

### Fixed
- Fixed unhandled compilation warning for an unused `hit` variable inside `WM_NCHITTEST` after stripping deprecated hit testing behaviors.
- Ensure that the primary floating main window correctly listens to `WM_DISPLAYCHANGE` so UI updates dynamically without waiting on taskbox interaction.
- Synchronously dispatch monitor termination messages using `SendMessageW` on monitor detachment to guarantee immediate closure of orphaned monitors prior to cache invalidation.
- Removed suppressed context menu events directly using `WM_RBUTTONDOWN` over `WM_RBUTTONUP` inside the monitor window subclass to properly exit instances via right-click without invoking default edit context menus.

## [v26.05.14c] - 2026-05-14

### Fixed
- Fixed orphaned remote monitor windows persisting after their physical monitor is disconnected.
- Properly process `WM_DISPLAYCHANGE` to update the monitor list and explicitly unregister missing hardware monitors while forcefully terminating their respective application windows.

## [v26.05.14b] - 2026-05-14

### Changed
- Simplified and optimized monitor interactions by completely removing the mouse hook and implementing direct window click teleportations.
- Replaced remote monitor cursor crosshair with a dynamically colored circular cursor indicating current mouse button statuses (red=default, green=left, blue=right, yellow=middle, all with black outlines).

## [v26.05.14a] - 2026-05-14

### Fixed
- Fixed unclickable remote icons caused by the OS ignoring injected mouse down events during an active physical mouse down state by delaying the exact injected input using `PostMessageW(WM_APP + 102)` and suppressing the physical button lift event.

## [v26.05.14] - 2026-05-14

### Fixed
- Fixed a compilation warning related to implicit conversion from `WPARAM` to `UINT` in `PostMessageW` call within the monitor interaction hook.

## [v26.05.13j] - 2026-05-13

### Changed
- Improved context menu compatibility on monitors: Monitor interaction uses a global mouse hook instead of window messages to intercept and teleport the physical mouse before the OS registers the click on the thumbnail, completely preventing remote context menus and focus windows from being dismissed by OS-level thumbnail focus activation.

## [v26.05.13i] - 2026-05-13

### Changed
- Fixed context menus automatically closing upon clicking monitor thumbnails by returning `MA_NOACTIVATE` to `WM_MOUSEACTIVATE` inside the monitor window message loop and creating the monitor window with `WS_EX_NOACTIVATE`, which prevents it from stealing focus during interactions.

## [v26.05.13h] - 2026-05-13

### Changed
- Fixed monitor thumbnail's title input box flickering issue by enforcing WS_CLIPCHILDREN style on the monitor window, preventing the background redraw cycle from overlapping the edit control.

## [v26.05.13g] - 2026-05-13

### Changed
- Refined monitor thumbnail mouse interaction sequence: During a click or drag, the local physical cursor is cleanly released and moved to the remote monitor before synthesizing the down event, and accurately waits for a 50ms interval after the physical release event to ensure drop mechanics register properly before snapping the cursor back.

## [v26.05.13f] - 2026-05-13

### Changed
- Improved monitor thumbnail interaction: The global cursor location is now highlighted on the thumbnail with a red crosshair in real-time (updated at 30 FPS). Clicking a thumbnail (representing a remote monitor) instantaneously jumps the physical cursor to the selected display to ensure native drag-and-drop operations perform flawlessly, then cleanly snaps the cursor back to the thumbnail exactly where the user released the mouse button.

## [v26.05.13e] - 2026-05-13

### Changed
- Refined monitor thumbnail input forwarding: Interaction (clicks, drags, scroll) is now securely bypassed when the thumbnail represents the monitor where the physical cursor is currently located, preventing local cursor conflicts while preserving seamless remote interactions.

## [v26.05.13d] - 2026-05-13

### Changed
- Improved monitor thumbnail interaction: Mouse clicks, drags, right-clicks, and scroll wheel events are now forwarded directly to the actual monitor using `PostMessage` without moving the user's cursor position. This prevents the cursor from jumping when interacting with the monitor thumbnail.

## [v26.05.13c] - 2026-05-13

### Added
- Added an edit box to the top of the monitor window to display the monitor's name.
- Mouse events (left click, right click, and dragging) on the monitor window's thumbnail are now accurately forwarded to the corresponding position on the actual monitor.

### Changed
- The monitor window can now be dragged by clicking and dragging its edit box.
- Right-clicking the monitor window's edit box now closes the monitor window.
- The font size of the monitor window's edit box now synchronizes with the taskbox's font settings dynamically.

## [v26.05.13b] - 2026-05-13

### Changed
- Refined monitor preview window border styling to perfectly match the taskbox and floater components (border thickness and background colors).

### Fixed
- Addressed compilation warnings (e.g. MAX_MONITORS sign conversion) and an undeclared identifier error (IDM_AUDIO_DEVICE_BASE).

## [v26.05.13a] - 2026-05-13

### Added
- Added individual monitor thumbnail viewing feature. You can open them from the floater context menu.

## [v26.05.10a] - 2026-05-10

### Changed
- Improved volume control logic: setting volume to a non-zero value now automatically unmutes the system.

## [v26.05.07o] - 2026-05-07

### Added
- Implemented Taskbox "M" button drag-to-move functionality using direct window position updates for smoother interaction.

## [v26.05.07n] - 2026-05-07

### Fixed
- Fixed Taskbox "M" button (move handle) functionality by explicitly targeting the taskbox window for the move system command.

## [v26.05.07m] - 2026-05-07

### Fixed
- Fixed audio output device selection failure by correctly mapping `IPolicyConfig10` versus `IPolicyConfig` GUIDs and prioritizing the most compatible interface for modern Windows 10/11 environments.

## [v26.05.07k] - 2026-05-07

### Fixed
- Further corrected `IPolicyConfig10` VTable structure to fix audio output switching on the latest Windows 10/11 updates.

## [v26.05.07j] - 2026-05-07

### Changed
- Refined the timing of audio device list updates: the scan now occurs only when the **Audio Output** sub-menu is about to be displayed (`WM_INITMENUPOPUP`), ensuring real-time accuracy without blocking the main context menu.

### Fixed
- Fixed audio output device switching failure by correcting the `IPolicyConfig10` interface VTable structure for Windows 10/11 compatibility.

## [v26.05.07i] - 2026-05-07

### Changed
- Changed audio output device list update to occur only when opening the context menu (real-time discovery).
- Removed periodic background caching of audio devices to reduce background resource usage.

### Fixed
- Further refined `IPolicyConfig10` VTable and logic for switching audio output devices on Windows 10/11.

## [v26.05.07h] - 2026-05-07

### Fixed
- Improved compatibility for switching audio output devices on Windows 10/11 by implementing fallback for `IPolicyConfig10` interface.

## [v26.05.07g] - 2026-05-07

### Fixed
- Fixed "multiple definition" linker errors by removing `initguid.h` and manually defining specifically required Core Audio GUIDs.
- Reverted unintentional library dependency in `build.bat`.

## [v26.05.07f] - 2026-05-07

### Fixed
- Resolved undefined reference errors for Core Audio GUIDs (IID_IAudioEndpointVolume, etc.) by including `initguid.h`.
- Updated `build.bat` to link against `mmdeviceapi` library.

## [v26.05.07e] - 2026-05-07

### Fixed
- Fixed compilation errors in `hgfloater.c` caused by missing `MAX_AUDIO_DEVICES` definition and redundant `ERole` types.
- Resolved type mismatch and sign conversion warnings in audio control logic.

## [v26.05.07d] - 2026-05-07

### Added
- Integrated system volume control into Floater context menu:
    - Displays current volume percentage.
    - Provides presets (0, 25, 50, 75, 100%).
- Integrated system audio output device selection into Floater context menu:
    - Lists active playback devices.
    - Allows switching default output device.
    - Implemented background caching for device list (updates every 60 seconds).

## [v26.05.07c] - 2026-05-07

### Added
- Replaced all instances of `TrackPopupMenu` with `TrackPopupMenuEx` for more standard Win32 menu management.

## [v26.05.07b] - 2026-05-07

### Added
- Enabled opening context menus using the **Enter** (VK_RETURN) key on focused taskbox items.

### Changed
- Separated context menu actions for better clarity:
    - **Window items**: Now only show "Focus" (Restore) and window management options (no "Run").
    - **Shortcut items**: Now only show "Run" (no "Focus").

## [v26.05.07a] - 2026-05-07

### Added
- Implemented comprehensive **Long Path Support** (32,768 characters) for Windows 10+ environments.
- Added `hgfloater.manifest` to the resource file to enable `longPathAware` support natively.
- Introduced `normalize_path_for_api` helper function to handle `\\?\` prefixing for core Win32 file APIs.
- Integrated path normalization into `CreateFileW`, `FindFirstFileW`, `GetFileAttributesW`, and `MoveFileW` calls.

### Changed
- Increased internal path buffer `HG_MAX_PATH` from 1024 to 32768.

### Fixed
- Fixed compiler warnings regarding signedness mismatch (`-Wsign-compare`) in `toolbar_proc` context menu logic.

## [v26.05.07] - 2026-05-07

### Added
- Added "Run" and "Focus / Restore" to the Taskbox context menus for shortcuts and window items respectively.
- Added "Open File Location" to the context menu for real shortcut items.
- Unified `Enter` key behavior in the Taskbox to always open the context menu for the focused item (both tasks and shortcuts).
- Restored keyboard functionality for the Taskbox: `Space` for launching/focusing items.
- Added a "System Shutdown (&S)" option to the Floater's context menu.
- Enabled the `F2` key to trigger the Floater's context menu when the Floater, Taskbox, or its edit control has focus.

### Fixed
- Fixed compilation error in `toolbar_proc` regarding undeclared `is_moving_taskbox`.
- Fixed shadow variable warnings in `window_proc` (`icon_size`, `rc_toolbar`).

### Changed
- Reverted code structure and features to a simplified state based on manual user edits.
- Updated versioning for the new date and state.

## [v26.05.06e] - 2026-05-06

### Changed
- Reverted the `Space` key in the Taskbox grid to its original behavior (focus window / execute shortcut).
- `Enter` key now triggers the item's context menu.

## [v26.05.06d] - 2026-05-06

### Changed
- Changed the `Enter` and `Space` key behavior in the Taskbox grid to invoke the item's context menu.
- Added a `Focus (&F)` or `Execute (&O)` option to the top of the context menu to allow activating windows or shortcuts.

## [v26.05.06c] - 2026-05-06

### Changed
- Changed the context menu trigger key from `Space` to `F2`. Pressing `F2` over the Floater, Taskbox, or the Taskbox's edit control will now open the Floater's context menu.

## [v26.05.06b] - 2026-05-06

### Added
- Added keyboard support for the context menu: pressing `Space` while the floater is focused will now open the context menu, equivalent to a right-click.

## [v26.05.06a] - 2026-05-06

### Added
- Added a "Power Off" option to the floater's right-click context menu. Selecting this triggers the native Windows Shutdown dialog, functioning identically to pressing `Alt+F4` on the desktop.

## [v26.05.06] - 2026-05-05

### Added
- Implemented `refresh_window_list(TRUE)` to force-reload icons when the taskbox is shown.
- Added explicit initialization for `hotkey_registered` and `accel_table` at the start of `wWinMain` to resolve compiler warnings regarding uninitialized variables during error cleanup paths.

### Fixed
- Fixed layout variable scope issues that caused build warnings.
- Ensured `refresh_window_list` only runs when the window is visible to save resources.
- Corrected `HGDI_ERROR` comparison by using explicit pointer-sized casts to suppress signed conversion warnings.

## [v26.05.05h] - 2026-05-05
### Fixed
- Fixed the `M` (Move Window) icon logic by implementing a manual move state machine in the toolbar control. This directly manipulates the parent taskbox position via `SetWindowPos`, providing a robust and perfectly synchronized move experience that bypasses multi-level window hierarchy messaging issues.

## [v26.05.05g] - 2026-05-05
### Fixed
- Fixed the `M` (Move Window) icon logic to move the entire taskbox instead of just the child toolbar. This was achieved by sending `WM_SYSCOMMAND` with `SC_MOVE | 0x0002` directly to the parent taskbox window on `WM_LBUTTONDOWN`, successfully bypassing the child window hit-test limitations.

## [v26.05.05f] - 2026-05-05
### Fixed
- Fixed the `M` (Move Window) icon failure by returning `HTCAPTION` for the icon area in the toolbar's `WM_NCHITTEST`. This leverages Windows' native caption-drag behavior, providing a perfectly smooth and reliable move interaction that works identically to standard window titles even from a child toolbar control.

## [v26.05.05e] - 2026-05-05
### Fixed
- Re-implemented the `M` (Move Window) icon logic using a reliable drag-threshold state machine in `WM_MOUSEMOVE` (similar to the main widget), ensuring the system move loop initiates correctly even from a custom toolbar child control.

## [v26.05.05d] - 2026-05-05
### Fixed
- Fixed the `M` (Move Window) icon failure by implementing an immediate move trigger using `WM_SYSCOMMAND` and `SC_MOVE | 0x0002` on `WM_LBUTTONDOWN` inside the toolbar procedure.

## [v26.05.05c] - 2026-05-05
### Fixed
- Fixed an issue where the `M` (Move Window) icon failed to initiate dragging on the taskbox by replacing `WM_NCLBUTTONDOWN` with a highly robust `WM_SYSCOMMAND` with `SC_MOVE | 0x0002` sent via `SendMessageW`, ensuring perfectly synchronized drag states across child controls and the parent system message loop.

## [v26.05.05b] - 2026-05-05
### Fixed
- Fixed a bug where dragging the `M` (Move Window) taskbox icon or interacting with edit controls incorrectly passed internal client-relative bounds natively to `DefWindowProc`, successfully replacing it with strict screen-relative `GetCursorPos` mapping for highly reliable system-level `WM_NCLBUTTONDOWN` drag movements globally across the taskbox window.

## [v26.05.05a] - 2026-05-05
### Added
- Expanded the default number of functional icons within the taskbox from 4 to 5 (R, M, X, D, S).
- Added a new 'M' (Move Window) icon allowing users to cleanly reposition the taskbox by dragging it directly.
### Changed
- Standardized taskbox sizing and cell layout logic globally across `hgfloater.c` to gracefully respond to the expanded grid index.
- Migrated hard-coded shortcut item index counts (magic number 4) into a structured `NUM_BASIC_ICONS` C macro.

## [v26.05.05] - 2026-05-05
### Changed
- Reverted context menu drawing logic back to standard Win32 native menus (removed Owner-Drawn custom rendering style that followed `font_name`), ensuring system UI familiarity for popup menus.
- Replaced the 'M' (Menu) toolbar button with an 'S' (Settings) button in both `hgfloater.c` and `index.html`. Tooltips and edit control logs were correctly updated to display "Settings" instead of "Menu" dynamically.

## [v26.05.04] - 2026-05-04
### Added
- Expanded task icon context menu to include options for moving windows to (0, 0) and resizing windows to various aspect ratios (4:3, 16:9, 9:16) with predefined dimensions.
- Added Taskbox Edit control font sizes storage persistently into config.ini beneath the [taskbox] section so scaling resets to saved value at startup natively.
- Implemented application quit hotkeys: `Ctrl+Q`, `Ctrl+X`, and `Alt+F4` universally on hover or focus.
- Implemented global reset hotkeys: `Ctrl+R`, `Ctrl+0`, and `F5` instantly resetting all layout sizes, UI opacity, and fonts simultaneously.
- Implemented `F1` shortcut to trigger About menu.
- Implemented `Ctrl` + `+/-` incrementing scaling controls efficiently everywhere properly matching `Ctrl+Mouse Wheel` behavior.
- Added `Esc` key support to close the About dialog directly while focused inside the About dialog or its Edit control.

### Fixed
- Fixed compilation errors arising from implicit function declarations by properly adding forward declarations for `save_config`, `save_floater_font_config`, and `save_taskbox_font_config` at the top of the file before their usage.

### Changed
- Integrated global custom typography support by loading `font_name` underneath the `[etc]` section inside `config.ini`, seamlessly synchronizing the selected font uniformly across the Main Edit Control, Explorer Tooltips, and the standalone About dialog window natively (Defaulting safely back to `Segoe UI` if omitted). Context menus remain native Win32 standard UI elements prioritizing OS familiarity.
- Explicitly documented and implemented missing `icon_size` storage underneath the `[taskbox]` section in `config.ini`, ensuring Taskbox icon bounds are automatically properly initialized, persisted correctly upon resizing, and mirrored accurately inside `README.md` and mockups natively.
- Refactored `config.ini` initialization to automatically populate all default layout coordinates, dimensions, and window alpha properties natively upon the first launch securing structural consistency out-of-the-box.
- Cleaned up obsolete font size parameters by migrating legacy `[etc] floater_font_size` natively into `[floater] font_size`, alongside erasing unneeded redundant entries like `time_font_size`, `date_font_size` and `[taskbox] edit_font_size` mapping purely structured configuration logic consistently.
- Removed the unused `icon_size` parameter from `[etc]` in `config.ini` and its documentation.
- Replaced the `explorer.exe`-specific icon workaround with a universal, stable approach that dynamically utilizes `CopyIcon` to safely take ownership and cache icons without depending on external processes preserving the handles.
- **Improved Taskbar Synchronization**: Implemented `RegisterShellHookWindow` natively capturing Windows Explorer's `HSHELL_REDRAW`, `HSHELL_WINDOWCREATED`, and `HSHELL_WINDOWDESTROYED` messages seamlessly. This perfectly synchronizes application changes (e.g. icon updates during tab switching) universally for all apps without relying on periodic polling delays nor hooking processes invasively (which trips anti-virus flags).
- Migrated legacy configuration files from `config.ini.txt` to standard `config.ini` extension, automatically copying existing configs to smoothly retain old state seamlessly.
- Removed disabled explanatory headers (e.g., `--- 4:3 ---`) from the task widget's right-click context menu for improved visual clarity and menu compactness.
- Removed unused legacy icon extraction logic (`extract_system_icon`, `get_icon_from_hwnd`) to resolve compiler warnings and maintain clean codebase.
- Fixed a sign conversion compiler warning in `load_taskbox_font_config`.
- Separated `Ctrl+(+/-)` and `Ctrl+Mouse Wheel` specific behaviors explicitly. Keyboard font scaling controls now exclusively target internal text boundaries preventing dynamic taskbar icon arrays resizing simultaneously unintentionally.
- Reprogrammed `update_edit_font_size` distinctly scaling the `hg_g_edit_font_size` variables completely disjointed from physical Toolbar icon constraints granting precise text readability tuning structurally cleanly.
- Fixed an issue where the tooltip font was not correctly reset to Segoe UI when triggering settings reset (e.g., `Ctrl+R`).
- Added fallback to `SHGetFileInfoW` for retrieving missing base icons from generic applications like explorer.exe.
- Made the top Edit Console font seamlessly synchronize with `update_size()` scaling instead of freezing at 16px.
- Extended hotkey functionality globally allowing hovered components (Edit, Taskbox, Floater) to respond intuitively to mouse-wheel resizes and alpha adjustment.

## [v26.04.63] - 2026-04-30
### Changed
- Replaced `hgfloater.ico` with user-uploaded file containing manually verified multiresolution (16x16, 32x32, 64x64) sizes to better guarantee sharp pixel art display on Windows taskbars.

## [v26.04.62] - 2026-04-30
### Fixed
- Fixed an issue where the main program icon (`hgfloater.ico`) appeared blurry or tiny on the Windows taskbar, especially on dark mode themes. This was caused by the lack of multi-resolution embedded icons. Generated scaling for 16, 24, 32, 48, 64, 128, and 256 pixel sizes using nearest-neighbor interpolation to preserve the pixel art look.

## [v26.04.61] - 2026-04-30
### Changed
- Converted `hgfloater.png` to `hgfloater.ico` and integrated it as the default executable icon by compiling it via `hgfloater.rc` in `build.bat`.

## [v26.04.60] - 2026-04-30
### Fixed
- Fixed an issue where the shortcuts directory path was incorrectly renamed to `hg_g_shortcuts` during refactoring, preventing user shortcuts from loading correctly.

## [v26.04.59] - 2026-04-30
### Changed
- Changed the date output format in the floater window and internal UI mockups from "Month DD, Day" to "Day, Month DD".

## [v26.04.58] - 2026-04-28
### Added
- Linked `propsys.lib` to resolve potential linker issues.
- Added a fallback parsing canonical shell ID (`shell:::{4234d49b-0245-4df3-b780-3893943456e1}\[AUMID]`) for robust UWP icon retrieval in the AppsFolder.

### Changed
- Refactored all global variables in `hgfloater.c` to consistently use the `hg_g_` prefix for better namespace isolation.
- Reordered the `get_window_icon` logic to prioritize finding child UWP app icons first for `ApplicationFrameHost.exe`.
- Separated standard `current_font_size` logic into `taskbox_icon_size_dp` and `taskbox_icon_size_px` for clear distinction over dynamic DPI variables.
- Renamed `color_scheme_dark` to `color_scheme_system` and `color_scheme_light` to `color_scheme_custom_dark`.
- Renamed `MIN_ALPHA` to `MIN_OPACITY_ALPHA_FOR_70_PERCENT_TRANSPARENCY`.

### Removed
- Removed the unused `extract_system_icon` function.

### Fixed
- Fixed improper memory string copies and bounds evaluation inside `get_aumid_from_hwnd`.
- Fixed missing visual UI taskbox updates when calling `refresh_window(TRUE)` not forcing an icon re-read natively cleanly flawlessly securely.
- Fixed COM issues correctly evaluating against `RPC_E_CHANGED_MODE`.

## [v26.04.57] - 2026-04-28
### Fixed
- Fixed a fundamental flaw in the icon retrieving sequence where the presence of a generic legacy Win32 default window class icon would prematurely abort the attempt to retrieve proper UWP or Packaged app icons. By specifically querying for dynamic window message icons first (`WM_GETICON`), then explicitly attempting AUMID/Package asset retrieval, and finally falling back to default class handles (`GCLP_HICON`), modern UWP apps such as Windows 11 `SystemSettings.exe` now properly display high-resolution icons rather than empty/fallback Win32 graphical placeholders natively cleanly flawlessly securely.

## [v26.04.56] - 2026-04-28
### Removed
- Removed the specific fallback algorithm targeted at `SystemSettings.exe` since the modern `GetApplicationUserModelId()` approach gracefully handles all generalized AppUserModelID retrievals inherently, creating a single robust generalized method applicable uniformly to all UWP apps accurately dynamically natively robustly without unnecessary specific redundant code.

### Fixed
- Fixed UWP icons not loading by substituting `SHCreateItemInKnownFolder` with `SHCreateItemFromParsingName` utilizing the `shell:AppsFolder\[AUMID]` URI scheme directly which seamlessly natively resolves modern AppUserModelIDs cleanly across the Windows 11 Virtual Folder Shell Namespace.

## [v26.04.55] - 2026-04-28
### Removed
- Removed the unnecessary persistence of `window_order` to `config.ini.txt` during icon reordering via drag & drop, correctly managing taskbar window order purely in-memory as native UI layout states strictly dictate dynamically at runtime without redundant permanent storage updates overhead.

### Fixed
- Fixed UWP app icon fetching resolution algorithms uniquely catering to modern Windows 11 paradigms intelligently dynamically; actively utilized native `GetApplicationUserModelId()` extracting Application User Model IDs bridging accurately towards `SHCreateItemInKnownFolder()` for definitive system-provided high-res assets retrieval flawlessly consistently addressing scenarios where prior manifest parsing inevitably failed cleanly for specific integrated generic store apps effectively completely rendering `SystemSettings.exe` perfectly smoothly locally optimally securely.

## [v26.04.54] - 2026-04-28
### Changed
- Reorganized codebase to consolidate all macro constants, structures, and global variables to a single, unified preprocessor section at the top of the file securely accommodating greater architectural readability globally natively.
- Re-implemented color palette swap connecting Light Mode arrays securely to static dark variables uniquely explicitly cleanly mapping earlier directives natively efficiently securely.

## [v26.04.53] - 2026-04-28
### Added
- Associated middle-mouse button click event (`WM_MBUTTONUP`) specifically upon runtime task window icons seamlessly triggering remote termination requests via standard asynchronous `WM_CLOSE` messages securely.

## [v26.04.52] - 2026-04-28
### Added
- Created context menu dynamically spawned by the newly mapped 'S' Settings icon mapping explicitly directly into `shortcuts_path` explorational opens, `config.ini` edits directly using Notepad execution, and native Layout/Geometry/Font `IDM_RESET_ALL` resets safely mapping.

### Changed
- Integrated standalone `uwp_icon_helpers.c` natively within central `hgfloater.c` source bypassing fractured dependencies maintaining strict single-file compilation purity accurately comprehensively.
- Swapped native 'D' (Desktop) and 'S' (Open Shortcuts) button rendering logically relocating Desktop bounds into index 2 and transmuting Shortcuts array index 3 directly into fully-fledged 'S' (Settings) interactive states gracefully.

## [v26.04.51] - 2026-04-27
### Changed
- Swapped Light Mode and Dark Mode color themes gracefully natively assigning Dark themes inherently during Light Mode environments and Light Themes during Dark Mode OS settings securely.
- Exported hardcoded RGB magic color integers to explicitly defined preprocessor `#define` configuration constants safely accommodating simple visual theme adjustments mapped reliably efficiently natively.
- Swapped `Ctrl+Arrow` and `Alt+Arrow` keyboard bindings logically assigning window layout movements strictly toward the `Alt` modifiers enforcing `Ctrl` boundaries toward internal grid layout array scaling completely securely gracefully natively.
- Eliminated residual vertical margin generation mappings enforcing exact grid sizing calculations mapping `rows * row_height` absolutely locking vertical adjustments effectively perfectly tight completely mapping dynamic boundaries accurately without unused slack padding internally.
- Refined internal Taskbox `edit_msg_wnd` header string reflecting accurately mapped layout control semantics referencing `Ctrl+Arrow` Grid capabilities cleanly directly inside Native UI.
- Fully synchronized `REQUIREMENTS.md` specifying dynamic Array columns and row algorithms reflecting `Ctrl+Up/Down/Left/Right` behaviors safely exactly natively correctly natively.

## [v26.04.50] - 2026-04-27
### Fixed
- Improved vertical resizing logic to naturally jump precisely by row limits without empty space margins for `Alt+Up/Down` keyboard scaling.
- Consolidated continuous vertical boundary matching for `R` drags and standard window border resizes ensuring perfect snap-to-grid dimensioning natively constraints.
- Restored vertical resizing (`Alt+Up/Down`) sizing the taskbox perfectly by precisely recalculating and aligning height rows individually securely natively.
- Reintroduced native drag-to-resize border logic (`HTTOP`/`HTBOTTOM`) suspending automatic snap bounds directly applying them upon `WM_EXITSIZEMOVE` guaranteeing smooth visual scalings completely robustly.

## [v26.04.49] - 2026-04-27
### Fixed
- Fixed critical MinGW-w64 GCC linker reference anomaly mapping `wWinMain` entry points natively bypassing `undefined reference` execution faults structurally resolving `-municode` flag integrations securely correctly.

## [v26.04.48] - 2026-04-27
### Fixed
- Fixed critical syntax mapping conflict inside `uwp_icon_helpers.c` structurally bounding `append_message` explicitly matching local debug constraints natively circumventing header substitution collisions cleanly preventing compiler faults natively.

## [v26.04.47] - 2026-04-27
### Fixed
- Fixed `update_theme_colors` dark mode detection reversing logic natively correctly distinguishing Light/Dark states while initializing registry `DWORD` properties reliably avoiding system theme misconfigurations cleanly.
- Implemented exact fallback guards gracefully managing `taskbox_wnd` structural initialization failing bounds triggering native `MessageBoxW` messages instead of falling into silent dead messaging loops natively.
- Evaluated UWP and WinUI child process bounding chains comprehensively extracting true package identifiers via `GetPackageFullName` seamlessly parsing `AppxManifest.xml` extracting highest scalable resolution native application tile properties directly gracefully natively parsing.
- Corrected C compilation warnings strictly handling signed-to-unsigned integral conversions mapping `CP_UTF8` memory blocks securely resolving multi-byte scaling arithmetic bounds explicitly natively natively.
- Refactored `get_window_icon` integrating `IShellItemImageFactory` and robust fallback UWP bounding techniques resolving complex transparent Windows 11 icons completely structurally perfectly natively.
- Eliminated redundant `load_shortcuts` cyclical calls natively from `refresh_window_list` enhancing native loop scalability preventing disk repetitive reads gracefully.
- Checked bounding logic structurally mapping `s_idx` correctly explicitly comparing against `g_app.window_count` before accessing `g_app.window_items` drawing calls specifically within the toolbar's `WM_PAINT` handlers natively.
- Validated `get_process_info` failed paths strictly initializing output buffer pointers towards `L'\0'` strings bypassing stack misread possibilities cleanly mapping robust default conditions explicitly.
- Freed active `g_taskbox_edit_brush` handles manually inside global `cleanup_app` sequence securing background static GDI references preventing memory leaks during application tear-down gracefully reliably.

## [v26.04.46] - 2026-04-27
### Fixed
- Hardened `get_window_icon` tracking `own_icon` boolean scopes preventing unowned borrowed handle `DestroyIcon` double-frees and potential execution crashes during native list refreshes properly natively.
- Migrated internal configurations folder setups mapping structurally robust `SHCreateDirectoryExW` generations ensuring `.HellGates` parent and subdirectories bootstrap smoothly avoiding silent write exceptions dynamically natively.
- Repositioned core `show_taskbox` rendering pipeline validating `ShowWindow` dispatches distinctly prior applying sequential `refresh_window_list` mapping constraints guaranteeing visual grid geometries paint reliably gracefully natively.
- Consolidated structural termination footprints securely mapping unified `cleanup_app` logic unwinding native active `RegisterHotKey` bindings, floating transparent windows fonts, shortcut graphics hooks, and explicit trailing `CoUninitialize` bounds systematically natively securely.
- Applied responsive dimensionality mapping rectifying generic low-byte shifts replacing purely with structural `GET_X_LPARAM` / `GET_Y_LPARAM` handling explicitly preventing negative relative cursor coordinates clipping smoothly cleanly safely.
- Inverted `update_theme_colors` generating explicit contrast values accurately pairing light mode dark grays backgrounds correcting dynamically reversed theme parameters visually intuitively.

## [v26.04.45] - 2026-04-27
### Added
- Created dedicated interactive `Help` dialog dynamically loading stripped `README.md` plain text directly inside standalone read-only, vertically scrollable standard multi-line edit environments bound efficiently via universal `F1` keystroke mappings.
- Implemented robust `SysLink` class-based standard user `About` modal dialog displaying hyperlinked, visually stylized developer identity descriptions mapping accurately to direct browser dispatch mechanisms.
- Expanded `get_window_icon` pipeline reliably bypassing unpopulated `ApplicationFrameHost.exe` shells tracking actual resident `Windows.UI.Core.CoreWindow` application container processes enabling rich visual native component extractions (UWP icon extraction fallback).

### Changed
- Refactored `about_proc` to replace the native `SysLink` control with a standard `EDIT` component. This resolves a silent failure where the About window displayed empty content due to `SysLink` dependency on ComCtl32.dll version 6 (which is unavailable without an application manifest) while preserving high-contrast text readability and integrating the dynamic `HG_ABOUT_TEXT` definitions effectively.
- Explicitly enforced a high-contrast Black-on-White (`RGB(0, 0, 0)` on `RGB(255, 255, 255)`) static color scheme for the `Help` and `About` windows and their internal `EDIT` components, detaching their appearance from dynamic system theme configurations natively.
- Replaced dynamic `README.md` runtime disk I/O reads with pre-compiled static macro header `HG_ABOUT_README_W` auto-generated by the internal `build.bat` workflow completely embedding documentation into standalone executable files securely.
- Added rule to `AGENTS.md` strictly prohibiting emojis in AI-generated texts or documentation elements natively.
- Removed arbitrary decorative emojis structurally from `README.md` securing conformity with newly integrated plain-text UI constraints natively cleanly.

### Fixed
- Fixed Win11 "Settings" app icon (SystemSettings.exe) resolution by structurally mapping exact static shell IDs (`imageres.dll`, `-114`) natively bypassing standard empty `.exe` assets yielding correct Windows gear icons natively.
- Broadened UWP ApplicationFrameHost cross-process enumeration by dropping explicit "Windows.UI.Core.CoreWindow" exact matching natively finding child rendering sites accurately matching Modern WinUI desktop bridging components.

## [v26.04.44] - 2026-04-27
### Added
- Associated middle-mouse button click event (`WM_MBUTTONDOWN`) specifically upon runtime task window icons seamlessly triggering remote termination requests via standard asynchronous `WM_CLOSE` messages securely.

## [v26.04.43] - 2026-04-27
### Changed
- Configured static inner spacing explicitly setting `TASKBOX_ICON_PADDING` layout macro bounding Taskbox grids reliably mapping `2px` increments identically across all dynamic scales cleanly natively.
- Eliminated all geometry text-boundary floating padding dynamics dynamically scaling inner offsets specifically within the primary Floater Window targeting aggressive 0px paddings securely snapping textual layouts tight mathematically.

## [v26.04.42] - 2026-04-27
### Added
- Integrated a global hotkey visual notification system triggering a tri-state 3x flashing animation sequentially toggling the `g_color_flash` highlighted background color dynamically across both Floater and Taskbox windows comprehensively.

### Changed
- Eliminated internal geometry paddings inside the core Taskbox `edit_msg_wnd` Control, mathematically shifting bounding parameters securely to `0` yielding tighter textual integration gracefully.
- Inverted Dark Mode and Light Mode internal color mappings mathematically flipping background/text contrasts mapping opposite native system settings effectively.
- Propagated `FW_BOLD` font weight definitions extensively augmenting rendering instances across both the main Taskbox Edit control and the secondary Floater Window Date text cleanly.

## [v26.04.41] - 2026-04-27
### Added
- Enabled independent active scrolling text scaling exclusively within Taskbox's Edit Control leveraging `Ctrl+Wheel` targeting uniquely bound text instances, explicitly bypassing generic toolbar icon resizes.
- Extended Taskbox vertical grid snapping mathematically matching diagonal `R` behavior correctly deriving column counts iteratively out of precise vertical pixel heights accurately locking internal row boundaries upon rigid edge-drags (`WMSZ_TOP` & `WMSZ_BOTTOM`).
- Linked immediate persistent settings commits `save_fonts_config()` and `save_alpha_config()` upon all mouse wheel runtime layout manipulations securing real-time configuration sync seamlessly.
- Activated Floater-specific time and date font scale controls through isolated `Ctrl+Wheel` execution bypassing existing default behaviors securely.

### Added
- Rebuilt Edit Control output buffer logic integrating append boundaries, enabling multiline history tracking and natively enforcing bottom-scroll via `EM_LINESCROLL`.
- Enforced instant Tooltips generation dispatching native `TTM_SETDELAYTIME` setting specifically bounding limits targeting exactly 0 milliseconds delay, exactly mirroring matched string payload identical to targeted Edit details immediately gracefully.

### Changed
- Converted isolated Floater time/date settings explicitly binding towards one unified `font_size` config, executing fixed static geometry scaling (x2.8 Time, x1.1 Date).
- Re-architected dynamic memory arrays mapping layout sorting loops bypassing implicit `GetClassLongPtr` randomization enforcing completely static Icon index allocations consistently caching previously registered instances optimally natively preventing ordering inconsistencies cleanly across click navigations securely.
- Bound fixed ~3px top/bottom internal margin calculations across the central output Edit Box organically yielding more dense display spaces mapping cleaner GUI constraints efficiently naturally accurately.
- Refined Floater interaction documentation natively logging left-click active visibility toggle behavior cleanly discarding outdated generic 'double-click' terminology within `REQUIREMENTS.md`.
- Stripped deprecated explicit `Ctrl + (+/-)` generic hotkeys directly targeting `floater_proc` to completely prioritize universally mapped `Ctrl+Wheel` navigation logic specifically restricting legacy overrides.

### Fixed
- Fixed critical notification "ding" sound and blocked updates inside the Taskbox Edit control by appending `ES_AUTOVSCROLL` and dynamically toggling `EM_SETREADONLY` bounding limits before execution cleanly mapped.
- Fixed trailing newline output buffering inside Taskbox's Edit Control by prepending `\r\n` line-breaks strictly when the history length exceeds 0, inherently rendering text on the top readable baseline instantly.
- Enabled native Tooltip dispatching dynamically forwarding `WM_MOUSEMOVE` via `CallWindowProc` properly yielding subclasses to cleanly render text gracefully rapidly without silent drops natively.
- Resolved implicit function declaration warnings for `apply_taskbox_fonts`, `save_fonts_config`, and `update_layout` by hoisting forward declarations natively before subclass procedure utilization gracefully securing clean GCC builds natively.

## [v26.04.40] - 2026-04-27
### Added
- Added dynamic calculation of 5% padding around taskbox toolbar icons, accurately reflected in grid scaling, resizing loops, and UI dimensions to prevent visual crowding.
- Extracted hardcoded icon size limits out into independent macro definitions (`MIN_ICON_SIZE`, `MAX_ICON_SIZE`, `DEFAULT_ICON_SIZE`) structurally isolating sizing variables.
- Developed empty cell structural protections bypassing mouse hover events and arrow-key navigations directly scaling through unfilled blank grid units to correctly select the next valid icon.
- Enhanced focused toolbar icon states changing the structural background filling directly to full accent colors (`g_color_accent`) instead of strictly framing borders.
- Inverted the text and background colors specifically within the Taskbox's Top-level Edit control to establish clear visual distinction separating it permanently from the Icon Grid array.
- Defined specific macro constants (`MAX_LOG_LINES`, `RETAIN_LOG_LINES`) implementing a robust bounded multi-line rolling log buffer internally managed through explicit capacity chunk deletions gracefully circumventing endless memory accretion.
- Documented internal Edit control's visual offset metrics inside `REQUIREMENTS.md`, noting how older text scrolls off-screen cleanly, while standardizing interactions ensuring the entire log accumulation exports comprehensively upon "Copy all logs" requests.
- Implemented `Alt + MouseWheel` functionality within the Taskbox to permit real-time adjustment of its transparency level (`taskbox_alpha`).
- Enabled dynamic font scaling directly within the Floater window using `Ctrl + (+/-)` commands natively adjusting `time` and `date` font metrics iteratively.
- Strengthened `Ctrl + (+/-)` combinations within the Taskbox scaling the internal Edit control font independently from the current `VK_CONTROL` flag interception logic.
- Configured keyboard window navigation using `Alt` + arrow keys (`W`, `A`, `S`, `D`, `H`, `J`, `K`, `L`) allowing precise positional adjustments for both Floater and Taskbox sequentially.
- Refactored `index.html` mock-up strictly merging toolbars and grids executing responsive resize spacer gap calculations correctly snapping bounding boxes securely exactly mapping to the C implementation rules.

### Changed
- Explicitly documented the sequential display rendering order (Active Windows -> Empty Spacer grids -> Shortcuts -> System Utilities) internally tracking the specific empty space bypassing mechanics accurately directly within `REQUIREMENTS.md`.
- Expanded taskbox Edit control horizontal constraints mapping continuously across exact window frame geometries matching interior container width 100% exactly upon dynamic horizontal sizing.
- Adjusted dynamic grid layout logic eliminating empty trailing cells exactly when calculating bounds targeting explicitly single internal array widths arrays (Rows = 1 or Cols = 1).
- Added explicit window state layout invalidation (`update_layout` / `InvalidateRect`) to immediately trigger visual repaints following boundary adjustments executed by `Ctrl+Arrow` scaling commands.
- Refactored `Ctrl+Up/Down` scaling routines strictly adapting columns aggressively to assure definitive expansion and reduction of overall vertical row totals accurately matching required scaling sizes.
- Improved HiDPI scaling accuracy for Edit control height calculation by incorporating `GetTextMetricsW` strictly bounding required vertical layout sizes.
- Precisely centered text formatting rectangle via `EM_SETRECT` preventing font lower clipping during scaling resizing interactions.
- Reordered subclass insertion for Taskbox's Edit control rendering logic resolving severe initialization state corruptions resulting in zero-opacity transparency masking.
- Implemented robust font sizing limits statically via newly-introduced scaling constraints bounds variables preventing excessive GUI geometry overlapping during scale commands.
- Repaired interactive vertical boundary clipping regressions which caused phantom extra rows spawning occasionally when expanding columns tightly.

## [v26.04.39] - 2026-04-27
### Changed
- Capped Taskbox icon scale limits drastically lower natively conforming directly to user constraints (min 16px, max 32px, default 24px).
- Disabled automatic closure of the Taskbox when clicking window icons or shortcuts via mouse & keyboard commands enabling rapid successive invocations.
- Reprogrammed Edit box layout boundaries structurally to inherently compensate font-size dynamic growth using `+14` explicit padded limits enforcing zero visual overlapping between toolbars and header inputs.
- Resequenced `MoveWindow` rendering commands ahead of blocking sizing early-returns ensuring the Taskbox header Edit control maintains visibility even during zero-differential height drag inputs.
- Snapped grid height limits to `min_cells` aggressively disabling vertical drag resizing from appending unnecessary empty icon rows beyond immediate content dimensions.
- Repaired grid geometry tracking inside `WM_KEYDOWN` and `WM_MOUSEWHEEL` by shifting coordinate extraction from Non-Client bounds (`GetWindowRect`) securely into internal canvas bounds (`GetClientRect`).
- Inherited `WS_CLIPCHILDREN` onto the Taskbox core root container preventing `WM_PAINT` backgrounds erasing child static UI panes transparently.

## [v26.04.38] - 2026-04-27
### Added
- Separated testing into the `tests` subfolder and added detailed documentation in `TESTS.md` describing intentions, scopes, and failure handling procedures for all current tests.
- Redesigned the Taskbox toolbar layout to completely eliminate unused padding or margins, perfectly conforming dynamic grid sizes identically to scaled properties constraint.
- Implemented intelligent reverse-alphabetical sorting for all loaded shortcuts prior to Taskbox placement, assuring visual stability.

### Changed
- Integrated an automated testing phase directly into the `build.bat` multi-menu loop, replacing manual compilation of test codes, with a dedicated `run_tests` subroutine that batches over all `.c` files in `/tests`.
- Relocated test behavior regulations and lists prominently into `/docs/ai/TESTS.md` serving as the source of truth for writing/running isolated tests.
- Removed deprecated CJS-based single-use structural refactoring algorithms (`fix_floater.cjs`, `fix_taskbox_drag.cjs`, etc.), centralizing remaining isolated test scripts exclusively into the `/tests` boundary folder.
- Re-adjusted default and min/max limits for the interactive Taskbox icons to scale smoothly at vastly smaller configurations (Min: 8px, Default: 16px, Max: 64px) providing tighter density.
- Prevented UI freezing during rapid iterative mousewheel zooming inside the Taskbox by excising aggressive synchronous disk `save_fonts_config()` I/O calls out of the live message loop, deferring persistency until standard app shutdown.
- Rendered Taskbox's log Edit control strictly read-only and interaction-transparent, transferring dragging operations gracefully back to the Taskbox window itself by intercepting `WM_LBUTTONDOWN` to mimic non-client `HTCAPTION` clicks.
- Intercepted contextual right-clicks directly on the Taskbox log Edit pane, overlaying an exclusive `Copy all logs` popup menu automatically cloning all accumulated status logs directly to the system clipboard upon activation.
- Re-activated standard window dragging interaction (`HTCAPTION` dispatch via `WM_NCLBUTTONDOWN`) unconditionally across all purely empty filler grids and borders inside the Taskbox layout, allowing spatial repositioning irrespective of grid layout size.
- Revamped dynamic Grid rendering: Reordered standard task icons (left-to-right), followed automatically by generated empty spacer grids, subsequently loading alphabetically sorted shortcut icons, and decisively pinning core system utilities ('S', 'D', 'X', 'R') to the extreme bottom-right edges consistently.
- Perfected `Ctrl+MouseWheel` shortcut to distinctly and exclusively augment scaling size of task icons, directly adapting window rectangle proportions iteratively to permanently maintain preexisting Row and Column capacities.
- Refined `Ctrl+(+/-)` keys within Taskbox purely towards modulating standard Edit box font sizes autonomously without shifting parent grid structures.

### Fixed
- Fixed nested function compilation error by adding missing closing brace in `append_message`, successfully resolving `expected declaration or statement at end of input` and `[-Wpedantic]` errors, resolving standard GUI build via GCC compiler.
- Addressed rendering anomalies rendering the unified logs Edit control invisible by explicitly catching `WM_CTLCOLORSTATIC` internally within the `taskbox_proc` pipeline, properly assigning an opaque `HBRUSH` aligned with the existing dark/light paradigm to paint the client area.
- Fixed integer sign conversion warnings (`-Wsign-conversion`) when compiling with GCC by explicitly casting return values of `GetWindowLongW` to `DWORD` and lengths to `SIZE_T` for memory allocations.

## [v26.04.37] - 2026-04-26
### Added
- Inserted explicit Javadoc-style function documentation universally across the codebase.

### Changed
- Reorganized `#define` macros and constants, explicitly locating them at the very top of `hgfloater.c` immediately succeeding includes.
- Altered Floater date formatting system to display in localized `ddd, MMM d` formulation (e.g., "Mon, Apr 7").
- Restructured `SC()` and `pt_to_px()` for numerical preservation over multiple DPI monitors using robust proportional division constants (`MulDiv`).

### Fixed
- Fixed linker errors by dynamically loading `CoInitializeEx` and `CoUninitialize` from `ole32.dll` to prevent `__imp_CoInitializeEx` linkage failures during static compilation (`-static`).
- Implemented native `wcsrchr` checks to replace `PathFindFileNameW` and `PathFindExtensionW`, sidestepping static linking dependencies for `shlwapi.lib`.
- Patched GCC `void*` vs function pointer cast warnings (`-Wcast-function-type`) for `GetProcAddress` returns by explicitly using `(void(*)(void))` instead of `FARPROC`.
- Corrected missing `append_message` symbol error by reimplementing the lost function logic to populate `g_app.edit_msg_wnd`.
- Reverted MinGW Unicode entry point signature to `wWinMain` per user request and verified `build.bat` correctly provisions `-municode`.
- Restored missing `WM_HOTKEY` and `WM_TIMER` logic in `floater_proc` ensuring the global hotkey triggers the Taskbox correctly.
- Addressed `WM_NCHITTEST` trapping all client-area clicks by restoring drag logic inside `WM_MOUSEMOVE` allowing standard `WM_LBUTTONUP` to spawn Taskbox upon Floater click.
- Enforced `update_floater_size()` immediately following Floater window creation ensuring initial geometry sizing avoids the 1-second `TIMER_REFRESH` startup discrepancy.
- Rectified Taskbox layout truncation bugs by passing calculated bounds through `AdjustWindowRectEx`, accounting for `WS_THICKFRAME` borders, guaranteeing precise visual representation instead of iterative shrinking frames.
- Replaced generic `LOCALE_USER_DEFAULT` date string allocations with absolute `LANG_ENGLISH`/`SUBLANG_ENGLISH_US` combinations rendering the Floater day/month strictly in English format without unneeded spacing.
- Perfected `update_floater_size()` height parameters via independent `pd` and `gap` integer metrics mapping font output natively within the Floater without unused paddings.
- Fixed `GetProcAddress` casting errors strictly bypassing `-Wpedantic` warnings regarding `void*` and function pointer implicit conversions by casting identically via `(FARPROC)`.
- Resolved `hwnd` uninitialized identifier within taskbox runtime bound initialization bounds at `wWinMain()`.
- Eradicated miscellaneous unneeded `.cjs`, `.js` and orphaned source components accumulated during hotfixing compiler syntax routines cleaning repository namespace.
- Remediated strict aliasing syntax anomalies and bounds risk concerning `h_icon` implicit mutations across `SendMessageTimeoutW` function bindings.
- Addressed severe source code corruption where `floater_proc` and `toolbar_subclass_proc` were merged unintentionally causing loss of old window dragging logic, Floater paint behavior and taskbox interactivity hooks. Completely separated them back to independent function blocks.
- Restored original format `GetDateFormatW` functionality for drawing dates in the Floater interface ("예전 방식") correcting the `2026-04-26` hardcode string calculations inside `update_floater_size` scaling loop.
- Adjusted Taskbox RXDS right-justification bounds to account for `_shortcut_count` offset offsets inside Toolinfo pointer assignment loops correctly bounding tooltips inside the rigid grid structure.

## [v26.04.36] - 2026-04-26
### Added
- Independent dynamic font scaling directly derived from true `pt` metrics using `-MulDiv` equivalent formulations combined with per-window DPI logic. Floater and Taskbox now maintain explicitly distinct font settings. 
- Integrated real-time keyboard grid navigation (Arrow keys, `W,A,S,D`, `H,J,K,L` for Vim compatibility) overlayed onto custom grid layout implementation.
### Changed
- Converted grid icon layout from dynamic gaps into a rigid exact rectangular grid with empty non-interactive filler blocks, ensuring systematic alignment of D,S,X,R controls flush right onto the bottom edge.
- Window resizing interactions natively resize grid cell arrays and completely omit artificial floating margins, preserving standard WINAPI interactive toolbar paradigms.

## [v26.04.35] - 2026-04-26
### Fixed
- Restored missing HiDPI and multi-monitor scaling capabilities lost during a previous refactor by implementing dynamic scale calculations per window via `GetDpiForWindow` fallbacks.
- Fixed broken boundary snapping geometries and font truncations caused by fixed `g_app.scale` variables on HiDPI displays.
- Added robust fallback DPI tracking for Windows 7 / 8 compatibility and reinstated `WM_DPICHANGED` handlers to ensure runtime consistency on multi-monitor drag.

## [v26.04.34] - 2026-04-26
### Added
- Implemented global exit hotkeys (`Ctrl+Q`, `Ctrl+X`, and `Alt+F4`), operational from both Floater and Taskbox.
- Implemented global font size zoom hotkeys (`+` and `-`) for dynamically scaling UI elements.
    - Floater mode: Scaling applies individually to the time string (`floater_time_font_size`) and date string (`floater_date_font_size`).
    - Taskbox mode: Scaling applies to the task icons (`taskbox_icon_size`) and the main edit box/tooltips (`taskbox_edit_font_size`) together.
- Mathematical window boundary snapping: Using `R` button or cursor borders to resize the Taskbox now rigidly snaps to the nearest exact grid boundaries (in real-time during `WM_SIZING` events).
### Changed
- Separated font configurations inside `config.ini` for deeper customization. Global `[etc]` font properties removed.
    - `[floater]` section: `font_name`, `time_font_size`, `date_font_size`
    - `[taskbox]` section: `font_name`, `edit_font_size`, `icon_size`
- Floater dimensions (width/height) are now calculated mathematically at runtime based on the defined font sizes using `GetTextExtentPoint32W`, rather than reading hard-coded boundaries from `config.ini`.
- Shortcut icons now explicitly request `SHGFI_LARGEICON` (32x32px) from the Windows Shell to ensure consistent visual scaling sizes alongside active task icons when adjusting grid icon scaling.

## [v26.04.33] - 2026-04-25
### Changed
- Improved website mockup (`index.html`) interactive behavior:
    - Fixed tooltip `z-index` and clipping issues by removing `overflow:hidden` constraints and ensuring top-most priority.
    - Simplified and optimized task grid layout using dynamic `auto-fill` logic to match the real application's behavior.
    - Finalized toolbar button order to follow RXDS (right-to-left) sequence at the bottom-right corner.
- Renamed `[fonts]` configuration section to `[etc]` in `config.ini` and updated documentation/mockup accordingly for better categorization of miscellaneous settings.
### Fixed
- Fixed linker error (`undefined reference to 'WinMain'`) under MinGW-w64 GCC by adding the `-municode` flag to `build.bat` to properly support `wWinMain`.
- Fixed compilation warnings when building with MinGW-w64 GCC by adjusting struct initializers (`HIGHCONTRASTW`), wrapping MSVC pragmas with `_MSC_VER` macros, and resolving function pointer cast warnings.
- Fixed `MAX_VALUE_NAME` undeclared missing compile-time error during shortcuts loading, by using standard `MAX_PATH` bounds.

## [v26.04.32] - 2026-04-25
### Added
- Dedicated `[fonts]` section in `config.ini` to allow custom font name and size selection.
- Support for `font_name`, `icon_size`, and `floater_font_size` in the `[fonts]` configuration.
### Changed
- Refactored configuration system to use the new `[fonts]` section, moving `icon_size` out of the `[taskbox]` section.
- Added validation for font sizes during loading; invalid values or font names will fallback to system defaults (Segoe UI).
- Real-time persistence: Mouse-wheel adjustments to font sizes in both the floater and taskbox are now immediately saved to the configuration file.

## [v26.04.31] - 2026-04-25
### Fixed
- Fixed website mockup layout issues: toolbar and shortcuts are now correctly aligned to the bottom-right and ordered right-to-left.
- Improved resizing script stability and responsiveness in `index.html`.
- Implemented wrap-reverse logic to ensure shortcuts stack upwards from the bottom-right corner when overflowing.

## [v26.04.30] - 2026-04-25
### Changed
- Improved website mockup (`index.html`) with interactive resizing via the 'R' button and border dragging.
- Updated mockup layout: Toolbar buttons ($X, R, D, S$) and shortcuts are now positioned at the bottom-left and wrap upwards for better space utilization.
- Propagated version update to `VER.txt`, `hgfloater.c`, and `index.html`.

## [v26.04.29] - 2026-04-25
### Changed
- Optimized `refresh_window_list` with a prioritized check sequence (Foreground -> HWND Sequence -> Title/Icon) to minimize CPU usage during idle states while maintaining 1s responsiveness for dynamic content.
- Replaced blocking `SendMessageW` with `SendMessageTimeoutW` in `get_window_icon` to prevent application hangs on unresponsive windows.
- Refactored `refresh_window_list` to use internal HWND tracking for faster Z-order detection.

## [v26.04.28] - 2026-04-25
### Changed
- Refactored `hgfloater.c` to consolidate global variables into a single `HgApp g_app` struct for better state management.
- Changed configuration file extension from `.ini.txt` to `.ini` for standard compliance.
- Optimized taskbox refresh logic:
  - Reverted `SetWinEventHook` logic to maintain a simpler timer-based structure.
  - Implemented `refresh_window_list_throttled` to prevent redundant updates within 200ms.
  - Restricted 1s timer refresh to only occur when the taskbox is visible.
  - Added immediate refresh triggers when showing the taskbox or when it receives focus (`WM_ACTIVATE`).
- Updated `index.html` and `README.md` to reflect the new configuration path and version.
### Added
- Implemented Priority 1 fixes based on code review:
  - Added boundary checks for `MAX_SHORTCUTS` and `MAX_WINDOW_ITEMS` to prevent potential array overflows.
  - Switched to `GetWindowLongPtrW` instead of `GetWindowLongW` for 64-bit API compatibility.
  - Replaced `CoInitialize` with `CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)` and added result validation.
  - Added return value checks for `RegisterClassExW` and `RegisterHotKey` for more robust error handling.
  - Moved DPI Awareness initialization (`SetProcessDpiAwarenessContext`) to the very beginning of `WinMain`.
  - Updated Mutex scope to `Local\` (e.g., `Local\hgfloater_single_instance_mutex`) for better isolation across user sessions.
- Consolidated code structure in `hgfloater.c`:
  - Removed duplicated/conflicting definitions of `WinMain` and `taskbox_proc`.
  - Refactored global logic into a cleaner, single-instance execution path.
- Implemented an interactive live demo of the Taskbox in `index.html` with functional Drag and Drop for task icons.
- Added shortcut icons (Photoshop, Steam examples) next to the system buttons (D, S, X, R) in the task switcher preview.
- Integrated custom responsive tooltips that appear on icon hover.

### Changed
- Disabled persistent saving of task icon order in `hgfloater.c` to ensure the order resets upon program restart, aligning with the "no persistence" requirement.
- Updated versioning logic to ensure `VER.txt` remains the source of truth.

## [v26.04.27] - 2026-04-25
### Fixed
- Synced the order of system icons (R, X, S, D) in `index.html` to match the actual rendering logic in `hgfloater.c` (D-S-X-R).

## [v26.04.26] - 2026-04-24
### Fixed
- Improved Explorer window icon robustness by increasing `WM_GETICON` timeout from 50ms to 200ms to handle busy processes.
- Added a fallback logic for Explorer windows to show a generic folder icon if all other icon retrieval methods fail, preventing missing icons.
- Refined `get_window_icon` logic to better handle ApplicationFrameHost windows.

## [v26.04.25] - 2026-04-24
### Added
- Created `CHANGELOG.md` to track all project changes in a structured way.
- Added versioning rules to `AGENTS.md`, designating `VER.txt` as the single source of truth for the version.
- Added changelog update rules to `AGENTS.md` to ensure all AI agents record future modifications.
- Applied the version strings to `hgfloater.c`, `index.html`, and `README.md` to align with `VER.txt`.
- Implemented `Ctrl+Mouse Wheel` support in the Taskbox edit control specifically to resize its font and the tooltip font (`edit_font_size`).
- Added automatic Taskbox window (`taskbox_wnd`) resizing logic (`update_layout`) tightly coupled with `edit_font_size` and integrated bounds checking.
- Enabled loading, preserving, and defining min/max parameters for the taskbox edit font size by recording it in the `config.ini.txt` to keep states synced on multiple executions.
