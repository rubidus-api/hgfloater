#ifndef HG_MONITOR_H
#define HG_MONITOR_H

#include "../hg_common.h"

LRESULT CALLBACK monitor_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
void toggle_monitor_window(int idx);

#endif /* HG_MONITOR_H */
