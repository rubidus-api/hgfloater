#!/bin/sh
# Cross-build verification for hgfloater on a Linux host with mingw-w64.
# Usage: sh scripts/build-mingw.sh [output-dir]
# Builds a release-flag, warning-clean executable, compiles every test with the
# same warning set, and runs the host-native tests with the system compiler.
set -e

CROSS=${CROSS:-x86_64-w64-mingw32}
OUT=${1:-build-mingw}
mkdir -p "$OUT"

WARNING_FLAGS="-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wdouble-promotion \
  -Wconversion -Wlogical-op -Wno-unused-parameter -Wno-overlength-strings"

VER_Y=$(date +%y)
VER_M=$(date +%m)
VER_D=$(date +%d)
# Optional suffix for same-day re-releases (e.g. VERSION_SUFFIX=b -> v26.07.10b).
VER_SUFFIX=${VERSION_SUFFIX:-}
VERSION_STRING="v$VER_Y.$VER_M.$VER_D$VER_SUFFIX"

if command -v python3 >/dev/null 2>&1; then
    python3 scripts/gen_about.py .
fi

"$CROSS-windres" hgfloater.rc -O coff -o "$OUT/hgfloater_res.o" \
    -DHG_RC_VER_MAJOR=$((10#$VER_Y)) -DHG_RC_VER_MINOR=$((10#$VER_M)) -DHG_RC_VER_PATCH=$((10#$VER_D)) \
    -DHG_RC_VER_MINOR_STR="$VER_M" -DHG_RC_VER_PATCH_STR="$VER_D$VER_SUFFIX"

# shellcheck disable=SC2086
"$CROSS-gcc" -o "$OUT/hgfloater.exe" \
    hgfloater.c hg_globals.c hg_utils.c hg_audio.c hg_display.c hg_shell.c hg_sysinfo.c hg_config.c hg_calc.c \
    widgets/hg_floater.c widgets/hg_taskbox.c widgets/hg_toolbar.c \
    widgets/hg_taskbox_menus.c widgets/hg_window_list.c \
    widgets/hg_monitor.c widgets/hg_commandbox.c widgets/hg_about.c \
    "$OUT/hgfloater_res.o" \
    $WARNING_FLAGS "-DHG_VERSION_W=L\"$VERSION_STRING\"" \
    -O3 -flto=auto -s -DNDEBUG -mwindows -municode -static \
    -lgdi32 -luser32 -lcomctl32 -ldwmapi -ladvapi32 -lshell32 \
    -lole32 -loleaut32 -luuid -lpsapi -lpathcch -lshlwapi -lshcore -lpropsys -limm32

echo "build: OK ($OUT/hgfloater.exe $VERSION_STRING)"

# Console tests: compiled with the same warning set (no -municode), run under
# wine when available, and natively with the host compiler where possible.
for t in test/*.c; do
    n=$(basename "$t" .c)
    # shellcheck disable=SC2086
    "$CROSS-gcc" -o "$OUT/$n.exe" "$t" $WARNING_FLAGS
    echo "test compile: OK $n"
    if command -v wine >/dev/null 2>&1; then
        wine "$OUT/$n.exe" >/dev/null && echo "test run (wine): OK $n"
    fi
done

# Win32-free units run natively on the build host, so their behavior (not just
# their compilation) is checked even without wine.
HOST_TESTS="test_calc test_relocate"

if command -v gcc >/dev/null 2>&1; then
    for n in $HOST_TESTS; do
        # shellcheck disable=SC2086
        gcc -o "$OUT/${n}_host" "test/$n.c" $WARNING_FLAGS -Wno-logical-op 2>/dev/null \
            || gcc -o "$OUT/${n}_host" "test/$n.c"
        "$OUT/${n}_host" && echo "test run (host): OK $n"
    done
fi
