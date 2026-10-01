#!/bin/bash
# FUNC-SWEEP-BOB: tap EVERY reachable live binding once during one Quick Mission flight (BOB_DIK_MS,
# list from tools/bob_keysweep_gen.py; flight-ending keys held back) and report crash / which tap was
# the last to fire. Runs on the disposable ~/bob-test drive_c. Needs a real GL display (use gl-lock).
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DC="${DC:-$HOME/bob-test/drive_c}"; OUT="${OUT:-$HOME/bob-gates/keysweep}"; mkdir -p "$OUT"
GD="$DC/Program Files/Rowan Software/Battle Of Britain"
python3 "$ROOT/tools/bob_keysweep_gen.py" "$ROOT/doc/keymap/live_keytable_260929.txt" "$ROOT/SRC/H/KEYMAPS.H" \
        "${START_MS:-15000}" "${STEP_MS:-600}" "$OUT/dik.map" > "$OUT/dik.txt"
LAST=$(tail -n 1 "$OUT/dik.map" | cut -f1); SECS=$(( LAST / 1000 + 60 ))
echo "taps=$(wc -l < "$OUT/dik.map") last@${LAST}ms timeout=${SECS}s"
( cd "$GD" && BOB_RUN_INIT=1 BOB_DRIVE_C="$DC" BOB_BOOT_FRONTEND=1 BOB_NO_EXIT_PREFSAVE=1 BOB_DIK_MS="@$OUT/dik.txt" \
      timeout -k 5 -s INT "$SECS" "$ROOT/build/bob" > "$OUT/run.log" 2>&1 ); rc=$?
crash=$(grep -acE '=== CRASH|Segmentation fault|SIGSEGV|Aborted' "$OUT/run.log")
taps=$(grep -ac '\[dikms\] tap' "$OUT/run.log"); last=$(grep -a '\[dikms\] tap' "$OUT/run.log" | tail -n 1)
echo "rc=$rc crash=$crash taps-fired=$taps"; echo "last: $last"
if [ -n "$last" ]; then n=$(sed -E 's/.*tap ([0-9]+) .*/\1/' <<<"$last"); sed -n "$((n+1))p" "$OUT/dik.map" | sed 's/^/last binding: /'; fi
