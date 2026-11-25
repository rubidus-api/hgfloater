#ifndef HG_GLYPH_H
#define HG_GLYPH_H

#include <windows.h>

/* The function buttons' icons (R1.9): drawn, not stored, so they are sharp at
 * whatever size the taskbox's icons are. Each is a short list of strokes on a
 * 100 x 100 square, and none of them uses the top-left quadrant - that is
 * where a button's key badge sits (toolbar_draw_badge), so every icon is drawn
 * in the other three, whether or not its button has a key today. */

/* Extra glyph slot for muted volume state (D-U3=A) and total cache slots. */
#define HG_GLYPH_VOLUME_MUTED 11
#define HG_GLYPH_COUNT 12

/* Nonzero when `index` (an HG_TOOL_ICON_* or HG_GLYPH_*) has an icon. */
BOOL hg_glyph_has(int index);

/* Draws button `index`'s icon in `rc` (the icon square): a hard-edged band of
 * `edge` first (skipped when edge == CLR_NONE for Win8 flat tiles), then the
 * icon itself in `fg`, smoothed via supersampled mask. */
void hg_glyph_draw(HDC dc, int index, const RECT *rc, COLORREF fg, COLORREF edge);

/* Frees the smoothing masks kept for the current size. */
void hg_glyph_release(void);

#endif /* HG_GLYPH_H */
