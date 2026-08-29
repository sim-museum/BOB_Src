#!/usr/bin/env bash
# tools/bob_dialslots.sh — R3.7: the logged-child dialog table must start EMPTY.
#
# WHY THIS EXISTS
# ---------------
# PO 2026-08-28: "I ran german campaign, then tried british campaign which ended in window
# disappearing." The session log ends:
#     [titleglyph] (655,57) -> IDOK (accept) on dialog 0xb2efe10 (16InterceptOffered)
#     [dlgclose] toolbar 1 child 12 closed (root=0xc525d10, asked=0xb2efe10)
#     === CRASH: signal 11 === CRToolBar::CloseLoggedChild(int) rdiallog.cpp:434
# with esi=0x0c and fault_addr=0x3bf95c60.
#
# Line 434 is `if (dialoglinks.loggedchild[i])` -- the ELEMENT READ. MAX_CHILD_DIALS is 16, so slot
# 12 is IN BOUNDS: this was never an overrun and never the recursion bug already fixed in this same
# function (S108). The array CONTENTS were garbage, because
#     loggedchild = new RDialog* [MAX_CHILD_DIALS];
# DEFAULT-initialises -- indeterminate for pointers. Every consumer assumes zeroed slots:
# `while (loggedchild[rv]) rv++` to find a free one, `if (loggedchild[i])` to guard a close. Heap
# residue therefore reads as a live dialog and is dereferenced.
#
# That is exactly why the PO saw it on the SECOND campaign. MSVC's debug allocator hands back zeroed
# pages, so the original shipped; glibc hands back whatever was last freed there, so the fault needs
# a churned heap -- german, THEN british. A first-run-only test would have called this fixed.
#
# WHAT IT ASSERTS
#   1. the German campaign still runs end to end with the slots value-initialised (no regression)
#   2. no SIGSEGV, and specifically none in CloseLoggedChild
#
# NEGATIVE CONTROL -- REPRODUCES THE PO'S CRASH ON DEMAND
# The fix is one character (`()`). A gate that cannot make the bug come back cannot claim to guard
# against it, so CONTROL=1 sets BOB_POISON_DIALSLOTS=1, which refills the freshly value-initialised
# array with residue around the real faulting address. The control MUST crash. If it survives, this
# gate is not measuring what it claims and must be treated as broken -- not as a pass.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_dialslots}"
TMO="${TMO:-420}"
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
POISON=""
[ "$CONTROL" = "1" ] && POISON="BOB_POISON_DIALSLOTS=1"

log="$OUT/dialslots.log"
echo "R3.7 — logged-child dialog slots start empty  (CONTROL=$CONTROL)"
( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
    BOB_AUTOCLICK="1,1,#1000:0,1,1" \
    BOB_MAP_ACCEPTDIR=40 BOB_CAMPAIGN_FLY=30 BOB_CAMPFLY_GO=1 BOB_MAP_TIMER=8 \
    $POISON BOB_SHOT=99999 "$BOB" ) >"$log" 2>&1
rc=$?
pkill -x "$(basename "$BOB")" 2>/dev/null

fail=0
say() { printf '  %-44s %s\n' "$1" "$2"; }
if grep -aq "=== CRASH" "$log"; then
  say "no crash banner" "CRASHED — FAIL"; fail=1
  grep -a -A3 "=== CRASH" "$log" | head -6 | sed 's/^/      /'
else
  say "no crash banner" "yes"
fi
if grep -aq "CloseLoggedChild" "$log"; then say "no fault in CloseLoggedChild" "NAMED IN TRACE — FAIL"; fail=1
else say "no fault in CloseLoggedChild" "yes"; fi
# the campaign must still WORK -- a fix that stops the crash by stopping the campaign is no fix
if grep -aq "InThe3D=1" "$log"; then say "campaign still reaches 3D" "yes"
else say "campaign still reaches 3D" "NO — FAIL"; fail=1; fi
[ $rc -ge 128 ] && { say "process exited without a signal" "signal $((rc-128)) — FAIL"; fail=1; } \
                || say "process exited without a signal" "yes"

echo "----------------------------------------"
if [ "$CONTROL" = "1" ]; then
  if [ "$fail" -eq 0 ]; then
    echo "CONTROL PASSED — THE GATE IS BROKEN, not the port."
    echo "  Poisoned slots must reproduce the PO's SIGSEGV. If they do not, this gate proves nothing."
    exit 1
  fi
  echo "CONTROL OK: poisoned slots reproduce the crash, so the gate is discriminating"
  exit 0
fi
[ "$fail" -eq 0 ] && { echo "PASS: dialog slots start empty and the campaign runs"; exit 0; }
echo "FAIL: R3.7 is not fixed"; exit 1
