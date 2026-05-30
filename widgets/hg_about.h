#ifndef HG_ABOUT_H
#define HG_ABOUT_H

#include "../hg_common.h"

void show_about_window(void);
LRESULT CALLBACK about_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

#endif /* HG_ABOUT_H */
