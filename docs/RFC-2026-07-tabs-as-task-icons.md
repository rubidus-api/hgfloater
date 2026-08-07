# RFC 2026-07: Browser and Explorer Tabs as Task Icons

Status: Implemented (v26.07.31e; the tab-strip scoping in D2a followed in v26.07.31g)
Date: 2026-07-31

## Summary

The taskbox lists windows. A browser with twelve tabs open is one window and
gets one icon, and reaching the eleventh tab from it takes as many clicks as it
would without hgfloater at all. This RFC works out what it would take to give
each tab its own icon, and settles the design before any of it is written.

The short version: it is possible, it is not free, and the cost is of a kind
this program has consistently refused to pay by default. So it ships **off**,
behind a setting, and everything below is arranged around keeping the cost
inside the seconds when it is on.

## Why the existing machinery cannot do it

`refresh_window_list` walks top-level windows with `GetTopWindow` /
`GetWindow(GW_HWNDNEXT)` and keeps the ones `is_alt_tab_window` accepts. That is
the right way to enumerate windows, and **a tab is not a window**:

- Chrome and Edge draw their whole tab strip inside one `Chrome_WidgetWin_1`
  window. There is no HWND per tab, and there has not been since the process
  model changed years ago.
- Windows 11 Explorer tabs live in one `CabinetWClass` window, the same way.

No amount of window enumeration will find them. Something has to ask the
application, and there is exactly one supported way to ask.

## The one supported way: UI Automation

UI Automation is the accessibility layer every one of these applications already
implements, because screen readers need it. It exposes a tree of elements per
window, and a tab is a **`UIA_TabItemControlTypeId`** element with a name.

- **Finding them.** `IUIAutomation::ElementFromHandle` on the window, then
  `FindAll(TreeScope_Descendants, ...)` with a property condition on the control
  type. Returns an `IUIAutomationElementArray`.
- **Reading one.** `get_CurrentName` is the tab's title.
- **Switching to one.** `GetCurrentPattern(UIA_SelectionItemPatternId)` and
  `IUIAutomationSelectionItemPattern::Select`. This is a real API call, not a
  synthesised click - it does not need the window in the foreground and it does
  not move the pointer.

That is the whole mechanism, and all of it is documented. What follows is about
what it costs.

## The cost, stated plainly

Every one of those calls is **cross-process**. UI Automation marshals a request
into the target application, which answers on its own UI thread. Three
consequences, and they are the whole reason for the design below:

1. **It is slow.** A `FindAll` over a browser's descendant tree is tens of
   milliseconds, sometimes more on a busy machine. The taskbox refreshes every
   second; doing this for three browser windows on that timer would spend a
   meaningful fraction of every second inside other processes.
2. **It can block.** If the target application's UI thread is stuck, the call
   waits. On our UI thread that is our window frozen because someone else's is.
   This program has bounded every such wait it has ever taken - WMI, DDC/CI -
   and this is the same hazard with a new name.
3. **It can be wrong.** The tree is what the application chose to publish.
   Names change as pages load; a tab strip may be virtualised so off-screen tabs
   are not in the tree at all.

None of these is a reason not to do it. All of them are reasons not to do it
every second, and not by default.

## Design

### D1. Off by default, one switch, in both places

A `[taskbox] show_tabs` key in `config.ini`, default `0`, and a checked menu
item in the `O` menu that toggles it. The menu item is the discoverable half and
the key is the durable half; they are the same setting and neither is a second
copy of it.

Off means **not a single UI Automation call is made**, and the COM object is
never created. A feature that is off must cost nothing, or "off" is a lie.

### D2. Its own clock, slower than the window list

Tabs are re-enumerated on their own counter - every fifth refresh, so about five
seconds - rather than with the window list every second. Between those passes
the tabs already found are reused, exactly as window icons are.

Five seconds is a judgement, not a measurement: switching tabs is something you
do with the tab strip that is right in front of you, and the taskbox is for the
window you cannot see. Being five seconds stale about a tab title is not a cost
anyone will notice; a taskbox that stutters once a second is.

### D2a. Find the strip by where it is, not by what its container is called

Two attempts, wrong in opposite directions, and the second one is why this
section is worth reading.

**Asking for every `TabItem` below the window** collected Explorer's Home page
sections - Favourites, Recent, Shared. Those are real tab items, of the page
rather than of the window. A tab is an ordinary control and content uses it.

**Then requiring a `UIA_TabControlTypeId` parent** fixed that and broke
Explorer's real tabs, which stopped appearing at all. What a XAML tab strip
publishes as its container is not something we get to decide, and a design that
depends on another application's choice of control type is a design that breaks
when that application changes its mind.

What is reliably true is **where** the strip is. A window's tabs are at the top
of the window, above everything, because being above everything is what makes
them the window's rather than the content's. So the filter is geometric and is
applied to the tab items themselves - within the upper quarter of the window -
and nothing at all is assumed about their parent.

That test is also what lets **no window tab strip** be an answer rather than a
wrong guess: an Explorer window showing only its Home page contributes no tab
items, which is correct, because its Home page's sections are not tabs of the
window.

The items are then ordered by their left edge rather than by tree order, because
the number on an icon has to mean the tab in that position on screen.

### D3. Only windows that can have tabs, and the list is editable

Nothing in the mechanism is specific to browsers - a tabbed application is a
tabbed application, and the same UI Automation tree answers for any of them.
What limits the feature is cost, so the enumeration runs only for windows whose
class is on a list. Everything else is skipped without a call.

Built in: `Chrome_WidgetWin_1` (Chromium, which is Chrome, Edge, Brave, Opera
and every Electron application), `CabinetWClass` (Explorer),
`MozillaWindowClass` (Firefox), `CASCADIA_HOSTING_WINDOW_CLASS` (Windows
Terminal), `Notepad`.

**A list compiled into the program can only ever be out of date**, so
`[taskbox] tab_classes` adds to it from `config.ini`, semicolon-separated, with
no rebuild. That is what turns "we support these five" into "we support what you
tell us about", and it costs one string read at startup.

For that to be usable a reader has to be able to find a window's class, so
`show windows class` prints it. A setting nobody can discover the value for is a
setting nobody can use.

An unlisted application keeps exactly the behaviour it has now, one icon for one
window, which is not a regression. Asking every window on the desktop so that
one unlisted application might work is how the cost in the previous section
becomes the cost of using the program.

### D4. A tab is an item, not a window

`WindowItem` grows two fields: a tab index, and a flag saying the item is a tab
rather than a window. A window with tabs contributes one item per tab instead of
one item for itself; a window without contributes itself, as now.

**Activating a tab item** raises its window and calls `Select` on the tab. Both
halves are needed: `Select` alone switches the tab inside a window that may be
behind three others.

**The tab is addressed by index, not by a held pointer.** Element pointers into
another process go stale when the page navigates or the tab strip rebuilds, and
holding one across a five-second gap is holding a reference to something that
may no longer mean what it did. The element is re-found at the moment of the
click, which costs one call in exchange for never acting on a stale handle.

### D5. Everything that counts items keeps working

`go`, `resize`, `move`, `find windows` and the `Shift`+label activation all
index into the same list, so they inherit tabs for free - but two of them stop
making sense on a tab, because a tab has no window geometry of its own:
`resize` and `move` on a tab item apply to its **window**, and say so in the
line they print. That is the honest behaviour; refusing would be worse, because
the window is genuinely what the reader is pointing at.

### D6. Failure is silence and one icon

Any failure - COM unavailable, the window not answering, no tab elements found -
falls back to the item the window would have contributed anyway. The taskbox
never shows an error, never shows a window twice, and never shows nothing where
a window is.

## Non-Goals

- No synthesised clicks, no key injection, no reading another process's memory.
  `SelectionItemPattern::Select` or nothing.
- No reordering tabs and no opening new ones. **Closing one was a non-goal and
  is no longer** (v26.07.31h): the task menu's Close acted on the window, so a
  right-click on one tab threw away every other tab in it. Once a tab is an item
  in that menu, the menu has to mean what it says, and the way to say it is to
  invoke the tab's own close button - not Ctrl+W, which needs focus and closes
  whichever tab is current rather than the one that was asked for.
- No favicons. The tab's icon would be another cross-process fetch per tab per
  pass, for a picture 16 pixels wide; tab items carry their window's icon and
  their own label.
- No per-application special cases beyond the class-name list in D3.

## Risks

- **A blocked target application blocks us.** The mitigation is D2 and D3 -
  fewer calls, less often, only where they can pay off - and, if it proves
  necessary in use, moving the enumeration off the UI thread entirely. That is
  deliberately not in this first pass: a background thread handing UI Automation
  results to a paint loop is a second concurrency problem, and it should be
  bought only if the first pass shows it is needed.
- **Tab counts change under the reader.** A tab closed between one pass and the
  next leaves an icon that activates the wrong tab. Bounded by D2's five
  seconds, and the re-find in D4 means it selects whatever is at that index now
  rather than acting on a dead handle.
- **The item list gets long.** Twelve tabs across two browsers is twenty-four
  icons where there were two. The taskbox grid already scrolls and reflows, but
  this is the setting's real cost to the reader and is why it is off by default.
- **Virtualised tab strips.** Some applications do not publish off-screen tabs.
  Those tabs will not appear. Nothing can be done from outside the application,
  and it is written down here rather than discovered later.

## Phases

- **P1 (done, v26.07.31e)** - The setting: `[taskbox] show_tabs`, the `O` menu
  item, and the plumbing that reads it.
- **P2 (done, v26.07.31e)** - UI Automation enumeration behind the setting, per
  D2 and D3, with tab items in the window list per D4.
  - **D2a was written after the fact**, from use: the first cut asked the window
    for every tab item and collected Explorer's Home page sections. Fixed in
    v26.07.31g, and the rule is recorded above rather than left in the code.
- **P3 (done, v26.07.31e)** - Activation via `SelectionItemPattern`.
- **P4 (done, v26.07.31e)** - README, SPEC, and the note about virtualised tab
  strips.

## What went wrong on the way

Two failures worth keeping, because both were avoidable:

- **A four-megabyte stack frame.** The expansion declared its output array as a
  local. A `WindowItem` is about 4 KB, so a thousand of them overflowed the
  one-megabyte stack on entry - before the early return for "the feature is off"
  could run. v26.07.31e therefore crashed at startup on every machine whether or
  not tabs were switched on. The lesson is in SPEC: item arrays in that file are
  static, and `gcc -fstack-usage` will find the next one.
- **Asking the wrong question.** See D2a.

## References

- IUIAutomation, `ElementFromHandle`, `FindAll`, `CreatePropertyCondition`.
  <https://learn.microsoft.com/en-us/windows/win32/api/uiautomationclient/nn-uiautomationclient-iuiautomation>
- `UIA_TabItemControlTypeId` - the control type a tab reports.
  <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controltype-ids>
- IUIAutomationSelectionItemPattern::Select - switching to a tab without a
  click.
  <https://learn.microsoft.com/en-us/windows/win32/api/uiautomationclient/nf-uiautomationclient-iuiautomationselectionitempattern-select>
- `docs/RFC-2026-07-brightness-control.md` and
  `docs/RFC-2026-07-temperature.md` - the two earlier cases of this program
  taking a bounded, cached, off-the-paint-path approach to a slow cross-process
  API, whose shape D2 follows.

## D6 - Enumeration moves to a worker thread (2026-08-05)

The synchronous design above eventually produced the very stutter D2 was meant
to avoid, and in a way that took weeks to show: Chromium turns its
accessibility support on the first time a UIA client queries it and leaves it
on for the browser session. The first walks are cheap because the trees are
shallow; an hour later, with a dozen complex pages materialized, the same
`FindAll` costs a hundred milliseconds - and it was running on the UI thread,
on the expand path, which is exactly where the user is looking.

So enumeration now runs on one dedicated worker thread:

- The worker owns its COM (MTA, per UIA client guidance) and its own
  `IUIAutomation` instance; interface pointers never cross the apartment
  boundary. The UI thread keeps a separate instance for the one-shot user
  actions (activate, close a tab), where a brief block on an explicit click is
  acceptable.
- The UI thread files a request batch (the current tab-class windows) and
  returns immediately; the expand path draws from its title cache on every
  pass and never waits. Requests are re-filed on a wall-clock cadence
  (5 seconds), not a pass count, because the completion message triggers a
  pass of its own and counting those would let each answer hasten the next
  question.
- The worker posts `HG_MSG_TABS_READY` to the floater when a batch completes.
  Visible taskbox: one refresh pass folds the answers in. Hidden: the results
  wait in the worker's table, and the next expand picks them up in its own
  pass - so expansion after idle shows tabs instantly, from data at most one
  request old.
- A new tabbed window appears as a single icon for the fraction of a second
  its first answer takes, then fans out. That replaces the old behaviour,
  where the whole dashboard froze for as long as the answer took.
- Shutdown signals the worker and waits briefly; a worker stuck inside a
  cross-process call is left to process teardown rather than terminated
  mid-call, because the tables it might still touch are process-lifetime
  statics.

## D7 - The walk itself made cheaper and rarer (2026-08-06)

D6 took the walk off the UI thread, and the stutter moved instead of dying:
the cost of `FindAll(TreeScope_Descendants)` lands in the *target* - the
browser materializes its accessibility tree to answer, across its renderer
processes - so a batch fired at every browser window in the instant after
expansion still hitched the whole machine. Current Chromium also drops its
accessibility mode after a while without UIA traffic, which is why the burst
is worst right after an idle stretch: the first ask pays for the tree being
rebuilt from nothing. Three changes, independent and stacked:

- **A window is re-asked only when its own title changed.** Switching or
  navigating a tab retitles a browser window, so title change is a cheap and
  honest proxy for "the strip probably changed". A 30-second backstop catches
  what a title does not carry (a background tab quietly opened). After
  expansion, windows the user never touched cost nothing at all.
- **The answer rides back in one call.** A cache request fetches Name and
  BoundingRectangle inside the FindAll itself, with
  `AutomationElementMode_None` so no live per-element reference is created.
  The uncached path cost two cross-process round trips per tab item on top of
  the walk.
- **Batches are staggered and the worker runs below normal priority.** 150 ms
  between windows, so no two browsers rebuild their trees in the same instant;
  the priority keeps our half of the work out of the foreground's way (the
  browser's half runs at its own priority regardless).

The user-visible trade: a tab strip changed without a title change (rare) can
lag up to 30 seconds. Ordinary tab switching and navigation update within the
usual beat.

## D8 - Proposed: tabs behind a hover sub-box, not beside the windows (2026-08-07)

The fan-out has structural problems the maintainer has now named: tab items
crowd the grid, cannot be reordered (their order is the strip's, not the
user's), and keeping their titles fresh is what all of D6/D7/B3's machinery
exists to pay for. The proposal inverts the presentation:

- The taskbox shows **one icon per window**, always - no fan-out. A window
  whose class can have tabs carries a small corner mark so the affordance is
  discoverable.
- **Hovering** that icon (a short settle, ~350 ms) opens a **sub-box**: a
  small popup listing that window's tabs **as titles, not icons** - one row
  per tab, in strip order, each prefixed with its label and a colon
  (`1: Inbox`). Click switches to the tab; right-click closes it; the box
  dismisses when the pointer leaves it and the icon.
- **Labels run 1-9, then a-z, then A-Z** (more than enough for the 24-tab
  cap), and **pressing a label key jumps to that row; `0` jumps to the last
  row** - the same philosophy as the taskbox badges, small keys for small
  distances.
- **The keyboard works without the box stealing focus.** The box opens
  no-activate; while it is visible the toolbar's key handler routes to it:
  Up/Down move the selection, a label key jumps, Enter switches to the
  selected tab, and **Esc closes the box and the keyboard is back in the
  taskbox** exactly where it was.
- **Enumeration happens only then.** The hover posts one request for that one
  window; the sub-box draws the cached titles immediately and folds the fresh
  answer in when HG_MSG_TABS_READY lands. No timer-driven re-asks at all -
  the title gate, backstop and breaker become the sub-box's freshness rules
  rather than a background cadence.

What this retires: the per-second fan-out rebuild, the tab items' icon
sharing, the reorder problem (windows reorder as windows always did), and the
steady-state UIA traffic (zero while nobody hovers). What it keeps: the
worker, the scoped read, `show tabs`, and hg_tabs_activate/close as the
sub-box's verbs. Status: agreed with the maintainer 2026-08-07, next
implementation batch after v0.6.0.
