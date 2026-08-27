#!/usr/bin/env bash
# tools/bob_blob_bisect.sh -- PO-73: name the draw that renders the grey ellipse.
#
# S306 eliminated the "texture never uploaded" hypothesis: the blob's centre is covered only by
# draws that ARE textured with successfully uploaded textures. So the culprit is one of them, and
# the question becomes which. S303b's recipe (BOB_BLOB_TEX=<glTex>, hilite, photograph) is right
# but linear -- ~20 candidates, one real-GL flight each.
#
# This bisects instead: drop HALF the candidate set per run and ask whether the ellipse is gone.
# log2(30) ~ 5 runs.
#
# ⚠️ IT SKIPS DRAWS, IT DOES NOT HILITE THEM. S303b's hilite does glDisable(GL_TEXTURE_2D) plus
# a flat colour, which discards the texture's ALPHA -- so a transparent cloud billboard paints as
# an OPAQUE magenta rectangle and every alpha draw over the region scores 100%. That is why "the
# whole sky turned magenta", and a bisect on that signal just follows the largest transparent quad
# (measured: hiliting {11} alone filled the box 100%). Skipping leaves every other draw untouched.
#
# ⚠️ THE DECISION IS MEASURED, NOT EYEBALLED. The ellipse is the only thing in this window
# darker than luminance 180 -- sky and cloud are all above it (verified on the S306 baseline). So
# the detector counts dark pixels; the culprit is the draw whose removal collapses that count.
#
# ⚠️ NEGATIVE CONTROL: the baseline arm skips nothing and MUST show a large dark-pixel count.
# A small one means the detector cannot see the ellipse and no verdict below it would mean
# anything.
#
# ⚠️ Needs real GL and an UNLOCKED session (a locked one hangs at the title -- see the port notes).
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_blob_bisect}"; mkdir -p "$OUT"
FRAME="${FRAME:-150}"
# a window generously around the ellipse at 800x600, measured from the S306 baseline capture
# (ellipse bbox x 18..133, y 62..81). DARK is the luminance below which only the ellipse falls.
BOX="${BOX:-0,45,170,100}"
DARK="${DARK:-180}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
pgrep -x bob >/dev/null && { echo "REFUSING: bob already running (pid $(pgrep -x bob|tr '\n' ' '))"; exit 2; }

# one flight; $1 = tag, $2 = comma-separated glTex set ("" = baseline, skip nothing)
fly() {
  local tag="$1" set="$2"
  ( cd "$GD" && timeout -k 5 300 env DISPLAY=:0 BOB_BOOT_FRONTEND=1 BOB_TRACE_BLOB=1 \
      ${set:+BOB_BLOB_TEX="$set"} \
      BOB_DUMP_FRAME="$FRAME" BOB_DUMP_PATH="$OUT/$tag.ppm" BOB_EXIT_AFTER_DUMP=1 \
      "$BOB" ) >"$OUT/$tag.log" 2>&1
  [ -s "$OUT/$tag.ppm" ] || { echo "  $tag: NO FRAME -- run failed, see $OUT/$tag.log"; return 1; }
}

# dark-pixel count inside the window -- the ellipse, and nothing else
score() {
  python3 - "$OUT/$1.ppm" "$BOX" "$DARK" <<'PYX'
from PIL import Image; import sys
im=Image.open(sys.argv[1]).convert('RGB'); px=im.load()
x0,y0,x1,y1=[int(v) for v in sys.argv[2].split(',')]; thr=int(sys.argv[3])
n=0
for y in range(y0,y1+1):
    for x in range(x0,x1+1):
        r,g,b=px[x,y]
        if (r*299+g*587+b*114)//1000 < thr: n+=1
print(n)
PYX
}

echo "bob blob bisect — which uploaded texture draws the PO-73 ellipse?"
echo "  ellipse box $BOX at frame $FRAME"

fly baseline "" || exit 2
base=$(score baseline)
echo "  NEGATIVE CONTROL (skip nothing): $base dark pixels = the ellipse"
[ "$base" -ge 400 ] || {
  echo "  CONTROL FAILED: only $base dark pixels — the detector cannot see the ellipse,"
  echo "  so no verdict below it would mean anything. Re-measure BOX/DARK against the frame."
  exit 1; }
GONE=$(( base / 4 ))   # "the ellipse is gone" = under a quarter of the baseline count

CANDS=$(grep -ao "glTex=[0-9]*" "$OUT/baseline.log" | grep -a -A0 . | sed 's/glTex=//' | sort -un | tr '\n' ' ')
CANDS=$(grep -a "^\[blobtex\]" "$OUT/baseline.log" | sed 's/.*glTex=\([0-9]*\).*/\1/' | sort -un | tr '\n' ',' | sed 's/,$//')
echo "  candidates over the blob: $CANDS"
[ -n "$CANDS" ] || { echo "  none logged — nothing to bisect"; exit 2; }

IFS=',' read -ra SET <<< "$CANDS"
round=0
while [ "${#SET[@]}" -gt 1 ]; do
  round=$((round+1))
  half=$(( ${#SET[@]} / 2 ))
  left=$(IFS=,; echo "${SET[*]:0:$half}")
  fly "r${round}" "$left" || exit 2
  sc=$(score "r$round")
  printf '  round %d: skip {%s} -> %s dark px (baseline %s)' "$round" "$left" "$sc" "$base"
  if [ "$sc" -lt "$GONE" ]; then
    echo "  => ELLIPSE GONE: it is in this half"
    SET=("${SET[@]:0:$half}")
  else
    echo "  => ellipse still there"
    SET=("${SET[@]:$half}")
  fi
done
echo "----------------------------------------"
echo "CULPRIT: glTex=${SET[0]}"
grep -a "glTex=${SET[0]} " "$OUT/baseline.log" | head -2
echo "confirm with: BOB_BLOB_SKIP=${SET[0]} and look at the frame"
