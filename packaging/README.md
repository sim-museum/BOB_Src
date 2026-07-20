# Rowan's Battle of Britain — Linux Port: packaging

Adapted from the FreeFalcon Linux port's `packaging/` (`install.sh`, `build-appdir.sh`),
with three BoB-specific realities baked in:

1. **The binary is 32-bit i386**, not x86-64. Every dependency check and bundle step works
   against `/usr/lib/i386-linux-gnu`, and any host that runs the result needs i386
   multiarch enabled. (An i386 AppImage is portable only to such hosts.)
2. **The game data lives in a Wine `drive_c` tree** — the engine wants
   `<drive_c>/Program Files/Rowan Software/Battle Of Britain` as its cwd and
   `BOB_DRIVE_C` pointing at `<drive_c>` (it derives that itself from the cwd's
   `drive_c` ancestor when unset — see `SRC/compat/bob_main.cpp`).
3. **Game data is not redistributable.** Packaging ships the engine and *ingests* a
   user-supplied install, exactly as the FreeFalcon port does.

## Quick install

```bash
cmake -S . -B build -G Ninja && ninja -C build bob
packaging/install.sh --drive-c /path/to/WP/drive_c [--prefix ~/.local]
```

This verifies the build and the data dir, checks the i386 runtime libraries, and installs
a `bob` launcher plus a `.desktop` entry. Then:

```bash
bob                                  # default boot
BOB_FRONTEND=1 BOB_OLE_DRAW=1 bob    # the real front-end / campaign map
BOB_BOOT_FRONTEND=1 bob              # straight into a Quick-Mission cockpit (needs real GL)
```

## ⚠ Unresolved blocker: the port is NOT relocatable yet

**The engine reads files out of the SOURCE CHECKOUT at runtime.** Two paths do this:

- `SRC/compat/bob_dlgtemplate.cpp` — the runtime `.rc` / DLGINIT dialog-template parser
  (control rects + design-time captions) opens `MIG.RC` / `BOB.RC` / `RESOURCE.H`;
- `SRC/RBUTTON/getfile.cpp` — `GetFileNum` opens `SRC/H/F_GRAFIX.G` to resolve art FileNums.

Both resolve their base directory as `getenv("BOB_RC_DIR")`, falling back to the
compile-time define `BOB_SRC_DIR`, which CMake sets to this checkout's absolute
`SRC/` path. So a binary copied to another machine — or an AppImage — silently loses its
dialog layouts and art lookups unless the source tree is present at the same path.

The scripts here **work around** this rather than fix it:

- `install.sh` pins `BOB_RC_DIR` back at the build checkout in the generated launcher.
  The install is therefore only valid while this repo stays put.
- `build-appdir.sh` copies the runtime-read files (`*.RC`, `RESOURCE.H`, `F_GRAFIX.G`)
  into `usr/share/bob/SRC/` inside the AppDir and points `BOB_RC_DIR` there. That makes the
  AppDir self-contained *for the files we currently know about*; it is a workaround, and any
  newly added runtime source read will silently break it.

**The real fix** (not done) is to treat those resources as installable data: have CMake copy
them to a data dir at install time and have the compat layer resolve them through a single
"resource root" that defaults to the install location. Until then, treat "relocatable
package" as unproven — do not claim the AppDir/AppImage works on a clean machine.

## Runtime dependencies (Debian/Ubuntu, i386)

```bash
sudo dpkg --add-architecture i386 && sudo apt-get update
sudo apt-get install libsdl2-2.0-0:i386 libgl1:i386 libopenal1:i386 \
                     libfluidsynth3:i386 libstdc++6:i386
```

Plus a working (32-bit-capable) OpenGL driver. **Music** additionally needs a General MIDI
SoundFont — `fluid-soundfont-gm` provides `/usr/share/sounds/sf2/FluidR3_GM.sf2`, or set
`BOB_SOUNDFONT=/path/to/bank.sf2`. With no SoundFont, no FluidSynth, or no audio device the
music path degrades silently to no music (`BOB_NOMUSIC` forces that).

## Relocatable AppDir

```bash
packaging/build-appdir.sh                     # -> ./BattleOfBritain.AppDir (ldd-verified)
BOB_DRIVE_C=/path/to/WP/drive_c ./BattleOfBritain.AppDir/AppRun
appimagetool ./BattleOfBritain.AppDir         # single-file .AppImage (needs appimagetool)
```

`AppRun` sets `LD_LIBRARY_PATH` to the bundled libs (no `patchelf` needed) and bundles the
multimedia stack (SDL2, OpenAL, FluidSynth + their private deps), leaving the host to
provide the GL driver, X/Wayland and glibc — the standard AppImage host/bundle split, same
deny-list as the FreeFalcon script.

**Verified locally:** the bundle assembles (30 i386 libraries), `ldd` resolves every entry
against the bundle, and `AppRun` with no game data exits 0. **Not verified:** a run on a
second machine, or an actual `.AppImage` (no `appimagetool` on this box) — and see the
relocatability blocker above, which makes cross-machine portability *structurally* false
today, not merely untested.
