#ifndef HG_CALC_H
#define HG_CALC_H

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

#endif /* HG_CALC_H */
