# Verification Checklist 2026-06

Status: Active checklist
Last full pass: 2026-07-10 on the Windows host against v26.07.10
Date: 2026-06-24

This checklist records the repeatable verification expected after hgfloater refactors. It is intentionally small and focused on the behavior most likely to regress during the staged refactor.

## Build Gate

Run a release build with the warning set used by `build.bat`:

```text
-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wdouble-promotion -Wconversion -Wlogical-op
```

Expected result:

- Build exits successfully.
- No compiler warnings are emitted.
- `hgfloater.exe` is produced.

## Smoke Tests

Run the tests available under `test/` through `build.bat` option 3 or an equivalent MinGW command.

Expected result:

- Every `test/*.c` file compiles.
- Every produced test executable exits with code 0.
- `test_toolbar_contract` confirms the built-in toolbar count and index order match the runtime descriptor assumptions.
- Console smoke tests are built without GUI subsystem entry-point flags, so `main()` tests link normally.

On Linux cross-build hosts without Wine, verify the compile step with the same warning flags and run the produced executables later on Windows.

## Manual Runtime Checks

Use a Windows 11 desktop with at least one normal application window open. Run the newly built `hgfloater.exe`.

### Floater And Taskbox

- Floater appears with the configured time/date font and alpha.
- Hovering over the floater opens the taskbox at the floater position.
- Moving the pointer briefly outside the taskbox does not immediately hide it during the 0.5 s grace interval.
- Moving the pointer away past the 0.5 s grace interval hides the taskbox and restores the floater.
- `Alt + Mouse Wheel` or `Alt +/-` changes floater/taskbox alpha and the value persists after restart.

### Toolbar

- Built-in toolbar labels appear in the expected order: `R`, move handle, `X`, desktop, `P`, `C`, `A`, `B`, `V`, `F`.
- `F` collapses to the floater in adjust mode: hover-expand stays paused, `Ctrl`/`Alt + Mouse Wheel` tune size/alpha, and a floater click returns to the taskbox.
- Task icons and shortcut icons draw without blank slots when available.
- Toolbar focus follows mouse press and keyboard/context menu actions target the focused item.
- `A`, `B`, and `V` tooltips/focus text show current alpha, brightness, and volume state.
- `V` click toggles mute state and redraws the muted border.

### Menus

- `P` opens the main popup menu.
- Open Shortcuts Folder, Edit Configuration, About, Reset Settings, Arrange Monitors, Lock Screen, and Exit menu items dispatch correctly.
- Select Audio Device lists active render devices.
- Audio Mute menu item reflects and toggles the current mute state.
- Task and shortcut context menus open from the expected toolbar items and close without leaving the menu-active state stuck.

### Monitor Preview

- Arrange Monitors opens a preview for each active monitor.
- Preview windows draw the remote desktop region and border.
- Cursor crosshair is visible and changes color for mouse button state.
- Disconnecting or disabling a monitor closes stale preview windows on display change.

### Command Box

- `C` toggles the command box window.
- `Ctrl + Space` focuses the command input.
- `Ctrl + Wheel` changes command box font size within bounds.
- `Alt + Wheel` changes command box alpha within bounds.
- `Ctrl + Arrow` resizes and `Alt + Arrow` moves the command box.
- Command box geometry, alpha, and font settings persist after restart.

### Reset And Shutdown

- Reset Settings restores floater, taskbox, command box, hotkey, and monitor defaults.
- Exiting from the menu closes auxiliary windows.
- Restarting after exit does not show missing fonts, missing brushes, stale shortcut icons, or stale task icons.

## Phase-Specific Checks

After resource-lifetime changes:

- Reopen taskbox menus repeatedly and verify no menu stays active after closing.
- Open and close monitor previews repeatedly.
- Reload shortcuts by adding/removing `.lnk` or `.url` files, then reopen the taskbox.
- Change audio device and mute state, then reopen the audio menu.
- Open Explorer-backed task items and verify their path display still resolves.

After toolbar/model changes:

- Verify every built-in toolbar item still responds to click, wheel, tooltip, and focus behavior as applicable.
- Verify shortcut items remain separate from built-in toolbar items.

After config/persistence changes:

- Change one setting at a time, exit, restart, and confirm only the intended setting changed.
