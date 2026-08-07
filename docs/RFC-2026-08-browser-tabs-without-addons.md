# RFC 2026-08: Browser Tabs Without Add-ons

Status: Partially implemented (2026-08-06) - see Implementation Notes
Date: 2026-08-06

Section labels here are B1-B6; the D-numbers belong to
`docs/RFC-2026-07-tabs-as-task-icons.md` and the two must not be confused -
that file's D6/D7 are the worker thread and the title gate this RFC builds on.

## Summary

Chrome/Edge 확장 프로그램, 별도 서비스, 디버깅 포트 없이 `hgfloater.exe` 하나만으로 탭 목록을
수집한다. 현재 UI Automation(UIA) 구현은 작업 스레드, `FindAllBuildCache`, 제목 기반 재조회,
30초 backstop, 창별 150 ms 간격까지 적용되어 있다. UIA 폴백으로는 올바른 방향이지만,
`FindAll(TreeScope_Descendants)`가 브라우저의 전체 접근성 subtree를 만들고 탐색하게 하는 근본
비용은 남는다.

제안은 다음 세 단계다.

1. 현재 tab worker의 종료 경쟁, 요청 덮어쓰기, stale identity 오류를 먼저 고친다.
2. Chromium 계열은 공식 지원이 더 완전한 MSAA/IAccessible을 우선 실험한다. 브라우저 상단 UI만
   제한 탐색하고 웹 페이지 subtree에는 들어가지 않는다.
3. MSAA가 안정적으로 탭 strip을 찾지 못하는 버전/브라우저는 UIA를 사용하되, 상단 container를
   한 번 찾아 그 작은 subtree만 갱신한다. 전체 descendant 탐색은 최초 발견과 복구 시에만 쓴다.

실제 favicon은 브라우저 접근성 API가 일관된 이미지 bytes/HICON을 제공하지 않으므로 이번 범위에서
보장하지 않는다. 탭별 표시는 기존 창 아이콘 공유 또는 제목 첫 글자 폴백을 유지한다. favicon을
얻기 위해 네트워크 요청, 화면 캡처, 캐시 파일 해킹을 추가하지 않는다.

## Goals

- 추가 설치 없는 단일 EXE를 유지한다.
- taskbox expand와 1초 창 목록 갱신에서 브라우저 전체 접근성 tree walk를 제거한다.
- 탭 목록 갱신이 UI 스레드와 브라우저 foreground 작업을 버벅이게 하지 않는다.
- 탭 활성화/닫기까지 멈춤 가능한 cross-process 호출을 UI 스레드 밖으로 옮긴다.
- Chrome, Edge, Explorer, Terminal, Notepad와 현재 지원 응용 프로그램의 fallback을 보존한다.
- pure C/WinAPI, 외부 라이브러리 없음, 메모리 상한 고정 원칙을 지킨다.

## Non-Goals

- 브라우저 확장 프로그램, Native Messaging host, background service를 설치하지 않는다.
- Chrome DevTools Protocol을 위해 remote-debugging port를 열지 않는다.
- 브라우저 profile/cache/history 파일을 읽지 않는다.
- 페이지 URL, 본문, 방문 기록, 쿠키를 수집하지 않는다.
- favicon을 얻기 위해 인터넷 접속이나 화면 OCR을 하지 않는다.
- UIA를 완전히 제거하지 않는다.

## Why the Current Walk Is Heavy

현재 `hg_tabs_read_titles`는 창 root에서 `UIA_TabItemControlTypeId`를 조건으로
`FindAllBuildCache(..., TreeScope_Descendants, ...)`를 실행한다. Name과 BoundingRectangle을 한
batch로 받으므로 탭별 round trip은 줄였지만, 검색 범위는 여전히 창 전체다.

Chromium은 renderer가 가진 접근성 정보를 browser process의 cache tree로 모은다. 접근성 client가
나타나면 이 구조를 활성화하고 materialize하는 비용이 대상 브라우저에 생긴다. 따라서 UIA 호출을
낮은 우선순위 worker로 옮기면 hgfloater UI의 멈춤은 줄어도 Chrome/Edge 자체가 치르는 비용까지
사라지지는 않는다.

Microsoft도 UIA의 `FindAll` 같은 호출은 UI thread 밖에서 수행하고, property/pattern을 개별
cross-process 호출하지 말고 cache request로 묶도록 권장한다. 현재 코드는 이 두 원칙을 이미
따른다. 다음 이득은 thread를 하나 더 만드는 데 있지 않고 탐색 범위를 줄이고 재탐색 원인을
정확하게 만드는 데 있다.

## Source Review

### Existing strengths

- `hg_tabs_read_titles`는 Name과 BoundingRectangle을 `FindAllBuildCache` 한 번으로 가져오고
  `AutomationElementMode_None`을 사용한다.
- 열거용 `IUIAutomation`은 전용 MTA worker가 소유하며 UI thread와 COM pointer를 공유하지 않는다.
- 알려진 window class만 조회하고, per-window title gate와 30초 backstop을 사용한다.
- UIA element pointer를 refresh 사이에 보존하지 않고 activate/close 때 다시 찾는다.
- fan-out된 탭은 한 HICON을 비소유 공유하여 `CopyIcon`/`DestroyIcon` churn을 피한다.
- `show_tabs=0`에서는 worker/UIA 객체를 만들지 않는다.

### F1 - Timed-out worker is treated as terminated (high)

`hg_tabs_shutdown`은 stop을 설정하고 2초 기다린 뒤 wait 결과와 무관하게 thread HANDLE을 닫고
`s_tabs_stop`을 0으로 되돌린다. `CloseHandle`은 thread를 종료하지 않는다. 결정적으로 stop을
0으로 되돌리는 것이 UIA 호출에 걸려 있던 thread를 **되살린다**: 호출이 끝나 loop 조건을 다시
검사할 때 종료 신호가 이미 사라져 있으므로 좀비가 아니라 정상 동작 thread로 복귀하고,
`show_tabs`를 다시 켜면 두 번째 worker가 생성되어 같은 auto-reset event를 두 thread가 기다린다.

Required fix:

- `WAIT_OBJECT_0`일 때만 HANDLE close와 stop reset을 수행한다.
- timeout이면 `STOPPING` 상태와 HANDLE을 보존하고 새 worker 생성을 금지한다.
- 다음 진입에서 zero-time wait로 종료를 reap한 뒤에만 재생성한다.
- 최종 process teardown과 runtime option toggle의 cleanup 경로를 분리한다.

### F2 - Replaced pending batches are stamped as completed (medium)

`hg_tabs_request`가 대기 batch를 최신 batch로 덮어쓰는 것은 적절한 coalescing이다. 그러나 호출자는
queue에 넣은 즉시 `cached_asked_title/tick`을 기록한다. worker가 다른 batch를 처리하는 동안 요청이
여러 번 교체되면 실제 조회되지 않은 창도 조회된 것으로 간주되어 최대 30초 stale 상태가 된다.

Required fix:

- request마다 증가하는 generation을 부여하고 result에 돌려준다.
- `asked_title/tick`은 matching generation result를 take할 때만 commit한다.
- result에 `OK`, `NO_TABS`, `FAILED`, `STALE` 상태를 둔다. 지금은 실패와 0 tabs가 모두 count 0이라
  일시적인 provider 실패가 기존 탭 fan-out을 없앨 수 있다.

### F3 - HWND alone is not a durable cache identity (medium-low)

UI cache와 result table은 HWND만 key로 쓴다. 파괴된 HWND 값이 cache compaction 전에 새 창에
재사용되면 새 창이 이전 창의 탭 결과를 잠시 받을 수 있다.

Required fix:

- identity를 `(HWND, process_id, class_name, generation)`으로 확장한다.
- process/class mismatch이면 cache와 pending result를 즉시 폐기한다.

### F4 - Activate and close can still block the UI thread (medium)

탭 클릭/닫기는 UI thread에서 `hg_tabs_collect`로 전체 descendant를 다시 찾는다. 명시적 사용자
동작이라 background refresh보다 빈도는 낮지만, hung provider에는 timeout이 없어 hgfloater가 오래
멈출 수 있다.

Required fix:

- activate/close를 action request로 worker에 전달한다.
- action은 enumeration batch보다 우선 처리하고 완료 message로 UI를 갱신한다.
- 단일 UIA/MSAA COM call은 안전하게 강제 종료할 수 없으므로 F1의 `STOPPING` 상태를 그대로 적용한다.

### Capacity limit

cache와 worker request는 16개 지원 창으로 제한된다. 17번째 이후 창은 tab expansion 대상이 되지
않는다. 고정 메모리 정책 자체는 합리적이지만 silent failure이므로 command output 또는 debug
counter로 `eligible/dropped` 수를 확인할 수 있어야 한다.

## Proposed Design

### B1 - One worker state machine

worker 상태를 `STOPPED`, `RUNNING`, `STOPPING`, `FAILED`로 명시한다. UI thread는 request queue만
갱신하며 COM 호출을 하지 않는다.

```text
RUNNING --disable--> STOPPING --thread exit/reap--> STOPPED
RUNNING --fatal init error--> FAILED
STOPPED --enable--> RUNNING
STOPPING --enable--> STOPPING (restart pending only)
```

queue는 창별 최신 request 하나만 유지한다. 전체 batch 덮어쓰기 대신 고정 16-slot table에
identity/generation/priority를 저장하면 한 창의 잦은 title change가 다른 창 request를 지우지 않는다.
worker는 action, dirty enumeration, backstop 순으로 하나씩 꺼낸다.

### B2 - Chromium fast path: bounded MSAA traversal

Chromium 공식 접근성 문서는 Windows에서 MSAA/IAccessible과 IAccessible2 지원은 complete,
IAccessibleEx/UIA는 very limited라고 설명한다. 따라서 `Chrome_WidgetWin_*` 창에는 MSAA fast path를
우선 검증한다.

Algorithm:

1. `AccessibleObjectFromWindow(hwnd, OBJID_CLIENT, IID_IAccessible, ...)`로 browser chrome root를 얻는다.
2. child를 breadth-first로 탐색하되 깊이, node 수, 시간 budget을 고정한다.
3. `ROLE_SYSTEM_PAGETABLIST`를 찾으면 그 직계/근접 `ROLE_SYSTEM_PAGETAB` child만 읽는다.
4. Bounding rectangle이 window 상단 band 안인 항목만 유지하고 left 좌표로 정렬한다.
5. `STATE_SYSTEM_INVISIBLE/OFFSCREEN`은 제외한다.
6. page/document role 아래로는 절대 내려가지 않는다. 웹 콘텐츠 tree를 만들지 않는 것이 핵심이다.

초기 budget 제안:

- 최대 depth 8
- 최대 visited node 256
- 창당 soft elapsed budget 20 ms; 넘으면 걷기를 중단하고 UIA fallback으로 넘어가되, 그때까지
  얻은 결과가 sanity check를 통과하면 버리지 않고 채택한다 - 이미 지불한 비용의 결과를
  폐기하면 재시도가 같은 비용을 반복한다
- 탭 최대 24개는 현재 상한 유지

MSAA의 `AccessibleChildren`도 cross-process COM이므로 worker에서만 호출한다. browser version별
role/tree shape가 다를 수 있어 class name만으로 성공을 가정하지 않고, 위치/role/title sanity check를
모두 통과한 경우만 fast path 결과를 채택한다.

### B3 - UIA scoped-container fallback

MSAA가 실패하거나 비Chromium 앱이면 UIA를 사용한다. 다만 매 refresh마다 window root의 전체
descendant를 검색하지 않는다.

1. Discovery: 기존 `FindAllBuildCache(TreeScope_Descendants)`를 한 번 실행해 상단 TabItem들을 찾는다.
2. 각 TabItem의 parent/ancestor를 비교해 공통의 가장 작은 container를 정한다.
3. container를 다시 찾을 **경로 힌트**를 cache한다: root에서 container까지의
   (ControlType, ClassName, AutomationId, child index) 체인. live pointer는 cache하지 않는다.
   주의: UIA runtime ID는 property condition 검색에 쓸 수 없으므로 "runtime ID로 재발견"은
   성립하지 않는다 - 재발견은 TreeWalker로 경로 힌트를 따라 내려가는 짧은 걷기다.
4. Refresh: 경로 힌트를 따라 container를 다시 잡은 뒤 그 subtree에만 `FindAllBuildCache`를
   실행한다.
5. 경로가 끊겼거나 sanity check가 실패할 때만 full discovery로 돌아간다.

경로 힌트는 provider 재구성 뒤 영구 identity가 아니므로 실패 시 폐기 가능한 hint로만 쓴다.
container 재발견이 오히려 많은 round trip을 만들면 해당 provider는 기존 one-shot full cache walk를
유지한다. 구현 전/후 호출 시간으로 선택해야 한다.

### B4 - Event-assisted invalidation, not event-driven enumeration

UIA는 structure/property/selection event subscription을 지원한다. 이를 데이터 source로 신뢰하지
않고 cache를 dirty로 만드는 hint로만 사용한다.

- container를 찾은 provider에만 `StructureChanged`와 selection event를 좁은 scope로 등록한다.
- callback은 COM 탐색을 하지 않고 atomic dirty bit와 timestamp만 기록하고 UI window에 post한다.
- event storm은 창당 250 ms debounce한다.
- event가 오지 않아도 30초 backstop은 유지한다.
- window root descendants 전체에 등록했을 때 web-content event storm이 생기면 그 provider의 event
  mode를 즉시 끄고 title/backstop 방식으로 되돌린다.
- add/remove handler는 동일 worker thread에서 직렬화한다. Microsoft는 여러 thread에서 event
  handler를 동시에 add/remove하지 말라고 명시한다.
- **명시해야 할 두 리스크**: (1) 구독은 대상 element의 live 참조를 구독 기간 내내 보유한다 -
  이 파일 계열의 "foreign pointer를 refresh 사이에 보존하지 않는다" 원칙의 명시적 예외이며,
  provider가 죽으면 참조 해제 경로를 반드시 검증해야 한다. (2) 구독의 존재 자체가 브라우저
  접근성 활성화를 붙들어 둘 수 있다 - Chromium이 유휴 시 접근성을 해체하는 이득(D7이 확인한
  효과)을 구독이 상쇄하는지 Phase 2 측정 항목에 넣는다.

이 설계는 “이벤트가 반드시 온다”에 기대지 않는다. 이벤트 품질이 나쁜 브라우저에서도 현재보다
나빠지지 않고, 잘 오는 provider에서만 불필요한 backstop walk를 줄인다.

### B5 - Icon policy without favicon extraction

추가 설치 없이 접근성 API만 사용할 때 탭 제목은 얻을 수 있지만 favicon image bytes/HICON은
표준적으로 보장되지 않는다. 다음 정책을 명시한다.

- 탭별 favicon을 찾기 위해 child image를 재귀 탐색하지 않는다. 다시 큰 tree walk가 된다.
- browser cache/profile DB/file을 읽지 않는다. format/version/lock/privacy 의존성이 생긴다.
- 화면 capture나 OCR을 사용하지 않는다.
- 첫 탭은 기존 window HICON을 소유하고 나머지는 같은 handle을 비소유 공유한다.
- HICON이 없으면 현재 title 첫 글자 rendering을 사용한다.

따라서 이번 RFC의 “아이콘”은 탭 항목의 안정적인 시각 표식을 뜻하며 실제 favicon은 non-goal이다.
favicon이 필수 요구가 되면 추가 설치 허용 여부를 별도 RFC에서 다시 결정해야 한다.

### B6 - Adaptive backoff and circuit breaker

provider/window별 elapsed time과 연속 실패를 기억한다.

- 50 ms 초과: 다음 최소 interval 30초
- 200 ms 초과 또는 failure 3회: 2분 circuit open
- circuit open 중 title change가 와도 즉시 탐색하지 않고 taskbox에는 마지막 snapshot 또는 window
  item 하나만 표시
- 사용자 `refresh`도 UI thread를 막지 않으며 worker request만 우선순위로 올림
- 성공 2회 후 기본 interval로 복귀

전체 시스템에 한 번에 부하가 몰리지 않도록 현재의 창별 150 ms stagger와 below-normal priority는
유지한다. `Sleep(150)` 대신 다음 due tick을 scheduler에 기록하면 action request를 즉시 처리할 수 있다.

## Alternatives Rejected

### Browser extension / Native Messaging

가장 정확한 탭 ID와 favicon을 event로 받을 수 있지만 추가 설치가 필요하므로 사용자 요구에 따라
제외한다.

### Chrome DevTools Protocol

remote-debugging 설정/port가 필요하고 공격 표면과 운영 복잡도가 커진다. 제외한다.

### UIA full-descendant polling with a longer interval

구현은 단순하지만 한 번의 큰 비용은 그대로며 stale 시간만 늘어난다. 최후 fallback으로만 유지한다.

### Browser profile/cache parsing

실행 중 잠금, browser별 schema, profile 분리, private browsing, privacy 문제가 있고 HWND와 탭을
신뢰성 있게 연결할 수도 없다. 제외한다.

### Screenshot/OCR/favicon pixel cropping

비용, DPI, theme, animation, occlusion에 취약하고 안정적인 tab identity도 제공하지 않는다. 제외한다.

## Delivery Plan

### Phase 0 - Correctness and measurement

- F1-F3를 수정하고 worker state/generation unit test를 추가한다.
- F4 action queue의 최소 골격을 추가한다.
- release build에서는 파일 log를 쓰지 않고 in-memory counter만 유지한다.
- counter: queued, coalesced, completed, failed, stale-dropped, elapsed buckets,
  shutdown-timeout, eligible-window-overflow.

Exit criteria:

- off/on 반복 중 살아 있는 tab worker는 최대 1개다.
- 덮어쓴 request는 완료로 기록되지 않는다.
- HWND reuse simulation에서 이전 snapshot이 적용되지 않는다.
- `refresh_window_list` p95는 탭 수와 무관하게 5 ms 이하다.

### Phase 1 - MSAA spike, ship only on evidence

- Chrome/Edge 각각에서 role/depth/node/time trace를 메모리 buffer로 수집한다.
- 1, 10, 24 tabs와 1시간 이상 사용한 browser에서 현재 UIA와 비교한다.
- title/order/selected state가 모두 맞고 p95가 유의하게 낮을 때만 provider로 채택한다.

Exit criteria:

- browser content document node 방문 0회.
- 창당 p95 20 ms 이하, max 50 ms 이하.
- Chrome/Edge에서 tab order/title 정확도 100%.
- 기준을 못 맞추면 MSAA path는 폐기하고 Phase 2만 진행한다.

### Phase 2 - Scoped UIA and invalidation hints

- discovery/container cache/validation을 구현한다.
- 좁은 scope event invalidation을 provider별 opt-in으로 추가한다.
- event storm detector와 30초 backstop을 검증한다.

Exit criteria:

- steady state에서 full window descendant discovery가 0회/분에 가깝다.
- tab create/remove/move/title change가 보통 1초 이내, 최악 30초 이내 반영된다.
- 웹 페이지 animation/DOM churn이 tab refresh를 반복 유발하지 않는다.

### Phase 3 - Runtime verification and default decision

- Windows Performance Recorder/WPA로 taskbox expand, browser CPU spike, worker CPU,
  UIA/MSAA call duration을 비교한다.
- 8개 browser windows, 각 24 tabs에서 expand 100회를 반복한다.
- GDI/USER/COM handle과 working set을 2시간 관찰한다.

Ship criteria:

- UI thread expand p95 16 ms 이하.
- 현재 v0.2.1 대비 browser-side CPU peak와 slow-call 횟수가 감소.
- tab worker shutdown timeout 뒤 off/on에서도 deadlock, duplicate worker, late apply 없음.
- 실패 provider는 기존 window item으로 안전하게 degrade.

## Verification Matrix

| Case | Expected |
|---|---|
| `show_tabs=0` | worker/UIA/MSAA 객체와 호출 0 |
| Chrome/Edge normal | MSAA fast path 또는 scoped UIA, content subtree 방문 0 |
| MSAA shape changed | sanity check 실패 후 UIA fallback |
| Explorer/Terminal/Notepad | scoped UIA 또는 기존 UIA fallback 유지 |
| provider event missing | 30초 backstop으로 복구 |
| provider event storm | debounce/circuit breaker, UI 영향 없음 |
| UIA/MSAA call hangs | UI 정상, off/on에서 duplicate worker 없음 |
| pending request replaced | generation 불일치 result 폐기 |
| HWND reused | PID/class mismatch로 이전 cache 폐기 |
| more than 16 eligible windows | bounded memory, overflow counter 증가, window item fallback |
| rapid tab churn | 최신 generation만 적용, 제거된 tab 재등장 없음 |
| icon missing | 공유 window icon 또는 title 첫 글자 표시 |

## Recommendation

Phase 0은 즉시 진행할 가치가 있다. 특히 F1은 설정 토글 후 duplicate worker가 생길 수 있는 실제
수명주기 결함이다.

성능 개선은 MSAA spike를 먼저 측정하되 결과가 좋을 것이라고 가정하지 않는다. Chromium 공식
문서상 Windows의 complete 접근성 경로는 MSAA이고 UIA는 제한적이므로 시험 가치가 있다. 다만
가장 안전한 공통 개선은 full descendant polling을 discovery-only로 낮추고, 작은 tab container
refresh와 bounded backoff를 사용하는 것이다.

확장 프로그램 없이 실제 favicon까지 안정적으로 얻는 효율적인 공식 API는 확인되지 않았다.
favicon 때문에 browser tree, profile file, 화면을 더 깊게 훑으면 원래 성능 문제를 되살리므로,
이번 RFC에서는 정확한 제목/순서/동작과 끊김 제거를 우선한다.

## Implementation Notes (2026-08-06)

Phase 0 plus the two performance pieces shipped together; the review that
preceded them adjusted three designs. What the code does where it differs from
the sections above:

- **F1**: implemented as the B1 state machine (STOPPED/RUNNING/STOPPING), with
  reap-before-create and the stop flag left set across a timeout. A FAILED
  state proved unnecessary: a worker that cannot initialise COM still answers,
  with failed results, and the breaker quiets it.
- **F2**: implemented with a simpler design than per-request generations -
  `hg_tabs_request` now **merges** into the pending set instead of replacing
  it, so a queued window can never be silently unqueued; with that, stamping
  asked-state at send time is truthful again. The OK/FAILED distinction is
  carried per result (`HgTabsAnswer.failed`), and a failed ask keeps the
  previous fan-out instead of folding it.
- **F3**: implemented as PID-only identity. The class check was dropped: a
  recycled HWND only reaches the tab path if the new window is also a
  tab-class window, and the PID catches that; asked-title mismatch then
  refreshes within a pass. The full (class, generation) tuple was judged
  over-engineering.
- **F4**: deferred, deliberately - RFC-2026-07 D6 records the trade (a click
  blocking briefly is the behaviour the click asked for). Revisit if a hung
  provider is ever actually observed.
- **B2 (MSAA)**: implemented in `hg_tabs_msaa_read` with the budgets above
  (depth 8, 256 nodes, 20 ms, 64 children per node), DOCUMENT-subtree and
  below-band pruning, and per-attempt adoption: only a PAGETABLIST with at
  least one visible PAGETAB in the top band is accepted; anything else falls
  through to UIA in the same ask. Which provider answered is visible per
  window in `show tabs`.
- **B3/B4**: not implemented - B3 awaits the path-hint redesign recorded
  above, B4 awaits Phase 2 and its two-risk measurement.
- **B6**: implemented UI-side in the window list: per-window earliest-ask
  (50 ms → 30 s quiet, 200 ms or 3 straight failures → 120 s), outranking
  even a changed title. The worker keeps the 150 ms stagger and below-normal
  priority; the `Sleep` stays until an action queue (F4) needs due-ticks.
- **Observability**: `show tabs` (`s t`) in the command box prints the
  counters (queued, answered, failed, msaa, over-50 ms, both overflow kinds,
  shutdown timeouts) and one line per known window with provider, tab count,
  elapsed and title. In-memory only; nothing is written to disk.

## References

- Microsoft, UI Automation threading issues:
  <https://learn.microsoft.com/en-us/dotnet/framework/ui-automation/ui-automation-threading-issues>
- Microsoft, `IUIAutomationElement::FindAllBuildCache`:
  <https://learn.microsoft.com/en-us/windows/win32/api/uiautomationclient/nf-uiautomationclient-iuiautomationelement-findallbuildcache>
- Microsoft, UI Automation cache requests:
  <https://learn.microsoft.com/en-us/windows/win32/api/uiautomationclient/nn-uiautomationclient-iuiautomationcacherequest>
- Microsoft, UI Automation event subscriptions:
  <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-eventsforclients>
- Microsoft, `AddStructureChangedEventHandler`:
  <https://learn.microsoft.com/en-us/windows/win32/api/uiautomationclient/nf-uiautomationclient-iuiautomation-addstructurechangedeventhandler>
- Microsoft, UI Automation and Microsoft Active Accessibility:
  <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-msaa>
- Chromium accessibility architecture and Windows API support:
  <https://www.chromium.org/developers/design-documents/accessibility/>
- Existing design history: `docs/RFC-2026-07-tabs-as-task-icons.md`

## Field Result (2026-08-06, v0.3.0 first runtime evidence)

The maintainer's first `show tabs` run answered **uia for every Chromium
window** - the MSAA fast path never engaged. The leading hypothesis, to be
confirmed by the instrumented note now shipped ('r'/'e'/'b'/'t'/'x' beside the
provider): modern Chromium serves a **stub MSAA tree** ('e') until its
progressive-accessibility machinery decides the client deserves a real one,
and a plain WM_GETOBJECT + IAccessible walk no longer qualifies as such a
client - while a UIA connection does, which is why UIA keeps answering.

If 'e' is confirmed, B2 is a dead end for stock Chrome/Edge **by Chrome's own
gating**, not by MSAA's shape: the activation that would populate the tree
(the IAccessible2/screen-reader honeypot) is exactly the switch that turns on
web-content accessibility - the cost this whole RFC exists to avoid. The MSAA
attempt stays (it is nearly free, and it will serve Chromium variants and
Electron apps whose accessibility is already active), but the performance
road for stock browsers then runs through B3, scoped UIA, as re-specified
above. 'b'/'x' would instead mean the budgets or the walk need tuning, which
is a smaller conversation.

**Verdict (2026-08-06, second run, v0.3.1 letters): `b` - the budgets.** The
instrumented sample:

```
tabs: queued 4, answered 4 (failed 0, msaa 0, over 50 ms 4)
  uia (-)  2 tab(s)  644 ms   Explorer window
  uia (b)  1 tab(s)  106 ms   Chrome window
  uia (b)  4 tab(s)  120 ms   Chrome window
  uia (-)  2 tab(s)  649 ms   Explorer window
```

Two facts, two responses:

1. **Chrome answers MSAA with a real tree** ('b', not 'e' - the stub-tree
   hypothesis is dead) and the walk died on the 20 ms clock, which at a few
   milliseconds per cross-process element access bought only a handful of
   nodes. Response: budget raised to 60 ms (still far under the 106-649 ms
   UIA walks it replaces), and the walk now visits each level's children
   **top-first** - on a clock budget the order of the walk is the walk, and
   the strip is by definition at the top.
2. **The priciest walks belonged to Explorer** (644/649 ms UIA), which the
   Chromium-only gate never even offered to MSAA. Response: the gate is gone;
   every tab-class window gets the bounded MSAA attempt first, and the letter
   says how far it got. If Explorer's XAML-to-MSAA bridge answers with
   PAGETABLIST, the biggest single cost in the sample disappears; if it
   answers 'x'/'e', the evidence decides B3's priority. (Parked alternative
   for Explorer specifically: each tab is already an entry in the
   IShellWindows collection this program walks for paths - tab titles could
   come from shell COM with no UIA at all, at the price of an unverified
   ordering. Worth an experiment if the MSAA bridge disappoints.)

## Field Result 2 - B2 retired, B3 shipped (2026-08-07)

The third run, on v0.3.2 (budget 60 ms, top-first walk, every tab-class
window):

```
tabs: queued 12, answered 12 (failed 0, msaa 0, over 50 ms 12)
  uia (b)  2 tab(s)  221 ms   Chrome
  uia (x)  2 tab(s) 1355 ms   Explorer
  uia (x)  2 tab(s)  664 ms   Explorer
  uia (b)  3 tab(s)  170 ms   Chrome
```

The verdict is structural, not parametric:

- **Chrome: still `b` at triple the budget with an ordered walk.** MSAA's cost
  model is the problem - one cross-process round trip per element access, and
  a node with twenty children costs forty of them. Any budget generous enough
  to reach the strip costs what the one-call UIA walk costs. There is no
  budget at which this wins.
- **Explorer: `x` - the XAML-to-MSAA bridge exposes a tree, but no
  PAGETABLIST in it.** Role mapping, not budget.
- Worse, the failed attempt now ADDED up to 60 ms to every ask (221 ms vs the
  previous 106-120 ms for Chrome).

**B2 is retired** per this RFC's own rule - ship only on evidence, and the
evidence, twice, says no. The MSAA walk is deleted, `-loleacc` is gone, and
the letters moved to grading what shipped in its place:

**B3, scoped UIA, as re-specified above**: discovery runs the usual one-call
full walk, then finds one live TabItem, takes its ControlView parent as the
strip container, and records the property chain (control type, class,
automation id) from the window root down to it. Every later refresh descends
that chain with one one-level FindFirst per step and runs the tab query on
the container's own subtree - Explorer's folder view, the walk's real cost,
is never considered. Any miss at any step invalidates the hint and the next
ask pays one full discovery, rebuilding it. `show tabs` letters are now
`s` (scoped), `f` (full discovery), `!` (failed); steady state should be all
`s` with milliseconds in the low tens, and `f` only after a provider rebuilds
its tree.

## Field Result 3 - B3 confirmed working (2026-08-07)

Fourth run, first with the scoped read shipped. Two consecutive `show tabs`:

```
first ask (discovery, as designed):
  uia(f)  1 tab(s) 1018 ms   Explorer
  uia(f)  5 tab(s)  602 ms   Chrome
  uia(f)  5 tab(s)  139 ms   Chrome
later (hints in place):
  uia(s)  1 tab(s)  112 ms   Explorer      (was 1018 - 9x)
  uia(s)  5 tab(s)   64 ms   Chrome
  uia(f) 10 tab(s)  226 ms   Chrome        (a new window: discovery, correct)
```

The scoped read behaves exactly as specified: `f` only on first contact and
provider rebuilds, `s` thereafter at a fraction of the cost. One consequence
tuned in response: the per-window breaker's slow threshold (50 ms) predated
these numbers and was putting healthy 60-112 ms scoped reads on a 30-second
leash; it moved to 150 ms (circuit threshold 200 to 300 ms) so the breaker
once again marks only what is actually slow for the new normal.
