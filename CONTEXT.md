# CONTEXT.md — hgfloater

## Current Goal

TODO.md 로드맵 순서대로 리팩터링 진행 중.
현재 다음 단계: **7. 설정 관리자 정리**.

---

## Project Overview

| 항목 | 내용 |
|---|---|
| 프로젝트 | `hgfloater` |
| 스택 | Pure C (C99/C11), WinAPI, 외부 라이브러리 없음 |
| 배포 경로 | `/opt/data/ai-share/hgfloater` |
| 비공개 경로 | `/opt/data/hgfloater-internal` |
| GitHub | https://github.com/rubidus-api/hgfloater |
| 빌드 서버 | `ssh -i /opt/data/ai-share/arch-dev-ssh-key-hermes -p 51265 hermes@192.168.120.112` |
| 빌드 경로 | `/home/hermes/hgfloater-build/` |
| ai-share 공유 | hermes 서버 `/opt/data/ai-share` = arch-dev `/home/hermes/ai-share` |

---

## Source Structure

```
hgfloater/
├── hgfloater.c          # 메인 소스 (6111줄, 단일 파일 구조)
├── hg_about_text.h      # About 창 텍스트 헤더
├── build.bat            # Windows 빌드 스크립트 (MinGW + windres)
├── test/
│   ├── test_placeholder.c
│   ├── test_cast.c
│   ├── test_cast2.c
│   └── test_controlbox.c
├── AGENTS.md            # 프로젝트 규칙 (시작점)
├── TODO.md              # 작업 순서 및 진행 상태
├── CONTEXT.md           # 현재 파일 (작업 맥락)
├── SPEC.md              # 기능 명세
├── CHECKLIST.md         # 구현 체크리스트
├── TESTS.md             # 테스트 계획
├── CHANGELOG.md         # 변경 이력
├── MEMORY.md            # 설계 결정 기록
└── README.md            # 공개 문서
```

---

## Status

### 완료된 작업

| 단계 | 내용 | 커밋 |
|---|---|---|
| 매크로 정리 | `HG_` 접두어 일괄 적용 | 초기 커밋 |
| Floater Controller | WM_* 핸들러를 helper로 분리 | 초기 커밋 |
| Taskbox Controller | WM_* 핸들러를 helper로 분리 | `81578b6` |
| Controlbox Controller | WM_* 핸들러를 helper로 분리 | `199dbc3` |
| Toolbar Controller | paint/mouse/button/wheel 핸들러 분리 완료 | `44cfebf` |
| 테마 처리 | DWM accent color, WM_SETTINGCHANGE, 고대비 모드 | `21a7333` |
| Window Manager | 창 클래스 등록/해제 중앙화, 보조 창 전환 helper | `41dfcba` |
| IME 처리 | read-only edit IME 비활성화, 입력 언어 흐름 정리 | `502afd3` |
| 단일 인스턴스 | mutex 기반 단일 실행 보장, 기존 인스턴스 활성화 | `e6fc2e0` |
| WM_COPYDATA IPC | 시작 인자를 기존 인스턴스에 전달 | `264d3e3` |
| CLI 디스패처 | `HgCliAction` 기반 옵션 파싱/디스패치 | `2f4b96d` |
| 전역 리소스 수명 주기 | wWinMain cleanup_finish 통합 해제 및 소유권 정리 | `2dbd45d` |
| 활성 포커스 시각 표시기 | WM_ACTIVATE 포커스 분기 및 액센트 도색, 활성 도트 구현 | `07ad611` |

### 미커밋 변경 (작업 트리)

- 없음 (모든 변경사항이 Git에 커밋 완료됨)

---

## Next Actions

1. **7. 설정 관리자 정리**:
   - 창 위치/크기/테마/IME/볼륨 설정 구조 정리
   - 저장 및 복원 흐름 정리

---

## Known Issues / Notes

- `hgfloater.c`가 6111줄 단일 파일로, 변경 영향 범위가 큼. 장기적으로 모듈 분리 검토 필요.
- AGENTS.md 기준 `HJKL` 바인딩 제거가 명시되어 있으나, Floater/Taskbox WM_KEYDOWN 경로에 일부 잔존. 추후 정리 필요.
- `build.bat`의 테스트 경로가 `tests\*.c`로 되어 있으나 실제 폴더는 `test/`. 빌드 스크립트 수정 필요.
- 테스트는 smoke test 수준. 런타임 동작 자동 검증 없음.
- 빌드 검증은 반드시 arch-dev에서 Windows x64 크로스 컴파일로 수행.
- 배포 경로 파일에 개인정보 포함 금지. 비공개 파일은 `/opt/data/hgfloater-internal`에 보관.

---

## Build & Test Commands (arch-dev)

```sh
# SSH 접속
ssh -i /opt/data/ai-share/arch-dev-ssh-key-hermes -p 51265 hermes@192.168.120.112

# Windows x64 크로스 컴파일
x86_64-w64-mingw32-gcc -o hgfloater.exe hgfloater.c \
  -mwindows -lcomctl32 -lole32 -lshell32 -luuid \
  -lwinmm -limm32 -ldwmapi \
  -Wall -Wextra -O2

# 테스트 개별 컴파일
x86_64-w64-mingw32-gcc -o test_controlbox.exe test/test_controlbox.c \
  -mwindows -lcomctl32 -lole32 -lshell32 -luuid

x86_64-w64-mingw32-gcc -o test_placeholder.exe test/test_placeholder.c \
  -subsystem,windows -mwindows

x86_64-w64-mingw32-gcc -o test_cast.exe test/test_cast.c
x86_64-w64-mingw32-gcc -o test_cast2.exe test/test_cast2.c
```

---

_Last updated: 2026-05-24_
