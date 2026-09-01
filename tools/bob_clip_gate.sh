#!/usr/bin/env bash
# GATE: R21 -- the text clip actually clips, and an EMPTY rect does not clip everything.
#
# Headless: bob_gdi_dc_bits is external to bob_gdi_font.cpp, so the probe stubs it with a plain
# buffer and links the SHIPPED font object. No SDL, no display, no gl-lock queue.
#
# WHY IT EXISTS. MA's twin probe found MA's identical-looking ETO_CLIPPED wiring was INERT -- MA has
# two pixel writers and the clip was on the one that only serves the bitmap-font fallback, while
# every real string went through an unclipped TTF path. That code had been READ twice and pronounced
# correct both times. BoB's clip was written the same way and had never been measured. It passes --
# but "it passes" is only worth saying because something could have failed.
set -u
ROOT="/home/admin/bob"
OBJ="$ROOT/build/SRC/compat/CMakeFiles/bob_compat.dir/bob_gdi_font.cpp.o"
PROBE=/tmp/bob_clip_probe
[ -f "$OBJ" ] || { echo "  no bob_gdi_font object -- build first"; exit 2; }
if [ ! -x "$PROBE" ] || [ "$ROOT/tools/clip_probe.cpp" -nt "$PROBE" ] || [ "$OBJ" -nt "$PROBE" ]; then
  g++ -m32 -fno-pie -no-pie -w -DBOB_LINUX -I "$ROOT/SRC/compat" -I "$ROOT/SRC/H" \
      "$ROOT/tools/clip_probe.cpp" "$OBJ" -o "$PROBE" || { echo "  FAIL: probe did not build"; exit 2; }
fi
echo "R21 clip gate -- text clip clips; empty rect is a no-op"
"$PROBE" 2>&1 | grep -av "face loaded"
exit ${PIPESTATUS[0]}
