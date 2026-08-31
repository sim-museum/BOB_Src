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
    else
        echo "  [scratch] BUILD FAILED -- refusing to run against the player's directory." >&2
        echo "            Set BOB_DRIVE_C explicitly if that is really what you want." >&2
        exit 2
    fi
fi
