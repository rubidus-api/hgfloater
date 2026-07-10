# T002: Operating File Role Guidance

## Requirement

Agents can identify the intended role of `SPEC.md`, `REQUIREMENTS.md`, and `docs/tests/test-index.md` without reading broad history.

## Verification Method

Run `scripts/project-check.sh`.

## Assertions

- `AGENTS.md` and `docs/operations/README.md` describe `SPEC.md` as the current behavior and UI contract.
- `AGENTS.md` and `docs/operations/README.md` describe `REQUIREMENTS.md` as current accepted requirements and constraints.
- `AGENTS.md` and `docs/operations/README.md` describe `docs/tests/test-index.md` as the compact process/TDD catalog.
- Detailed test procedures remain under `docs/tests/cases/`, not in the compact index.
