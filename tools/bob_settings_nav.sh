#!/usr/bin/env bash
# tools/bob_settings_nav.sh — reach the Sim Config screen headlessly and prove a setting is STORED.
#
# WHY THIS EXISTS
# ---------------
# S289 asked "does the Preferences UI write GD_GUNCAMERAATSTART?" and could not answer it: the
# combo instrument recorded ZERO calls, because no recipe here had ever opened that dialog. A zero
# from an instrument whose code path is never reached says nothing at all. This is that recipe.
#
# NAVIGATION (all indices measured with BOB_DUMP_HITTARGETS=1, never guessed from a screenshot):
#   main menu  0=Quick Shots 1=Campaigns 2=Multi-Player 3=Load Game 4=Replay
#              5=PC Config  6=Sim Config 7=Credits 8=Quit 9=Website
#   Sim Config tab bar   0=Flight 1=Game 2=Mission 3=Views 5=PC 6=Continue
#   Gun Camera lives on VIEWS (id 1075), not Game -- it is a view setting, not a difficulty one.
#
# ⚠️ WHAT THIS ASSERTS is that the combo's value reaches Save_Data, watched at the write-back
# (BOB_TRACE_SETFIELD) rather than read off the rendered text. The combo DRAWING "On" would prove
# only what was painted; ADDBIT maps val bit 1 -> GD_GUNCAMERAATSTART, so val=2 is the claim.
#
# ⚠️ WHAT IT DOES NOT ASSERT: that this run then records. That link is proven separately (S287:
# setting the preference yields 182137 bytes + 511 ACMI markers). The two halves have been measured
# in SEPARATE runs and joined at a plain variable both read -- stated because a single continuous
# UI->recording session has NOT been run, and the join is an inference, however short.
set -u
. "$(cd "$(dirname "$0")" && pwd)/bob_safe_kill.sh"   # S392: never kill a bob this gate did not start
bob_snapshot_pids
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
BOB="${BOB:-/home/admin/bob/build/bob}"
OUT="${OUT:-/tmp/bob_settings_nav}"; mkdir -p "$OUT"
CLICKS="${CLICKS:-6,3,#1075,#1075,6}"     # Sim Config -> Views -> cycle Gun Camera twice -> Continue
# R13 (S340): THE EXPECTED VALUE IS DERIVED, NOT HARDCODED.
# This gate was permanently red on WANT=2, and S331-S4 established why by observation:
# `[combo] SetIndex id=1075 <- 2` -- the gun-camera preference ALREADY reads at-start before the
# gate touches it, so two cycles run 2 -> 0 -> 1 and land on 1. WANT=2 silently assumed a start of
# 0 that is never true, and that assumption appeared nowhere in the recipe.
# Changing the constant to 1 would turn the gate green today and make it lie tomorrow, because it
# would still encode an unstated premise about the start state. What this gate actually claims --
# see the header -- is that the combo's value reaches Save_Data. That is testable with no absolute
# at all: the stored value must equal the combo's own arithmetic, (start + clicks) mod N, with both
# endpoints read from the log. NITEMS=3 is from the declaration, not from tuning:
#   SVIEWER.CPP:92  SETFIELD(..., IDC_CBO_GUNCAMERAONATSTART, RESCOMBO(CAMERAOFF,3), NOLEVEL)
NITEMS="${NITEMS:-3}"                      # RESCOMBO(CAMERAOFF,3): off / on-trigger / at-start
# S405: refuse only when this gate would touch the PLAYER'S game directory.
#
# The guard used to be a blanket `pgrep -x bob` refusal, which made this gate -- and three other
# HEADLESS ones -- unrunnable for as long as a player session existed. That cost real time: with the
# PO playing for four hours, the front-end gates that would have caught MA's ETO_CLIPPED regression
# (parity_2d's twin here) could not run, and the guard was not the display but this line.
#
# The refusal was right when it was written: these gates drove the real drive_c. S373 then built
# bob_use_scratch.sh, which gives a writing gate its own tree, and S392 made cleanup kill only what
# the gate itself started. With both, a dummy-video run against a scratch tree cannot disturb a
# player session -- so refuse on the actual hazard (sharing the player's data), not on the mere
# existence of another process.
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"   # sets BOB_DRIVE_C + GD to a scratch tree
case "${BOB_DRIVE_C:-}" in
    */bob_scratch_*) ;;                                  # isolated: safe to run alongside a session
    *) pgrep -x bob >/dev/null && { echo "REFUSING: bob already running and this run would use the player's game directory"; exit 2; } ;;
esac
( cd "$GD" && timeout -k 5 -s KILL "${TMO:-300}" env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="${BOB_DRIVE_C:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c}" \
    BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
    BOB_AUTOCLICK="$CLICKS" BOB_TRACE_SETFIELD=1 BOB_TRACE_COMBO=1 \
    "$BOB" ) >"$OUT/nav.log" 2>&1
bob_kill_new
fail=0
n=$(grep -ac '\[setfield\]' "$OUT/nav.log")
echo "  settings write-backs seen: $n"
[ "$n" -gt 0 ] || { echo "  the settings screen was never committed — nothing below is meaningful — FAIL"; exit 1; }
got=$(grep -a 'setfield.*id=1075' "$OUT/nav.log" | tail -1 | sed -n 's/.*val=\([0-9-]*\).*/\1/p')
# start index: the combo is SET and never read, so the only place the start state appears is
# CRCombo::SetIndex (S331-S4 -- tracing GetIndex could never answer it, because GetIndex fires
# once at the write-back and so only ever reports the END state).
start=$(grep -a '\[combo\] SetIndex id=1075' "$OUT/nav.log" | head -1 | sed -n 's/.*<- *\([0-9-]*\).*/\1/p')
clicks=$(printf '%s' "$CLICKS" | grep -ao '#1075' | wc -l)
if [ -z "${start:-}" ]; then
    # Do not fall back to an absolute. A gate that green-lights on a number it cannot justify is
    # worse than a red one -- say the instrument went quiet and stop.
    echo "  INCONCLUSIVE: no '[combo] SetIndex id=1075' line, so the START index is unknown"
    echo "  (the assertion is relative to it; without it nothing here can be judged)"
    exit 2
fi
want=$(( ( start + clicks ) % NITEMS ))
echo "  gun-camera combo (id=1075): start=$start + $clicks clicks mod $NITEMS -> expect $want, stored ${got:-none}"
[ "${got:-x}" = "$want" ] && echo "  the UI writes the preference: yes" || { echo "  MISMATCH — FAIL"; fail=1; }
u=$(grep -ac 'NOBODY ANSWERED' "$OUT/nav.log")
echo "  combo dispatches returning stack garbage: $u  (S289 hazard; must stay 0)"
[ "$u" -eq 0 ] || fail=1
echo "----------------------------------------"
[ "$fail" -eq 0 ] && echo "PASS" || { echo "FAIL"; exit 1; }
