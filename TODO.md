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
- [ ] 창 위치/크기/테마/IME/볼륨 관련 설정 구조 정리
- [ ] 저장 및 복원 흐름 정리

## 비고
- 문서는 실제 코드 상태와 맞지 않으면 즉시 수정한다.
- 과장된 설명보다 현재 구현 상태를 우선한다.
- 완료한 항목은 커밋 전에 체크 상태와 설명을 함께 갱신한다.
