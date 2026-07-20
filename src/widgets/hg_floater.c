#include "hg_floater.h"
#include "../hg_utils.h"
#include "../hg_config.h"

/* Forward declaration for show_about_window which is in hgfloater.c (or widgets/hg_about.c when created) */
void show_about_window(void);

/* Tiny label font for the status bars; recreated whenever the floater font size or DPI changes. */
static HFONT s_floater_label_font = NULL;
/* Thin host-name font; same lifecycle as the label font. */
static HFONT s_floater_host_font = NULL;

void update_floater_font_size(int delta)
{
    int new_size = hg_g_floater_font_size + (delta > 0 ? 2 : -2);
    if (new_size < HG_FLOATER_MIN_FONT_SIZE)
        new_size = HG_FLOATER_MIN_FONT_SIZE;
    if (new_size > HG_FLOATER_MAX_FONT_SIZE)
        new_size = HG_FLOATER_MAX_FONT_SIZE;
    if (hg_g_floater_font_size != new_size) {
        hg_g_floater_font_size = new_size;
        release_font_handle(&hg_g_floater_time_font, FALSE);
        release_font_handle(&hg_g_floater_date_font, FALSE);
        release_font_handle(&s_floater_label_font, FALSE);
        release_font_handle(&s_floater_host_font, FALSE);
        update_floater_layout(hg_g_floater_wnd);
        InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
        save_floater_font_config();
    }
}

void update_floater_alpha(int delta)
{
    if (!hg_step_alpha_value(&hg_g_floater_alpha, delta))
        return;
    if (hg_g_floater_wnd) {
        SetLayeredWindowAttributes(hg_g_floater_wnd, 0, hg_g_floater_alpha, LWA_ALPHA);
    }
    save_alpha_config();
}

/* Compact status panel on the floater's left side: CPU/MEM/BAT horizontal bar
 * graphs (top to bottom, red/blue/green) with tiny labels. The panel keeps the
 * floater at its clock height; the battery row hides on systems without one. */
static int s_stat_cpu = -1;
static int s_stat_mem = -1;
static int s_stat_bat = -1;
static BOOL s_stat_has_bat = FALSE;
static BOOL s_stat_charging = FALSE;

static BOOL floater_refresh_stats(void)
{
    int cpu = -1;
    int mem = -1;
    int bat = 0;
    BOOL has_bat = FALSE;
    BOOL charging = FALSE;

    if (hg_g_floater_show_stats) {
        cpu = hg_get_cpu_percent();
        mem = hg_get_memory_percent();
        has_bat = hg_get_battery_percent(&bat, &charging);
    }
    if (cpu == s_stat_cpu && mem == s_stat_mem && has_bat == s_stat_has_bat &&
        (!has_bat || (bat == s_stat_bat && charging == s_stat_charging))) {
        return FALSE;
    }
    s_stat_cpu = cpu;
    s_stat_mem = mem;
    s_stat_has_bat = has_bat;
    s_stat_bat = bat;
    s_stat_charging = charging;
    return TRUE;
}

/* Only the tiny labels claim exclusive horizontal space; the bars themselves
 * run underneath the clock and date as background. The label font follows the
 * floater font size so the labels stay proportionally small. */
/* The clock is the reference: its size is 120% of the configured floater font
 * size, and everything else is a fixed ratio of the clock. */
static int floater_time_font_height(void)
{
    int size = hg_g_floater_font_size * 12 / 10;
    if (size < 12)
        size = 12;
    return size;
}

/* Ratios of the clock font: labels 30%, host name 40%, date 70%. */
static int floater_label_font_height(void)
{
    int size = floater_time_font_height() * 30 / 100;
    if (size < 7)
        size = 7;
    return size;
}

static int floater_host_font_height(void)
{
    int size = floater_time_font_height() * 40 / 100;
    if (size < 8)
        size = 8;
    return size;
}

static int floater_date_font_height(void)
{
    int size = floater_time_font_height() * 70 / 100;
    if (size < 9)
        size = 9;
    return size;
}

static HFONT floater_label_font(void)
{
    if (!s_floater_label_font) {
        s_floater_label_font = CreateFontW(SC(floater_label_font_height()), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
    }
    return s_floater_label_font;
}

/* Host name line across the top of the floater: light weight, a little larger
 * than the tiny bar labels. Cached once (the computer name does not change
 * while the process runs). */
static WCHAR s_floater_host[MAX_COMPUTERNAME_LENGTH + 1] = L"";

static const WCHAR *floater_host_name(void)
{
    if (!s_floater_host[0]) {
        DWORD len = HG_ARRAYSIZE(s_floater_host);
        if (!GetComputerNameW(s_floater_host, &len)) {
            s_floater_host[0] = L'\0';
        }
    }
    return s_floater_host;
}

static HFONT floater_host_font(void)
{
    if (!s_floater_host_font) {
        s_floater_host_font = CreateFontW(SC(floater_host_font_height()), 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
    }
    return s_floater_host_font;
}

/* ---------------------------------------------------------------------------
 * Layout metrics
 *
 * Every length is proportional: it derives from a measured text extent (which
 * already scales with the floater font size and the monitor DPI) or from a
 * percentage of the floater font size run through SC(). No fixed pixel gaps,
 * so the whole widget keeps its proportions at any font size and DPI.
 *
 * Vertical:   pad + host + gap + time + date + pad  == window height (exact)
 * Horizontal: pad + [label strip + bar/text column] + pad == window width
 * ------------------------------------------------------------------------- */

/* Fraction of the floater font size, scaled for DPI, floored at one pixel. */
static int floater_metric(int numerator, int denominator)
{
    int value = SC(hg_g_floater_font_size * numerator / denominator);
    return (value > 0) ? value : 1;
}

static int floater_pad_x(void)
{
    return floater_metric(1, 4); /* 25% of the font size */
}

static int floater_pad_y(void)
{
    return floater_metric(1, 6); /* ~17% */
}

/* No vertical gaps: the host name, clock, and date stack flush against each
 * other by design. Only the horizontal label/bar gap remains. */
static int floater_gap_x(void)
{
    return floater_metric(1, 8);
}

/* Vertical inset of a bar inside its row (keeps bars from touching). */
static int floater_bar_inset(void)
{
    return floater_metric(1, 14);
}

/* Ink bounds of a rendered line: the cell height a font reports carries slack
 * above the caps and below the baseline, which shows up as gaps between the
 * stacked lines and makes the glyphs look bottom-heavy. Rendering the text to
 * a scratch bitmap and scanning for the first and last row that actually got
 * ink gives the true glyph box, so lines can sit flush and be centered on what
 * the eye sees rather than on the font's cell. */
typedef struct HgInkExtent {
    int cx;      /* advance width (from GetTextExtentPoint32W) */
    int cy;      /* ink height; 0 when the text is empty */
    int top;     /* ink top, relative to the text cell's top */
} HgInkExtent;

static HgInkExtent floater_ink_extent(HDC hdc, HFONT font, const WCHAR *text)
{
    HgInkExtent ink = {0, 0, 0};
    SIZE sz = {0};

    if (!hdc || !font || !text || !text[0])
        return ink;

    HFONT old_font = (HFONT)SelectObject(hdc, font);
    GetTextExtentPoint32W(hdc, text, (int)lstrlenW(text), &sz);
    SelectObject(hdc, old_font);
    ink.cx = sz.cx;
    if (sz.cx <= 0 || sz.cy <= 0)
        return ink;

    /* Fallback if the scratch surface cannot be made: the full cell. */
    ink.cy = sz.cy;
    ink.top = 0;

    HDC scratch = CreateCompatibleDC(hdc);
    if (!scratch)
        return ink;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = sz.cx;
    bmi.bmiHeader.biHeight = -sz.cy; /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP bmp = CreateDIBSection(scratch, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!bmp || !bits) {
        if (bmp)
            DeleteObject(bmp);
        DeleteDC(scratch);
        return ink;
    }

    HBITMAP old_bmp = (HBITMAP)SelectObject(scratch, bmp);
    HFONT scratch_old_font = (HFONT)SelectObject(scratch, font);
    RECT full = {0, 0, sz.cx, sz.cy};
    FillRect(scratch, &full, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetBkMode(scratch, TRANSPARENT);
    SetTextColor(scratch, RGB(255, 255, 255));
    TextOutW(scratch, 0, 0, text, (int)lstrlenW(text));
    GdiFlush();

    const DWORD *pixels = (const DWORD *)bits;
    int first = -1;
    int last = -1;
    for (int y = 0; y < sz.cy; ++y) {
        const DWORD *row = pixels + (size_t)y * (size_t)sz.cx;
        for (int x = 0; x < sz.cx; ++x) {
            if ((row[x] & 0x00FFFFFFu) != 0) {
                if (first < 0)
                    first = y;
                last = y;
                break;
            }
        }
    }

    SelectObject(scratch, scratch_old_font);
    SelectObject(scratch, old_bmp);
    DeleteObject(bmp);
    DeleteDC(scratch);

    if (first >= 0 && last >= first) {
        ink.top = first;
        ink.cy = last - first + 1;
    }
    return ink;
}

static SIZE floater_text_extent(HDC hdc, HFONT font, const WCHAR *text)
{
    HgInkExtent ink = floater_ink_extent(hdc, font, text);
    SIZE sz = {ink.cx, ink.cy};
    return sz;
}

/* Where to place a text cell so its ink lands centered in a line of the given
 * height: subtract the ink offset, then split the leftover slack evenly. */
static int floater_ink_top(HDC hdc, HFONT font, const WCHAR *text, int line_top, int line_h)
{
    HgInkExtent ink = floater_ink_extent(hdc, font, text);
    int slack = line_h - ink.cy;

    if (slack < 0)
        slack = 0;
    return line_top - ink.top + slack / 2;
}

/* Host line extent (zeroes when there is no name to draw). */
static SIZE floater_host_extent(HDC hdc)
{
    return floater_text_extent(hdc, floater_host_font(), floater_host_name());
}

/* Exclusive strip width for the labels, from the widest label's measured
 * extent plus a proportional gap. */
static int floater_stats_label_width(HDC hdc)
{
    SIZE sz = floater_text_extent(hdc, floater_label_font(), L"BAT+");

    if (sz.cx <= 0)
        return 0;
    return sz.cx + floater_gap_x();
}

/* One place computes the geometry; both the layout pass and the paint pass use
 * it, so what is measured is exactly what is drawn. */
typedef struct HgFloaterMetrics {
    int width;       /* required window width */
    int height;      /* required window height */
    int pad_x;
    int pad_y;
    int host_h;      /* 0 when no host line */
    int time_h;      /* clock ink plus a breathing margin above and below */
    int date_h;
    int label_w;     /* 0 when the status bars are off */
    int content_x;   /* left edge of the label strip / content column */
    int column_x;    /* left edge of the bars and the clock/date column */
    int column_w;    /* width of that column */
    int rows_y;      /* top of the clock/date (and of the bar rows) */
    int rows_h;      /* combined height of the clock and date */
} HgFloaterMetrics;

static void floater_compute_metrics(HDC hdc, const WCHAR *time_str, const WCHAR *date_str, HgFloaterMetrics *out)
{
    SIZE sz_time = floater_text_extent(hdc, hg_g_floater_time_font, time_str);
    SIZE sz_date = floater_text_extent(hdc, hg_g_floater_date_font, date_str);
    SIZE sz_host = floater_host_extent(hdc);

    ZeroMemory(out, sizeof(*out));
    out->pad_x = floater_pad_x();
    out->pad_y = floater_pad_y();
    out->host_h = sz_host.cy;
    /* The lines are otherwise ink-flush, which reads as cramped around the
     * clock, so give it a margin of a tenth of its own ink height above and
     * below. The ink-centering places the glyphs in the middle of that. */
    int time_margin = sz_time.cy / 10;
    if (time_margin < 1)
        time_margin = 1;
    out->time_h = sz_time.cy + time_margin * 2;
    out->date_h = sz_date.cy;
    out->label_w = hg_g_floater_show_stats ? floater_stats_label_width(hdc) : 0;

    /* Width: the label strip plus the widest of clock/date, and the host name
     * spans the full inner width, so it only widens the box when it is wider. */
    int text_w = (sz_time.cx > sz_date.cx) ? sz_time.cx : sz_date.cx;
    int inner_w = out->label_w + text_w;
    if (sz_host.cx > inner_w)
        inner_w = sz_host.cx;
    out->width = out->pad_x * 2 + inner_w;

    /* Height: pad + host + time + date + pad, exactly; no gaps in between. */
    out->height = out->pad_y * 2 + out->host_h + out->time_h + out->date_h;

    out->content_x = out->pad_x;
    out->column_x = out->pad_x + out->label_w;
    out->column_w = out->width - out->pad_x - out->column_x;
    /* The bars and the clock/date share the area below the host name. */
    out->rows_y = out->pad_y + out->host_h;
    out->rows_h = out->time_h + out->date_h;
}

/* Bars fill the column to the right of the labels; labels are centered in their
 * row so they line up with the bar they belong to. */
static void floater_draw_stats_panel(HDC dc, const HgFloaterMetrics *m)
{
    struct {
        const WCHAR *label;
        int value;
        COLORREF color;
        BOOL present;
    } rows[3] = {
        {L"CPU", s_stat_cpu, hg_g_color_stat_cpu, s_stat_cpu >= 0},
        {L"MEM", s_stat_mem, hg_g_color_stat_mem, s_stat_mem >= 0},
        {s_stat_charging ? L"BAT+" : L"BAT", s_stat_bat, hg_g_color_stat_bat, s_stat_has_bat},
    };

    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (rows[i].present)
            count++;
    }
    if (count == 0 || m->label_w <= 0 || m->rows_h <= 0)
        return;

    HFONT font = floater_label_font();
    HFONT old_font = NULL;
    if (font)
        old_font = (HFONT)SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, HG_COLOR_TEXT_DEFAULT);

    int row_h = m->rows_h / count;
    int bar_gap = floater_bar_inset();
    int drawn = 0;
    for (int i = 0; i < 3; i++) {
        if (!rows[i].present)
            continue;
        int top = m->rows_y + drawn * row_h;
        /* Center on the ink, not the font cell, so the label lines up with its bar. */
        int label_y = floater_ink_top(dc, font, rows[i].label, top, row_h);
        RECT label_rc = {m->content_x, label_y, m->content_x + m->label_w, label_y + row_h * 4};
        DrawTextW(dc, rows[i].label, -1, &label_rc, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

        RECT track = {m->column_x, top + bar_gap, m->column_x + m->column_w, top + row_h - bar_gap};
        if (track.right > track.left && track.bottom > track.top) {
            /* No track fill: the floater background shows through, so the bars
             * never clash with theme colors (the blue system highlight used to
             * hide the blue MEM bar in dark mode). */
            int value = rows[i].value;
            if (value < 0)
                value = 0;
            if (value > 100)
                value = 100;
            RECT fill = track;
            fill.right = track.left + ((track.right - track.left) * value) / 100;
            HBRUSH fill_brush = hg_cached_solid_brush(rows[i].color);
            if (fill_brush && fill.right > fill.left)
                FillRect(dc, &fill, fill_brush);
        }
        drawn++;
    }
    if (old_font)
        SelectObject(dc, old_font);
}

void update_floater_layout(HWND hwnd)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    WCHAR time_str[16], date_str[32];
    hellgates_wsprintf(time_str, 16, L"%02d:%02d", st.wHour, st.wMinute);
    const WCHAR *months[] = {L"",    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                             L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
    const WCHAR *days[] = {L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"};
    hellgates_wsprintf(date_str, 32, L"%ls, %ls %d", days[st.wDayOfWeek], months[st.wMonth], st.wDay);

    if (!hg_g_floater_time_font)
        hg_g_floater_time_font = CreateFontW(SC(floater_time_font_height()), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
    if (!hg_g_floater_date_font)
        hg_g_floater_date_font = CreateFontW(SC(floater_date_font_height()), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
    if (!hg_g_floater_time_font || !hg_g_floater_date_font)
        return;

    HDC hdc = GetDC(hwnd);
    if (!hdc)
        return;

    HgFloaterMetrics m;
    floater_compute_metrics(hdc, time_str, date_str, &m);

    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (rc.right - rc.left != m.width || rc.bottom - rc.top != m.height) {
        SetWindowPos(hwnd, NULL, 0, 0, m.width, m.height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    ReleaseDC(hwnd, hdc);
}

static LRESULT floater_controller_on_create(HWND hwnd)
{
    hg_g_shellhook_msg = RegisterWindowMessageW(L"SHELLHOOK");
    RegisterShellHookWindow(hwnd);
    floater_refresh_stats();

    SetLayeredWindowAttributes(hwnd, 0, hg_g_floater_alpha, LWA_ALPHA);

    apply_dwm_attributes(hwnd);

    update_floater_layout(hwnd);
    SetTimer(hwnd, HG_TIMER_FLOATER_CLOCK, 1000, NULL);
    return 0;
}

static LRESULT floater_controller_on_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (rc.right > 0 && rc.bottom > 0) {
        HgPaintBuffer paint_buffer;
        if (hg_paint_buffer_begin(hdc, rc.right, rc.bottom, &paint_buffer)) {
            HDC mem_dc = paint_buffer.dc;

                if (!hg_g_floater_time_font)
                    hg_g_floater_time_font = CreateFontW(SC(floater_time_font_height()), 0, 0, 0, FW_BOLD, FALSE, FALSE,
                                                         FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);
                if (!hg_g_floater_date_font)
                    hg_g_floater_date_font = CreateFontW(SC(floater_date_font_height()), 0, 0, 0, FW_BOLD, FALSE, FALSE,
                                                         FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, hg_g_font_name);

                if (hg_g_floater_time_font && hg_g_floater_date_font) {
                    COLORREF bg_color = HG_COLOR_BG_DEFAULT;
                    if (hg_g_floater_highlight_ticks > 0 && (hg_g_floater_highlight_ticks % 2 != 0)) {
                        bg_color = HG_COLOR_BG_FLASH;
                    }
                    HBRUSH bg_brush = CreateSolidBrush(bg_color);
                    int pen_width = SC(HG_BORDER_THICKNESS);

                    COLORREF border_color = HG_COLOR_BG_TOOLBAR;
                    HPEN border_pen = CreatePen(PS_SOLID, pen_width, border_color);

                    HBRUSH old_brush = NULL;
                    HPEN old_pen = NULL;
                    if (bg_brush)
                        old_brush = (HBRUSH)SelectObject(mem_dc, bg_brush);
                    if (border_pen)
                        old_pen = (HPEN)SelectObject(mem_dc, border_pen);

                    FillRect(mem_dc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

                    RECT draw_rc = rc;
                    InflateRect(&draw_rc, -pen_width / 2, -pen_width / 2);
                    if (bg_brush && border_pen) {
                        Rectangle(mem_dc, draw_rc.left, draw_rc.top, draw_rc.right, draw_rc.bottom);
                    }

                    SYSTEMTIME st;
                    GetLocalTime(&st);
                    WCHAR time_str[16], date_str[32];
                    hellgates_wsprintf(time_str, 16, L"%02d:%02d", st.wHour, st.wMinute);
                    const WCHAR *months[] = {L"",    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                                             L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
                    const WCHAR *days[] = {L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"};
                    hellgates_wsprintf(date_str, 32, L"%ls, %ls %d", days[st.wDayOfWeek], months[st.wMonth], st.wDay);

                    SetBkMode(mem_dc, TRANSPARENT);
                    SetTextColor(mem_dc, HG_COLOR_TEXT_DEFAULT);

                    /* Same metrics the layout pass used, so the drawing lands
                     * exactly where the window was sized for. */
                    HgFloaterMetrics m;
                    floater_compute_metrics(mem_dc, time_str, date_str, &m);
                    HFONT old_font_in_paint = (HFONT)SelectObject(mem_dc, hg_g_floater_time_font);

                    /* Bars first: they run behind the clock and date. */
                    if (m.label_w > 0) {
                        floater_draw_stats_panel(mem_dc, &m);
                    }

                    /* Each line's cell is placed so its ink is vertically centered
                     * in the height reserved for it, which also keeps the lines
                     * flush: the reserved heights are the ink heights. */
                    if (m.host_h > 0) {
                        HFONT host_font = floater_host_font();
                        if (host_font) {
                            const WCHAR *host = floater_host_name();
                            int top = floater_ink_top(mem_dc, host_font, host, m.pad_y, m.host_h);
                            RECT host_rc = {m.pad_x, top, rc.right - m.pad_x, top + m.host_h * 4};
                            SelectObject(mem_dc, host_font);
                            DrawTextW(mem_dc, host, -1, &host_rc,
                                      DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_END_ELLIPSIS);
                        }
                    }

                    /* Clock and date center in the column right of the labels. */
                    int time_top = m.rows_y;
                    int date_top = time_top + m.time_h;
                    int time_y = floater_ink_top(mem_dc, hg_g_floater_time_font, time_str, time_top, m.time_h);
                    int date_y = floater_ink_top(mem_dc, hg_g_floater_date_font, date_str, date_top, m.date_h);

                    RECT time_rc = {m.column_x, time_y, m.column_x + m.column_w, time_y + m.time_h * 4};
                    RECT date_rc = {m.column_x, date_y, m.column_x + m.column_w, date_y + m.date_h * 4};

                    SelectObject(mem_dc, hg_g_floater_time_font);
                    DrawTextW(mem_dc, time_str, -1, &time_rc, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

                    SelectObject(mem_dc, hg_g_floater_date_font);
                    DrawTextW(mem_dc, date_str, -1, &date_rc, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

                    SelectObject(mem_dc, old_font_in_paint);

                    if (old_brush)
                        SelectObject(mem_dc, old_brush);
                    if (old_pen)
                        SelectObject(mem_dc, old_pen);
                    if (bg_brush)
                        DeleteObject(bg_brush);
                    if (border_pen)
                        DeleteObject(border_pen);
                }

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
            hg_paint_buffer_end(&paint_buffer);
        }
    }

    EndPaint(hwnd, &ps);
    return 0;
}

static LRESULT floater_controller_on_keydown(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    int dx = 0, dy = 0;
    int move_step = SC(20);
    BOOL is_ctrl = (GetKeyState(VK_CONTROL) < 0);
    BOOL is_alt = (GetKeyState(VK_MENU) < 0) || (msg == WM_SYSKEYDOWN);

    if (is_alt) {
        if (w_param == VK_LEFT || w_param == 'A')
            dx = -move_step;
        else if (w_param == VK_RIGHT || w_param == 'D')
            dx = move_step;
        else if (w_param == VK_UP || w_param == 'W')
            dy = -move_step;
        else if (w_param == VK_DOWN || w_param == 'S')
            dy = move_step;

        if (dx != 0 || dy != 0) {
            move_window_by_offset(hwnd, dx, dy);
            ensure_window_visible(hwnd, L"floater");
            return 0;
        }

        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_floater_alpha(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_floater_alpha(-1);
            return 0;
        }
    }

    if (is_ctrl) {
        if (w_param == VK_OEM_PLUS || w_param == VK_ADD) {
            update_floater_font_size(1);
            return 0;
        } else if (w_param == VK_OEM_MINUS || w_param == VK_SUBTRACT) {
            update_floater_font_size(-1);
            return 0;
        }
    }

    if (w_param == VK_F2) {
        SendMessageW(hwnd, WM_RBUTTONUP, 0, 0);
        return 0;
    }

    if (!is_ctrl && !is_alt) {
        if (w_param == 'C') {
            show_commandbox_window();
            return 0;
        } else if (w_param == 'T') {
            PostMessageW(hwnd, WM_HOTKEY, 1, 0);
            return 0;
        }
    }



    if (msg == WM_SYSKEYDOWN)
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}

static BOOL floater_dispatch_monitor_command(UINT cmd)
{
    if (cmd < HG_IDM_MONITOR_BASE || cmd >= HG_IDM_MONITOR_BASE + HG_MAX_MONITORS)
        return FALSE;

    int idx = (int)(cmd - HG_IDM_MONITOR_BASE);
    if (idx >= 0 && idx < hg_g_monitor_count) {
        toggle_monitor_window(idx);
    }
    return TRUE;
}

static BOOL floater_dispatch_audio_device_command(UINT cmd)
{
    if (cmd < HG_IDM_AUDIO_DEVICE_BASE || cmd >= HG_IDM_AUDIO_DEVICE_BASE + HG_MAX_AUDIO_DEVICES)
        return FALSE;

    int idx = (int)(cmd - HG_IDM_AUDIO_DEVICE_BASE);
    if (idx >= 0 && idx < hg_g_audio_device_count) {
        if (set_default_audio_device(hg_g_audio_devices[idx].id)) {
            update_audio_device_list();
        }
    }
    return TRUE;
}

static BOOL floater_dispatch_volume_command(UINT cmd)
{
    int volume = -1;
    switch (cmd) {
    case HG_IDM_VOLUME_SET_0:
        volume = 0;
        break;
    case HG_IDM_VOLUME_SET_25:
        volume = 25;
        break;
    case HG_IDM_VOLUME_SET_50:
        volume = 50;
        break;
    case HG_IDM_VOLUME_SET_75:
        volume = 75;
        break;
    case HG_IDM_VOLUME_SET_100:
        volume = 100;
        break;
    default:
        return FALSE;
    }

    set_system_volume(volume);
    return TRUE;
}

static void floater_refresh_volume_ui(void)
{
    update_toolbar_tooltips(hg_g_toolbar_wnd);
    if (hg_g_toolbar_wnd) {
        InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
    }
}

static LRESULT floater_controller_on_command(HWND hwnd, WPARAM w_param, LPARAM l_param)
{
    UINT cmd = LOWORD(w_param);

    BOOL handled = floater_dispatch_monitor_command(cmd) || floater_dispatch_audio_device_command(cmd) ||
                   floater_dispatch_volume_command(cmd);
    if (!handled) {
        if (cmd == HG_IDM_CLOSE_APP) {
            DestroyWindow(hwnd);
        } else if (cmd == HG_IDM_MUTE) {
            set_system_mute(!get_system_mute());
            floater_refresh_volume_ui();
        } else if (cmd == HG_IDM_OPEN_SHORTCUTS) {
            ShellExecuteW(NULL, L"open", hg_g_shortcuts_path, NULL, NULL, SW_SHOWNORMAL);
        } else if (cmd == HG_IDM_EDIT_CONFIG) {
            ShellExecuteW(NULL, L"open", L"notepad.exe", hg_g_config_path, NULL, SW_SHOWNORMAL);
        } else if (cmd == HG_IDM_ABOUT) {
            show_about_window();
        } else if (cmd == HG_IDM_RESET_ALL) {
            hg_config_reset_all(hwnd);
        } else if (cmd == HG_IDM_FONT_UP) {
            update_floater_font_size(1);
        } else if (cmd == HG_IDM_FONT_DOWN) {
            update_floater_font_size(-1);
        } else if (cmd == HG_IDM_POWER_OFF) {
            HWND h_shell = FindWindowW(L"Shell_TrayWnd", NULL);
            if (h_shell)
                PostMessageW(h_shell, WM_COMMAND, 506, 0);
        }
    }

    return DefWindowProcW(hwnd, WM_COMMAND, w_param, l_param);
}

static LRESULT floater_controller_on_destroy(HWND hwnd)
{
    DeregisterShellHookWindow(hwnd);
    KillTimer(hwnd, HG_TIMER_FLOATER_CLOCK);
    KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
    if (hg_g_about_wnd && IsWindow(hg_g_about_wnd)) {
        DestroyWindow(hg_g_about_wnd);
        hg_g_about_wnd = NULL;
    }
    if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
        DestroyWindow(hg_g_taskbox_wnd);
        hg_g_taskbox_wnd = NULL;
    }
    unregister_global_hotkey(hwnd);
    release_font_handle(&hg_g_floater_time_font, FALSE);
    release_font_handle(&hg_g_floater_date_font, FALSE);
    release_font_handle(&s_floater_label_font, FALSE);
    release_font_handle(&s_floater_host_font, FALSE);
    hg_g_floater_wnd = NULL;
    PostQuitMessage(0);
    return 0;
}

LRESULT CALLBACK floater_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static int initial_floater_font_size = 0;
    static BOOL font_drag_active = FALSE;
    switch (msg) {
    case WM_DISPLAYCHANGE: {
        update_monitor_enum();
        /* Recover widgets stranded on a removed or rearranged display. */
        ensure_window_visible(hg_g_floater_wnd, L"floater");
        ensure_window_visible(hg_g_taskbox_wnd, L"taskbox");
        ensure_window_visible(hg_g_commandbox_wnd, L"commandbox");
        /* Repaint everything after a display change; stale layered content can
         * otherwise linger looking washed out. */
        InvalidateRect(hg_g_floater_wnd, NULL, TRUE);
        if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
            InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
            if (hg_g_toolbar_wnd) {
                InvalidateRect(hg_g_toolbar_wnd, NULL, TRUE);
            }
        }
        return 0;
    }
    case WM_DPICHANGED: {
        /* The floater/taskbox pair is co-located, so it owns the process scale. */
        hg_update_scale_from_dpi(LOWORD(w_param));
        hg_apply_dpi_suggested_rect(hwnd, l_param);
        release_font_handle(&hg_g_floater_time_font, FALSE);
        release_font_handle(&hg_g_floater_date_font, FALSE);
        release_font_handle(&s_floater_label_font, FALSE);
        release_font_handle(&s_floater_host_font, FALSE);
        update_floater_layout(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
            update_layout(hg_g_taskbox_wnd);
            InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
        }
        return 0;
    }
    case WM_MOUSEACTIVATE: {
        if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd))
            return MA_NOACTIVATE;
        return MA_ACTIVATE;
    }
    case WM_ACTIVATE: {
        if (LOWORD(w_param) != WA_INACTIVE) {
            if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                SetForegroundWindow(hg_g_taskbox_wnd);
            }
        }
        return 0;
    }
    case WM_CREATE:
        return floater_controller_on_create(hwnd);
    case WM_PAINT:
        return floater_controller_on_paint(hwnd);
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        return floater_controller_on_keydown(hwnd, msg, w_param, l_param);
    case WM_LBUTTONDOWN: {
        hg_g_drag_start_pt.x = GET_X_LPARAM(l_param);
        hg_g_drag_start_pt.y = GET_Y_LPARAM(l_param);
        font_drag_active = FALSE;
        if (GetKeyState(VK_CONTROL) < 0) {
            initial_floater_font_size = hg_g_floater_font_size;
        }
        SetCapture(hwnd);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (w_param & MK_LBUTTON && GetCapture() == hwnd) {
            POINT pt = {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};
            if (GetKeyState(VK_CONTROL) < 0) {
                if (pt.x != hg_g_drag_start_pt.x || pt.y != hg_g_drag_start_pt.y) {
                    font_drag_active = TRUE;
                }
                int delta = (pt.x - hg_g_drag_start_pt.x) / 5;
                int new_size = initial_floater_font_size + delta;
                if (new_size < HG_FLOATER_MIN_FONT_SIZE)
                    new_size = HG_FLOATER_MIN_FONT_SIZE;
                if (new_size > HG_FLOATER_MAX_FONT_SIZE)
                    new_size = HG_FLOATER_MAX_FONT_SIZE;
                if (hg_g_floater_font_size != new_size) {
                    hg_g_floater_font_size = new_size;
                    release_font_handle(&hg_g_floater_time_font, FALSE);
                    release_font_handle(&hg_g_floater_date_font, FALSE);
                    release_font_handle(&s_floater_label_font, FALSE);
                    release_font_handle(&s_floater_host_font, FALSE);
                    update_floater_layout(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE);
                    save_floater_font_config();
                }
            }
        } else if (!hg_g_floater_adjust_mode) {
            // Hover logic (suppressed in F floater-adjust mode so Ctrl/Alt+Wheel can
            // tune size/alpha without the floater expanding into the taskbox)
            if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd) && !IsWindowVisible(hg_g_taskbox_wnd)) {
                hg_expand_taskbox_from_floater(hwnd, hg_g_taskbox_wnd);
                // Make it appear instantly, refresh without forcing icon reload
                refresh_window_list(FALSE);
                ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                ShowWindow(hwnd, SW_HIDE);
                /* After a focus-preserving auto-collapse this process may not own
                 * the foreground anymore, so a plain SetForegroundWindow would be
                 * refused and keys would keep going to the other application. */
                hg_force_foreground(hg_g_taskbox_wnd);
                SetFocus(hg_g_toolbar_wnd);
                hg_g_hover_check_armed = TRUE;
                SetTimer(hg_g_taskbox_wnd, HG_TIMER_HOVER_CHECK, 100, NULL);
            }
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if (GetCapture() == hwnd) {
            ReleaseCapture();
            hg_g_floater_adjust_mode = FALSE;   /* a click leaves floater-adjust mode */
            if (font_drag_active) {
                /* Releasing a Ctrl+drag font-resize gesture must not toggle the taskbox. */
                font_drag_active = FALSE;
                return 0;
            }
            if (hg_g_taskbox_wnd && IsWindow(hg_g_taskbox_wnd)) {
                if (IsWindowVisible(hg_g_taskbox_wnd)) {
                    hide_taskbox(hg_g_taskbox_wnd);
                } else {
                    PostMessageW(hwnd, WM_HOTKEY, 1, 0);   /* expand back to the taskbox */
                }
            }
        }
        return 0;
    }
    /* WM_RBUTTONUP removed per user request */
    case WM_COMMAND:
        return floater_controller_on_command(hwnd, w_param, l_param);
    case WM_MOUSEWHEEL: {
        if (GetKeyState(VK_MENU) < 0) {
            short delta = (short)HIWORD(w_param);
            update_floater_alpha(delta > 0 ? 1 : -1);
        } else if (GetKeyState(VK_CONTROL) < 0) {
            short delta = (short)HIWORD(w_param);
            update_floater_font_size(delta > 0 ? 1 : -1);
        }
        return 0;
    }
    case WM_NCHITTEST: {
        return HTCLIENT;
    }
    case WM_ENTERSIZEMOVE: {
        hg_g_in_sizemove = TRUE;
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
    case WM_EXITSIZEMOVE: {
        hg_g_in_sizemove = FALSE;
        RECT rc = {0};
        GetWindowRect(hwnd, &rc);
        save_floater_geometry_config(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
        return 0;
    }
    case WM_TIMER: {
        if (w_param == HG_TIMER_FLOATER_CLOCK) {
            static SYSTEMTIME last_st = {0};
            SYSTEMTIME st;
            GetLocalTime(&st);
            if (st.wMinute != last_st.wMinute || st.wHour != last_st.wHour || st.wDay != last_st.wDay ||
                st.wMonth != last_st.wMonth || st.wYear != last_st.wYear) {
                last_st = st;
                update_floater_layout(hwnd);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            if (IsWindowVisible(hwnd) && floater_refresh_stats()) {
                InvalidateRect(hwnd, NULL, FALSE); /* fixed panel width: repaint only */
            }
            /* Periodic revalidation: layered-window content can go stale after
             * display sleep or DWM hiccups and then looks washed out until the
             * next repaint, and both widgets can otherwise go minutes without
             * one. Force a full repaint of whichever is visible every few
             * seconds so any such state heals itself. */
            static int revalidate_ticks = 0;
            if (++revalidate_ticks >= 5) {
                revalidate_ticks = 0;
                if (IsWindowVisible(hwnd)) {
                    InvalidateRect(hwnd, NULL, TRUE);
                }
                if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                    InvalidateRect(hg_g_taskbox_wnd, NULL, TRUE);
                    if (hg_g_toolbar_wnd) {
                        InvalidateRect(hg_g_toolbar_wnd, NULL, TRUE);
                    }
                }
            }
            for (int i = 0; i < hg_g_monitor_count; i++) {
                if (hg_g_monitors[i].active && hg_g_monitors[i].hwnd) {
                    InvalidateRect(hg_g_monitors[i].hwnd, NULL, FALSE);
                }
            }
        } else if (w_param == HG_TIMER_HIGHLIGHT) {
            hg_g_floater_highlight_ticks--;
            if (hg_g_floater_highlight_ticks <= 0) {
                KillTimer(hwnd, HG_TIMER_HIGHLIGHT);
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_HOTKEY: {
        if (w_param == 1) {
            HWND fg_wnd = GetForegroundWindow();
            if (fg_wnd && fg_wnd != hg_g_taskbox_wnd && fg_wnd != hg_g_floater_wnd &&
                fg_wnd != hg_g_about_wnd) {
                hg_g_prev_active_hwnd = fg_wnd;
            }

            ensure_window_visible(hg_g_floater_wnd, L"floater");
            ensure_window_visible(hg_g_taskbox_wnd, L"taskbox");

            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            hg_g_floater_highlight_ticks = HG_HIGHLIGHT_TICKS;
            if (!SetTimer(hwnd, HG_TIMER_HIGHLIGHT, 100, NULL)) {
                hg_g_floater_highlight_ticks = 0;
            }
            InvalidateRect(hwnd, NULL, FALSE);

            if (hg_g_taskbox_wnd) {
                if (!IsWindowVisible(hg_g_taskbox_wnd)) {
                    hg_expand_taskbox_from_floater(hwnd, hg_g_taskbox_wnd);

                    refresh_window_list(FALSE);
                    ShowWindow(hg_g_taskbox_wnd, SW_SHOW);
                } else {
                    refresh_window_list(FALSE);
                }
                hg_force_foreground(hg_g_taskbox_wnd);
                
                ShowWindow(hwnd, SW_HIDE); // Hide floater just like hover
                
                hg_g_hover_check_armed = FALSE;
                GetCursorPos(&hg_g_last_mouse_pos);
                SetTimer(hg_g_taskbox_wnd, HG_TIMER_HOVER_CHECK, 100, NULL);

                hg_g_taskbox_highlight_ticks = HG_HIGHLIGHT_TICKS;
                if (!SetTimer(hg_g_taskbox_wnd, HG_TIMER_HIGHLIGHT, 100, NULL)) {
                    hg_g_taskbox_highlight_ticks = 0;
                }
                InvalidateRect(hg_g_taskbox_wnd, NULL, FALSE);

                SetFocus(hg_g_toolbar_wnd);
                reset_taskbox_focus();

                InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
            } else {
                hg_force_foreground(hwnd);
            }
        }
        return 0;
    }
    case WM_COPYDATA:
        return handle_copydata_command_line((const COPYDATASTRUCT *)l_param);
    case WM_DESTROY:
        return floater_controller_on_destroy(hwnd);
    default: {
        if (hg_g_shellhook_msg && msg == hg_g_shellhook_msg) {
            if (w_param == HSHELL_REDRAW) {
                HWND target_hwnd = (HWND)l_param;
                BOOL found = FALSE;
                for (int i = 0; i < hg_g_window_count; i++) {
                    if (hg_g_window_items[i].hwnd == target_hwnd) {
                        release_window_item_icon(&hg_g_window_items[i]);
                        hg_g_window_items[i].icon =
                            get_window_icon(target_hwnd, ABS(hg_g_current_font_size), &hg_g_window_items[i].own_icon);
                        found = TRUE;
                        break;
                    }
                }
                if (found && hg_g_toolbar_wnd && hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                    InvalidateRect(hg_g_toolbar_wnd, NULL, FALSE);
                }
            } else if (w_param == HSHELL_WINDOWCREATED || w_param == HSHELL_WINDOWDESTROYED ||
                       w_param == HSHELL_WINDOWREPLACED) {
                if (hg_g_taskbox_wnd && IsWindowVisible(hg_g_taskbox_wnd)) {
                    refresh_window_list(FALSE);
                }
            }
            return 0;
        }
        break;
    }
    }
    return DefWindowProcW(hwnd, msg, w_param, l_param);
}
