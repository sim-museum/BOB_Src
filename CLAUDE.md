# Rowan's Battle of Britain — Linux Native Port

Goal: get the open-source **Battle of Britain** (Rowan, 2000) running **native on Linux**,
faithfully — every function and every image intact, the way it ran on Windows. The original
is a Win32 / MFC / DirectX 7 (DirectDraw7 + Direct3D7) / DirectInput / DirectSound title.
The port keeps the **game code unedited** and provides a compatibility layer that maps the
Windows/DirectX APIs onto **SDL2 + OpenGL + OpenAL** under `#if FF_LINUX` / `BOB_LINUX`.

## Layout
- `SRC/` — original game sources (LIB3D, 3D, MFC, BFIELDS, …). Avoid editing game logic.
- `SRC/compat/` — the Linux compat layer (the porting work lives here):
  - `bob_video.cpp` — DDraw7/D3D7 → SDL2/OpenGL backend (surfaces, device, RTT, present).
  - `bob_*.cpp` + `compat_*.h` + Win32/MFC/DX header shims.
- `PORT.md` — **the running engineering log** (newest entry on top). Read it first; it holds
  every milestone, fix, and diagnosis with evidence. Append a dated entry per session.
- `build/` — CMake/Ninja build tree.

## Build & run
```
cd build && ninja bob
# run against a real game-data install:
cd "/home/m/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
BOB_RUN_INIT=1 BOB_DRIVE_C="/home/m/sgl/TUE/BattleOfBritain/WP/drive_c" /home/m/bob/build/bob
```
`BOB_BOOT_FRONTEND=1` boots straight into a flying Quick-Mission cockpit (3D bring-up).
Many `BOB_*` env toggles gate diagnostics/captures (frame/texture dumps, traces) — see
`getenv("BOB_` sites and PORT.md. Frame/texture dumps land in `/tmp/*.ppm` (convert via PIL).

## Status (2026-06-12)
The 3D world renders **daylit and stable**: sky, terrain, scenery, aircraft, and a
recognizable cockpit. Reaches an interactive Quick-Mission flight. Input (keyboard→DIK)
works. Remaining work is rendering **fidelity**, not bring-up.

Status update (2026-06-15): **the cockpit is fixed** and now matches the Windows reference
(RAF green, riveted struts, legible gauges, gunsight glass + reticle). Root cause was a
**surface refcount use-after-free**: `GLSurface7` had no reference count, so `SURF_Release`
freed a texture on the first call while `_CreateTextureMap`'s `UpdateMipMaps` AddRef/Release
pair (and `textureTable`) still referenced it — the freed block was reused and its head
overwritten, producing the `w=0x700000` garbage binds. Fixed by adding real COM refcounting
(see top PORT.md entry; before/after frames in `doc/reference/`). We have a Windows reference
now (original `bob.exe` under wine) in `doc/reference/`.

Current focus — remaining **rendering fidelity**: per-stage texture addressing (terrain
over-tiling; `BOB_CLAMP` probe), minor cockpit polish (gunsight sun-screen, panel ambient),
and a general check for other artifacts the refcount bug may have masked. Diagnostics
(default-off): `BOB_CHECK_SURF`, `BOB_TRACE_SETTEX=<frame>`, `BOB_GARBAGE_HILITE`.

## Conventions
- **Anonymous repo.** Commit as `curator <noreply@anthropic.com>`; never expose a real email.
- Keep changes in `SRC/compat/`; do not edit game logic. Diagnostics are env-gated and
  default-off. Document each session in PORT.md (newest on top) with reproducible evidence.
