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

## Status (2026-06-16)
Playable/working paths, all behind env toggles (default `./bob` exits 0):
- **`BOB_BOOT_FRONTEND=1`** — boots a Quick-Mission **3D flight** on real GL: daylit sky/
  terrain/scenery/aircraft and a **cockpit matching the Windows reference**.
- **`BOB_FRONTEND=1`** (+ **`BOB_OLE_DRAW=1`**) — the **real front-end**: navigable main menu,
  and the **config screens are working forms** — genuine ActiveX controls hosted, positioned
  from the real `.rc`/DLGINIT, populated, and rendered (labels + dropdown values + borders).

Run: `BOB_RUN_INIT=1 BOB_DRIVE_C=<wine drive_c> BOB_FRONTEND=1 BOB_OLE_DRAW=1 ./build/bob`
(or `BOB_BOOT_FRONTEND=1` for flight — needs a real GL display, not dummy SDL). Windows
reference + dated captures live in `doc/reference/`. Engine notes (also handed to the
parallel MiG Alley port): `doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md`.

### What works
- **R\* ActiveX control hosting (Workstream A, this session).** The front-end UI is real OLE
  control hosting now, not the old `InvokeHelper` no-op. Three genuine controls compiled into
  the build and hosted — **RListBox / RCombo / RStatic** (`SRC/RLISTBOX/bob_ole*.cpp`,
  `SRC/RCOMBO/bob_ole_rcombo.cpp`, `SRC/RSTATIC/bob_ole_rstatic.cpp`): `CWnd::CreateControl`
  (driven by `DDX_Control`) instantiates the real `CR*Ctrl`; `InvokeHelper`/`Get`/`SetProperty`
  route each dispid to its genuine (protected) methods via a per-control host + a `CWnd*`→host
  side-table. **Positioning + labels** come from a runtime `.rc`/DLGINIT parser
  (`SRC/compat/bob_dlgtemplate.cpp`): control rects (dialog units → px) from MIG.RC/BOB.RC and
  design-time label captions from the binary DLGINIT property bags. `CDialog::Create` drives
  `DoDataExchange`+`OnInitDialog`; `GetDlgItem(id)` resolves to the hosted control so the
  config's populate pass fills every combo. **Result:** the GFX & Sound config tabs are
  complete labelled forms (verified general); rendering via the controls' own `OnDraw` onto the
  `bob_gdi` framebuffer (`CDC` text/line primitives wired). Captures in `doc/reference/`.
- **3D flight / cockpit** — runs on real GL (`BOB_BOOT_FRONTEND`); the cockpit corruption was a
  surface refcount use-after-free (fixed with real COM refcounting + GL-texture free).
- **Landscape ground via FBO render-to-texture — now DEFAULT.** The airfield ground was black
  because the landscape composite had no real render target; the FBO RTT path in `bob_video.cpp`
  (accept RTT surfaces → GL texture + FBO, `DEV_SetRenderTarget` bind/restore, `SURF_Lock` reads
  the FBO back into the bits the game's `UploadTexture` copies) fills the terrain. A/B (now
  default-on vs `BOB_NO_FBO_RTT` escape hatch): ground non-black **51% → 99%** (black → green
  landscape), stable at frames 80 and 150. The setup hang that blocked this was a latent
  game-code NULL `Release()` exposed by the compat over-advertising `D3DPRASTERCAPS_ZBUFFERLESSHSR`;
  fixed by clearing that cap (game source stays pristine). `BOB_NO_FBO_RTT` reverts to the old
  back-buffer fallback (no RTT) for A/B + safety. See PORT.md (2026-06-16).
- **Front-end GDI 2D pipeline** — window framebuffer + present (`bob_gdi_*`), `SetDIBitsToDevice`,
  stb_truetype text (`bob_gdi_font.cpp`), mouse-navigable menu (`bob_frontend_tick`).

### Open fronts (next work)
1. **Landscape RTT polish.** The FBO path is now **default-on** (ground renders; stable to frame
   150; `BOB_NO_FBO_RTT` reverts). Remaining polish: the land RT is only 128×128 (option table
   picked `biggestWH=128` — push for higher land-texture options/sharper detail tiles); A/B the
   rear-view **mirror** (same FBO path); confirm long-session texture lifetime beyond a few
   hundred frames.
2. **Front-end fidelity polish** (cosmetic): widget box-art / dropdown arrows need the icon/
   bitmap **blit subsystem** (`MaskIcon`/`BitBlt` → framebuffer); faithful fonts need a coherent
   **DPI/scale pass** (the panels are scaled-up but the game fonts are native-DLU); minor combo/
   label Y-alignment. Then apply the (general) pipeline to the campaign/loadout/map screens.
   *Interaction is live:* clicking a hosted combo cycles its value (`bob_ole_click` →
   `HostRCombo::onClick` → repaint; `SG2C_WRITEBACK` persists it). **Adopted from the MiG Alley
   port (`~/ma`)** this session: **RLE8 (BI_RLE8) BMP decode** in `bob_gdi_setdibits` (was a
   latent gap — backgrounds now decode). *Not adopted (verified N/A for BoB): RButton hosting +
   RTTI eventsink — BoB's dialogs host only RCombo/RListBox/RStatic; buttons render via the
   separate `bob_frontend_tick`/`bob_draw_menu` path, not as OCX.* Shared engine notes:
   `doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md` (== `~/ma/port/BOB_PORT_LESSONS.md`, kept in sync);
   cross-port dialogue in PORT.md (newest entries).
3. Secondary: terrain over-tiling, intro Smacker, audio (DirectSound→OpenAL, stubbed),
   joystick/mouse-via-DInput.

Diagnostics (env-gated, default-off): `BOB_OLE_DRAW`, `BOB_TRACE_OLE`, `BOB_AUTOCLICK=<seq>`
(comma-sep, e.g. `5,3`), `BOB_CLICKXY="tick,x,y;…"` (inject clicks at framebuffer coords —
headless combo-click tests), `BOB_NO_FBO_RTT` (disable the now-default landscape RTT),
`BOB_TRACE_RTT`, `BOB_DUMP_FRAME=<n>`/`BOB_DUMP_GDI`, `BOB_CHECK_SURF`, `BOB_RC_DIR`. See PORT.md
(newest first) for the full dated history.

## Conventions
- **Anonymous repo.** Commit as `curator <noreply@anthropic.com>`; never expose a real email.
- Keep changes in `SRC/compat/`; do not edit game logic. Diagnostics are env-gated and
  default-off. Document each session in PORT.md (newest on top) with reproducible evidence.
