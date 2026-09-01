# Sourced by any gate that WRITES into the game directory.
#
# S373: running a writing gate standalone used to point at the player's real drive_c, because
# that is what the scripts default to. tools/bob_gates.sh redirects the whole suite to a scratch
# tree (R17/S366), but a gate invoked by hand got no such protection -- and running
# bob_replay_record.sh once, to classify it, overwrote the player's VIDEOS/replay.dat. It was
# restored from a backup made hours earlier, which is luck rather than design.
#
# So the DEFAULT is inverted here: with no BOB_DRIVE_C set, build a scratch tree and use that.
# Touching the player's tree now requires saying so explicitly (BOB_DRIVE_C=<real path>), which
# is the right way round -- the safe thing should be the thing that happens when you type nothing.
#
#   . "$(dirname "$0")/bob_use_scratch.sh"      # sets BOB_DRIVE_C and GD if unset
BOB_REAL_DC="${BOB_REAL_DC:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c}"
if [ -z "${BOB_DRIVE_C:-}" ]; then
    _scr="${BOB_SCRATCH_DIR:-/tmp/bob_scratch_$$}"
    if bash "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/bob_scratch_gamedir.sh" \
            "$BOB_REAL_DC" "$_scr" >/dev/null 2>&1; then
        export BOB_DRIVE_C="$_scr"
        GD="$_scr/Program Files/Rowan Software/Battle Of Britain"
        echo "  [scratch] writing gate -> $_scr (player's tree untouched; set BOB_DRIVE_C to override)"
        # S424: REMOVE IT ON EXIT. This built a tree per invocation and never cleaned up, so every
        # gate that sources it left one behind -- for as long as this file has existed. Running it
        # in a loop during R25's bisection made that visible by helping to exhaust the /tmp quota,
        # which broke every shell command until the PO cleared it. (The bulk turned out to be
        # accumulated CAPTURE output, not these trees -- ~2-3 MB each -- but a gate that leaves
        # litter on every run is still a gate that eventually fills a disk.)
        # ⚠️ This matters beyond tidiness: S378 records that when /tmp last hit its quota a gate's
        # backup came out empty and the restore wrote a 0-byte file over the player's campaign save.
        # BOB_KEEP_SCRATCH=1 keeps the tree when you need to inspect it after a failure.
        if [ -z "${BOB_KEEP_SCRATCH:-}" ]; then
            trap 'rm -rf "'"$_scr"'" 2>/dev/null' EXIT
        fi
    else
        echo "  [scratch] BUILD FAILED -- refusing to run against the player's directory." >&2
        echo "            Set BOB_DRIVE_C explicitly if that is really what you want." >&2
        exit 2
    fi
fi
