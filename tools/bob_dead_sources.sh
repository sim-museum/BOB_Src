#!/usr/bin/env bash
# tools/bob_dead_sources.sh — list .CPP/.cpp files under SRC/ that nothing compiles.
#
# WHY (S386, 2026-08-31). ma has 32 case-duplicate groups / 50 uncompiled files, and reading one of
# them cost a whole PO-78 sprint (port/dead_sources.sh there). bob's trap is a different shape but
# the same class: this port replaced whole original subsystems with the SDL/GL compat layer and left
# the originals in the tree. SRC/HARDWARE/WIN3D.CPP is 428 KB of Direct3D wrapper that is in no
# build file, is included by nothing (_HARD.CPP:78 has `//#include "..\HARDWARE\Win3d.cpp"`,
# commented out), and contributes ZERO symbols to the binary -- `nm -C build/bob | grep direct_3d`
# is empty against 15041 symbols. It still answers every grep.
#
# That matters for cross-port work especially: ma's PO-88 (texture quality can be lowered but never
# raised) lives in ma's LIVE Win3d.cpp and in bob's DEAD WIN3D.CPP. Grepping bob would have said
# "bob has it too" -- and the correct answer is that bob does not build that code at all.
#
# A file counts as compiled if it is named in build/build.ninja, or #included by a file that is
# (the unity builds -- _HARD.CPP, 3DCODE.CPP and friends pull sources in by #include).
set -u
cd "$(dirname "$0")/.." || exit 1
NINJA=build/build.ninja
[ -f "$NINJA" ] || { echo "no $NINJA -- configure the build first (it is the oracle here)"; exit 2; }

live=$(mktemp); pend=$(mktemp)
grep -ao '[A-Za-z0-9_./-]*\.[Cc][Pp][Pp]' "$NINJA" | xargs -r -n1 basename | sort -u > "$live"
# transitive closure: anything #included by a live file is also live
for _ in 1 2 3 4 5; do
    : > "$pend"
    while read -r b; do
        f=$(find SRC -name "$b" -type f | head -1); [ -n "$f" ] || continue
        # The unity includes use WINDOWS path separators -- `#include "..\MFC\MIG.CPP"`. The first
        # cut of this split on '/' and '"' only, so every backslash path stayed whole, matched no
        # file on disk, and its target was reported DEAD. It called MIG.CPP dead, which is included
        # by 3DCODE.CPP and plainly compiled. Split on BOTH separators.
        grep -ao '#include[[:space:]]*"[^"]*\.[Cc][Pp][Pp]"' "$f" 2>/dev/null |
            sed 's/.*"//; s/.*[\\/]//; s/"$//' >> "$pend"
    done < "$live"
    sort -u "$pend" "$live" -o "$pend".all 2>/dev/null
    if cmp -s "$pend".all "$live"; then break; fi
    mv "$pend".all "$live"
done

dead=0; twins=0
while read -r f; do
    b=$(basename "$f")
    if ! grep -qxF "$b" "$live"; then
        # A dead file with a SAME-NAME live twin is the dangerous kind: SRC/MFC/MIG.CPP is dead
        # while SRC/MFC/MIG.cpp is compiled (_MFC.CPP:82 includes "./MIG.cpp"), so a grep lands on
        # frozen import-era code that looks current. A dead file with NO twin is a whole subsystem
        # this port replaced -- still a trap for cross-porting, but it cannot be confused for its
        # own live version.
        t=$(find SRC -iname "$b" -type f | wc -l)
        if [ "$t" -gt 1 ]; then twins=$((twins+1)); mark="DEAD+twin"; else mark="DEAD     "; fi
        printf '  %s  %-46s %8d bytes\n' "$mark" "$f" "$(stat -c%s "$f")"
        dead=$((dead+1))
    fi
done < <(find SRC -name "*.[cC][pP][pP]" -type f | sort)
tot=$(find SRC -name "*.[cC][pP][pP]" -type f | wc -l)
rm -f "$live" "$pend" "$pend".all
echo "----------------------------------------"
echo "$dead of $tot source file(s) are not compiled."
echo "  $twins of them have a SAME-NAME compiled twin -- a grep on those reads frozen import-era code."
echo "They match every grep over SRC/. Do not read them as current, and do not cross-port from them."
