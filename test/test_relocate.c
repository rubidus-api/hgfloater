/* Host-native test for the pure placement math: the toolbar M click's relocation
 * search and the floater's follow-the-taskbox offset used when collapsing.
 * The unit is included directly so a plain host gcc needs no extra link step. */
#include <stdio.h>
#include <wchar.h>

#include "../src/hg_calc.c"

double hg_g_scale_factor = 1.0;

static int failures = 0;

static void expect(int cond, const char *what)
{
    if (!cond) {
        printf("[FAIL] %s\n", what);
        failures++;
    }
}

static HgBox box(int left, int top, int right, int bottom)
{
    HgBox b = {left, top, right, bottom};
    return b;
}

static int box_equals(HgBox a, HgBox b)
{
    return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
}

int main(void)
{
    /* 1920x1040 work area, a 400x300 box parked in the middle. */
    HgBox work = box(0, 0, 1920, 1040);
    HgBox out;

    /* The move is the smallest one that clears the occupied area: the box lands
     * flush against it, not against the far edge of the screen. */
    HgBox middle = box(700, 400, 1100, 700);
    expect(hg_calc_relocation(middle, middle, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NORTH,
           "a centered box goes north first");
    expect(box_equals(out, box(700, 100, 1100, 400)), "north sits directly above the occupied area");

    /* No room for a full height above: north is out, so west takes over, again
     * by the smallest step that clears the area. */
    HgBox high = box(700, 200, 1100, 500);
    expect(hg_calc_relocation(high, high, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_WEST,
           "a box too close to the top falls through to west");
    expect(box_equals(out, box(300, 200, 700, 500)), "west sits directly left of the occupied area");

    /* Flush against the top-left corner: north and west do not fit, so south wins. */
    HgBox cornered = box(0, 0, 400, 300);
    expect(hg_calc_relocation(cornered, cornered, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_SOUTH,
           "a top-left box falls through to south");
    expect(box_equals(out, box(0, 300, 400, 600)), "south sits directly below the occupied area");

    /* A full-height left column leaves only east open. */
    HgBox tall = box(0, 0, 400, 1040);
    expect(hg_calc_relocation(tall, tall, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_EAST,
           "a full-height left box can only go east");
    expect(box_equals(out, box(400, 0, 800, 1040)), "east sits directly right of the occupied area");

    /* A box as large as the work area has nowhere to go. */
    expect(hg_calc_relocation(work, work, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NONE, "a full-screen box stays put");

    /* The step clears the whole occupied region, not just the box itself: a
     * floater parked above the taskbox pushes the north landing further up. */
    HgBox taskbox = box(700, 400, 1100, 700);
    HgBox with_floater = box(700, 350, 1100, 700);
    expect(hg_calc_relocation(taskbox, with_floater, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NORTH,
           "the occupied region drives the north step");
    expect(box_equals(out, box(700, 50, 1100, 350)), "north clears the floater's spot too");

    /* When even that does not fit above, the search moves on. */
    HgBox blocked_north = box(700, 150, 1100, 700);
    expect(hg_calc_relocation(taskbox, blocked_north, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_WEST,
           "an occupied strip reaching the top pushes the move west");
    expect(box_equals(out, box(300, 400, 700, 700)), "west clears the whole occupied strip");

    /* Touching edges do not count as overlap: landing exactly against the work
     * top is still a valid north step. */
    HgBox snug = box(700, 300, 1100, 600);
    expect(hg_calc_relocation(snug, snug, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NORTH,
           "an exactly adjacent north slot is accepted");
    expect(box_equals(out, box(700, 0, 1100, 300)), "the adjacent north slot sits flush against the work top");

    /* Work areas do not have to start at the origin (secondary monitors, taskbar). */
    HgBox offset_work = box(-1920, 100, 0, 1180);
    HgBox on_second = box(-1000, 600, -600, 900);
    expect(hg_calc_relocation(on_second, on_second, offset_work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NORTH,
           "a box on a negative-origin monitor still goes north");
    expect(box_equals(out, box(-1000, 300, -600, 600)), "north steps up by its own height");

    /* The perpendicular axis is clamped back inside the work area. */
    HgBox hanging = box(1800, 400, 2200, 700);
    expect(hg_calc_relocation(hanging, hanging, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NORTH,
           "an off-screen-wide box still relocates north");
    expect(box_equals(out, box(1520, 100, 1920, 400)), "north clamps the x span inside the work area");

    /* Degenerate inputs must not move anything. */
    expect(hg_calc_relocation(middle, middle, work, HG_RELOCATE_NORTH, NULL) == HG_RELOCATE_NONE, "a null output is refused");
    HgBox empty_work = box(0, 0, 0, 0);
    expect(hg_calc_relocation(middle, middle, empty_work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NONE, "an empty work area is refused");

    /* The click keeps its heading: the search starts at the direction it used
     * last time and only turns when that one is blocked. */
    expect(hg_calc_relocation(middle, middle, work, HG_RELOCATE_WEST, &out) == HG_RELOCATE_WEST,
           "a westward heading keeps going west");
    expect(box_equals(out, box(300, 400, 700, 700)), "the west step clears the occupied area");
    expect(hg_calc_relocation(middle, middle, work, HG_RELOCATE_SOUTH, &out) == HG_RELOCATE_SOUTH,
           "a southward heading keeps going south");
    expect(hg_calc_relocation(middle, middle, work, HG_RELOCATE_EAST, &out) == HG_RELOCATE_EAST,
           "an eastward heading keeps going east");

    /* Blocked, it turns counter-clockwise: north, west, south, east. */
    HgBox at_left = box(0, 400, 400, 700);
    expect(hg_calc_relocation(at_left, at_left, work, HG_RELOCATE_WEST, &out) == HG_RELOCATE_SOUTH,
           "a blocked west turns to south, not back to north");
    expect(box_equals(out, box(0, 700, 400, 1000)), "the turn takes the south step");

    HgBox at_bottom = box(700, 740, 1100, 1040);
    expect(hg_calc_relocation(at_bottom, at_bottom, work, HG_RELOCATE_SOUTH, &out) == HG_RELOCATE_EAST,
           "a blocked south turns to east");

    /* The turn wraps around from east back to north. */
    HgBox at_right = box(1520, 400, 1920, 700);
    expect(hg_calc_relocation(at_right, at_right, work, HG_RELOCATE_EAST, &out) == HG_RELOCATE_NORTH,
           "a blocked east wraps around to north");
    expect(box_equals(out, box(1520, 100, 1920, 400)), "the wrapped turn takes the north step");

    /* A heading of NONE (nothing moved yet) starts at north. */
    expect(hg_calc_relocation(middle, middle, work, HG_RELOCATE_NONE, &out) == HG_RELOCATE_NORTH,
           "an unset heading starts north");

    /* Walking a heading to the wall: repeated north steps stack up, then stop. */
    HgBox walker = box(700, 700, 1100, 1000);
    expect(hg_calc_relocation(walker, walker, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NORTH,
           "the first north step of a walk");
    expect(box_equals(out, box(700, 400, 1100, 700)), "the walk steps up one height");
    walker = out;
    expect(hg_calc_relocation(walker, walker, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_NORTH,
           "the second north step of a walk");
    expect(box_equals(out, box(700, 100, 1100, 400)), "the walk steps up another height");
    walker = out;
    expect(hg_calc_relocation(walker, walker, work, HG_RELOCATE_NORTH, &out) == HG_RELOCATE_WEST,
           "the walk turns west once the top is reached");

    /* The floater follows the taskbox by exactly the distance it travelled. */
    HgBox home = box(800, 500, 880, 555);
    HgBox tb_before = box(700, 400, 1100, 700);
    expect(box_equals(hg_calc_follow_move(home, tb_before, tb_before, work), home),
           "an unmoved taskbox leaves the floater exactly where it was");
    expect(box_equals(hg_calc_follow_move(home, tb_before, box(900, 300, 1300, 600), work),
                      box(1000, 400, 1080, 455)),
           "the floater travels the same delta as the taskbox");
    expect(box_equals(hg_calc_follow_move(home, tb_before, box(700, 100, 1100, 400), work),
                      box(800, 200, 880, 255)),
           "a taskbox moved north drags the floater north by the same step");

    /* The result is kept on screen even when the taskbox is pushed to an edge. */
    expect(box_equals(hg_calc_follow_move(home, tb_before, box(1900, 400, 2300, 700), work),
                      box(1840, 500, 1920, 555)),
           "the follow move clamps against the work right edge");
    expect(box_equals(hg_calc_follow_move(home, tb_before, box(700, -600, 1100, -300), work),
                      box(800, 0, 880, 55)),
           "the follow move clamps against the work top edge");

    /* A floater larger than the work area anchors at its top-left corner. */
    HgBox huge = box(0, 0, 3000, 2000);
    expect(box_equals(hg_calc_follow_move(huge, tb_before, box(900, 400, 1300, 700), work),
                      box(0, 0, 3000, 2000)),
           "an oversized floater anchors at the work origin");

    /* The taskbox status clock has one fixed shape. */
    wchar_t clock[32];
    int written = hg_calc_format_clock(clock, 32, 2026, 11, 23, 2, 13, 24);
    expect(written == 24, "the clock is 24 characters long");
    expect(wcscmp(clock, L"2026. 11. 23.(Tue) 13:24") == 0, "the clock matches the requested format");

    expect(hg_calc_format_clock(clock, 32, 2026, 7, 5, 0, 9, 3) == 24, "single-digit parts still fill the shape");
    expect(wcscmp(clock, L"2026. 07. 05.(Sun) 09:03") == 0, "single-digit parts are zero padded");

    expect(hg_calc_format_clock(clock, 32, 2026, 12, 31, 6, 0, 0) == 24, "midnight new year's eve formats");
    expect(wcscmp(clock, L"2026. 12. 31.(Sat) 00:00") == 0, "midnight is 00:00, not 24:00");

    /* A buffer that cannot hold the whole string is left alone. */
    wchar_t small[8] = {L'k', 0};
    expect(hg_calc_format_clock(small, 8, 2026, 11, 23, 2, 13, 24) == 0, "a short buffer is refused");
    expect(small[0] == L'k', "a refused format writes nothing");
    expect(hg_calc_format_clock(NULL, 32, 2026, 11, 23, 2, 13, 24) == 0, "a null buffer is refused");

    if (failures) {
        printf("relocation math: %d failure(s)\n", failures);
        return 1;
    }
    printf("relocation math passed\n");
    return 0;
}
