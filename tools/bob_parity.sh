#!/usr/bin/env bash
# GATE: BoB screen parity -- captured front-end screens vs committed references.
#
# WHY THIS EXISTS (S407). BoB had NO parity gate. Its suite captures 14 screens in GATE 1 and reads
# only their EXIT CODES; GATE 3 compares dummy against real GL, never against a reference. So a
# rendering regression in BoB was invisible by construction.
#
# That is not theoretical. MA's identical ETO_CLIPPED change regressed THREE front-end screens
# (title 4290 px, prefs_3d 77, prefs_others 79) and was caught only because MA's parity_2d compares
# against gold. The same change in BoB could not be judged, so it had to be defaulted OFF blind
# (S405). This gate is what would let it be judged.
#
# Headless: SDL_VIDEODRIVER=dummy, no GL, no gl-lock -- it runs while someone is playing.
#
# References live in doc/ref/native/. A MISSING reference is reported, never silently created: a
# reference seeded from an unreviewed capture would enshrine whatever was on screen that day, and a
# gate whose baseline is unexamined is exactly the "green because nobody looked" failure this port
# has booked repeatedly. Seed deliberately with SEED=1 after eyeballing the captures.
set -u
ROOT="/home/admin/bob"
BOB="${BOB:-$ROOT/build/bob}"
REF="${REF:-$ROOT/doc/ref/native}"
OUT="${OUT:-/tmp/bob_parity}"
SEED="${SEED:-0}"
# S413: RUN AGAINST A SCRATCH TREE, not the player's.
#
# The first cut read the real drive_c, which makes the gate NON-HERMETIC: the game loads its
# settings from that directory, so anything that writes there moves the captures. It did. The
# config-control reference was seeded at S407 and by S411 the same binary produced a 210-byte
# difference -- deterministic run to run, unchanged by every clip setting, and the diff was a combo
# VALUE rendering differently. Between those sprints the PO had been playing (their GFX screenshot
# is from this session) and a campaign gate had run against the real tree.
#
# A parity gate whose baseline moves when someone plays the game is worse than no gate: it produces
# red that means nothing, which is how a real regression gets waved through. bob_use_scratch.sh
# (S373) already builds an isolated tree for exactly this.
. "$(cd "$(dirname "$0")" && pwd)/bob_use_scratch.sh"   # sets BOB_DRIVE_C + GD to a scratch tree
GD="${GD:-/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain}"
mkdir -p "$OUT" "$REF"
[ -x "$BOB" ] || { echo "no binary at $BOB"; exit 2; }
E="BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 SDL_VIDEODRIVER=dummy"

# name | shot idle | extra env   -- the same recipes GATE 1 already drives, so the captures are the
# ones the suite has been taking all along; only the comparison is new.
RECIPES="mainmenu:40:
config-gfx:70:BOB_CONFIGSCREEN=gfx
config-gfx2:70:BOB_CONFIGSCREEN=gfx2
config-control:70:BOB_CONFIGSCREEN=control
config-sound:70:BOB_CONFIGSCREEN=sound
sim-flight:70:BOB_CONFIGSCREEN=flight
sim-game:70:BOB_CONFIGSCREEN=game
sim-mission:70:BOB_CONFIGSCREEN=mission"

fail=0; missing=0; n=0
echo "BoB screen parity -- captures vs $REF"
while IFS= read -r line; do
    [ -z "$line" ] && continue
    name="${line%%:*}"; rest="${line#*:}"; shot="${rest%%:*}"; xenv="${rest#*:}"
    n=$((n+1))
    ( cd "$GD" && timeout -k 5 240 env $E $xenv BOB_SHOT="$shot" \
        BOB_SHOT_PATH="$OUT/$name.ppm" "$BOB" ) >"$OUT/$name.out" 2>&1
    if [ ! -s "$OUT/$name.ppm" ]; then
        printf '  %-16s NO CAPTURE\n' "$name"; fail=$((fail+1)); continue
    fi
    r="$REF/$name.ppm"
    if [ ! -f "$r" ]; then
        if [ "$SEED" = "1" ]; then cp "$OUT/$name.ppm" "$r"; printf '  %-16s SEEDED reference\n' "$name"
        else printf '  %-16s NO REFERENCE (run with SEED=1 after reviewing %s)\n' "$name" "$OUT/$name.ppm"; missing=$((missing+1)); fi
        continue
    fi
    d=$(cmp -l "$OUT/$name.ppm" "$r" 2>/dev/null | wc -l)
    if [ "$d" = "0" ]; then printf '  %-16s OK byte-identical\n' "$name"
    else printf '  %-16s DIFF (%s bytes differ)\n' "$name" "$d"; fail=$((fail+1)); fi
done <<< "$RECIPES"

echo "----------------------------------------"
[ "$missing" -gt 0 ] && echo "$missing screen(s) have no reference yet -- seed them deliberately, do not auto-accept"
if [ "$fail" -eq 0 ] && [ "$missing" -eq 0 ]; then echo "PASS: $n screen(s) byte-identical"; exit 0; fi
[ "$fail" -eq 0 ] && exit 2
echo "FAIL: a screen differs from its reference (captures in $OUT)"; exit 1
