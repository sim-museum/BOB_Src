#!/usr/bin/env bash
# tools/bob_convoy_campaign.sh — gate: the GERMAN (Luftwaffe) Convoys campaign runs end to end.
#
# WHY THIS EXISTS. The PO's gold video (`~/gold standard/bob/bob_convoy_campaign.mp4`, 185 s) walks
# the Luftwaffe Convoys campaign from the title screen to a Bf 109 cockpit over the Channel. Getting
# there crosses six subsystems that had never been driven together, and S192/S193 found two real
# defects on the way plus one wrong conclusion. None of that is protected by the existing gates:
# they cover the front end, the modal, a quick-mission flight and the terrain, and every one of them
# passed while the campaign was broken.
#
# WHAT IT ASSERTS, in the order gold does it:
#   1. Luftwaffe side + Convoys phase         the phase is chosen by COLUMN (#1000:0). S192: naming
#                                             the listbox without a column silently selected
#                                             "Critical Period" -- a working-looking screen with the
#                                             wrong campaign in it.
#   2. the campaign starts on 10 July         date/time from the game's own shot-state banner
#   3. Luftwaffe Directives open              as the LW player the game opens them itself; without
#                                             accepting them the campaign has no raids of its own
#   4. accepting them creates a raid          MakeLWPackages -> a package with attackmethod >= 8
#   5. the raid LAUNCHES                      a squadron with a live instance. S193: `instance == 0`
#                                             means "no air group" and TWO opposite histories produce
#                                             it -- not yet launched, and already finished -- so this
#                                             asserts on the scan FINDING one, not on a flag.
#   6. briefing -> Fly -> InThe3D=1           the seam gold reaches at t~77 and t~87
#
# TIMING MATTERS AND IS PART OF THE TEST. The scan must start before the raid's sortie is over
# (S193: with a fast map clock the whole sortie ran before the first scan and every scan saw a
# corpse -- PS_COMPLETE with its instances zeroed by Profile::PackageComplete). BOB_CAMPAIGN_FLY=30
# with BOB_MAP_TIMER=8 keeps the airborne window wide in paint terms.
# NEGATIVE CONTROL -- now RUNNABLE, was prose. CONTROL=1 runs the same drive WITHOUT
# BOB_MAP_ACCEPTDIR (i.e. never accept the Luftwaffe's orders) and the last four assertions must go
# to zero: no hipack, no playersquadron, no InThe3D=1. That is the campaign correctly having
# nothing to fly, and it is what this gate exists to distinguish from a break.
#
# S209: this control used to be a PARAGRAPH saying it had been checked once. That is not a control
# arm, and this gate is the proof: for NINE SPRINTS it shipped an assertion that could never pass
# (S206 -- it grepped for `phase=0`, a field printed only when a BOB_SHOT capture fires, while the
# recipe sets BOB_SHOT=99999), and the "checked" control never caught it because nothing re-ran it.
# A control that cannot be re-run tests the day it was written and nothing after.
# Answering MA's §8-MA135, whose claim this sharpens: it is not enough for the control arms to
# SCORE differently -- they have to still EXIST as something you can execute.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_convoy_gate}"
TMO="${TMO:-600}"
CONTROL="${CONTROL:-0}"
ACCEPT="BOB_MAP_ACCEPTDIR=40"
[ "$CONTROL" = "1" ] && ACCEPT=""
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }

log="$OUT/convoy.log"
echo "German (Luftwaffe) Convoys campaign — end to end"
# Run this UNDER gl-lock like the other gates; it does not take the lock itself.
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
    BOB_AUTOCLICK="1,1,#1000:0,1,1" \
    $ACCEPT BOB_CAMPAIGN_FLY=30 BOB_CAMPFLY_GO=1 BOB_MAP_TIMER=8 \
    BOB_TRACE_CAMPFLY=1 BOB_TRACE_LWDIR=1 \
    BOB_SHOT=99999 BOB_SHOT_PATH="$OUT/convoy.ppm" "$BOB" ) >"$log" 2>&1
pkill -x "$(basename "$BOB")" 2>/dev/null

fail=0
say() { printf '  %-42s %s\n' "$1" "$2"; }
chk() { if grep -aq "$2" "$log"; then say "$1" "yes"; else say "$1" "NO — FAIL"; fail=1; fi; }

# Each assertion keys on something the run actually WRITES. The first draft grepped for "10th July"
# -- which is drawn on the screen and never logged -- and for scan fields that the scan's own
# throttle can skip when the raid is found immediately. Both would have failed a working campaign.
# An assertion has to name evidence the run emits, not evidence a human can see.
chk "Luftwaffe side selected"        "localplayer=2(LW)"
# S206: WAS `chk "Convoys phase (index 0)" "phase=0"`, keyed on the [shot-state] banner -- which
# only prints when a BOB_SHOT capture fires, and this recipe sets BOB_SHOT=99999 so it never does.
# The assertion could not pass on the day it was written (S195) and the gate has been red ever
# since; bob_gates.sh printed "campaign: PASS" unconditionally (clobbered ${PIPESTATUS}) so it was
# never seen. Now keyed on [campphase], emitted unconditionally by FULLPANE.CPP where the choice is
# actually made. See the header rule above: name evidence the RUN EMITS -- and check that the
# recipe you ship actually emits it.
chk "Convoys phase (index 0)"        "\[campphase\] whichcamp=0"
chk "Directives opened for the LW"   "OpenDirectivetoggle(NULL)"
chk "directives accepted"            "\[directives\] accept"
chk "a raid package was selected"    "hipack=[0-9]"
chk "a LIVE squadron was chosen"     "playersquadron=[0-9]"
chk "briefing raised"                "LaunchFullPane(bobfrag"
chk "Fly reached 3D"                 "InThe3D=1"
if grep -aq "FATAL" "$log"; then say "no fatal error" "FATAL present — FAIL"; fail=1; else say "no fatal error" "yes"; fi

echo "----------------------------------------"
if [ "$CONTROL" = "1" ]; then
  # inverted: the control is healthy when the gate goes RED
  if [ "$fail" -ne 0 ]; then
    echo "CONTROL OK: with the LW orders never accepted, the campaign has nothing to fly and this"
    echo "            gate fails -- so it is discriminating, not just passing."
    exit 0
  else
    echo "CONTROL BROKEN: passed even with the orders never accepted. This gate does not test what"
    echo "                it claims; see the header."
    exit 1
  fi
fi
if [ "$fail" -eq 0 ]; then
  echo "PASS: the German Convoys campaign reaches 3D (log $log)"
else
  echo "FAIL"; exit 1
fi
