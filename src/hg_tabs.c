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

/* The UI thread's own automation instance, for the one-shot user actions
 * (activate, close). The worker thread has its own; COM objects are not
 * shared across the apartment boundary. */
static IUIAutomation *s_automation = NULL;
static BOOL s_automation_failed = FALSE;

/* ------------------------------------------------------------- the worker
 *
 * One thread, owning its own MTA COM and its own IUIAutomation. The UI thread
 * talks to it through a pending set and a result table, both under one
 * critical section, and is never made to wait: file a request, keep drawing
 * from the cache, fold the answer in when HG_MSG_TABS_READY arrives.
 *
 * The lifecycle is a small explicit machine, because the implicit one had a
 * real defect: a worker stuck past the shutdown wait was "forgotten" by
 * resetting the stop flag, which is exactly what let it come back to life -
 * and a re-enable then started a second one.
 *
 *   STOPPED --enable--> RUNNING --disable--> (exits in time) --> STOPPED
 *                          |                     |
 *                          |                     +-- (stuck in a call) --> STOPPING
 *                          |                                                  |
 *                          +---- request while STOPPING: reap if the thread --+
 *                                has since exited, else drop the request;
 *                                stop stays set so the stuck thread exits the
 *                                moment its call returns. */
typedef enum HgTabsState {
    HG_TABS_STOPPED = 0,
    HG_TABS_RUNNING,
    HG_TABS_STOPPING, /* stop requested, thread not yet seen to exit */
} HgTabsState;

typedef struct HgTabsResult {
    HWND hwnd; /* NULL = empty slot */
    DWORD pid; /* of the window when it was asked; a recycled HWND fails this */
    int count;
    BOOL failed;      /* the ask broke; count is meaningless */
    DWORD elapsed_ms; /* self-measured cost of the whole ask */
    WCHAR provider;   /* L'U' = UIA; the byte stays for whatever comes next */
    WCHAR walk_note;  /* 's' scoped refresh, 'f' full discovery, '!' failed */
    BOOL fresh;       /* set by the worker, cleared by hg_tabs_take_result */
    WCHAR titles[HG_TABS_MAX_PER_WINDOW][HG_MAX_STR];
} HgTabsResult;

static CRITICAL_SECTION s_tabs_lock;
static BOOL s_tabs_lock_ready = FALSE; /* init/create only ever on the UI thread */
static HANDLE s_tabs_thread = NULL;
static HANDLE s_tabs_wake = NULL; /* auto-reset: "the pending set is non-empty" */
static volatile LONG s_tabs_stop = 0;
static HgTabsState s_tabs_state = HG_TABS_STOPPED; /* UI thread only */
static HWND s_tabs_request_hwnds[HG_TABS_WORKER_WINDOWS];
static int s_tabs_request_count = 0;
static HgTabsResult s_tabs_results[HG_TABS_WORKER_WINDOWS];

/* Observability, all under the lock. The delivery plan's exit criteria are
 * judged against these through `show tabs`, not against feelings. */
static unsigned s_stat_queued = 0;      /* windows accepted into the pending set */
static unsigned s_stat_completed = 0;   /* answers stored, failed or not */
static unsigned s_stat_failed = 0;      /* answers whose ask broke */
static unsigned s_stat_scoped = 0;      /* answers served by the scoped read */
static unsigned s_stat_slow = 0;        /* answers that took over 50 ms */
static unsigned s_stat_req_overflow = 0;  /* requests dropped: pending set full */
static unsigned s_stat_pass_overflow = 0; /* eligible windows past the table cap */
static unsigned s_stat_shutdown_timeouts = 0;

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
static int hg_tabs_collect(IUIAutomation *automation, HWND hwnd, IUIAutomationElement **out, int max)
{
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

/* MSAA was tried here and retired by two field runs (RFC-2026-08, Field
 * Result): Chromium answers with a real tree but per-element cross-process
 * chattiness means a bounded walk either dies on its clock or costs what the
 * one-call UIA walk costs; Explorer's XAML bridge never exposes a PAGETABLIST
 * at all. What remains below is UIA only, in two gears - a scoped read that
 * refreshes just the remembered tab-strip container, and the full-window
 * discovery walk that finds that container in the first place. */

static DWORD hg_tabs_now_ms(void)
{
    static LARGE_INTEGER s_freq; /* worker-only */
    if (!s_freq.QuadPart)
        QueryPerformanceFrequency(&s_freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (DWORD)((ULONGLONG)now.QuadPart * 1000u / (ULONGLONG)s_freq.QuadPart);
}

/* The cached bounding rectangle comes back as a VARIANT holding four doubles:
 * left, top, width, height. */
static BOOL hg_tabs_cached_rect(IUIAutomationElement *element, RECT *out)
{
    VARIANT value;
    VariantInit(&value);
    if (FAILED(IUIAutomationElement_GetCachedPropertyValue(element, UIA_BoundingRectanglePropertyId, &value)))
        return FALSE;

    BOOL ok = FALSE;
    if (V_VT(&value) == (VT_ARRAY | VT_R8) && V_ARRAY(&value)) {
        double *edges = NULL;
        if (SUCCEEDED(SafeArrayAccessData(V_ARRAY(&value), (void **)&edges))) {
            LONG lower = 0, upper = -1;
            SafeArrayGetLBound(V_ARRAY(&value), 1, &lower);
            SafeArrayGetUBound(V_ARRAY(&value), 1, &upper);
            if (upper - lower + 1 >= 4) {
                out->left = (LONG)edges[0];
                out->top = (LONG)edges[1];
                out->right = (LONG)(edges[0] + edges[2]);
                out->bottom = (LONG)(edges[1] + edges[3]);
                ok = TRUE;
            }
            SafeArrayUnaccessData(V_ARRAY(&value));
        }
    }
    VariantClear(&value);
    return ok;
}

/* Collect and read the titles in one sweep. Runs on whichever thread owns the
 * automation instance passed in - in practice the worker.
 *
 * Everything is fetched through a cache request: the name and the rectangle
 * ride back inside the FindAll answer itself, and AutomationElementMode_None
 * means no live cross-process reference is even created per element. The
 * uncached path costs two round trips per tab item on top of the walk; with a
 * browser holding a dozen tabs, that difference is the difference between one
 * cross-process call and twenty-five.
 *
 * Returns -1 when the ask itself broke - no provider, a dead element, a failed
 * walk - and 0 only when the walk genuinely answered "no tabs". The caller
 * keeps its previous answer on -1: a transient provider failure must not fold
 * a living fan-out back into one icon. */
static int hg_tabs_read_from(IUIAutomation *automation, IUIAutomationElement *scope, WCHAR titles[][HG_MAX_STR],
                             int max, LONG strip_limit)
{
    if (!automation || !scope || max <= 0)
        return -1;
    if (max > HG_TABS_MAX_PER_WINDOW)
        max = HG_TABS_MAX_PER_WINDOW;

    IUIAutomationCacheRequest *cache = NULL;
    if (FAILED(IUIAutomation_CreateCacheRequest(automation, &cache)) || !cache)
        return -1;
    IUIAutomationCacheRequest_AddProperty(cache, UIA_NamePropertyId);
    IUIAutomationCacheRequest_AddProperty(cache, UIA_BoundingRectanglePropertyId);
    IUIAutomationCacheRequest_put_AutomationElementMode(cache, AutomationElementMode_None);

    IUIAutomationCondition *condition = hg_tabs_type_condition(automation, UIA_TabItemControlTypeId);
    IUIAutomationElementArray *found = NULL;
    if (condition) {
        if (FAILED(IUIAutomationElement_FindAllBuildCache(scope, TreeScope_Descendants, condition, cache, &found)))
            found = NULL;
        IUIAutomationCondition_Release(condition);
    }
    IUIAutomationCacheRequest_Release(cache);
    if (!found)
        return -1; /* the walk failed; 0 is reserved for a real "no tabs" */

    LONG lefts[HG_TABS_MAX_PER_WINDOW];
    int count = 0;
    int length = 0;
    if (SUCCEEDED(IUIAutomationElementArray_get_Length(found, &length))) {
        for (int i = 0; i < length && count < max; ++i) {
            IUIAutomationElement *element = NULL;
            if (FAILED(IUIAutomationElementArray_GetElement(found, i, &element)) || !element)
                continue;

            RECT bounds;
            if (!hg_tabs_cached_rect(element, &bounds) || bounds.top > strip_limit ||
                bounds.right <= bounds.left) {
                IUIAutomationElement_Release(element);
                continue;
            }

            WCHAR title[HG_MAX_STR];
            VARIANT name;
            VariantInit(&name);
            if (SUCCEEDED(IUIAutomationElement_GetCachedPropertyValue(element, UIA_NamePropertyId, &name)) &&
                V_VT(&name) == VT_BSTR && V_BSTR(&name)) {
                StringCchCopyW(title, HG_ARRAYSIZE(title), V_BSTR(&name));
            } else {
                StringCchCopyW(title, HG_ARRAYSIZE(title), L"(tab)");
            }
            VariantClear(&name);
            IUIAutomationElement_Release(element);

            /* Left to right, the order the reader sees. */
            int at = count;
            while (at > 0 && lefts[at - 1] > bounds.left) {
                lefts[at] = lefts[at - 1];
                StringCchCopyW(titles[at], HG_MAX_STR, titles[at - 1]);
                --at;
            }
            lefts[at] = bounds.left;
            StringCchCopyW(titles[at], HG_MAX_STR, title);
            ++count;
        }
    }
    IUIAutomationElementArray_Release(found);
    return count;
}

/* The full-window walk: what discovery and the pre-B3 code always did. */
static int hg_tabs_read_titles(IUIAutomation *automation, HWND hwnd, WCHAR titles[][HG_MAX_STR], int max,
                               LONG strip_limit)
{
    if (!automation)
        return -1;
    IUIAutomationElement *root = NULL;
    if (FAILED(IUIAutomation_ElementFromHandle(automation, hwnd, &root)) || !root)
        return -1;
    int count = hg_tabs_read_from(automation, root, titles, max, strip_limit);
    IUIAutomationElement_Release(root);
    return count;
}

/* ------------------------------------------ the scoped read (RFC-2026-08 B3)
 *
 * The full walk asks the provider to consider the entire window - toolbars,
 * panes, and in Explorer's case the whole folder view - to find a strip that
 * sits in one small container. So the container's ADDRESS is remembered: the
 * property chain (control type, class, automation id) of each ancestor from
 * the window root down to the strip's parent. A refresh then descends that
 * chain with one one-level FindFirst per step and runs the tab query on the
 * container's own little subtree.
 *
 * The chain is a hint, never an identity: providers rebuild their trees, so
 * any miss at any step just invalidates the hint and the next ask pays for
 * one full discovery again. Worker-only state throughout. */
#define HG_TABS_PATH_MAX 10

typedef struct HgTabsPathStep {
    int control_type;
    WCHAR class_name[64];
    WCHAR automation_id[64];
} HgTabsPathStep;

typedef struct HgTabsHint {
    HWND hwnd; /* NULL = empty slot */
    DWORD pid;
    BOOL valid;
    int depth; /* steps, root's child first, container last */
    HgTabsPathStep steps[HG_TABS_PATH_MAX];
} HgTabsHint;

static HgTabsHint s_tabs_hints[HG_TABS_WORKER_WINDOWS]; /* worker-only */

static HgTabsHint *hg_tabs_hint_slot(HWND hwnd, DWORD pid, BOOL create)
{
    for (int i = 0; i < HG_TABS_WORKER_WINDOWS; ++i) {
        if (s_tabs_hints[i].hwnd == hwnd && s_tabs_hints[i].pid == pid)
            return &s_tabs_hints[i];
    }
    if (!create)
        return NULL;
    for (int i = 0; i < HG_TABS_WORKER_WINDOWS; ++i) {
        if (s_tabs_hints[i].hwnd == NULL || !IsWindow(s_tabs_hints[i].hwnd)) {
            SecureZeroMemory(&s_tabs_hints[i], sizeof(s_tabs_hints[i]));
            s_tabs_hints[i].hwnd = hwnd;
            s_tabs_hints[i].pid = pid;
            return &s_tabs_hints[i];
        }
    }
    static int s_next = 0;
    HgTabsHint *slot = &s_tabs_hints[s_next];
    s_next = (s_next + 1) % HG_TABS_WORKER_WINDOWS;
    SecureZeroMemory(slot, sizeof(*slot));
    slot->hwnd = hwnd;
    slot->pid = pid;
    return slot;
}

/* One step of the descent as a condition: control type always, class and
 * automation id when the step recorded them. */
static IUIAutomationCondition *hg_tabs_step_condition(IUIAutomation *automation, const HgTabsPathStep *step)
{
    IUIAutomationCondition *all = hg_tabs_type_condition(automation, step->control_type);
    if (!all)
        return NULL;

    for (int part = 0; part < 2; ++part) {
        const WCHAR *text = (part == 0) ? step->class_name : step->automation_id;
        PROPERTYID prop = (part == 0) ? UIA_ClassNamePropertyId : UIA_AutomationIdPropertyId;
        if (!text[0])
            continue;
        VARIANT value;
        VariantInit(&value);
        V_VT(&value) = VT_BSTR;
        V_BSTR(&value) = SysAllocString(text);
        IUIAutomationCondition *one = NULL;
        if (V_BSTR(&value) &&
            SUCCEEDED(IUIAutomation_CreatePropertyCondition(automation, prop, value, &one)) && one) {
            IUIAutomationCondition *joined = NULL;
            if (SUCCEEDED(IUIAutomation_CreateAndCondition(automation, all, one, &joined)) && joined) {
                IUIAutomationCondition_Release(all);
                IUIAutomationCondition_Release(one);
                all = joined;
            } else {
                IUIAutomationCondition_Release(one);
            }
        }
        VariantClear(&value);
    }
    return all;
}

static int hg_tabs_read_scoped(IUIAutomation *automation, HWND hwnd, const HgTabsHint *hint,
                               WCHAR titles[][HG_MAX_STR], int max, LONG strip_limit)
{
    if (!automation || !hint->valid || hint->depth <= 0)
        return -1;

    IUIAutomationElement *node = NULL;
    if (FAILED(IUIAutomation_ElementFromHandle(automation, hwnd, &node)) || !node)
        return -1;

    for (int i = 0; i < hint->depth && node; ++i) {
        IUIAutomationCondition *condition = hg_tabs_step_condition(automation, &hint->steps[i]);
        IUIAutomationElement *next = NULL;
        if (condition) {
            if (FAILED(IUIAutomationElement_FindFirst(node, TreeScope_Children, condition, &next)))
                next = NULL;
            IUIAutomationCondition_Release(condition);
        }
        IUIAutomationElement_Release(node);
        node = next; /* NULL = the path broke; the hint is stale */
    }
    if (!node)
        return -1;

    int count = hg_tabs_read_from(automation, node, titles, max, strip_limit);
    IUIAutomationElement_Release(node);
    /* 0 from a remembered container is suspicious - the strip may have moved
     * house. Report it as a miss so the caller re-discovers. */
    return (count <= 0) ? -1 : count;
}

/* After a successful full walk: find one live tab item, take its parent as
 * the container, and record the property chain from the window root down to
 * it. Costs a handful of round trips, paid only on discovery. */
static void hg_tabs_hint_rebuild(IUIAutomation *automation, HWND hwnd, DWORD pid)
{
    HgTabsHint *hint = hg_tabs_hint_slot(hwnd, pid, TRUE);
    hint->valid = FALSE;
    hint->depth = 0;

    IUIAutomationElement *root = NULL;
    if (FAILED(IUIAutomation_ElementFromHandle(automation, hwnd, &root)) || !root)
        return;

    IUIAutomationElement *tab = NULL;
    IUIAutomationCondition *condition = hg_tabs_type_condition(automation, UIA_TabItemControlTypeId);
    if (condition) {
        if (FAILED(IUIAutomationElement_FindFirst(root, TreeScope_Descendants, condition, &tab)))
            tab = NULL;
        IUIAutomationCondition_Release(condition);
    }
    if (!tab) {
        IUIAutomationElement_Release(root);
        return;
    }

    IUIAutomationTreeWalker *walker = NULL;
    if (FAILED(IUIAutomation_get_ControlViewWalker(automation, &walker)) || !walker) {
        IUIAutomationElement_Release(tab);
        IUIAutomationElement_Release(root);
        return;
    }

    /* Climb from the strip container toward the root, keeping the chain. */
    IUIAutomationElement *chain[HG_TABS_PATH_MAX];
    int chain_len = 0;
    BOOL reached_root = FALSE;

    IUIAutomationElement *node = NULL;
    if (FAILED(IUIAutomationTreeWalker_GetParentElement(walker, tab, &node)))
        node = NULL;
    IUIAutomationElement_Release(tab);

    while (node && chain_len < HG_TABS_PATH_MAX) {
        chain[chain_len++] = node; /* owned */
        IUIAutomationElement *parent = NULL;
        if (FAILED(IUIAutomationTreeWalker_GetParentElement(walker, node, &parent)))
            parent = NULL;
        if (!parent)
            break;
        BOOL same = FALSE;
        if (SUCCEEDED(IUIAutomation_CompareElements(automation, parent, root, &same)) && same) {
            IUIAutomationElement_Release(parent);
            reached_root = TRUE;
            break;
        }
        node = parent;
    }
    IUIAutomationTreeWalker_Release(walker);
    IUIAutomationElement_Release(root);

    if (reached_root && chain_len > 0) {
        /* chain[] runs container-first; the hint wants root's child first. */
        for (int i = 0; i < chain_len; ++i) {
            IUIAutomationElement *elem = chain[chain_len - 1 - i];
            HgTabsPathStep *step = &hint->steps[i];
            CONTROLTYPEID control_type = 0;
            if (SUCCEEDED(IUIAutomationElement_get_CurrentControlType(elem, &control_type)))
                step->control_type = (int)control_type;
            BSTR text = NULL;
            step->class_name[0] = L'\0';
            if (SUCCEEDED(IUIAutomationElement_get_CurrentClassName(elem, &text)) && text) {
                StringCchCopyW(step->class_name, HG_ARRAYSIZE(step->class_name), text);
                SysFreeString(text);
            }
            text = NULL;
            step->automation_id[0] = L'\0';
            if (SUCCEEDED(IUIAutomationElement_get_CurrentAutomationId(elem, &text)) && text) {
                StringCchCopyW(step->automation_id, HG_ARRAYSIZE(step->automation_id), text);
                SysFreeString(text);
            }
        }
        hint->depth = chain_len;
        hint->valid = TRUE;
    }
    for (int i = 0; i < chain_len; ++i)
        IUIAutomationElement_Release(chain[i]);
}

/* Store one window's answer. Slot choice: the window's existing slot, then an
 * empty one, then one whose window has died, then round-robin - the table is
 * as large as the request batch, so a live batch always fits. */
static void hg_tabs_store_result(HWND hwnd, DWORD pid, WCHAR provider, WCHAR walk_note, BOOL failed,
                                 DWORD elapsed_ms, const WCHAR titles[][HG_MAX_STR], int count)
{
    static int s_next = 0; /* worker-only */

    EnterCriticalSection(&s_tabs_lock);
    ++s_stat_completed;
    if (failed)
        ++s_stat_failed;
    if (walk_note == L's')
        ++s_stat_scoped;
    if (elapsed_ms > 50)
        ++s_stat_slow;

    HgTabsResult *slot = NULL;
    for (int i = 0; i < HG_TABS_WORKER_WINDOWS && !slot; ++i) {
        if (s_tabs_results[i].hwnd == hwnd)
            slot = &s_tabs_results[i];
    }
    for (int i = 0; i < HG_TABS_WORKER_WINDOWS && !slot; ++i) {
        if (s_tabs_results[i].hwnd == NULL || !IsWindow(s_tabs_results[i].hwnd))
            slot = &s_tabs_results[i];
    }
    if (!slot) {
        slot = &s_tabs_results[s_next];
        s_next = (s_next + 1) % HG_TABS_WORKER_WINDOWS;
    }

    slot->hwnd = hwnd;
    slot->pid = pid;
    slot->provider = provider;
    slot->walk_note = walk_note;
    slot->failed = failed;
    slot->elapsed_ms = elapsed_ms;
    slot->count = failed ? 0 : count;
    slot->fresh = TRUE;
    for (int i = 0; i < slot->count; ++i)
        StringCchCopyW(slot->titles[i], HG_MAX_STR, titles[i]);
    LeaveCriticalSection(&s_tabs_lock);
}

static DWORD WINAPI hg_tabs_worker(LPVOID unused)
{
    (void)unused;
    /* MTA, and an automation instance of this thread's own: the UIA client
     * guidance wants clients off the UI thread, and COM interface pointers do
     * not cross apartments. */
    BOOL co = SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED));
    IUIAutomation *automation = NULL;
    if (co) {
        if (FAILED(CoCreateInstance(&CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, &IID_IUIAutomation,
                                    (void **)&automation)))
            automation = NULL;
    }

    /* 48 KB of titles: static rather than a stack frame, and safe as such
     * because only this one worker thread ever runs this function. */
    static WCHAR s_titles[HG_TABS_MAX_PER_WINDOW][HG_MAX_STR];

    while (!s_tabs_stop) {
        WaitForSingleObject(s_tabs_wake, INFINITE);
        if (s_tabs_stop)
            break;

        HWND batch[HG_TABS_WORKER_WINDOWS];
        int batch_count = 0;
        EnterCriticalSection(&s_tabs_lock);
        batch_count = s_tabs_request_count;
        for (int i = 0; i < batch_count; ++i)
            batch[i] = s_tabs_request_hwnds[i];
        s_tabs_request_count = 0;
        LeaveCriticalSection(&s_tabs_lock);

        for (int i = 0; i < batch_count && !s_tabs_stop; ++i) {
            if (!IsWindow(batch[i]))
                continue;
            /* One window at a time, with air between them. The walk makes the
             * target build its accessibility tree, and firing that at every
             * browser window in the same instant is what a whole-system hitch
             * is made of; staggered, each window's cost lands alone. */
            if (i > 0)
                Sleep(150);

            DWORD pid = 0;
            GetWindowThreadProcessId(batch[i], &pid);

            RECT window_rc;
            if (!GetWindowRect(batch[i], &window_rc) || window_rc.bottom <= window_rc.top) {
                hg_tabs_store_result(batch[i], pid, L'U', L'-', TRUE, 0, s_titles, 0);
                continue;
            }
            /* The upper quarter: the band a window's own tab strip lives in. */
            LONG strip_limit = window_rc.top + (window_rc.bottom - window_rc.top) / 4;

            DWORD started = hg_tabs_now_ms();
            WCHAR walk_note = L'!';
            int count = -1;

            /* The scoped read first: descend the remembered property chain
             * and query only the strip container's little subtree. Any miss
             * invalidates the hint, and the full-window discovery answers -
             * rebuilding the hint on its way out, so the NEXT ask is scoped
             * again. Steady state is all scoped reads; discovery is paid per
             * provider rebuild, not per refresh. */
            HgTabsHint *hint = hg_tabs_hint_slot(batch[i], pid, FALSE);
            if (hint && hint->valid) {
                count = hg_tabs_read_scoped(automation, batch[i], hint, s_titles, HG_TABS_MAX_PER_WINDOW,
                                            strip_limit);
                if (count >= 1)
                    walk_note = L's';
                else
                    hint->valid = FALSE;
            }
            if (count < 1) {
                count = automation ? hg_tabs_read_titles(automation, batch[i], s_titles, HG_TABS_MAX_PER_WINDOW,
                                                         strip_limit)
                                   : -1;
                if (count >= 0)
                    walk_note = L'f';
                if (count >= 1)
                    hg_tabs_hint_rebuild(automation, batch[i], pid);
            }

            DWORD elapsed = hg_tabs_now_ms() - started;
            hg_tabs_store_result(batch[i], pid, L'U', walk_note, count < 0, elapsed, s_titles,
                                 (count < 0) ? 0 : count);
        }

        /* Even an all-zero answer is an answer: the list may be waiting to
         * drop a fan-out that no longer exists. */
        if (batch_count > 0 && hg_g_floater_wnd)
            PostMessageW(hg_g_floater_wnd, HG_MSG_TABS_READY, 0, 0);
    }

    if (automation)
        IUIAutomation_Release(automation);
    if (co)
        CoUninitialize();
    return 0;
}

/* Try to reap a STOPPING worker whose thread may have exited by now. UI
 * thread only. Returns TRUE when the machine is back in STOPPED. */
static BOOL hg_tabs_worker_reap(void)
{
    if (s_tabs_state != HG_TABS_STOPPING)
        return TRUE;
    if (WaitForSingleObject(s_tabs_thread, 0) != WAIT_OBJECT_0)
        return FALSE; /* still inside its call; stop stays set, no new worker */
    CloseHandle(s_tabs_thread);
    s_tabs_thread = NULL;
    s_tabs_stop = 0;
    s_tabs_state = HG_TABS_STOPPED;
    return TRUE;
}

/* UI thread only, like every entry point below. */
static BOOL hg_tabs_worker_ensure(void)
{
    if (!hg_tabs_worker_reap())
        return FALSE; /* one worker at a time, alive or stuck */
    if (s_tabs_state == HG_TABS_RUNNING)
        return TRUE;

    if (!s_tabs_lock_ready) {
        InitializeCriticalSection(&s_tabs_lock);
        s_tabs_lock_ready = TRUE;
    }
    if (!s_tabs_wake) {
        s_tabs_wake = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!s_tabs_wake)
            return FALSE;
    }

    s_tabs_stop = 0;
    s_tabs_thread = CreateThread(NULL, 0, hg_tabs_worker, NULL, 0, NULL);
    if (s_tabs_thread) {
        /* Tab titles are never urgent. Whatever the machine is doing when a
         * batch runs matters more than how soon the titles arrive. */
        SetThreadPriority(s_tabs_thread, THREAD_PRIORITY_BELOW_NORMAL);
        s_tabs_state = HG_TABS_RUNNING;
    }
    return s_tabs_thread != NULL;
}

void hg_tabs_request(const HWND *hwnds, int count)
{
    if (!hg_tabs_enabled() || !hwnds || count <= 0)
        return;
    if (!hg_tabs_worker_ensure())
        return;

    if (count > HG_TABS_WORKER_WINDOWS)
        count = HG_TABS_WORKER_WINDOWS;

    /* Merge, never replace: a window queued a moment ago and not yet served
     * must not be silently unqueued by a newer, smaller batch - that was how
     * windows ended up stamped as asked without ever being asked. A full set
     * drops the newcomer and says so in the counters. */
    EnterCriticalSection(&s_tabs_lock);
    for (int i = 0; i < count; ++i) {
        BOOL known = FALSE;
        for (int p = 0; p < s_tabs_request_count && !known; ++p)
            known = (s_tabs_request_hwnds[p] == hwnds[i]);
        if (known)
            continue;
        if (s_tabs_request_count >= HG_TABS_WORKER_WINDOWS) {
            ++s_stat_req_overflow;
            continue;
        }
        s_tabs_request_hwnds[s_tabs_request_count++] = hwnds[i];
        ++s_stat_queued;
    }
    LeaveCriticalSection(&s_tabs_lock);
    SetEvent(s_tabs_wake);
}

int hg_tabs_take_result(HWND hwnd, WCHAR titles[][HG_MAX_STR], int max, HgTabsAnswer *answer)
{
    if (!s_tabs_lock_ready || !titles || max <= 0)
        return -1;

    int taken = -1;
    EnterCriticalSection(&s_tabs_lock);
    for (int i = 0; i < HG_TABS_WORKER_WINDOWS; ++i) {
        HgTabsResult *slot = &s_tabs_results[i];
        if (slot->hwnd != hwnd || !slot->fresh)
            continue;
        int count = (slot->count < max) ? slot->count : max;
        for (int t = 0; t < count; ++t)
            StringCchCopyW(titles[t], HG_MAX_STR, slot->titles[t]);
        if (answer) {
            answer->failed = slot->failed;
            answer->pid = slot->pid;
            answer->elapsed_ms = slot->elapsed_ms;
            answer->provider = slot->provider;
        }
        slot->fresh = FALSE;
        taken = count;
        break;
    }
    LeaveCriticalSection(&s_tabs_lock);
    return taken;
}

void hg_tabs_note_overflow(void)
{
    if (!s_tabs_lock_ready)
        return;
    EnterCriticalSection(&s_tabs_lock);
    ++s_stat_pass_overflow;
    LeaveCriticalSection(&s_tabs_lock);
}

void hg_tabs_report(void (*emit)(const WCHAR *line))
{
    if (!emit)
        return;
    if (!hg_tabs_enabled()) {
        emit(L"tabs: off (show_tabs=0) - no worker, no COM objects, no calls");
        return;
    }
    if (!s_tabs_lock_ready) {
        emit(L"tabs: on, nothing asked yet");
        return;
    }

    WCHAR line[256];
    EnterCriticalSection(&s_tabs_lock);
    hellgates_wsprintf(line, HG_ARRAYSIZE(line),
                       L"tabs: queued %u, answered %u (failed %u, scoped %u, over 50 ms %u), "
                       L"dropped: queue-full %u, table-full %u, shutdown timeouts %u",
                       s_stat_queued, s_stat_completed, s_stat_failed, s_stat_scoped, s_stat_slow, s_stat_req_overflow,
                       s_stat_pass_overflow, s_stat_shutdown_timeouts);
    emit(line);
    for (int i = 0; i < HG_TABS_WORKER_WINDOWS; ++i) {
        const HgTabsResult *slot = &s_tabs_results[i];
        if (!slot->hwnd || !IsWindow(slot->hwnd))
            continue;
        WCHAR title[64] = L"";
        GetWindowTextW(slot->hwnd, title, (int)HG_ARRAYSIZE(title));
        hellgates_wsprintf(line, HG_ARRAYSIZE(line), L"  uia(%lc) %2d tab(s) %4u ms %ls%ls  %.48ls",
                           slot->walk_note ? slot->walk_note : L'-', slot->count, (unsigned)slot->elapsed_ms,
                           slot->failed ? L"FAILED " : L"", slot->fresh ? L"pending " : L"", title);
        emit(line);
    }
    emit(L"  (letter: s = scoped strip refresh, f = full-window discovery,"
         L" ! = the ask failed, - = nothing asked yet)");
    LeaveCriticalSection(&s_tabs_lock);
}

BOOL hg_tabs_activate(HWND hwnd, int tab_index)
{
    if (!hg_tabs_enabled() || tab_index < 0)
        return FALSE;

    IUIAutomationElement *tabs[HG_TABS_MAX_PER_WINDOW];
    int count = hg_tabs_collect(hg_tabs_automation(), hwnd, tabs, HG_TABS_MAX_PER_WINDOW);

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
    int count = hg_tabs_collect(automation, hwnd, tabs, HG_TABS_MAX_PER_WINDOW);
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
    /* Stop the worker first: it must not touch the tables while they reset.
     * If it exits within the wait, the machine returns to STOPPED. If it is
     * stuck inside a cross-process call, it moves to STOPPING: the stop flag
     * STAYS SET - resetting it here is what used to bring a stuck worker back
     * to life - the handle is kept for a later reap, and no new worker can be
     * created until that reap succeeds. A truly wedged call is left to
     * process teardown rather than terminated mid-call. */
    if (s_tabs_state == HG_TABS_RUNNING) {
        s_tabs_stop = 1;
        SetEvent(s_tabs_wake);
        if (WaitForSingleObject(s_tabs_thread, 2000) == WAIT_OBJECT_0) {
            CloseHandle(s_tabs_thread);
            s_tabs_thread = NULL;
            s_tabs_stop = 0;
            s_tabs_state = HG_TABS_STOPPED;
        } else {
            s_tabs_state = HG_TABS_STOPPING;
            if (s_tabs_lock_ready) {
                EnterCriticalSection(&s_tabs_lock);
                ++s_stat_shutdown_timeouts;
                LeaveCriticalSection(&s_tabs_lock);
            }
        }
    } else if (s_tabs_state == HG_TABS_STOPPING) {
        hg_tabs_worker_reap();
    }

    if (s_tabs_lock_ready && s_tabs_state == HG_TABS_STOPPED) {
        /* Only when no worker can be inside the tables. */
        EnterCriticalSection(&s_tabs_lock);
        s_tabs_request_count = 0;
        SecureZeroMemory(s_tabs_results, sizeof(s_tabs_results));
        LeaveCriticalSection(&s_tabs_lock);
    }

    if (s_automation) {
        IUIAutomation_Release(s_automation);
        s_automation = NULL;
    }
    s_automation_failed = FALSE;
}
