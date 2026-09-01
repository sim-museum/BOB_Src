# Sourced by any gate that launches the game and then cleans up after itself.
#
# S392: ten gates ended with `pkill -x bob`, which kills EVERY bob on the machine -- including a
# session the PO is playing. That was found the direct way: the PO had bob open, and running
# bob_mp_uijoin.sh would have killed it mid-game. It is the same family as the player-data hazard
# R17/S366 and S373 fixed (a gate reaching outside its own sandbox), and the same family as the
# `pkill -f` self-match already documented -- a process searching by NAME cannot tell its own
# target from someone else's.
#
# Usage:
#   . "$(dirname "$0")/bob_safe_kill.sh"
#   bob_snapshot_pids          # BEFORE launching the game
#   ...run the game...
#   bob_kill_new               # instead of `pkill -x bob`
#
# bob_kill_new kills only PIDs that did NOT exist at snapshot time, so a pre-existing session is
# never touched. If the snapshot was never taken it refuses to kill anything and says so, because
# killing everything is the failure this exists to prevent.
BOB_PRE_PIDS=""
BOB_SNAPSHOT_TAKEN=0

bob_snapshot_pids() {
    BOB_PRE_PIDS=" $(pgrep -x bob 2>/dev/null | tr '\n' ' ') "
    BOB_SNAPSHOT_TAKEN=1
    case "$BOB_PRE_PIDS" in
        *[0-9]*) echo "  [safe-kill] $(echo $BOB_PRE_PIDS | wc -w) bob process(es) already running -- they will NOT be killed" ;;
    esac
}

bob_kill_new() {
    if [ "$BOB_SNAPSHOT_TAKEN" != "1" ]; then
        echo "  [safe-kill] no snapshot taken -- refusing to kill any bob (see tools/bob_safe_kill.sh)" >&2
        return 0
    fi
    local p
    for p in $(pgrep -x bob 2>/dev/null); do
        case "$BOB_PRE_PIDS" in
            *" $p "*) continue ;;                 # pre-existing: leave it alone
        esac
        kill -9 "$p" 2>/dev/null
    done
}
