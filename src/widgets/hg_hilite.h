#ifndef HG_HILITE_H
#define HG_HILITE_H

#include "../hg_common.h"

/* An outline drawn around a window on the desktop, to answer "which one is
 * that?" while the pointer rests on its task icon. It never takes the focus and
 * never takes a click - it only says where. */
void hg_hilite_show(HWND target);
void hg_hilite_hide(void);
void hg_hilite_shutdown(void);

#endif /* HG_HILITE_H */
