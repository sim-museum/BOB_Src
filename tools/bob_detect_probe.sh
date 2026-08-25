#!/usr/bin/env bash
# tools/bob_detect_probe.sh — S204: WHY is the raid never detected?
#
# ⚠️ S206: READ THIS FIRST. The premise below — "the raid is never detected", "the LW pack never
# executes a waypoint" — is a property of THIS PROBE'S RECIPE, not of the game. The recipe sets
# BOB_CAMPAIGN_FLY, and in 3D flight the strategic clock advances at roughly real time, so the raid
# is still ~1/3 short of its Bomb Rendezvous when the timeout fires. Run the same drive WITHOUT the
# flight (tools/bob_strategic_soak.sh, GATE 7) and the raid executes 37 waypoints, reaches
# PS_OUTGOING, is detected, and SetRAFIntercept is called. This probe is still useful for the
# census and the intercept-gate readout; just do not read its zeros as findings. See §8-BoB206.
#
# S203 established that no aircraft in this port ever fights, and that the manoeuvre code is fine
# and simply unreached: across all 39 waypoint executions of the combat soak the raid reported
# `detected=0` and no squadron ever carried `method=AM_INTERCEPT`. That located the dead link but
# did not explain it. This probe measures the two things that must be true for the RAF to be
# tasked, and it measures them rather than reasoning about them (§8-BoB203):
#
#   1. THE CENSUS — which packages actually exist, what attackmethod each carries, and therefore
#      which of the four detector classes each one is dispatched to. S203 sampled attackmethod=0
#      (= AM_RAF/AM_PATROL) in a run of the GERMAN campaign, which is either the answer or a sign
#      that the 39 samples were all RAF patrols and the LW raid was never sampled at all.
#
#   2. THE INTERCEPT GATE — RAFDirectivesResults::SetRAFIntercept is the ONLY route by which the
#      RAF is ever tasked to intercept, and its first statement is
#          if (afteregress == MMC.directives.raf.current.interceptbeforetarget) return;
#      Every caller on the detection path passes afteregress=false, so a zero `interceptbeforetarget`
#      makes the whole function a no-op silently. The probe reports the directive values and counts
#      how many calls leave by that door.
#
# This is a PROBE, not a gate: it asserts nothing and cannot go red. It answers a question.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_detect}"
TMO="${TMO:-700}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
if pgrep -x bob >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: bob is already running (pid $(pgrep -x bob | tr '\n' ' '))." >&2
  exit 2
fi

log="$OUT/detect.log"
echo "detection probe — German Convoys campaign, ${TMO}s"
# Same drive as GATE 5 / the combat soak: the only path that reliably gets airborne with both sides.
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
    BOB_AUTOCLICK="1,1,#1000:0,1,1" \
    BOB_MAP_ACCEPTDIR=40 BOB_CAMPAIGN_FLY=30 BOB_CAMPFLY_GO=1 BOB_MAP_TIMER=8 \
    BOB_TRACE_DETECT=1 BOB_TRACE_GROUNDVIS=1 BOB_TRACE_SAGWP=1 \
    BOB_SHOT=99999 BOB_SHOT_PATH="$OUT/detect.ppm" "$BOB" ) >"$log" 2>&1
pkill -x bob 2>/dev/null

echo "  ---- package census (FIRST and LAST — the picture changes) ----"
# The census re-dumps whenever the live set changes. Show the first and the last: a probe that
# printed only the first reported six PS_PLANNED packages, i.e. the state before anything had
# taken off, and read as the answer.
awk '/^\[detect\] CENSUS/{n++} n==1' "$log" | sed 's/^/  /'
echo "  ..."
awk '/^\[detect\] CENSUS/{last=NR} {l[NR]=$0} END{for(i=last;i<=NR;i++) if (l[i] ~ /^\[detect\]/) print l[i]}' "$log" | sed 's/^/  /'
echo "  (censuses recorded: $(grep -ac "^\[detect\] CENSUS" "$log"))"
grep -aq "^\[detect\] CENSUS" "$log" || echo "    (no census line — MoveAllSAGs never saw a live package)"
echo "  ---- detector branch histogram ----"
grep -a "^\[detect\] dispatches=" "$log" | tail -3 | sed 's/^/  /'
grep -aq "^\[detect\] dispatches=" "$log" || echo "    (no dispatches — the detection loop never ran)"
echo "  ---- the LW raid, squadron by squadron (first and last census) ----"
awk '/^\[detect\]     LWpack=/{print}' "$log" | head -8 | sed 's/^/  /'
echo "  ..."
awk '/^\[detect\]     LWpack=/{print}' "$log" | tail -8 | sed 's/^/  /'
grep -aq "^\[detect\]     LWpack=" "$log" || echo "    no LW package was ever live"
echo "  ---- LWDetectLW: the branch the LW RAID takes ----"
grep -a "^\[lwdetectlw\]" "$log" | tail -3 | sed 's/^/  /'
grep -aq "^\[lwdetectlw\]" "$log" || echo "    LWDetectLW never ran — no LW package was ever dispatched to it."
echo "  ---- GroundVisible: the RAF radar/observer network ----"
grep -a "^\[groundvis\]" "$log" | tail -4 | sed 's/^/  /'
grep -aq "^\[groundvis\]" "$log" || echo "    GroundVisible never ran."
echo "  ---- waypoint executions, by package attackmethod ----"
# PS_FORMING -> PS_INCOMING happens in SAGExecuteWaypoint on the Bomb/Esc Rendezvous waypoint.
# If the LW pack never executes a waypoint it can never leave PS_FORMING, never climb, and so can
# never enter a radar height band. Count executions per attackmethod: >=8 is an LW pack.
grep -a "^\[sagwp\] execute" "$log" | grep -o "attackmethod=[0-9]*" | sort | uniq -c | sed 's/^/    /'
echo "    total executions: $(grep -ac "^\[sagwp\] execute" "$log")"
echo "  ---- the intercept gate ----"
grep -a "^\[intercept\]" "$log" | head -8 | sed 's/^/  /'
grep -aq "^\[intercept\]" "$log" || echo "    SetRAFIntercept was NEVER CALLED in this run."
echo "  (full log: $log)"
