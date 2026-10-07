#ifndef HG_CALC_H
#define HG_CALC_H

#include <stddef.h> /* wchar_t, for the status-clock formatter below */

/* Pure scalar and layout math shared by the widgets. This header has no Win32
 * dependency, so host-native test binaries can compile and link the unit
 * directly (see test/test_calc.c). */

#define HG_MIN_ALPHA ((int)(255 * (100 - 70) / 100))
#define HG_MAX_ALPHA 255

extern double hg_g_scale_factor;

/* DPI scaling for design-time pixel values (rounds away from zero). */
static inline int hg_calc_scale_by(double scale, int x)
{
    return (int)(x * scale + (x >= 0 ? 0.5 : -0.5));
}

static inline int hg_calc_scale(int x)
{
    return hg_calc_scale_by(hg_g_scale_factor, x);
}

int hg_clamp_alpha(int alpha);
int hg_snap_width_for_cols(int cols, int icon_size);
int get_items_per_row(int width, int icon_size);

/* Screen rectangle in the RECT layout (left/top inclusive, right/bottom
 * exclusive) without pulling in windows.h, so the placement math stays testable
 * on the host. */
typedef struct HgBox {
    int left;
    int top;
    int right;
    int bottom;
} HgBox;

/* Where a relocation sent the window, in the order the search tries them. */
typedef enum HgRelocateDirection {
    HG_RELOCATE_NONE = 0,
    HG_RELOCATE_NORTH,
    HG_RELOCATE_WEST,
    HG_RELOCATE_SOUTH,
    HG_RELOCATE_EAST
} HgRelocateDirection;

/* Step `target` clear of `occupied`, staying inside `work`. The search starts at
 * `start` - the direction the last step used - and only turns when that one has
 * no room, so repeated clicks walk one way instead of bouncing. Turns go
 * counter-clockwise: north, west, south, east, north. Returns the direction
 * taken, or HG_RELOCATE_NONE when no direction has room. */
HgRelocateDirection hg_calc_relocation(HgBox target, HgBox occupied, HgBox work, HgRelocateDirection start,
                                       HgBox *out);

/* Offset `home` by however far the taskbox travelled from `from` to `to`, kept
 * inside `work`. Collapsing uses it so the floater follows every taskbox move. */
HgBox hg_calc_follow_move(HgBox home, HgBox from, HgBox to, HgBox work);

/* The taskbox grid: which cell holds what.
 *
 * Three kinds of item share it - task icons, shortcut icons, and the function
 * buttons. Tasks fill from the first cell. The buttons fill the last cells,
 * the first button in the very last one, so the row of buttons ends in the
 * bottom-right corner whatever the window's size.
 *
 * Grouped, each kind starts on a row of its own: the tasks' rows, then the
 * shortcuts' rows, then the buttons' rows, each wrapping when it is longer than
 * a row. Not grouped, tasks and buttons share rows and the shortcuts are not on
 * the grid at all (pass 0 for them) - they are in the Run box.
 *
 * Every piece of code that places, paints, hit-tests or walks the grid asks
 * these, so they cannot come to disagree about where an icon is. */
typedef struct HgGrid {
    int cols;
    int rows;
    int cells;          /* rows * cols */
    int tasks;
    int shortcuts;      /* on the grid; 0 when they are not */
    int buttons;
    int shortcut_first; /* the first shortcut's cell */
    int button_first;   /* cells - buttons: the last button's cell */
} HgGrid;

enum { HG_GRID_TASK = 0, HG_GRID_BUTTON = 1 };
enum { HG_GRID_LEFT = 0, HG_GRID_RIGHT, HG_GRID_UP, HG_GRID_DOWN };

/* The rows that many items need at that many columns. Never less than 1. */
int hg_calc_grid_rows(int cols, int tasks, int shortcuts, int buttons, int grouped);
/* The fewest columns at which they fit in target_rows rows - or, when no width
 * gets them into so few, the width at which they take the fewest rows. */
int hg_calc_grid_cols_for_rows(int target_rows, int tasks, int shortcuts, int buttons, int grouped);
/* The grid at that many columns, at least visible_rows tall. */
void hg_calc_grid(int cols, int visible_rows, int tasks, int shortcuts, int buttons, int grouped, HgGrid *out);
/* An item's cell, or -1. type is HG_GRID_TASK with the task's index, or
 * HG_GRID_BUTTON with a button's index - and, past the last button, a
 * shortcut's (buttons + n), which is how a shortcut has always been numbered. */
int hg_calc_grid_cell(const HgGrid *grid, int type, int index);
/* What a cell holds. 0 when it is empty or out of range. */
int hg_calc_grid_item(const HgGrid *grid, int cell, int *out_type, int *out_index);
/* The cell one arrow key away. Left and Right wrap to the neighbouring row;
 * a step that lands on an empty cell goes on to the next item in that direction,
 * so the gaps between the groups cost no extra key. Always an occupied cell
 * (the one given, if there is nowhere to go). */
int hg_calc_grid_move(const HgGrid *grid, int cell, int direction);

/* Render the taskbox status clock as "2026. 11. 23.(Tue) 13:24" into `out`.
 * `dow` is 0=Sunday. Writes nothing unless the whole string plus terminator
 * fits; returns the character count written (0 when it does not fit). */
int hg_calc_format_clock(wchar_t *out, int cch, int year, int month, int day, int dow, int hour, int minute);

#endif /* HG_CALC_H */
