#ifndef HG_COMMANDBOX_H
#define HG_COMMANDBOX_H

#include "../hg_common.h"

LRESULT CALLBACK commandbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
void show_commandbox_window(void);
void load_commandbox_font(void);

#endif /* HG_COMMANDBOX_H */
