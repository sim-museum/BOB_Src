#!/usr/bin/env bash
# tools/bob_mp_two_instance.sh -- MP-5: two real bob instances, host + client, through to 3D.
#
# The MP-5 sprints drove this by hand from the shell and nothing survived a reboot, so the one
# recipe that reached "both sides in 3D" had to be reconstructed from the backlog each time. It
# lives here now.
#
#   host   BOB_AUTOCLICK="2,1,1[,1]"   Multi-Player -> Create Game -> Continue [-> Fly]
#   client BOB_AUTOCLICK="2,2,1,1"     Multi-Player -> Join -> Select -> Continue
#          BOB_SDL_CLICK_MS="...,156,747"   the session ROW, scheduled in MILLISECONDS because the
#          pump counter stops advancing exactly where the client blocks (MP-5 cont. 9).
#
# ASSERTS on the client reaching 3D without the random-list timeout, which is MP-5's open defect:
#   ⛔ "Timed out (SIP)"  = the client never received the host's 114-byte random list.
set -u
. "$(cd "$(dirname "$0")" && pwd)/bob_safe_kill.sh"
bob_snapshot_pids
ROOT="/home/admin/bob"
BOB="${BOB:-$ROOT/build/bob}"
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
DC="${BOB_DRIVE_C:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c}"
OUT="${OUT:-/home/admin/Documents/260912/logs/bob_mp2}"
SECS="${SECS:-150}"
CLIENT_DELAY="${CLIENT_DELAY:-25}"
HOST_CLICKS="${HOST_CLICKS:-2,1,1}"
# The host's Fly is menu item 1 on the Ready Room (artnum 27918) at (121,747). Its autoclick steps fire
# as each screen appears, so a 4th step flies within seconds -- long before a client that is still
# working through Join/Select/Continue, and the FlyNow broadcast the client waits for is then already
# past. Schedule the host's Fly in milliseconds instead, AFTER the client is in its own Ready Room.
HOST_FLY_MS="${HOST_FLY_MS:-150000,121,747}"
# MP-5 cont.18 (2026-09-13): LATEJOIN=1 drives the path cont.17 actually fixed.
# `Joining=TRUE` is set in exactly one place, DPlay::JoinGame (WINMOVE.CPP:3741), reached from
# exactly one condition (FULLPANE.CPP:556):
#       if (DPlay::H2H_Player[0].status == DPlay::CPS_3D)   // "if game in progress then join"
# -- the HOST MUST ALREADY BE FLYING. In the default arm both peers fly together, so the client goes
# through UINetworkSelectFly, which sets Joining=FALSE (COMMS.CPP:795): that is why every run has
# measured Joining=0 and why the joining branch of SendInitPacket has never been exercised.
# This arm puts the host into 3-D FIRST and starts the client afterwards -- "join a game already in
# progress".
#
# ⚠️ It MUST sit after every default above and use its OWN override names. The first version put it
# before SECS/CLIENT_DELAY were defaulted and wrote `${SECS:-420}`, which is a no-op once SECS is
# already 150 -- the arm then ran with the default 25 s client delay, the client started before the
# host was flying, and it measured Joining=0 all over again while looking like it had run.
if [ "${LATEJOIN:-0}" = "1" ]; then
    HOST_FLY_MS="${LATEJOIN_FLY_MS:-70000,121,747}"
    CLIENT_DELAY="${LATEJOIN_DELAY:-140}"
    SECS="${LATEJOIN_SECS:-420}"
    echo "  [latejoin] host flies at ${HOST_FLY_MS%%,*}ms; client starts at +${CLIENT_DELAY}s; ${SECS}s total"
fi
CLIENT_CLICKS="${CLIENT_CLICKS:-2,2,1,1}"
CLIENT_ROW_MS="${CLIENT_ROW_MS:-20000,156,747}"
export BOB_DPLAY_PORT="${BOB_DPLAY_PORT:-47624}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
echo "bob MP-5 two-instance  (host $HOST_CLICKS + fly@$HOST_FLY_MS, client $CLIENT_CLICKS, ${SECS}s)"
( cd "$GD" && timeout -k 5 -s KILL "$SECS" env BOB_RUN_INIT=1 BOB_DRIVE_C="$DC" \
    BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_DPLAY=1 BOB_TRACE_ADDPLAYER=1 BOB_TRACE_IAMIN=1 ${BOB_TRACE_ACMI:+BOB_TRACE_ACMI=1} BOB_AUTOCLICK="$HOST_CLICKS" \
    BOB_SDL_CLICK_MS="$HOST_FLY_MS" \
    "$BOB" ) >"$OUT/host.log" 2>&1 &
hpid=$!
sleep "$CLIENT_DELAY"
( cd "$GD" && timeout -k 5 -s KILL "$((SECS - CLIENT_DELAY))" env BOB_RUN_INIT=1 BOB_DRIVE_C="$DC" \
    BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_DPLAY=1 BOB_TRACE_ADDPLAYER=1 BOB_TRACE_IAMIN=1 ${BOB_TRACE_ACMI:+BOB_TRACE_ACMI=1} BOB_AUTOCLICK="$CLIENT_CLICKS" \
    BOB_SDL_CLICK_MS="$CLIENT_ROW_MS" \
    "$BOB" ) >"$OUT/client.log" 2>&1 &
cpid=$!
wait $hpid 2>/dev/null; wait $cpid 2>/dev/null
bob_kill_new
fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }
for who in host client; do
  l="$OUT/$who.log"
  grep -aq 'InThe3D=1\|InThe3D = 1' "$l" && say "$who enters 3D" "PASS" || { say "$who enters 3D" "FAIL"; fail=1; }
done
if grep -aq 'Timed out (SIP)' "$OUT/client.log"; then say "client clears the random-list wait" "FAIL (SIP timeout)"; fail=1
else say "client clears the random-list wait" "PASS"; fi
printf '  logs: %s/{host,client}.log\n' "$OUT"
[ "$fail" = 0 ] && echo "  MP-5 TWO-INSTANCE: PASS" || echo "  MP-5 TWO-INSTANCE: FAIL"
exit $fail
