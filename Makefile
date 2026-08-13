# hgfloater build
#
# Native (MSYS2 MinGW64 on Windows):
#     make               release build -> hgfloater.exe
#     make debug         unoptimised build with symbols
#     make test          build and run the console tests
#     make clean
#
# Cross build (Linux/macOS with a mingw-w64 toolchain):
#     make CROSS=x86_64-w64-mingw32-
#
# Useful variables:
#     OUT=build          where object files and the exe are written (default: .)
#
# build.bat offers the same builds through an interactive menu on Windows; both
# read the source list from here so the two cannot drift apart.

CROSS   ?=
CC      := $(CROSS)gcc
WINDRES := $(CROSS)windres
PYTHON  ?= python3

OUT ?= .
EXE := $(OUT)/hgfloater.exe

# The version is semantic and lives in VER.txt (vMAJOR.MINOR.PATCH); bump it
# there when cutting a release. The build stamp is the moment this build was
# made, shown in the About window - the version says what, the stamp says when.
VERSION_STRING := $(strip $(shell cat VER.txt))
VER_CORE  := $(subst v,,$(VERSION_STRING))
VER_MAJOR := $(word 1,$(subst ., ,$(VER_CORE)))
VER_MINOR := $(word 2,$(subst ., ,$(VER_CORE)))
VER_PATCH := $(word 3,$(subst ., ,$(VER_CORE)))
# Overridable so a release can carry the exact stamp written into the README:
#   BUILD_STAMP="2026-08-05 15:33 KST" make release
BUILD_STAMP ?= $(shell date '+%Y-%m-%d %H:%M %Z')

# Strict by default: the build is expected to stay warning-clean.
WARNING_FLAGS := -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wdouble-promotion \
                 -Wconversion -Wlogical-op -Wno-unused-parameter -Wno-overlength-strings

RELEASE_FLAGS := -O3 -flto=auto -s -DNDEBUG
DEBUG_FLAGS   := -g -O0 -DDEBUG

WIN_FLAGS := -mwindows -municode -static
LIBS := -lgdi32 -luser32 -lcomctl32 -ldwmapi -ladvapi32 -lshell32 -lole32 -loleaut32 \
        -luuid -lpsapi -lpdh -luuid -loleaut32 -lpathcch -lshlwapi -lshcore -lpropsys -limm32

SRC := \
	src/hgfloater.c \
	src/hg_globals.c \
	src/hg_utils.c \
	src/hg_audio.c \
	src/hg_display.c \
	src/hg_wmi.c \
	src/hg_shell.c \
	src/hg_sysinfo.c \
	src/hg_config.c \
	src/hg_calc.c \
	src/hg_command.c \
	src/hg_values.c \
	src/hg_tabs.c \
	src/hg_caphook.c \
	src/widgets/hg_floater.c \
	src/widgets/hg_taskbox.c \
	src/widgets/hg_toolbar.c \
	src/widgets/hg_taskbox_menus.c \
	src/widgets/hg_window_list.c \
	src/widgets/hg_monitor.c \
	src/widgets/hg_commandbox.c \
	src/widgets/hg_note.c \
	src/widgets/hg_clip.c \
	src/widgets/hg_tabbox.c \
	src/widgets/hg_hilite.c \
	src/widgets/hg_about.c

RES := $(OUT)/hgfloater_res.o

TESTS := $(wildcard test/*.c)
# Units free of Win32 also run on the build host, so their behaviour is checked
# even when no Windows runtime (or wine) is available.
HOST_TESTS := test_calc test_relocate

.PHONY: all release debug test test-compile test-host about clean help

all: release

release: MODE_FLAGS := $(RELEASE_FLAGS)
release: $(EXE)

debug: MODE_FLAGS := $(DEBUG_FLAGS)
debug: $(EXE)

# The About dialog renders README.md, so the generated header is refreshed on
# every build rather than edited by hand.
about:
	@$(PYTHON) scripts/gen_about.py . || echo "[Warning] About text not regenerated."

$(RES): src/hgfloater.rc | $(OUT)
	$(WINDRES) src/hgfloater.rc -O coff -o $@ \
		-DHG_RC_VER_MAJOR=$(VER_MAJOR) \
		-DHG_RC_VER_MINOR=$(VER_MINOR) \
		-DHG_RC_VER_PATCH=$(VER_PATCH) \
		-DHG_RC_VER_MINOR_STR=$(VER_MINOR) \
		-DHG_RC_VER_PATCH_STR=$(VER_PATCH)

$(EXE): about $(SRC) $(RES) | $(OUT)
	$(CC) -o $@ $(SRC) $(RES) $(WARNING_FLAGS) $(MODE_FLAGS) $(WIN_FLAGS) \
		-DHG_VERSION_W=L\"$(VERSION_STRING)\" \
		'-DHG_BUILD_STAMP_W=L"$(BUILD_STAMP)"' $(LIBS)
	@echo "build: OK ($@ $(VERSION_STRING), built $(BUILD_STAMP))"

$(OUT):
	@mkdir -p $(OUT)

# Every test compiles for Windows with the same warning set; the Win32-free ones
# also build and run natively so failures surface without a Windows host.
test: test-compile test-host

test-compile: | $(OUT)
	@for t in $(TESTS); do \
		n=$$(basename $$t .c); \
		$(CC) -o $(OUT)/$$n.exe $$t $(WARNING_FLAGS) || exit 1; \
		echo "test compile: OK $$n"; \
	done

test-host: | $(OUT)
	@for n in $(HOST_TESTS); do \
		gcc -o $(OUT)/$${n}_host test/$$n.c $(WARNING_FLAGS) -Wno-logical-op 2>/dev/null \
			|| gcc -o $(OUT)/$${n}_host test/$$n.c || exit 1; \
		$(OUT)/$${n}_host || exit 1; \
		echo "test run (host): OK $$n"; \
	done

clean:
	rm -f $(EXE) $(RES) $(OUT)/*.exe $(OUT)/*_host

help:
	@echo "make [release|debug|test|clean]   CROSS=x86_64-w64-mingw32-   OUT=build"
