#!/usr/bin/env bash
# tools/bob_strategic_soak.sh -- GATE 7: the campaign's STRATEGIC layer actually runs a raid.
#
# WHY THIS EXISTS (S206)
# ----------------------
# S201-S205 built an increasingly confident story that this port's AI never fights:
#   S201  AUTO_COMBAT movecode ticks = 0 over 1.46M dispatches
#   S202  ArtInt::SetEngage never called
#   S203  the raid is never detected            (withdrawn -- measured on RAF patrols)
#   S204  THE LW RAID NEVER EXECUTES A WAYPOINT (withdrawn HERE -- see below)
#
# S204's finding was an artifact of RUN LENGTH, and the mechanism is the campaign clock:
#
#   * on the strategic map, BOB_MAP_TIMER=N drives N clock ticks per PAINT, and the sim pins
#     the band to ACCEL_RAIDSPD once a raid is live -- game time runs far faster than wall time;
#   * in 3D flight the map tick does not run at all (it is guarded on !InThe3D), so the strategic
#     sim advances at roughly REAL time.
#
# Every soak set BOB_CAMPAIGN_FLY, so it spent its wall clock inside the flight. Measured: the
# lead Ju87 squadron gets ~10 SAG frames per 420 s soak and needs ~21 to reach its Bomb
# Rendezvous -- it was two thirds of the way there when every previous run was killed. Nothing was
# stuck. "It never happens" was "the tape ran out", which looks identical from the log.
#
# Given the game time it needs, the strategic layer runs end to end:
#   pack 6 executes BombRendezvous -> DogLeg -> IP -> target -> egress, PS_FORMING(14) -> 15 -> 16
#   -> PS_ENEMYSIGHTED(18) / PS_OUTGOING(19); RAFDirectivesResults::SetRAFIntercept is called for
#   the first time in this port's life; and RAF AM_INTERCEPT packages (attackmethod=1) appear in
#   the census, which had only ever held AM_PATROL and the raid itself.
#
# WHAT IT ASSERTS
#   1. the sim soaked                 -- MoveAllSAGs dispatches, so a dead run cannot pass
#   2. the LW raid executes waypoints -- the exact property S204 measured as zero
#   3. the raid progresses past PS_FORMING(14)
#   4. the RAF is tasked              -- SetRAFIntercept reached
#   5. an AM_INTERCEPT package exists -- the RAF acted on the tasking
#   6. no crash over the soak
#
# WHAT IT DELIBERATELY DOES NOT ASSERT
#   Aircraft-level AUTO_COMBAT/AirCombat. Those codes belong to EXPANDED SAGs near the player, and
#   a map-only run has no player aircraft, so zero is correct here and is not evidence about the
#   ACM tree either way. GATE 6 owns that question; reaching it needs a flight timed to the
#   interception, which is the next sprint's work. Do not "strengthen" this gate with a movecode
#   assertion -- it would assert a thing this recipe cannot produce.
#
# NEGATIVE CONTROL: STARVE=1 re-runs S204'S OWN RECIPE -- the same campaign drive plus
# BOB_CAMPAIGN_FLY, so the run spends its wall clock in the 3D flight where the strategic tick
# barely advances. Assertions 2-5 must go RED, reproducing S204's zero on today's binary. That is
# the point: the difference between the two arms is the RECIPE, not the code.
#
# A first attempt at this control was wrong and is recorded rather than deleted: it starved the
# run by dropping BOB_MAP_TIMER from 8 to 1 -- and the gate PASSED anyway (39 waypoint executions
# in 90 s). The clock is driven once per map PAINT, and paints are frequent enough that the
# multiplier hardly matters. Being ON THE MAP is what matters. The control caught an overspecific
# mechanism in this very header before it was published.
set -u
# S415: this gate is DUMMY-VIDEO, so it never needed exclusive use of anything -- it inherited that
# from sharing the PLAYER'S game directory and from killing bob by name. Both are fixed here, and
# the blanket refusal below is dropped, because refusing to run for as long as the PO plays is how a
# headless gate goes unrun for a whole day (S405 found four in that state).
. "$(cd "$(dirname "$0")" && pwd)/bob_safe_kill.sh"    # kill ONLY what this script started
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"  # run against a scratch tree, not the player's
bob_snapshot_pids
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_strategic_soak}"
STARVE="${STARVE:-0}"
if [ "$STARVE" = "1" ]; then TMO="${TMO:-420}"; FF="${FF:-8}"; MIN_DISP="${MIN_DISP:-0}"
                            FLY="BOB_CAMPAIGN_FLY=30 BOB_CAMPFLY_GO=1"
else                         TMO="${TMO:-300}"; FF="${FF:-8}"; MIN_DISP="${MIN_DISP:-3000}"
                            FLY=""; fi
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }


log="$OUT/strategic.log"
echo "strategic soak -- German Convoys campaign, ${TMO}s at BOB_MAP_TIMER=$FF$([ "$STARVE" = 1 ] && echo '  [NEGATIVE CONTROL: S204 recipe, flight takes the wall clock]')"
# NB: no BOB_CAMPAIGN_FLY. Staying on the map is the whole point -- it is what gives the raid the
# game time it needs. Headless (SDL dummy), so this takes no display lock.
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
    BOB_AUTOCLICK="1,1,#1000:0,1,1" \
    BOB_MAP_ACCEPTDIR=40 BOB_MAP_TIMER="$FF" $FLY \
    BOB_TRACE_SAGWP=1 BOB_TRACE_DETECT=1 \
    BOB_SHOT=99999 BOB_SHOT_PATH="$OUT/soak.ppm" "$BOB" ) >"$log" 2>&1
# S415: was `for p in $(pgrep -x bob); do kill -9 "$p"` -- that kills the PLAYER'S session too,
# the exact hazard S392 wrote bob_kill_new for. The blanket refusal at the top of this file was the
# only thing standing between this line and the PO's game; removing the refusal without fixing the
# kill would have been strictly worse than leaving both.
bob_kill_new

fail=0
red() { echo "  $1 -- FAIL"; fail=1; }

# 1. did the sim actually run?
disp=$(grep -a "^\[detect\] dispatches=" "$log" | tail -1 | sed -n 's/.*dispatches=\([0-9]*\).*/\1/p')
echo "  1. MoveAllSAGs dispatches: ${disp:-0} (need >= $MIN_DISP)"
[ "${disp:-0}" -ge "$MIN_DISP" ] || red "the sim did not soak, so nothing below is meaningful"

# 2. the property S204 measured as ZERO
lwwp=$(grep -a "\[sagwp\] execute" "$log" | grep -ac "attackmethod=1[0-9]")
echo "  2. LW-raid waypoint executions: $lwwp (need >= 5; S204 measured 0)"
[ "$lwwp" -ge 5 ] || red "the LW raid executed no waypoints"

# 3. did the raid get past PS_FORMING?
maxst=$(grep -a "\[sagwp\] execute" "$log" | grep -a "attackmethod=1[0-9]" \
        | sed -n 's/.* status=\([0-9]*\) .*/\1/p' | sort -n | tail -1)
echo "  3. highest LW squadron status: ${maxst:-0} (need >= 15 = PS_INCOMING; PS_FORMING is 14)"
[ "${maxst:-0}" -ge 15 ] || red "the raid never left PS_FORMING"

# 4. is the RAF ever tasked?
if grep -aq "\[intercept\] FIRST CALL" "$log"; then
  echo "  4. SetRAFIntercept reached: yes -- $(grep -a '\[intercept\] FIRST CALL' "$log" | head -1 | sed 's/.*FIRST CALL //')"
else
  echo "  4. SetRAFIntercept reached: NO"; red "the RAF is never tasked to intercept"
fi

# 5. did an interceptor package actually appear?
icept=$(grep -a "^\[detect\]   pack=" "$log" | grep -ac "attackmethod=1  ")
echo "  5. AM_INTERCEPT package sightings in the census: $icept (need >= 1)"
[ "$icept" -ge 1 ] || red "no RAF interceptor package was ever created"

# 6. the safety property
if grep -aq "=== CRASH: signal" "$log"; then
  echo "  6. $(grep -a '=== CRASH: signal' "$log" | head -1 | sed 's/^=== /CRASHED: /;s/ ===$//')"
  grep -a "bob() \[0x" "$log" | head -6 | sed 's/.*\[\(0x[0-9a-f]*\)\].*/\1/' | while read -r a; do
    echo "        $a  $(addr2line -f -C -e "$BOB" "$a" 2>/dev/null | head -1)"
  done
  red "crashed during the soak"
else
  echo "  6. no crash over the soak: yes"
fi

echo "----------------------------------------"
if [ "$STARVE" = "1" ]; then
  # inverted: the control is healthy when the gate goes red
  if [ "$fail" -ne 0 ]; then echo "CONTROL OK: starved of game time, the gate fails (this is what S204 measured)"; exit 0
  else echo "CONTROL BROKEN: the gate passed even when starved -- it is not testing what it claims"; exit 1; fi
fi
if [ "$fail" -eq 0 ]; then
  echo "PASS: the raid flew its route ($lwwp waypoint executions, status $maxst), the RAF was tasked, $icept interceptor sighting(s)"
else
  echo "FAIL"; exit 1
fi
