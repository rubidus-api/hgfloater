# T001: Project Operations Check

## Requirement

The project keeps a lightweight, repeatable operations check for documentation policy, script syntax, privacy patterns, current-truth files, and git whitespace.

## Verification Method

Run `scripts/project-check.sh`.

## Assertions

- Required operations and test catalog files exist.
- `SPEC.md` and `REQUIREMENTS.md` exist as local current-truth files.
- Shell scripts parse.
- Python scripts compile.
- Documentation checks pass.
- Git whitespace checks pass.
