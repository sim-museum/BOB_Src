#!/usr/bin/env bash
# tools/bob_mp_connect.sh -- R7.1: multiplayer gets past its front door.
#
# WHY THIS EXISTS. Multiplayer was never missing game code: the engine's DPlay class and Aggrgtor
# packet layer are compiled in, and the lobby screens render and navigate. What was missing was the
# OBJECT, and the gap was ONE call --
#
#     DPlay::CreateDPlayInterface()  (SRC/COMMS/Comms.cpp:807)
#       -> CoCreateInstance(CLSID_DirectPlay, ..., IID_IDirectPlay4A, &lpDP4)
#
# -- against a compat CoCreateInstance that returned E_NOINTERFACE for every CLSID. So
# CreateDPlayInterface() -> FALSE -> UIMultiPlayInit() -> FALSE -> StartCommsSession() -> FALSE ->
# the not-connected box, exactly as the game intends when DirectPlay is absent.
#
# (Both backlogs first recorded this gap as a missing `DirectPlayCreate`. True, and irrelevant --
# the game never calls it. Corrected in ma S323 / bob R6-S318.)
#
# PASS = clicking Multi-Player (main-menu index 2) creates the DirectPlay object, enumerates a
#        service provider, and REACHES THE LOBBY SCREEN (artnum 27920) rather than the box.
#
# The negative control is real and is run every time: BOB_NO_DPLAY=1 restores E_NOINTERFACE, and
# the gate REQUIRES that arm to stay on the main menu. A gate that cannot fail proves nothing --
# maximized_nav in the sister port passed its own control once, which is why this runs both arms.
set -u
. "$(cd "$(dirname "$0")" && pwd)/bob_safe_kill.sh"   # S392: never kill a bob this gate did not start
bob_snapshot_pids
ROOT="/home/admin/bob"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_mp}"; mkdir -p "$OUT"
TMO="${TMO:-90}"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
pgrep -x bob >/dev/null && { echo "  REFUSING: bob already running"; exit 2; }

run() {  # $1=log  $2=extra env assignment (may be empty)
  ( cd "$GD" && timeout -k 5 -s KILL "$TMO" env ${2:+$2} \
      BOB_RUN_INIT=1 BOB_DRIVE_C="${BOB_DRIVE_C:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c}" \
      BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_DPLAY=1 BOB_AUTOCLICK="2" \
      "$BOB" ) >"$1" 2>&1
  bob_kill_new; return 0
}

echo "bob R7.1 -- multiplayer connectivity: Multi-Player -> DirectPlay -> lobby"
fail=0
say(){ printf '  %-46s %s\n' "$1" "$2"; }
bad(){ say "$1" "$2"; fail=1; }

run "$OUT/on.log" ""
grep -aq "CoCreateInstance(CLSID_DirectPlay)" "$OUT/on.log" \
  && say "DirectPlay object created" "yes" || bad "DirectPlay object created" "NO"
grep -aq "EnumConnections -> 1 provider" "$OUT/on.log" \
  && say "service provider enumerated" "yes" || bad "service provider enumerated" "NO"
# artnum 27920 is the multiplayer lobby; 28937 is the main menu we started on.
grep -aq "artnum=27920" "$OUT/on.log" \
  && say "reached the lobby screen (27920)" "yes" || bad "reached the lobby screen (27920)" "NO"

echo "  --- negative control: BOB_NO_DPLAY=1 ---"
run "$OUT/off.log" "BOB_NO_DPLAY=1"
if grep -aq "artnum=27920" "$OUT/off.log"; then
  bad "control stays on the main menu" "NO -- reached the lobby WITHOUT DirectPlay"
  echo "  => the gate proves nothing: the lobby is reachable without the object under test"
else
  say "control stays on the main menu" "yes (no lobby, as before)"
fi

echo "----------------------------------------"
if [ "$fail" -eq 0 ]; then
  echo "PASS: Multi-Player -> DirectPlay -> lobby, and the control cannot reach it"; exit 0
fi
echo "FAIL -- logs in $OUT"; exit 1
