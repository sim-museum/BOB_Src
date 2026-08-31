#!/usr/bin/env bash
# tools/bob_mp_uijoin.sh -- R7.1: the game's JOIN screen actually lists a hosted session.
#
# WHY THIS EXISTS
# ---------------
# bob_mp_connect.sh proves the game reaches the multiplayer lobby; bob_mp_packet.sh proves two
# processes can discover each other and exchange a packet. Neither proves the thing a player does:
# that the game's own Join list is POPULATED from a real host.
#
# The path is plumbed end to end -- GetSessions (FULLPANE.CPP) -> DPlay::UIGetSessionListUpdate
# (COMMS.CPP) -> lpDP4->EnumSessions(..., EnumSessionsCallback, ...) -> _DPlay.SessionList -- and
# the compat EnumSessions does invoke that callback with the host's session name. Nobody had ever
# looked at the RESULT.
#
# ⚠️ IT ASSERTS ON THE LIST CONTENTS, NOT ON REACHING THE SCREEN. A Join screen renders identically
# whether the list holds one entry or none: that is exactly the "painted but inert" failure S82
# booked for this front end, and a gate keyed on the screen appearing would pass with an empty list.
# BOB_TRACE_SESSIONS (COMMS.CPP) prints the list after enumeration; this gate keys on that.
#
# ⚠️ NEGATIVE CONTROL: CONTROL=1 runs the SAME drive with NO host. The list must then be EMPTY.
# A gate that has never been watched report zero cannot distinguish "the list is populated" from
# "my assertion always fires".
set -u
ROOT="/home/admin/bob"
BOB="${BOB:-$ROOT/build/bob}"
PROBE="${PROBE:-/tmp/dplay_probe}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
OUT="${OUT:-/tmp/bob_mp_uijoin}"
CONTROL="${CONTROL:-0}"
export BOB_DPLAY_PORT="${BOB_DPLAY_PORT:-47624}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
[ -x "$PROBE" ] || { echo "no probe at $PROBE -- run tools/bob_mp_packet.sh first" >&2; exit 2; }

echo "bob R7.1 -- the JOIN screen lists a hosted session  (CONTROL=$CONTROL)"

hostpid=""
if [ "$CONTROL" != "1" ]; then
  "$PROBE" host >"$OUT/host.log" 2>&1 &
  hostpid=$!
  sleep 2
  grep -aq "hosting as pid" "$OUT/host.log" && echo "  host is up" || echo "  host did not report -- continuing anyway"
else
  echo "  NEGATIVE CONTROL: no host started"
fi
trap '[ -n "$hostpid" ] && kill -9 $hostpid 2>/dev/null; wait $hostpid 2>/dev/null' EXIT

log="$OUT/join.log"
# bob_mp_connect.sh's recipe VERBATIM -- it demonstrably reaches the lobby. The first cut guessed
# BOB_AUTOCLICK="1,4" and omitted BOB_DRIVE_C, and UIGetSessionListUpdate never ran at all.
( cd "$GD" && timeout -k 5 -s KILL 180 env \
    BOB_RUN_INIT=1 BOB_DRIVE_C="${BOB_DRIVE_C:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c}" \
    BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_DPLAY=1 BOB_TRACE_SESSIONS=1 \
    BOB_AUTOCLICK="${MPCLICK:-2}" \
    "$BOB" ) >"$log" 2>&1
pkill -x "$(basename "$BOB")" 2>/dev/null

fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }
n=$(grep -a "UIGetSessionListUpdate ->" "$log" | tail -1 | sed -n 's/.*-> \([0-9]*\) session.*/\1/p')
if [ -z "${n:-}" ]; then
  # NOT a control success. "The enumeration never ran" and "the list came back empty" are different
  # facts, and only the second says anything about the host. The first cut let this fall through to
  # the control's pass path, which would have reported a working control for a drive that never
  # reached the code.
  say "the game enumerated sessions at all" "NO -- UIGetSessionListUpdate never ran"
  say "verdict" "INCONCLUSIVE: the drive did not reach the Join path"
  echo "----------------------------------------"
  echo "INCONCLUSIVE (rc=2): nothing was measured -- fix the drive before reading either arm"
  exit 2
else
  say "the game enumerated sessions at all" "yes"
  say "sessions the JOIN list would show" "$n"
  if [ "$CONTROL" = "1" ]; then
    [ "$n" -eq 0 ] || { say "control: list is EMPTY with no host" "NO -- $n listed"; fail=1; }
  else
    [ "$n" -ge 1 ] || { say "list is POPULATED with a host up" "NO -- 0 listed"; fail=1; }
  fi
fi

echo "----------------------------------------"
if [ "$CONTROL" = "1" ]; then
  [ "$fail" -eq 0 ] && { echo "CONTROL OK: with no host the list is empty, so a populated list means something"; exit 0; }
  echo "CONTROL FAILED: the list was not empty without a host -- the assertion is not measuring the host"; exit 1
fi
[ "$fail" -eq 0 ] && { echo "PASS: the game's Join list is populated from a real host"; exit 0; }
echo "FAIL: the Join list is not populated"; exit 1
