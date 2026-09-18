#!/usr/bin/env bash
# R20 S6: F12 at 60 s of flight, then read the GL back buffer at every front-end present (the composite).
set -u
ROOT=/home/admin/bob
OUT=$HOME/bob-gates/r20
rm -f "$OUT"/clouda/pres*.ppm
export BOB_SCRATCH_DIR=/tmp/bob_scratch_f12_$$
. "$ROOT/tools/bob_use_scratch.sh"
( cd "$GD" && timeout -k 5 -s KILL 600 env DISPLAY=:0 \
    BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 \
    BOB_AUTOCLICK="1,1,#1000:0,1,1" BOB_MAP_ACCEPTDIR=40 \
    BOB_CAMPAIGN_FLY=90 BOB_CAMPFLY_GO=1 BOB_MAP_TIMER=8 BOB_TRACE_CAMPFLY=1 \
    BOB_SDL_KEY_MS="90000,1073741893,200" BOB_TRACE_CLOUDA=1 BOB_DUMP_GLTEX=300 BOB_GLTEX_ALPHA=1 BOB_AUTOFLY=view1c BOB_TRACE_GREY=1 BOB_TRACE_NOTEX=200000 BOB_SHOT3D_EVERY=300 BOB_SHOT3D_PATH=$HOME/bob-gates/r20/clouda/f BOB_SDL_KEY_MS_LAUNCHREL=1 \
    BOB_SHOT2D_EVERY=1 BOB_SHOT2D_AFTER3D=1 BOB_SHOT2D_PATH="$OUT/clouda/pres" \
    BOB_DUMP_GDI=1 BOB_R20_HARNESS=1 "$ROOT/build/bob" ) > "$OUT/f12_clouda.log" 2>&1
echo "EXIT=$?"
ls -la "$OUT"/padlock | tail -5
