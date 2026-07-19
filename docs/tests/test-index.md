# Test Index

Short check catalog for `hgfloater`.

Read this before implementing or changing behavior. Open detailed case files only when relevant.

| ID | Requirement | Purpose | Command | Detail | Status |
|---|---|---|---|---|---|
| T001 | project operations check | Validate script syntax, operation docs, tracked-file privacy patterns, local current-truth files, and git whitespace | `scripts/project-check.sh` | docs/tests/cases/T001-project-operations.md | active |
| T002 | operating file role guidance | Confirm `SPEC.md`, `REQUIREMENTS.md`, and `docs/tests/test-index.md` roles are explicit | `scripts/project-check.sh` | docs/tests/cases/T002-operating-file-roles.md | active |
| T004 | status clock format | Verify the taskbox status line's idle clock renders as `2026. 11. 23.(Tue) 13:24`, zero padded, and refuses short buffers | `sh scripts/build-mingw.sh` (host run of `test_relocate`) | docs/tests/cases/T004-status-clock-format.md | active |
| T003 | move-aside placement | Verify the north/west/south/east search picks the first slot that clears the occupied area and stays inside the work area | `sh scripts/build-mingw.sh` (host run of `test_relocate`) | docs/tests/cases/T003-move-aside-placement.md | active |
