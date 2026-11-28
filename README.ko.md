**한국어** | [English](README.md) — **HGFloater v0.18.3** — [Pages](https://rubidus-api.github.io/hgfloater/) — [EXE(x64)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater.exe) · [PDF(en)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-en.pdf) · [PDF(ko)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-ko.pdf) · [HTML(en)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-en-html.zip) · [HTML(ko)](https://github.com/rubidus-api/hgfloater/releases/download/v0.18.3/hgfloater-manual-0.18.3-ko-html.zip)

# HGFloater

**윈도우 11**용으로, 떠 있는 시계를 누르면 작업 전환기 · 실행기 · 제어판이 펼쳐지는 프로그램입니다.
순수 C 와 Win32 API 로 작성한 약 700 KB짜리 실행 파일 하나이고, 설치 과정도 런타임도 없습니다.

**v0.18.3** — 빌드 2026-11-28

[홈페이지](https://rubidus-api.github.io/hgfloater/ko.html) · [사용 설명서](https://rubidus-api.github.io/hgfloater/ko/) · [User manual](https://rubidus-api.github.io/hgfloater/en/)

<img src="https://rubidus-api.github.io/hgfloater/screenshots/taskbox.png" alt="태스크박스: 실행 중인 창과 단축 아이콘이 아이콘으로 놓이고, 버튼 열 개가 오른쪽 아래 구석에서 끝나며, 맨 위에 상태 표시줄이 있다" width="480">

## 왜 HGFloater인가

윈도우 기본 작업 표시줄은 창, 프로그램, 볼륨, 밝기, 설정을 서로 다른 곳에 두고, 저마다 몇 번씩
눌러 들어가야 합니다. HGFloater 는 이것들을 어느 모니터에나, 작업 표시줄 위에도 둘 수 있는 작은
상자 하나에 모읍니다.

- **아이콘 하나에 창 하나, 아이콘 하나에 키 하나.** 열린 창, 모아 둔 단축 아이콘, 컨트롤이 모두
  격자 하나에 있습니다. `Shift + 3` 은 언제나 네 번째 창이고 `Shift + K` 는 언제나 같은 프로그램입니다.
- **처음부터 가볍게.** 서비스도, 뒤에서 도는 도우미도, 네트워크 접속도, 로그도 없습니다. 설정과
  노트는 폴더 하나에 든 평범한 파일입니다.
- **키보드와 마우스가 동등합니다.** 모든 동작에 마우스 조작과 키가 있고, 키는 모두 바꿀 수 있습니다.

## 내려받기와 설치

**[hgfloater.exe 내려받기 — v0.18.3, 700 KB](https://github.com/rubidus-api/hgfloater/releases/latest/download/hgfloater.exe)** · [모든 릴리스](https://github.com/rubidus-api/hgfloater/releases)

윈도우 11 이상, 64비트에서 돕니다. 설치 과정이 없으니 `hgfloater.exe` 를 아무 곳에나 두고 실행하면
됩니다. 서명이 없는 파일이라 릴리스마다 SHA-256 을 적어 둡니다. PowerShell 에서
`Get-FileHash .\hgfloater.exe -Algorithm SHA256` 으로 확인할 수 있습니다.

## 빠르게 써 보기

1. `hgfloater.exe` 를 실행합니다. 포인터가 있는 모니터에 시계가 든 작은 상자, 플로터가 나타납니다.
2. 플로터를 클릭하거나 어디서든 `Win + Alt + Space` 를 누르면, 창들과 버튼이 든 태스크박스가
   열립니다.
3. `Set` 을 가리키고 **Open Shortcuts Folder** 를 고른 뒤, 아무 `.lnk` 나 그 폴더에 복사해 넣습니다.
   1초 안에 태스크박스에 아이콘이 생깁니다.
4. `F1` 은 내장 도움말, `Ctrl + Q` 는 끝내기입니다.

## 기능

- **작업 전환기**: 창마다 키 배지가 달린 아이콘이 하나씩 있습니다. 아이콘을 가리키면 바탕화면에서
  그 창의 윤곽이 보이고, 브라우저와 탐색기의 탭도 따로 아이콘이 될 수 있습니다.
- **실행기**: 단축 아이콘 폴더가 곧 설정입니다. 폴더 바로 가기는 `Dir` 목록으로 갑니다.
- **소리와 화면**: `Vol` · `Mon` · `Alp` 이 볼륨 · 밝기 · 투명도를 게이지로 보이고 휠로 바뀝니다.
  A/V 제어 센터에서 오디오 장치, 화면 배치, 배율, 해상도, DDC/CI 밝기를 바꿉니다.
- **노트**: 평범한 `.txt` 파일이고, 검색 · 보관 · 끌어서 순서 바꾸기가 됩니다.
- **클립보드 히스토리**: 텍스트만, 메모리에만 두고 디스크에는 쓰지 않습니다.
- **커맨드 박스**: 번호로 창을 다루고(`go 1`, `move 1 100 100 2`), 노트를 열고, 어떤 설정이든
  바꿉니다.
- **직접 쓰는 시계 서식**: 플로터의 두 줄이 `strftime` 코드를 받고, 옆에 CPU · 온도 · GPU · 메모리 ·
  배터리 막대가 있습니다.

기능, 키, 설정은 [사용 설명서](https://rubidus-api.github.io/hgfloater/ko/)에 모두 있습니다.

## 하지 않는 것

작업 표시줄이나 시작 메뉴를 대신하지 않고, 서비스를 설치하지 않고, 관리자로 돌지 않고, 네트워크에
접속하지 않습니다. 실행 파일은 소켓이나 HTTP 함수를 하나도 가져오지 않습니다. 실행 파일에 코드
서명을 하기 전까지 공개 빌드에서는 두 기능을 꺼 둡니다. 최대화 버튼 메뉴(시스템 전역 마우스 훅이
필요합니다)와 **Start with Windows** 입니다.

## 상태

릴리스마다 경고 없이 빌드하고, 게시하기 전에 윈도우 11 가상 머신에서 동작을 자동으로 검사합니다.
그 머신에는 가상 모니터 하나뿐이고 실제 백라이트와 USB 음성 녹음기가 없습니다. 그래서 세 가지는
아직 확인하지 못했습니다: DDC/CI 로 실제 모니터의 밝기를 바꾸는 일, 모니터 둘 이상에서 쓰는 일,
녹음기의 시계 명령입니다. 녹음기 설정 창은 제작자가 실제 녹음기에서 확인했습니다.

## 문서

- 스크린샷이 있는 [홈페이지](https://rubidus-api.github.io/hgfloater/ko.html).
- 사용 설명서: [웹(한국어)](https://rubidus-api.github.io/hgfloater/ko/) ·
  [웹(영어)](https://rubidus-api.github.io/hgfloater/en/). PDF 와 오프라인 HTML 판은 릴리스마다
  첨부합니다.
- 릴리스 기록은 [CHANGELOG.md](CHANGELOG.md).

## 빌드

MinGW-w64 GCC 와 GNU `make` 만 있으면 됩니다.

```sh
make CROSS=x86_64-w64-mingw32- OUT=build release    # 리눅스나 macOS 에서
build.bat release                                    # MSYS2 를 설치한 윈도우에서
```

툴체인 준비와 릴리스 절차는 [설명서](https://rubidus-api.github.io/hgfloater/ko/reference-build.html)에
있습니다.

## 라이선스

MIT 입니다. [LICENSE](LICENSE)를 보세요. HGFloater 는 rubidus-api 가 AI 의 도움을 받아 만드는 작은
윈도우 유틸리티 모음, HellGates 시리즈의 하나입니다.
