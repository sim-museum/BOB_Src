#!/usr/bin/env bash
# tools/bob_replay_record.sh — EPIC R / R1: a flown mission produces a recording.
#
# WHY THIS EXISTS
# ---------------
# S258 reported "BoB never records". S260 showed the recorder WORKS when armed -- and that S258's
# evidence had been invalid (it read a trace gated on BOB_TRACE_RECLOG, which was never set, so
# "no [reclog]" meant "the trace was off"). The conclusion survived by luck. This gate exists so
# the question is never re-answered by reading a log with the wrong flags again.
#
# WHAT IT ASSERTS
#   1. the flight actually launched   -- else everything below is vacuous
#   2. the recorder was armed         -- the StartRecordFlag branch was taken
#   3. OpenRecordLog succeeded        -- handle=ok, with the file it opened named
#   4. a recording exists and is substantial
#
# ⚠️ NEGATIVE CONTROL: CONTROL=1 drops BOB_GUNCAM, so nothing arms. Assertions 2-4 MUST go red.
# That control is the whole point here: the DEFAULT behaviour is "no recording", so a gate that
# only ever runs armed cannot tell "recording works" from "my assertions never fire".
#
# ⚠️ BOB_GUNCAM=1 is a TEST HOOK, not the shipping path. It arms _Replay.StartRecordFlag directly,
# exercising the real recorder while SKIPPING the gun-camera preference that decides whether to arm.
# A pass means "the recorder works", NOT "the preference works" -- those are separate claims and the
# second is still untested (R1 residual).
#
# ⚠️ Needs real GL. Run under gl-lock.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"   # S373: default to a SCRATCH tree, never the player's
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_replay_record}"
TMO="${TMO:-120}"
MINBYTES="${MINBYTES:-2000}"
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
if pgrep -x bob >/dev/null 2>&1; then
  echo "  REFUSING TO RUN: bob is already running (pid $(pgrep -x bob | tr '\n' ' '))."; exit 2
fi
fail=0
say(){ printf '  %-46s %s\n' "$1" "$2"; }
bad(){ say "$1" "$2"; fail=1; }

REC="$GD/VIDEOS/replay.dat"     # NB: uppercase VIDEOS -- S260 lost two searches to assuming Videos
echo "bob replay record — fly, arm the gun camera, expect a recording"
[ "$CONTROL" = 1 ] && echo "   [NEGATIVE CONTROL: no BOB_GUNCAM, nothing arms; assertions 2-4 MUST go RED]"
rm -f "$REC"
log="$OUT/fly.log"
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="${BOB_DRIVE_C:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c}" \
    BOB_BOOT_FRONTEND=1 BOB_TRACE_RECLOG=1 \
    $([ "$CONTROL" = 1 ] || echo BOB_GUNCAM=1) \
    "$BOB" ) >"$log" 2>&1
pkill -x bob 2>/dev/null

# 1. reach
if grep -aq "View3d interactive" "$log"; then say "flight launched" "yes"
else bad "flight launched" "NO — nothing below is meaningful"; fi

# 2. armed
if grep -aq "StartRecordFlag branch TAKEN" "$log"; then say "recorder armed" "yes"
else bad "recorder armed" "NO"; fi

# 3. the log opened
if grep -aq "OpenRecordLog:.*handle=ok" "$log"; then
  say "OpenRecordLog succeeded" "$(grep -a 'OpenRecordLog:' "$log" | head -1 | sed "s/.*file='\([^']*\)'.*/\1/")"
else bad "OpenRecordLog succeeded" "NO"; fi

# 4. a real recording
sz=$(stat -c%s "$REC" 2>/dev/null || echo 0)
if [ "${sz:-0}" -ge "$MINBYTES" ]; then say "recording written" "$sz bytes"
else bad "recording written" "only ${sz:-0} bytes (need >= $MINBYTES)"; fi

echo "----------------------------------------"
if [ "$CONTROL" = 1 ]; then
  if [ "$fail" -ne 0 ]; then echo "CONTROL OK: unarmed, nothing recorded, and the assertions fired"; exit 0
  else echo "CONTROL FAILED: nothing was armed yet everything passed — the assertions are asleep"; exit 1; fi
fi
if [ "$fail" -eq 0 ]; then echo "PASS: BoB flew, armed the recorder, and wrote a $sz-byte recording"
else echo "FAIL"; exit 1; fi
