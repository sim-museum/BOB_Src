#!/bin/bash
# BoB per-sprint DoD gate suite Ã¢ÂÂ one gl-lock acquisition, one command.
#
# Usage:  gl-lock tools/bob_gates.sh <outdir> [baseline-dir]
#   <outdir>        where this run's captures go (created)
#   [baseline-dir]  optional: a previous run's capture dir to A/B against
#
# Replaces the per-sprint copy-and-sed script that lived in the scratchpad. That arrangement
# broke twice: once when a `sed` rename silently failed to match (so a sprint's sweep overwrote
# the previous sprint's baseline), and once when the recipes drifted from the prose that
# documented them. Recipes live HERE, versioned, so a sprint's gate is reproducible from the repo.
#
# Ã¢ÂÂÃ¢ÂÂ Gate integrity (S148/SP.19) Ã¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂ
# S148's sweep came back 13/14 and the cause was not code: the gate sat in the gl-lock queue while
# the tree was rebuilt twice, so its 14 sequential `bob` invocations straddled two binaries and one
# recipe was captured mid-swap. Two innocent explanations were available for the differing frame
# (live device state; this port's run-to-run variance class) and both would have been comfortable
# to write down. **A gate whose inputs can change under it is not a gate.** So: hash the binary
# before and after, and fail loudly if it moved. Treat "queued" as "running" Ã¢ÂÂ do not touch the
# tree once this is submitted.
set -u

HERE_ABS="$(cd "$(dirname "$0")" && pwd)"   # S351: absolute, resolved BEFORE any cd
BOB=/home/admin/bob/build/bob
BOB_DRIVE_C_REAL="/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c"
GD="$BOB_DRIVE_C_REAL/Program Files/Rowan Software/Battle Of Britain"
OUT="${1:?usage: bob_gates.sh <outdir> [baseline-dir]}"
BASE="${2:-}"

mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"   # S366: absolute BEFORE any cd -- the scratch drive_c is built under it
HASH_BEFORE=$(md5sum "$BOB" | cut -d' ' -f1)
echo "### binary $BOB md5=$HASH_BEFORE"

cd "$GD" || exit 1
E="BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy"

# S199 (answering MA note MA118): this sweep PRINTED `exit=N` for all 14 recipes and NOTHING
# EVER READ THEM -- and stderr went to /dev/null, so the binary's own "=== CRASH: signal N"
# banner was discarded too. A recipe that segfaulted printed `exit=139` in a line of identical
# shape to the thirteen that passed, and the gate's verdict did not change. MiG Alley shipped
# a gate that reported PASS on a run that crashed (its S171); this one could not even have
# noticed. Keep each recipe's stderr, assert the exit code, and say which one failed.
# S199: one checker for every run in this file. `exit=N` printed and never read was not a
# GATE 1 problem -- GATES 2, 3 and 4 did the same. Prints the same "<label> exit=N" line the
# old code did (other tooling reads it), appends a reason only when something is wrong, and
# counts it. Pass the stderr file so a crash banner survives /dev/null.

# ---- S365 (R17): PLAYER-DATA GUARD -------------------------------------------------
# This suite was confirmed twice to overwrite the player's SAVEGAME/dreplay.dat -- a real
# recording replaced by a gate's own, deterministically, on every run. A gate that eats
# player data is a bug in the gate, so the suite now carries its own protection.
#
# Two deliberate design choices:
#   * DETECT per gate, RESTORE once at the end. Restoring between gates would be stronger
#     protection but it would change what the gates see: later recipes (R11's ACMI export)
#     read a recording an earlier one (GATE 8) writes, so a mid-suite rollback could break
#     a passing gate and the failure would look like a regression in the port. Attribution
#     does not need the rollback; the player's file only needs it by the time we exit.
#   * Hash EVERY file in SAVEGAME, not just dreplay.dat. Package.dat has a fresh mtime too,
#     and naming one victim before checking for others is how the second one gets missed.
# ---- S366 (R17): REDIRECT, so the gates never touch the player's directory ------------
# S365's guard detects damage and undoes it afterwards. That is a bandage: the run still eats
# the data, and anything that kills the suite mid-flight (the gl-lock timeout, a crash, Ctrl-C)
# skips the restore entirely. This points the whole run at a scratch drive_c instead --
# symlinks all the way down to the game dir, with SAVEGAME a real copy and VIDEOS a real
# directory (R11's .acmi export must not write back through a link into the player's tree).
#
# It has to be a whole drive_c, not just the game dir. A first attempt linked only the game
# directory and ran with cwd pointed at it; bob segfaulted on
#   [fileman] missing file 8c00=\Program Files\...\landscap\DIR.DIR
# because it builds ABSOLUTE paths from BOB_DRIVE_C -- the working directory redirects nothing.
#
# Verified standalone on GATE 5's own recipe, the measured culprit: gate PASSES (all nine
# checks, reaches 3D), the scratch SAVEGAME takes the dreplay.dat/Package.dat writes, and the
# player's SAVEGAME comes out byte-identical to a pristine backup.
#
# The S365 guard below is KEPT and now watches the PLAYER's directory, where it should see
# nothing. It has become the assertion that the redirect held: if it ever fires again, the
# redirect has broken and it still names the gate. BOB_NO_SCRATCH=1 runs the old way.
PLAYER_GD="$GD"
if [ -z "${BOB_NO_SCRATCH:-}" ]; then
  if bash "$HERE_ABS/bob_scratch_gamedir.sh" "$BOB_DRIVE_C_REAL" "$OUT/drive_c"; then
    export BOB_DRIVE_C="$OUT/drive_c"
    GD="$OUT/drive_c/Program Files/Rowan Software/Battle Of Britain"
    # EXPORT it. The sub-gates take `GD="${GD:-<real path>}"` from the ENVIRONMENT, and an
    # unexported shell variable is invisible to a child script -- so bob_convoy_campaign.sh and
    # bob_combat_soak.sh fell back to the real game directory and cd'd there, and the replay
    # write landed in the player's SAVEGAME even though BOB_DRIVE_C pointed at the scratch tree.
    # The S365 guard caught it and named both gates, which is exactly the job it was kept for.
    export GD
    cd "$GD" || exit 1
  else
    echo "### SCRATCH BUILD FAILED -- falling back to the player's directory (guard still armed)"
  fi
fi

# S368: watch VIDEOS as well as SAVEGAME. VIDEOS/replay.dat is the flight recording -- player
# data by any measure -- and the guard never looked at it, so GATE R1 quietly rewrote the real
# one on every run while the report said "player data untouched". It was found only because the
# redirect made R1 fail and the mtime on the real file gave it away. A guard that watches one
# directory reports honestly about that directory and says nothing at all about the rest.
SAVEDIR="$PLAYER_GD/SAVEGAME"
VIDEODIR="$PLAYER_GD/VIDEOS"
GUARD_BAK="$OUT/_savegame_before"
mkdir -p "$GUARD_BAK"
mkdir -p "$GUARD_BAK/SAVEGAME" "$GUARD_BAK/VIDEOS"
cp -p "$SAVEDIR"/*  "$GUARD_BAK/SAVEGAME/" 2>/dev/null
cp -p "$VIDEODIR"/* "$GUARD_BAK/VIDEOS/"   2>/dev/null
sg_hash() { md5sum "$SAVEDIR"/* "$VIDEODIR"/* 2>/dev/null | sort -k2 | md5sum | cut -d" " -f1; }
guard_prev=$(sg_hash)
guard_hits=0
guard_culprits=""
replay_check() {   # $1 = the gate that has just finished
  local now; now=$(sg_hash)
  [ "$now" = "$guard_prev" ] && return 0
  local changed="" f b d n
  for d in "$SAVEDIR" "$VIDEODIR"; do
    n="$(basename "$d")"
    for f in "$d"/*; do
      [ -f "$f" ] || continue
      b="$GUARD_BAK/$n/$(basename "$f")"
      if [ ! -f "$b" ] || ! cmp -s "$f" "$b"; then changed="$changed $n/$(basename "$f")"; fi
    done
  done
  # A DELETED file has no counterpart to iterate over, so the loop above cannot see it and the
  # census would stay silent about the one kind of damage that is hardest to notice. Walk the
  # backup side too and name what has gone missing.
  for d in SAVEGAME VIDEOS; do
    for b in "$GUARD_BAK/$d"/*; do
      [ -f "$b" ] || continue
      [ -f "$PLAYER_GD/$d/$(basename "$b")" ] || changed="$changed $d/$(basename "$b")(DELETED)"
    done
  done
  echo "###   PLAYER-DATA: $1 wrote SAVEGAME:$changed"
  guard_hits=$((guard_hits+1))
  guard_culprits="$guard_culprits [$1:$changed ]"
  guard_prev="$now"
}
guard_restore() {
  local n=0 d=0 f b dir
  for dir in SAVEGAME VIDEOS; do
    for b in "$GUARD_BAK/$dir"/*; do
      [ -f "$b" ] || continue
      f="$PLAYER_GD/$dir/$(basename "$b")"
      if ! cmp -s "$b" "$f"; then cp -p "$b" "$f"; n=$((n+1)); fi
    done
  done
  # Files the run CREATED are litter too: restoring only what we backed up leaves the
  # directory a superset of how it started, and the next run's "before" snapshot then
  # bakes the litter in as if it were player data. Safe here precisely because a gate run
  # has no player in it -- anything new in SAVEGAME was written by a gate.
  for dir in SAVEGAME VIDEOS; do
    for f in "$PLAYER_GD/$dir"/*; do
      [ -f "$f" ] || continue
      [ -f "$GUARD_BAK/$dir/$(basename "$f")" ] && continue
      rm -f "$f"; d=$((d+1))
    done
  done
  [ "$d" -gt 0 ] && echo "###   removed $d file(s) the run created"
  if [ "$guard_hits" -eq 0 ]; then echo "### PLAYER DATA: untouched by this run"
  else
    echo "### PLAYER DATA: $guard_hits gate(s) wrote to SAVEGAME --$guard_culprits"
    echo "###   restored $n file(s) from $GUARD_BAK; originals preserved there"
  fi
}

gates_fail=0
checkrun() {   # $1 = label, $2 = rc, $3 = stderr file (may be missing)
  local label="$1" rc="$2" errf="${3:-}" note=""
  if [ -n "$errf" ] && [ -s "$errf" ] && grep -aq "=== CRASH: signal" "$errf"; then
    note="  <-- CRASHED: $(grep -a '=== CRASH: signal' "$errf" | head -1 | sed 's/^=== //;s/ ===$//')"
    gates_fail=$((gates_fail+1))
  elif [ "$rc" -ne 0 ]; then
    note="  <-- FAIL (exit $rc$([ "$rc" -ge 124 ] && echo '; 124=timeout, >=128=signal'))"
    gates_fail=$((gates_fail+1))
  fi
  echo "  $label exit=$rc$note"
}
echo "### GATE 1: 14-recipe headless sweep"
g1_fail=0
run() { local name="$1"; local shot="$2"; shift 2
  timeout -k 5 240 env $E "$@" BOB_SHOT="$shot" BOB_SHOT_PATH="$OUT/$name.ppm" "$BOB" \
      >"$OUT/$name.out" 2>"$OUT/$name.err"
  local rc=$? before=$gates_fail
  checkrun "$name" "$rc" "$OUT/$name.err"
  [ "$gates_fail" -ne "$before" ] && g1_fail=$((g1_fail+1)); return 0; }
run mainmenu       40
run config-gfx     70 BOB_CONFIGSCREEN=gfx
run config-gfx2    70 BOB_CONFIGSCREEN=gfx2
run config-control 70 BOB_CONFIGSCREEN=control
run config-sound   70 BOB_CONFIGSCREEN=sound
run sim-flight     70 BOB_CONFIGSCREEN=flight
run sim-game       70 BOB_CONFIGSCREEN=game
run sim-mission    70 BOB_CONFIGSCREEN=mission
run sim-views      70 BOB_CONFIGSCREEN=views
run quickshots    220 BOB_STARTFLYING=click BOB_AUTOCLICK=0
run sideselect    250 BOB_AUTOCLICK=1
run phaseselect   380 BOB_AUTOCLICK=1,1
run entername     520 BOB_AUTOCLICK=1,1,1
run bobfrag       120 BOB_BOBFRAG=1
if [ "$g1_fail" -eq 0 ]; then echo "  GATE 1: PASS (14/14 clean exits, no crash banners)"
else echo "  GATE 1: FAIL ($g1_fail of 14 recipes crashed or exited non-zero)"; fi

# Ã¢ÂÂÃ¢ÂÂ GATE 1c (S165): the game's own confirmation box answers what the player clicked Ã¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂ
# CDialog::DoModal was `return -1` and EndDialog a no-op, so RDialog::RMessageBox always reported
# the same answer -- and CMainFrame::OnBye reads `rv<2` as "quit", so the game left a campaign
# without ever showing the Save/Yes/Cancel it was written to show. There is no headless trigger for
# a modal (OnBye needs the system box; the bad-weather prompt needs a campaign day whose weather
# says so), so BOB_TEST_MODAL fires the real RMessageBox from the idle loop and prints its return.
# Assert on the ANSWER, per button: three distinct codes is the only thing that proves a loop ran.
replay_check "GATE 1"
echo "### GATE 1c: modal message box (Save=0 Yes=1 Cancel=2)"
modal_ok=1
# S181: these coordinates are DERIVED from IDDS_MODAL_DIALOG, not fitted to the screen.
#   dialog 272x104 DLU at 1024x768 -> 408x169 px, centred at (308,299)
#   IDC_OK/IDC_CANCEL/IDC_RETRY: DLU x=13/98/183, y=70, 80x18  (px = DLU*6/4 horiz, *13/8 vert)
#   -> centres (387,426) (515,426) (642,426)
# They used to be y=411, x=390/518/647 -- fitted to the layout BEFORE the modal was clipped to its
# template (the box drew its whole 780x585 art sheet from the origin). So this gate was pinned to
# the buggy geometry, and the S181 fix made it fail: a gate that encodes a defect passes only while
# the defect survives. Assert the geometry line too, so a future layout change fails LOUDLY here
# rather than by three silently-missed clicks.
geom=$(cd "$GD" && timeout -k 5 120 env BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
       SDL_VIDEODRIVER=dummy BOB_TEST_MODAL=60 BOB_TEST_MODAL_EXIT=1 \
       BOB_CLICKXY="100,387,426" "$BOB" 2>&1 | grep -a -m1 "\[modal\] template" | tr -d '\r')
case "$geom" in
  *"272x104 DLU -> 408x169 px at (308,299)"*) echo "  geometry: $geom" ;;
  *) echo "  geometry CHANGED -> $geom"; echo "  (button coords below are derived from the old template; re-derive them)"; modal_ok=0 ;;
esac
for bx in "387 Save 0" "515 Yes 1" "642 Cancel 2"; do
  set -- $bx
  got=$(cd "$GD" && timeout -k 5 200 env BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
        SDL_VIDEODRIVER=dummy BOB_TEST_MODAL=60 BOB_TEST_MODAL_EXIT=1 \
        BOB_CLICKXY="100,$1,426;200,$1,426;400,$1,426" "$BOB" 2>&1 |
        grep -a "RMessageBox returned" | sed "s/.*returned \\([0-9-]*\\).*/\\1/")
  if [ "$got" = "$3" ]; then echo "  $2 -> $got"; else echo "  $2 -> $got (EXPECTED $3)"; modal_ok=0; fi
done
[ $modal_ok -eq 1 ] && echo "  modal: PASS" || echo "  modal: FAIL"

replay_check "GATE 1c"
echo "### GATE 2: safe default (BOB_NO_RUN)"
timeout -k 5 120 env BOB_NO_RUN=1 "$BOB" >/dev/null 2>"$OUT/default.err"; checkrun default $? "$OUT/default.err"

replay_check "GATE 2"
echo "### GATE 3: phase select dummy vs real GL"
timeout -k 5 300 env $E BOB_AUTOCLICK=1,1,#1000:1 BOB_SHOT=520 \
  BOB_SHOT_PATH="$OUT/gl_dummy.ppm" "$BOB" >/dev/null 2>"$OUT/gl_dummy.err"; checkrun dummy $? "$OUT/gl_dummy.err"
timeout -k 5 300 env DISPLAY=:0 BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
  BOB_AUTOCLICK=1,1,#1000:1 BOB_SHOT=520 BOB_SHOT_PATH="$OUT/gl_real.ppm" "$BOB" >/dev/null 2>"$OUT/gl_real.err"
checkrun realGL $? "$OUT/gl_real.err"
cmp -s "$OUT/gl_dummy.ppm" "$OUT/gl_real.ppm" && echo "  dummy==GL BYTE-IDENTICAL" || echo "  dummy!=GL DIFFERS"

replay_check "GATE 3"
echo "### GATE 4: flight frame-150 (real GL)"
rm -f "$OUT/flight.ppm"
timeout -k 5 300 env DISPLAY=:0 BOB_BOOT_FRONTEND=1 BOB_DUMP_FRAME=150 \
  BOB_DUMP_PATH="$OUT/flight.ppm" BOB_EXIT_AFTER_DUMP=1 "$BOB" >/dev/null 2>"$OUT/flight.err"
checkrun flight $? "$OUT/flight.err"
python3 - "$OUT/flight.ppm" <<'PY'
import sys
try:
    from PIL import Image
    im = Image.open(sys.argv[1]).convert('RGB')
    d = list(im.getdata())
    print(f"  flight frame-150: {100.0*sum(1 for p in d if p!=(0,0,0))/len(d):.1f}% non-black")
except Exception as e:
    print(f"  flight measure failed: {e}")
PY

# Ã¢ÂÂÃ¢ÂÂ GATE 4b (S173v): terrain tiles must not come out black Ã¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂ
# BOB-PO-2 was a D3D->GL viewport origin that was never flipped: correct for a full-size viewport
# and wrong for every smaller one, so the landscape tile compositor read its scratch target's
# top-left corner while GL had rendered into the bottom-left, and 64x64/128x128 tiles uploaded
# black. It survived nine eliminated mechanisms precisely because nothing asserted on it. Assert on
# the NUMBER, not the picture: BOB_TRACE_TEXBLACK counts quads drawn with an all-black texture, and
# the correct answer is zero. Needs real GL (the tiles are composited in an FBO).
replay_check "GATE 4"
echo "### GATE 4b: terrain tiles textured (blackTex must be 0)"
tb=$(cd "$GD" && timeout -k 5 400 env DISPLAY=:0 BOB_BOOT_FRONTEND=1 BOB_QM_INDEX=11 \
      BOB_AUTOFLY=view40 BOB_TRACE_TEXBLACK=20000 BOB_DUMP_FRAME=300 \
      BOB_DUMP_PATH=/dev/null BOB_EXIT_AFTER_DUMP=1 "$BOB" 2>&1 |
      grep -a "quads: blackTex=" | tail -1 | sed "s/.*blackTex=\\([0-9]*\\).*/\\1/")
if [ -z "$tb" ]; then echo "  blackTex: NO SAMPLE (run failed?)"
elif [ "$tb" = "0" ]; then echo "  blackTex=0 Ã¢ÂÂ PASS"
else echo "  blackTex=$tb Ã¢ÂÂ FAIL (expected 0; terrain tiles are uploading black)"; fi

# S196: the German Convoys campaign, end to end. It belongs in the DoD set because S192-S195
# established that EVERY gate above passed while that campaign was broken -- the front end, the
# modal, a quick-mission flight and the terrain all say nothing about whether a campaign can be
# started, planned, launched and flown. Nested inside this suite's single lock, like its siblings.
replay_check "GATE 4b"
echo "### GATE 5: German (Luftwaffe) Convoys campaign end to end"
# absolute path: this suite cd's into the game directory in its subshells, so a $(dirname $0)
# relative path resolves against whatever the last cd left behind (measured: "No such file").
# S206: ${PIPESTATUS[@]} IS CLOBBERED BY THE NEXT COMMAND, INCLUDING AN `echo` OR AN `if`. This
# block used to read GATE 5's status AFTER running GATE 6 and after an intervening `if`, so `cg`
# was the exit status of that `if`'s echo -- ZERO, always. "campaign: PASS" was printed
# unconditionally, whatever the PO's gold-standard campaign gate actually did, and GATE 5 never
# contributed to gates_fail. Capture every pipeline's status on the very next line, always.
bash /home/admin/bob/tools/bob_convoy_campaign.sh 2>&1 | sed 's/^/  /'
cg=${PIPESTATUS[0]}
if [ "$cg" = "0" ]; then echo "  campaign: PASS"; else echo "  campaign: FAIL (exit=$cg)"; gates_fail=$((gates_fail+1)); fi

replay_check "GATE 5"
echo "### GATE 6: combat soak (S201)"
# The dogfight crash (S200) passed every gate in this file because none of them flies long
# enough to hit a crash in the sim. GATE 6 soaks the campaign for its full run and asserts
# crash-freedom over ~1.45M movecode dispatches; it also REPORTS aircraft-level combat activity.
# S206: that report reads zero because this recipe flies, and a flight leaves the strategic sim
# advancing at roughly real time -- the raid it would have to meet is still forming when the soak
# ends. The zero is about the RECIPE'S REACH, not about the ACM tree. GATE 7 below covers the
# strategic half on a recipe that can actually produce it.
bash /home/admin/bob/tools/bob_combat_soak.sh 2>&1 | sed 's/^/  /'
cs=${PIPESTATUS[0]}
if [ "$cs" = "0" ]; then echo "  soak: PASS"; else echo "  soak: FAIL (exit=$cs)"; gates_fail=$((gates_fail+1)); fi

replay_check "GATE 6"
echo "### GATE 7: strategic soak -- the raid flies and the RAF is tasked (S206)"
# The property S204 measured as zero and concluded was broken. It is not: give the campaign the
# game time it needs (stay on the map) and the raid flies its whole route, SetRAFIntercept is
# reached and RAF AM_INTERCEPT packages appear. Has a negative control: STARVE=1 reruns S204's
# own flight recipe and must go red on the same binary.
bash /home/admin/bob/tools/bob_strategic_soak.sh 2>&1 | sed 's/^/  /'
ss=${PIPESTATUS[0]}
if [ "$ss" = "0" ]; then echo "  strategic: PASS"; else echo "  strategic: FAIL (exit=$ss)"; gates_fail=$((gates_fail+1)); fi

# -- GATE 8 (S317): R1 -- the UI's own preference arms the recorder, in ONE process ------------
replay_check "GATE 7"
echo "### GATE 8: R1 continuous -- Sim Config -> Gun Camera -> Fly -> recording (S313-S317)"
# Committed RED by S313 and deliberately left OUT of this suite until it could pass. It now does,
# and three real defects had to be cleared to get there, each hiding the next: S315 (the strategic
# map segfault -- pItem[SagBAND] off a NULL array, which this harness had been ROUTING AROUND with
# a comment rather than fixing), S316 ("try indices 0,1,2 for Fly" clicks BACK first on screen
# 27917; Fly is index 2), and S317 (the route itself -- main -> Sim Config -> Continue never passes
# through Quick Shots, so no mission is selected and 3D entry has no player aircraft).
# Has a negative control and it is REAL: CONTROL=1 flies the same route with the gun camera never
# switched on and must record 0 bytes. Measured -- 181382 bytes with the preference, 0 without.
bash /home/admin/bob/tools/bob_r1_continuous.sh 2>&1 | sed 's/^/  /'
r1=${PIPESTATUS[0]}
if [ "$r1" = "0" ]; then echo "  r1: PASS"; else echo "  r1: FAIL (exit=$r1)"; gates_fail=$((gates_fail+1)); fi

if [ -n "$BASE" ] && [ -d "$BASE" ]; then
  echo "### GATE 5: A/B vs $BASE"
  python3 - "$BASE" "$OUT" <<'PY'
import sys, os
from PIL import Image, ImageChops
base, out = sys.argv[1], sys.argv[2]
names = ['mainmenu','config-gfx','config-gfx2','config-control','config-sound','sim-flight',
         'sim-game','sim-mission','sim-views','quickshots','sideselect','phaseselect',
         'entername','bobfrag']
ident = 0
for n in names:
    a, b = os.path.join(base, n+'.ppm'), os.path.join(out, n+'.ppm')
    if not (os.path.exists(a) and os.path.exists(b)):
        print(f"  {n:16s} MISSING"); continue
    bb = ImageChops.difference(Image.open(a).convert('RGB'), Image.open(b).convert('RGB')).getbbox()
    if bb is None: ident += 1
    else: print(f"  {n:16s} DIFF bbox={bb}")
print(f"  {ident}/{len(names)} byte-identical")
PY
fi

# ---- R11: ACMI export orientation (S276/S327) -----------------------------------------------
# Offline and displayless: reads whatever .acmi this run (or a previous one) produced and asserts
# the Tacview CONVENTION, not merely internal consistency. The yaw-sense bug passed every
# self-consistency check ever run against it, because yaw and course share the convention.
replay_check "GATE 8"
echo "### GATE R9: 1920x1080 canvas placement (default / centre / scale)"
# S361: R9's two remedies were each verified ONCE by hand in the session that built them, and the
# S342 bug -- placement computed from the PREVIOUS frame, so the first frame after a mode change
# painted wrong -- is exactly the kind that returns unnoticed. This asserts the LIT REGION of a real
# capture for all three placements, using the same glReadPixels instrument that found R9, rather
# than the [centre]/[scale] log lines, which only say what the code intended.
if "$HERE_ABS/bob_r9_layout.sh"; then
  echo "###   R9 PASS"
else
  echo "###   R9 FAIL -- canvas placement regressed"; gates_fail=$((gates_fail+1))
fi

replay_check "GATE R9"
echo "### GATE R11: ACMI export orientation"
# ONLY the live export. VIDEOS/*.acmi are an ARCHIVE and every one predates S276, so scanning
# them would leave this gate permanently red -- which trains the reader to ignore it.
_acmi=""
[ -f "$GD/acmi_current.txt" ] && _acmi="$GD/acmi_current.txt"
if [ -z "$_acmi" ]; then
  echo "###   SKIP: no .acmi produced -- nothing to check (fly a sortie with BOB_ACMI unset/on)"
else
  # S351: $0 is a RELATIVE path and this gate has already cd'd to "$GD", so dirname "$0" resolved
  # to a "tools" directory that does not exist there -- the suite reported "R11 FAIL -- yaw
  # convention regressed" when the script simply could not be found. A missing harness must never
  # be reported as a regression in the thing it measures. Resolve the script directory once,
  # absolutely, before any cd.
  if "$HERE_ABS/bob_acmi_orientation.sh" "$_acmi"; then   # S365: MUST be quoted -- the game dir has spaces
    echo "###   R11 PASS"
  else
    _r=$?
    if [ "$_r" -eq 2 ]; then echo "###   R11 INCONCLUSIVE (no east-west leg in these sorties)"
    else echo "###   R11 FAIL -- yaw convention regressed"; gates_fail=$((gates_fail+1)); fi
  fi
fi

HASH_AFTER=$(md5sum "$BOB" | cut -d' ' -f1)
if [ "$HASH_BEFORE" != "$HASH_AFTER" ]; then
  echo "### !!! GATE INVALID: the binary CHANGED during this run"
  echo "###     before=$HASH_BEFORE after=$HASH_AFTER"
  echo "###     The sweep straddled two builds; its captures cannot be read as a code result."
  echo "###     Re-run without rebuilding (treat 'queued' as 'running'). See SP.19 / PORT.md S148."
  echo "### DONE (INVALID)"
  exit 2
fi
echo "### binary unchanged (md5=$HASH_AFTER) Ã¢ÂÂ gate valid"
# S199: one line that says whether ANY run in this file crashed or exited non-zero. Without it
# the outcome of every run was printed and never judged, so a crashed recipe looked like a
# passing one unless a human read the numbers.
# ---- S370: gates that existed but nothing ran ------------------------------------------------
# An audit of tools/ against this file found TEN scripts the suite never invoked. A gate nobody
# runs protects nothing -- the same finding as ma S372 and the julia gates.sh, and the reason
# BoB's own campaign gate stayed green for nine sprints while being unpassable.
# Each was RUN before being added, because adding a red gate makes the suite permanently red and
# adding a probe makes it report failure for a tool that never had a verdict to give:
#   ADDED (all measured green): bob_mp_connect, bob_mp_packet, check_notes_sync,
#                               bob_mp_uijoin, bob_dialslots, bob_settings_nav
#   S372 CORRECTION -- two of the three original exclusions were MY MEASUREMENT ERROR, not gate
#   faults, and recording them as gate faults would have retired two working gates:
#     bob_dialslots     "times out (exit 124)" was a 300s cap against a gate that documents TWO
#                       420s runs. Given its own budget it PASSES all five arms.
#     bob_settings_nav  "exceeded a 10-minute cap" was measured while bob_dialslots held gl-lock
#                       in the background, so it spent that time QUEUED FOR THE DISPLAY, not
#                       running. With the display free it PASSES in 5m00s.
#     bob_mp_uijoin     the one real finding: its drive was a click short (S371), now fixed.
#   The lesson is cheap to state and was not: do not time a gate without checking what it costs,
#   and do not measure anything on this box while another display gate is queued.
#   AUDIT CLOSED (S373). Ten orphans: SIX added (mp_connect, mp_packet, mp_uijoin, dialslots,
#   settings_nav, check_notes_sync); FOUR correctly excluded, each for a stated reason:
#     bob_detect_probe  a PROBE, no verdict; its own header warns not to read its zeros as findings
#     bob_blob_bisect   an investigation instrument, not an assertion
#     bob_validate      a capture HARNESS -- parks a camera, grabs a frame, reports stats. Its
#                       lone exit 1 is a usage error, so it has no verdict to contribute.
#     bob_replay_record FAILS, and its own report contradicts itself: "recorder armed NO" beside
#                       "recording written 81178 bytes". The bytes are real and freshly written,
#                       so the FEATURE works and the ASSERTION is stale -- it greps for
#                       "[guncam] StartRecordFlag branch TAKEN", an unconditional trace in
#                       WINMOVE.CPP, while the recording evidently reaches the file another way.
#                       Adding it would park a permanently red gate in the suite for a working
#                       feature. The contradiction in its own output is the lead for re-deriving
#                       the arming check against the path recording ACTUALLY takes.
echo "### GATE MP1: multiplayer front door (DirectPlay object + lobby)"
bash "$HERE_ABS/bob_mp_connect.sh" 2>&1 | sed 's/^/  /'
mp1=${PIPESTATUS[0]}
if [ "$mp1" = "0" ]; then echo "  mp_connect: PASS"; else echo "  mp_connect: FAIL (exit=$mp1)"; gates_fail=$((gates_fail+1)); fi
replay_check "GATE MP1"
echo "### GATE MP2: discovery, join and a packet across two processes"
bash "$HERE_ABS/bob_mp_packet.sh" 2>&1 | sed 's/^/  /'
mp2=${PIPESTATUS[0]}
if [ "$mp2" = "0" ]; then echo "  mp_packet: PASS"; else echo "  mp_packet: FAIL (exit=$mp2)"; gates_fail=$((gates_fail+1)); fi
replay_check "GATE MP2"
echo "### GATE MP3: the Join list is populated from a real host (with a no-host control)"
bash "$HERE_ABS/bob_mp_uijoin.sh" 2>&1 | sed 's/^/  /'
mp3=${PIPESTATUS[0]}
if [ "$mp3" = "0" ]; then echo "  mp_uijoin: PASS"; else echo "  mp_uijoin: FAIL (exit=$mp3)"; gates_fail=$((gates_fail+1)); fi
replay_check "GATE MP3"
echo "### GATE R3.7: the logged-child close path does not fault (two runs, ~14 min)"
bash "$HERE_ABS/bob_dialslots.sh" 2>&1 | sed 's/^/  /'
ds=${PIPESTATUS[0]}
if [ "$ds" = "0" ]; then echo "  dialslots: PASS"; else echo "  dialslots: FAIL (exit=$ds)"; gates_fail=$((gates_fail+1)); fi
replay_check "GATE R3.7"
echo "### GATE SET: the settings UI writes its preference (combo dispatch, S289 hazard)"
bash "$HERE_ABS/bob_settings_nav.sh" 2>&1 | sed 's/^/  /'
sn=${PIPESTATUS[0]}
if [ "$sn" = "0" ]; then echo "  settings_nav: PASS"; else echo "  settings_nav: FAIL (exit=$sn)"; gates_fail=$((gates_fail+1)); fi
replay_check "GATE SET"
echo "### GATE NOTES: cross-port lessons doc in sync with MiG Alley"
bash "$HERE_ABS/check_notes_sync.sh" 2>&1 | sed 's/^/  /'
nt=${PIPESTATUS[0]}
if [ "$nt" = "0" ]; then echo "  notes_sync: PASS"; else echo "  notes_sync: FAIL (exit=$nt)"; gates_fail=$((gates_fail+1)); fi

replay_check "GATE R11"
guard_restore

if [ "$gates_fail" -eq 0 ]; then echo "### RUNS: all clean (no crashes, no non-zero exits)"
else echo "### RUNS: $gates_fail run(s) CRASHED or exited non-zero Ã¢ÂÂ see the notes above"; fi
echo "### DONE"
[ "$gates_fail" -eq 0 ] || exit 1
