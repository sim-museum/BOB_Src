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

Current focus — **restore the cockpit instrument/overlay layer**. We now have a Windows
reference (original `bob.exe` under wine) in `doc/reference/`; comparison shows the native
port is missing the entire 2D `COverlay` layer — instrument gauges, gunsight reticle, HUD
text, and compass — while the 3D cockpit shell renders (with secondary fidelity issues:
grey/over-tiled struts, missing RAF green). Root cause (top PORT.md entry): a malformed
surface (`w=0x700000 h=0 bpp=0 glTex=0xffffffff`) is bound for the overlay draws every
frame and skipped by `draw_fvf`'s garbage guard; it is not created via `DD_CreateSurface`,
so the overlay's surface-creation path is untracked by the compat. Next: trace where that
surface is created (BOB_TRACE_SETTEX → `COverlay::LoaderScreen → EndScene → SetTexture`).
Secondary leads: per-stage texture addressing (terrain over-tiling; `BOB_CLAMP` probe) and
the all-black 64/128 cockpit textures.

## Conventions
- **Anonymous repo.** Commit as `curator <noreply@anthropic.com>`; never expose a real email.
- Keep changes in `SRC/compat/`; do not edit game logic. Diagnostics are env-gated and
  default-off. Document each session in PORT.md (newest on top) with reproducible evidence.
