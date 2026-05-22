\# hgfloater Refactoring Work Order (Refined TODO)
본 문서는 `hgfloater.c` 단일 C 소스코드의 일관성을 유지하면서 시스템 아키텍처와 디자인 감각을 Windows 11 Fluent Design 가이드라인에 맞추어 격상하기 위한 \*\*통합 리팩토링 상세 설계서(작업 지시서)\*\*입니다. 직접적인 코드 구현 단계 전, 작업 제어 지향성 및 요구 스펙을 완벽히 규정하는 역할을 수행합니다.
\---
\## 1. 전역 매크로 상수 네이밍 규칙 일체화 (`HG\_` 접두어 적용)
> \*\*작업 지침 (Agent Execution Guidelines)\*\*
> \* 본 TODO.md의 내용은 \*\*반드시 한 단계(Phase/Section)씩 순차적으로 진행\*\*해야 합니다.
> \* 한 턴에 여러 단계를 진행하지 마십시오. 한 섹션을 완료하면 체크박스(`\[ ]` -> `\[x]`)를 갱신해 저장하고, 작업을 보고한 뒤 대기하십시오.
> \* GUI 시각화 파트는 시스템 자원 절약을 최우선으로 하여, 애니메이션 효과(페이드 인/아웃, 자연스러운 전환 같은 고비용 효과)는 철저히 배제하고 단순 정적 구조나 깜빡임 정도만 사용해 간단하게 구현하십시오.
\---
\## 1. 전역 매크로 상수 네이밍 규칙 일체화 (`HG\_` 접두어 적용)
코드 가독성을 높이고 네임스페이스 오염을 근본적으로 차단하기 위해, 소스코드 내 모든 사용자 정의 매크로 상수에 `HG\_` 접두어를 붙입니다.
\- \[x] \*\*Windows 클래스명 및 시스템 리소스 배율 관련 상수\*\*
&#x20; - `WINDOW\_WIDTH` $\\rightarrow$ `HG\_WINDOW\_WIDTH`
&#x20; - `WINDOW\_HEIGHT` $\\rightarrow$ `HG\_WINDOW\_HEIGHT`
&#x20; - `MIN\_WINDOW\_WIDTH` $\\rightarrow$ `HG\_MIN\_WINDOW\_WIDTH`
&#x20; - `MIN\_WINDOW\_HEIGHT` $\\rightarrow$ `HG\_MIN\_WINDOW\_HEIGHT`
&#x20; - `BORDER\_THICKNESS` $\\rightarrow$ `HG\_BORDER\_THICKNESS`
\- \[x] \*\*컨트롤 ID 리스트\*\*
&#x20; - `IDC\_LISTBOX` $\\rightarrow$ `HG\_IDC\_LISTBOX`
&#x20; - `IDC\_TOOLBAR` $\\rightarrow$ `HG\_IDC\_TOOLBAR`
&#x20; - `IDC\_EDIT\_MSG` $\\rightarrow$ `HG\_IDC\_EDIT\_MSG`
\- \[x] \*\*구조 제한 및 최대 용량 한도 관련 상수\*\*
&#x20; - `MAX\_TITLE\_LEN` $\\rightarrow$ `HG\_MAX\_TITLE\_LEN`
&#x20; - `MAX\_WINDOW\_ITEMS` $\\rightarrow$ `HG\_MAX\_WINDOW\_ITEMS`
&#x20; - `MAX\_SHORTCUTS` $\\rightarrow$ `HG\_MAX\_SHORTCUTS`
&#x20; - `MAX\_AUDIO\_DEVICES` $\\rightarrow$ `HG\_MAX\_AUDIO\_DEVICES`
&#x20; - `NUM\_BASIC\_ICONS` $\\rightarrow$ `HG\_NUM\_BASIC\_ICONS`
&#x20; - `MAX\_ALPHA` $\\rightarrow$ `HG\_MAX\_ALPHA`
&#x20; - `MIN\_ALPHA` $\\rightarrow$ `HG\_MIN\_ALPHA`
\- \[x] \*\*컨텍스트 메뉴 \& 핫키 ID 리스트\*\*
&#x20; - `IDM\_MINIMIZE` $\\rightarrow$ `HG\_IDM\_MINIMIZE`
&#x20; - `IDM\_CLOSE` $\\rightarrow$ `HG\_IDM\_CLOSE`
&#x20; - `IDM\_MOVE` $\\rightarrow$ `HG\_IDM\_MOVE`
&#x20; - `IDM\_SIZE` $\\rightarrow$ `HG\_IDM\_SIZE`
&#x20; - `IDM\_CLOSE\_APP` $\\rightarrow$ `HG\_IDM\_CLOSE\_APP`
&#x20; - `IDM\_CLEAR\_EDIT` $\\rightarrow$ `HG\_IDM\_CLEAR\_EDIT`
&#x20; - `IDM\_EDIT\_COPYALL` $\\rightarrow$ `HG\_IDM\_EDIT\_COPYALL`
&#x20; - `IDM\_ABOUT` $\\rightarrow$ `HG\_IDM\_ABOUT`
&#x20; - `IDM\_RESET\_ALL` $\\rightarrow$ `HG\_IDM\_RESET\_ALL`
&#x20; - `IDM\_FONT\_UP` $\\rightarrow$ `HG\_IDM\_FONT\_UP`
&#x20; - `IDM\_FONT\_DOWN` $\\rightarrow$ `HG\_IDM\_FONT\_DOWN`
&#x20; - `IDM\_POWER\_OFF` $\\rightarrow$ `HG\_IDM\_POWER\_OFF`
&#x20; - `IDM\_VOLUME\_PERCENT` $\\rightarrow$ `HG\_IDM\_VOLUME\_PERCENT`
&#x20; - `IDM\_OPEN\_CONTROLBOX` $\\rightarrow$ `HG\_IDM\_OPEN\_CONTROLBOX`
&#x20; - `IDM\_AUDIO\_DEVICE\_BASE` $\\rightarrow$ `HG\_IDM\_AUDIO\_DEVICE\_BASE`
&#x20; - `IDM\_MONITOR\_BASE` $\\rightarrow$ `HG\_IDM\_MONITOR\_BASE`
\- \[x] \*\*타이머 및 기타 값\*\*
&#x20; - `TIMER\_HIGHLIGHT` $\\rightarrow$ `HG\_TIMER\_HIGHLIGHT`
&#x20; - `HIGHLIGHT\_TICKS` $\\rightarrow$ `HG\_HIGHLIGHT\_TICKS`
&#x20; - `TRANSPARENT\_KEY` $\\rightarrow$ `HG\_TRANSPARENT\_KEY`
&#x20; - `ARRAYSIZE` $\\rightarrow$ `HG\_ARRAYSIZE`
\---
\## 2. 윈도 이벤트 루프의 Controller 패턴 분리
단일 모놀리식 C 파일의 논리 구조와 유지보수성 극대화를 위해 각 윈도 프로시저(`WndProc`) 내 거대한 `switch-case` 블록의 세부 로직을 전용 `Controller` 함수군으로 완전 격리합니다. 윈도 프로시저는 메시지 디스패치 및 매개변수 전처리 역할에 집중시킵니다.
- [x] **Floater Widget Controller 구현 및 바인딩**
&#x20; - `floater\_controller\_on\_create(HWND hwnd)`
&#x20; - `floater\_controller\_on\_paint(HWND hwnd, HDC hdc, RECT\* prc)`
&#x20; - `floater\_controller\_on\_keydown(HWND hwnd, WPARAM w\_param, LPARAM l\_param)`
&#x20; - `floater\_controller\_on\_command(HWND hwnd, int id)`
&#x20; - `floater\_controller\_on\_destroy(HWND hwnd)`
\- \[ ] \*\*Taskbox Controller 구현 및 바인딩\*\*
&#x20; - `taskbox\_controller\_on\_create(HWND hwnd)`
&#x20; - `taskbox\_controller\_on\_paint(HWND hwnd)`
&#x20; - `taskbox\_controller\_on\_keydown(HWND hwnd, WPARAM w\_param, LPARAM l\_param)`
&#x20; - `taskbox\_controller\_on\_command(HWND hwnd, int id, HWND ctrl\_hwnd)`
&#x20; - `taskbox\_controller\_on\_destroy(HWND hwnd)`
\- \[ ] \*\*Controlbox Controller 구현 및 바인딩\*\*
&#x20; - `controlbox\_controller\_on\_create(HWND hwnd)`
&#x20; - `controlbox\_controller\_on\_paint(HWND hwnd)`
&#x20; - `controlbox\_controller\_on\_command(HWND hwnd, int id)`
&#x20; - `controlbox\_controller\_on\_scroll(HWND hwnd, HWND slider\_hwnd)`
&#x20; - `controlbox\_controller\_on\_destroy(HWND hwnd)`
\- \[ ] \*\*Toolbar (Icon Grid) Controller 구현 및 바인딩\*\*
&#x20; - `toolbar\_controller\_on\_paint(HWND hwnd)`
&#x20; - `toolbar\_controller\_on\_mouse\_move(HWND hwnd, int x, int y)`
&#x20; - `toolbar\_controller\_on\_lbutton\_up(HWND hwnd, int x, int y)`
\---
\## 3. Windows 11 기반 '의미 중심' 시스템 테마 추상화 계층
OS 순정 디자인과의 일체감 제공 및 고대비(High Contrast) 접근성을 만족하기 위해 시스템 테마 설정값을 가로채어 동적으로 갱신하는 융합형 테마 추상화 시스템을 구축합니다.
\- \[ ] \*\*의미적(Semantic) 테마 스키마 정의\*\*
&#x20; Windows 11의 플루언트 디자인 배색 규칙에 맞추어 스타일 속성을 추상화하는 `HG\_THEME\_SCHEME` 구조체를 선언합니다.
&#x20; ```c
&#x20; typedef struct \_HG\_THEME\_SCHEME {
&#x20;     COLORREF bg\_window;         // 기본 윈도 백그라운드 색상
&#x20;     COLORREF bg\_control;        // 편집 상자, 입력 컨트롤, 슬라이더 배경
&#x20;     COLORREF text\_primary;      // 가독성이 높은 메인 활성 텍스트 (기본 한글/영문)
&#x20;     COLORREF text\_secondary;    // 슬레이트 뉘앙스의 타이틀 설명 및 비활성 안내 텍스트
&#x20;     COLORREF accent\_color;      // Windows 11 Accent Color와 동기화되는 주요 컨트롤 색상
&#x20;     COLORREF border\_normal;     // 일반 경계선 색상
&#x20;     COLORREF border\_active;     // 포커스/활성화된 상태의 하이라이트 경계선
&#x20;     COLORREF flash\_highlight;   // 경고성 인디케이트 또는 점멸 연출 색상
&#x20;     HBRUSH   hbr\_window;        // 브러시 리소스 캐시
&#x20;     HBRUSH   hbr\_control;       // 컨트롤 배경 브러시 리소스 캐시
&#x20; } HG\_THEME\_SCHEME;
&#x20; ```
\- \[ ] \*\*시스템 테마 감지 엔진 설계\*\*
&#x20; - \*\*다크/라이트 테마 자동 판별:\*\* 레지스트리 경로 `HKEY\_CURRENT\_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize`의 `AppsUseLightTheme` 값을 읽어 다크 모드 활성화 유무를 탐지합니다.
&#x20; - \*\*강조 색상(Accent Color) 동기화:\*\* `DwmGetColorizationColor` API를 사용하여 윈도 시스템 테마의 강조 색상(Accent Color) 값을 가져와 `accent\_color` 필드 및 테두리 컬러에 반영합니다.
&#x20; - \*\*고대비 테마(High Contrast) 감지 및 지원:\*\* `SystemParametersInfoW(SPI\_GETHIGHCONTRAST, ...)` 호출을 통해 고대비 모드가 활성화된 경우, 가독성이 극대화된 기본 시스템 컬러(`COLOR\_WINDOW`, `COLOR\_WINDOWTEXT` 등)로 대체하도록 분기 로직을 구성합니다.
\- \[ ] \*\*실시간 테마 전환 및 동적 갱신 핸들링\*\*
&#x20; - 개별 메인 윈도 프로시저에 `WM\_SETTINGCHANGE` 분기를 편입하여 테마, 디스플레이 해상도 혹은 시스템 구성이 온라인 변경될 때 즉각적으로 테마 구조체 데이터를 재동기화하도록 제어 루프를 바인딩합니다.
&#x20; - `WM\_CTLCOLOREDIT` 및 `WM\_CTLCOLORSTATIC` 메시지 수신 시, 의미적 추상화 브러시(`hbr\_control` 등)를 리턴하여 자식 컨트롤 편집기의 컬러 테마 일체감을 완전하게 보장합니다.
\---
\## 4. 통합 윈도 매니저(Window Manager) 서브시스템 명세
제어 주체 역할의 부재로 인해 발생하는 윈도 계층 구조 꼬임, 포커스 소실, 불규칙적인 글로벌 플래그 오버랩 문제를 원천 방지하기 위해 통합 윈도 관리자 계층을 정의합니다.
\- \[ ] \*\*윈도 상태 레지스트리 및 인스턴스 테이블 구조화\*\*
&#x20; ```c
&#x20; typedef enum \_HG\_WINDOW\_TYPE {
&#x20;     HG\_WND\_FLOATER = 0,
&#x20;     HG\_WND\_TASKBOX,
&#x20;     HG\_WND\_CONTROLBOX,
&#x20;     HG\_WND\_ABOUT,
&#x20;     HG\_WND\_MONITOR\_BASE,
&#x20;     HG\_WND\_COUNT
&#x20; } HG\_WINDOW\_TYPE;
&#x20; typedef struct \_HG\_WINDOW\_MANAGER {
&#x20;     HWND    hwnd\_registry\[HG\_WND\_COUNT]; // 활성 윈도 핸들 배열 관리
&#x20;     BOOL    is\_minimized\_state;          // 전체 최소화 감지 플래그
&#x20;     HWND    last\_active\_hwnd;            // 최종 포커싱 윈도 핸들 트래킹
&#x20; } HG\_WINDOW\_MANAGER;
&#x20; ```
\- \[ ] \*\*Life-cycle \& Coordination API 디자인\*\*
&#x20; 단일 글로벌 `HG\_WINDOW\_MANAGER` 구조체 전역 변수 하나만 노출하여 모든 창들의 조정을 위임합니다.
&#x20; - `hg\_winmgr\_register(HG\_WINDOW\_TYPE type, HWND hwnd)`: 윈도 생성 시 매니저 테이블에 안전하게 등록합니다.
&#x20; - `hg\_winmgr\_unregister(HG\_WINDOW\_TYPE type)`: 윈도 소멸 시 메모리 제거 및 포인터 정리를 일원화합니다.
&#x20; - `hg\_winmgr\_show\_controlbox()`: 컨트롤 박스 제어 조율. 이미 떠 있다면 숨기지 않고 포커스를 전면(`BringWindowToTop`/`SetForegroundWindow`)으로 당기며, 활성화 모드로 전환합니다.
&#x20; - `hg\_winmgr\_close\_sub\_windows()`: 부차 윈도(Controlbox, Monitor, About 등)를 한 번의 트래버스 루프만으로 예외 없이 일괄 해제합니다.
\---
\## 5. IME 입력기 선택 및 실시간 핫키 설정 체계
글 입력 시 한글/영문 자판 상태가 유실되는 전구 상태(Context State) 꼬임 현상을 치유하고, 다채로운 입력 환경에서 사용자가 유연하게 입력 상태를 조작 및 고정할 수 있는 전용 IME 서비스 계층을 구축합니다.
\- \[ ] \*\*IMM32 라이브러리 연동 및 IME 동작 추상화\*\*
&#x20; - \*\*작업 환경 타겟팅:\*\* `#include <imm.h>` 및 `#pragma comment(lib, "imm32.lib")`를 연계하여 Windows Native Input Method Manager 기능을 제어합니다.
\- \[ ] \*\*윈도별 IME 기본 진입 상태 강제 고정 엔진\*\*
&#x20; 윈도 활성화/입력 활성화(`WM\_SETFOCUS`) 시점의 IME 영문/국문 상태를 INI 설정 정보에 준해 미리 강제 지정할 수 있는 편의 기능을 제공합니다.
&#x20; ```c
&#x20; void hg\_ime\_set\_state(HWND hwnd, BOOL focus\_to\_korean) {
&#x20;     HIMC himc = ImmGetContext(hwnd);
&#x20;     if (himc) {
&#x20;         // IME 활성화/비활성화 상태 제어 (영문은 FALSE, 국문은 TRUE)
&#x20;         ImmSetOpenStatus(himc, focus\_to\_korean);
&#x20;         
&#x20;         if (focus\_to\_korean) {
&#x20;             DWORD conv\_mode, sentence\_mode;
&#x20;             if (ImmGetConversionStatus(himc, \&conv\_mode, \&sentence\_mode)) {
&#x20;                 // 한글 입력 모드로 플래그 변환 강제화
&#x20;                 conv\_mode = (conv\_mode \& \~IME\_CMODE\_ALPHANUMERIC) | IME\_CMODE\_KOREAN;
&#x20;                 ImmSetConversionStatus(himc, conv\_mode, sentence\_mode);
&#x20;             }
&#x20;         }
&#x20;         ImmReleaseContext(hwnd, himc);
&#x20;     }
&#x20; }
&#x20; ```
\- \[ ] \*\*설정 파일 전용 키 추가\*\*
&#x20; - `hgfloater.ini` 내 `\[ime]` 섹션을 신설하여 윈도별 초기 IME 타겟값을 지정해 영타 중심 텍스트 제어 혹은 한타 위주 볼륨 가이드 등 개별 인터페이스의 진입 허들을 낮춥니다.
&#x20; ```ini
&#x20; \[ime]
&#x20; floater\_korean=0        ; 플로터 위젯 진입 시 초기 영타 강제
&#x20; taskbox\_korean=1        ; 태스크 박스 기동 시 기본 한타 세팅
&#x20; controlbox\_korean=0     ; 볼륨/밝기 제어 컨트롤 박스는 영타 유지
&#x20; ```
\- \[ ] \*\*사용가능 자판 레이아웃(Keyboard Layout) 모니터링 메뉴 추가\*\*
&#x20; - `GetKeyboardLayoutList` API를 사용해 시스템에 장착되어 활성화되어 있는 타사 IME 리스트를 수집하고, Context 메뉴를 통하여 실시간으로 입력기를 토글 선택 갱신할 수 있는 브리지 함수를 기안합니다.
\---
\## 6. CLI 환경 명령행 매개변수 파싱 및 IPC 설계 일체화
애플리케이션 추가 실행 시 무분별한 중복 기동을 차단하고, 새롭게 인자(`Command`)와 함께 백엔드 기동된 보조 인스턴스가 기존 실행 중인 프라이머리 윈도로 해당 시그널 스트링을 비침습 방식으로 바이패스 전송(IPC)하여 기존 인스턴스 전면 교정 제어 루프를 활성화합니다.
\- \[ ] \*\*메모리 보호 단일 기동 엔진 (Single Instance Lock)\*\*
&#x20; - `CreateMutexW(NULL, TRUE, L"Local\\\\hgfloater\_single\_instance\_mutex");`를 사용해 중복 진입 시 `FindWindowW`를 통해 세션을 즉각 획득합니다.
\- \[ ] \*\*`WM\_COPYDATA` 기반의 무상태 IPC 메시지 수송 방식 도입\*\*
&#x20; 신규 프로세스가 기동 인자를 감지했을 경우, 프라이머리 타겟 프로세스로 인자 버퍼를 담아 해당 윈도 메시지로 쏘아 올린 후 자신은 클린 퇴장합니다.
\- \[ ] \*\*커맨드 세부 규격 및 명령어 토큰 디스패처\*\*
&#x20; 커맨드라인의 주요 옵션별 타겟 가동 제어를 설계합니다.
&#x20; - `--open-controlbox`: 프라이머리 윈도의 컨트롤박스를 전면 스택으로 밀어 올립니다.
&#x20; - `--toggle-taskbox`: 메인 대시보드 창 투명 토글 온/오프 상태 전환.
&#x20; - `--vol \[relative/percent]`: 현재 시스템 오디오 볼륨을 백라이트 수준에 연주 정교 동정 제어합니다.
&#x20; - `--bright \[percent]`: 모니터 하이레벨 인터럽트 드라이버 루프를 구동해 해당 계조값으로 가변 튜닝을 실행합니다.
&#x20; - `--close`: 실행 중이던 모든 백그라운드 포함 프로세스를 부드러운 순서(`DestroyWindow`)로 폐기하고 완전히 정적 가동합니다.
&#x20; - `--help | --version`: 간단한 CLI API 정보 가이드 팝업을 무음 메인 환경에서 안전하게 토대 제공합니다.
\---
\## 7. 공유 전역 리소스 라이프사이클 관리 고도화 및 누수 차단
개별 윈도가 닫히거나 생성되는 과정에서 공유 전역 자원(공통 폰트, 브러시, 툴팁 윈도 등)이 부적절하게 해제되거나 무효화되는 문제가 존재합니다. 공유 자원의 소유권을 개별 윈도로부터 격리하여 효율적인 전역 수명 주기를 설계합니다.
\- \[ ] \*\*공유 전역 자원 해제 위치 이동\*\*
&#x20; - Taskbox 윈도의 `WM\_DESTROY` 혹은 Controlbox 윈도의 `WM\_DESTROY` 내부에 잔존하는 `DeleteObject(hg\_g\_main\_font)`, `DeleteObject(hg\_g\_edit\_bg\_brush)` 등을 제거합니다.
&#x20; - 이를 전체 애플리케이션의 최종 소멸 단계인 `wWinMain`의 `cleanup\_finish:` 이관하여, 윈도의 생성/소멸 주기와 전역 리소스의 주기를 분리합니다.
\- \[ ] \*\*Lazy-loading 혹은 명시적 일괄 초기화 구조 확립\*\*
&#x20; - 애플리케이션 기동 시 `wWinMain`에서 모든 필수 전역 GDI 리소스를 일괄 캐싱(Pre-cache)하거나, `get\_shared\_main\_font()`와 같은 헬퍼 함수를 통해 리소스가 필요한 시점에 지연 할당(Lazy Allocate) 후 보관하는 안전한 싱글톤 패턴 형태의 접근 제어를 제공합니다.
\- \[ ] \*\*리소스 더블 프리(Double Free) \& 소실 예방 점검\*\*
&#x20; - 각 컨트롤이 다시 칠해지거나 테마 변경으로 재생성될 때, 기존 할당 객체가 유효한지 NULL 검사 후 안전하게 소멸(`DeleteObject` 후 `NULL` 대입) 및 재할당되도록 라이프사이클을 강화합니다.
\---
\## 8. 실시간 포커스 상태 시각 표시기 (Active Focus Visual Indicator)
사용자가 현재 다수의 윈도(Floater, Taskbox, Controlbox) 중 어느 창을 활성화하여 사용하고 있는지 단번에 인식할 수 있도록 직관적이고 시선 집중이 잘 되는 시각적 단서를 추가합니다.
\- \[ ] \*\*CPU/시스템 자원 효율적인 무-애니메이션 비주얼 렌더링 설계\*\*
&#x20; - \*\*절대 불가 사항:\*\* 디졸브(Dissolve), 페이드 인/아웃(Fade in/out), 복잡한 물리 궤적, 키프레임 보간 애니메이션 등 GDI의 반복 연산이나 프레임 동기화를 요구하는 모든 고비용 고급 동적 그래픽 효과는 \*\*일절 가미하지 않고 배제\*\*합니다.
&#x20; - \*\*시각화 원칙:\*\* 오직 CPU 자원을 전혀 낭비하지 않는 순수 정적 드로잉(Static Paint) 및 On/Off 토글 상태 갱신 방식(깜빡임 포함)으로만 경량화 및 단순하게 구현합니다.
\- \[ ] \*\*윈도별 활성화 상태 트래킹 연동\*\*
&#x20; - `WM\_ACTIVATE` 및 `WM\_NCACTIVATE` 이벤트를 포착하여 포커스 상태 변경 시 주 타겟 윈도들의 화면 재갱신(`InvalidateRect(hwnd, NULL, TRUE)`)을 즉각 실행합니다.
\- \[ ] \*\*액티브 보더 페인팅 (Active Border Repainting)\*\*
&#x20; - 비활성 상태(`Inactive`)일 때는 테넌시 장식 테두리를 차분한 회색 조의 기본 테두리(`border\_normal` 혹은 시스템 경계색)로 렌더링합니다.
&#x20; - 활성 상태(`Active`)일 때는 테두리 영역 전체를 Windows 11 Accent Color인 활성 테두리 색(`border\_active`)으로 덧칠하여 엣지 선명도를 높입니다.
\- \[ ] \*\*윈도 좌측 상단 상태 원형 도트 (Focus State LED Indicator Dot)\*\*
&#x20; - 윈도의 좌측 상단 여백 안쪽 구석(예: 보더 경계선 내부 `(x, y) = (border + 8, border + 8)`)에 지름 `8px` 크기의 둥근 포커스 도트 표시 영역을 하드페인팅 기법으로 정적 렌더링합니다.
&#x20; - \*\*활성 포커스 상태:\*\* Windows 11 Accent Color(`accent\_color`) 값을 바탕으로 브러시를 생성해 생동감 넘치고 명도가 높은 채워진 원형 도트(Colored LED Dot)를 그려 현재 활성 상태임을 알립니다.
&#x20; - \*\*비활성 포커스 상태:\*\* 배경색과 완만히 동화되는 단색 회색 도트 혹은 아예 그리지 않도록 단순 펜 처리를 함으로써 시선 분산을 최소화합니다.
\---
\## 9. 통합 설정 관리자(Configuration \& Settings Manager) 서브시스템 및 테마 긴밀 공조 설계
프로그램 전체의 속성(좌표, 폰트 크기, 사운드 볼륨, 밝기 레벨, 테마 설정, IME 지정값 등)을 효율적으로 입출력하고 보존하기 위해, \*\*통합 설정 제어 유틸리티 모듈\*\*을 수립하고 기동 시 테마 엔진과의 긴밀한 동기화 성능을 최적화합니다.
\- \[ ] \*\*통합 설정 구조체(`HG\_CONFIG\_MANAGER`) 정의\*\*
&#x20; - 전역 설정을 중앙 처리 및 상태 동적 조율을 위한 매니저 구조체를 선언합니다.
&#x20; ```c
&#x20; typedef struct \_HG\_SETTINGS {
&#x20;     // 윈도 위치 및 크기 속성
&#x20;     int cx\_floater, cy\_floater, cw\_floater, ch\_floater;
&#x20;     int cx\_taskbox, cy\_taskbox, cw\_taskbox, ch\_taskbox;
&#x20;     int cx\_controlbox, cy\_controlbox, cw\_controlbox, ch\_controlbox;
&#x20;     
&#x20;     // 사용자 선호 모드 정보
&#x20;     int  theme\_index;            // 0: OS 연동(Default Windows Theme), 1: 라이트, 2: 다크, 3: 고대비
&#x20;     int  use\_accent\_color;       // 윈도 Accent Color를 테두리 및 도트에 사용할지의 여부 (BOOL)
&#x20;     int  ime\_floater\_korean;     // Floater 윈도 진입 시 IME 설정 (0: 영타, 1: 한타)
