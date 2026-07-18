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

    HgBox middle = box(700, 400, 1100, 700);
    expect(hg_calc_relocation(middle, middle, work, &out) == HG_RELOCATE_NORTH,
           "a centered box goes north first");
    expect(box_equals(out, box(700, 0, 1100, 300)), "north keeps the x span and hugs the work top");

    /* Already flush against the top: north would land on itself, so west wins. */
    HgBox topped = box(700, 0, 1100, 300);
    expect(hg_calc_relocation(topped, topped, work, &out) == HG_RELOCATE_WEST,
           "a box already at the top falls through to west");
    expect(box_equals(out, box(0, 0, 400, 300)), "west keeps the y span and hugs the work left");

    /* Flush against the top-left corner: north and west are taken, so south wins. */
    HgBox cornered = box(0, 0, 400, 300);
    expect(hg_calc_relocation(cornered, cornered, work, &out) == HG_RELOCATE_SOUTH,
           "a top-left box falls through to south");
    expect(box_equals(out, box(0, 740, 400, 1040)), "south keeps the x span and hugs the work bottom");

    /* Occupying the whole left column leaves only east open. */
    HgBox tall = box(0, 0, 400, 1040);
    expect(hg_calc_relocation(tall, tall, work, &out) == HG_RELOCATE_EAST,
           "a full-height left box can only go east");
    expect(box_equals(out, box(1520, 0, 1920, 1040)), "east hugs the work right");

    /* A box as large as the work area has nowhere to go. */
    expect(hg_calc_relocation(work, work, work, &out) == HG_RELOCATE_NONE, "a full-screen box stays put");

    /* Overlap is checked against the whole occupied region, not just the box:
     * the floater home sitting north of the taskbox blocks the north move. */
    HgBox taskbox = box(700, 400, 1100, 700);
    HgBox occupied = box(700, 150, 1100, 700); /* taskbox plus a floater parked above it */
    expect(hg_calc_relocation(taskbox, occupied, work, &out) == HG_RELOCATE_WEST,
           "an occupied strip to the north pushes the move west");

    /* Touching edges do not count as overlap: a box exactly one height below the
     * work top still qualifies for the north slot. */
    HgBox snug = box(700, 300, 1100, 600);
    expect(hg_calc_relocation(snug, snug, work, &out) == HG_RELOCATE_NORTH,
           "an exactly adjacent north slot is accepted");
    expect(box_equals(out, box(700, 0, 1100, 300)), "the adjacent north slot sits flush above");

    /* Work areas do not have to start at the origin (secondary monitors, taskbar). */
    HgBox offset_work = box(-1920, 100, 0, 1180);
    HgBox on_second = box(-1000, 600, -600, 900);
    expect(hg_calc_relocation(on_second, on_second, offset_work, &out) == HG_RELOCATE_NORTH,
           "a box on a negative-origin monitor still goes north");
    expect(box_equals(out, box(-1000, 100, -600, 400)), "north honours the monitor work top");

    /* The perpendicular axis is clamped back inside the work area. */
    HgBox hanging = box(1800, 400, 2200, 700);
    expect(hg_calc_relocation(hanging, hanging, work, &out) == HG_RELOCATE_NORTH,
           "an off-screen-wide box still relocates north");
    expect(box_equals(out, box(1520, 0, 1920, 300)), "north clamps the x span inside the work area");

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
