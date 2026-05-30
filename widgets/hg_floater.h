#ifndef HG_FLOATER_H
#define HG_FLOATER_H

#include "../hg_common.h"

/* Floater Widget Interface */
LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
void update_floater_layout(HWND hwnd);
void update_floater_font_size(int delta);
void update_floater_alpha(int delta);

#endif /* HG_FLOATER_H */
