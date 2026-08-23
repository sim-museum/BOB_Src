#!/usr/bin/env bash
# tools/bob_combat_soak.sh — the soak gate the dogfight crash exposed (S200/S201).
#
# WHY THIS EXISTS
# ---------------
# A player's BoB exited during a dogfight. The cause was a one-past-the-end read of the cloud
# layers in ACMAirStruc::DefenceManoeuvre (S200, ACMMAN.CPP: `for(i = 0; i <= 3)` over
# `Cloud Layer[3]`). The full gate suite passed WITH AND WITHOUT that fix, because nothing in it
# exercises air combat: GATE 5 drives the German Convoys campaign to a Bf 109 cockpit and stops.
# The suite's coverage ended at the cockpit door.
#
# WHAT IT ASSERTS
#   1. the sim actually SOAKED        — a large number of movecode dispatches. Without this the
#                                       gate would "pass" on a run that never got airborne, which
#                                       is the S171 failure mode (asserting on a prefix of a log).
#   2. no crash banner                — the safety property. This is the one that would have caught
#                                       S200 had combat been reached.
#
# WHAT IT REPORTS BUT DOES NOT ASSERT
#   3. combat activity — AUTO_COMBAT movecode ticks, AirCombat() entries, DefenceManoeuvre calls.
#
# ⚠️ ALL THREE ARE CURRENTLY **ZERO**, and that is the finding, not a gate bug. Measured over
# 1,460,001 movecode dispatches of the campaign raid: movecodes 0/1/5/39 are in play and
# AUTO_COMBAT (=AutoMoveCodeMask=63) never appears once.
#
# S204 traced why, and the answer is upstream of everything this gate can see: THE LW RAID NEVER
# EXECUTES A WAYPOINT. Its seven squadrons take off, reach PS_FORMING together, and stay there --
# so they never reach the Bomb/Esc Rendezvous, never climb past ~4,177 ft, never enter the RAF
# radar's 4,000-19,000 ft height bands, are never detected, and no interceptor is ever tasked.
# GroundVisible returned UID_NULL 700 times out of 700 for exactly that reason. The radar grid and
# the detection code are both fine. See PORT.md 2026-08-23 (S204) and tools/bob_detect_probe.sh.
#
# NB the line above used to say the aircraft "fly their waypoints and never engage". They do not
# fly their waypoints; that was an assumption, and it was wrong. Two earlier attempts to explain
# this were also wrong and are recorded rather than deleted: BOB_TRACE_SEENAC is an S192 guard
# about the player's seen-aircraft, not a detection counter; and S203's "the raid is never
# detected" was measured on samples that were all RAF PATROL packages, never the raid.
#
# So this gate does NOT fail on zero combat: a gate that always fails is noise, and asserting a
# property the port has never had would be asserting a wish. It fails on a CRASH or a DEAD SIM, and
# it prints the combat numbers every run so the gap cannot quietly persist. When combat starts
# working, turn assertion 3 on -- the numbers are already here.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_combat_soak}"
TMO="${TMO:-600}"
MIN_DISPATCHES="${MIN_DISPATCHES:-200000}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
if pgrep -x bob >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: bob is already running (pid $(pgrep -x bob | tr '\n' ' '))."
  echo "  A stray process would fight this one for the run directory (cf. MA's S177)."
  exit 2
fi

log="$OUT/soak.log"
echo "combat soak — German Convoys campaign, ${TMO}s"
# Same drive as GATE 5's campaign run: it is the only path that reliably gets airborne with both
# sides present. Run UNDER gl-lock; this does not take the lock itself.
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
    BOB_AUTOCLICK="1,1,#1000:0,1,1" \
    BOB_MAP_ACCEPTDIR=40 BOB_CAMPAIGN_FLY=30 BOB_CAMPFLY_GO=1 BOB_MAP_TIMER=8 \
    BOB_TRACE_MOVECODE=1 BOB_TRACE_ACM=1 \
    BOB_SHOT=99999 BOB_SHOT_PATH="$OUT/soak.ppm" "$BOB" ) >"$log" 2>&1
pkill -x bob 2>/dev/null

fail=0

# 1. did the sim actually run?
disp=$(grep -a "\[movecode\] after" "$log" | tail -1 | sed -n 's/.*after \([0-9]*\) dispatches.*/\1/p')
echo "  movecode dispatches: ${disp:-0}"
if [ "${disp:-0}" -ge "$MIN_DISPATCHES" ]; then
  echo "  the sim soaked (>= $MIN_DISPATCHES dispatches): yes"
else
  echo "  only ${disp:-0} dispatches — the sim did not soak, so nothing below is meaningful — FAIL"
  fail=1
fi

# 2. the safety property
if grep -aq "=== CRASH: signal" "$log"; then
  echo "  $(grep -a '=== CRASH: signal' "$log" | head -1 | sed 's/^=== /CRASHED: /;s/ ===$//') — FAIL"
  grep -a "bob() \[0x" "$log" | head -6 | sed 's/.*\[\(0x[0-9a-f]*\)\].*/\1/' | while read -r a; do
    echo "      $a  $(addr2line -f -C -e "$BOB" "$a" 2>/dev/null | head -1)"
  done
  fail=1
else
  echo "  no crash over the soak: yes"
fi

# 3. reported, not asserted
hist=$(grep -a "\[movecode\] after" "$log" | tail -1 | sed 's/.*dispatches://;s/(.*//')
combat=$(grep -a "AirCombat ticks" "$log" | tail -1 | sed -n 's/.*ticks=\([0-9]*\).*/\1/p')
defence=$(grep -ac "DefenceManoeuvre calls" "$log")
echo "  movecodes seen:${hist:- none}"
echo "  AirCombat ticks: ${combat:-0}   DefenceManoeuvre samples: ${defence:-0}"
if [ "${combat:-0}" -gt 0 ]; then
  echo "  COMBAT REACHED — turn assertion 3 on and make this a real combat gate"
else
  echo "  ⚠ COMBAT NOT REACHED: no aircraft entered AUTO_COMBAT (63) in this run."
  echo "    The ACM decision tree is still untested code. This is a known gap (S201), not a"
  echo "    gate failure — see the header. Do not delete this line to make the output tidy."
fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: the sim soaked ${disp:-0} dispatches with no crash (combat: ${combat:-0} ticks)"
else
  echo "FAIL"; exit 1
fi
