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

### D2a. The window's tab strip is not the only tab control in the window

Asking for every `TabItem` below the window is the obvious query and it is the
wrong one. Explorer's Home page has a tab strip of its own - Favourites, Recent,
Shared - and those are real tab items that have nothing to do with the window's
tabs. A browser's page content can contain tabs too, for the same reason: a tab
is a normal control and pages use it.

So the query is in two steps. Find the tab **controls**, take the one at the top
of the window, and read that one's children. A window's tab strip is above
everything by definition of what it is; a tab control further down belongs to
what is being displayed, not to the window.

"At the top" is a geometric test - within the upper quarter of the window - and
it is what lets **no window tab strip** be an answer rather than a wrong guess.
An Explorer window showing only its Home page then contributes no tab items,
which is correct: it has one tab, and its Home page's sections are not tabs of
the window.

Children rather than descendants at the second step, for the matching reason: a
tab that contains a control which is itself a tab item is one tab.

### D3. Only windows that can have tabs

The enumeration runs only for windows whose class is one of the known tabbed
shells - `Chrome_WidgetWin_1` (Chrome, Edge, and the rest of the Chromium
family) and `CabinetWClass` (Explorer). Every other window is skipped without a
call.

This is a list of class names and it will be incomplete, and that is the correct
trade: an unknown application keeps exactly the behaviour it has now, one icon
for one window, which is not a regression. Asking every window on the desktop
for its tabs so that one unlisted browser might work is how the cost in the
previous section becomes the cost of using the program.

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
