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

Current focus — **cockpit instrument panel corruption** (rainbow-banded panel textures).
Confirmed this session: it is **NOT** a render-to-texture/FBO problem (the panel is ordinary
`RENDERTARGET_PRIMARY` geometry; the only rejected RTT surface is the 128×128 mirror probe,
whose back-buffer fallback already works). The real defect is **bad source pixel data** for a
565 `TF_NORM` cockpit texture — a `.shp`/asset-pipeline bug. Next: trace which cockpit `.shp`
the rainbow map comes from and why its `body` bytes are wrong. See the top PORT.md entry.

## Conventions
- **Anonymous repo.** Commit as `curator <noreply@anthropic.com>`; never expose a real email.
- Keep changes in `SRC/compat/`; do not edit game logic. Diagnostics are env-gated and
  default-off. Document each session in PORT.md (newest on top) with reproducible evidence.
