#ifndef HG_CONFIG_H
#define HG_CONFIG_H

#include "hg_common.h"

/* Config Functions */
void load_config(const WCHAR *section, int *x, int *y, int *w, int *h, int def_x, int def_y, int def_w, int def_h);
void save_config(const WCHAR *section, int x, int y, int w, int h);
void load_floater_font_config(void);
void save_floater_font_config(void);
void load_taskbox_font_config(void);
void save_taskbox_font_config(void);
void save_hotkey_config(void);
int get_alpha_config(const WCHAR *section, int def_alpha);
void load_font_name_config(void);
void load_hotkey_config(void);
void save_alpha_config(void);
void hg_config_reset_all(HWND hwnd);

#endif /* HG_CONFIG_H */
