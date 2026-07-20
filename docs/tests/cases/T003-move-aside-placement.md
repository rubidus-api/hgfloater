# T003: Move-Aside Placement and Floater Follow

## Requirement

A click on the toolbar's Move handle relocates the floater/taskbox pair to a spot
that does not overlap the area they occupy at that moment. The search keeps the
heading of the previous click and turns counter-clockwise - north, west, south,
east - only when that heading has no room, so repeated clicks walk the pair
around the screen. Collapsing the taskbox afterwards puts the floater at the new
spot: the floater follows every taskbox move, not just this one.

## Verification Method

`hg_calc_relocation` holds the placement math with no Win32 dependency, so
`test/test_relocate.c` compiles and runs on the build host. Run
`sh scripts/build-mingw.sh`, which cross-compiles every test and runs the
host-native ones, or compile the single test directly with the host compiler.

## Assertions

- The search starts at the given heading and keeps it while that direction has
  room; repeated steps in one direction stack up until the work-area edge.
- A blocked heading turns counter-clockwise (north, west, south, east) and wraps
  from east back to north; an unset heading starts north.
- Each candidate takes the smallest step that clears the occupied region in its
  direction, landing flush against it rather than at the work-area edge, and
  keeps the other axis where it was, clamped back inside the work area.
- A candidate that would fall outside the work area is skipped.
- A candidate is clear only when it shares no area with the occupied region; the
  occupied region is the union of the taskbox and the floater's own spot.
- Touching edges do not count as overlap.
- Work areas that do not start at the origin (secondary monitors, taskbar
  reservations) are honored.
- A window as large as the work area, or a request with no output, does not move.
- `hg_calc_follow_move` offsets the floater's pre-expand spot by the distance the
  taskbox travelled while it was open, so an unmoved taskbox leaves the floater
  untouched and a moved one carries it along.
- The followed position is clamped inside the work area, and a floater larger
  than the work area anchors at the work-area origin.
