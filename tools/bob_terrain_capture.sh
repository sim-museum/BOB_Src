#!/usr/bin/env bash
# tools/bob_terrain_capture.sh -- TERRAIN-1/2 evidence recipe (2026-09-26), real GL, scratch drive_c.
#
#   gl-lock tools/bob_terrain_capture.sh <outdir> <name> [env...]
#
# Quick Mission (QM=1 "Landing", ~4,000 ft with an airfield in view under BOB_QM_WEATHER=0 +
# BOB_NOCLOUDS=1; QM=18 "Low level attack", ~600 ft), F6 external view at 6 s, P (pause) at 9 s,
# one numpad-4 view step at 14 s. Dumps 200 consecutive presents starting 400 presents after the
# first key: static paused frames, then the view move, then static again. Add
#   BOB_PERSP_FLIP_EVERY=40    to render the SAME paused state with and without TERRAIN-1
#   BOB_NO_LANDMIP=1           for TERRAIN-2's control arm
# BOB_TRACE_PERSP=20 is on: the log carries the affine-error / SWIM readings and the land-texture list.
set -u
OUT="${1:?usage: bob_terrain_capture.sh <outdir> <name> [env...]}"; NAME="${2:?name}"; shift 2
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd)"
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"
BOB="${BOB:-$(cd "$(dirname "$0")/.." && pwd)/build/bob}"
cd "$GD" || exit 2
timeout -k 5 -s KILL "${TMO:-90}" env DISPLAY="${DISPLAY:-:0}" BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
  BOB_BOOT_FRONTEND=1 BOB_QM_INDEX="${QM:-1}" BOB_QM_WEATHER=0 BOB_NOCLOUDS=1 BOB_TRACE_HUD=1 BOB_TRACE_PERSP=20 \
  BOB_SDL_KEY_MS_LAUNCHREL=1 BOB_SDL_KEY_MS="6000,1073741887,150;9000,112,150;14000,1073741916,120" \
  BOB_DUMP_AFTER_KEY=400 BOB_DUMP_FRAME_COUNT=200 BOB_EXIT_AFTER_DUMP=1 BOB_DUMP_PATH="$OUT/$NAME.ppm" \
  "$@" "$BOB" > "$OUT/$NAME.log" 2>&1
rc=$?
n=$(ls "$OUT/$NAME".ppm.[0-9][0-9][0-9].ppm 2>/dev/null | wc -l)
echo "  $NAME: exit=$rc, $n frames dumped, last HUD: $(grep -a '^\[hud\]' "$OUT/$NAME.log" | tail -1)"
[ "$n" -gt 0 ]
