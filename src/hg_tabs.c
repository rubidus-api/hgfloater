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

/* The tabbed shells worth asking. Incomplete on purpose: an unlisted
 * application keeps the behaviour it has now - one icon for one window, which
 * is not a regression - and asking every window on the desktop for its tabs is
 * how a per-window cost becomes the cost of running the program. */
BOOL hg_tabs_window_may_have_tabs(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd))
        return FALSE;

    WCHAR class_name[64];
    if (GetClassNameW(hwnd, class_name, (int)HG_ARRAYSIZE(class_name)) <= 0)
        return FALSE;

    /* Chromium's window class covers Chrome, Edge, Brave, Opera and the rest;
     * CabinetWClass is Explorer, which grew tabs in Windows 11. */
    return lstrcmpiW(class_name, L"Chrome_WidgetWin_1") == 0 || lstrcmpiW(class_name, L"CabinetWClass") == 0;
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

/* The window's own tab strip, which is not the only tab control in the window.
 *
 * Sweeping the whole tree for TabItem was the first attempt and it was wrong:
 * Explorer's Home page has a tab strip of its own - Favourites, Recent, Shared -
 * and those are real TabItems that have nothing to do with the window's tabs.
 * Asking for TabItem anywhere in the window is asking the wrong question.
 *
 * So: find the tab *controls*, and take the one at the top of the window. A
 * window's tab strip sits above everything, by definition of what it is;
 * anything further down belongs to the content. The upper-quarter test is what
 * makes "there is no window tab strip here" an answer rather than a wrong
 * guess - Explorer showing only its Home page then contributes no tabs, which
 * is exactly right, because it has none. */
static IUIAutomationElement *hg_tabs_find_strip(IUIAutomation *automation, IUIAutomationElement *root, HWND hwnd)
{
    RECT window_rc;
    if (!GetWindowRect(hwnd, &window_rc))
        return NULL;
    LONG window_height = window_rc.bottom - window_rc.top;
    if (window_height <= 0)
        return NULL;
    LONG strip_limit = window_rc.top + window_height / 4;

    IUIAutomationCondition *condition = hg_tabs_type_condition(automation, UIA_TabControlTypeId);
    if (!condition)
        return NULL;

    IUIAutomationElementArray *controls = NULL;
    if (FAILED(IUIAutomationElement_FindAll(root, TreeScope_Descendants, condition, &controls)) || !controls) {
        IUIAutomationCondition_Release(condition);
        return NULL;
    }
    IUIAutomationCondition_Release(condition);

    int length = 0;
    IUIAutomationElement *best = NULL;
    LONG best_top = 0;
    if (SUCCEEDED(IUIAutomationElementArray_get_Length(controls, &length))) {
        for (int i = 0; i < length; ++i) {
            IUIAutomationElement *element = NULL;
            if (FAILED(IUIAutomationElementArray_GetElement(controls, i, &element)) || !element)
                continue;

            RECT bounds;
            if (SUCCEEDED(IUIAutomationElement_get_CurrentBoundingRectangle(element, &bounds)) &&
                bounds.top <= strip_limit && (!best || bounds.top < best_top)) {
                if (best)
                    IUIAutomationElement_Release(best);
                best = element;
                best_top = bounds.top;
                continue;
            }
            IUIAutomationElement_Release(element);
        }
    }
    IUIAutomationElementArray_Release(controls);
    return best;
}

/* The tab strip's own children, and nothing else in the window. TreeScope_Children
 * rather than Descendants for the same reason: a tab that contains a control
 * which is itself a TabItem is not two tabs. */
static IUIAutomationElementArray *hg_tabs_find(HWND hwnd)
{
    IUIAutomation *automation = hg_tabs_automation();
    if (!automation)
        return NULL;

    IUIAutomationElement *root = NULL;
    if (FAILED(IUIAutomation_ElementFromHandle(automation, hwnd, &root)) || !root)
        return NULL;

    IUIAutomationElement *strip = hg_tabs_find_strip(automation, root, hwnd);
    IUIAutomationElement_Release(root);
    if (!strip)
        return NULL;

    IUIAutomationCondition *condition = hg_tabs_type_condition(automation, UIA_TabItemControlTypeId);
    IUIAutomationElementArray *found = NULL;
    if (condition) {
        if (FAILED(IUIAutomationElement_FindAll(strip, TreeScope_Children, condition, &found)))
            found = NULL;
        IUIAutomationCondition_Release(condition);
    }
    IUIAutomationElement_Release(strip);
    return found;
}

int hg_tabs_enumerate(HWND hwnd, WCHAR titles[][HG_MAX_STR], int max)
{
    if (!hg_tabs_enabled() || !titles || max <= 0)
        return 0;
    if (!hg_tabs_window_may_have_tabs(hwnd))
        return 0;

    IUIAutomationElementArray *found = hg_tabs_find(hwnd);
    if (!found)
        return 0;

    int length = 0;
    if (FAILED(IUIAutomationElementArray_get_Length(found, &length)) || length <= 0) {
        IUIAutomationElementArray_Release(found);
        return 0;
    }

    int count = 0;
    for (int i = 0; i < length && count < max; ++i) {
        IUIAutomationElement *element = NULL;
        if (FAILED(IUIAutomationElementArray_GetElement(found, i, &element)) || !element)
            continue;

        BSTR name = NULL;
        if (SUCCEEDED(IUIAutomationElement_get_CurrentName(element, &name)) && name) {
            StringCchCopyW(titles[count], HG_MAX_STR, name);
            SysFreeString(name);
        } else {
            StringCchCopyW(titles[count], HG_MAX_STR, L"(tab)");
        }
        ++count;
        IUIAutomationElement_Release(element);
    }

    IUIAutomationElementArray_Release(found);
    return count;
}

BOOL hg_tabs_activate(HWND hwnd, int tab_index)
{
    if (!hg_tabs_enabled() || tab_index < 0)
        return FALSE;

    IUIAutomationElementArray *found = hg_tabs_find(hwnd);
    if (!found)
        return FALSE;

    int length = 0;
    BOOL selected = FALSE;
    if (SUCCEEDED(IUIAutomationElementArray_get_Length(found, &length)) && tab_index < length) {
        IUIAutomationElement *element = NULL;
        if (SUCCEEDED(IUIAutomationElementArray_GetElement(found, tab_index, &element)) && element) {
            IUIAutomationSelectionItemPattern *pattern = NULL;
            if (SUCCEEDED(IUIAutomationElement_GetCurrentPatternAs(element, UIA_SelectionItemPatternId,
                                                                   &IID_IUIAutomationSelectionItemPattern,
                                                                   (void **)&pattern)) &&
                pattern) {
                /* A real API call rather than a synthesised click: it needs
                 * neither the foreground nor the pointer. */
                selected = SUCCEEDED(IUIAutomationSelectionItemPattern_Select(pattern));
                IUIAutomationSelectionItemPattern_Release(pattern);
            }
            IUIAutomationElement_Release(element);
        }
    }
    IUIAutomationElementArray_Release(found);

    /* Both halves are needed: selecting a tab inside a window that is behind
     * three others is not what the reader asked for. */
    if (IsWindow(hwnd)) {
        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
    }
    return selected;
}

void hg_tabs_shutdown(void)
{
    if (s_automation) {
        IUIAutomation_Release(s_automation);
        s_automation = NULL;
    }
    s_automation_failed = FALSE;
}
