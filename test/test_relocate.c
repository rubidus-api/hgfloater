/* Host-native test for the pure relocation math behind the toolbar M click.
 * The unit is included directly so a plain host gcc needs no extra link step. */
#include <stdio.h>

#include "../hg_calc.c"

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
    expect(hg_calc_relocation(middle, middle, work, &out) == HG_RELOCATE_NORTH,
           "a centered box goes north first");
    expect(box_equals(out, box(700, 100, 1100, 400)), "north sits directly above the occupied area");

    /* No room for a full height above: north is out, so west takes over, again
     * by the smallest step that clears the area. */
    HgBox high = box(700, 200, 1100, 500);
    expect(hg_calc_relocation(high, high, work, &out) == HG_RELOCATE_WEST,
           "a box too close to the top falls through to west");
    expect(box_equals(out, box(300, 200, 700, 500)), "west sits directly left of the occupied area");

    /* Flush against the top-left corner: north and west do not fit, so south wins. */
    HgBox cornered = box(0, 0, 400, 300);
    expect(hg_calc_relocation(cornered, cornered, work, &out) == HG_RELOCATE_SOUTH,
           "a top-left box falls through to south");
    expect(box_equals(out, box(0, 300, 400, 600)), "south sits directly below the occupied area");

    /* A full-height left column leaves only east open. */
    HgBox tall = box(0, 0, 400, 1040);
    expect(hg_calc_relocation(tall, tall, work, &out) == HG_RELOCATE_EAST,
           "a full-height left box can only go east");
    expect(box_equals(out, box(400, 0, 800, 1040)), "east sits directly right of the occupied area");

    /* A box as large as the work area has nowhere to go. */
    expect(hg_calc_relocation(work, work, work, &out) == HG_RELOCATE_NONE, "a full-screen box stays put");

    /* The step clears the whole occupied region, not just the box itself: a
     * floater parked above the taskbox pushes the north landing further up. */
    HgBox taskbox = box(700, 400, 1100, 700);
    HgBox with_floater = box(700, 350, 1100, 700);
    expect(hg_calc_relocation(taskbox, with_floater, work, &out) == HG_RELOCATE_NORTH,
           "the occupied region drives the north step");
    expect(box_equals(out, box(700, 50, 1100, 350)), "north clears the floater's spot too");

    /* When even that does not fit above, the search moves on. */
    HgBox blocked_north = box(700, 150, 1100, 700);
    expect(hg_calc_relocation(taskbox, blocked_north, work, &out) == HG_RELOCATE_WEST,
           "an occupied strip reaching the top pushes the move west");
    expect(box_equals(out, box(300, 400, 700, 700)), "west clears the whole occupied strip");

    /* Touching edges do not count as overlap: landing exactly against the work
     * top is still a valid north step. */
    HgBox snug = box(700, 300, 1100, 600);
    expect(hg_calc_relocation(snug, snug, work, &out) == HG_RELOCATE_NORTH,
           "an exactly adjacent north slot is accepted");
    expect(box_equals(out, box(700, 0, 1100, 300)), "the adjacent north slot sits flush against the work top");

    /* Work areas do not have to start at the origin (secondary monitors, taskbar). */
    HgBox offset_work = box(-1920, 100, 0, 1180);
    HgBox on_second = box(-1000, 600, -600, 900);
    expect(hg_calc_relocation(on_second, on_second, offset_work, &out) == HG_RELOCATE_NORTH,
           "a box on a negative-origin monitor still goes north");
    expect(box_equals(out, box(-1000, 300, -600, 600)), "north steps up by its own height");

    /* The perpendicular axis is clamped back inside the work area. */
    HgBox hanging = box(1800, 400, 2200, 700);
    expect(hg_calc_relocation(hanging, hanging, work, &out) == HG_RELOCATE_NORTH,
           "an off-screen-wide box still relocates north");
    expect(box_equals(out, box(1520, 100, 1920, 400)), "north clamps the x span inside the work area");

    /* Degenerate inputs must not move anything. */
    expect(hg_calc_relocation(middle, middle, work, NULL) == HG_RELOCATE_NONE, "a null output is refused");
    HgBox empty_work = box(0, 0, 0, 0);
    expect(hg_calc_relocation(middle, middle, empty_work, &out) == HG_RELOCATE_NONE, "an empty work area is refused");

    if (failures) {
        printf("relocation math: %d failure(s)\n", failures);
        return 1;
    }
    printf("relocation math passed\n");
    return 0;
}
