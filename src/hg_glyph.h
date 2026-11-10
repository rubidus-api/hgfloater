#ifndef HG_GLYPH_H
#define HG_GLYPH_H

#include <windows.h>

/* The function buttons' icons (R1.9): drawn, not stored, so they are sharp at
 * whatever size the taskbox's icons are. Each is a short list of strokes on a
 * 100 x 100 square, and none of them uses the top-left quadrant - that is
 * where a button's key badge sits (toolbar_draw_badge), so every icon is drawn
 * in the other three, whether or not its button has a key today. */

/* Nonzero when `index` (an HG_TOOL_ICON_*) has an icon. */
BOOL hg_glyph_has(int index);

/* Draws button `index`'s icon in `rc` (the icon square): a hard-edged band of
 * `edge` first, then the icon itself in `fg`, smoothed, inside that band. The
 * band is what keeps it readable over any plate or the desktop, as the outline
 * does for words (draw_outlined_text), and being hard-edged it never blends
 * with the window's colour key. */
void hg_glyph_draw(HDC dc, int index, const RECT *rc, COLORREF fg, COLORREF edge);

/* Frees the smoothing masks kept for the current size. */
void hg_glyph_release(void);

#endif /* HG_GLYPH_H */
