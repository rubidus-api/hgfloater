#include "hg_calc.h"

#define SC(x) hg_calc_scale(x)

int hg_clamp_alpha(int alpha)
{
    if (alpha < HG_MIN_ALPHA)
        return HG_MIN_ALPHA;
    if (alpha > HG_MAX_ALPHA)
        return HG_MAX_ALPHA;
    return alpha;
}

/* One source for the taskbox column-snap width; the formula was previously
 * repeated at every resize, keyboard, and layout site. */
int hg_snap_width_for_cols(int cols, int icon_size)
{
    if (cols < 1)
        cols = 1;
    return (cols - 1) * (icon_size + SC(15)) + icon_size + SC(20);
}

static int hg_clamp_int(int value, int low, int high)
{
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

static int hg_boxes_overlap(HgBox a, HgBox b)
{
    /* Shared edges do not count: the boxes must share actual area. */
    return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
}

/* Pick the first cardinal slot, north then west then south then east, that holds
 * the target's size fully inside the work area without touching the occupied
 * region. Each candidate hugs the work-area edge in that direction and keeps the
 * other axis where it was (clamped back on screen), so the window clears out as
 * far as it can instead of merely stepping aside. */
HgRelocateDirection hg_calc_relocation(HgBox target, HgBox occupied, HgBox work, HgBox *out)
{
    if (!out)
        return HG_RELOCATE_NONE;

    int w = target.right - target.left;
    int h = target.bottom - target.top;
    int work_w = work.right - work.left;
    int work_h = work.bottom - work.top;
    if (w <= 0 || h <= 0 || w > work_w || h > work_h)
        return HG_RELOCATE_NONE;

    int kept_x = hg_clamp_int(target.left, work.left, work.right - w);
    int kept_y = hg_clamp_int(target.top, work.top, work.bottom - h);

    const struct {
        HgRelocateDirection dir;
        int x;
        int y;
    } candidates[] = {
        {HG_RELOCATE_NORTH, kept_x, work.top},
        {HG_RELOCATE_WEST, work.left, kept_y},
        {HG_RELOCATE_SOUTH, kept_x, work.bottom - h},
        {HG_RELOCATE_EAST, work.right - w, kept_y},
    };

    for (unsigned i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        HgBox slot = {candidates[i].x, candidates[i].y, candidates[i].x + w, candidates[i].y + h};
        if (!hg_boxes_overlap(slot, occupied)) {
            *out = slot;
            return candidates[i].dir;
        }
    }

    return HG_RELOCATE_NONE;
}

int get_items_per_row(int width, int icon_size)
{
    if (width <= 0)
        return 1;
    int denom = icon_size + SC(15);
    if (denom <= 0)
        return 1;
    int n = (width - icon_size - SC(20)) / denom + 1;
    return (n > 0) ? n : 1;
}
