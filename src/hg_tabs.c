/* Tabs as task icons, through UI Automation.
 *
 * See docs/RFC-2026-07-tabs-as-task-icons.md. The three rules that shape every
 * function here:
 *
 * - Off is free. When the setting is off nothing in this file creates a COM
 *   object or makes a call, because a feature that costs something while
 *   switched off is not switched off.
 * - Ask only where it can pay. A class-name test gates every cross-process
 *   call, so the desktop's other thirty windows cost a string compare each.
 * - Never hold a foreign pointer. Elements are found, used, and released
 *   within one call; a tab is addressed by index and re-found when clicked,
 *   because an element pointer across five seconds may no longer mean what it
 *   did when the page navigates. */
#define CINTERFACE
#define COBJMACROS
#include "hg_tabs.h"
#include "hg_utils.h"
#include "hg_globals.h"
#include <uiautomation.h>

static BOOL s_enabled_read = FALSE;
static BOOL s_enabled = FALSE;
static IUIAutomation *s_automation = NULL;
static BOOL s_automation_failed = FALSE;

BOOL hg_tabs_enabled(void)
{
    if (!s_enabled_read) {
        s_enabled_read = TRUE;
        s_enabled = (GetPrivateProfileIntW(L"taskbox", L"show_tabs", 0, hg_g_config_path) != 0);
    }
    return s_enabled;
}

void hg_tabs_set_enabled(BOOL enabled)
{
    (void)hg_tabs_enabled(); /* read the file before overwriting the cache */
    s_enabled = enabled ? TRUE : FALSE;
    WritePrivateProfileStringW(L"taskbox", L"show_tabs", s_enabled ? L"1" : L"0", hg_g_config_path);

    if (!s_enabled)
        hg_tabs_shutdown(); /* off gives the COM object back rather than idling on it */
}

/* The window classes worth asking. Nothing here is specific to browsers - a
 * tabbed application is a tabbed application - but the list has to exist,
 * because asking every window on the desktop for its tabs is how a per-window
 * cost becomes the cost of running the program.
 *
 * An unlisted application keeps the behaviour it has now, one icon for one
 * window, which is not a regression. And because a list compiled into the
 * program can only ever be out of date, `[taskbox] tab_classes` in config.ini
 * adds to it without a rebuild. */
static const WCHAR *const hg_tab_classes[] = {
    L"Chrome_WidgetWin_1",             /* Chromium: Chrome, Edge, Brave, Opera, and Electron apps */
    L"CabinetWClass",                  /* Explorer, which grew tabs in Windows 11 */
    L"MozillaWindowClass",             /* Firefox */
    L"CASCADIA_HOSTING_WINDOW_CLASS",  /* Windows Terminal */
    L"Notepad",                        /* Notepad, which grew tabs in Windows 11 */
};

static BOOL hg_tabs_class_listed_extra(const WCHAR *class_name)
{
    /* Read once. A list of class names is not something that changes while the
     * program runs, and re-reading a file per window per pass would undo the
     * point of having a cheap gate at all. */
    static BOOL read = FALSE;
    static WCHAR extra[512];
    if (!read) {
        read = TRUE;
        GetPrivateProfileStringW(L"taskbox", L"tab_classes", L"", extra, (DWORD)HG_ARRAYSIZE(extra),
                                 hg_g_config_path);
    }
    if (!extra[0])
        return FALSE;

    /* Semicolon-separated, spaces around a name ignored. */
    const WCHAR *cursor = extra;
    while (*cursor) {
        while (*cursor == L' ' || *cursor == L';')
            ++cursor;
        const WCHAR *start = cursor;
        while (*cursor && *cursor != L';')
            ++cursor;
        const WCHAR *end = cursor;
        while (end > start && end[-1] == L' ')
            --end;

        size_t len = (size_t)(end - start);
        if (len > 0 && len == wcslen(class_name) && _wcsnicmp(start, class_name, len) == 0)
            return TRUE;
    }
    return FALSE;
}

BOOL hg_tabs_window_may_have_tabs(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return FALSE;

    WCHAR class_name[64];
    if (GetClassNameW(hwnd, class_name, (int)HG_ARRAYSIZE(class_name)) <= 0)
        return FALSE;

    for (size_t i = 0; i < HG_ARRAYSIZE(hg_tab_classes); ++i) {
        if (lstrcmpiW(class_name, hg_tab_classes[i]) == 0)
            return TRUE;
    }
    return hg_tabs_class_listed_extra(class_name);
}

static IUIAutomation *hg_tabs_automation(void)
{
    if (s_automation || s_automation_failed)
        return s_automation;

    /* The apartment is whatever the process already initialised; this asks for
     * the object rather than deciding the threading model, the same way the
     * audio and WMI clients do. */
    HRESULT hr = CoCreateInstance(&CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, &IID_IUIAutomation,
                                  (void **)&s_automation);
    if (FAILED(hr) || !s_automation) {
        s_automation = NULL;
        s_automation_failed = TRUE; /* asked once; not once per refresh forever */
    }
    return s_automation;
}

static IUIAutomationCondition *hg_tabs_type_condition(IUIAutomation *automation, int control_type)
{
    VARIANT wanted;
    VariantInit(&wanted);
    V_VT(&wanted) = VT_I4;
    V_I4(&wanted) = control_type;

    IUIAutomationCondition *condition = NULL;
    if (FAILED(IUIAutomation_CreatePropertyCondition(automation, UIA_ControlTypePropertyId, wanted, &condition)))
        condition = NULL;
    VariantClear(&wanted);
    return condition;
}

/* The window's tab strip, found by where its tabs are rather than by what the
 * container calls itself.
 *
 * Two earlier attempts were wrong in opposite directions. Asking for every
 * TabItem below the window collected Explorer's Home page sections - Favourites,
 * Recent, Shared - which are real tabs of the page and not of the window. Then
 * requiring them to sit under a UIA_TabControlTypeId parent lost Explorer's real
 * tabs entirely, because what a XAML tab strip publishes as its container is not
 * something we get to decide.
 *
 * What is reliably true is where the strip is: a window's tabs are at the top of
 * the window, above everything, because that is what makes them the window's.
 * So the filter is geometric and applied to the tab items themselves, and no
 * assumption is made about their parent at all.
 *
 * Elements come back AddRef'd; the caller releases every one. */
static int hg_tabs_collect(HWND hwnd, IUIAutomationElement **out, int max)
{
    IUIAutomation *automation = hg_tabs_automation();
    if (!automation || !out || max <= 0)
        return 0;

    RECT window_rc;
    if (!GetWindowRect(hwnd, &window_rc))
        return 0;
    LONG window_height = window_rc.bottom - window_rc.top;
    if (window_height <= 0)
        return 0;
    /* The upper quarter. An address bar and a toolbar still fit above the page,
     * so a page's own tabs land below this line while the window's do not. */
    LONG strip_limit = window_rc.top + window_height / 4;

    IUIAutomationElement *root = NULL;
    if (FAILED(IUIAutomation_ElementFromHandle(automation, hwnd, &root)) || !root)
        return 0;

    IUIAutomationCondition *condition = hg_tabs_type_condition(automation, UIA_TabItemControlTypeId);
    IUIAutomationElementArray *found = NULL;
    if (condition) {
        if (FAILED(IUIAutomationElement_FindAll(root, TreeScope_Descendants, condition, &found)))
            found = NULL;
        IUIAutomationCondition_Release(condition);
    }
    IUIAutomationElement_Release(root);
    if (!found)
        return 0;

    LONG lefts[HG_TABS_MAX_PER_WINDOW];
    int count = 0;
    int length = 0;
    if (SUCCEEDED(IUIAutomationElementArray_get_Length(found, &length))) {
        for (int i = 0; i < length && count < max; ++i) {
            IUIAutomationElement *element = NULL;
            if (FAILED(IUIAutomationElementArray_GetElement(found, i, &element)) || !element)
                continue;

            RECT bounds;
            if (FAILED(IUIAutomationElement_get_CurrentBoundingRectangle(element, &bounds)) ||
                bounds.top > strip_limit || bounds.right <= bounds.left) {
                IUIAutomationElement_Release(element);
                continue;
            }

            /* Left to right, which is the order the reader sees and therefore
             * the order the tab numbers have to be in. Tree order usually
             * matches; usually is not a guarantee worth resting on. */
            int at = count;
            while (at > 0 && lefts[at - 1] > bounds.left) {
                lefts[at] = lefts[at - 1];
                out[at] = out[at - 1];
                --at;
            }
            lefts[at] = bounds.left;
            out[at] = element;
            ++count;
        }
    }
    IUIAutomationElementArray_Release(found);
    return count;
}

static void hg_tabs_release(IUIAutomationElement **elements, int count)
{
    for (int i = 0; i < count; ++i) {
        if (elements[i])
            IUIAutomationElement_Release(elements[i]);
    }
}

int hg_tabs_enumerate(HWND hwnd, WCHAR titles[][HG_MAX_STR], int max)
{
    if (!hg_tabs_enabled() || !titles || max <= 0)
        return 0;
    if (!hg_tabs_window_may_have_tabs(hwnd))
        return 0;

    IUIAutomationElement *tabs[HG_TABS_MAX_PER_WINDOW];
    int count = hg_tabs_collect(hwnd, tabs, (max < HG_TABS_MAX_PER_WINDOW) ? max : HG_TABS_MAX_PER_WINDOW);

    for (int i = 0; i < count; ++i) {
        BSTR name = NULL;
        if (SUCCEEDED(IUIAutomationElement_get_CurrentName(tabs[i], &name)) && name) {
            StringCchCopyW(titles[i], HG_MAX_STR, name);
            SysFreeString(name);
        } else {
            StringCchCopyW(titles[i], HG_MAX_STR, L"(tab)");
        }
    }

    hg_tabs_release(tabs, count);
    return count;
}

BOOL hg_tabs_activate(HWND hwnd, int tab_index)
{
    if (!hg_tabs_enabled() || tab_index < 0)
        return FALSE;

    IUIAutomationElement *tabs[HG_TABS_MAX_PER_WINDOW];
    int count = hg_tabs_collect(hwnd, tabs, HG_TABS_MAX_PER_WINDOW);

    BOOL selected = FALSE;
    if (tab_index < count) {
        IUIAutomationSelectionItemPattern *pattern = NULL;
        if (SUCCEEDED(IUIAutomationElement_GetCurrentPatternAs(tabs[tab_index], UIA_SelectionItemPatternId,
                                                               &IID_IUIAutomationSelectionItemPattern,
                                                               (void **)&pattern)) &&
            pattern) {
            /* A real API call rather than a synthesised click: it needs neither
             * the foreground nor the pointer. */
            selected = SUCCEEDED(IUIAutomationSelectionItemPattern_Select(pattern));
            IUIAutomationSelectionItemPattern_Release(pattern);
        }
    }
    hg_tabs_release(tabs, count);

    /* Both halves are needed: selecting a tab inside a window that is behind
     * three others is not what the reader asked for. */
    if (IsWindow(hwnd)) {
        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
    }
    return selected;
}

/* The tab's own close button, invoked the way a click would invoke it.
 *
 * Not Ctrl+W: that is key injection, it needs the window focused, and it acts
 * on whatever tab is current rather than the one that was asked for. The button
 * is right there in the tree and it closes exactly the tab it belongs to. */
BOOL hg_tabs_close(HWND hwnd, int tab_index)
{
    if (!hg_tabs_enabled() || tab_index < 0)
        return FALSE;

    IUIAutomation *automation = hg_tabs_automation();
    if (!automation)
        return FALSE;

    IUIAutomationElement *tabs[HG_TABS_MAX_PER_WINDOW];
    int count = hg_tabs_collect(hwnd, tabs, HG_TABS_MAX_PER_WINDOW);
    if (tab_index >= count) {
        hg_tabs_release(tabs, count);
        return FALSE;
    }

    IUIAutomationCondition *condition = hg_tabs_type_condition(automation, UIA_ButtonControlTypeId);
    IUIAutomationElementArray *buttons = NULL;
    if (condition) {
        if (FAILED(IUIAutomationElement_FindAll(tabs[tab_index], TreeScope_Descendants, condition, &buttons)))
            buttons = NULL;
        IUIAutomationCondition_Release(condition);
    }
    hg_tabs_release(tabs, count);
    if (!buttons)
        return FALSE;

    /* The rightmost button, when there is more than one. Matching the name
     * against "Close" would work in English and nowhere else; where the close
     * control sits is the same in every language. */
    int button_count = 0;
    IUIAutomationElement *best = NULL;
    LONG best_left = 0;
    if (SUCCEEDED(IUIAutomationElementArray_get_Length(buttons, &button_count))) {
        for (int i = 0; i < button_count; ++i) {
            IUIAutomationElement *element = NULL;
            if (FAILED(IUIAutomationElementArray_GetElement(buttons, i, &element)) || !element)
                continue;

            RECT bounds;
            if (SUCCEEDED(IUIAutomationElement_get_CurrentBoundingRectangle(element, &bounds)) &&
                (!best || bounds.left > best_left)) {
                if (best)
                    IUIAutomationElement_Release(best);
                best = element;
                best_left = bounds.left;
                continue;
            }
            IUIAutomationElement_Release(element);
        }
    }
    IUIAutomationElementArray_Release(buttons);
    if (!best)
        return FALSE;

    BOOL closed = FALSE;
    IUIAutomationInvokePattern *invoke = NULL;
    if (SUCCEEDED(IUIAutomationElement_GetCurrentPatternAs(best, UIA_InvokePatternId, &IID_IUIAutomationInvokePattern,
                                                           (void **)&invoke)) &&
        invoke) {
        closed = SUCCEEDED(IUIAutomationInvokePattern_Invoke(invoke));
        IUIAutomationInvokePattern_Release(invoke);
    }
    IUIAutomationElement_Release(best);
    return closed;
}

void hg_tabs_shutdown(void)
{
    if (s_automation) {
        IUIAutomation_Release(s_automation);
        s_automation = NULL;
    }
    s_automation_failed = FALSE;
}
