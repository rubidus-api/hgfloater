/* Host-native test for the pure calc helpers. The unit is included directly so
 * the single-file test runner (and a plain host gcc) needs no extra link step. */
#include <stdio.h>

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

int main(void)
{
    expect(hg_clamp_alpha(-5) == HG_MIN_ALPHA, "alpha clamps up to the minimum");
    expect(hg_clamp_alpha(HG_MIN_ALPHA) == HG_MIN_ALPHA, "minimum alpha passes through");
    expect(hg_clamp_alpha(204) == 204, "in-range alpha passes through");
    expect(hg_clamp_alpha(300) == HG_MAX_ALPHA, "alpha clamps down to the maximum");

    expect(hg_snap_width_for_cols(0, 32) == hg_snap_width_for_cols(1, 32), "column count floors at one");
    expect(hg_snap_width_for_cols(2, 32) - hg_snap_width_for_cols(1, 32) == 32 + 15,
           "column pitch is icon plus padding at scale 1");

    for (int cols = 1; cols <= 12; cols++) {
        for (int icon = 16; icon <= 128; icon += 8) {
            int width = hg_snap_width_for_cols(cols, icon);
            expect(get_items_per_row(width, icon) == cols, "items_per_row inverts snap width");
        }
    }
    expect(get_items_per_row(0, 32) == 1, "zero width yields one column");
    expect(get_items_per_row(-100, 32) == 1, "negative width yields one column");

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("Passed: calc helpers");
    printf("\n");
    return 0;
}
