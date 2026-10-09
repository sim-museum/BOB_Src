#!/bin/bash
# MP2-BOB-2 S2: the quick-mission boot flight with table fog on -- the second 3-D path (no crash, fog applies).
set -u
ROOT=/home/admin/bob
OUT=$HOME/bob-gates/r20/s9qm; mkdir -p "$OUT"; rm -f "$OUT"/q.*
export BOB_SCRATCH_DIR=/tmp/bob_scratch_qm_$$
. "$ROOT/tools/bob_use_scratch.sh"
( cd "$GD" && timeout -k 5 -s INT 75 env DISPLAY=:0 BOB_RUN_INIT=1 BOB_BOOT_FRONTEND=1 BOB_TRACE_TABLEFOG=1 \
    BOB_TABLEFOG_AB=1 BOB_SHOT3D_EVERY=1200 BOB_SHOT3D_PATH="$OUT/q" "$ROOT/build/bob" ) > "$OUT/qm.log" 2>&1
echo "EXIT=$?"
