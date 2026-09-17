#!/usr/bin/env bash
# GATE: BoB screen parity -- captured front-end screens vs committed references.
#
# WHY THIS EXISTS (S407). BoB had NO parity gate. Its suite captures 14 screens in GATE 1 and reads
# only their EXIT CODES; GATE 3 compares dummy against real GL, never against a reference. So a
# rendering regression in BoB was invisible by construction.
#
# That is not theoretical. MA's identical ETO_CLIPPED change regressed THREE front-end screens
# (title 4290 px, prefs_3d 77, prefs_others 79) and was caught only because MA's parity_2d compares
# against a committed baseline. The same change in BoB could not be judged, so it had to be
# defaulted OFF blind (S405). This gate is what would let it be judged.
#
# BOBFONT (2026-09-16) CORRECTS ONE WORD ABOVE. This header used to say MA's parity_2d "compares
# against gold". It does not, and MA's own script said the same thing about itself until
# CAMPSCREEN-1 S9 checked: port/ref/native/README.md dates every MA oracle to a sprint of THAT PORT,
# and one of them was re-seeded by the repo in 2026-09. Both ports' parity gates -- this one
# included -- are REGRESSION oracles: they answer "did this change?", never "is this right?".
# The real-game captures live in ma/port/reference/wine-gold/ and bob/doc/reference/260915_gold_*.
# Recorded here because the wrong word had already been copied from one port to the other once.
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
OUT="${OUT:-$HOME/bob-gates/bob_parity}"
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

# GOLDPROV-BOB-1 S4 (2026-09-17): EXTENDED 8 -> 14. GATE 1 captured fourteen screens and this
# gate referenced eight, so six were photographed on every run and thrown away -- sim-views,
# quickshots, sideselect, phaseselect, entername, bobfrag. Two of those six (sideselect,
# phaseselect) are exactly the screens the 2026-09-15 gold videos cover and that GOLDVID-BOB-3
# measures captions on, so the port's most active gold work had no deterministic baseline while
# eight config screens did. The recipes are copied verbatim from bob_gates.sh:196-201.
# name | shot idle | extra env   -- the same recipes GATE 1 already drives, so the captures are the
# ones the suite has been taking all along; only the comparison is new.
RECIPES="mainmenu:40:
config-gfx:70:BOB_CONFIGSCREEN=gfx
config-gfx2:70:BOB_CONFIGSCREEN=gfx2
config-control:70:BOB_CONFIGSCREEN=control
config-sound:70:BOB_CONFIGSCREEN=sound
sim-flight:70:BOB_CONFIGSCREEN=flight
sim-game:70:BOB_CONFIGSCREEN=game
sim-mission:70:BOB_CONFIGSCREEN=mission
sim-views:70:BOB_CONFIGSCREEN=views
quickshots:220:BOB_STARTFLYING=click BOB_AUTOCLICK=0
sideselect:250:BOB_AUTOCLICK=1
phaseselect:380:BOB_AUTOCLICK=1,1
entername:520:BOB_AUTOCLICK=1,1,1
bobfrag:120:BOB_BOBFRAG=1"

fail=0; missing=0; n=0; captured=0
echo "BoB screen parity -- captures vs $REF"
while IFS= read -r line; do
    [ -z "$line" ] && continue
    name="${line%%:*}"; rest="${line#*:}"; shot="${rest%%:*}"; xenv="${rest#*:}"
    n=$((n+1))
    # TMPFS-BOB-1 S2: DELETE THE PREVIOUS CAPTURE FIRST. $OUT persists between runs, so until this
    # line existed a run that captured NOTHING -- crash, timeout, missing data dir, a scaffold that
    # never reached the screen -- silently compared the LAST run's file and reported "OK
    # byte-identical". Demonstrated, not theorised: `BOB=/bin/true bash tools/bob_parity.sh` printed
    # "PASS: 14 screen(s) byte-identical" from a binary that cannot render. This gate is the port's
    # only regression oracle, so a green it cannot earn is the worst failure it has.
    # This is GATEHYGIENE-1's own recommendation applied to the gate it was written about: assert
    # that the STEP fired, not only that the outcome matched.
    rm -f "$OUT/$name.ppm"
    ( cd "$GD" && timeout -k 5 240 env $E $xenv BOB_SHOT="$shot" \
        BOB_SHOT_PATH="$OUT/$name.ppm" "$BOB" ) >"$OUT/$name.out" 2>&1
    if [ ! -s "$OUT/$name.ppm" ]; then
        printf '  %-16s NO CAPTURE (this run produced no image -- see %s)\n' "$name" "$OUT/$name.out"
        fail=$((fail+1)); continue
    fi
    captured=$((captured+1))
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
# precondition, stated before the verdict: every screen this run claims to have compared must
# have been photographed BY THIS RUN. Without it "14 byte-identical" can mean "14 files from
# yesterday".
if [ "$captured" -ne "$n" ]; then
    echo "FAIL: only $captured of $n screen(s) were captured by this run -- the game did not render"
    exit 1
fi
if [ "$fail" -eq 0 ] && [ "$missing" -eq 0 ]; then echo "PASS: $n screen(s) byte-identical ($captured captured this run)"; exit 0; fi
[ "$fail" -eq 0 ] && exit 2
echo "FAIL: a screen differs from its reference (captures in $OUT)"; exit 1
