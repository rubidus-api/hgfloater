#ifndef HG_AVBOX_H
#define HG_AVBOX_H

#include <windows.h>

/* Multi-stage Win8 flat Audio / Video Control Center popup (D-U7=B, R1.15). */

enum {
    HG_AVBOX_SECTION_AUDIO = 0,
    HG_AVBOX_SECTION_TOPOLOGY = 1,
    HG_AVBOX_SECTION_DISPLAY = 2
};

void hg_avbox_open(int section, int monitor_index);
void hg_avbox_toggle(int section, int monitor_index);
void hg_avbox_close(void);
BOOL hg_avbox_is_open(void);
HWND hg_avbox_window(void);
void hg_avbox_refresh_theme(void);

LRESULT CALLBACK avbox_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

#endif /* HG_AVBOX_H */
