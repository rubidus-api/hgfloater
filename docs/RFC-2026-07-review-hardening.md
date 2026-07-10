# RFC 2026-07: Full Project Review and Hardening Plan

Status: Draft
Date: 2026-07-09

## Summary

This RFC records the results of a full project review (core sources, widgets, build/test tooling, and documentation) performed after the RFC 2026-06 staged refactor, and proposes a hardening plan derived from it.

The review found no GDI or COM leaks and confirmed that the centralized-cleanup work paid off. The remaining risk is concentrated in a small set of correctness defects (theme brush lifetime, dark/light scheme selection, WM_COPYDATA bounds, stale window-list indices across modal menus, focus stealing on auto-collapse), a structural DPI gap (per-monitor awareness declared but a single startup scale factor used), UI-thread performance debt (COM activation per paint), and documentation drift against the current build.

The project must remain pure C and WinAPI-only. Each phase must preserve current behavior unless the phase explicitly documents a user-visible change.

## Goals

- Fix the confirmed correctness defects before any further structural work.
- Make DPI and multi-monitor behavior match the awareness level the process declares.
- Remove blocking COM and disk work from paint and hide paths.
- Reduce the measured cross-widget duplication (roughly 15-20 percent of widget code) behind small shared helpers.
- Reconcile SPEC, README, TESTS, VERIFICATION, and VER.txt with current behavior.
- Make the build scriptable and keep the version stamp working on current Windows 11.

## Non-Goals

- No C++ rewrite, external dependencies, or new UI framework.
- No redesign of the interaction model.
- No configuration format migration.
- No mandatory CI service dependency; local verification remains the baseline.

## Review Findings: Strengths

These should be preserved by every phase:

- Resource lifetime discipline. All CreateFont/CreateSolidBrush/DC pairs, COM interfaces, and BSTRs are funneled through shared release helpers (`release_font_handle`, `release_brush_handle`, `hg_paint_buffer_begin/end`, `HG_RELEASE_COM`, `HG_HEAP_FREE`). The review found zero GDI or COM leaks across roughly 4,700 lines of paint-heavy code.
- Defensive parsing. The UWP icon pipeline caps manifest reads, clamps icon and bitmap dimensions, and checks every allocation. Config load clamps every value with write-back self-healing. `StringCch*` usage is near-universal.
- Data-driven toolbar. The built-in descriptor table with click/drag/value roles and a compile-time count check lets new buttons (for example the F button) land without touching dispatch logic.
- Strict warning set (`-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wdouble-promotion -Wconversion -Wlogical-op`) applied to all builds including tests.
- Documentation system. File roles are defined in AGENTS.md and machine-checked by `scripts/check-docs.py` via `scripts/project-check.sh`, RFCs are maintained as living documents, and the changelog follows a documented format. The privacy scanner over tracked and untracked files is well matched to the workflow.
- Correct alt-tab filtering (cloaked windows, tool windows, owner semantics), centralized popup menu lifetime with the `hg_g_menu_active` guard, and single-instance mutex with WM_COPYDATA CLI forwarding.

## Review Findings: Defects and Debt

### Correctness (highest severity first)

- `refresh_theme_surfaces` (hg_utils.c) deletes and recreates `hg_g_main_bg_brush` but only re-stamps `GCLP_HBRBACKGROUND` on the taskbox and about windows. The floater, taskbox, and commandbox window classes registered with that brush keep a deleted GDI handle, a use-after-delete on every later `WM_ERASEBKGND`.
- `init_color_scheme` (hg_utils.c) appears inverted: the custom near-black palette is stored in `hg_g_color_scheme_light` while legacy `GetSysColor` values (which do not follow app dark mode) are stored in `hg_g_color_scheme_dark`.
- `handle_copydata_command_line` (hgfloater.c) copies the WM_COPYDATA payload with `StringCchCopyW`, which reads to a NUL. Foreign-process payloads are not guaranteed NUL-terminated within `cbData`; the copy must be bounded by `cbData`.
- Task context menu dispatch (hg_taskbox.c) reads `hg_g_window_items[cur_index]` after `TrackPopupMenuEx` returns, but the refresh timer keeps firing during the modal loop and can reorder or shrink the list; `hg_g_menu_active` guards only the hover-collapse timer. Close/Resize/Move can hit the wrong window. The keyboard path also lacks a bounds check on the focus index.
- `hide_taskbox` (hg_taskbox.c) calls `SetForegroundWindow` on every hide including the 0.5 s auto-collapse, stealing focus from whatever application the user switched to. It also runs `load_shortcuts` (directory scan plus COM icon extraction) on every hide.
- `update_edit_font_size` (hg_taskbox.c) recreates `hg_g_main_font` but never re-sends `WM_SETFONT` to the about window edit, leaving it holding a destroyed HFONT.
- The 100 ms hover-check timer is only killed by its own collapse path; hiding via Esc, the X button, or a floater click leaves it running permanently, and `taskbox_controller_on_destroy` does not kill it.
- Releasing a Ctrl+drag font-resize gesture on the floater also toggles the taskbox because `WM_LBUTTONUP` has no did-a-drag-happen flag.
- Smaller items: `toolbar_controller_on_mouse_leave` leaves `source_index` drag state stale and calls `ReleaseCapture` unconditionally; the monitor deferred-drop timer branch is dead code (its window props are never set); `GetDC` in the monitor edit measurement block is unchecked; extern array sizes in hg_globals.h repeat literals instead of the macros used by the definitions; `HG_MIN_ALPHA` contradicts the 128 clamp in `get_alpha_config`; taskbox `WM_NCHITTEST` resize bands are asymmetric by one pixel.

### DPI and multi-monitor

- The process declares `PROCESS_PER_MONITOR_DPI_AWARE` but computes one global `hg_g_scale_factor` from the primary monitor at startup and never handles `WM_DPICHANGED`. On mixed-DPI setups every widget renders at the wrong size on secondary monitors, and because per-monitor awareness is declared, Windows will not bitmap-stretch as a fallback.
- `ensure_window_visible` (hgfloater.c) snaps a partially off-screen window to absolute (0,0), the primary monitor, instead of clamping into the nearest monitor work area. Negative-coordinate multi-monitor layouts jump to the wrong display.
- `WM_DISPLAYCHANGE` runs `update_monitor_enum` twice (floater and taskbox both handle it) and neither path calls `ensure_window_visible`, so removing a monitor can strand widgets off-screen until the next hotkey press.
- Monitor preview geometry persists under index-based INI keys; after re-enumeration reorders monitors, saved geometry attaches to the wrong display even though runtime matching is by name.

### UI-thread performance

- Toolbar paint and tooltip refresh call `get_system_volume`/`get_system_mute`/`get_system_brightness` per `WM_PAINT`; each volume call performs a full `CoCreateInstance`/`GetDefaultAudioEndpoint`/`Activate` round trip, and brightness performs blocking DDC/CI monitor I/O.
- `refresh_window_list` runs every second while visible and CoCreates `IShellWindows` per Explorer window, giving quadratic COM traffic in the number of Explorer windows.
- The commandbox writes its geometry to the INI on every `WM_MOVE` during a drag; every other widget correctly saves on `WM_EXITSIZEMOVE`.
- Path helpers stack multiple `WCHAR[HG_MAX_PATH]` (64 KB) locals; one icon lookup chain approaches 600 KB of the default 1 MB stack from window-proc depth. Static buffers (`ShortcutItem`, two 1024-entry `WindowItem` arrays) also hold roughly 17 MB of BSS.
- Toolbar paint creates and destroys dozens of solid brushes per frame.

### Structure and duplication

- hg_taskbox.c (2,393 lines) contains at least four separable units: menu construction/dispatch, the toolbar controller, window-list refresh plus Explorer path lookup, and the taskbox window proc with layout math.
- hg_utils.c (2,194 lines) mixes audio COM, DDC/CI brightness, gamma, UWP manifest parsing, toolbar metadata, tooltips, and IME helpers; at least three natural modules.
- Measured duplication is roughly 15-20 percent of widget code: the edit-height measurement block appears five times, the column-snap width formula nine times, the `WM_CTLCOLOR*` handler four times, the IME-suppressing readonly edit subclass three times, the alpha-step logic three times, and the Alt/Ctrl keydown handling blocks three times. The roughly 70-line resize-to-columns computation exists twice in hg_taskbox.c.
- hg_common.h acts as a coupling hub: it declares every module's window procs and defines per-TU `static const WCHAR` class-name arrays.

### Documentation drift

- SPEC.md still lists `HG_NUM_BASIC_ICONS = 9` and omits the F (floater-adjust) button that README and the current build include; the VERIFICATION toolbar-order check also lacks F.
- README (both languages), TESTS.md, and VERIFICATION-2026-06 still describe the pre-change 1 s auto-hide grace period; the current build uses 0.5 s. The Korean toolbar list in README omits the F button, and appendix rows still reference the removed floater context menu.
- VER.txt is pinned at v26.05.31 while the changelog has four unreleased dated entries whose heading style differs from released entries, and there is no `[Unreleased]` section despite the operations rule requiring one.
- HANDOFF.md describes a finished documentation-alignment task rather than the current resume state.

### Build and repository hygiene

- build.bat is interactive-only (menu prompt), so it cannot be scripted; there is no Makefile and no incremental build.
- Version stamping parses `wmic os get localdatetime`; wmic is removed from current Windows 11 builds, so the version string will silently break.
- The `hg_about_text.h` generator is a PowerShell script emitted line-by-line from the batch file via escaped echo statements; it should be a standalone script.
- hgfloater.rc has no VERSIONINFO block, so the executable carries no file-version metadata.
- The operations/test/scripts infrastructure (`scripts/`, `docs/operations/`, `docs/tests/`) and the enabling `.gitignore` change are uncommitted, and local main is well ahead of origin; a fresh clone cannot pass `project-check.sh` because required files are untracked or ignored.
- Orphan build artifacts (an old test executable with no source in history, a stale resource object) sit in the working tree, and the vestigial empty `tests/` directory duplicates `test/`.

## Phase 0: Repository and Release Hygiene

Purpose: remove data-loss risk and dead artifacts before code changes.

Progress:
- 2026-07-09: `[Unreleased]` changelog section added (landed with the Phase 1 entries).
- 2026-07-10: The operations, process-test, and check-script infrastructure is committed along with the enabling `.gitignore` change, and the operations notes document that `project-check.sh` validates the local workspace rather than a fresh clone (several required documents are local-only by design).
- 2026-07-10: Orphan binaries removed (the source-less controlbox test executable in two places, the stale resource object, stale test executables beside their sources) and the vestigial empty `tests/` directory deleted; the working release executable is untouched.
- 2026-07-10: The four dated unreleased changelog sections now live under `[Unreleased]` as dated subsections, matching the documented format.
- 2026-07-10: Backup policy: local-only commits are backed up as git bundles in the private companion directory; pushing to the public remote remains a release-time decision for the maintainer.

Tasks:
- Commit the operations/test/scripts infrastructure and the `.gitignore` change that enables it.
- Delete orphan binaries and the vestigial empty `tests/` directory.
- Decide and document the push/backup policy for local-only commits.
- Add an `[Unreleased]` changelog section and normalize the four dated entries to the documented format.

Exit criteria:
- `project-check.sh` passes from a fresh checkout of tracked files, or the check documents which inputs are local-only by design.
- No orphan executables or stale objects in the working tree.

## Phase 1: Correctness Fixes

Purpose: fix the defects found in review, narrowly and separately.

Progress:
- 2026-07-09: Bounded the WM_COPYDATA command-line copy by cbData.
- 2026-07-09: Theme refresh now re-stamps every class registered with the shared background brush, and lazily created about/commandbox windows heal their class on creation.
- 2026-07-09: Corrected the swapped dark/light scheme assignment and moved the accent flash override to the system (light) scheme.
- 2026-07-09: Task menu commands dispatch on the hwnd captured before the modal loop, the keyboard index path is bounds-checked, and window-list refresh pauses while a menu is active.
- 2026-07-09: Taskbox hide restores foreground only when this process owns it, rescans shortcuts only when the directory changed, and kills the hover-check timer on every hide path and on destroy.
- 2026-07-09: About edit follows main font size changes; releasing a floater Ctrl+drag font-resize no longer toggles the taskbox.
- 2026-07-09: Smaller items landed: pending toolbar drag state cleared on mouse leave with owned-capture release, dead monitor deferred-drop branch removed, monitor edit metric DC guarded, extern array sizes use the shared macros, alpha load clamp matches the runtime minimum, resize hit bands symmetric.
- 2026-07-09: Release build and all smoke tests compile warning-clean with the full MinGW warning set on the cross-build host; runtime verification on the Windows host is still pending.

Tasks:
- Re-stamp `GCLP_HBRBACKGROUND` for every class registered with the theme brush when `refresh_theme_surfaces` recreates it, or stop registering the shared brush as a class background.
- Audit and correct the dark/light scheme assignment and selection in `init_color_scheme`.
- Bound the WM_COPYDATA copy by `cbData` and require a terminator.
- Capture the target HWND (not the list index) before entering any modal menu loop, and bounds-check the keyboard focus index; suppress or snapshot around the refresh timer while a menu is open.
- Restore foreground only when the taskbox or floater owns focus at hide time; move `load_shortcuts` off the hide path (load on show or on directory change).
- Route the about-window edit through the shared font update path.
- Kill the hover-check timer on every hide path and in `taskbox_controller_on_destroy`.
- Add a drag-occurred flag so Ctrl+drag release does not toggle the taskbox.
- Fix the smaller items: stale drag `source_index`, unconditional `ReleaseCapture`, dead deferred-drop branch, unchecked `GetDC`, extern size literals, `HG_MIN_ALPHA` contradiction, hit-test asymmetry.

Exit criteria:
- Each fix lands as a separate narrow commit with a changelog entry.
- Theme toggle, menu-driven close/resize/move, auto-collapse focus behavior, and font resize gestures verified per the manual checklist.

## Phase 2: DPI and Multi-Monitor Correctness

Purpose: make behavior match the declared per-monitor DPI awareness.

Progress:
- 2026-07-09: `ensure_window_visible` clamps into the nearest monitor work area instead of snapping to absolute (0,0).
- 2026-07-09: Display changes are handled once, by the floater, which re-enumerates monitors and immediately recovers the floater, taskbox, and command box into visible work areas.
- 2026-07-09: Monitor preview geometry persists under sanitized display device names, with the old index keys read as migration defaults.
- 2026-07-09: `WM_DPICHANGED` is handled by every top-level widget: the co-located floater/taskbox pair updates the process scale, adopts the suggested rect, and rebuilds scale-dependent fonts and layout; command box, about, and monitor previews adopt the suggested rect.
- 2026-07-09: Startup scale derives from the monitor containing the loaded floater position via `GetDpiForMonitor` instead of always the primary monitor.
- 2026-07-09: Release build compiles warning-clean on the cross-build host; mixed-DPI runtime verification on the Windows host is pending.
- Remaining staged work: independent per-window scaling for command box, about, and monitor previews when they sit on a different-DPI monitor than the floater.

Tasks:
- Handle `WM_DPICHANGED` in every top-level widget: adopt the suggested rect and recompute scaled metrics from the new DPI.
- Replace the single startup scale factor with per-window (or at minimum per-monitor) scaling used by `SC()` call sites, staged widget by widget.
- Clamp `ensure_window_visible` into the nearest monitor work area instead of snapping to (0,0).
- Call the visibility recovery path from `WM_DISPLAYCHANGE`, and deduplicate the double enumeration.
- Key monitor preview geometry by device name instead of index.

Exit criteria:
- Widgets render at correct size after moving between monitors of different DPI and after a DPI change, verified on the Windows runtime host.
- Removing a monitor leaves all widgets reachable without a hotkey press.

## Phase 3: UI-Thread Performance

Purpose: remove blocking COM and disk work from paint and hide paths.

Progress:
- 2026-07-09: Volume and mute calls share one cached `IAudioEndpointVolume`; a failed call drops the cache and retries once, and opening the audio menu re-acquires it.
- 2026-07-09: Brightness reads are cache-only; startup, the taskbox refresh timer (every 5 s while visible), and the setter keep the DDC/CI-backed cache current.
- 2026-07-09: The window-list refresh acquires `IShellWindows` once per pass instead of per Explorer window.
- 2026-07-09: Command box geometry persists on `WM_EXITSIZEMOVE` (and after keyboard moves) instead of every `WM_MOVE`.
- 2026-07-09: Toolbar and taskbox painting use a color-keyed solid-brush cache flushed on theme change and shutdown.
- 2026-07-09: `ShortcutItem` display names shrank to `HG_MAX_STR` and the deep UWP icon path buffers moved to static scratch, cutting BSS by several MB and the worst window-proc stack chain by roughly 600 KB.
- 2026-07-09: Release build compiles warning-clean on the cross-build host; runtime verification on the Windows host is pending.

Tasks:
- Cache audio state: keep one `IAudioEndpointVolume` (re-acquired on device change) or cached values refreshed by a timer or `RegisterControlChangeNotify`, so paint and tooltip paths never activate COM.
- Cache or rate-limit brightness reads; never perform DDC/CI I/O inside `WM_PAINT`.
- Acquire `IShellWindows` once per refresh pass instead of per Explorer window.
- Save commandbox geometry on `WM_EXITSIZEMOVE` like the other widgets.
- Reduce per-paint brush churn by caching the small set of state-dependent brushes.
- Convert the deep `WCHAR[HG_MAX_PATH]` local chains to heap or static scratch buffers, and right-size `ShortcutItem` name storage.

Exit criteria:
- No `CoCreateInstance`, device activation, or DDC/CI I/O inside any paint or tooltip path.
- Dragging the commandbox produces no INI writes until the drag ends.

## Phase 4: Duplication and Module Boundaries

Purpose: continue the RFC 2026-06 direction with the review's measured targets.

Progress:
- 2026-07-10: Added shared helpers and converted every duplicate site: `hg_measure_edit_height` (five copies), `hg_snap_width_for_cols` (ten copies), `hg_on_ctlcolor_edit` (four copies), `hg_step_alpha_value` (three copies), and `hg_readonly_edit_common` for the taskbox/monitor/about edit subclasses.
- 2026-07-10: Unified the twin height-to-columns computations behind `taskbox_cols_from_height`.
- 2026-07-10: The toolbar window class registers through the shared class table (with per-spec style and cursor) and unregisters with the others.
- Extern array sizing with shared macros landed earlier under Phase 1.
- 2026-07-10: Release build compiles warning-clean on the cross-build host; runtime verification on the Windows host is pending.
- 2026-07-10: Split hg_taskbox.c (2,362 lines) into four translation units: hg_taskbox.c (window proc, layout, mutators; 1,057), hg_toolbar.c (773), hg_taskbox_menus.c (354), and hg_window_list.c (184), with cross-unit state in widgets/hg_taskbox_internal.h and the build source list updated. The split compiles warning-clean on the cross-build host.
- Remaining: split hg_utils.c into system-facing modules in the later pass this phase already defers.

Tasks:
- Add shared helpers for the repeated blocks: edit-height measurement, column-snap width and apply-columns, readonly edit subclass (IME, Esc, move-parent, wheel-forward via `dwRefData`), `WM_CTLCOLOR` handling, and alpha stepping with persistence.
- Unify the twin resize-to-columns computations in hg_taskbox.c behind one helper.
- Split hg_taskbox.c along its natural seams (menus, toolbar controller, window-list refresh, window proc) once the helpers are in place.
- Split hg_utils.c into system-facing modules (audio, display/brightness, shell/UWP icons) as a later, separate pass.
- Register the toolbar window class in the shared class table instead of inside `WM_CREATE`.
- Replace extern array-size literals in hg_globals.h with the shared macros.

Exit criteria:
- Each duplicated block above has exactly one implementation.
- No file exceeds roughly 1,000 lines except where a window proc makes that impractical, and behavior is unchanged per the manual checklist.

## Phase 5: Documentation Reconciliation

Purpose: make current-truth documents actually current.

Progress:
- 2026-07-10: README (both languages) now describes the 0.5 s grace period, lists the F button in the Korean toolbar section, documents the `[commandbox]` configuration section and the 76-255 alpha range, and no longer references the removed floater context menu; About text regenerated from the updated README.
- 2026-07-10: VERIFICATION toolbar checks include the F button and the explicit 0.5 s grace interval.
- 2026-07-10: Local current-truth documents updated in place: SPEC.md lists ten built-in icons including F, TESTS.md uses the 0.5 s wording, and HANDOFF.md is a real resume packet for this RFC.
- 2026-07-10: VER.txt intentionally stays at the last released version; HANDOFF.md records that a release waits on the Windows runtime smoke pass.

Tasks:
- Add the F button to SPEC.md (count, order, roles) and to the VERIFICATION toolbar checks.
- Update README (both languages), TESTS.md, and VERIFICATION-2026-06 to the 0.5 s auto-hide grace period; add the F button to the Korean toolbar list; remove appendix references to the removed floater context menu.
- Refresh HANDOFF.md to the actual resume state.
- Document the `[commandbox]` config section in README's configuration guide.
- Cut a release or explicitly record why VER.txt intentionally lags, so the changelog and VER.txt agree.

Exit criteria:
- `check-docs.py` passes and a manual cross-read finds no statement contradicting current behavior.

## Phase 6: Build and Verification Infrastructure

Purpose: keep the build working and scriptable, and grow real test coverage.

Progress:
- 2026-07-10: build.bat derives the version date from PowerShell instead of the removed wmic tool.
- 2026-07-10: The about-text generator lives in scripts/gen_about.ps1 (invoked by build.bat) with a byte-compatible scripts/gen_about.py mirror for non-Windows hosts; the fragile echo-assembled inline script is gone.
- 2026-07-10: hgfloater.rc carries a VERSIONINFO block fed by numeric defines plus stringized zero-padded components, with a valid fallback for bare windres invocations; verified with and without defines on the cross-build host.
- 2026-07-10: build.bat accepts debug/release/test arguments for unattended runs: no menu, no auto-start, exit code reflects the result.
- 2026-07-10: Pure calc helpers (alpha clamp, snap width, items-per-row, DPI scale) moved to hg_calc.c/h with no Win32 dependency; test/test_calc.c runs host-native (verified on the Linux dev host and the cross-build host) and under the existing runner.
- 2026-07-10: scripts/build-mingw.sh reproduces the release build, test compilation, and host-native test run on any Linux host with mingw-w64; verified end to end on the cross-build host.
- Remaining: a CI job stays a user decision under the privacy policy, and build.bat's interactive/argument paths still need one confirmation run on the Windows host.

Tasks:
- Replace the wmic date parsing with a supported mechanism before it breaks on current Windows 11.
- Extract the about-text generator into a standalone script (PowerShell or Python) usable from both the batch build and the Linux host.
- Add non-interactive build entry points (arguments or a MinGW-compatible Makefile) while keeping build.bat as the interactive wrapper.
- Add a VERSIONINFO block to hgfloater.rc fed by the same version string.
- Make pure logic (config parsing, string and geometry helpers) host-compilable and grow helper-level tests beyond the toolbar contract test.
- Optionally add cross-compile plus smoke-test tooling on the Linux host, and consider a CI job if it fits the privacy policy.

Exit criteria:
- A scripted, non-interactive release build produces a version-stamped executable on a current Windows 11 toolchain.
- At least the config clamp/normalization helpers run under a host-native test binary.

## Working Rules

- One phase at a time; Phase 1 items may land first regardless of other phase order.
- Commit narrow fixes separately from refactors.
- Update this RFC's phase progress as work lands, following the RFC 2026-06 convention.
- Every behavior-adjacent change re-runs the relevant VERIFICATION checklist section.
- Do not mix unrelated cleanup into behavior fixes.
