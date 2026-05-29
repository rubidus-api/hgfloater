# hgfloater TODO

이 문서는 hgfloater의 현재 진행 상태와 다음 작업 순서를 관리하는 단일 원천입니다.

## 작업 규칙
- 한 번에 한 단계씩만 진행한다.
- 각 단계가 끝나면 해당 체크박스를 즉시 갱신한다.
- 코드 변경이 있으면 TODO.md도 함께 갱신한다.
- 빌드와 검증은 arch-dev에서 수행한다.
- 새 작업을 시작하기 전에 관련 코드와 문서를 먼저 확인한다.

## 현재 진행 상태
- [x] 사용자 정의 매크로 상수에 `HG_` 접두어 적용
- [x] Floater Widget Controller 분리 및 바인딩
- [x] Taskbox Controller 분리 및 바인딩
- [x] Controlbox Controller 분리 및 바인딩
- [x] Toolbar Controller 분리 및 바인딩

## 세부 진행 로그

### 2026-05-22
- 매크로 상수 네이밍 정리 완료
- Floater / Taskbox / Controlbox controller 분리 완료
- Toolbar controller의 남은 메시지 핸들러를 helper로 정리 완료
- Toolbar controller 분리 작업은 현재 완료 상태

### 2026-05-23
- 시스템 테마 색상과 DWM accent color를 읽어 테마 팔레트를 갱신하는 구조를 정리함
- 고대비 모드는 별도 항목으로 남겨 두고, 현재 단계에서는 accent color 반영만 적용함
- `WM_SETTINGCHANGE`, `WM_THEMECHANGED`, `WM_SYSCOLORCHANGE`, `WM_DWMCOLORIZATIONCOLORCHANGED`를 같은 테마 갱신 경로로 묶음
- 고대비 모드는 시스템 색상 팔레트와 DWM dark mode 비활성화로 대응함

## 후속 로드맵

### 1. 테마 처리 정리
- [x] 시스템 테마/Accent Color를 반영하는 테마 구조 정리
- [x] `WM_SETTINGCHANGE` 및 색상 변경 메시지 처리 정리
- [x] 고대비 모드 대응 검토

### 2. Window Manager 정리
- [x] 창 등록/해제용 중앙 관리 구조 정리
- [x] controlbox / about / monitor 류 보조 창 전환 로직 정리

### 2026-05-23
- 창 클래스 등록/해제용 중앙 관리 헬퍼를 도입하고, 관련 클래스명을 상수로 묶음
- Floater / Taskbox / Controlbox / About / Monitor 클래스 등록 경로를 단일 함수로 정리함
- 읽기 전용 edit 컨트롤의 IME 초기 상태를 창 생성 및 포커스 경로에서 끄도록 정리함

### 3. IME 및 입력 상태 처리
- [x] IME 초기 상태 설정 경로 정리
- [x] 설정 파일 기반 초기값 로딩 검토
- [x] 키보드 레이아웃/입력기 선택 흐름 정리

### 2026-05-23
- 설정 파일에서 불러오는 초기값 보정 경로를 점검하고, hotkey 초기화가 일부 키 누락 시에도 기본값으로 복원되도록 정리함
- 읽기 전용 edit 컨트롤에서 IME 재활성화와 입력 언어 변경 요청을 흡수해 키보드 레이아웃/입력기 선택 흐름이 흔들리지 않도록 정리함
- 단일 인스턴스 진입 경로를 helper로 정리하고, 기존 인스턴스 활성화 동작을 중앙화함
- `WM_COPYDATA`로 시작 인자를 기존 인스턴스에 전달하고, 현재 인스턴스에서도 동일한 command line을 보관하도록 정리함

### 4. 단일 인스턴스 및 IPC
- [x] 단일 실행 보장 로직 정리
- [x] `WM_COPYDATA` 기반 인자 전달 정리
- [x] CLI 옵션 디스패처 정리

### 2026-05-24
- CLI 옵션 파싱과 실행을 `HgCliAction` 기반 디스패처로 정리함
- `--show`, `--hide`, `--toggle`, `--about`, `--exit`, `--help`를 단일 실행 인스턴스와 기존 인스턴스 전달 경로에서 동일하게 처리하도록 묶음

### 5. 전역 리소스 수명 주기
- [x] 공통 폰트/브러시/툴팁 등의 소유권 정리
- [x] 생성/해제 위치를 애플리케이션 종료 지점 기준으로 재배치
- [x] 더블 프리 및 NULL 재초기화 점검

### 2026-05-29
- wWinMain의 `cleanup_finish:` 블록으로 모든 전역 GDI 리소스(HFONT, HBRUSH), 윈도우 핸들(taskbox, controlbox, about, floater, tooltip), 아이콘 리소스를 안전하게 해제하도록 단일 원천화 및 통합 리팩터링 수행 완료.
- 각 리소스 해제 후 NULL 재초기화를 보장하여 더블 프리(Double Free) 방지 및 안전한 수명 주기 관리 구축 완료.

### 6. 활성 포커스 시각 표시기
- [x] 활성/비활성 상태 재도색 경로 정리
- [x] 테두리 및 상태 도트 렌더링 방식 검토

### 2026-05-29
- taskbox와 controlbox에 `WM_ACTIVATE` 메시지 처리기를 설계하여, 사용자가 다른 애플리케이션으로 포커스를 전환(WA_INACTIVE)할 경우 자동으로 창이 닫히도록 완벽한 활성/비활성 상태 전환 메커니즘을 구현함.
- 앱 내부 팝업 메뉴 및 설정 관련 창 활성화 시에 오작동이 일어나지 않도록 프로세스 ID(PID) 매칭 필터링을 더해 실동작 수준의 UX 견고함을 확보함.
- 포커스 상태에 따라 테두리 색상(활성 시 시스템 액센트 컬러 `HG_COLOR_BORDER_SELECTED`, 비활성 시 중립 `HG_COLOR_BG_TOOLBAR`)이 동적으로 도색되도록 Paint Logic을 고도화하고, `WM_ACTIVATE` 발생 시 `InvalidateRect`를 통해 즉시 재도색 경로를 타도록 정비함.
- 현재 OS상의 활성(Foreground) 창을 핫키 시점에 감지(`hg_g_prev_active_hwnd`)하여, 태스크 스위처 내 해당 프로세스 아이콘 하단에 테마 기반의 우아한 액센트 닷/필(Pill) 형태의 활성 상태 표시기(Active Status Dot)를 동적으로 그리도록 검토 및 구현 완수함.

### 7. 설정 관리자 정리
- [x] 창 위치/크기/테마/IME/볼륨 관련 설정 구조 정리
- [x] 저장 및 복원 흐름 정리

### 2026-05-29
- 분산되어 있던 설정 데이터 흐름과 리셋 로직을 단일 인터페이스로 정제하여, 모든 위젯(floater, taskbox, controlbox) 및 시스템 속성(알파, 폰트명, 폰트 크기, 핫키, 모니터 배치 등)을 안전하게 원자적으로 일괄 리셋하고 파일 및 런타임에 동시 적용하는 `hg_config_reset_all()` 설정 관리자 통합 제어 함수를 완벽히 구현 및 바인딩 완료함.
- `HG_IDM_RESET_ALL` 메시지 처리 시에 기존의 불안정했던 인라인 강제 복구 및 undefined 변수(GDI leak 및 컴파일 에러 유발 가능 코드)를 깔끔하게 걷어내고, 새롭게 추상화된 설정 관리자 단일 함수를 호출하도록 구조 변경을 완료하여 설정 저장 및 복원 흐름의 모듈성을 전폭 향상시킴.

### 8. 레거시 HJKL 바인딩 완전 제거
- [x] floater 및 taskbox 내 모든 interaction paradigm에서 legacy HJKL 단축키를 완전히 제거하고 Alt + WASD / Alt + Arrow Keys 또는 Ctrl + A/D, Ctrl + W/S 위주로 정리 완료.

### 2026-05-29
- floater_controller_on_keydown 및 taskbox_controller_on_keydown에 남아 있던 레거시 VI-style HJKL 단축키 매핑을 완전히 걷어내고, SPEC.md 명세에 맞춰 Alt + WASD, Alt + Arrow Keys 또는 Ctrl + A/D, Ctrl + W/S 위주로 키 바인딩을 통일하고 주석도 최신화함.

### 9. 실동작 버그 2종 수정
- [x] 모니터 창이 로드되지 않던 현상 해결 (태스크박스 Destroy 시 GDI 전역 폰트/브러시 리소스를 무분별하게 조기 파괴하여 이후 창 생성 시 누락되던 버그를 wWinMain cleanup_finish에 단일 일임하여 해결)
- [x] 컨트롤박스가 빈 영역 마우스 우클릭으로 제거되지 않던 문제 해결 (controlbox_proc에 WM_RBUTTONUP 메시지 핸들러 추가 및 DestroyWindow 바인딩 완료)

### 2026-05-29 (추가)
- taskbox_controller_on_destroy 내부에서 전역 HFONT, HBRUSH, 툴팁 윈도우를 소거하던 중복 삭제 구문을 완전히 거두어내고 wWinMain의 단일 리소스 관리 지점으로 일임하여 모니터 뷰어 및 다른 보조 창들이 리소스 유실 없이 영구히 잘 생성되도록 조치 완료.
- controlbox_proc에 WM_RBUTTONUP 라우트를 정상 탑재하여 컨트롤박스 빈 영역 우클릭 감지 즉시 파괴(close)되도록 사용자 요구사항 완수함.

### 10. 실동작 버그 3종 수정 (추가 요구사항)
- [x] 컨트롤박스 상단의 제목(캡션) 우클릭 시 창 닫히지 않는 문제 해결 (controlbox_proc에 WM_NCRBUTTONUP 및 HTCAPTION 처리기 추가)
- [x] F5키 누를 시 컨트롤박스 위치가 0,0으로 초기화되지 않는 문제 해결 (메시지 루프에서 controlbox 및 그 자식 포커스 시 VK_F5 강제 전달 및 controlbox_proc에 VK_F5 핸들러 바인딩)
- [x] 모니터박스 최초 기동 시 컨트롤 에디트 글꼴이 기본 시스템 글꼴로 폴백되는 문제 해결 (monitor_wnd_proc의 WM_CREATE 및 WM_SIZE에 hg_g_main_font 레이지 로드 방어 코드 구축)

### 2026-05-30
- `controlbox_proc`에 `WM_NCRBUTTONUP` 메시지 라우트를 추가하고 `w_param == HTCAPTION`일 때 창을 닫도록 구현하여, 컨트롤박스 상단 제목 영역 우클릭 시 즉각 닫히도록 완수함.
- `GetMessage` 메시지 루프에서 포커스가 `controlbox` 또는 그 자식 윈도우에 있고 `VK_F5`가 눌렸을 때 직접 `WM_KEYDOWN` 메시지를 전달하도록 라우팅 필터를 설계하여, 자식 트랙바에 포커스가 묶인 상황이나 액셀러레이터 테이블 간섭 상황에서도 0,0 좌표로 리셋되도록 완수함.
- `monitor_wnd_proc`의 `WM_CREATE` 및 `WM_SIZE` 진입부에 lazy-initialization 구조를 더해 `hg_g_main_font`가 미처 준비되지 않은 시점에 모니터 뷰어가 열려도 깨끗한 전역 폰트 자원을 직접 빌드 및 로드하도록 구현하여 시스템 기본 폰트 폴백 버그를 완벽히 예방하고 완수함.

## 비고
- 문서는 실제 코드 상태와 맞지 않으면 즉시 수정한다.
- 과장된 설명보다 현재 구현 상태를 우선한다.
- 완료한 항목은 커밋 전에 체크 상태와 설명을 함께 갱신한다.
