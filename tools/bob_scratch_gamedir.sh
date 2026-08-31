#!/usr/bin/env bash
# tools/bob_scratch_gamedir.sh -- build a scratch game directory for gate runs.
#
# R17: the suite writes SAVEGAME/dreplay.dat and Package.dat (GATE 5 and GATE 6, measured),
# destroying the player's recordings. S365 added a detect-and-restore guard, which works but
# is a bandage: the run still eats the data and hands it back afterwards, and anything that
# kills the suite mid-flight (the gl-lock timeout, a crash, Ctrl-C) skips the restore.
#
# The clean fix is for gates never to touch the player's directory at all. A code-level
# redirect would mean intercepting File_Man's FIL_SAVEGAMEDIR, whose paths are assembled at
# runtime from resource-named entries -- invasive, and a wrong path there is silent. So do it
# at the filesystem: a directory of SYMLINKS to every entry of the real game dir, except
# SAVEGAME, which is a real copy. The game reads all its assets through the links (no disk
# cost, no drift) and writes its saves into the copy.
#
# S366: THE FARM HAS TO BE AT THE drive_c LEVEL, not the game dir. A first attempt linked
# only the game directory and ran the gate with cwd pointed at it; the game segfaulted with
#   [fileman] missing file 8c00=\Program Files\Rowan Software\Battle Of Britain\landscap\DIR.DIR
# because it builds ABSOLUTE paths from BOB_DRIVE_C plus a fixed sub-path -- the working
# directory redirects nothing. So the scratch tree is a whole drive_c, linked at every level
# down to the game dir, and BOB_DRIVE_C points at it.
#
#   tools/bob_scratch_gamedir.sh <real-drive-c> <scratch-drive-c>
set -u
REAL="${1:?usage: bob_scratch_gamedir.sh <real-drive-c> <scratch-drive-c>}"
SCRATCH="${2:?usage: bob_scratch_gamedir.sh <real-drive-c> <scratch-drive-c>}"
[ -d "$REAL" ] || { echo "no such drive_c: $REAL" >&2; exit 1; }
GAMEREL="Program Files/Rowan Software/Battle Of Britain"

rm -rf "$SCRATCH"; mkdir -p "$SCRATCH" || exit 1
n=0
# Link every level down to the game dir as a REAL directory, everything beside it as a link.
mirror() {   # $1 = real dir, $2 = scratch dir, $3 = child that must stay real ("" = none)
  local rd="$1" sd="$2" keep="$3" e b
  mkdir -p "$sd"
  for e in "$rd"/*; do
    b="$(basename "$e")"
    if [ -n "$keep" ] && [ "$b" = "$keep" ]; then continue; fi
    ln -s "$e" "$sd/$b"; n=$((n+1))
  done
}
mirror "$REAL"                                       "$SCRATCH"                                       "Program Files"
mirror "$REAL/Program Files"                         "$SCRATCH/Program Files"                         "Rowan Software"
mirror "$REAL/Program Files/Rowan Software"          "$SCRATCH/Program Files/Rowan Software"          "Battle Of Britain"
REAL="$REAL/$GAMEREL"; SCRATCH="$SCRATCH/$GAMEREL"
mkdir -p "$SCRATCH"
for e in "$REAL"/*; do
  b="$(basename "$e")"
  case "$b" in
    SAVEGAME) cp -a "$e" "$SCRATCH/$b" ;;          # real copy: this is what gets written
    *)        ln -s "$e" "$SCRATCH/$b"; n=$((n+1)) ;;
  esac
done
# Writable dirs the gates also produce into. VIDEOS takes replay.dat and the .acmi exports, so it
# must be a real directory or those writes follow the link into the player's tree.
# COPY it, do not create it empty. S367 made it an empty mkdir and GATE R1 failed: the directory
# also holds DIR.DIR and Bob.cam, and a Rowan directory without its DIR.DIR is the same missing-
# index failure that segfaulted the first scratch attempt on landscap/DIR.DIR. "Writable" means
# the game writes INTO it, not that it starts empty.
for w in VIDEOS; do
  if [ -e "$REAL/$w" ]; then rm -rf "$SCRATCH/$w"; cp -a "$REAL/$w" "$SCRATCH/$w"; fi
done
[ -d "$SCRATCH/SAVEGAME" ] || { echo "scratch build FAILED: no SAVEGAME copy" >&2; exit 1; }
echo "scratch game dir: $SCRATCH  ($n linked, SAVEGAME copied$([ -d "$SCRATCH/VIDEOS" ] && echo ', VIDEOS real'))"
