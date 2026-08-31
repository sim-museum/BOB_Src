#!/usr/bin/env bash
# GATE R16-S5: the padlock extrapolation must not sawtooth.
#
# view_dt is the time since the last world move; ViewFudge extrapolates the padlocked item by
# vel*view_dt so it is drawn where it will be between 25 Hz moves. move_time_ms was stamped once
# (ThreeDee ctor) and never again, so view_dt ramped 0 -> 400 ms and snapped to 0 at 2.5 Hz -- the
# bogie drawn tens of metres ahead and yanked back, which is the PO's "about 5 fps near a padlocked
# bogie during rapid motion". DoMoveCycle now stamps it every move.
#
# Two arms on the SAME flight recipe. The negative control (BOB_NO_MOVESTAMP=1) must show the
# sawtooth; the fix must not. And the recipe must PROVE it padlocked (the tap line), because a
# synthetic keypress that never fires looks exactly like one the game ignored.
set -u
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
BOB=/home/admin/bob/build/bob; OUT=/tmp/bob_r16; mkdir -p "$OUT"; SECS=${SECS:-150}
arm() { # $1 tag, $2 extra env
  ( cd "$GD" && timeout -k 5 -s KILL "$SECS" env BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
      BOB_BOOT_FRONTEND=1 BOB_AUTOFLY=padlock BOB_TRACE_VIEWDT=1 $2 "$BOB" ) > "$OUT/$1.log" 2>&1
  local last; last=$(grep -a "^\[viewdt\] n=" "$OUT/$1.log" | tail -1)
  local tap; tap=$(grep -ac "PADLOCKTOG tapped" "$OUT/$1.log")
  echo "  $1: padlock tapped=$tap"; echo "  $1: ${last:-<no viewdt report>}"
  [ -z "$last" ] && { echo "  INCONCLUSIVE: no view_dt samples in the $1 arm -- the flight never rendered"; exit 2; }
  [ "$tap" -ge 1 ] || { echo "  INCONCLUSIVE: PADLOCKTOG never fired in the $1 arm"; exit 2; }
  MAX=$(echo "$last" | sed -n 's/.*max=\([0-9]*\).*/\1/p'); RESETS=$(echo "$last" | sed -n 's/.*resets=\([0-9]*\).*/\1/p')
  N=$(echo "$last" | sed -n 's/.*n=\([0-9]*\).*/\1/p')
}
echo "R16-S5 -- view_dt sawtooth, ${SECS}s per arm"
arm off "BOB_NO_MOVESTAMP=1"; OFFMAX=$MAX; OFFRES=$RESETS
arm on  ""; ONMAX=$MAX; ONRES=$RESETS; ONN=$N
echo "----------------------------------------"
if [ "${OFFMAX:-0}" -lt 300 ] || [ "${OFFRES:-0}" -lt 1 ]; then
  echo "FAIL/RETHINK: the control did NOT sawtooth (max=$OFFMAX resets=$OFFRES) -- the premise is wrong"; exit 1; fi
if [ "${ONMAX:-999}" -ge 60 ]; then
  echo "FAIL: with the stamp in, view_dt still reaches ${ONMAX} ms (one move period is 40)"; exit 1; fi
# Negative deltas mean the stamp is running AHEAD of the draw clock -- two writers fighting (S5
# measured 41% of frames that way with a stamp beside the counter). Allow only clock jitter.
if [ $(( ONRES * 100 )) -gt $(( ONN * 2 )) ]; then
  echo "FAIL: ${ONRES} negative/clamp resets in ${ONN} frames -- move_time_ms has two writers"; exit 1; fi
echo "PASS: control drifts to ${OFFMAX} ms (${OFFRES} snaps); with the clock stamp view_dt peaks at ${ONMAX} ms, resets ${ONRES}/${ONN}"
