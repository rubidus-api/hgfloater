#include "../hg_common.h"
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

    failed |= expect_int("HG_NUM_BASIC_ICONS", HG_NUM_BASIC_ICONS, 11);
    failed |= expect_int("HG_TOOL_ICON_RESIZE", HG_TOOL_ICON_RESIZE, 0);
    failed |= expect_int("HG_TOOL_ICON_MOVE", HG_TOOL_ICON_MOVE, 1);
    failed |= expect_int("HG_TOOL_ICON_CLOSE", HG_TOOL_ICON_CLOSE, 2);
    failed |= expect_int("HG_TOOL_ICON_DESKTOP", HG_TOOL_ICON_DESKTOP, 3);
    failed |= expect_int("HG_TOOL_ICON_MENU", HG_TOOL_ICON_MENU, 4);
    failed |= expect_int("HG_TOOL_ICON_COMMAND", HG_TOOL_ICON_COMMAND, 5);
    failed |= expect_int("HG_TOOL_ICON_ALPHA", HG_TOOL_ICON_ALPHA, 6);
    failed |= expect_int("HG_TOOL_ICON_BRIGHTNESS", HG_TOOL_ICON_BRIGHTNESS, 7);
    failed |= expect_int("HG_TOOL_ICON_VOLUME", HG_TOOL_ICON_VOLUME, 8);
    failed |= expect_int("HG_TOOL_ICON_FLOATER", HG_TOOL_ICON_FLOATER, 9);
    failed |= expect_int("HG_TOOL_ICON_PIN", HG_TOOL_ICON_PIN, 10);

    int seen[HG_NUM_BASIC_ICONS] = {0};
    const int icons[] = {HG_TOOL_ICON_RESIZE,  HG_TOOL_ICON_MOVE,       HG_TOOL_ICON_CLOSE,
                         HG_TOOL_ICON_DESKTOP, HG_TOOL_ICON_MENU,       HG_TOOL_ICON_COMMAND,
                         HG_TOOL_ICON_ALPHA,   HG_TOOL_ICON_BRIGHTNESS, HG_TOOL_ICON_VOLUME,
                         HG_TOOL_ICON_FLOATER, HG_TOOL_ICON_PIN};
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

    if (failed) {
        printf("toolbar contract failed\n");
        return 1;
    }

    printf("toolbar contract passed\n");
    return 0;
}
