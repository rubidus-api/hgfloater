#ifndef HG_COMMANDBOX_H
#define HG_COMMANDBOX_H

#include "../hg_common.h"

LRESULT CALLBACK commandbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
void commandbox_focus_input(void);
/* Empty the transcript. The history is a separate thing and is left alone. */
void commandbox_clear(void);
void show_commandbox_window(void);
void load_commandbox_font(void);
/* The fixed-pitch size in points, and setting it outright. The globals hold
 * scaled pixels, which is not what a font dialog hands back. */
int hg_commandbox_font_point_size(void);
void hg_commandbox_set_font_point_size(int points);
/* The fixed-pitch size in points, and setting it outright. The globals hold
 * scaled pixels, which is not what a font dialog hands back. */
int hg_commandbox_font_point_size(void);
void hg_commandbox_set_font_point_size(int points);

#endif /* HG_COMMANDBOX_H */
