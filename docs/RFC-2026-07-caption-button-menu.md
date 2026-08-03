# RFC 2026-07: A Menu on Every Window's Maximize Button

Status: Implemented (v26.07.31k; teardown and D5a in v26.08.03; D5b and D5c in v26.08.03c)
Date: 2026-07-31

## Summary

Right-click the maximize button on **any** window's title bar and get hgfloater's
move-and-resize menu for that window: send it to 0,0, or set it to one of the
size presets. The behaviour exists only while hgfloater is running and is gone
the moment it exits.

This is a different kind of feature from everything else in the program. Every
other thing hgfloater does happens inside its own windows. This one changes what
a click does in **someone else's** window, which means a system-wide input hook,
and that deserves its costs written down before any of it is written.

## Is it possible

Yes, without elevation, without injecting anything into other processes, and
without a driver. Three pieces:

### 1. Seeing the click: `WH_MOUSE_LL`

A low-level mouse hook is global, needs no DLL in the target process, and runs
**in our own process** on the thread that installed it. It sees every mouse
message before the target window does and can swallow one by returning non-zero.

The alternative, `WH_MOUSE`, needs a DLL loaded into every process on the
desktop. That is not something this program is going to do.

### 2. Knowing the click was on the maximize button: `WM_NCHITTEST`

`SendMessageTimeout(hwnd, WM_NCHITTEST, 0, MAKELPARAM(x, y))` asks the window
what is at that point. **`HTMAXBUTTON`** is the answer we are looking for.

This is the window's own answer, so it is right by construction for any window
that uses a normal frame - and it is the *window's* answer, so an application
that draws its own title bar may return `HTCLIENT` and never say `HTMAXBUTTON`
at all.

### 3. The fallback for custom title bars: `DWMWA_CAPTION_BUTTON_BOUNDS`

`DwmGetWindowAttribute` with `DWMWA_CAPTION_BUTTON_BOUNDS` returns the rectangle
the caption buttons occupy. Minimize, maximize and close divide it in three, so
the middle third is the maximize button.

Geometry rather than the window's own answer, so it is a guess - but a narrow
one, bounded to a rectangle the system computed, used only when step 2 declined
to answer.

## What it cannot do, stated first

- **Elevated windows are out of reach.** Windows will not let an unelevated
  process hook input destined for an elevated one. Right-clicking Task Manager's
  maximize button will do what it always did. hgfloater runs unelevated on
  purpose and that is not changing for this.
- **Two-button title bars, no title bar, full screen.** Anything with no
  maximize button has nothing to right-click.
- **Applications that answer neither.** A window that returns `HTCLIENT` *and*
  reports no caption button bounds keeps its ordinary behaviour.

None of these is a failure mode that breaks anything. In each case the click
does what it would have done with hgfloater not running.

## The cost, and the one real risk

A low-level mouse hook is called for **every mouse event on the desktop**. Two
consequences:

1. **It must be fast, always.** The hook does one comparison - is this a right
   button press - and returns immediately for everything else. The hit-test only
   runs on a right press, which is rare.
2. **A blocked hook thread blocks the mouse.** Windows gives a low-level hook a
   timeout (`LowLevelHooksTimeout`, 300 ms by default) and silently stops
   calling a hook that keeps exceeding it. This is the real risk, and it is
   sharper here than it would have been a month ago: the same UI thread also
   makes UI Automation calls for the tab feature, which can block on another
   application's UI thread.

   So the hit-test uses `SendMessageTimeout` with `SMTO_ABORTIFHUNG` and a short
   timeout, and never a plain `SendMessage`. A hung target application must cost
   us the menu, not the mouse.

## Design

### D1. The hook installs and uninstalls with the setting, and with the program

`UnhookWindowsHookEx` at exit, and on toggling off. When hgfloater is not
running there is nothing installed and no behaviour to remove.

**A force-kill is safe by construction.** A hook is a process-owned resource:
the callback lives in this process, and Windows removes the registration when
the process ends, whichever way it ends - Task Manager, `TerminateProcess`, a
crash. Nothing survives to change how anyone else's window behaves, and no
other application was ever modified in the first place, which is the deeper
reason: this feature never wrote anything into another process.

That guarantee is the system's, and this program does not lean on it. The hook
is taken out on every exit we can still run code on:

- the ordinary exit path,
- **`WM_ENDSESSION`**, so logging off or shutting down releases it while there
  is still a message loop to do it in,
- an **unhandled exception**, through a top-level filter installed with the
  first hook and chaining to whatever filter was there before, so the crash
  still crashes and still gets reported.

None of these is load-bearing. They exist because relying on a cleanup you
never perform is how you discover it was not doing what you assumed.

### D2. A setting, defaulted on

`[etc] caption_menu`, default `1`, and a checked entry in the `O` menu. This is
the first thing hgfloater has ever done outside its own windows, so it must be
switchable off from inside the program rather than only by not running it. It
defaults on because a feature nobody can find is a feature nobody has.

### D3. The menu is the one that already exists

The same entries the task icon's context menu offers - **Move to (0, 0)** and
the size presets - built from the same preset table. There is no second list of
sizes and no second idea of what the presets are; a menu that could disagree
with the taskbox about what "4:3 1" means would be worse than no menu.

### D4. The hook posts, it does not act

The hook procedure captures the target window and posts a message to hgfloater's
own window. Everything else - building the menu, tracking it, moving the target
- happens on the normal message loop.

A hook that opened a menu inline would be a hook that runs a modal loop inside
the mouse's input path, which is the way to hang the desktop.

### D5. Swallow the click, but only when the menu will appear

The hook returns non-zero (eat the message) only when it has decided this was a
right press on a maximize button and it has posted the message. Every other
event is passed on untouched.

Right-clicking a caption button does nothing at all in Windows, so nothing is
being taken away when we do claim it.

*Amended by D5b: it is the press and its release that are claimed together, and
the message is posted from the release.*

### D6. A watchdog, because being dropped is silent

The risk in the section above is not theoretical and it has no error to report:
Windows stops calling a hook that has been too slow, the handle stays valid,
`UnhookWindowsHookEx` would still succeed, and the only symptom is that nothing
happens any more. There is no API that answers "am I still hooked".

So it is inferred, every 30 seconds, from evidence we can get for free:

- The hook increments a counter on every callback. One increment is the
  cheapest thing that can be done per event, and it is the only work done for
  the events this feature does not care about.
- The watchdog compares the pointer position with the previous check. **If the
  pointer moved and the counter did not, the hook was dropped**, because mouse
  movement is mouse events and a live hook is called for them.

That direction is what makes it safe: it has **no false positives**. If the
pointer has not moved, nothing is concluded and nothing is done - silence is
not evidence of death, and re-installing on a hunch would churn the input path
for no reason. A cursor moved by `SetCursorPos` rather than by a mouse could
cost one needless re-install; re-installing is cheap and harmless, and a
feature that has silently stopped working is neither.

It runs on the floater's clock rather than the taskbox's refresh, because the
taskbox's only ticks while the taskbox is visible and the taskbox is hidden most
of the time. The interval lives inside the watchdog, so calling it from a second
place later can only ever be harmless.

### D5b. The press arms, the release opens

The first version posted the message from the button **press** and swallowed
only that. Both halves of that were wrong, and the second one is what made the
feature look broken:

- The menu appeared while the right button was still physically down, directly
  under the cursor. The release that came a moment later went into the menu that
  had just opened: sometimes it only dismissed it, which reads as a menu that
  flashes and vanishes; sometimes it selected the entry under the cursor, which
  is the top one - **Move to (0, 0)** - and the window jumped to the corner
  before the menu was even read.
- The release was never swallowed, so the target application got an up with no
  down before it. A window that draws its own title bar is free to act on that.

So the press arms and is swallowed; the release is swallowed with it and posts
the message, but only if it is still on the maximize button that took the press.
Press and drag away is how every button in Windows is cancelled, and this one is
no different. A press that we do not arm clears any arm left over from before,
so a release that never arrives cannot make us eat an unrelated one later.

Taking both halves takes nothing away that D5 did not already account for:
right-clicking a caption button does nothing in Windows. Taking *one* half is
the outcome that gives another application something it never would have seen.

### D5c. One menu, and nothing may hide it

Two things could end this menu before it was used, both of them ours:

- The message that opens it is posted, and a posted message is dispatched inside
  a menu's own modal loop too - so a second right-click on some other maximize
  button would stack a second menu on the first. The hook now passes a press
  through untouched while one of our menus is up, so that click dismisses the
  menu, which is what a click outside a menu should do.
- The menu is owned by the taskbox, and the taskbox collapses itself once the
  cursor has been outside it for half a second - which the cursor is, for the
  whole life of this menu, because it is out on someone else's title bar.
  Hiding a window cancels the menu it owns. The `hg_g_menu_active` flag that
  already holds that timer off for the taskbox's own menus now covers this one.

### D5a. Never swallow a click that produced nothing

The hook eats the right-click only after it has decided *and* successfully
posted the message. If the post fails - a full queue, or the window gone between
the check and the post - the click goes through untouched. Taking an input event
and giving nothing back is the one outcome worse than not having the feature.

## Non-Goals

- No left-click behaviour. Maximize still maximizes.
- No hooking anything but the maximize button. The system menu on the caption,
  the close button, and everything else stay as they are.
- No keyboard hook, ever.
- No per-application rules.

## Risks

- **The hook is a system-wide input path.** Mitigated by D4, D5 and the
  bounded hit-test; and by D2, which means it can be switched off from the menu
  the moment it misbehaves.
- **A `SendMessageTimeout` to a hung window.** Bounded, `SMTO_ABORTIFHUNG`, and
  falls through to the geometric test.
- **Security software.** A global mouse hook is a thing keyloggers do, and
  hgfloater is already an unsigned binary that has drawn a false positive once.
  This may draw another. Worth knowing before it happens rather than after; the
  hook reads only the button and the coordinates, and the README will say so.

## Phases

- **P1 (done, v26.07.31k)** - The hook, the hit-test, the posted message,
  install and teardown.
- **P2 (done, v26.07.31k)** - The menu, from the shared preset table. A
  **Close** entry was added in v26.08.02: Escape and an outside click already
  dismissed it, but this menu opens on a button in someone else's title bar,
  where a stray click is likelier than usual to land on something that acts.
- **P3 (done, v26.07.31k)** - The `O` menu entry, the `config.ini` key, README
  and SPEC.
- **P4 (done, v26.08.03)** - D1's teardown on every exit that can still run
  code, and D5a.
- **P5 (done, v26.08.03c)** - D5b and D5c, after the menu was reported appearing
  at the screen corner and disappearing again. Everything P1 to P4 describes was
  working: the menu was simply opened at the one moment when the next input
  event was certain to close it.

## References

- `SetWindowsHookEx` / `WH_MOUSE_LL` - global, no injection, runs in the
  installing process.
  <https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelmouseproc>
- `WM_NCHITTEST` and `HTMAXBUTTON`.
  <https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest>
- `DwmGetWindowAttribute`, `DWMWA_CAPTION_BUTTON_BOUNDS`.
  <https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetwindowattribute>
- `SendMessageTimeout`, `SMTO_ABORTIFHUNG`.
  <https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessagetimeouta>
- Sizer - the application named in the request, which does this and which this
  is meant to replace. Its behaviour was the specification; none of its source
  was read.
