#!/usr/bin/env bash
# GATE R18: the periodic move timer must keep an ABSOLUTE schedule.
#
# The timer drives the move cycle, and the move cycle advances the game's own clock
# (timeofday += FRAMETIME per tick). The original loop slept the period in 5 ms usleep slices and
# then ran the callback, so each period cost the period PLUS the sleep overshoots PLUS the callback,
# and the error accumulated: measured 40.742 ms against a nominal 40, i.e. 1.86% slow -- about 11
# seconds lost per ten minutes of mission. R16-S5 found this drift downstream (the padlock
# extrapolation reached its 1000 ms clamp in ~50 s) and fixed the symptom; this fixes the cause.
#
# Two arms on the same flight. BOB_TIMER_SLICES=1 restores the old loop and MUST drift, or the
# fixed arm's 40.000 proves nothing about the defect.
set -u
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
BOB=/home/admin/bob/build/bob; OUT=/tmp/bob_r18; mkdir -p "$OUT"; SECS=${SECS:-90}
arm() {  # $1 = tag, $2 = extra env
  ( cd "$GD" && timeout -k 5 -s KILL "$SECS" env BOB_RUN_INIT=1 BOB_DRIVE_C="$BOB_DRIVE_C" \
      BOB_BOOT_FRONTEND=1 BOB_TRACE_TIMER=1 $2 "$BOB" ) > "$OUT/$1.log" 2>&1
  local last; last=$(grep -a "^\[timer\] id=1 " "$OUT/$1.log" | tail -1)
  [ -z "$last" ] && { echo "  INCONCLUSIVE: the $1 arm produced no timer report -- it never ran the move cycle"; exit 2; }
  PERIOD=$(echo "$last" | sed -n 's/.*-> \([0-9.]*\) ms\/period.*/\1/p')
  FIRED=$(echo  "$last" | sed -n 's/.*fired=\([0-9]*\) .*/\1/p')
  echo "  $1: $FIRED periods, $PERIOD ms/period (nominal 40)"
}
echo "R18 -- move-timer schedule, ${SECS}s per arm"
arm legacy "BOB_TIMER_SLICES=1"; LEG=$PERIOD
arm fixed  "";                   FIX=$PERIOD
echo "----------------------------------------"
# awk for the float comparisons: the shell cannot do them, and rounding to integers would hide
# exactly the 1.9% this gate exists to catch.
awk -v l="$LEG" -v f="$FIX" 'BEGIN{
  if (l < 40.25) { print "FAIL/RETHINK: the control did NOT drift ("l" ms) -- the premise is wrong"; exit 1 }
  if (f > 40.10 || f < 39.90) { print "FAIL: the fixed arm is "f" ms/period, off the 40 ms nominal"; exit 1 }
  printf "PASS: control drifts to %s ms/period (%.2f%% slow); the absolute deadline holds %s\n", l, (l-40)/40*100, f
}'
