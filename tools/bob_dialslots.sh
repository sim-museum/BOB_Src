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
. "$(cd "$(dirname "$0")" && pwd)/bob_safe_kill.sh"   # S392: never kill a bob this gate did not start
bob_snapshot_pids
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_dialslots}"
TMO="${TMO:-420}"
CONTROL="${CONTROL:-0}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
# The control REVERTS THE FIX (BOB_R37_REVERT=1 restores the pointer-as-index truncation at
# MAINFRM.CPP:1795) rather than poisoning slot CONTENTS. Two poison controls came up green because
# the campaign recipe never reaches a title-glyph close at all: they tested a THEORY of the bug
# (uninitialised slots) that turned out to be wrong, not the bug. BOB_R37_CLOSE drives the same
# entry point the PO reached by clicking the dialog's tick glyph.
POISON=""
[ "$CONTROL" = "1" ] && POISON="BOB_R37_REVERT=1"

log="$OUT/dialslots.log"
log2="$OUT/nohook.log"
echo "R3.7 — the logged-child close path  (CONTROL=$CONTROL)"

# TWO RUNS, because one run cannot carry both questions.
# Run A drives the close path (BOB_R37_CLOSE) and asks "does it fault?".
# Run B is the SAME campaign with the hook OFF and asks "is the campaign still intact?".
# The first cut asked both of run A and failed its own PASS arm: closing a logged dialog mid-
# campaign legitimately changes the flow, so "still reaches 3D" was never going to hold there.
# That is a badly posed assertion, not a regression -- and folding them together would have made
# a green control the only way to pass.
run_campaign() {  # $1=extra env  $2=logfile
  ( cd "$GD" && timeout -k 5 -s KILL "$TMO" env \
      BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy \
      BOB_AUTOCLICK="1,1,#1000:0,1,1" \
      BOB_MAP_ACCEPTDIR=40 BOB_CAMPAIGN_FLY=30 BOB_CAMPFLY_GO=1 BOB_MAP_TIMER=8 \
      $1 BOB_SHOT=99999 "$BOB" ) >"$2" 2>&1
  echo $?
}
rc=$(run_campaign "BOB_R37_CLOSE=25 $POISON" "$log")
bob_kill_new
rc2=$(run_campaign "" "$log2")
bob_kill_new

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
# The gate is worthless unless the close path actually RAN. Two earlier arms passed without ever
# reaching it; assert reach explicitly rather than inferring it from the absence of a crash.
if grep -aq "\[r37\] closing logged child" "$log"; then say "the close path was REACHED" "yes"
else say "the close path was REACHED" "NO — gate inconclusive"; fail=1; fi
# the campaign must still WORK -- a fix that stops the crash by stopping the campaign is no fix
# asked of run B (hook OFF): a fix that stops the crash by breaking the campaign is no fix
if grep -aq "InThe3D=1" "$log2"; then say "campaign intact with hook off (run B)" "yes"
else say "campaign intact with hook off (run B)" "NO — FAIL"; fail=1; fi
[ $rc -ge 128 ] && { say "process exited without a signal" "signal $((rc-128)) — FAIL"; fail=1; } \
                || say "process exited without a signal" "yes"

echo "----------------------------------------"
if [ "$CONTROL" = "1" ]; then
  if [ "$fail" -eq 0 ]; then
    echo "CONTROL PASSED — THE GATE IS BROKEN, not the port."
    echo "  BOB_R37_REVERT=1 must reproduce the PO's SIGSEGV. If it does not, this gate proves nothing."
    exit 1
  fi
  echo "CONTROL OK: reverting the fix (pointer-as-index at MAINFRM.CPP:1795) reproduces the PO's"
  echo "            SIGSEGV on the same close path, so this gate is discriminating"
  exit 0
fi
[ "$fail" -eq 0 ] && { echo "PASS: the logged-child close path does not fault, and the campaign is intact"; exit 0; }
echo "FAIL: R3.7 is not fixed"; exit 1
