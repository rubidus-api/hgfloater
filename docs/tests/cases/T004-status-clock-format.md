# T004: Status Clock Format

## Requirement

When the taskbox status line has shown a message for ten seconds, it falls back
to the current local time, written as `2026. 11. 23.(Tue) 13:24`, and refreshes
as the minute changes.

## Verification Method

`hg_calc_format_clock` renders the string with no Win32 and no CRT dependency,
so `test/test_relocate.c` exercises it on the build host. Run
`sh scripts/build-mingw.sh`, which cross-compiles every test and runs the
host-native ones.

## Assertions

- The rendered string is exactly `2026. 11. 23.(Tue) 13:24` for that instant,
  24 characters long.
- Month, day, hour, and minute are zero padded to two digits; midnight is
  `00:00`, never `24:00`.
- Weekday abbreviations are `Sun` through `Sat`, indexed the way `SYSTEMTIME`
  reports them (0 = Sunday).
- A buffer too small for the whole string is refused and left untouched, as is a
  null buffer.

## Notes

Timing is not covered by the host test: that the ten-second idle fallback and
the per-minute refresh run off the taskbox's one-second timer, and that showing
the clock does not itself restart the idle countdown, are checked by inspection
and on a Windows desktop.
