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

static int hg_grid_rows_of(int count, int cols)
{
    return (count > 0) ? (count + cols - 1) / cols : 0;
}

int hg_calc_grid_rows(int cols, int tasks, int shortcuts, int buttons, int grouped)
{
    if (cols < 1)
        cols = 1;
    if (tasks < 0)
        tasks = 0;
    if (shortcuts < 0)
        shortcuts = 0;
    if (buttons < 0)
        buttons = 0;

    int rows = grouped ? hg_grid_rows_of(tasks, cols) + hg_grid_rows_of(shortcuts, cols) +
                             hg_grid_rows_of(buttons, cols)
                       : hg_grid_rows_of(tasks + buttons, cols);
    return (rows > 0) ? rows : 1;
}

int hg_calc_grid_cols_for_rows(int target_rows, int tasks, int shortcuts, int buttons, int grouped)
{
    if (tasks < 0)
        tasks = 0;
    if (shortcuts < 0)
        shortcuts = 0;
    if (buttons < 0)
        buttons = 0;
    if (target_rows < 1)
        target_rows = 1;

    /* Past the widest group (or, ungrouped, the whole run) more columns change
     * nothing, so that is where the search ends. */
    int widest = grouped ? tasks : tasks + buttons;
    if (grouped && shortcuts > widest)
        widest = shortcuts;
    if (grouped && buttons > widest)
        widest = buttons;
    if (widest < 1)
        widest = 1;

    for (int cols = 1; cols < widest; ++cols) {
        if (hg_calc_grid_rows(cols, tasks, shortcuts, buttons, grouped) <= target_rows)
            return cols;
    }
    return widest;
}

void hg_calc_grid(int cols, int visible_rows, int tasks, int shortcuts, int buttons, int grouped, HgGrid *out)
{
    if (!out)
        return;
    if (cols < 1)
        cols = 1;
    if (tasks < 0)
        tasks = 0;
    if (shortcuts < 0 || !grouped)
        shortcuts = 0;
    if (buttons < 0)
        buttons = 0;

    int rows = hg_calc_grid_rows(cols, tasks, shortcuts, buttons, grouped);
    if (visible_rows > rows)
        rows = visible_rows;

    out->cols = cols;
    out->rows = rows;
    out->cells = rows * cols;
    out->tasks = tasks;
    out->shortcuts = shortcuts;
    out->buttons = buttons;
    out->grouped = grouped ? 1 : 0;
    out->shortcut_first = hg_grid_rows_of(tasks, cols) * cols;
    /* Grouped, the buttons start the first of their own rows; otherwise they
     * end in the last cell. */
    out->button_first = grouped ? (rows - hg_grid_rows_of(buttons, cols)) * cols : out->cells - buttons;
}

int hg_calc_grid_cell(const HgGrid *grid, int type, int index)
{
    if (!grid || index < 0)
        return -1;
    if (type == HG_GRID_TASK)
        return (index < grid->tasks) ? index : -1;
    if (type != HG_GRID_BUTTON)
        return -1;
    if (index < grid->buttons)
        return grid->button_first + (grid->buttons - 1 - index); /* the highest index is leftmost */
    index -= grid->buttons;
    return (index < grid->shortcuts) ? grid->shortcut_first + index : -1;
}

int hg_calc_grid_item(const HgGrid *grid, int cell, int *out_type, int *out_index)
{
    int type = -1;
    int index = -1;

    if (grid && cell >= 0 && cell < grid->cells) {
        if (cell < grid->tasks) {
            type = HG_GRID_TASK;
            index = cell;
        } else if (cell >= grid->button_first && cell < grid->button_first + grid->buttons) {
            type = HG_GRID_BUTTON;
            index = grid->buttons - 1 - (cell - grid->button_first);
        } else if (cell >= grid->shortcut_first && cell < grid->shortcut_first + grid->shortcuts) {
            type = HG_GRID_BUTTON;
            index = grid->buttons + (cell - grid->shortcut_first);
        }
    }
    if (out_type)
        *out_type = type;
    if (out_index)
        *out_index = index;
    return type >= 0;
}

int hg_calc_grid_move(const HgGrid *grid, int cell, int direction)
{
    if (!grid || grid->cells <= 0)
        return cell;
    if (cell < 0)
        cell = 0;
    if (cell >= grid->cells)
        cell = grid->cells - 1;

    /* Left and Right read the grid as one line, so the end of a row leads to
     * the start of the next. Off either end of the grid, or above the first
     * row or below the last, there is nowhere to go. */
    int target = cell;
    switch (direction) {
    case HG_GRID_LEFT:
        target = cell - 1;
        break;
    case HG_GRID_RIGHT:
        target = cell + 1;
        break;
    case HG_GRID_UP:
        target = cell - grid->cols;
        break;
    case HG_GRID_DOWN:
        target = cell + grid->cols;
        break;
    default:
        return cell;
    }
    if (target < 0 || target >= grid->cells)
        return cell;
    if (hg_calc_grid_item(grid, target, NULL, NULL))
        return target;

    /* An empty cell: on to the next item the way the key was heading, and
     * failing that the nearest one behind it. */
    int forward = (target > cell);
    for (int pass = 0; pass < 2; ++pass) {
        if (forward) {
            for (int next = target + 1; next < grid->cells; ++next) {
                if (hg_calc_grid_item(grid, next, NULL, NULL))
                    return next;
            }
        } else {
            for (int next = target - 1; next >= 0; --next) {
                if (hg_calc_grid_item(grid, next, NULL, NULL))
                    return next;
            }
        }
        forward = !forward;
    }
    return cell;
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
