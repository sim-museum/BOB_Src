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

## Status (2026-06-15)
Two playable paths exist, both behind env toggles (default `./bob` exits 0):
- **`BOB_BOOT_FRONTEND=1`** — boots a Quick-Mission **3D flight**: daylit sky/terrain/
  scenery/aircraft and a **cockpit that matches the Windows reference** (fixed this session).
- **`BOB_FRONTEND=1`** — boots the **real front-end**: the main menu now **renders and is
  navigable** (mouse clicks change screens).

Run: `BOB_RUN_INIT=1 BOB_DRIVE_C=<wine drive_c> BOB_FRONTEND=1 ./build/bob` (or
`BOB_BOOT_FRONTEND=1`). Windows reference (original `bob.exe` under wine) + before/after
captures live in `doc/reference/`. Engine-level porting notes (also handed to the parallel
MiG Alley port) are in `doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md`.

### What works
- **3D flight / cockpit** — the cockpit corruption was a **surface refcount use-after-free**
  (`GLSurface7` had no refcount; `SURF_Release` freed on the first call while
  `_CreateTextureMap`'s `UpdateMipMaps` AddRef/Release + `textureTable` still referenced the
  block). Fixed with real COM refcounting; also added GL-texture free + a `BOB_CHECK_SURF`
  canary. Texture lifetime audited (no steady-state leak).
- **Front-end (Workstream A)** — drives the natural flow (`CMainFrame::Initialise` →
  `LaunchFullPane(title)`) after fixing `AfxGetMainWnd`/`GetActiveView`/the `WM_PAINT`
  bootstrap. Built a **GDI 2D paint pipeline**: a window framebuffer + present
  (`bob_gdi_*` in `bob_video.cpp`), real `SetDIBitsToDevice` (menu background BMP), and TTF
  text via **stb_truetype** (`bob_gdi_font.cpp`) rendering the game's own fonts. The menu
  draws (background + items in the game font, centred; horizontal tab layout on sub-screens
  via `listalign`), and **mouse clicks navigate** (`bob_frontend_tick` → `OnSelectRlistbox`).

### Open fronts (next work)
1. **R\* controls need OLE/ActiveX hosting** (the big one for the UI). The config/loadout/
   map widgets are ActiveX controls (`CRListBoxCtrl : COleControl`) hosted by `CWnd`
   wrappers that forward via `InvokeHelper` — **a no-op in the compat**, so the controls are
   never instantiated/populated (nothing to render). Real task: minimal OLE hosting
   (instantiate the `CR*Ctrl`, route `InvokeHelper(dispid,…)` to its `DISP_FUNCTION` map),
   then draw (`m_list` is public; the font/blit pieces exist). Unlocks every R* screen.
   The title menu works only because it's drawn directly from `textlists`, bypassing the control.
2. **Black landscape ground** — the airfield detail tiles are black because the landscape
   **render-to-texture compositing** submits no geometry / reads empty back-buffer bits
   (no FBO RTT). Needs FBO render-to-texture (also fixes the rear-view mirror).
3. Secondary fidelity: per-stage texture addressing (terrain over-tiling), faithful menu
   font (`Intel.ttf` won't parse with stb; using the game's FC-Glamour), hover/continuous
   repaint, the intro Smacker, audio (DirectSound→OpenAL, stubbed), mouse/joystick input.

Diagnostics (env-gated, default-off): `BOB_CHECK_SURF`, `BOB_TRACE_SETTEX=<frame>`,
`BOB_GARBAGE_HILITE`, `BOB_TRACE_BLACKGND`, `BOB_DUMP_GDI`, `BOB_AUTOCLICK=<item>`,
`BOB_TRACE_LIFETIME`. See PORT.md (newest first) for the full dated history.

## Conventions
- **Anonymous repo.** Commit as `curator <noreply@anthropic.com>`; never expose a real email.
- Keep changes in `SRC/compat/`; do not edit game logic. Diagnostics are env-gated and
  default-off. Document each session in PORT.md (newest on top) with reproducible evidence.
