#include "hg_calc.h"

#define HG_CLOCK_TEXT_CCH 25 /* "2026. 11. 23.(Tue) 13:24" plus terminator */

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

/* Keep going the way the last step went, turning counter-clockwise only when
 * that direction runs out of room, so repeated clicks walk the pair around the
 * screen instead of cycling between two spots. Each candidate takes the smallest
 * step that clears the occupied region - it lands flush against it, not against
 * the far edge of the screen - and keeps the other axis where it was (clamped
 * back on screen). */
HgRelocateDirection hg_calc_relocation(HgBox target, HgBox occupied, HgBox work, HgRelocateDirection start,
                                       HgBox *out)
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
        {HG_RELOCATE_NORTH, kept_x, occupied.top - h},
        {HG_RELOCATE_WEST, occupied.left - w, kept_y},
        {HG_RELOCATE_SOUTH, kept_x, occupied.bottom},
        {HG_RELOCATE_EAST, occupied.right, kept_y},
    };

    unsigned count = sizeof(candidates) / sizeof(candidates[0]);
    unsigned first = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (candidates[i].dir == start) {
            first = i;
            break;
        }
    }

    for (unsigned n = 0; n < count; ++n) {
        unsigned i = (first + n) % count;
        HgBox slot = {candidates[i].x, candidates[i].y, candidates[i].x + w, candidates[i].y + h};
        if (slot.left < work.left || slot.top < work.top || slot.right > work.right || slot.bottom > work.bottom)
            continue;
        if (!hg_boxes_overlap(slot, occupied)) {
            *out = slot;
            return candidates[i].dir;
        }
    }

    return HG_RELOCATE_NONE;
}

/* The floater sits at a fixed offset from the taskbox it expanded into, so when
 * the taskbox is dragged, nudged, or resized while open, the floater travels the
 * same distance instead of snapping back to where it started. */
HgBox hg_calc_follow_move(HgBox home, HgBox from, HgBox to, HgBox work)
{
    int w = home.right - home.left;
    int h = home.bottom - home.top;
    int x = home.left + (to.left - from.left);
    int y = home.top + (to.top - from.top);

    /* Right/bottom first, then left/top: a floater larger than the work area
     * still ends up anchored at its top-left corner instead of off screen. */
    if (x + w > work.right)
        x = work.right - w;
    if (y + h > work.bottom)
        y = work.bottom - h;
    if (x < work.left)
        x = work.left;
    if (y < work.top)
        y = work.top;

    HgBox moved = {x, y, x + w, y + h};
    return moved;
}

/* Hand-rolled so the unit stays free of both Win32 and the CRT: the taskbox
 * clock is the only formatted text here and its shape is fixed. */
static int hg_write_padded(wchar_t *out, int pos, int value, int digits)
{
    for (int place = digits - 1; place >= 0; --place) {
        int divisor = 1;
        for (int i = 0; i < place; ++i)
            divisor *= 10;
        out[pos++] = (wchar_t)(L'0' + (value / divisor) % 10);
    }
    return pos;
}

int hg_calc_format_clock(wchar_t *out, int cch, int year, int month, int day, int dow, int hour, int minute)
{
    static const wchar_t *days[] = {L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"};

    if (!out || cch < HG_CLOCK_TEXT_CCH)
        return 0;
    if (dow < 0 || dow > 6)
        dow = 0;

    int pos = 0;
    pos = hg_write_padded(out, pos, year, 4);
    out[pos++] = L'.';
    out[pos++] = L' ';
    pos = hg_write_padded(out, pos, month, 2);
    out[pos++] = L'.';
    out[pos++] = L' ';
    pos = hg_write_padded(out, pos, day, 2);
    out[pos++] = L'.';
    out[pos++] = L'(';
    for (int i = 0; days[dow][i]; ++i)
        out[pos++] = days[dow][i];
    out[pos++] = L')';
    out[pos++] = L' ';
    pos = hg_write_padded(out, pos, hour, 2);
    out[pos++] = L':';
    pos = hg_write_padded(out, pos, minute, 2);
    out[pos] = L'\0';
    return pos;
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
