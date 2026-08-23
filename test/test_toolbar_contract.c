#include "../src/hg_common.h"
#include <stdio.h>

static int expect_int(const char *name, int actual, int expected)
{
    if (actual != expected) {
        printf("%s: expected %d, got %d\n", name, expected, actual);
        return 1;
    }
    return 0;
}

int main(void)
{
    int failed = 0;

    /* The row: nine buttons, in the order they are drawn. */
    failed |= expect_int("HG_NUM_BASIC_ICONS", HG_NUM_BASIC_ICONS, 9);
    failed |= expect_int("HG_TOOL_ICON_RESIZE", HG_TOOL_ICON_RESIZE, 0);
    failed |= expect_int("HG_TOOL_ICON_MOVE", HG_TOOL_ICON_MOVE, 1);
    failed |= expect_int("HG_TOOL_ICON_CLOSE", HG_TOOL_ICON_CLOSE, 2);
    failed |= expect_int("HG_TOOL_ICON_DESKTOP", HG_TOOL_ICON_DESKTOP, 3);
    failed |= expect_int("HG_TOOL_ICON_COMMAND", HG_TOOL_ICON_COMMAND, 4);
    failed |= expect_int("HG_TOOL_ICON_NOTE", HG_TOOL_ICON_NOTE, 5);
    failed |= expect_int("HG_TOOL_ICON_CLIP", HG_TOOL_ICON_CLIP, 6);
    failed |= expect_int("HG_TOOL_ICON_DIR", HG_TOOL_ICON_DIR, 7);
    failed |= expect_int("HG_TOOL_ICON_SETTINGS", HG_TOOL_ICON_SETTINGS, 8);

    int seen[HG_NUM_BASIC_ICONS] = {0};
    const int icons[] = {HG_TOOL_ICON_RESIZE,  HG_TOOL_ICON_MOVE,    HG_TOOL_ICON_CLOSE,
                         HG_TOOL_ICON_DESKTOP, HG_TOOL_ICON_COMMAND, HG_TOOL_ICON_NOTE,
                         HG_TOOL_ICON_CLIP,    HG_TOOL_ICON_DIR,     HG_TOOL_ICON_SETTINGS};
    for (size_t i = 0; i < HG_ARRAYSIZE(icons); ++i) {
        if (icons[i] < 0 || icons[i] >= HG_NUM_BASIC_ICONS) {
            printf("toolbar icon index out of range: %d\n", icons[i]);
            failed = 1;
            continue;
        }
        if (seen[icons[i]]) {
            printf("duplicate toolbar icon index: %d\n", icons[i]);
            failed = 1;
        }
        seen[icons[i]] = 1;
    }
    for (int i = 0; i < HG_NUM_BASIC_ICONS; ++i) {
        if (!seen[i]) {
            printf("no button claims row index %d\n", i);
            failed = 1;
        }
    }

    /* The five that moved into the Se box keep ids, and those ids must stay
     * clear of the row - a loop over the toolbar must not reach them - and
     * clear of each other. */
    const int in_box[] = {HG_TOOL_ICON_ALPHA, HG_TOOL_ICON_BRIGHTNESS, HG_TOOL_ICON_VOLUME, HG_TOOL_ICON_PIN,
                          HG_TOOL_ICON_MENU};
    for (size_t i = 0; i < HG_ARRAYSIZE(in_box); ++i) {
        if (in_box[i] < HG_NUM_BASIC_ICONS) {
            printf("box-only button %d overlaps the row\n", in_box[i]);
            failed = 1;
        }
        for (size_t j = i + 1; j < HG_ARRAYSIZE(in_box); ++j) {
            if (in_box[i] == in_box[j]) {
                printf("duplicate box-only button id: %d\n", in_box[i]);
                failed = 1;
            }
        }
    }

    if (failed) {
        printf("toolbar contract failed\n");
        return 1;
    }

    printf("toolbar contract passed\n");
    return 0;
}
