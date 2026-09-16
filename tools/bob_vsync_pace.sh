#!/usr/bin/env bash
# GATE: without true vsync the game must not present faster than the display can show.
#
#   PO 2026-09-04: "in bob appImage dogfight the screen flickered baddly".
#
# WHY THIS EXISTS. The port asks for swap interval 1 and falls back to ADAPTIVE (-1) if the driver
# refuses. Adaptive stops synchronising the moment a frame misses the refresh, so it tears exactly
# when the scene gets heavy -- a dogfight -- while looking clean in level flight. Measured on the
# dev box with the interval forced to 0: 334 fps, about SIX swaps per 60 Hz refresh, so several
# tear lines are on screen at once. That is what "flickered badly" looks like.
#
# The fix paces frames in software whenever the achieved interval is not 1. It cannot align a swap
# to scanout (only real vsync does that), so it does not claim to remove tearing -- it removes the
# multiple-swaps-per-refresh case, which is the severe one.
#
# THE CONTROL MATTERS MORE THAN THE FIX HERE. This gate cannot run on a machine whose driver
# refuses interval 1 (the dev box grants it), so the defect is reproduced by FORCING interval 0.
# If the control ever stops running fast, the premise is gone and the gate must say so rather than
# quietly passing.
set -u
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"
BOB=${BOB:-/home/admin/bob/build/bob}
OUT=${OUT:-/tmp/bob_vsync_pace}; mkdir -p "$OUT"
SECS=${SECS:-42}; QM=${QM:-11}          # QM 11 = title 2237 = IDS_MISTYPE_DOGFIGHTING

arm() { # $1 tag, $2 extra env
  ( cd "$GD" && timeout -k 5 -s KILL "$SECS" env DISPLAY=:0 BOB_RUN_INIT=1 \
      BOB_DRIVE_C="$BOB_DRIVE_C" BOB_BOOT_FRONTEND=1 BOB_QM_INDEX=$QM \
      BOB_AUTOFLY=padlock BOB_TRACE_FRAMETIME=1 $2 "$BOB" ) > "$OUT/$1.log" 2>&1
  FPS=$(grep -a "=> .* fps" "$OUT/$1.log" | tail -1 | sed -n 's/.*=> \([0-9.]*\) fps.*/\1/p')
  IVAL=$(grep -a "swap interval ->" "$OUT/$1.log" | tail -1)
  PACED=$(grep -ac "pacing frames in software" "$OUT/$1.log")
  echo "  $1: fps=${FPS:-<none>} paced=$PACED"
  echo "      ${IVAL:-<no swap-interval line>}"
  [ -z "${FPS:-}" ] && { echo "  INCONCLUSIVE: no frametime report in the $1 arm -- it never flew"; exit 2; }
}

echo "vsync / frame-pacing gate (PO: dogfight flicker), ${SECS}s per arm"
arm control "BOB_VSYNC=0 BOB_NO_FPSCAP=1"; CFPS=$FPS; CPACED=$PACED
arm paced   "BOB_VSYNC=0";                 PFPS=$FPS; PPACED=$PACED
arm default "";                            DFPS=$FPS; DPACED=$PACED; DIVAL="$IVAL"
echo "----------------------------------------"
ok=1
# 1. the control must actually reproduce the defect, else this gate proves nothing
if [ "$(echo "${CFPS:-0} > 150" | bc -l)" != "1" ]; then
  echo "FAIL/RETHINK: the control ran at ${CFPS} fps, not the runaway rate the defect needs."
  echo "              Without a reproduced defect this gate cannot show the fix does anything."; ok=0
elif [ "$CPACED" != "0" ]; then
  echo "FAIL: the control paced (BOB_NO_FPSCAP=1 must disable pacing entirely)"; ok=0
else
  echo "PASS  control reproduces it: ${CFPS} fps with no vsync and pacing off"
fi
# 2. the fix must cap it
if [ "$(echo "${PFPS:-999} > 75" | bc -l)" = "1" ]; then
  echo "FAIL: with pacing on, still ${PFPS} fps -- not held to the refresh"; ok=0
elif [ "$PPACED" -lt 1 ]; then
  echo "FAIL: pacing never engaged despite no vsync"; ok=0
else
  echo "PASS  pacing holds it to ${PFPS} fps (control ${CFPS})"
fi
# 3. and it must be inert when vsync works
if echo "$DIVAL" | grep -q "> 1 (vsync)"; then
  if [ "$DPACED" != "0" ]; then
    echo "FAIL: pacing engaged even though the driver granted vsync"; ok=0
  else
    echo "PASS  inert under real vsync: interval 1, no pacing, ${DFPS} fps"
  fi
else
  echo "  NOTE: this driver did not grant interval 1, so the inert-under-vsync arm is untested here."
  echo "        ($DIVAL)"
fi
[ "$ok" = 1 ] && echo "VSYNC/PACING GATE: PASS" || { echo "VSYNC/PACING GATE: FAIL"; exit 1; }
