/* The function buttons' icons (R1.9 / R1.15). See hg_glyph.h.
 *
 * Each icon is a list of strokes centered on a 100 x 100 square (D-U1 drops
 * the permanent top-left badge hole because badges now appear only on hover,
 * keyboard focus, or modifier peek): polylines, outlines, filled shapes,
 * arcs and dots. Drawing one has two optional passes over the same list:
 *
 *   1. the band: skipped when edge == CLR_NONE (flat tile mode). Otherwise,
 *      every stroke with a pen wider by the outline on each side, in edge.
 *   2. the icon: the same strokes four times larger, white on black, in a
 *      bitmap of their own; each 4 x 4 block is one pixel's coverage, and the
 *      icon colour is mixed into the paint buffer by it.
 *
 * The coverage masks depend only on the size, so they are kept for the size
 * last drawn and rebuilt when it changes. */

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

/* D - show the desktop: centered screen and arrow taking windows down. */
static const short g_desktop[] = {GOP_LOOP, 4, 16, 18, 84, 18, 84, 68, 16, 68,
                                  GOP_LINE, 2, 50, 68, 50, 82, GOP_LINE, 2, 32, 82, 68, 82,
                                  GOP_LINE, 2, 50, 28, 50, 56, GOP_LINE, 3, 39, 45, 50, 56, 61, 45,
                                  GOP_END};

/* Cmd - the command box: centered prompt. */
static const short g_command[] = {GOP_LINE, 3, 20, 24, 46, 50, 20, 76, GOP_LINE, 2, 52, 76, 82, 76, GOP_END};

/* Note - a pen writing a line: centered filled pen body and written line. */
static const short g_note[] = {GOP_FILL, 5, 74, 16, 84, 26, 44, 66, 26, 74, 34, 56,
                               GOP_LINE, 2, 16, 84, 78, 84, GOP_END};

/* Clip - a centered clipboard sheet. */
static const short g_clip[] = {GOP_LOOP, 4, 24, 22, 76, 22, 76, 86, 24, 86,
                               GOP_FILL, 4, 38, 14, 62, 14, 62, 28, 38, 28,
                               GOP_LINE, 2, 36, 46, 64, 46, GOP_LINE, 2, 36, 60, 64, 60, GOP_LINE, 2, 36, 74, 56, 74,
                               GOP_END};

/* Set - settings and switches: three centered sliders. */
static const short g_settings[] = {GOP_LINE, 2, 16, 24, 84, 24, GOP_DOT, 66, 24, 7,
                                   GOP_LINE, 2, 16, 50, 84, 50, GOP_DOT, 34, 50, 7,
                                   GOP_LINE, 2, 16, 76, 84, 76, GOP_DOT, 60, 76, 7, GOP_END};

/* A/V - sound and picture: speaker lower-left, screen upper-right. */
static const short g_menu[] = {GOP_FILL, 6, 12, 56, 22, 56, 34, 46, 34, 84, 22, 74, 12, 74,
                               GOP_ARC, 34, 65, 13, -45, 45, GOP_ARC, 34, 65, 22, -45, 45,
                               GOP_LOOP, 4, 46, 16, 88, 16, 88, 44, 46, 44, GOP_LINE, 2, 67, 44, 67, 52,
                               GOP_LINE, 2, 56, 52, 78, 52, GOP_END};

/* Dir - centered folder. */
static const short g_dir[] = {GOP_LOOP, 8, 14, 22, 42, 22, 48, 32, 86, 32, 86, 80, 14, 80,
                              GOP_LINE, 2, 14, 44, 86, 44, GOP_END};

/* Alp - opacity: two centered overlapping sheets. */
static const short g_alpha[] = {GOP_LOOP, 4, 16, 34, 66, 34, 66, 84, 16, 84,
                                GOP_LOOP, 4, 34, 16, 84, 16, 84, 66, 34, 66,
                                GOP_FILL, 4, 34, 34, 66, 34, 66, 66, 34, 66, GOP_END};

/* Mon - brightness: centered lit bulb with rays. */
static const short g_monitor[] = {GOP_DOT, 50, 44, 14, GOP_FILL, 4, 42, 56, 58, 56, 56, 68, 44, 68,
                                  GOP_LINE, 2, 44, 76, 56, 76, GOP_LINE, 2, 46, 84, 54, 84,
                                  GOP_LINE, 2, 50, 22, 50, 14, GOP_LINE, 2, 68, 28, 75, 21,
                                  GOP_LINE, 2, 32, 28, 25, 21, GOP_LINE, 2, 72, 44, 82, 44,
                                  GOP_LINE, 2, 28, 44, 18, 44, GOP_END};

/* Vol - volume: centered speaker and three sound waves. */
static const short g_volume[] = {GOP_FILL, 6, 14, 39, 28, 39, 46, 24, 46, 76, 28, 61, 14, 61,
                                 GOP_ARC, 46, 50, 14, -45, 45, GOP_ARC, 46, 50, 25, -45, 45,
                                 GOP_ARC, 46, 50, 36, -42, 42, GOP_END};

/* Vol (muted) - speaker with sharp X cross. */
static const short g_volume_muted[] = {GOP_FILL, 6, 14, 39, 28, 39, 46, 24, 46, 76, 28, 61, 14, 61,
                                       GOP_LINE, 2, 58, 37, 84, 63, GOP_LINE, 2, 84, 37, 58, 63, GOP_END};

/* BakREC - record dot and arrow down into centered tray. */
static const short g_recbak[] = {GOP_LINE, 4, 14, 60, 14, 84, 86, 84, 86, 60, GOP_DOT, 30, 68, 8,
                                 GOP_LINE, 2, 60, 16, 60, 68, GOP_LINE, 3, 47, 55, 60, 68, 73, 55, GOP_END};

static const short *glyph_table(int index)
{
    switch (index) {
    case HG_TOOL_ICON_DESKTOP: return g_desktop;
    case HG_TOOL_ICON_COMMAND: return g_command;
    case HG_TOOL_ICON_NOTE: return g_note;
    case HG_TOOL_ICON_CLIP: return g_clip;
    case HG_TOOL_ICON_SETTINGS: return g_settings;
    case HG_TOOL_ICON_DIR: return g_dir;
    case HG_TOOL_ICON_ALPHA: return g_alpha;
    case HG_TOOL_ICON_MONITOR: return g_monitor;
    case HG_TOOL_ICON_VOLUME: return g_volume;
    case HG_GLYPH_VOLUME_MUTED: return g_volume_muted;
    case HG_TOOL_ICON_MENU: return g_menu;
    case HG_TOOL_ICON_RECBAK: return g_recbak;
    default: return NULL;
    }
}

BOOL hg_glyph_has(int index)
{
    return glyph_table(index) != NULL;
}

/* The pen: square ends and mitred joins, so a stroke ends and turns on a
 * corner rather than a curve (owner, 2026-11-12: sharp, not rounded). The
 * band's pen is the same kind, only wider, so its mitre runs parallel to the
 * icon's and the outline stays even round a corner too. */
static HPEN glyph_pen(int width, COLORREF color)
{
    LOGBRUSH lb = {BS_SOLID, color, 0};
    if (width < 1)
        width = 1;
    return ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_MITER, (DWORD)width, &lb, 0, NULL);
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

/* The stroke width in pixels for an icon `size` across: thin, a line drawing
 * rather than a marker one (owner, 2026-11-12), never under one and a quarter. */
static double glyph_stroke(int size)
{
    double w = size * 0.06;
    return (w < 1.25) ? 1.25 : w;
}

static int s_mask_size = 0;
static BYTE *s_masks[HG_GLYPH_COUNT];

void hg_glyph_release(void)
{
    for (int i = 0; i < HG_GLYPH_COUNT; ++i) {
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
    if (index < 0 || index >= HG_GLYPH_COUNT)
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

    /* 1. The band, hard-edged; skipped when edge == CLR_NONE (flat tiles). */
    if (edge != CLR_NONE) {
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
    }

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
