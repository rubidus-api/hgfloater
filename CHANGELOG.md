# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [2026-06-24] - Menu Construction Refactor

### Added
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
- Added a repeatable verification checklist covering warning-clean builds, smoke tests, and focused manual runtime checks.
- Added a toolbar contract smoke test for built-in toolbar count and index invariants.

### Fixed
- Update the stored toolbar focus index on mouse press so keyboard-triggered context menus follow the clicked item.
- Guard monitor preview painting against failed memory DC, bitmap, screen DC, and pen allocation paths.
- Guard Command Box font metric calculation against failed `GetDC()` and remove the temporary dummy window.
- Fix the `build.bat` test runner so `main()`-based console smoke tests link correctly, and silence existing cast smoke-test warnings.

## [2026-06-23] - Stabilization and Refactor Planning

### Added
- Added `docs/RFC-2026-06-staged-refactor.md` with a staged refactor plan covering configuration persistence, toolbar modeling, menu helpers, widget state boundaries, resource lifetime audit, and verification expansion.
- Added named geometry persistence helpers for floater, taskbox, and command box windows as the first Phase 1 configuration-boundary refactor step.
- Added named command box font config helpers and a shared font-name save helper for Phase 1 persistence cleanup.
- Added centralized global hotkey register/unregister helpers.
- Added a built-in toolbar descriptor table for labels and static focus/tooltip text as the first Phase 2 toolbar-model extraction step.
- Added toolbar descriptor value roles for alpha, brightness, and volume dynamic focus/tooltip text and wheel dispatch.
- Added toolbar descriptor click and drag roles for built-in toolbar button dispatch.

### Fixed
- Persist floater/taskbox alpha changes made through `Alt + Mouse Wheel` and `Alt +/-` paths by routing both runtime alpha update helpers through `save_alpha_config()`.
- Route command box alpha persistence through a named helper and skip redundant updates when the value is already clamped.
- Clamp command box font size loaded from `config.ini` to the supported 8-72 range and persist normalized defaults.
- Normalize invalid hotkey modifier bits and invalid virtual-key values loaded from `config.ini` before registration.
- Normalize floater/taskbox font settings before saving and skip redundant taskbox font/icon refreshes at clamp boundaries.
- Avoid redundant layered-window updates when alpha is already clamped at its minimum or maximum.

## [2026-06-22] - Floater to Taskbox Hover UX & UI Adjustments

### Changed
- Converted the interaction model from clicking the Floater to simply **hovering** over it to spawn the Taskbox in place instantly (`WM_MOUSEMOVE` triggering `ShowWindow(SW_HIDE)` for floater and `SW_SHOW` for taskbox). Left-click on the floater still toggles the taskbox.
- Added an automatic `HG_TIMER_HOVER_CHECK` timer in Taskbox to automatically close and return to the Floater UI when the mouse leaves the bounds of the Taskbox, with a ~1 second (10 tick) grace delay that aborts the hide if the cursor briefly leaves and re-enters.
- **Removed the Floater's own right-click context menu** to avoid redundancy. The main context menu (Open Shortcuts Folder, Edit Configuration, About, Reset Settings, Select Audio Device, Arrange Monitors, Lock Screen, Exit) now lives solely on the Taskbox toolbar button.
- **Renamed the Taskbox `M` (Menu) toolbar button to `P` (PopupMenu)**; left-clicking it opens the main context menu.
- Unified the Time and Date font sizes in the Floater by aligning their multiplier ratio to `1:1` (while preserving variables separately for future independent scaling).

### Build / Internal
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
