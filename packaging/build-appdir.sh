#!/bin/bash
# Rowan's Battle of Britain — Linux Port: assemble a relocatable AppDir.
# Adapted from the FreeFalcon port's packaging/build-appdir.sh, with one large
# difference: THIS BINARY IS 32-BIT i386. The bundle must therefore collect i386
# libraries (/usr/lib/i386-linux-gnu) and the host must provide a 32-bit GL driver
# stack — an AppImage of an i386 binary is only portable to hosts with i386 multiarch
# enabled. No patchelf/appimagetool needed here: AppRun sets LD_LIBRARY_PATH.
#
# Usage: packaging/build-appdir.sh [outdir]   (default: ./BattleOfBritain.AppDir)
#
# NOTE — NOT YET TRULY RELOCATABLE: the engine reads BOB.RC / F_GRAFIX.G from the
# source tree at runtime (compiled-in BOB_SRC_DIR; runtime override $BOB_RC_DIR). This
# script copies the needed source-tree resource files into the AppDir and points
# BOB_RC_DIR at them, which
# works on this box but is a workaround, not a fix. See packaging/README.md.
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$REPO/build/bob"
APPDIR="${1:-$REPO/BattleOfBritain.AppDir}"
[ -x "$BIN" ] || { echo "build the engine first: cmake -S . -B build -G Ninja && ninja -C build bob" >&2; exit 1; }

case "$(file -b "$BIN")" in
  *"ELF 32-bit"*) : ;;
  *) echo "!! $BIN is not a 32-bit ELF — this script assumes the i386 port." >&2 ;;
esac

# Host-provided libraries — MUST NOT be bundled (driver/display/base-system ABI).
# Same deny-list as the FreeFalcon port; the paths differ (i386 multiarch dir) but the
# host/bundle split is identical.
DENY='^(libGL\.|libGLX|libGLdispatch|libEGL|libOpenGL|libdrm|libgbm|libc\.|libm\.|libdl|libpthread|librt|ld-linux|libmvec|libstdc\+\+|libgcc_s|libwayland|libX|libxcb|libXau|libXdmcp|libxkb|libdbus|libsystemd|libapparmor|libudev|libasound|libpulse)'

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" "$APPDIR/usr/share/bob"
cp "$BIN" "$APPDIR/usr/bin/bob"

echo ">> bundling i386 libraries (excluding host-provided)..."
n=0
while read -r name _arrow path _addr; do
  [ -f "${path:-}" ] || continue
  if echo "$name" | grep -qE "$DENY"; then continue; fi
  cp -L "$path" "$APPDIR/usr/lib/" && n=$((n+1))
done < <(ldd "$BIN" | sed 's/^[[:space:]]*//')
echo ">> bundled $n libraries"

# Runtime-read source resources (the BOB_SRC_DIR wrinkle — see README).
echo ">> copying runtime-read source resources (.rc / RESOURCE.H / F_GRAFIX.G)..."
for f in $(cd "$REPO" && find SRC -maxdepth 3 \( -iname '*.rc' -o -iname 'RESOURCE.H' -o -iname 'F_GRAFIX.G' \) 2>/dev/null); do
  mkdir -p "$APPDIR/usr/share/bob/$(dirname "$f")"
  cp "$REPO/$f" "$APPDIR/usr/share/bob/$f"
done

cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:${LD_LIBRARY_PATH:-}"
# Runtime-read resources shipped alongside the binary (see packaging/README.md).
export BOB_RC_DIR="${BOB_RC_DIR:-$HERE/usr/share/bob/SRC}"
# Game data is NOT bundled (not redistributable). Point BOB_DRIVE_C at your Wine
# drive_c, or just run the AppRun from inside the install directory — the engine
# derives BOB_DRIVE_C from the cwd's drive_c ancestor.
if [ -n "${BOB_DRIVE_C:-}" ] && [ -z "${BOB_NO_CHDIR:-}" ]; then
  cd "$BOB_DRIVE_C/Program Files/Rowan Software/Battle Of Britain" 2>/dev/null || true
fi
exec "$HERE/usr/bin/bob" "$@"
EOF
chmod +x "$APPDIR/AppRun"

cat > "$APPDIR/bob.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Battle of Britain (Linux Port)
Exec=AppRun
Icon=bob
Categories=Game;Simulation;
EOF
: > "$APPDIR/bob.png"   # placeholder; appimagetool requires an icon

echo ">> AppDir ready: $APPDIR"
echo ">> verifying the bundle resolves..."
if LD_LIBRARY_PATH="$APPDIR/usr/lib" ldd "$APPDIR/usr/bin/bob" | grep -E "not found"; then
  echo "!! unresolved libraries above — add them to the bundle"; exit 1
fi
echo ">> all libraries resolve."
echo
echo "Run locally:  BOB_DRIVE_C=/path/to/WP/drive_c $APPDIR/AppRun"
echo "Package:      appimagetool $APPDIR   # i386 host with multiarch required"
