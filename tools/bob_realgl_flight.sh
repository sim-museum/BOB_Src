#!/usr/bin/env bash
# tools/bob_realgl_flight.sh — R3.2 S4. THE REAL-GL AIRBORNE RECIPE, written down.
#
# R3.2 S3 found that the campaign recipe in bob_combat_soak.sh already reaches a flight IN THE AIR
# and had only ever been run with SDL_VIDEODRIVER=dummy, where draw_fvf never executes and no frame
# can be captured. Dropping that one variable and running the same clicks on DISPLAY=:0 gives a
# cockpit at ~4,800 ft on real GL. That recipe was reconstructed by hand in S3 and is the bridge
# THREE items depend on (R3.2 clouds, R3.7 muzzle flash, R3.9 grey square), so it lives in a script
# now rather than in a sprint note -- a recipe kept in prose is a recipe that silently stops being
# run the same way twice (see bob_combat_soak.sh's own four-times-wrong header).
#
#   gl-lock tools/bob_realgl_flight.sh [outdir]
#
#   EVERY=N   dump every Nth 3D frame (default 600, i.e. ~10 s apart at 60 fps)
#   TMO=S     seconds of wall clock (default 600)
#
# It captures only; scoring is tools/bob_cloud_score.py, which reads the dumped sequence.
set -u
. "$(cd "$(dirname "$0")" && pwd)/bob_safe_kill.sh"
bob_snapshot_pids
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${1:-${OUT:-/tmp/bob_realgl_flight}}"
TMO="${TMO:-600}"
EVERY="${EVERY:-600}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
if pgrep -x bob >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: bob is already running (pid $(pgrep -x bob | tr '\n' ' '))." >&2; exit 2
fi

log="$OUT/flight.log"
echo "real-GL campaign flight — German Convoys, ${TMO}s, dumping every ${EVERY}th 3D frame"
# Identical to bob_combat_soak.sh's drive EXCEPT: no SDL_VIDEODRIVER=dummy, and a real DISPLAY.
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    DISPLAY="${DISPLAY:-:0}" \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
    BOB_AUTOCLICK="1,1,#1000:0,1,1" \
    BOB_MAP_ACCEPTDIR=40 BOB_CAMPAIGN_FLY=30 BOB_CAMPFLY_GO=1 BOB_MAP_TIMER=8 \
    BOB_TRACE_HUD=1 BOB_SHOT3D_EVERY="$EVERY" BOB_SHOT3D_PATH="$OUT/f" \
    "$BOB" ) >"$log" 2>&1
bob_kill_new

n=$(ls "$OUT"/f.*.ppm 2>/dev/null | wc -l)
echo "  3D frames dumped: $n"
# Prove the run reached the state it claims to test, BEFORE any measurement is believed.
# BOB_TRACE_HUD prints the numbers the HUD draws, so the run can SAY it got airborne.
nhud=$(grep -ac "^\[hud\]" "$log")
echo "  HUD samples: $nhud"
if [ "$nhud" -gt 0 ]; then
  echo "  first: $(grep -a '^\[hud\]' "$log" | head -1)"
  echo "  peak : $(grep -a '^\[hud\]' "$log" | sed 's/.*alt=\([0-9-]*\).*/\1 &/' | sort -rn | head -1 | cut -d' ' -f2-)"
  echo "  last : $(grep -a '^\[hud\]' "$log" | tail -1)"
else
  echo "  NO HUD SAMPLES — the flight never reached the 3D overlay, so nothing below is meaningful"
fi
[ "$n" -gt 0 ] || { echo "  NO FRAMES — nothing to score (see $log)"; exit 1; }
