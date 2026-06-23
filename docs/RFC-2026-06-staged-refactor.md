# RFC 2026-06: Staged Refactor Plan

Status: Draft
Date: 2026-06-23

## Summary

This RFC proposes a staged refactor of hgfloater after the modular split. The goal is to reduce global-state coupling, make widget behavior easier to verify, and keep the executable small and dependency-free.

The project must remain pure C and WinAPI-only. Each phase must preserve current behavior unless the phase explicitly documents a user-visible change.

## Goals

- Keep the existing floater, taskbox, monitor, command box, and about window behavior stable.
- Make resource ownership explicit for windows, fonts, brushes, menus, tooltips, icons, monitor snapshots, and COM objects.
- Move repeated toolbar/menu/geometry calculations behind small local helpers.
- Improve build-time verification so warning regressions and simple behavior regressions are caught early.
- Keep each change small enough to review and revert independently.

## Non-Goals

- No C++ rewrite.
- No external dependencies.
- No new UI framework.
- No broad redesign of the user interaction model.
- No configuration format migration unless required by a later approved RFC.

## Current Issues

The current code is functional, but several areas still carry risk:

- Global handles and settings are shared across widgets, making ownership and lifetime easy to blur.
- Some settings have multiple update paths; these can drift unless persistence is centralized.
- Toolbar item behavior is split across painting, hit testing, tooltips, clicks, wheel handling, and keyboard focus logic.
- Menu construction uses repeated Win32 calls inline with command dispatch.
- Runtime behavior is mostly manual-tested; the existing tests are smoke-level only.
- Some historical docs still describe old implementation details in changelog entries, which is fine for history but not useful as current architecture guidance.

## Phase 0: Stabilization Baseline

Purpose: prevent regressions before structural changes.

Tasks:
- Keep release builds warning-clean with the existing MinGW warning set.
- Add narrow smoke tests for pure helper logic where possible.
- Record each fixed defect in the changelog and local task state.
- Avoid broad movement of code in this phase.

Exit criteria:
- Release build succeeds without warnings.
- Worktree diff is limited to narrowly justified fixes and documentation.

## Phase 1: Configuration and Persistence Boundary

Purpose: make runtime setting changes consistently persist.

Progress:
- 2026-06-23: Floater/taskbox alpha update helpers now persist `Alt` adjustment paths through the alpha config save path.
- 2026-06-23: Floater/taskbox/commandbox geometry writes now go through named geometry helpers instead of direct section-string writes at call sites.
- 2026-06-23: Command box alpha writes now go through a named helper and skip redundant writes at clamp boundaries.
- 2026-06-23: Command box font name/size persistence now goes through named helpers, with loaded font sizes normalized to the supported 8-72 range.
- 2026-06-23: Hotkey config load now strips invalid modifier bits, rejects invalid virtual-key values, and routes registration/unregistration through named helpers.
- 2026-06-23: Floater/taskbox font save paths now normalize values before writing, and taskbox font/icon updates skip redundant work at clamp boundaries.

Tasks:
- Centralize alpha, font, geometry, and hotkey persistence helpers.
- Ensure every UI path that mutates a persisted setting uses the same helper.
- Add helper-level tests for clamp and conversion logic where practical.

Exit criteria:
- Floater/taskbox alpha, font, size, and position changes persist consistently.
- Duplicate config-write code is reduced or isolated behind named helpers.

## Phase 2: Toolbar Model Extraction

Purpose: make toolbar behavior table-driven without changing UI.

Progress:
- 2026-06-23: Added a built-in toolbar descriptor table for labels and static focus/tooltip text, with a compile-time descriptor count check against `HG_NUM_BASIC_ICONS`.
- 2026-06-23: Added descriptor value roles for alpha, brightness, and volume so dynamic focus/tooltip text and wheel dispatch share the built-in toolbar metadata.
- 2026-06-23: Added descriptor click and drag roles so built-in button activation and resize/move drag startup are routed through toolbar metadata.

Tasks:
- Define a static descriptor table for built-in toolbar items.
- Use descriptors for label, tooltip, hit test role, click role, wheel role, and focus message.
- Keep shortcut items separate from built-in tool items.

Exit criteria:
- `HG_NUM_BASIC_ICONS` and `HG_TOOL_ICON_*` remain consistent with one descriptor source.
- Adding/removing a toolbar item requires changing one table and command handler only.

## Phase 3: Menu Construction Helpers

Purpose: reduce inline menu construction and command-routing risk.

Progress:
- 2026-06-24: Extracted the taskbox main popup menu, audio submenu, monitor submenu, and selected command forwarding into named helpers while preserving command IDs.
- 2026-06-24: Extracted task and shortcut context menu creation plus selected command dispatch into helpers, and reused the audio submenu builder for the volume context menu.
- 2026-06-24: Extracted floater monitor, audio device, and fixed-volume command handling into named dispatch helpers.
- 2026-06-24: Extracted taskbox forwarded floater commands and taskbox font commands into named dispatch helpers.

Tasks:
- Extract main popup menu construction from taskbox activation.
- Extract audio and monitor submenu builders.
- Route selected command IDs through a single command dispatch function.
- Preserve current command IDs.

Exit criteria:
- Main menu contents are unchanged.
- Menu ownership is clear: every created menu is either attached to a parent menu or destroyed on failure.

## Phase 4: Widget Context Boundaries

Purpose: reduce broad global access over time.

Progress:
- 2026-06-24: Replaced separate toolbar callback static variables with a single `ToolbarControllerState` context instance passed directly to controller helpers.
- 2026-06-24: Moved taskbox reorder drag state into a toolbar-local `HgTaskboxDragState` context and removed the taskbox-only drag globals.

Tasks:
- Introduce small context structs only where they remove real ambiguity.
- Start with toolbar hover/press/drag state and taskbox layout state.
- Keep process-wide resources global when they are truly singletons.

Exit criteria:
- Widget-local transient state is not stored as file-scope statics unless required by Win32 callbacks.
- Global declarations shrink or become better grouped by ownership.

## Phase 5: Resource Lifetime Audit

Purpose: make every owned Win32 object auditable.

Tasks:
- Document owner and destroy site for GDI objects, menus, image lists, icons, timers, hooks, and COM interfaces.
- Add helper cleanup functions where repeated release sequences exist.
- Re-run display/audio/monitor code paths after every lifetime change.

Exit criteria:
- No obvious leaked menu, GDI, or COM ownership paths in reviewed code.
- Shutdown and reset paths remain centralized.

## Phase 6: Verification Expansion

Purpose: make future refactors safer.

Tasks:
- Add lightweight C tests for deterministic helpers.
- Add build scripts or documented commands for warning-clean release verification.
- Where runtime automation is not practical, maintain a manual verification checklist focused on hover, menu, audio, monitor, command box, and reset behavior.

Exit criteria:
- Every refactor phase has a narrow automated or manual verification record.
- The manual checklist is specific enough for another maintainer to repeat.

## Working Rules

- One phase at a time.
- Commit narrow fixes separately from refactors.
- Update the RFC if the plan changes.
- Update local task state after each step so another maintainer can resume without guessing.
- Do not mix unrelated cleanup into behavior fixes.
