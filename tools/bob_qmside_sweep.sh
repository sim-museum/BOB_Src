#!/usr/bin/env bash
# QMSIDE-2: for every quick-mission family, cycle the family combo k times, open the Luftwaffe tab, click the
# piloted flag on the German line, and record whether the player moved to side=1. Headless (SDL dummy).
set -u
while pgrep -x bob >/dev/null; do sleep 15; done
ROOT=/home/admin/bob; OUT=$HOME/bob-gates/qmside; mkdir -p $OUT
export BOB_SCRATCH_DIR=/tmp/bob_scratch_qm_$$
. "$ROOT/tools/bob_use_scratch.sh"
for k in 0 1 2 3 4 5 6 7; do
  seq="0"; for ((i=0;i<k;i++)); do seq="$seq,#2056"; done; seq="$seq,#1057:2,#2144"
  ( cd "$GD" && timeout -k 5 -s KILL 150 env SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_OLE=1 BOB_TRACE_CLICK=1 BOB_TRACE_QS=1 \
      BOB_AUTOCLICK="$seq" BOB_QM_HARNESS=1 "$ROOT/build/bob" ) > "$OUT/sweep_$k.log" 2>&1
  fam=$(grep -a '\[qs\] family index=' $OUT/sweep_$k.log | tail -1 | cut -c1-80)
  names=$(grep -a '\[qs\] family\[' $OUT/sweep_$k.log | head -12 | sed 's/.*= quickmissions\[[0-9]*\] //' | tr '\n' '|')
  flag=$(grep -a '\[qs\] piloted flag clicked' $OUT/sweep_$k.log | tail -1 | cut -c1-120)
  echo "k=$k  ${fam:-<no family event>}  flag: ${flag:-<NOT CLICKED>}"
  [ $k = 0 ] && echo "families: $names"
  tot=$(grep -a '\[qs\] .* families total' $OUT/sweep_$k.log | head -1 | sed 's/.*\] \([0-9]*\) families.*/\1/')
  [ -n "${tot:-}" ] && [ $((k+1)) -ge "$tot" ] && break
done
echo QMSIDE-SWEEP-done
