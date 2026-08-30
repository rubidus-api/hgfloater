#ifndef HG_FLOATER_H
#define HG_FLOATER_H

#include "../hg_common.h"

/* Floater Widget Interface */
LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
void update_floater_layout(HWND hwnd);

/* Re-assert the floater's layered opacity and repaint it now.
 *
 * The alpha is a window attribute the compositor holds, not something the
 * paint restores, and the floater is hidden, moved and shown again every time
 * the taskbox opens and closes. When that attribute or the composited surface
 * goes stale - a display change, a DPI change, a resume from sleep - the widget
 * stays on screen looking washed out, and no amount of repainting content fixes
 * it because the content was never the problem. This sets both again. */
void hg_floater_refresh_surface(void);
void update_floater_font_size(int delta);
/* That size outright, for anything that knows what it wants. */
void hg_set_floater_font_size(int size);
void update_floater_alpha(int delta);

#endif /* HG_FLOATER_H */
