#!/usr/bin/env bash
set -u
ROOT=/home/admin/bob; OUT=$HOME/bob-gates/qmside
export BOB_SCRATCH_DIR=/tmp/bob_scratch_qm_$$
. "$ROOT/tools/bob_use_scratch.sh"
( cd "$GD" && timeout -k 5 -s KILL 150 env SDL_VIDEODRIVER=dummy \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_OLE=1 BOB_TRACE_CLICK=1 \
    BOB_AUTOCLICK="0,#2056,#2056,#1057:2,#2144" \
    BOB_DUMP_GDI=1 BOB_QM_HARNESS=1 "$ROOT/build/bob" ) > "$OUT/qmside.log" 2>&1
echo "EXIT=$?"
