# Operations

Project workflow and automation notes for `hgfloater`.

This directory captures repeatable working procedures. It does not replace
`SPEC.md`, `REQUIREMENTS.md`, `TESTS.md`, `CHANGELOG.md`, or the C source files.

## Standard Flow

1. Read `AGENTS.md`, `CONTEXT.md`, `SPEC.md`, `REQUIREMENTS.md`, and `docs/tests/test-index.md` as needed.
   For task selection or resume work, also read `BACKLOGS.md` and `HANDOFF.md`.
2. Identify the smallest C source, header, resource, documentation, and test scope.
3. Define the TDD or verification path before behavior changes.
4. Run the narrowest relevant build or smoke test first.
5. Run `scripts/project-check.sh` before reporting or committing.

## Automation Scripts

- Put repeated mechanical work under `scripts/` instead of re-explaining it in every AI session.
- Scripts should be deterministic, narrow, and cheap to run.
- Linux x86_64 is the baseline for local automation; Windows 11 is the product target.
- If a required program is missing, report the exact missing tool and stop instead of improvising.
- Use scripts to save tokens for mechanical checks, documentation policy checks, and bootstrap validation.

## Document Update Rules

- Update `README.md` for public install, usage, visible behavior, or release-facing changes.
- Update `CHANGELOG.md` for public release history.
- Update `SPEC.md` for accepted behavior, UI, keybinding, toolbar, configuration, or workflow-contract changes.
- Update `REQUIREMENTS.md` for accepted requirement, constraint, or success-criterion changes.
- Update `docs/tests/test-index.md` when a process/TDD check, detail file, command, or status changes.
- Update `TESTS.md` for active local verification notes and manual/runtime plans.
- Keep local handoff and working state in ignored files. Use `BACKLOGS.md` for the compact current queue, `HANDOFF.md` for the current resume packet, and `TODO.md` only as legacy detailed backlog input.

## Changelog Rules

- Follow Keep a Changelog 2.0.0 style unless the user asks to refresh from `https://keepachangelog.com/`.
- Use `# Changelog`, a short preamble, and `## [Unreleased]` at the top when maintaining release notes.
- Group changes under `Added`, `Changed`, `Deprecated`, `Removed`, `Fixed`, and `Security` when applicable.
- Write notable user/developer impact, not raw commit logs.

## Local-Only Inputs

- Several files required by `scripts/check-docs.py` are intentionally gitignored
  (for example `SPEC.md`, `REQUIREMENTS.md`, `TESTS.md`, `BACKLOGS.md`,
  `HANDOFF.md`): they are local current-truth working documents.
- `scripts/project-check.sh` therefore validates the full local workspace, not a
  fresh clone. A fresh clone fails the required-file check by design until the
  local working documents are restored from the maintainer's workspace.
