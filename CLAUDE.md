# Rowan's Battle of Britain — Linux Native Port

Goal: get the open-source **Battle of Britain** (Rowan, 2000) running **native on Linux**,
faithfully — every function and every image intact, the way it ran on Windows. The original
is a Win32 / MFC / DirectX 7 (DirectDraw7 + Direct3D7) / DirectInput / DirectSound title.
The port keeps the **game code unedited** and provides a compatibility layer that maps the
Windows/DirectX APIs onto **SDL2 + OpenGL + OpenAL** under `#if FF_LINUX` / `BOB_LINUX`.

## Working mode
You are porting a Windows codebase to Linux. Work autonomously until the task is complete.
- Do NOT pause to ask for confirmation
- When choosing between approaches, pick the most idiomatic Linux/POSIX solution and proceed
- Only stop if you encounter a hard blocker with no reasonable path forward

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
cd "/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
BOB_RUN_INIT=1 BOB_DRIVE_C="/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c" /home/admin/bob/build/bob
```
`BOB_BOOT_FRONTEND=1` boots straight into a flying Quick-Mission cockpit (3D bring-up).
Many `BOB_*` env toggles gate diagnostics/captures (frame/texture dumps, traces) — see
`getenv("BOB_` sites and PORT.md. Frame/texture dumps land in `/tmp/*.ppm` (convert via PIL).

## Status (2026-07-05)
Playable/working paths, all behind env toggles (default `./bob` exits 0). **See `PORT.md` (newest
first) for the authoritative running log — the below is a snapshot.**
- **`BOB_BOOT_FRONTEND=1`** — boots a Quick-Mission **3D flight** on real GL: daylit sky/
  terrain/scenery/aircraft and a **cockpit matching the Windows reference**.
- **`BOB_FRONTEND=1`** (+ **`BOB_OLE_DRAW=1`**) — the **real front-end**: navigable main menu,
  config screens as working forms, and the **CAMPAIGN strategic map** — matches the wine reference
  (terrain + unit icons + labels + Nm ruler + footer with live event log/date-time + two toolbar rows
  of sheet icons) and is **fully interactive**: toolbar-button handlers, pan/zoom, accel/time clock,
  unit selection (S83–S98).
- **Campaign MULTI-DAY LOOP works** (`BOB_DAYLOOP`, S104–S112): the campaign runs day after day
  headlessly — each day rebuilds its world (`StartUpMapWorld`), the clock advances, the sim is
  ASan-clean (9 days validated), and save/load round-trips the multi-day state. Fixed 3 real game-code
  bugs en route (`m_currentpage`-gates-OnTimer, production-array overflow, `CloseLoggedChild` recursion).
- **OOB info sub-dialogs render** (S113–S117): clicking a map toolbar button opens + shows its dialog over
  the map — e.g. the Bases panel shows the RAF Order of Battle (squadron lists) via hosted controls; renders
  across dialog structures, ASan-clean (`BOB_NO_OOB` reverts).
- **F6 external-view z-fighting FIXED** (S118–S119): 3D scene depth-sorting is now default (`BOB_NO_ZDEPTH`
  reverts) — the external Spitfire renders with proper camo/roundels instead of washed-out painter's-order
  self-occlusion. Both PO backlog items (#1 z-fighting, #2 full campaign) are now addressed.
- **Remaining (documented in PORT.md):** a faithful `EndDayReview`-screen day-advance (the multi-day loop
  currently uses the `BOB_DAYLOOP` scaffold hook), and OOB polish (selected-tab, faithful placement).

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
- **Daylit sky with clouds + FULL_RES terrain (default).** The QM bring-up now defaults to faithful
  settings that were off-by-default minimal: **fluffy clouds** (`HW_FLUFFYCLOUDS`, `CLOUD.CPP`) so the
  sky matches the Windows reference (`BOB_NOCLOUDS` reverts), and **FULL_RES land textures** (256×256 RT,
  4× detail; `BOB_TEXQ=0` reverts). The cockpit view now closely matches `doc/reference/cockpit-windows-
  spitfire-1.png`. Pure game-side config enables — set in the `BOB_BOOT_FRONTEND` boot scaffold (MIG.CPP).
- **Audio (DirectSound → OpenAL).** Was a silent `E_FAIL` stub; now a real backend
  (`SRC/compat/openal_dsound.cpp`): `DirectSoundCreate`/`CreateSoundBuffer`/`Lock`/`Unlock`/`Play`/
  `SetVolume`/`SetFrequency`/`SetPan` + the 3D buffer/listener (`SetPosition`/`SetVelocity`) map onto
  OpenAL buffers + sources. The game's `Sound`/`DigitalDriver` drives it: in flight the **looping
  engine sound** (RPM-modulated via `SetFrequency`→`AL_PITCH`) + one-shot effects load, upload their
  PCM, and play (verified: 10 sources playing, engine looping). Volumes default on (the QM boot left
  them 0 → silent); `BOB_NOSOUND` disables. (Found + worked around a latent game-code stack overflow:
  `Sample::LoadBuffer` copies a 20-byte `PCMWAVEFORMAT` into an 18-byte `WAVEFORMATEX` — benign on
  Windows, tripped `-fstack-protector`; disabled it for the HARDWARE TU.)
- **Keyboard flight input (DirectInput → SDL).** Real keypresses on the GL window drive flight: SDL
  keydown → DIK scancode → buffered DInput keyboard device → the game's event-driven `OnKeyInput`
  (MIG.CPP:809 → STUB3D.CPP) → `OnKeyDown(dik)` → `commonkeymaps` → flight command. Proven end-to-end
  (`BOB_AUTOFLY=throttle` → `[key] OnKeyDown dik=0x0b -> index=50`). Joystick is not enumerated yet
  (`DI_EnumDevices` returns 0; also no joystick hardware here) — keyboard is the working control path.
- **Front-end GDI 2D pipeline** — window framebuffer + present (`bob_gdi_*`), `SetDIBitsToDevice`,
  stb_truetype text (`bob_gdi_font.cpp`), mouse-navigable menu (`bob_frontend_tick`).

### Open fronts (next work)
1. **Landscape RTT polish + the rear-view mirror.** The landscape FBO path is **default-on**
   (ground renders; stable to frame 150; `BOB_NO_FBO_RTT` reverts). The rear-view **mirror** rides
   the same RTT machinery but is **dormant by default** (gated on the Reflections setting
   `COCK3D_SKYIMAGES`, which defaults OFF; force it with `BOB_MIRROR`). A/B'd (`BOB_DUMP_RTT` dumps
   each RTT FBO, `draw_fvf` per-quad tracing): the mirror FBO is created/bound/displayed and **real
   geometry reaches it** — 296 textured fullscreen quads = the **horizon/`InfiniteStrip` backdrop**
   (`RenderMirrorLandscape` renders the distant horizon+sky, *by design not* the detailed near-ground
   tiles the land RTT composites). It looks blank (variance 0, systematic across
   frames) because the mirror horizon quads carry **garbage v-texcoords** (`v≈-2.4e24`, clamped to one
   edge texel → flat) — a latent game-side bug in `InfiniteStrip`'s horizon UV setup (our FVF parsing is
   fine; `u` reads correctly, only `v` is garbage). RTT plumbing is correct; the fix is game-side horizon
   UV work (or a compat texcoord sanitiser) — deferred. **Land textures: now default FULL_RES** — the QM
   boot defaults `Save_Data.textureQuality=4`, so the land RT is **256×256** (4× the old 128 detail; sharp
   fields/runways), default flight + cockpit stable. `BOB_TEXQ`/`BOB_FILTER` override; `BOB_TEXQ=0` reverts.
   Trilinear is now the **default and faithful** filtering (S67, 2026-06-29): `InitPreferences` sets
   `filtering=2` (TRILINEAR — the Windows default), and the R1.3c bilinear pin is gone (R3.5), so the
   default boot runs trilinear. The old `CopyMapToSurface` mip-upload crash no longer reproduces
   (incidentally fixed by the S47/S48/S60 texture/surface hardening). **Soak-validated S67:** 90 s ASan
   flight + 60 s normal flight (with terrain streaming → mip re-uploads), **0 ASan errors, 0 crashes,
   frame-300 97 % non-black**. `BOB_BILINEAR` reverts to bilinear for A/B; `BOB_TEXQ`/`BOB_FILTER` override.
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
headless combo-click tests), `BOB_NO_FBO_RTT` (disable the now-default landscape RTT), `BOB_MIRROR`
(force Reflections/`COCK3D_SKYIMAGES` on so the rear-view mirror RTT activates), `BOB_TEXQ=<0-4>` /
`BOB_FILTER=<0-3>` (land texture quality / filtering; default max/bilinear), `BOB_NOCLOUDS` (disable the
now-default clouds), `BOB_NOSOUND` (disable audio), `BOB_TRACE_SND` (audio trace), `BOB_HUD` (enable the
in-flight HUD info bar), `BOB_GROUNDSHADE`
(ground-shading vertex lighting), `BOB_TRACE_TSS`, `BOB_TRACE_RTT`,
`BOB_DUMP_RTT` (dump each RTT FBO → `/tmp/rtt_<ptr>.ppm`), `BOB_DUMP_FRAME=<n>`/`BOB_DUMP_GDI`,
`BOB_DUMP_PATH=<file>` (private frame-dump target — `/tmp` is shared with the MiG Alley port),
`BOB_SHOT=<n>`/`BOB_SHOT_PATH=<file>` (S123: deterministic one-shot GDI screen capture after n
front-end ticks/map paints, then exit — the screen-parity harness; see `doc/screen-parity.md`),
`BOB_NO_LISTXY` (revert menu lists to the pre-S123 synthetic anchors),
`BOB_NO_PE_RSRC` (S124: revert dialog layout/caption data to the source-checkout .rc parse —
default is the INSTALLED build's PE resources, boblang.dll = BDG 0.99, the parity oracle),
`BOB_CHECK_SURF`, `BOB_RC_DIR`.

**Interaction / dispatch (S156–S158).** `BOB_MAP_CLICK="x,y"` injects a map click at the dispatch
(layer 3); **`BOB_MAP_SDLCLICK="x,y"` / `BOB_SDL_CLICK="tick,x,y"` push a REAL `SDL_MOUSEBUTTONDOWN`**
(layer 1) — only meaningful on a real GL display, because under `SDL_VIDEODRIVER=dummy`
`SDL_CreateWindow` fails and `pump_events` is never called at all, so no headless run can exercise
the SDL layer. `BOB_TRACE_CLICKPATH` traces the click through all five layers.
`BOB_NO_OOB_CLICK` reverts S156's open-dialog-first-refusal routing (before S156 the map's OOB
dialogs were **render-only** — paintable, never clickable). `BOB_MSG_DISPATCH` enables S158's real
message map (compat's `SendMessage` was an allowlist of 3; 16 of the 20 `WM_*` routes the game sends
died returning 0); `BOB_TRACE_MSG` reports dispatched/unhandled routes, deduped one line per id.
See `doc/scaffold-audit.md` for which capabilities are actually proven vs only ever driven by a
scaffold, and PORT.md (newest first) for the full dated history.

## Conventions
- **Anonymous repo.** Commit as `curator <noreply@anthropic.com>`; never expose a real email.
- Keep changes in `SRC/compat/`; do not edit game logic. Diagnostics are env-gated and
  default-off. Document each session in PORT.md (newest on top) with reproducible evidence.
