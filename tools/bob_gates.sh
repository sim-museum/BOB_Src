#!/bin/bash
# BoB per-sprint DoD gate suite — one gl-lock acquisition, one command.
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
# ── Gate integrity (S148/SP.19) ────────────────────────────────────────────────────────────────
# S148's sweep came back 13/14 and the cause was not code: the gate sat in the gl-lock queue while
# the tree was rebuilt twice, so its 14 sequential `bob` invocations straddled two binaries and one
# recipe was captured mid-swap. Two innocent explanations were available for the differing frame
# (live device state; this port's run-to-run variance class) and both would have been comfortable
# to write down. **A gate whose inputs can change under it is not a gate.** So: hash the binary
# before and after, and fail loudly if it moved. Treat "queued" as "running" — do not touch the
# tree once this is submitted.
set -u

BOB=/home/admin/bob/build/bob
GD="/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
OUT="${1:?usage: bob_gates.sh <outdir> [baseline-dir]}"
BASE="${2:-}"

mkdir -p "$OUT"
HASH_BEFORE=$(md5sum "$BOB" | cut -d' ' -f1)
echo "### binary $BOB md5=$HASH_BEFORE"

cd "$GD" || exit 1
E="BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy"

echo "### GATE 1: 14-recipe headless sweep"
run() { local name="$1"; local shot="$2"; shift 2
  timeout -k 5 240 env $E "$@" BOB_SHOT="$shot" BOB_SHOT_PATH="$OUT/$name.ppm" "$BOB" >/dev/null 2>&1
  echo "  $name exit=$?"; }
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

# ── GATE 1c (S165): the game's own confirmation box answers what the player clicked ────────────
# CDialog::DoModal was `return -1` and EndDialog a no-op, so RDialog::RMessageBox always reported
# the same answer -- and CMainFrame::OnBye reads `rv<2` as "quit", so the game left a campaign
# without ever showing the Save/Yes/Cancel it was written to show. There is no headless trigger for
# a modal (OnBye needs the system box; the bad-weather prompt needs a campaign day whose weather
# says so), so BOB_TEST_MODAL fires the real RMessageBox from the idle loop and prints its return.
# Assert on the ANSWER, per button: three distinct codes is the only thing that proves a loop ran.
echo "### GATE 1c: modal message box (Save=0 Yes=1 Cancel=2)"
modal_ok=1
for bx in "390 Save 0" "518 Yes 1" "647 Cancel 2"; do
  set -- $bx
  got=$(cd "$GD" && timeout -k 5 200 env BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
        SDL_VIDEODRIVER=dummy BOB_TEST_MODAL=60 BOB_TEST_MODAL_EXIT=1 \
        BOB_CLICKXY="100,$1,411;200,$1,411;400,$1,411" "$BOB" 2>&1 |
        grep -a "RMessageBox returned" | sed "s/.*returned \\([0-9-]*\\).*/\\1/")
  if [ "$got" = "$3" ]; then echo "  $2 -> $got"; else echo "  $2 -> $got (EXPECTED $3)"; modal_ok=0; fi
done
[ $modal_ok -eq 1 ] && echo "  modal: PASS" || echo "  modal: FAIL"

echo "### GATE 2: safe default (BOB_NO_RUN)"
timeout -k 5 120 env BOB_NO_RUN=1 "$BOB" >/dev/null 2>&1; echo "  default exit=$?"

echo "### GATE 3: phase select dummy vs real GL"
timeout -k 5 300 env $E BOB_AUTOCLICK=1,1,#1000:1 BOB_SHOT=520 \
  BOB_SHOT_PATH="$OUT/gl_dummy.ppm" "$BOB" >/dev/null 2>&1; echo "  dummy exit=$?"
timeout -k 5 300 env DISPLAY=:0 BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
  BOB_AUTOCLICK=1,1,#1000:1 BOB_SHOT=520 BOB_SHOT_PATH="$OUT/gl_real.ppm" "$BOB" >/dev/null 2>&1
echo "  realGL exit=$?"
cmp -s "$OUT/gl_dummy.ppm" "$OUT/gl_real.ppm" && echo "  dummy==GL BYTE-IDENTICAL" || echo "  dummy!=GL DIFFERS"

echo "### GATE 4: flight frame-150 (real GL)"
rm -f "$OUT/flight.ppm"
timeout -k 5 300 env DISPLAY=:0 BOB_BOOT_FRONTEND=1 BOB_DUMP_FRAME=150 \
  BOB_DUMP_PATH="$OUT/flight.ppm" BOB_EXIT_AFTER_DUMP=1 "$BOB" >/dev/null 2>&1
echo "  flight exit=$?"
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

# ── GATE 4b (S173v): terrain tiles must not come out black ─────────────────────────────────────
# BOB-PO-2 was a D3D->GL viewport origin that was never flipped: correct for a full-size viewport
# and wrong for every smaller one, so the landscape tile compositor read its scratch target's
# top-left corner while GL had rendered into the bottom-left, and 64x64/128x128 tiles uploaded
# black. It survived nine eliminated mechanisms precisely because nothing asserted on it. Assert on
# the NUMBER, not the picture: BOB_TRACE_TEXBLACK counts quads drawn with an all-black texture, and
# the correct answer is zero. Needs real GL (the tiles are composited in an FBO).
echo "### GATE 4b: terrain tiles textured (blackTex must be 0)"
tb=$(cd "$GD" && timeout -k 5 400 env DISPLAY=:0 BOB_BOOT_FRONTEND=1 BOB_QM_INDEX=11 \
      BOB_AUTOFLY=view40 BOB_TRACE_TEXBLACK=20000 BOB_DUMP_FRAME=300 \
      BOB_DUMP_PATH=/dev/null BOB_EXIT_AFTER_DUMP=1 "$BOB" 2>&1 |
      grep -a "quads: blackTex=" | tail -1 | sed "s/.*blackTex=\\([0-9]*\\).*/\\1/")
if [ -z "$tb" ]; then echo "  blackTex: NO SAMPLE (run failed?)"
elif [ "$tb" = "0" ]; then echo "  blackTex=0 — PASS"
else echo "  blackTex=$tb — FAIL (expected 0; terrain tiles are uploading black)"; fi

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

HASH_AFTER=$(md5sum "$BOB" | cut -d' ' -f1)
if [ "$HASH_BEFORE" != "$HASH_AFTER" ]; then
  echo "### !!! GATE INVALID: the binary CHANGED during this run"
  echo "###     before=$HASH_BEFORE after=$HASH_AFTER"
  echo "###     The sweep straddled two builds; its captures cannot be read as a code result."
  echo "###     Re-run without rebuilding (treat 'queued' as 'running'). See SP.19 / PORT.md S148."
  echo "### DONE (INVALID)"
  exit 2
fi
echo "### binary unchanged (md5=$HASH_AFTER) — gate valid"
echo "### DONE"
