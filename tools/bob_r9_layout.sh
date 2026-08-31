#!/usr/bin/env bash
# R9: the 1920x1080 canvas placement, for BOTH options the PO is choosing between.
#
# WHY THIS EXISTS
#   R9 root-caused the front end painting only its top-left 1024x768 in a 1920x1080 window. Two
#   remedies now exist -- BOB_CENTRE_UI (1:1, centred) and BOB_SCALE_UI (uniform scale, letterboxed)
#   -- and both were verified ONCE, by hand, in the session that built them. Nothing in the suite
#   would notice if either regressed, and the S342 bug (placement computed from the PREVIOUS frame,
#   so the first frame after a mode change painted wrong) is exactly the kind that comes back.
#
#   Asserts the LIT REGION of a captured frame, using BOB_SHOT2D's own glReadPixels -- the same
#   instrument that found R9 -- rather than trusting the [centre]/[scale] log lines, which report
#   what the code INTENDED. A log line and a pixel disagreeing is the whole point of checking.
#
# Usage:  tools/bob_r9_layout.sh          (run under gl-lock; needs a real GL context)
set -u
BOB="${BOB:-/home/admin/bob/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_r9_layout}"; mkdir -p "$OUT"
TMO="${TMO:-120}"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
fail=0
say(){ printf '  %-34s %s\n' "$1" "$2"; }
bad(){ say "$1" "$2"; fail=1; }

# $1 = label, $2 = extra env, $3..$6 = expected x0 y0 x1 y1 of the lit region
arm() {
  local label="$1" envv="$2" ex0="$3" ey0="$4" ex1="$5" ey1="$6"
  rm -f "$OUT/$label".*.ppm
  ( cd "$GD" && timeout -k 5 -s KILL "$TMO" env BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
      BOB_FORCE_MODE=1920x1080 BOB_SHOT2D_EVERY=1 BOB_SHOT2D_PATH="$OUT/$label" $envv \
      "$BOB" ) > "$OUT/$label.log" 2>&1
  pkill -x "$(basename "$BOB")" 2>/dev/null
  local ppm; ppm=$(ls "$OUT/$label".*.ppm 2>/dev/null | head -1)
  if [ -z "$ppm" ]; then bad "$label: captured a frame" "NO -- see $OUT/$label.log"; return; fi
  # measure the lit bounding box in the capture
  local got; got=$(python3 - "$ppm" <<'PY'
import sys
f=open(sys.argv[1],'rb'); assert f.readline().strip()==b'P6'
l=f.readline()
while l.startswith(b'#'): l=f.readline()
w,h=map(int,l.split()); f.readline(); d=f.read(w*h*3)
x0,y0,x1,y1=w,h,-1,-1
for y in range(h):
    row=d[y*w*3:(y+1)*w*3]
    for x in range(w):
        if row[x*3] or row[x*3+1] or row[x*3+2]:
            if x<x0: x0=x
            if x>x1: x1=x
            if y<y0: y0=y
            if y>y1: y1=y
print("%d %d %d %d"%(x0,y0,x1+1,y1+1))
PY
)
  set -- $got
  if [ "$1" = "$ex0" ] && [ "$2" = "$ey0" ] && [ "$3" = "$ex1" ] && [ "$4" = "$ey1" ]; then
    say "$label: lit region" "($1,$2)-($3,$4) as expected"
  else
    bad "$label: lit region" "($1,$2)-($3,$4), expected ($ex0,$ey0)-($ex1,$ey1)"
  fi
}

echo "bob R9 -- 1920x1080 canvas placement, both options"
# S369: centring is now the DEFAULT, so the default arm asserts the CENTRED region. The old
# expectation (top-left) was the defect, and this gate correctly went red when the default moved
# -- which is the whole reason it exists. It is updated because the change was deliberate, not to
# make a failure go away, and the top-left placement keeps an arm of its own below so the revert
# path stays covered rather than becoming untested the moment it stopped being the default.
arm default ""                        448 156 1472  924
# The revert: BOB_NO_CENTRE_UI restores the original top-left placement.
arm topleft "BOB_NO_CENTRE_UI=1"        0   0 1024  768
# Centre: same 1024x768 canvas, centred. (1920-1024)/2=448, (1080-768)/2=156.
arm centre  "BOB_CENTRE_UI=1"  448 156 1472  924
# Scale: uniform x1.406 to 1440x1080, letterboxed 240 px each side. Aspect preserved, not stretched.
arm scale   "BOB_SCALE_UI=1"   240   0 1680 1080
echo "----------------------------------------"
[ "$fail" -eq 0 ] && { echo "PASS: all three placements are pixel-correct"; exit 0; }
echo "FAIL"; exit 1
