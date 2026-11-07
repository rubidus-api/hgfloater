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

/* The taskbox's top strip is the status line, the whole width (owner's
 * request, 2026-11-07: D R M X, which shared it from 2026-10-14, are back at
 * the end of the grid's buttons). The grid is never narrower than
 * HG_GRID_MIN_COLS: the snap width, the columns a width holds and the columns
 * a row count asks for all floor there. */
#define HG_GRID_MIN_COLS 6
/* Grouped, the grid's buttons are two lines (owner's request, 2026-10-17; the
 * separated default since 2026-11-08 passes no break): the first
 * HG_GRID_BUTTON_BREAK in reading order - A/V Vol Mon Alp, the ones that are
 * read and turned - then a break, then the rest, Dir Set Clip Note Cmd D R M X. */
#define HG_GRID_BUTTON_BREAK 4

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

/* Where the floater starts (owner's request, 2026-11-07): on the monitor under
 * the pointer. `home` is its saved place, `home_work` the work area of the
 * monitor it mostly sits on, `cursor_work` that of the pointer's monitor.
 * `visible` is zero when `home` touches no monitor at all; it is then centred
 * in `cursor_work`. On another monitor it keeps its place in proportion to the
 * free room, so a floater docked to a right edge lands on the right edge.
 * Either way, and on the same monitor, it ends inside `cursor_work` - whatever
 * box the caller passes: the floater passes whole monitors (R1.5), so a
 * place on the taskbar is kept. */
HgBox hg_calc_startup_place(HgBox home, int visible, HgBox home_work, HgBox cursor_work);

/* The taskbox grid: which cell holds what.
 *
 * Three kinds of item share it - task icons, shortcut icons, and the function
 * buttons. Tasks fill from the first cell.
 *
 * Grouped, each kind starts on a row of its own: the tasks' rows, then the
 * shortcuts' rows, then the buttons' rows. Tasks and shortcuts flow left to
 * right from a row's left edge. The buttons take the last rows of the grid,
 * read in their on-screen order - the highest index first - and end at the
 * right edge: the lowest index is in the bottom-right corner, and a line
 * longer than a row starts part way along its first row.
 *
 * Not grouped, everything shares rows: the tasks from the first cell, and one
 * run that ends in the last cell - the shortcuts and then the buttons, the
 * first button in the very last cell, so the run ends in the bottom-right
 * corner.
 *
 * `line` (when above 0 and below the button count) breaks the buttons in two:
 * that many, in reading order, then the rest on a row of their own. Grouped,
 * both lines end at a row's right edge. Not grouped, the second line ends in
 * the bottom-right corner, the cells left of it on its first row stay empty,
 * and the run of shortcuts and the first line ends at the row above it.
 *
 * The first `header` buttons are not in the grid at all: they sit in a strip
 * above it, the first of them flush right and the rest leftwards from it. The
 * taskbox passes none since 2026-11-07; the rule stays for the strip's sake. They
 * have cells of their own past the grid's - `cells + slot` - so the arrows can
 * reach them: Up from the top row goes to the header button above that column
 * (or the nearest one), Down from the header comes back.
 *
 * Every piece of code that places, paints, hit-tests or walks the grid asks
 * these, so they cannot come to disagree about where an icon is. */
typedef struct HgGrid {
    int cols;
    int rows;
    int cells;          /* rows * cols, the header's cells not counted */
    int tasks;
    int shortcuts;
    int buttons;        /* the buttons in the grid, the header's not counted */
    int header;         /* the buttons in the strip above it */
    int grouped;
    int shortcut_first; /* the leftmost shortcut's cell */
    int button_first;   /* the cell of the leftmost button, which is the last by index */
    int line;           /* the buttons on the first line; the rest are on the second */
    int button_second;  /* the cell of the second line's leftmost button */
    int separator_tasks;   /* separated: the cell between the tasks and the run, or -1 */
    int separator_buttons; /* separated: the cell between the shortcuts and the buttons, or -1 */
} HgGrid;

enum { HG_GRID_TASK = 0, HG_GRID_BUTTON = 1 };
/* The `grouped` argument. SEPARATED is the shared layout with a separator cell
 * between neighbouring kinds (owner's request, 2026-11-08): the tasks from the
 * first cell, then a gap, then separator, shortcuts, separator, buttons ending
 * in the last cell. A separator is an empty cell to every query but
 * hg_calc_grid_is_separator: nothing is in it and the arrows step over it. */
enum { HG_GRID_SHARED = 0, HG_GRID_GROUPED = 1, HG_GRID_SEPARATED = 2 };
enum { HG_GRID_LEFT = 0, HG_GRID_RIGHT, HG_GRID_UP, HG_GRID_DOWN };

/* The rows that many items need at that many columns. Never less than 1. */
int hg_calc_grid_rows(int cols, int tasks, int shortcuts, int buttons, int line, int grouped);
/* The fewest columns at which they fit in target_rows rows - or, when no width
 * gets them into so few, the width at which they take the fewest rows. Never
 * fewer than HG_GRID_MIN_COLS. */
int hg_calc_grid_cols_for_rows(int target_rows, int tasks, int shortcuts, int buttons, int line, int grouped);
/* The grid at that many columns, at least visible_rows tall. `buttons` counts
 * only the grid's own buttons; `header` more come before them by index. */
void hg_calc_grid(int cols, int visible_rows, int tasks, int shortcuts, int buttons, int line, int header,
                  int grouped, HgGrid *out);
/* Nonzero when the cell is one of a separated grid's separators. */
int hg_calc_grid_is_separator(const HgGrid *grid, int cell);
/* An item's cell, or -1. type is HG_GRID_TASK with the task's index, or
 * HG_GRID_BUTTON with a button's index - the header's first (cells + index),
 * then the grid's - and, past the last button, a shortcut's
 * (header + buttons + n), which is how a shortcut has always been numbered. */
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

/* HP's BIOS setting "Action Key Mode": which of the firmware's PossibleValues
 * means on (`on` nonzero: Enable or Enabled) or off (Disable or Disabled).
 * Case and surrounding spaces are ignored, nothing else is: the value written
 * back is the firmware's own string, never a spelling guessed here. Returns the
 * index of the one match, or -1 when there is none or more than one. */
int hg_calc_hp_mode_value(const wchar_t *const *values, int count, int on);

#endif /* HG_CALC_H */
