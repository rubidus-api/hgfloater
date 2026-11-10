/* The function buttons' icons (R1.9). See hg_glyph.h.
 *
 * Each icon is a list of strokes on a 100 x 100 square: polylines, outlines,
 * filled shapes, arcs and dots. Drawing one is two passes over the same list:
 *
 *   1. the band: every stroke with a pen wider by the outline on each side, in
 *      the outline colour, straight into the paint buffer with plain GDI. Not
 *      smoothed, on purpose: the toolbar's background is the window's colour
 *      key, and a smoothed edge would mix with the key into a colour that is
 *      no longer the key, so it would show as a fringe on the desktop.
 *   2. the icon: the same strokes four times larger, white on black, in a
 *      bitmap of their own; each 4 x 4 block is one pixel's coverage, and the
 *      icon colour is mixed into the paint buffer by it. All of that falls on
 *      the band, never on the key - and a pixel that is still the key is left
 *      alone anyway, so a rounding difference cannot make a fringe either.
 *
 * The coverage masks depend only on the size, so they are kept for the size
 * last drawn and rebuilt when it changes.
 *
 * The rule every table keeps: nothing in the top-left quadrant (x < 50 and
 * y < 50). That is where a button's key badge is drawn, over the icon. A round
 * shape centred off the middle may touch its corner; nothing more. */

#include "hg_glyph.h"
#include "hg_common.h"

#include <math.h>

enum { GOP_END = 0, GOP_LINE, GOP_LOOP, GOP_FILL, GOP_ARC, GOP_DOT };

/* GOP_LINE n x y ...   open polyline of n points
 * GOP_LOOP n x y ...   closed outline
 * GOP_FILL n x y ...   filled shape
 * GOP_ARC cx cy r a0 a1  counter-clockwise from a0 to a1 degrees (0 = right,
 *                        90 = up); a0 == a1 is the whole circle
 * GOP_DOT cx cy r      filled circle */

/* X - quit: a plain cross (owner, 2026-11-11). */
static const short g_close[] = {GOP_LINE, 2, 50, 42, 90, 82, GOP_LINE, 2, 90, 42, 50, 82, GOP_END};

/* M - move: arrows four ways. */
static const short g_move[] = {GOP_LINE, 2, 32, 61, 90, 61,     GOP_LINE, 2, 61, 32, 61, 90,
                               GOP_LINE, 3, 42, 51, 32, 61, 42, 71, GOP_LINE, 3, 80, 51, 90, 61, 80, 71,
                               GOP_LINE, 3, 51, 42, 61, 32, 71, 42, GOP_LINE, 3, 51, 80, 61, 90, 71, 80,
                               GOP_END};

/* R - resize: a small window filled in the top-left corner of a large frame,
 * and an arrow from it to the far corner - the window grows that way (the
 * owner's design, 2026-11-10). This one alone uses the top-left quadrant, on
 * the owner's choice: R has no key, so no badge sits there; were one bound,
 * it would land on the filled corner and hide nothing else. */
static const short g_resize[] = {GOP_LOOP, 4, 12, 12, 90, 12, 90, 90, 12, 90,
                                 GOP_FILL, 4, 12, 12, 46, 12, 46, 46, 12, 46,
                                 GOP_LINE, 2, 58, 58, 78, 78, GOP_LINE, 3, 66, 78, 78, 78, 78, 66, GOP_END};

/* D - show the desktop: a screen, and an arrow taking the windows down. */
static const short g_desktop[] = {GOP_LOOP, 4, 16, 52, 90, 52, 90, 78, 16, 78,
                                  GOP_LINE, 2, 53, 78, 53, 88, GOP_LINE, 2, 38, 88, 68, 88,
                                  GOP_LINE, 2, 72, 14, 72, 40, GOP_LINE, 3, 63, 31, 72, 40, 81, 31,
                                  GOP_END};

/* Cmd - the command box: a prompt. */
static const short g_command[] = {GOP_LINE, 3, 18, 54, 40, 71, 18, 88, GOP_LINE, 2, 50, 88, 84, 88, GOP_END};

/* Note - a pen writing a line: a filled pen body from the top-right down to its
 * nib, and the line it has written. Kept unlike Clip's page on purpose. */
static const short g_note[] = {GOP_FILL, 5, 82, 18, 90, 26, 60, 56, 42, 66, 52, 48,
                               GOP_LINE, 2, 14, 86, 64, 86, GOP_END};

/* Clip - a clipboard, and the sheet behind it: the history. */
static const short g_clip[] = {GOP_LOOP, 4, 52, 22, 90, 22, 90, 90, 52, 90,
                               GOP_FILL, 4, 62, 14, 80, 14, 80, 28, 62, 28,
                               GOP_LINE, 2, 60, 46, 82, 46, GOP_LINE, 2, 60, 60, 82, 60, GOP_LINE, 2, 60, 74, 76, 74,
                               GOP_LINE, 4, 42, 58, 16, 58, 16, 90, 42, 90, GOP_END};

/* Set - settings and switches: three sliders. */
static const short g_settings[] = {GOP_LINE, 2, 56, 26, 88, 26, GOP_DOT, 76, 26, 7,
                                   GOP_LINE, 2, 14, 56, 88, 56, GOP_DOT, 34, 56, 7,
                                   GOP_LINE, 2, 14, 84, 88, 84, GOP_DOT, 66, 84, 7, GOP_END};

/* A/V - sound and picture: a speaker below, a screen above it on the right. */
static const short g_menu[] = {GOP_FILL, 6, 12, 62, 22, 62, 34, 52, 34, 88, 22, 78, 12, 78,
                               GOP_ARC, 34, 70, 12, -50, 50, GOP_ARC, 34, 70, 23, -50, 50,
                               GOP_LOOP, 4, 56, 14, 90, 14, 90, 38, 56, 38, GOP_LINE, 2, 73, 38, 73, 44,
                               GOP_LINE, 2, 64, 46, 82, 46, GOP_END};

/* Dir - folders: the tab stands up on the right. */
static const short g_dir[] = {GOP_LOOP, 8, 14, 52, 52, 52, 52, 24, 66, 24, 72, 32, 90, 32, 90, 88, 14, 88,
                              GOP_LINE, 2, 52, 52, 90, 52, GOP_END};

/* Alp - opacity: two sheets, one over the other, where they overlap filled. */
static const short g_alpha[] = {GOP_LOOP, 4, 14, 50, 64, 50, 64, 90, 14, 90,
                                GOP_LOOP, 4, 50, 18, 90, 18, 90, 64, 50, 64,
                                GOP_FILL, 4, 50, 50, 64, 50, 64, 64, 50, 64, GOP_END};

/* Mon - brightness: a lit bulb - filled, on its neck and screw base, with
 * rays. (A sun read as a gear at the guest's size, a hanging lamp as an arrow.) */
static const short g_monitor[] = {GOP_DOT, 64, 42, 13, GOP_FILL, 4, 56, 52, 72, 52, 70, 64, 58, 64,
                                  GOP_LINE, 2, 58, 74, 70, 74, GOP_LINE, 2, 60, 84, 68, 84,
                                  GOP_LINE, 2, 64, 18, 64, 12, GOP_LINE, 2, 84, 30, 89, 26,
                                  GOP_LINE, 2, 86, 46, 92, 48, GOP_LINE, 2, 42, 56, 36, 60, GOP_END};

/* Vol - volume: a speaker and its waves. */
static const short g_volume[] = {GOP_FILL, 6, 14, 60, 26, 60, 40, 50, 40, 86, 26, 76, 14, 76,
                                 GOP_ARC, 40, 68, 13, -45, 45, GOP_ARC, 40, 68, 24, -45, 45,
                                 GOP_ARC, 40, 68, 35, -42, 42, GOP_END};

/* BakREC - the recorder's recordings into a folder: the record dot, an arrow
 * going down into a tray. */
static const short g_recbak[] = {GOP_LINE, 4, 14, 62, 14, 88, 90, 88, 90, 62, GOP_DOT, 32, 70, 8,
                                 GOP_LINE, 2, 66, 14, 66, 72, GOP_LINE, 3, 53, 59, 66, 72, 79, 59, GOP_END};

static const short *glyph_table(int index)
{
    switch (index) {
    case HG_TOOL_ICON_CLOSE: return g_close;
    case HG_TOOL_ICON_MOVE: return g_move;
    case HG_TOOL_ICON_RESIZE: return g_resize;
    case HG_TOOL_ICON_DESKTOP: return g_desktop;
    case HG_TOOL_ICON_COMMAND: return g_command;
    case HG_TOOL_ICON_NOTE: return g_note;
    case HG_TOOL_ICON_CLIP: return g_clip;
    case HG_TOOL_ICON_SETTINGS: return g_settings;
    case HG_TOOL_ICON_DIR: return g_dir;
    case HG_TOOL_ICON_ALPHA: return g_alpha;
    case HG_TOOL_ICON_MONITOR: return g_monitor;
    case HG_TOOL_ICON_VOLUME: return g_volume;
    case HG_TOOL_ICON_MENU: return g_menu;
    case HG_TOOL_ICON_RECBAK: return g_recbak;
    default: return NULL;
    }
}

BOOL hg_glyph_has(int index)
{
    return glyph_table(index) != NULL;
}

/* The pen: round ends and joins, so a stroke's band is the same width all the
 * way round its corners. */
static HPEN glyph_pen(int width, COLORREF color)
{
    LOGBRUSH lb = {BS_SOLID, color, 0};
    if (width < 1)
        width = 1;
    return ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND, (DWORD)width, &lb, 0, NULL);
}

/* One pass over a table: point u maps to ox + u * k. The pen and the fill
 * brush are the caller's, already selected. */
static void glyph_emit(HDC dc, const short *op, double ox, double oy, double k, HBRUSH fill)
{
    HGDIOBJ hollow = GetStockObject(NULL_BRUSH);
    while (op && *op != GOP_END) {
        int kind = *op++;
        if (kind == GOP_LINE || kind == GOP_LOOP || kind == GOP_FILL) {
            int n = *op++;
            POINT pts[12];
            int used = 0;
            for (int i = 0; i < n; ++i, op += 2) {
                if (used < (int)HG_ARRAYSIZE(pts)) {
                    pts[used].x = (LONG)floor(ox + op[0] * k + 0.5);
                    pts[used].y = (LONG)floor(oy + op[1] * k + 0.5);
                    ++used;
                }
            }
            if (kind == GOP_LINE) {
                Polyline(dc, pts, used);
            } else {
                HGDIOBJ old = SelectObject(dc, kind == GOP_FILL ? (HGDIOBJ)fill : hollow);
                Polygon(dc, pts, used);
                SelectObject(dc, old);
            }
        } else if (kind == GOP_ARC) {
            double cx = ox + op[0] * k, cy = oy + op[1] * k, r = op[2] * k;
            double a0 = op[3] * 3.14159265358979 / 180.0, a1 = op[4] * 3.14159265358979 / 180.0;
            int l = (int)floor(cx - r + 0.5), t = (int)floor(cy - r + 0.5);
            int rr = (int)floor(cx + r + 0.5), b = (int)floor(cy + r + 0.5);
            if (op[3] == op[4]) {
                HGDIOBJ old = SelectObject(dc, hollow);
                Ellipse(dc, l, t, rr, b);
                SelectObject(dc, old);
            } else {
                /* The radials only give the direction, so they are put well
                 * outside the circle to keep their rounding out of it. */
                Arc(dc, l, t, rr, b, (int)floor(cx + 4 * r * cos(a0) + 0.5), (int)floor(cy - 4 * r * sin(a0) + 0.5),
                    (int)floor(cx + 4 * r * cos(a1) + 0.5), (int)floor(cy - 4 * r * sin(a1) + 0.5));
            }
            op += 5;
        } else if (kind == GOP_DOT) {
            double cx = ox + op[0] * k, cy = oy + op[1] * k, r = op[2] * k;
            HGDIOBJ old = SelectObject(dc, fill);
            Ellipse(dc, (int)floor(cx - r + 0.5), (int)floor(cy - r + 0.5), (int)floor(cx + r + 0.5),
                    (int)floor(cy + r + 0.5));
            SelectObject(dc, old);
            op += 3;
        } else {
            break;
        }
    }
}

/* The stroke width in pixels for an icon `size` across: thin enough to keep
 * the shapes apart, never under one and a half. */
static double glyph_stroke(int size)
{
    double w = size * 0.085;
    return (w < 1.5) ? 1.5 : w;
}

static int s_mask_size = 0;
static BYTE *s_masks[HG_NUM_BASIC_ICONS];

void hg_glyph_release(void)
{
    for (int i = 0; i < HG_NUM_BASIC_ICONS; ++i) {
        if (s_masks[i])
            HeapFree(GetProcessHeap(), 0, s_masks[i]);
        s_masks[i] = NULL;
    }
    s_mask_size = 0;
}

/* A 32-bit top-down DIB section of w x h in its own DC. */
static HDC glyph_dib(int w, int h, HBITMAP *out_bmp, HGDIOBJ *out_old, DWORD **out_bits)
{
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void *bits = NULL;
    HBITMAP bmp = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!bmp || !bits) {
        if (bmp)
            DeleteObject(bmp);
        return NULL;
    }
    HDC dc = CreateCompatibleDC(NULL);
    if (!dc) {
        DeleteObject(bmp);
        return NULL;
    }
    *out_old = SelectObject(dc, bmp);
    *out_bmp = bmp;
    *out_bits = (DWORD *)bits;
    return dc;
}

static void glyph_dib_free(HDC dc, HBITMAP bmp, HGDIOBJ old)
{
    SelectObject(dc, old);
    DeleteDC(dc);
    DeleteObject(bmp);
}

/* The icon's coverage at `size`: one byte a pixel, 0..255. */
static const BYTE *glyph_mask(int index, int size)
{
    if (index < 0 || index >= HG_NUM_BASIC_ICONS)
        return NULL;
    if (size != s_mask_size) {
        hg_glyph_release();
        s_mask_size = size;
    }
    if (s_masks[index])
        return s_masks[index];

    const short *table = glyph_table(index);
    BYTE *mask = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)size * (SIZE_T)size);
    if (!table || !mask) {
        if (mask)
            HeapFree(GetProcessHeap(), 0, mask);
        return NULL;
    }

    int big = size * 4;
    HBITMAP bmp = NULL;
    HGDIOBJ old_bmp = NULL;
    DWORD *bits = NULL;
    HDC dc = glyph_dib(big, big, &bmp, &old_bmp, &bits);
    if (!dc) {
        HeapFree(GetProcessHeap(), 0, mask);
        return NULL;
    }
    /* Black by creation; white strokes. A low-resolution pixel's centre sits
     * at x + 0.5, which is 4x + 2 here, hence the 1.5 (the centre of the
     * fine pixel just before it). */
    HPEN pen = glyph_pen((int)floor(glyph_stroke(size) * 4.0 + 0.5), RGB(255, 255, 255));
    HBRUSH fill = CreateSolidBrush(RGB(255, 255, 255));
    if (pen && fill) {
        HGDIOBJ old_pen = SelectObject(dc, pen);
        SetArcDirection(dc, AD_COUNTERCLOCKWISE);
        glyph_emit(dc, table, 1.5, 1.5, size * 4.0 / 100.0, fill);
        SelectObject(dc, old_pen);
        GdiFlush();
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                unsigned sum = 0;
                for (int j = 0; j < 4; ++j) {
                    const DWORD *row = bits + (size_t)(y * 4 + j) * (size_t)big + (size_t)x * 4;
                    sum += (row[0] & 0xFF) + (row[1] & 0xFF) + (row[2] & 0xFF) + (row[3] & 0xFF);
                }
                mask[(size_t)y * (size_t)size + (size_t)x] = (BYTE)(sum / 16);
            }
        }
    }
    if (pen)
        DeleteObject(pen);
    if (fill)
        DeleteObject(fill);
    glyph_dib_free(dc, bmp, old_bmp);
    s_masks[index] = mask;
    return mask;
}

void hg_glyph_draw(HDC dc, int index, const RECT *rc, COLORREF fg, COLORREF edge)
{
    const short *table = glyph_table(index);
    if (!dc || !rc || !table)
        return;
    int size = rc->right - rc->left;
    if (rc->bottom - rc->top < size)
        size = rc->bottom - rc->top;
    if (size < 8)
        return;

    /* 1. The band, hard-edged. */
    int band = SC(1);
    if (band < 1)
        band = 1;
    HPEN pen = glyph_pen((int)floor(glyph_stroke(size) + 2.0 * band + 0.5), edge);
    HBRUSH fill = CreateSolidBrush(edge);
    if (pen && fill) {
        HGDIOBJ old_pen = SelectObject(dc, pen);
        int old_dir = SetArcDirection(dc, AD_COUNTERCLOCKWISE);
        glyph_emit(dc, table, rc->left, rc->top, size / 100.0, fill);
        if (old_dir)
            SetArcDirection(dc, old_dir);
        SelectObject(dc, old_pen);
    }
    if (pen)
        DeleteObject(pen);
    if (fill)
        DeleteObject(fill);

    /* 2. The icon, mixed in by coverage. */
    const BYTE *mask = glyph_mask(index, size);
    if (!mask)
        return;
    HBITMAP bmp = NULL;
    HGDIOBJ old_bmp = NULL;
    DWORD *bits = NULL;
    HDC tmp = glyph_dib(size, size, &bmp, &old_bmp, &bits);
    if (!tmp)
        return;
    BitBlt(tmp, 0, 0, size, size, dc, rc->left, rc->top, SRCCOPY);
    GdiFlush();
    COLORREF key = HG_TRANSPARENT_KEY;
    DWORD key_px = ((DWORD)GetRValue(key) << 16) | ((DWORD)GetGValue(key) << 8) | GetBValue(key);
    int fr = GetRValue(fg), fgc = GetGValue(fg), fb = GetBValue(fg);
    for (int i = 0; i < size * size; ++i) {
        unsigned a = mask[i];
        if (!a)
            continue;
        DWORD px = bits[i] & 0x00FFFFFF;
        if (px == key_px)
            continue;
        int r = (int)((px >> 16) & 0xFF), g = (int)((px >> 8) & 0xFF), b = (int)(px & 0xFF);
        r += ((fr - r) * (int)a) / 255;
        g += ((fgc - g) * (int)a) / 255;
        b += ((fb - b) * (int)a) / 255;
        bits[i] = ((DWORD)r << 16) | ((DWORD)g << 8) | (DWORD)b;
    }
    BitBlt(dc, rc->left, rc->top, size, size, tmp, 0, 0, SRCCOPY);
    glyph_dib_free(tmp, bmp, old_bmp);
}
