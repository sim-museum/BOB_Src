#!/usr/bin/env bash
# tools/bob_r1_continuous.sh — EPIC R / R1: ONE process, UI -> preference -> flight -> recording.
#
# !! THIS GATE DOES NOT PASS YET, AND THAT IS ITS CURRENT PURPOSE. It encodes what R1 requires and
# fails at a known, named point. It is deliberately NOT in tools/bob_gates.sh -- a red gate in the
# suite trains people to ignore the suite. Remove this banner and add it there when it goes green.
#
# WHERE IT STOPS (S313): assertion 1 PASSES -- the UI writes GD_GUNCAMERAATSTART (val=2) inside the
# combined process, which is new; that half had only ever been measured in a run of its own.
# Assertion 2 fails: the flight never launches, because the two halves use boot modes that do not
# compose. See "WHY THE TWO HALVES COULD NOT SIMPLY BE CONCATENATED" below, and:
#
#   BOB_STARTFLYING=click expects BOB_AUTOCLICK to be EXACTLY the navigation to Fly -- it prints
#   "navigate to Fly by clicks (use BOB_AUTOCLICK=0,1,2)" -- and it runs its pre-flight on a
#   hard-coded timer (`if (++sf_t < 30) return;`, FULLPSYS.CPP) regardless of where the menus are.
#   Prepending the Sim Config trip therefore changes which screen Continue returns to: artnum 27917
#   with the harness, 28937 without it. Menu indices are per-screen, so the trailing "0" that means
#   Quick Shots on 28937 means the STRATEGIC MAP on 27917.
#
# NEXT STEP: give that pre-flight a delay knob (the 30 is hard-coded) so the preference trip can
# finish before the harness advances screens, then hand it the 0,1,2 it asks for.
#
# WHY THIS EXISTS
# ---------------
# The recording chain has been proven in two halves and joined by inference:
#   bob_settings_nav   the Sim Config combo writes GD_GUNCAMERAATSTART   (UI -> preference)
#   bob_replay_record  an armed flight yields a replay.dat                (armed -> recording)
# STATUS has said for many sprints that "a single continuous UI->recording session has NOT been run,
# and the join is an inference, however short". This runs it.
#
# WHY THE TWO HALVES COULD NOT SIMPLY BE CONCATENATED: they use different boot modes.
# bob_replay_record uses BOB_BOOT_FRONTEND=1, which SKIPS the menus entirely and stands up a Quick
# Mission directly, so it can never pass through Sim Config. This uses BOB_FRONTEND (the real menus)
# plus BOB_STARTFLYING=click, which reaches Fly by actual menu clicks.
#
# ⚠️ NEITHER BOB_GUNCAM NOR ANY OTHER ARMING HOOK IS SET. The recorder must be armed by the game's
# own read of the preference the UI wrote (STUB3D.CPP: `if (Save_Data.gamedifficulty
# [GD_GUNCAMERAATSTART] ...) _Replay.Record=TRUE;`). That is the whole point: BOB_GUNCAM=1 tests the
# recorder and BOB_GUNCAM=pref tests the reader, and neither tests the UI's effect on either.
#
# ⚠️ NEGATIVE CONTROL: CONTROL=1 drops the two #1075 clicks, so the gun camera is never switched on.
# The flight must still launch and NO recording may appear. Without that, a gate that records
# unconditionally (a stale replay.dat, a default-on preference) would pass while proving nothing.
#
# ⚠️ BOB_STARTFLYING=click is still a scaffold: it feeds the two Start3d paint bits our compat layer
# never dispatches (no WM_PAINT). It does not touch the preference, the recorder, or the menus.
#
# ⚠️ Needs real GL. Run under gl-lock.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_r1}"; mkdir -p "$OUT"
TMO="${TMO:-300}"
MINBYTES="${MINBYTES:-2000}"
CONTROL="${CONTROL:-0}"
REC="$GD/VIDEOS/replay.dat"     # uppercase VIDEOS (S260)

# Sim Config -> Views -> cycle Gun Camera twice (-> at-start) -> Continue.
# NO trailing menu click to reach Fly: BOB_STARTFLYING=click drives that itself. The first version
# appended "0" for Quick Shots and it opened the STRATEGIC MAP instead, then crashed in
# CMapDlg::InvalidateAnotherItem -> Persons2::ConvertPtrUID. The reason is that the harness advances
# screens on its own tick, so after Continue the front end was NOT on the main menu (artnum 27917,
# not 28937) and index 0 meant something else. Menu indices are only meaningful per screen, and two
# things driving the same menus do not compose by concatenating their click lists.
# Sim Config -> Views -> cycle Gun Camera twice -> Continue, THEN the harness's own fly navigation
# (it prints "use BOB_AUTOCLICK=0,1,2"). BOB_STARTFLYING_DELAY pushes the pre-flight past the
# preference trip so the two do not interleave -- see S314 in FULLPSYS.CPP.
# S316: was "...,6,0,1,2" -- try indices until one is Fly. BOB_DUMP_MENU=1 enumerates screen 27917
# as exactly three items -- 0 "Back", 1 "Sim Config", 2 "Fly" -- so index 0 LEAVES the screen on the
# first click (its onselect ran LaunchMap, which is how S314 met the S315 segfault) and 1/2 then
# address whatever Back navigated to. Fly is index 2; click it directly.
# S317: and the ROUTE was wrong too, not just the tail. BOB_DUMP_MENU=1 maps the screen graph:
#   28937 main : 0 Quick Shots -> 27923 ... 6 Sim Config -> 27911
#   27923      : 0 Back        1 Fly -> 27917
#   27917      : 0 Back        1 Sim Config -> 27911      2 Fly -> flight
# Going main -> Sim Config -> Continue LANDS on 27917 without ever passing through 27923, so no
# quick mission is ever selected and Fly enters 3D with no player aircraft:
#   *** FATAL: Persons3.cpp:3384 "No player A/C set up on entering 3d!"
# Sim Config is a DETOUR off the briefing screen, not the route to it. Select the mission first,
# then detour through Sim Config, then Fly -- which still exercises the Sim Config UI this gate
# exists to test. (Seeding currquickmiss via the forced pre-flight arm would make the gate pass
# while testing nothing about the menus.)
# S351: THREE cycles, not two. The gun-camera combo STARTS at index 2 (S331-S4 observed
# `[combo] SetIndex id=1075 <- 2`), and this gate needs it to END at 2 -- that is the
# GD_GUNCAMERAATSTART bit which arms the recorder, so unlike R13's gate the value here is
# SEMANTIC, not just an arithmetic result. Two clicks run 2 -> 0 -> 1 and land on ON-TRIGGER,
# which arms nothing, so every downstream assertion in this gate was failing for a reason that
# had nothing to do with recording. Three clicks return it to 2.
# The start index is verified below rather than assumed; if it ever changes, this recipe cannot
# reach 2 and the gate says so instead of reporting a recording failure it did not test.
CLICKS="${CLICKS:-0,1,1,3,#1075,#1075,#1075,6,2}"
[ "$CONTROL" = 1 ] && CLICKS="0,1,1,3,6,2"

[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
pgrep -x bob >/dev/null && { echo "  REFUSING: bob already running (pid $(pgrep -x bob|tr '\n' ' '))"; exit 2; }

echo "bob R1 continuous — one process: Sim Config -> Gun Camera -> Fly -> recording"
[ "$CONTROL" = 1 ] && echo "   [NEGATIVE CONTROL: gun camera never switched on; the flight must still fly and record NOTHING]"
rm -f "$REC"
log="$OUT/run.log"
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="${BOB_DRIVE_C:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c}" \
    BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_STARTFLYING=click BOB_AUTOCLICK="$CLICKS" \
    BOB_TRACE_SETFIELD=1 BOB_TRACE_RECLOG=1 BOB_TRACE_COMBO=1 \
    "$BOB" ) >"$log" 2>&1
pkill -x bob 2>/dev/null

fail=0
say(){ printf '  %-46s %s\n' "$1" "$2"; }
bad(){ say "$1" "$2"; fail=1; }

# 1. the UI wrote the preference (skipped in the control, which never clicks the combo)
if [ "$CONTROL" != 1 ]; then
  v=$(grep -a '\[setfield\] combo id=1075' "$log" | tail -1 | sed 's/.*val=//')
  # S351: this gate needs the AT-START value (2), because that is the bit that arms the recorder.
  # But 2 is only reachable if the combo starts where we think it does, so check the premise and
  # report a broken premise AS one -- not as a recording failure the gate never got far enough to
  # test. Start index from CRCombo::SetIndex (the combo is set and never read; GetIndex fires only
  # at write-back, so it can only ever report the END state -- S331-S4).
  st=$(grep -a '\[combo\] SetIndex id=1075' "$log" | head -1 | sed -n 's/.*<- *\([0-9-]*\).*/\1/p')
  nclick=$(printf '%s' "$CLICKS" | grep -ao '#1075' | wc -l)
  if [ -n "${st:-}" ]; then
    exp=$(( ( st + nclick ) % 3 ))
    if [ "$exp" != "2" ]; then
      bad "recipe can reach the arming value" "start=$st + $nclick clicks mod 3 = $exp, not 2 — RECIPE, not a recording fault"
    fi
    if [ "${v:-}" = "$exp" ]; then say "UI wrote the combo" "start=$st +$nclick -> val=$v"
    else bad "UI wrote the combo" "start=$st +$nclick predicts $exp, got '${v:-none}'"; fi
  else
    say "UI wrote the combo" "start index not observed — cannot verify (need BOB_TRACE_COMBO)"
  fi
  if [ "${v:-}" = "2" ]; then say "UI wrote GD_GUNCAMERAATSTART" "val=2 (recorder armed)"
  else bad "UI wrote GD_GUNCAMERAATSTART" "got '${v:-none}' (want 2) — the rest is meaningless"; fi
fi

# 2. the same process reached flight
if grep -aq "View3d interactive\|MakeInteractive" "$log"; then say "flight launched in the same process" "yes"
else bad "flight launched in the same process" "NO"; fi

# 3. armed WITHOUT any arming hook
if grep -aq "StartRecordFlag branch TAKEN\|OpenRecordLog:" "$log"; then say "recorder armed by the preference" "yes"
else say "recorder armed by the preference" "no trace"; fi

# 4. a recording exists
sz=$(stat -c%s "$REC" 2>/dev/null || echo 0)
echo "----------------------------------------"
if [ "$CONTROL" = 1 ]; then
  if [ "${sz:-0}" -lt "$MINBYTES" ] && [ "$fail" -eq 0 ]; then
    echo "CONTROL OK: flew with the gun camera off and recorded nothing (${sz:-0} bytes)"; exit 0; fi
  echo "CONTROL FAILED: ${sz:-0} bytes recorded with the gun camera never switched on"; exit 1
fi
if [ "${sz:-0}" -ge "$MINBYTES" ]; then say "recording written" "$sz bytes"
else bad "recording written" "only ${sz:-0} bytes (need >= $MINBYTES)"; fi
[ "$fail" -eq 0 ] && { echo "PASS: UI -> preference -> flight -> recording, in one process"; exit 0; }
echo "FAIL — log in $log"; exit 1
