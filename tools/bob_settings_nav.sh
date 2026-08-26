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
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
BOB="${BOB:-/home/admin/bob/build/bob}"
OUT="${OUT:-/tmp/bob_settings_nav}"; mkdir -p "$OUT"
CLICKS="${CLICKS:-6,3,#1075,#1075,6}"     # Sim Config -> Views -> cycle Gun Camera twice -> Continue
WANT="${WANT:-2}"                          # expected stored val (2 = at-start bit)
pgrep -x bob >/dev/null && { echo "REFUSING: bob already running"; exit 2; }
( cd "$GD" && timeout -k 5 -s KILL "${TMO:-300}" env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c" \
    BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
    BOB_AUTOCLICK="$CLICKS" BOB_TRACE_SETFIELD=1 BOB_TRACE_COMBO=1 \
    "$BOB" ) >"$OUT/nav.log" 2>&1
pkill -x bob 2>/dev/null
fail=0
n=$(grep -ac '\[setfield\]' "$OUT/nav.log")
echo "  settings write-backs seen: $n"
[ "$n" -gt 0 ] || { echo "  the settings screen was never committed — nothing below is meaningful — FAIL"; exit 1; }
got=$(grep -a 'setfield.*id=1075' "$OUT/nav.log" | tail -1 | sed -n 's/.*val=\([0-9-]*\).*/\1/p')
echo "  gun-camera combo (id=1075) stored val: ${got:-none}  (want $WANT)"
[ "${got:-x}" = "$WANT" ] && echo "  the UI writes the preference: yes" || { echo "  MISMATCH — FAIL"; fail=1; }
u=$(grep -ac 'NOBODY ANSWERED' "$OUT/nav.log")
echo "  combo dispatches returning stack garbage: $u  (S289 hazard; must stay 0)"
[ "$u" -eq 0 ] || fail=1
echo "----------------------------------------"
[ "$fail" -eq 0 ] && echo "PASS" || { echo "FAIL"; exit 1; }
