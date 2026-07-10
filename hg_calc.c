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
