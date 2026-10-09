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

# MP-5 (2026-09-17): SECS must OUTLAST the fly click, or the run ends at the exact moment of the
# step this gate asserts on. The shipped defaults were SECS=150 with HOST_FLY_MS=150000 -- the click
# fired as the timeout expired, the host log contained ZERO occurrences of its coordinates, and the
# gate reported "host enters 3D FAIL / client enters 3D FAIL" while the known SIP defect PASSED.
# That is a harness failure wearing a port failure's clothes. With SECS=270 the same build scores
# 3/3. Auto-extend rather than fail, and say so, so this cannot silently return.
fly_s=$(( ${HOST_FLY_MS%%,*} / 1000 ))
if [ "$SECS" -le "$fly_s" ]; then
    echo "  NOTE: SECS=$SECS does not outlast the fly click at ${fly_s}s -- extending to $((fly_s + 120))s"
    SECS=$((fly_s + 120))
fi
export BOB_DPLAY_PORT="${BOB_DPLAY_PORT:-47624}"
# MP S9: two windows on two monitors. Centred on the same monitor the client covered the host and
# XWayland throttled the occluded host to 1 frame/s -- InitSyncPhase ran once a second and never synced.
HOST_WINPOS="${HOST_WINPOS:-0,0}"
CLIENT_WINPOS="${CLIENT_WINPOS:-1920,0}"
mkdir -p "$OUT"
[ -x "$BOB" ] || { echo "no binary at $BOB" >&2; exit 2; }
echo "bob MP-5 two-instance  (host $HOST_CLICKS + fly@$HOST_FLY_MS, client $CLIENT_CLICKS, ${SECS}s)"
( cd "$GD" && timeout -k 5 -s KILL "$SECS" env BOB_RUN_INIT=1 BOB_DRIVE_C="$DC" \
    ${HOST_ENV:-} BOB_WINPOS="$HOST_WINPOS" BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_DPLAY=1 BOB_TRACE_ADDPLAYER=1 BOB_TRACE_IAMIN=1 ${BOB_TRACE_ACMI:+BOB_TRACE_ACMI=1} BOB_AUTOCLICK="$HOST_CLICKS" \
    BOB_SDL_CLICK_MS="$HOST_FLY_MS" \
    "$BOB" ) >"$OUT/host.log" 2>&1 &
hpid=$!
sleep "$CLIENT_DELAY"
# MP2-BOB-1 (2026-10-08): CLIENT_GD/CLIENT_DC give the guest its OWN game tree. Sharing the host's, the two peers
# write the same savegame/Package.dat/runpack files -- something two PCs never do -- and the host flew a package set
# it had not saved (crash in Persons2::LoadSubPiece on a missing runpack10.bf).
( cd "${CLIENT_GD:-$GD}" && timeout -k 5 -s KILL "$((SECS - CLIENT_DELAY))" env BOB_RUN_INIT=1 BOB_DRIVE_C="${CLIENT_DC:-$DC}" \
    ${CLIENT_ENV:-} BOB_WINPOS="$CLIENT_WINPOS" BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_DPLAY=1 BOB_TRACE_ADDPLAYER=1 BOB_TRACE_IAMIN=1 ${BOB_TRACE_ACMI:+BOB_TRACE_ACMI=1} BOB_AUTOCLICK="$CLIENT_CLICKS" \
    BOB_SDL_CLICK_MS="$CLIENT_ROW_MS" \
    "$BOB" ) >"$OUT/client.log" 2>&1 &
cpid=$!
# E2-5: optional EXTRA players (guests 2..5), headless (SDL dummy video: more windows than the two monitors would cover
# each other, and an occluded window ticks at 1 Hz) and, with GD<k>/DC<k>, each in its own tree so guests don't race
# on the shared savegame/dcomms.dat. CLIENT<k>_CLICKS turns guest k on; CLIENT<k>_DELAY (s after the host, ascending),
# CLIENT<k>_ENV, CLIENT<k>_ROW_MS. (GD2/DC2 kept from the three-player version.)
PLAYERS="host client"; xpids=""; last_delay=$CLIENT_DELAY; nguests=1
for k in 2 3 4 5; do
  eval "clicks=\${CLIENT${k}_CLICKS:-}"; [ -n "$clicks" ] || continue
  eval "dly=\${CLIENT${k}_DELAY:-$((last_delay + 60))}"; eval "xenv=\${CLIENT${k}_ENV:-}"; eval "rowms=\${CLIENT${k}_ROW_MS:-20000,156,747}"
  eval "gdk=\${GD${k}:-$GD}"; eval "dck=\${DC${k}:-$DC}"
  sleep "$((dly - last_delay))"; last_delay=$dly
  ( cd "$gdk" && timeout -k 5 -s KILL "$((SECS - dly))" env SDL_VIDEODRIVER=dummy BOB_RUN_INIT=1 BOB_DRIVE_C="$dck" \
      $xenv BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_TRACE_DPLAY=1 BOB_TRACE_ADDPLAYER=1 BOB_TRACE_IAMIN=1 BOB_AUTOCLICK="$clicks" \
      BOB_SDL_CLICK_MS="$rowms" "$BOB" ) >"$OUT/client$k.log" 2>&1 &
  xpids="$xpids $!"; PLAYERS="$PLAYERS client$k"; nguests=$((nguests + 1))
done
c2pid="$xpids"
wait $hpid 2>/dev/null; wait $cpid 2>/dev/null; for x in $xpids; do wait $x 2>/dev/null; done
bob_kill_new
fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }
for who in $PLAYERS; do
  l="$OUT/$who.log"
  grep -aq 'InThe3D=1\|InThe3D = 1' "$l" && say "$who enters 3D" "PASS" || { say "$who enters 3D" "FAIL"; fail=1; }
done
if grep -aq 'Timed out (SIP)' "$OUT/client.log"; then say "client clears the random-list wait" "FAIL (SIP timeout)"; fail=1
else say "client clears the random-list wait" "PASS"; fi
# EPIC M / MP S10 (2026-09-26): "both in 3-D" was the whole assertion, and it passed while the client
# flew the HOST's aeroplane and died 20 s later. Assert the things a player means by "it works".
for who in $PLAYERS; do
  l="$OUT/$who.log"
  if grep -aq '=== CRASH' "$l"; then say "$who survives the flight" "FAIL ($(grep -a -m1 '=== CRASH' "$l"))"; fail=1
  else say "$who survives the flight" "PASS"; fi
  # the seat: the player's own aircraft uid (needs BOB_TRACE_AGG=1 for the [psq] line)
  u=$(grep -a -m1 -oE '\[psq\] player aircraft uid=[0-9]*|\[mpcamp\] 3D: my aircraft uid=[0-9]*' "$l" | grep -o '[0-9]*$')   # quick mission | co-op campaign (E2-5)
  eval "uid_$who=\${u:-none}"
done
if [ "$uid_host" = none ] || [ "$uid_client" = none ]; then say "each player seated in own aircraft" "FAIL (host=$uid_host client=$uid_client; BOB_TRACE_AGG=1?)"; fail=1
elif [ "$uid_host" = "$uid_client" ]; then say "each player seated in own aircraft" "FAIL (both uid $uid_host)"; fail=1
else say "each player seated in own aircraft" "PASS (host uid $uid_host, client uid $uid_client)"; fi
if [ -n "$c2pid" ]; then   # E2-5: more than two players -- every aircraft distinct, and the host sees every guest
  allu="$uid_host $uid_client"; bad=""
  for who in $PLAYERS; do case "$who" in client[2-9]) eval "u=\$uid_$who"; allu="$allu $u"; [ "$u" = none ] && bad="$bad $who";; esac; done
  dups=$(echo $allu | tr ' ' '\n' | grep -v none | sort | uniq -d | tr '\n' ' ')
  if [ -n "$bad$dups" ]; then say "every player in own aircraft" "FAIL (uids: $allu)"; fail=1
  else say "every player in own aircraft" "PASS (uids: $allu)"; fi
  seen=$(grep -aoE '\[addplayer\] slot=[0-9]+ .*FOUND' "$OUT/host.log" | grep -aoE 'slot=[0-9]+' | sort -u | wc -l)
  [ "$seen" -ge "$nguests" ] && say "host makes all $nguests guests visible" "PASS ($seen guest slots)" || { say "host makes all $nguests guests visible" "FAIL ($seen)"; fail=1; }
fi
if grep -aq '\[hist\] slot .* no aircraft' "$OUT/client.log" "$OUT/host.log"; then say "every slot has an aircraft" "FAIL"; fail=1
else say "every slot has an aircraft" "PASS"; fi
# 2026-10-06: judge sync whenever the aggregator trace is in the log -- it used to run only when BOB_TRACE_AGG was in
# the GATE's environment, so a trace switched on through HOST_ENV/CLIENT_ENV skipped the check, and a co-op campaign
# run whose peers never synced (both stuck after the aircraft-id exchange, csync=0 to the end) reported PASS.
for who in host client; do
  if [ -n "${BOB_TRACE_AGG:-}" ] || grep -aq '^\[agg\]' "$OUT/$who.log"; then
    grep -aq 'synched=1 csync=1' "$OUT/$who.log" && say "$who comms-synced (csync=1)" "PASS" || { say "$who comms-synced (csync=1)" "FAIL"; fail=1; }
  fi
done
# E2-5 (2026-10-06): "the guest sees the host move" -- with BOB_TRACE_MPPOS=1 in HOST_ENV and CLIENT_ENV. For each
# player, the path its OWN peer logged is compared with the path the other peer logged for it, over the same wall
# seconds: the other side must see at least half of it. A player that moved under 100 m gives nothing to see and is
# skipped (a campaign recipe that reaches 3-D seconds before the end). The peers must also place every player within
# 50 m of each other in the same second. Measured, quick-mission co-op: seen = own path (14.4 km), 5.3-11.6 m apart.
if grep -aq '^\[mppos\]' "$OUT/host.log" && grep -aq '^\[mppos\]' "$OUT/client.log"; then
  python3 - "$OUT" > "$OUT/mppos_check.txt" <<'PYEOF'
import re, math, sys, collections
D, own, dead = {}, {}, {}
for w in ("host", "client"):
    d = collections.defaultdict(dict)
    for l in open(sys.argv[1] + "/" + w + ".log", errors="replace"):
        m = re.match(r"\[mppos\] t=(\d+) frame=\d+ slot=(\d+)(\(me\))? uid=\S+ pos=\((-?\d+),(-?\d+),(-?\d+)\).*? dead=(\d+)", l)
        if m:
            d[int(m.group(2))][int(m.group(1))] = tuple(int(x) for x in m.group(4, 5, 6))
            dead[(w, int(m.group(2)), int(m.group(1)))] = int(m.group(7))
            if m.group(3): own[int(m.group(2))] = w
    D[w] = d
def path(ser, ts): return sum(math.dist(ser[a], ser[b]) for a, b in zip(ts, ts[1:])) / 100
res, worst, nskip = [], 0.0, 0
for slot, owner in own.items():
    viewer = "client" if owner == "host" else "host"
    ts = sorted(set(D[owner].get(slot, {})) & set(D[viewer].get(slot, {})))
    # the second the owner's own Status.deadtime changes (shot down) and the one after are left out: the remote copy
    # of an aircraft being killed was measured once at the world origin for one sample (campaign run, 10-06)
    flips = {t for t in ts if dead.get((owner, slot, t)) != dead.get((owner, slot, t - 1), dead.get((owner, slot, t)))}
    skipped = {t for t in ts if t in flips or t - 1 in flips}
    nskip += len(skipped)
    for t in ts:
        if t not in skipped: worst = max(worst, math.dist(D[owner][slot][t], D[viewer][slot][t]) / 100)
    p_own, p_seen = path(D[owner][slot], ts), path(D[viewer][slot], ts)
    verdict = "SKIP" if p_own < 100 else ("PASS" if p_seen >= 0.5 * p_own else "FAIL")
    res.append("%s %s's aircraft: own path %.0f m, %s saw %.0f m over %d s" % (verdict, owner, p_own, viewer, p_seen, len(ts)))
for r in res: print(r)
if nskip: print("NOTE %d seconds at a shoot-down left out of the agreement check" % nskip)
print("WORST %.1f" % worst)
PYEOF
  while read -r v rest; do
    case "$v" in
      PASS) say "other side sees it move" "PASS ($rest)";;
      SKIP) say "other side sees it move" "SKIP ($rest)";;
      NOTE) say "  (position checks)" "$rest";;
      FAIL) say "other side sees it move" "FAIL ($rest)"; fail=1;;
      WORST) awk -v w="$rest" 'BEGIN{exit !(w <= 50)}' && say "peers agree on every aircraft (<= 50 m)" "PASS (worst $rest m)" \
               || { say "peers agree on every aircraft (<= 50 m)" "FAIL (worst $rest m)"; fail=1; };;
    esac
  done < "$OUT/mppos_check.txt"
fi
printf '  logs: %s/{host,client}.log\n' "$OUT"
[ "$fail" = 0 ] && echo "  MP-5 TWO-INSTANCE: PASS" || echo "  MP-5 TWO-INSTANCE: FAIL"
exit $fail
