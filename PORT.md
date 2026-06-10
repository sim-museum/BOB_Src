# Rowan's Battle of Britain — Linux Native Port

> ## MAJOR FIX (2026-06-10): std-stream packing ABI — BIStream/saves work
> Root cause (same class as the original Lib3D fstream crash, but now it bit a
> *used* stream): std C++ stream/locale types were compiled with -fpack-struct=1
> (global /Zp1), so a `std::ifstream` (e.g. `BIStream`) built by game code had a
> packed subobject layout mismatching libstdc++ → libstdc++ scribbled memory
> operating on it (a `BIStream` save load silently clobbered the caller's stack →
> the earlier `EE E9` path corruption was a *symptom*, not a fileman bug). Fix:
> `#pragma pack(push,8)` around the std stream-header includes (iostream.h/fstream.h
> shims + the direct `<sstream>/<fstream>/<iostream>` in compat_types.h/wtypes.h) so
> they keep the native ABI despite -fpack-struct (proven to override it, as for
> struct stat). **All std-stream file I/O now works.** Verified: `BIStream` opens +
> deserialises; the path corruption is gone.
>
> Route (a) progress on top of that:
> - `CFiling::LoadGame` opens `savegame/<fname>` relative to cwd (the game dir) via
>   the resolver (bypasses the corrupted fileman fakefile path; FILING.CPP).
> - Savegame version check (embeds build `__DATE__`, fatal under NDEBUG) → BOB_LINUX
>   loads anyway (SAVEGAME.CPP).
> - **Finding:** the bundled `SAVEGAME/*.dat` are NOT loadable savegames —
>   `Package.dat` is a mission *package*; `blank_nt.dat` is the node-tree template
>   (`NodeData::LoadCleanNodeTree`, which self-regenerates on version mismatch). No
>   ready-made "Rowan Savegame:" file exists. So the world for `new Inst3d` must come
>   from the **new-campaign init** (`Miss_Man.camp` from a campaign template +
>   `Persons4::StartUpMapWorld` + battlefields/persons), normally driven by the
>   front-end "New Campaign" dialog flow — a substantial campaign-data subsystem and
>   the next focused step for route (a). Default `./bob` (exit 0)/`BOB_RUN_INIT=1` OK.

> ## ROUTE (a) progress (2026-06-10): std-stream paths fixed; blocked on fileman fake-file
> Chose the campaign/in-game-map route (renders through the GL pipeline). Found the
> load entry: `CFiling::LoadGame(fname)` → `BIStream bis(...); bis>>Miss_Man` (the
> full campaign deserialise incl. ShutDownMapWorld/StartUpMapWorld). The `BOB_BOOT_FRONTEND`
> probe now calls `CFiling::LoadGame("Package.dat")` before `new Inst3d`.
>
> **Fixed (committable, general):** `BIStream`/`BOStream` (BSTREAM.H) derive from
> std `ifstream`/`ofstream` and opened the **raw path** — Windows `\`/drive-absolute/
> case never resolved. Added `bob_resolve_path` (bob_stubs.cpp, exposing the nocase
> resolver) and routed both stream ctors through it. (Needed for ALL std-stream file
> access, not just saves.)
>
> **BLOCKER — fileman fake-file global-state bug.** Save files are opened via
> `fakefile(FIL_SAVEGAMEDIR, name)` + `namenumberedfile`, which build the path in
> shared globals (`namedirdir`/`pathname`/`assumefakedir`, FILEMAN.CPP:262/934). The
> `Package.dat` path comes out corrupted at the FRONT: bytes `EE E9` replace `\P`
> (→ `[??rogram Files\…\savegame\Package.dat]`), so it isn't drive-absolute and the
> `BOB_DRIVE_C` map misses → `LoadGame→0` → `new Inst3d` crashes (no world).
> Diagnostic finding: the SAME fake mechanism renders a CLEAN path for another fake
> file (`radscrpt.bin`: `pathnameptr=\Program Files\…\savegame\radscrpt.bin`), and the
> corrupted `Package.dat` call **doesn't even take the fake branch** — so the
> `fakefile→namenumberedfile` global fake-state is being lost/clobbered between the
> two calls (fragile globals; plausibly disturbed now the move-cycle timer thread runs
> concurrently — Phase 2c). NEXT: make the fake-file path construction robust/atomic
> (or bypass it for saves), then `bis>>Miss_Man` (the large save-deserialise) is the
> step after. Default `./bob` (exit 0) and `BOB_RUN_INIT=1` unaffected.

> ## PHASE 2c-A (2026-06-10): front-end trigger wired; first frame blocked on CAMPAIGN
> The front-end view trigger is wired as an opt-in probe (`BOB_BOOT_FRONTEND`,
> MIG.CPP end-of-InitInstance): the Rtestsh1 pattern `ShutDownMapWorld → new Inst3d
> → new View3d → MakeInteractive(WinMode::WIN)` (→ `SetDriverAndMode` opens the
> SDL/GL window + spawns the draw thread).
>
> **FINDING — the scoped "valid map world" key risk is real and large.** `new Inst3d`
> runs full in-game 3D setup including `Persons2::LoadSetPiece`, which iterates the
> campaign battlefields (`FindNextBf`) and **crashes with no campaign loaded**;
> `ShutDownMapWorld` alone isn't enough. Loading a campaign goes through
> `RFullPanelDial::LoadCampaign`/`LoadCampaignData` + `Persons4::StartUpMapWorld` +
> the save/load system (FULLPANE.CPP) — a substantial front-end subsystem. And the
> true *pre-campaign* main menu is itself dialog-based (DialBox/LaunchDial/FullPanel),
> not the in-game map view. So the first visible frame is **not** "a view + minimal
> render": it needs the campaign/front-end subsystem to produce a valid world first.
>
> Default `./bob` (exit 0) and `BOB_RUN_INIT=1` (clean idle loop) are unaffected; only
> the explicit `BOB_BOOT_FRONTEND` probe hits the campaign dependency (kept as a
> documented marker of the exact blocker).
>
> **Revised path to first frame (two viable routes, both larger than first scoped):**
> - **(a) Campaign route:** bring up enough of the mission-manager/campaign load
>   (`Miss_Man.camp` + battlefields + `StartUpMapWorld`) — possibly via a saved game —
>   so `new Inst3d` + the in-game map view render. Then minimal Phase 3 (textured
>   quads) draws it.
> - **(b) Front-end route:** drive the pre-campaign main-menu dialog system
>   (FullPanel/DialBox), which renders via the RDialog/GDI path (a different backend:
>   `SetDIBitsToDevice`/`BitBlt`/`ExtTextOut`) — needs the GDI-on-SDL surface work
>   deferred earlier. This is the actual first screen a user sees.
> Either is a multi-session subsystem; the choice determines whether the first visible
> output is the in-game map (a) or the main menu (b).

> ## SCOPE (2026-06-09): the FIRST VISIBLE FRAME (front-end + minimal Phase 3)
> Three pieces must come together. Good news: the menu path is far lighter than the
> 3D flight path.
>
> **(1) Front-end trigger** — after InitInstance nothing creates a view. A
> `BootFrontend` hook (env-gated at first) does the Rtestsh1 pattern (RTESTSH1.CPP:319-330):
> `new Inst3d` → `new View3d(inst, hWnd, frame)` → `MakeInteractive(WinMode::WIN)`
> (→ `SetDriverAndMode` opens the SDL/GL window + spawns the draw thread) →
> `OverLay.SetToUIScreen(&firstMapScr)` selects the menu.
>
> **(2) The render path is `rendermap`, NOT `render3d`** — `ThreeDee::render`
> (3DCODE.CPP:3527) branches on `vp->drawSpecialFlags`: the map/menu screen takes
> `rendermap(vp,world)` (+ overlay), which **avoids `render3d`'s `init_scene →
> world->sunpos` unconditional deref** (3DCODE.CPP:1144/1452 — the crash point for the
> 3D path). The remaining world dependency is the overlay's `current_world->pMapItemHead`
> (do_ui_objects), so a **valid but minimal "map world"** is still needed — load the
> campaign/map world (cf. `Persons4::ShutDownMapWorld`) or synthesise an empty one.
> THE KEY RISK/UNKNOWN: getting a world object valid enough for rendermap+overlay
> without the full campaign load.
>
> **(3) Minimal Phase 3 = 2D textured-quad rasterisation only** (a small subset of the
> full DX7→GL). The UI is *pure textured quads* — background, per-character font
> glyphs, option icons — via these Lib3D methods (measured in OVERLAY.CPP
> ProcessUIScreen ~3163):
> - `BeginScene`/`Clear`/`EndScene` → `glClear` + framebuffer (already present-wired).
> - `BeginPoly`/`EndPoly` → accumulate a textured quad → GL (immediate-mode quad with
>   the bound texture + vertex colour). THE core primitive.
> - Texture pipeline: `CreateTexture`/`UploadTexture` from `Image_Map` MAPDESC pixel
>   data (fonts/icons/backgrounds; palettised/16-bit) → GL textures; `SetTexture` → bind.
> - 2D setup: `LoadIdentity` on the matrix stacks + `GiveHint(HINT_2DRENDER)` → GL ortho,
>   depth/lighting off; `SetFontColour` → vertex colour; `SetGlobal`/`SetObjectLighting`
>   → GL state.
> - `ScreenSwap` → present (done + verified, Phase 2b).
> NONE of the heavy 3D pipeline (lighting, z-buffer, vertex buffers/`DrawPrimitiveVB`,
> render-state matrix, the texture *combiner*) is needed for the menu.
>
> **Incremental path:**
> - **Step A — cleared frame:** front-end trigger + a valid-enough map world → the draw
>   loop runs `BeginScene`/`Clear`/`EndScene`/`ScreenSwap` (other methods stay no-op) →
>   a **cleared coloured window driven by the game** (first frame on screen, proves
>   loop→render→present end-to-end).
> - **Step B — the menu:** implement `BeginPoly`/`EndPoly` + texture create/upload/bind
>   + 2D ortho + `SetFontColour` → `ProcessUIScreen` draws the background + menu text/icons
>   → **the main menu is visible.**
>
> Effort: moderate+ but bounded — Step A is mostly the front-end trigger + the world
> prerequisite (the main uncertainty); Step B is the textured-quad subset (BeginPoly/
> EndPoly + the Image_Map→GL texture path). Together this is the first visible frame,
> and far smaller than full 3D-world rendering (Phase 3 proper: landscape, aircraft,
> lighting, vertex buffers, the multi-texture combiner).

> ## PHASE 2c DONE (2026-06-09): threads + timers + events live
> The render-loop's two stubbed primitives are now real (the compat layer already
> had pthread-backed events/CreateThread/WaitForSingleObject):
> - **`AfxBeginThread`** (afxwin.h) → `bob_begin_thread` (bob_threads.cpp): runs the
>   MFC `AFX_THREADPROC` on a detached pthread. The per-view draw loop
>   (`View3d::drawloop`) can now actually run.
> - **`timeSetEvent`/`timeKillEvent`** (mmsystem.h) → `bob_time_set_event` /
>   `_kill_event`: a real periodic/one-shot timer thread invoking the
>   `LPTIMECALLBACK`. Drives `Mast3d::StaticTimeProc → TimeProc` (the "move cycle").
>   Safe to start at static-init: `TimeProc` loops over `currinst` (NULL until a
>   3D instance exists), so it just ticks until the game is ready.
> - Win32 events (`doneframe`, `htable`) were already pthread-cond-backed.
>
> Verified: `BOB_RUN_INIT=1 ./bob` runs with **two live threads** — the main thread
> idling in the message loop and the **timer thread firing `TimeProc`** — at ~0% CPU,
> no crash (default ./bob still exits 0). (gotcha fixed: dosdefs.h `#define proc void`
> clashed with a parameter name.)
>
> **What's left to open the window with content (Phase 2c-step4 + Phase 3):** nothing
> yet creates a `View3d` with a loaded scene — the front-end is a dialog/menu state
> machine (DialBox/LaunchDial/Rtestsh1 + the overlay `MapScr` screens, e.g.
> `SetToUIScreen(&firstMapScr)`), entered by navigating menus. A direct View3d smoke
> path would `SetDriverAndMode` (open the SDL/GL window) + spawn the draw thread, but
> `drawloop → Three_Dee.render(…, inst->world)` derefs a not-yet-loaded world, and the
> Lib3D GL render methods are still Phase-1 no-ops. So the visible window needs the
> front-end flow to select a screen AND Phase 3 (DX7→GL rendering) together.

> ## SCOPE (2026-06-09): Phase 2c — what actually drives the UI (corrected)
> Investigation overturned the earlier assumption that the menu needs a Win32/GDI
> message backend. **The main game UI (main menu, map screen, briefings) renders
> through the Lib3D 3D/overlay pipeline, NOT GDI:** a periodic timer drives the game
> "move cycle", a per-view draw thread renders the current overlay screen, and the
> frame is presented via `ScreenSwap` — which is already GL-backed (Phase 2b). The
> GDI `RDialog`/`CR*` control system (RDIALOG.CPP, RBUTTONC.CPP — `SetDIBitsToDevice`
> /`ExtTextOut`/`BitBlt`) is a **separate, deferrable** subsystem used only for the
> config/options dialogs.
>
> **The render loop (STUB3D.CPP / 3dcode.cpp / OVERLAY.CPP):**
> `Mast3d` ctor: `timeSetEvent(StaticTimeProc, TIME_PERIODIC)` (211) → `DoMoveCycle`
> (game logic, sets the `doneframe` event). Each `View3d` (created via MakeInteractive
> → `SetDriverAndMode`, STUB3D:1031 → opens the SDL/GL window) spawns
> `AfxBeginThread(drawloop,…)` (910). `View3d::drawloop` (1524) waits on `doneframe`,
> calls `Three_Dee.render` → `OverLay.ProcessUIScreen` (renders the current screen
> `pCurScr` via Lib3D Begin/Draw/End) → `g_lpLib3d->ScreenSwap()` (1601) → GL present.
>
> **The real blockers (all stubbed):**
> 1. **Threads** — `AfxBeginThread` returns NULL (afxwin.h:1086) → the draw thread
>    never runs. Need a real `AfxBeginThread` → pthread running the `AFX_THREADPROC`,
>    plus a minimal `CWinThread`.
> 2. **Multimedia timer** — `timeSetEvent` is a no-op (mmsystem.h:188) → `DoMoveCycle`
>    never fires. Need a periodic timer thread invoking the `LPTIMECALLBACK`.
> 3. **Win32 events / sync** — `doneframe` and `Master_3d.htable` (the move-cycle ↔
>    draw-thread ↔ message-loop handshake) use CreateEvent/SetEvent/WaitForSingleObject
>    /MsgWaitForMultipleObjects. Need real pthread-cond/semaphore-backed events so the
>    threads synchronise instead of busy-returning.
> 4. **Trigger the first view/screen** — after InitInstance nothing creates a `View3d`
>    or sets the initial overlay screen (`OverLay.SetToUIScreen(...)`). Find/hook the
>    normal front-end entry so the main menu screen is selected and a View3d opens.
> 5. **(Overlaps Phase 3) fill the Lib3D GL render methods** — `BeginScene/Clear/
>    SetRenderState/SetTextureStageState/DrawPrimitiveVB/textures` are Phase-1 no-ops,
>    so `ProcessUIScreen` currently draws nothing → a blank present. Real DX7→GL
>    fixed-function translation (Phase 3) turns the blank window into the actual menu.
>
> **Path to a visible menu:** Phase 2c (threads + timers + events + trigger view/screen)
> → the SDL/GL window opens and the overlay render loop spins, presenting frames
> (blank until Phase 3). Phase 3 (fill GL rendering) → the menu/map pixels appear.
> Concurrency note: this introduces real multithreading (draw thread ∥ move cycle);
> the Win32 event handshake must be faithfully mapped to avoid races. The GDI RDialog
> backend remains a later, independent phase (config dialogs only).

> ## PHASE 2b (2026-06-09): present pipeline proven (window shows surface pixels)
> Implemented and **verified** the path both 2D (DDraw blits/locks) and 3D pixels
> reach the screen through: `present_surface()` in bob_video.cpp uploads a surface's
> bits as a GL texture (RGB565 / BGRA) and draws a fullscreen quad (compat-profile
> immediate mode, V-flipped DDraw→GL), then `SDL_GL_SwapWindow`. `IDirectDrawSurface7::Flip`
> now presents the primary's back buffer through it.
> - Smoke test: `BOB_VID_SMOKETEST=1 ./bob` opens an 800×600 window, fills a back
>   surface with a gradient, presents 120 frames, and `glReadPixels` confirms it:
>   `centre pixel rgb=(123,125,123), glErr=0` on GL 4.6 (NVIDIA). The SDL2 window +
>   GL present is proven end-to-end, independent of the UI flow.
>
> **Remaining for the actual menu (the large part of 2D UI bring-up):** the game's
> menu/map renders through Lib3D's DDraw surfaces + overlay (`OVERLAY.CPP` MapScr →
> `g_lpLib3d->ScreenSwap`), driven by the MFC paint/update cycle. That cycle never
> fires because `CWnd` is a headless stub — **no real window, no WM_PAINT/WM_TIMER
> dispatch**, so the UI state machine sits idle. The core remaining work is a Win32
> **window + message backend on SDL** (deliver WM_PAINT/mouse/keyboard/timer to the
> MFC frame/view/dialogs) so the paint cycle runs and the overlay renders into the
> surfaces this present pipeline already displays. (The `HARDWARE` `DirectDD` DDraw2
> path appears legacy; the live path is Lib3D's DDraw7, already GL-backed.)

> ## PHASE 2a (2026-06-09): message loop runs clean; view created
> `CMIGApp::Run()` (the MFC message pump) now runs cleanly instead of busy-spinning
> or crashing:
> - `MsgWaitForMultipleObjects` (compat_winuser.h) returned 0 (=WAIT_OBJECT_0, the
>   first 3D event) -> Run looped forever on `Inst3d::OnKeyInput`. It now calls
>   `bob_msg_wait` (bob_video.cpp): pump SDL events + `SDL_Delay(3)` + return
>   WAIT_TIMEOUT, so Run takes its idle path at ~0% CPU.
> - That idle path (`CMIGApp::OnIdle`) dereferences `RDialog::m_pView` (the CMIGView),
>   which nothing had created -> segfault. InitInstance's `#if BOB_LINUX` bridge now
>   also creates the view: `CMIGView::CreateObject()` (DYNCREATE factory; the ctor is
>   protected and sets `RDialog::m_pView=this`) and sets `m_currentpage=1`.
>
> Result: `BOB_RUN_INIT=1 ./bob` boots fully and sits in its real message loop,
> idle, at 0% CPU, no crash (default ./bob still exits 0). **Still not visible:** the
> on-screen window + UI need the next layer — `CMIGView::OnInitialUpdate` (creates the
> map dialog + scrollbars), the 2D GDI/DDraw rendering path for the menu/map UI, and
> the game's UI state machine to advance (and, for 3D scenes, `View3d`→SetDriverAndMode
> opening the SDL/GL window). NEXT (Phase 2b): drive OnInitialUpdate + a real SDL
> window for the MFC main view + begin the 2D blit/GDI rendering so the menu draws.

> ## PHASE 1.5 DONE (2026-06-09): PE resource loader — InitInstance COMPLETES
> `compat/bob_resources.cpp` (new) is a minimal Win32 PE resource loader. The
> game's UI strings/dialogs live in a resource-only DLL (`boblang.dll`, ~650KB,
> `.rsrc`); `LoadString` (RT_STRING) has ~199 call sites and an empty result used
> to spin the font setup. The loader parses the PE (DOS→PE→optional→sections), the
> `IMAGE_RESOURCE_DIRECTORY` tree (Type/Name/Lang), and resolves RVAs — all
> offset-based (no packed overlays). Wiring:
> - `LoadLibraryA` (compat_winbase.h) → `bob_LoadLibrary` (opens via `fopen_nocase`
>   so the `\Program Files\…`/`BOB_DRIVE_C` path resolves, mallocs+parses the image).
> - `AfxGet/SetResourceHandle` (afxwin.h) → the loaded module.
> - `CString::LoadString(nID)` (cstring_impl.cpp) → `bob_load_string`: RT_STRING
>   bundle = `(id>>4)+1`, index `id&15`, WORD-prefixed UTF-16 → Latin-1.
> - `bob_res_get` for FindResource/LoadResource (bitmaps/custom; secondary).
>
> Plus a GDI fix: `EnumFontFamiliesExA` (compat_wingdi.h) now invokes the callback
> once (reports the font found). `CreatePointFont` (MIG.cpp) loops over font names
> calling it and only breaks when the callback sets a flag — the never-calling stub
> spun forever (the `jmp self` hangs seen in InitInstance).
>
> Result: `BOB_RUN_INIT=1 BOB_DRIVE_C=… ./bob` now **completes `CMIGApp::InitInstance()`
> (returns 1)** and enters `CMIGApp::Run()` — the real message pump. It currently
> busy-spins there: `MsgWaitForMultipleObjects` is stubbed (returns "keys ready"),
> so Run loops on `Inst3d::OnKeyInput()`. **The on-screen window still needs the MFC
> view lifecycle** — `SetDriverAndMode` (→ the SDL2 window) is called from
> `View3d::MakePassive` (STUB3D.CPP:1031), reached only when the doc/view framework
> creates a 3D view. NEXT (Phase 2): drive the message loop from SDL events + create
> the CMIGView/View3d so the view setup fires SetDriverAndMode and the window opens;
> then fill the GL rendering methods.

> ## PHASE 1 DONE (2026-06-09): SDL2/OpenGL backend — 3D + input init pass
> `compat/bob_video.cpp` (new) implements the DirectDraw7/Direct3D7 COM interfaces
> (from compat/ddraw.h + d3d.h) as concrete GL-backed objects: each is a
> `{ Vtbl*, ...state... }` whose lpVtbl points at our function tables, so the game's
> `p->Method()` calls dispatch to us with **no LIB3D.CPP edits**. `DirectDrawCreateEx`/
> `DirectDrawEnumerateExA` hand out the objects; `GetDXVersion` returns 0x0700
> (`#if BOB_LINUX` in GETDXVER.CPP). It creates a real **SDL2 window + GL context**
> (`ensure_window`, lazy on SetCooperativeLevel/SetDisplayMode) and pumps SDL events
> (ESC/close → exit). Rendering methods (Clear/DrawPrimitiveVB/SetRenderState/…) are
> safe no-ops for now; present = `SDL_GL_SwapWindow`. Also added a **non-fatal
> DirectInput stub** (reports no input) so `Mast3d::Init` proceeds. Build links
> `-lSDL2 -lGL` (32-bit libs present; no sdl2.pc → linked by name).
>
> Result: `BOB_RUN_INIT=1 BOB_DRIVE_C=… ./bob` now passes the **entire 3D-driver
> init** — `Lib3D::Initialise` enumerates our driver/modes/device, `Mast3d::MainInit`
> completes, `DirectInputCreate` succeeds — and stops cleanly at the **next**
> subsystem: the Windows resource DLL `boblang.dll` (MIG.CPP:499 `LoadLibrary`, fatal
> "Can't find language file"). The window itself is created later, in
> `Lib3D::SetDriverAndMode` (STUB3D.CPP:1031), which InitInstance reaches only AFTER
> the language resources + font setup. Faking the DLL handle gets past the load check
> but feeds empty resource data into a font-setup `for(;;)` spin (MIG.CPP:613) -- so
> the clean fatal is kept until **PHASE 1.5: a minimal PE resource loader** for
> boblang.dll (LoadString/FindResource/LoadResource over the .rsrc section) lands.
> That, plus the GDI/font path, is what stands between init and the on-screen window.

> ## SCOPE (2026-06-09): DDraw7/Direct3D7 → OpenGL backend
> The render engine (`Lib3D`, in `SRC/LIB3D/LIB3D.CPP`) is a **DirectX 7
> fixed-function renderer**. Key insight for a FAITHFUL port: the game calls the
> DDraw/D3D objects as C++ `p->Method()` on the interfaces declared in
> `compat/ddraw.h` + `compat/d3d.h`. So the entire backend can live in the **compat
> layer** — make those interfaces concrete GL-backed C++ classes and implement
> `DirectDrawCreateEx`/`DirectDrawEnumerateEx` to hand them out. **No LIB3D.CPP
> edits needed.** A real SDL2 window + GL context replaces the headless stub; the
> backend can own the window (ignore the MFC `hWnd`).
>
> **Call surface (measured across SRC/LIB3D + SRC/3D):**
> - Lifecycle: `Lib3D::Initialise` (DX-version gate + `DirectDrawEnumerateEx`
>   driver list + `EnumDisplayModes`), `Lib3D::SetDriverAndMode` (LIB3D.CPP:3521 —
>   `SetCooperativeLevel`/`SetDisplayMode`, `CreateSurface` primary+back+zbuffer via
>   complex flip chain, `EnumZBufferFormats`, `QueryInterface(IID_IDirect3D7)`,
>   `EnumDevices` HAL, `CreateDevice`, `SetViewport`, `CreateClipper` windowed),
>   `BeginScene` (4236: `SetRenderTarget`,`SetViewport`,`BeginScene`,`Clear` z-only),
>   `EndScene` (4561: render lists,`EndScene`), `ScreenSwap` (4022: fullscreen
>   `pDDSP7->Flip` / windowed `pDDSP7->Blt` → **maps to `SDL_GL_SwapWindow`**).
> - Surfaces (DDraw7): `Lock`/`Unlock`(31/30), `GetSurfaceDesc`(25),
>   `GetAttachedSurface`/`AddAttachedSurface`, `Blt`(12), `SetPalette`, `GetDC`
>   (GDI text on 2D surfaces), `Flip`. Primary/back/z = the GL default framebuffer.
> - D3D7 device: `SetTextureStageState`(193), `SetRenderState`(126 — 21 distinct:
>   ALPHABLEND/SRC+DESTBLEND, Z ENABLE/FUNC/WRITE/BIAS, LIGHTING/AMBIENT/SPECULAR,
>   FOG×6, DITHER, CLIPPING, TEXTUREPERSPECTIVE), `SetTransform` WORLD/VIEW/PROJECTION,
>   `Clear`, `SetTexture`, `SetMaterial`/`SetLight`/`LightEnable`(lighting, ≥6 lights),
>   `Create/Apply StateBlock`(16), `DrawPrimitiveVB`/`DrawIndexedPrimitiveVB`,
>   `SetViewport`/`GetViewport`, `EnumTextureFormats`.
> - Geometry: vertex buffers (`CreateVertexBuffer`, `Lock`/`Unlock`), FVF mix of
>   **pre-transformed** `XYZRHW`/TLVERTEX (UI/2D → identity MV + screen ortho) and
>   **untransformed lit** R3DVERTEX (normals → transform+light pipeline); prim types
>   TRIANGLEFAN / LINELIST / POINTLIST.
> - Textures: DDraw surfaces w/ `DDSCAPS_TEXTURE`, D3D-managed
>   (`DDSCAPS2_D3DTEXTUREMANAGE`). Formats **DXT1/DXT3** (→ GL S3TC, direct upload),
>   RGB565/1555/4444 (→ GL formats), **8-bit palettised** (expanded to RGBA in the
>   existing Lock/memcpy path). Upload = Lock→convert→Unlock (→ `glTexSubImage`).
>   2 texture stages; stage states COLOROP/ALPHAOP/ARG1/ARG2 (the **combiner** — the
>   one real-fidelity risk), MIN/MAG/MIPFILTER, ADDRESS, TEXCOORDINDEX.
>
> **Strategy:** GL **compatibility profile (fixed-function)** maps DX7 almost 1:1
> (glLight/glMaterial/matrix stack/glTexEnv combiners) → fastest to pixels; migrate
> to a small GLSL fixed-pipeline emulator later if needed.
>
> **Phased plan (each independently runnable):**
> 1. SDL2 window + GL context; `DirectDrawEnumerateEx`/`EnumDevices`/mode-enum report
>    one HAL driver at native res → `Initialise`+`SetDriverAndMode` succeed.
> 2. GL-backed `IDirectDraw7`/`IDirectDrawSurface7`/`IDirect3D7`/`IDirect3DDevice7`
>    objects (primary/back/z = framebuffer). `BeginScene`/`Clear`/`EndScene`/`Flip`
>    → `glClear`+`SDL_GL_SwapWindow` → **clears to colour on screen (first frame).**
> 3. Vertex buffers + `DrawPrimitiveVB` + FVF → `glDrawArrays` → **untextured geometry.**
> 4. Texture create/upload (DXT/16-bit/palettised) + `SetTexture` → **textured world.**
> 5. Render-state + texture-stage-state translation, transforms, lighting/fog →
>    **full-fidelity 3D.** Then the 2D DDraw `Blt`/`GetDC` path for UI/text.
>
> Estimated ~2–4k lines of GL backend in compat, over several sessions. Milestone 2
> ("first frame on screen") is the near-term proof point. Dependencies: SDL2 +
> system OpenGL (libGL), GL_EXT_texture_compression_s3tc for DXT.

> ## MILESTONE (2026-06-09): GAME DATA PIPELINE WORKS — reaches 3D-driver boundary
> Pointed at a real install (`BOB_DRIVE_C=.../WP/drive_c`, run from the game dir),
> `BOB_RUN_INIT=1 ./bob` now loads `ROOTS.DIR` + the data archives and runs
> `Mast3d::MainInit`: **`fileman::InitFileSystem` ✓, `Image_Map.InitImageMaps` ✓,
> `_Miles.Init` (sound) ✓, `_Radio.Init` ✓** — then stops at `g_lpLib3d->Initialise`
> → **"Unable to find suitable DirectX 7.0 or later 3D Driver"** (STUB3D.CPP:259),
> because `DirectDrawCreateEx`/D3D7 are stubbed to E_FAIL. The DirectDraw7/Direct3D7
> → OpenGL backend is the next (large) subsystem; everything up to 3D device
> creation works on Linux. Two fixes unlocked this:
> - **`-fpack-struct=1` was corrupting libc structs in the compat layer.** `struct
>   stat` packed to 1 → `stat()` overran it (stack smash) and `st_mode` was garbage,
>   so `fopen_nocase`'s `S_ISDIR` guard returned NULL for every real file (ROOTS.DIR
>   "not found"). Fix: `#pragma pack(push,8)` around the system-header includes in
>   `bob_stubs.cpp` — native ABI for `stat`/`dirent`, while game-facing structs
>   (`_finddata_t`, GUID…) stay packed=1. **General hazard: any compat code that
>   field-accesses a libc-filled struct must wrap its system includes this way.**
> - **Windows drive-absolute paths.** The game's stored paths are `\Program Files\
>   Rowan Software\Battle Of Britain\…` (drive-relative) / `C:\…`. `resolve_nocase`
>   now maps a leading `/` or `X:` onto `$BOB_DRIVE_C` (the Wine `drive_c` dir), so
>   they resolve case-insensitively under the real install.
>
> **How to run with assets:** `cd "<install>/WP/drive_c/Program Files/Rowan Software/
> Battle Of Britain"; BOB_RUN_INIT=1 BOB_DRIVE_C="<install>/WP/drive_c" .../build/bob`

> ## MILESTONE (2026-06-09): full init runs; reaches game-DATA boundary cleanly
> With a headless main window (`new CMainFrame` for `m_pMainWnd`) and a stub `CDC`
> from `CWnd::GetDC()`, **`CMIGApp::InitInstance()` now runs its ENTIRE init
> sequence as real game code** and reaches the game's own data-load stage:
> `InitInstance → Master_3d.Init → Mast3d::MainInit → fileman::InitFileSystem →
> makerootdirlist` → **`Error::SayAndQuit("Can't find ROOTS.DIR")`**. This is the
> game's *intended* error path for absent data — `ROOTS.DIR` and the BoB data
> archives are **not in this source-only repo**. `SayAndQuit` now prints the fatal
> message and `_exit(1)`s on Linux (MessageBox is a stub; and it must not run C++
> static teardown, which would hit the never-initialised `Sound::ShutDownSound`).
> So `BOB_RUN_INIT=1 ./bob` exits **cleanly with "Can't find ROOTS.DIR" (exit 1)**,
> no segfault. **The port is functionally sound up to the asset boundary**; supply
> the game data files next to a `bob` working dir to proceed into the game proper.
> (Default `./bob` with no env still exits 0.)

> ## RUNTIME MAP (2026-06-09): `CMIGApp::InitInstance()` now driven; boundary located
> The MFC boot path is wired and exercising real game code. `bob_main.cpp` calls
> the C-linkage hook `bob_init_instance()` (MIG.CPP, `#if BOB_LINUX`) →
> `theApp.InitInstance()`; `AfxGetApp()` returns the real `&theApp` (via
> `g_pBobApp`). Opt-in with **`BOB_RUN_INIT=1 ./bob`** (default run stays clean/exit 0).
>
> **InitInstance runs its ENTIRE framework-setup phase without crashing** — the
> registry block (`RegOpenKeyEx`/`RegQueryValueEx` stubs), `AfxOleInit`,
> `AfxEnableControlContainer`, `Enable3dControlsStatic`, `SetRegistryKey`,
> `LoadStdProfileSettings`, `new CSingleDocTemplate(...)` + `AddDocTemplate`,
> `RCommandLineInfo` + `ParseCommandLine`, `ProcessShellCommand` — then **stops at
> the first use of the main window** (MIG.CPP ~467: `m_pMainWnd->ModifyStyle/
> SetWindowText`). Confirmed by disasm: `mov 0x4(%esi),%eax` loads `m_pMainWnd`=NULL,
> `push 0x4(%eax)` derefs null. Cause: the MFC **doc/view framework is stubbed**, so
> `ProcessShellCommand` never runs `OnFileNew → OpenDocumentFile` to create the
> `CMainFrame`/`CMIGDoc`/`CMIGView`; `m_pMainWnd` stays NULL.
>
> **NEXT subsystem chain (each a real impl):** create the main window (`CMainFrame`,
> a `CFrameWnd` — needs a window backend, SDL2/X11) → `Master_3d.Init(hInst, hWnd)`
> (3D device; DirectDraw is stubbed→E_FAIL, must degrade or use GL) → GDI DC/font
> block (`GetDC` must return a real/stub `CDC`; `IconDescUI::LoadInstances(*pdc)`
> **derefs pdc**, so a NULL DC crashes) → `File_Man` numbered-file loads (needs the
> **game data assets**, which are NOT in this source repo) → `InitInstance` returns
> TRUE → `CMIGApp::Run()` message loop. `CMainFrame::InitialiseSafe()` itself is
> cheap (sets `havesafe`; real `Initialise()` waits for first `OnPaint`).

> ## MILESTONE (2026-06-09): `bob` COMPILES, LINKS, and RUNS (exit 0)
> The 4.5M 32-bit i386 ELF now executes through **all global constructors** and
> exits cleanly. Two runtime-bring-up crashes were fixed past the link:
> - **Static-init SIGSEGV** in `Lib3D::Lib3D()` → `std::basic_ios::init` →
>   `std::locale::operator=`. Cause: `Lib3D` has a value member `fstream diagFile`
>   (old `<fstream.h>` → std `<fstream>`); the `Lib3D` object is built by a global
>   ctor (`STUB3D`'s `Inst3d::commonkeymaps` TU init → `Lib3DCreate` → `new Lib3D`)
>   **before the C++ runtime locale is set up**, so the std::fstream member ctor
>   deref'd a null global locale. `init_priority(101)` on a forced `ios_base::Init`
>   did NOT help (the member ctor runs in the static-init storm regardless). Fix:
>   make `diagFile` a **lazily-allocated `fstream*`** (`new`'d in `OpenDiags()`,
>   which only runs when device diagnostics are enabled — off by default), so no
>   std stream is constructed at static-init time. All sites `#if BOB_LINUX`-guarded
>   so the Windows build is byte-identical (LIB3D.CPP:528-575, ctor ~2700).
>   (Note: NOT a `-fpack-struct` ABI clash as first suspected — recompiling LIB3D
>   without packing did not move the crash; it was purely the locale-before-init.)
> - **Exit-time SIGSEGV** in a global dtor `Mast3d::~Mast3d()` →
>   `Sound::ShutDownSound()`, derefing DirectSound state that `InitInstance()`
>   never brought up (sound creation is stubbed to E_FAIL). Fix: `bob_main.cpp`
>   `_exit(0)` (after `fflush(NULL)`) to skip C++ static teardown of
>   never-initialised subsystems until the real runtime loop exists.
> NEXT (runtime bring-up proper): drive `AfxWinMain`→`theApp.InitInstance()` and
> wire SDL2 window / OpenGL present / OpenAL behind the stubbed DX entries.


> ## SCOPE (2026-06-09, corrected): source is COMPLETE; remaining work is wiring unbuilt TUs
> A whole-archive trial link of all 14 module libs surfaces **465 undefined
> symbols**. Correctly traced (see the RETRACTION note below):
> - **~335 of the 359 C++ symbols are DEFINED in the source — in TUs not yet wired
>   into a module lib.** By dir: **3D ~195** (the `_3D.CPP` unity — COLLIDED/UI3D/
>   IMPACT/… — isn't built; 3D lib only has LANDSCAP/LSTREAM/TILEMAKE/WEAPPAK/
>   OVERLAY), MISSMAN ~62, **BFIELDS ~47** (the never-built bit-field/persons
>   module), MFC ~15, AI ~12 (the `_AI.CPP` unity), MOVECODE ~5. These are
>   mechanical to add (the porting recipe applies) — they hold the real impls of
>   `Collide::GroundAltitude/HaveWeLanded/LowestSafeAlt` (COLLIDED.CPP:688-849),
>   `AirStruc`/`ArtInt`/`AnimControl`/etc.
> - **~52 external**: DirectX/DirectPlay creation entries (36), Miles/AIL sound
>   (9), CRT `_findfirst`/`FindFirstFileA`/`fopen_nocase`/`_itoa` (7).
> - **~24 residual**, mostly **bob's own `CString` inline methods** (false
>   negatives — defined inline in cstring.h, ODR-resolved at use) plus a handful
>   to check case-by-case (CampaignZero::NextMission, item::Formation_xyz,
>   RMdlDlg::OnCommandHelp — likely inline/template).
>
> **A real link IS reachable from this source.** Path: add the unbuilt unity TUs
> (`_3D`, `_AI`, `_AIRC`, …) + the BFIELDS module + external stubs + the 6
> MASM→nasm + a `bob` add_executable/entry → iterate link → runtime bring-up.
>
> > ### RETRACTION of an earlier (wrong) "incomplete source" alarm
> > I briefly concluded ~276 core functions were "absent from the source." That was
> > a **grep artifact**: many .cpp carry **ISP-8859 high-bytes in their licence
> > headers** (e.g. COLLIDED.CPP is "ISO-8859 text"), so plain `grep` treats them
> > as **binary and reports no matches**. My defined-symbol index was built without
> > `grep -a`, so every ISO-8859 file's definitions were silently dropped → false
> > "absent". With `grep -a` the definitions are all there (COLLIDED.CPP defines
> > `Collide::GroundAltitude` at line 838, etc.). **Lesson: always `grep -a` /
> > `rg --text` on this tree.** The source is complete; the gap is unbuilt TUs.

Goal: build the game to run **natively on Ubuntu 26.04** (no Wine), from the
original Windows source in `SRC/`. Game data: a working Wine install at
`/home/m/sgl/TUE/BattleOfBritain` — specifically
`.../WP/drive_c/Program Files/Rowan Software/Battle Of Britain/` (contains
`bob.exe`, `Lib3D.dll`, `BoB.pdb`, and all assets). Reference port:
`/home/m/ff` (a completed FreeFalcon Ubuntu 26.04 port — same era/tech;
its `CLAUDE.md` is the playbook and `src/compat/` is reusable).

## Architecture of the original (reconnaissance, 2026-06-08)

- **`bob.exe`** — PE32 i386 GUI, ~2.9 MB. Built by `SRC/MFC/BOB.DSP` (VC6),
  **1090 source entries**, `/machine:I386`, `/Zp1` (1-byte struct packing — the
  binary file formats depend on this), `/G6`, `_AFXDLL` (MFC as shared DLL).
  Links: `ddraw dplay dinput dsound dxguid winmm htmlhelp quartz strmbase
  vfw32 lib3d`.
- **`Lib3D.dll`** — software 3D engine, built by `SRC/LIB3D/LIB3D.DSP`.
- **~615K LOC** C/C++ across `SRC/` (660 .H, 464 .CPP + more). Main exe pulls
  mostly from `SRC/MFC/` (454 local .cpp) plus `3D MODEL BFIELDS MISSMAN
  MOVECODE HARDWARE AIRCRAFT FILES COMMS MYCMDS GENERAL AI MATH INPUT GRAPHICS`,
  headers in `SRC/H/`.
- **Standalone tools** (separate `main()`/`.dsp`, NOT part of the game exe,
  port last/never): `MEDITOR`, `PLACENAM`, `ITEMGRID`, `BFIELDS/BATNODE`.
- **Hand x86 ASM, ~9.8K lines, 8 files** — all **32-bit flat** MASM
  (`.386` / `USE32`; the "16 bit" in comments = 16-bit *color*, not code):
  `GRAPHICS/GRAFPASM.ASM` (6755), `MEDITOR/TPAINTWL.ASM` (editor-only),
  `3D/LSTRASM.ASM`, `MATH/MATRASM.ASM`, `HARDWARE/{PRO,PROLOG,HARDPASM}.ASM`,
  `FILES/CDROM.ASM`. Plus 8 files with inline `__asm`.
- **Rendering**: software rasteriser (the ASM blitters + `HARDWARE/HARD320*`),
  final framebuffer presented via **DirectDraw** (only ~11 files touch DDraw;
  **zero Direct3D**). This is good news: keep the software renderer, replace
  only the present path.
- **MFC**: `_AFXDLL` globally, but only ~12 files `#include <afx*>` directly
  (rest via `stdafx.h` PCH). Dialogs/CWinApp usage is the porting risk to size.

## Strategic decisions

1. **Build 32-bit native (i386 ELF on x86-64 Ubuntu via multilib).** This is the
   faithful, far-more-tractable path vs. the 64-bit route `ff` took:
   - The `.386/USE32` ASM assembles directly (no rewrite to C).
   - `/Zp1` packing, pointer sizes, and all binary file-format assumptions stay
     valid → **avoids the entire class of 32/64-bit pointer-truncation bugs**
     that dominated the `ff` port logs.
   - i386 multiarch is already enabled on this box.
2. **Reuse `/home/m/ff/src/compat`** as the Windows compat seed (windows.h,
   ddraw.h, dsound.h, dinput.h, io.h, etc.), adapted to 32-bit and extended for
   bob's MFC/DirectPlay/VFW surface. Seeded into `SRC/compat/`.
3. **SDL2 (window/input/timing) + OpenGL (present the software framebuffer as a
   texture) + OpenAL (DirectSound shim)**, mirroring `ff`.
4. **MASM ASM** → assemble with a MASM-compatible assembler (`jwasm`/`uasm`)
   targeting `elf32`; fallback = translate to NASM. Editor-only ASM is skipped.
5. **Case-insensitive file IO shim** (`open_nocase`, like `ff`) — game data is
   mixed-case; Linux is case-sensitive.
6. Stub first, implement later: DirectPlay (multiplayer), DirectShow/VFW (intro
   videos), HTML Help → stubs to reach a running single-player build.

## Toolchain (NEEDS USER — no passwordless sudo here)

i386 multiarch is enabled; apt is reachable. Please run:

```bash
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib libc6-dev-i386 \
    nasm cmake ninja-build \
    libsdl2-dev:i386 libgl1-mesa-dev:i386 libglu1-mesa-dev:i386 \
    libglew-dev:i386 libopenal-dev:i386
# MASM-compatible assembler for the .asm files (try in order):
sudo apt-get install -y jwasm || sudo apt-get install -y uasm || echo "fallback: translate ASM to nasm"
```

(If the `:i386` dev packages conflict, the fallback is to extract `.deb`s into a
local `extern/` like `ff` does — see `ff/CLAUDE.md` "extern/usr".)

## Phased plan

- **Phase 0 — Foundation** *(in progress)*: recon ✓, `linux-port` branch ✓,
  compat seed ✓, this doc ✓, top-level CMake skeleton, toolchain install (user).
- **Phase 1 — Compat layer + leaf builds**: 32-bit CMake per module; assemble the
  ASM; get `MATH`, `LIB3D` compiling against the compat headers; grow the shim
  until each module's TU compiles.
- **Phase 2 — Link**: resolve/stub every undefined Win32/MFC/DirectX symbol;
  produce a linking (if not-yet-running) `bob` ELF.
- **Phase 3 — Runtime bringup**: SDL2 window + GL present of the software
  framebuffer; case-insensitive IO; reach the main menu.
- **Phase 4 — Subsystems**: input (SDL), sound (OpenAL), then in-sim rendering
  and gameplay, iterating like `ff` did (instant action → campaign).

## Status log

- **2026-06-08 (1)**: Reconnaissance complete; decisions above locked in. Created
  `linux-port` branch, `SRC/compat/` (seeded 24 headers from `ff`), `build/`,
  this doc. Blocked on toolchain install (handed to user).
- **2026-06-08 (2)**: Toolchain installed & verified — `gcc/g++ -m32` work;
  **SDL2 + OpenGL + OpenAL link in 32-bit** (smoke test passed). GLEW dropped
  (we only need a basic GL textured quad); jwasm/uasm unavailable -> ASM is
  translated to **NASM/elf32**.
- **2026-06-08 (3)**: **ASM porting approach proven end-to-end** (the riskiest
  unknown). `MATH/MATRASM.ASM` (MASM) hand-translated to `MATH/matrasm.nasm`
  (NASM/elf32) — assembles, exports `XASMTransform`/`XASMDoBigXProd`. A C test
  using the **GCC inline-asm wrapper** (`call X...` with `"a"/"d"/"b"/"c"` reg
  constraints, non-PIE) calls `XASMDoBigXProd` and returns **bit-exact correct**
  results. Locked in `-fno-pie -no-pie` (CMakeLists). This is the reusable recipe
  for all 8 `.asm` files + the ~30 inline-`_asm` sites.

- **2026-06-08 (4)**: **MATH module fully ported & building via CMake/Ninja** ->
  `libbob_math.a` (MATH.CPP + MATRIX.CPP + matrasm.nasm.o, 32-bit). Established
  the full repeatable methodology (see recipes below). Foundational headers now
  Linux-clean and used tree-wide: `mathasm.h`, `vector.h`, `modvec.h`,
  `mymath.h`, `dosdefs.h`, `worldinc.h`, `uidvals.g`.
  - **DOSDEFS.H**: added a `__GNUC__` branch that `#define`s `__MSVC__` (reuse
    the MSVC non-asm code paths), `BOB_LINUX`, `__forceinline inline`, and uses
    the system `FILE`/`<cstdio>` instead of the Windows `_iobuf`.
  - **Case-insensitive includes**: `fix_include_case.py` (953 symlinks); a
    one-shot pass rewrote `\`->`/` in `#include`s across 261 files.
  - **DOS Ctrl-Z (0x1A) EOF bytes** stripped from 41 source/header files.
  - **CMake**: `FF_LINUX` defined globally (the ff compat headers gate on it);
    nasm object-format set to elf32 *before* project(); C/CXX compile options
    wrapped in `$<COMPILE_LANGUAGE:C,CXX>` so they don't leak into nasm.
  - Two genuine MSVC-isms fixed: `mymath.h` redeclared `pow/exp` w/o noexcept
    (-> `<cmath>` under Linux); a cross-class friend named a private
    `mobileitem::MoveList` (made public). A corrupt `uidvals.g` (spurious
    `#endif`, mangled `UI_56`) was repaired.

### Build (current)
```bash
cd /home/m/bob/build && cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release && ninja
```
Per-file probe (fast iteration):
```bash
g++ -m32 -fno-pie -fpermissive -fno-strict-aliasing -fcommon -fpack-struct=1 -w \
  -DNDEBUG -DBOB_LINUX -D_LINUX -DFF_LINUX -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp \
  -Dstrcmpi=strcasecmp -ISRC/compat -ISRC/H -c SRC/<dir>/<FILE>.CPP -o /tmp/x.o
```

- **2026-06-08 (5)**: Tree-wide cross-cutting fixes + remaining-work survey.
  - **Opaque enum forward-decls**: MSVC allows `enum X;` (no underlying type);
    GCC needs `enum X : int;`. A script converted 22 such enums *and* added
    `: int` to their full definitions (else "underlying type mismatch", e.g.
    `Angles`, `PhraseTables`). Faithful — all values fit in int.
  - **Unity-build aggregators excluded** (8 files that `#include "SOURCE=.."`
    the individual .cpp's): `MATHU MISSP MOVEP 3D/3D FILEI GENEC AI/AI MODES`.
    We compile the individual TUs, so these are never built.
  - **Survey** (8 core dirs, unity excluded): ~23 pass / ~78 fail. The failures
    are dominated by **inline asm still to convert in shared headers**:
    `hardpasm.h`, `fastmath.h`, `polygon.h`, `myvector.h`, `keytest.h`, plus
    per-file `_asm`. Also: bob's 2-arg `assert(expr,str)` macro (MYERROR.H) vs
    the system 1-arg assert — needs a consistent BOB_LINUX policy; a few more
    opaque enums used pre-declaration (`KeyVal3D`, `MAXqueuesize`).

- **2026-06-08 (6)**: Converted the shared inline-asm headers that blocked the
  most files (recipe in "Inline-asm conversion recipe"):
  - `FASTMATH.H` (fpSqrt/FloatToInt -> `__builtin_sqrt`/`lrint`),
    `POLYGON.H` (`ASM_Call` -> GCC indirect register-ABI call),
    `MYVECTOR.H` (was a byte-identical dup of vector.h -> now `#include "vector.h"`),
    `HARDPASM.H` (VGA I/O-port + DPMI/DOS-int routines -> Linux stubs;
    `ASM_Blat`->memcpy, `ASM_Splat`->dword/byte fill),
    `KEYTEST.H` (`const int` + comment-suffixed opaque enum).
  - Other cross-cutting: `const NAME=val;` implicit-int (FileMan.h) -> `const int`;
    `__assume`/`_assume` -> no-op in DOSDEFS.H GNU block; comprehensive opaque-enum
    normalization (forward-decls + definitions get `: int`).
  - **Survey now 25/101 compile** (8 core dirs). The asm cascade is gone; the
    remaining ~76 fail on per-module C++ issues, not shared headers.
  - REMAINING per-module roots to chase next: bob's 2-arg `assert(expr,str)` macro
    vs system 1-arg (MYERROR.H — decide a BOB_LINUX policy); class-not-forward-
    declared (`ArtInt` in ai.h); incomplete-type uses (`MESSAGE_STRUC`); an
    `enum ImageMapNumber`/`PlaneTypeSelect` "underlying type mismatch" that only
    appears in full-include context (imagemap.h compiles standalone) — bisect the
    include chain (persons2.h/landscap.h/transite.h) to find the stray
    `enum X;` (likely macro-generated) that lacks `: int`.

- **2026-06-08 (7)**: **Solved the enum underlying-type problem** (a deep C++
  conformance issue that drove a 1500-error cascade) and settled several more
  cross-cutting roots. MATH still builds.
  - **Root cause**: MSVC allows opaque `enum X;` and auto-widens enum values;
    GCC (1) rejects opaque forward decls without a fixed underlying type, and
    (2) forbids mixing fixed/unfixed declarations of the same unscoped enum.
  - **Resolution (consistent across decl + def)**: every opaque-forward-declared
    enum is `: int` on BOTH its `enum X;` forward decl AND its definition —
    including the macro-generated ones: `MINMAX(name,min,max)` (DOSDEFS.H) now
    emits `enum name : int {...}` under BOB_LINUX. `: int` handles the negative
    MINMAX ranges (e.g. -32768) too. The dozen velocity range-types whose max
    sentinel was `2147483648` (INT_MAX+1, and unrepresentable in MakeField's
    `int` template param anyway) were clamped to `2147483647`.
  - **Do NOT** put `: int`/`: unsigned int` on *anonymous* enums or on enum
    definitions that have no opaque forward decl — GCC auto-sizes those
    correctly (incl. `0xffffffff` flag enums); forcing a type breaks them.
  - Also fixed: `assert(expr,str)` -> variadic no-op under BOB_LINUX (MYERROR.H);
    `INT3`/`NOP` -> no-ops (DOSDEFS.H); `__assume`/`_assume` no-ops; `class
    ArtInt;` forward decl (ai.h).
  - **Survey holds at ~25/101**: the cascade is gone, but the surveyed dirs sit
    HIGH in the dependency tree, so each remaining file fails on the FIRST of
    several independent issues (missing class forward-decls, the `INSTANCEAI`
    macro, `ItemBase`->`ItemPtr` C-style downcasts GCC rejects, incomplete-type
    `MESSAGE_STRUC` uses). These are per-file/per-module, not cross-cutting.
  - **Recommended pivot for next session**: stop surveying high-tree dirs; build
    bottom-up via CMake instead — pick the next *leaf* modules (LIB3D, then
    GENERAL/FILES) whose whole file set compiles, add each to SRC/CMakeLists.txt,
    and only climb the tree as dependencies come online. The per-file C++ fixes
    (forward decls, downcasts, INSTANCEAI) are then localized per module.

- **2026-06-08 (8)**: **LIB3D module — 5 of 6 files compile.**
  - Compiling: `L3DGUID.CPP`, `GETDXVER.CPP`, `3D/MONOTXT.CPP`, and `ALLOC.C` /
    `RADIX.C` (these are C — they use `new`/`old` as identifiers — so CMake
    builds them with `LANGUAGE C`). `FLAGSW.CPP` is excluded (it is `#include`d
    by LIB3D.CPP as inline `Lib3D::` method bodies). `SRC/LIB3D/CMakeLists.txt`
    written (not yet added to SRC/CMakeLists.txt — waiting on LIB3D.CPP).
  - Compat/header additions (help the whole tree): `interface`->`struct`
    (compat_types.h, for the DirectX interface headers); `__int64`/`__uint64`
    in the DOSDEFS.H GNU block; `<fstream.h>`/`<iostream.h>` shims; guarded out
    the unused `<dmusici.h>` (DirectMusic) include in GETDXVER (its probe is
    commented out anyway).
  - **LIB3D.CPP (the 18k-line software rasteriser): 196 -> 135 errors.** All
    inline asm converted (the hard part): cpu_id (stubbed -> generic non-MMX
    path), SetToTopBit, FloatToInt, SineCosine x2, fpSqrt, MagicRotate,
    MaskAndRot (color-channel rotates -> portable C), and the rdtsc TIMER macros
    (no-ops). Remaining 135, by category:
    * ~56 **MSVC for-loop-scope leaks** — a function declares `for(int i=...)`
      then reuses `i` in later `for(i=...)` loops; GCC scopes `i` to the first
      loop. Fix = hoist `int i;` (and sometimes `int j;`) to function scope in
      ~13 functions. NOTE: a regex hoist is UNSAFE here (for-loops used as
      unbraced if/while bodies, and double-declaration) — verified twice that it
      regresses; do these **surgically**, per function.
    * ~10 const-correctness (`const SVertex*&` bound to `SVertex*`).
    * DirectDraw compat gaps: `DDERR_NODRIVERSUPPORT`, `ChangeDisplaySettings`,
      `IDirectDrawGammaControl` (incomplete), gamma-control members.
    * 3 fstream-by-value (deleted copy ctor), 3 `RNDCOLOUR`->`DNDCOLOR` ambiguous
      conversion, `va_start`/`va_end` (need `<cstdarg>`).

- **2026-06-08 (9)**: **LIB3D module BUILDS** -> `libbob_lib3d.a`, wired into
  SRC/CMakeLists.txt next to MATH. `ninja` now produces both static libs.
  LIB3D.CPP (18k-line rasteriser): **196 -> 0 errors**. Finished what (8) started:
  ~60 for-loop-scope leaks hoisted surgically; ClipSetCols const-correctness
  (decl+def); R3DCOLOUR ambiguity (drop value `operator ULong()`, keep ref one)
  + reinterpret-cast at SetColLighting sites; getNextToken fstream-by-ref; `0i64`
  -> `0LL`. New **`compat/bob_dx_extra.h`** supplies the missing Win32/DDraw/D3D
  symbols (D3DERR_* codes, DEVMODE, DDGAMMARAMP, IDirectDrawGammaControl, the
  DLL_/DM_/CDS_ constants, ChangeDisplaySettings/EnumDisplaySettings/StretchDIBits
  /_i64toa stubs, IIDs) and is pulled in from windows.h. Adding `#include
  <windows.h>` to LIB3D.CPP (it only had ddraw/d3d/objbase) cleared ~59 at once.
  `ALLOC.C` `new` identifier renamed; the .C files build as C++.

  **Two modules now build: bob_math + bob_lib3d.** General-purpose recipes that
  recur tree-wide and are now proven: inline-asm conversion, for-scope hoisting
  (surgical only), MSVC opaque enums, `interface`/`__int64`/`0i64`/`i64toa`
  -isms, and the bob_dx_extra compat header for DirectX symbols.

- **2026-06-08 (10)**: **Phase 2 underway — AIRCRAFT is the 3rd module to build**
  (`libbob_aircraft.a`). Cross-cutting fixes this batch unblocked it entirely and
  lifted AI/MODEL/3D: MSVC calling-convention keywords (`__cdecl` etc.) in the
  DOSDEFS GNU block; member-function-TYPE typedefs `typedef R(Class::Name)(args)`
  -> `typedef R(Name)(args)` (VIEWSEL.H/COLLIDED.H); `PlaneTypeSelect` def given
  `: int` (FLYINIT.H, comment-suffixed); `=NULL` pure-specifier -> `=0`; compat
  shims `winerror.h`/`ole2.h`; `Vfw.h` case-alias.
  - **De-duplicated real per-dir status** (earlier counts were inflated by the
    case-alias symlinks; survey now skips `-L`): AIRCRAFT 8/8 ✓, MISSMAN 8/17,
    3D 7/30, MODEL 6/21, FILES 4/7, GENERAL 2/4, AI 1/7, MOVECODE 1/11,
    HARDWARE/BFIELDS low. MFC/ (454 files, the game core) not yet surveyed.
  - **Identified next roots** (Phase 2 grind):
    * **~13 files: `_asm` blocks** in game .cpp (per-file conversion; 28 game
      .cpp files contain inline asm total).
    * **bit-field-overlay template conflict** — `replay.h`/`persons2.h` use
      `LASTFIELD`/`MidField`/`Overview` macros (BITCOUNT.H) to build bit-packed
      unions; GCC reports `Overview ... conflicts with a previous declaration`.
      Deep template issue MSVC tolerated; affects ~13 files. Handle carefully.
    * **`Wrapper.h`** missing — actively included by the legacy VGA HARDWARE
      files (HARD320/HARDVBE*, the pre-DirectDraw software renderers). Likely
      stub it, or these may be excludable deadcode for the DirectDraw build.
    * **`Vfw.h`** (Video for Windows) — alias added; `vfw.h` compat may need
      more for the movie/intro path. Token-pasting `##` macro issues (Landscap).

- **2026-06-08 (11)**: **FILES is the 5th module to build** (libbob_files.a:
  DOSFILE/LOADLIST/DISKIO/LOADLIB/WINFILE/FILEMAN). Five module libs now build:
  math, lib3d, aircraft, general, files.
  - FILEMAN: `FileMan : public fileman` on Linux (codebase calls
    `FILEMAN.publicMethod()` externally, only valid with an accessible base;
    MSVC was lax); `eip()`/`esp()` -> `__builtin_return_address`/
    `__builtin_frame_address`; `_set_new_handler` guarded; implicit-int consts.
  - More cross-cutting roots cleared: `_WINBASE_` marker (compat winbase headers;
    bob gates Win32 structs on it), `stub3d.h` atomic xchg (18 files),
    `ole2ver.h` shim, FASTMATH for-scope, SHAPES `fileblock` fwd-decl, `__cdecl`,
    member-func-type typedefs, etc.
  - **KNOWN HARD BLOCKER (deferred): the bit-field overlay** (BITCOUNT.H
    FIRSTFIELD/BITFIELD/LASTFIELD via replay.h/persons2.h, ~13 files). Each
    FIRSTFIELD needs an `Overview` typedef (= storage size) visible to its
    BITFIELD/LASTFIELD. Original puts it in the anonymous union (GCC: "Overview
    conflicts"); hoisting to the struct (current BOB_LINUX) fixes single-
    FIRSTFIELD structs but multi-FIRSTFIELD ones (e.g. _asprim_values) hit
    "member typedef redeclaration". `-fms-extensions` doesn't help. The clean fix
    is to rework the macros to thread the storage type through each field macro
    (or generate a unique-per-union typedef) WITHOUT editing every BITFIELD call
    site in the headers - needs a focused, careful pass. This gates a chunk of
    AI/MODEL/MISSMAN.

- **2026-06-08 (12)**: **Bit-field overlay blocker SOLVED** (commit ba832b3).
  The only multi-FIRSTFIELD struct is replay.h's `_asprim_values` (two UByte
  unions). Added a **NEXTFIELD** macro: a 2nd+ FIRSTFIELD-style union that, on
  Linux, reuses the struct-scope `Overview` typedef the first FIRSTFIELD hoisted
  (avoids the member-typedef redeclaration); on MSVC it's just FIRSTFIELD.
  WORLDINC.H's 7 FIRSTFIELDs are each in their own single-FIRSTFIELD struct, so
  the hoist alone covers them. Clears bitcount.h for the replay/persons2 cluster.

- **2026-06-09 (13)**: **Two more modules build — INPUT (6th) and 3D (8th, 4/5)
  — plus a partial MISSMAN (7th, 7/10). Eight module libs total.** Also a major
  **survey-methodology fix**: many BoB .cpp are *fragments* (zero `#include`,
  pulled into a unity aggregator like MFC/_MFC.cpp); the survey now excludes both
  unity aggregators AND zero-include fragments, so per-module counts reflect real
  standalone TUs.
  - **INPUT** (ANALOGUE + KEYLIST; KEYSTUB is a _MFC fragment). Compat additions
    that recur tree-wide: `IN`/`OUT`/`OPTIONAL` SAL macros + `FIELD_OFFSET`
    (compat_types.h); `DECLARE_INTERFACE`/`_` COM macros (objbase.h); joystick
    API JOYINFO(EX)/JOY_*/joyGetPos* (mmsystem.h ×2); DInput A-aliases
    (IDirectInputDevice2A/7A/8A as #defines so `struct X;` fwd-decls still work),
    DIDOI_* flags, DIEB_NOTRIGGER (dinput.h).
  - **Cross-cutting roots (high leverage, no regressions):**
    * **string ambiguity** — the iostream.h/fstream.h shims did `using namespace
      std`, dragging in std::string which collided with BoB's own
      `typedef char* string` (dosdefs.h) → every `string&` param ambiguous.
      Replaced with selective `using std::<stream-name>` (VC6's <iostream.h>
      exposed stream names globally but NOT std::string). Faithful + fixes it
      tree-wide.
    * **old-iostream BSTREAM.H** — BOB_LINUX branches: openmode `+`->`|`
      (operator+ on ios::openmode decays to int, no matching open()); emulate the
      MSVC `ios::noreplace` extension (fail-if-exists) via an existence check.
    * **PROF.H** — all 4 inline-_asm 64-bit timer routines -> portable C
      (ht:lt add/sub, /1000 quotient+remainder, rdtsc via __builtin_ia32_rdtsc).
    * legacy un-prefixed keywords `pascal`/`_pascal`/`cdecl` -> no-ops (DOSDEFS).
    * `DAM(...)` variadic trampoline (MSVC fills omitted macro args empty).
  - **MISSMAN** 7/10 (DEBRIEF/INTRMISS/NODEBOB/NODEKILL/ONEMISS/PEACMISS/
    SO51MISS). NODEBOB also needed 2 for-scope hoists + `SUBCALL` macro
    `assert(this)`->`assert(this);` (the -DNDEBUG assert is `((void)0)`, an
    expression, not the {}-statement form MSVC used). NODEBOB.H now
    `#include "package.h"` (struct Profile::PackageStatus/BetterRule).
  - **3D** 4/5: LANDSCAP done (InterpLight `->##p1` paste, fpSqrt/fpTan/fpSinCos
    asm, for-scope ×6, abs(unsigned) cast, `Shape.newco`->`Shape::newco` static).

- **2026-06-09 (14)**: **Four more modules build — AI (9th), MOVECODE (10th),
  MODEL (11th), HARDWARE (12th). Twelve module libs total.** Two big methodology
  shifts this batch:
  - **Filter to BOB.DSP game files (303 TUs).** Many .cpp in the tree are
    editor/tool/deadcode NOT in the game exe (CEDITOR, MEGLOBAL, HARD320*,
    PERSONS5, vcl/devstudio-path files). The survey now skips anything whose
    basename isn't in BOB.DSP — stops wasting effort on non-game files.
  - **Compile the `_XXX.CPP` unity per module, not individual TUs.** For modules
    whose DSP build uses a unity aggregator (`_MODE`/`_MOVE`/`_HARD`/`_COMM`/
    `_BFIE`...), THAT unity is the faithful, link-complete build unit: it pulls
    the fragment files (which have no own #includes), gives every fragment the
    include context the others established, and avoids duplicate symbols.
    Dramatic effect — MODEL's 16 fragments compiled as a unity reduced to **2
    errors** total; HARDWARE 52->0, MOVECODE clean, once the standalone roots
    were cleared. (NOTE: the earlier modules MATH..AI/MISSMAN/3D were wired from
    standalone TUs and are therefore link-INCOMPLETE — they miss fragment-only
    files; revisit by switching them to their unities for the Phase-2 link.)
  - **Cross-cutting roots (no regression to the 12 libs):**
    * **NODEBOB.H now #includes uniqueid.h** — cleared the `UniqueID has not been
      declared` cascade from package.h/nodebob.h/bfnumber.h **tree-wide (111->1)**.
    * **assert/nassert under BOB_LINUX expand to NOTHING** (was `((void)0)`) —
      handles bob's no-semicolon `assert(x) if(...)` and unbraced
      `if(c) assert(x); else` idioms that `((void)0)` breaks.
    * **compat LONG = `long`** (Win32 ABI; was int32_t) — fixes the
      int32_t-vs-long conflicting-typedef vs cstring.h.
    * MIDI output API stubbed (mmsystem.h); DirectSound ANSI aliases +
      IDirectSound_* C-macros (dsound.h); CLSCTX_INPROC/IStream/LPSTREAM
      (objbase.h); __RPC_FAR/FAR/NEAR (compat_types.h); raddef.h-before-radio.g
      pattern for USE_PHRASE_* aircraft enums.
  - Recurring per-file idioms now well-understood: **static-member-via-type** the
    MSVC `.`-on-a-typename laxity — `Shape.newco`/`TimerCode.FRAMETIME`/
    `mobileitem.ACList`/`LandScape._blockWidth` -> `::`; **for-scope hoists**;
    **FPU asm** -> `__builtin_sqrt/atan2/sin/cos`.

- **2026-06-09 (15)**: **COMMS is the 13th module** (via `_COMM.CPP` unity).
  DirectPlay stubbed (minimal IDirectPlayLobby3 in compat/dplobby.h — the empty
  compat stub had been shadowing the real SDK header). **compat DWORD/ULONG are
  now `unsigned long`** (Win32 ABI, matches bob's ULong) — fixes ULong&/DWORD
  bind mismatches, no regression. DEFINE_GUID declaration-form made variadic
  (1-arg `DEFINE_GUID(BOB_GUID)` MSVC empty-fill). BAD_RV implicit-int extern;
  implicit-int statics; n for-scope hoists. **Thirteen module libs build.**
  Remaining non-MFC: BFIELDS (bfrefs.g corruption), GRAPHICS (asm-only),
  MYCMDS (empty). Then the MFC game core.

### Deferred / known work
- **BFIELDS**: blocked by a **corruption in generated header SRC/H/bfrefs.g**
  (~line 209: a GR_Pack_TakeTime table's declaration + first entries are missing,
  leaving a dangling `eTime_W2G1},`). Present in the original imported source.
  GLOBREFS pair/pair04 macro fixes are already committed; module needs bfrefs.g
  regenerated/reconstructed before it can build.
- **COMMS** (_COMM.CPP, ~41 errs): DirectPlay-heavy (IDirectPlayLobby3 incomplete,
  ULong&/DWORD bind, FILE_ATTRIBUTE_*, BOB_GUID). Multiplayer — deferred to stubs
  per the phased plan.
- **MFC game core (177 .cpp in SRC/MFC) — THE major remaining work.** Scoped this
  session: `compat/afxwin.h` (and afx.h/afxext/afxcmn/...) are **empty stubs** —
  no CWnd/CDC/CFont/CDialog/CWinApp/CView/message-maps. **FreeFalcon's afxwin.h is
  ALSO empty (FF didn't use MFC), so there is no MFC layer to borrow** — this is a
  from-scratch MFC compat buildout. Groundwork done: stubbed the remaining afx
  umbrella headers (afxole/afxodlgs/afxauto/afxpriv/afxmt/afxdisp/afxtempl) and
  added case-alias symlinks for the 17 `.cpp` fragments `_MFC.CPP` #includes
  (MIGView.cpp->MIGVIEW.CPP, etc.). `_MFC.CPP` now reaches `resource.h` + the
  `#error include 'stdafx.h'` PCH guard (MIG.h gates on `#ifndef __AFXWIN_H__`) —
  past those lies the real flood of undefined MFC classes. Add `-ISRC/MFC` to the
  MFC target include path (resource.h lives there).
  - **Exact MFC surface to implement** (from a `: public CXxx` survey): base
    classes **CWnd (15 derived), CDialog (6), CWinApp, CView, CFrameWnd,
    COleDocument, COleDispatchDriver, CCommandLineInfo** + roots **CObject,
    CCmdTarget, CWinThread**; GDI **CDC, CFont, CGdiObject, CPen, CBrush,
    CBitmap**; value types **CRect, CPoint, CSize** (RECT/POINT already in
    compat_types.h); **CDataExchange**; no-op message-map macros
    (DECLARE_MESSAGE_MAP, BEGIN/END_MESSAGE_MAP, ON_*, DECLARE_DYNCREATE,
    IMPLEMENT_*). afxwin.h must `#define __AFXWIN_H__`. **CString is bob's own
    (SRC/H/cstring.h), NOT MFC.** Also need a **streams.h** (DirectShow) stub.
    Back the layer with SDL/GL at runtime later; covers 3D/OVERLAY too.
- **MISSMAN** PACKAGES/SAVEGAME/UIMSG, **AI** MSGAI/USERMSG: rchatter.h not
  self-contained, incomplete MissMan/CString, SECSPERMIN/Directives::RAF. (Most
  of these likely resolve when built via the module unity rather than standalone.)
- **ACMSIMPL GentleBankData**: used in 3 sites, defined nowhere in the tree —
  extern-declared for now; **Phase-2 link TODO**.

### NEXT ACTIONS (resume here)
1. **Switch the standalone-wired modules to their `_XXX.CPP` unities** for
   link-completeness (AIRCRAFT/MISSMAN/3D/AI/INPUT — MATH/LIB3D/GENERAL/FILES are
   small/likely complete). This both fixes missing fragment code AND tends to
   reduce errors (shared context).
2. **MFC game core (177 .cpp)** — the big remaining chunk. Build a from-scratch
   minimal MFC compat layer in `compat/afxwin.h` (CObject/CCmdTarget/CWnd/CDialog/
   CDC/CFont/CWinApp/CView/CDocument/CFrameWnd + no-op message-map macros), since
   neither bob's nor ff's afxwin.h has it. Then `_MFC.CPP` + the standalone MFC
   TUs. (afx stubs + `_MFC.CPP` cpp case-symlinks already in place.)
3. Reconstruct/​regenerate **bfrefs.g** to unblock BFIELDS.
4. Then **Phase 2 (link)**: assemble remaining `.asm` (GRAPHICS/GRAFPASM,
   3D/LSTRASM, HARDWARE/*) to nasm; resolve undefined symbols (GentleBankData,
   DirectPlay/DirectShow stubs); produce the `bob` ELF.

- **2026-06-09 (16)**: **MFC compat FOUNDATION built** (`compat/afxwin.h`,
  `compat/streams.h`). afxwin.h is now a real minimal MFC: `__AFXWIN_H__` define;
  no-op message-map/runtime-class macros (DECLARE/BEGIN/END_MESSAGE_MAP, ON_*,
  DECLARE_DYNCREATE, IMPLEMENT_*, afx_msg, RUNTIME_CLASS); value types CRect/
  CPoint/CSize (on RECT/POINT/SIZE); the class hierarchy CObject -> CCmdTarget ->
  {CWnd -> CDialog/CView/CFrameWnd, CDocument -> COleDocument, CWinThread ->
  CWinApp}; GDI CGdiObject -> CDC/CFont/CPen/CBrush/CBitmap; plus CCommandLineInfo,
  CDataExchange, COleDispatchDriver, AfxGetApp/AfxGetMainWnd. streams.h is a
  minimal DirectShow stub (IGraphBuilder/IMediaControl/IVideoWindow/IMediaEventEx/
  IBasicAudio + CLSIDs) so FULLPSYS.CPP (intro movies) compiles; video deferred.
  - **Result: `_MFC.CPP` no longer has ANY missing-MFC-base-class errors** — the
    foundation resolves. It now shows ~2655 BoB-specific errors (down from "no MFC
    at all"), so MFC is now normal per-root grinding like every other module:
    top roots = map-tile macros `MAP_<N>` (~900, a macro not expanding),
    FIL_MAP* enums, BoB's own UI classes incomplete (RDialog/CRButton/CSystemBox —
    ordering), OLE types (OLE_COLOR, VT_*). Compile the MFC module with
    **`-ISRC/MFC`** (resource.h). 13 module libs still build (afxwin/streams are
    not included by them).
  - **The ~900 `MAP_<N>` errors all cascade from `FIL_MAP_TABLE`** (frmap2.h),
    which is **defined nowhere in the tree** ("did you mean FIL_MAP_xARMY" — the
    sibling map file-enums exist but not this one) — another genuinely-missing
    symbol like GentleBankData/BOB_GUID, likely from an uncommitted/generated
    files.g. Resolve before the MAP table will compile. The MFC grind also has
    tractable cross-cutting roots to clear first: OLE_COLOR (typedef = DWORD) and
    VT_* VARIANT constants (add to a compat oleauto/wtypes), then the per-file
    ordering of bob's own RDialog/CRButton/CSystemBox UI classes.

- **2026-06-09 (17)**: **MFC module grind — `_MFC.CPP` 2655 -> 723 errors** on the
  built MFC foundation. Compile the MFC module with **`-ISRC/MFC`**.
  - **afxwin.h fleshed out**: common control wrappers (CStatic/CButton/CEdit/
    CListBox/CComboBox/CScrollBar/CToolBar), container templates CArray/CList
    (std-backed; CList POSITION iteration stubbed empty — UI lists not driven
    yet), diagnostic macros (ASSERT/VERIFY/TRACE*), OLE event/dispatch map macros
    (BEGIN_EVENTSINK_MAP/ON_EVENT/DISP_*/VTS_*), CRect operators, and many
    CDC/CWnd/CWinApp/COleDispatchDriver methods. POSITION/CCreateContext types.
  - **compat_types.h**: OLE base types (OLE_COLOR/VARENUM VT_*/BSTR/VARTYPE/
    LPDISPATCH/DATE/...). **compat_wingdi.h**: PS_*/TRANSPARENT/MM_TEXT/R2_*/
    GetDeviceCaps indices/DT_* flags.
  - **The file-enum map cascade fix** (~1289 errors): `_MFC.CPP` now defines
    F_BATTLE and force-includes dosdefs.h + files.g at the top, locking FileNum
    with F_COMMON.G(FIL_MAP_TABLE)+F_GRAFIX.G+F_SOUNDS.G before any fragment's
    own files.g (a fragment defined F_COMMON first, excluding the map enums).
  - **PACKAGE.H now #includes uniqueid.h** (self-contained, like nodebob.h) —
    cleared the UniqueID cluster. `_MFC.CPP` early-includes uniqueid/cstring/
    rdialog/rbutton so the dialog/map fragments see bob's own UI base classes
    (their headers don't self-include them).
  - **_MFC.CPP driven 723 -> 301** (cumulative 2655 -> 301). Cleared this pass:
    globdefs.h ON_MESSAGE map-builders no-op'd; cstring.h included BEFORE stdafx
    (so CString is complete when __AFX_H__ flips bob's "MFC present" branches);
    afxwin.h grew controls (CButton/CEdit/CListBox/CComboBox/CScrollBar/CMenu/
    CToolBar), CArray/CList(+POSITION), CFile/CArchive/CPrintInfo, CRect/CPoint/
    CSize arithmetic, OLE-ctl factory/proppage/connection macros, CDC CString-
    template text methods, AFX_CMDHANDLERINFO/HELP_*/AfxLoadString; the file-enum
    map-cascade fix; PACKAGE.H self-contains uniqueid.h; _MFC early-includes the
    bob UI base headers (rdialog/rbutton/rlistbox/rmdldlg/maintbar/titlebar/
    sysbox/hintbox) + case-alias symlinks for them.
  - **_MFC.CPP driven 301 -> 172** (cumulative 2655 -> 172, ~94%). Added: full
    CWnd message-handler virtuals (OnLButtonDown/OnMouseMove/OnPaint/...), all the
    MFC ON_WM_*/ON_*N map-entry macros as no-ops, CMenu/CFile/CArchive/CPrintInfo/
    CPropExchange, CDC GDI methods (Polygon/Ellipse/ExtTextOut CString overloads),
    CRect/CPoint arithmetic, NMHDR/MINMAXINFO/HELPINFO/HTASK, SIZE_*/TPM_*/QS_*/
    CBRS_*/DISPATCH_*/HID_* consts, AfxLoadString/HELP_*; cstring.h moved before
    stdafx (CString complete when __AFX_H__ flips bob's branches); globdefs.h
    ON_MESSAGE no-op'd; many bob UI headers early-included.
    COleControl base class (bob's CR*Ctrl ActiveX impls derive from it).
  - **Control classes resolved**: they ARE fully defined in headers (RSTATIC.H/
    RCOMBO.H/...); the earlier "only in .cpp" read was wrong — it was include
    ORDER (composite CRComboExtra:public CRCombo seen before CRCombo). _MFC.CPP
    now early-includes leaf control wrappers before composites.
  - **Remaining ~172 = a flat 1-2-per-root tail** spread across MainFrm/RDialog/
    MIGView/MIG/MapDlg: per-file bob symbols (wpacnoactionno/MAPFILTERSMAX/
    m_msgCur/pDocTemplate/...), a couple of HWND-deref sites, one inline `_asm`,
    scattered missing CWnd/Win32 methods/consts. Then the other ~150 standalone
    MFC .cpp beyond `_MFC.CPP`. Recipe is mechanical: run _MFC, add the next
    method/macro/const to afxwin.h or early-include the bob header, repeat.
    **13 module libs keep building — afxwin/streams aren't included by them, so
    all MFC work is regression-safe.**

- **2026-06-09 (18)**: **MFC game-core unity `_MFC.CPP` COMPILES CLEAN (2655 -> 0)
  — MFC is the 14th module lib.** The from-scratch MFC compat layer
  (compat/afxwin.h ~700 lines + streams.h) is now complete enough to compile the
  game's main MFC TU (MIGView/MainFrm/MapDlg/MIG/MIGDoc/RDialog/Keystub/fullpsys).
  Fourteen module libs build: math, lib3d, aircraft, general, files, input,
  missman, 3d, ai, movecode, model, hardware, comms, **mfc**.
  - The long-tail recipe that got the last ~300: add the next MFC class/method/
    macro/const to afxwin.h (or a Win32/GDI/OLE const to compat_{winuser,wingdi,
    types}.h); early-include the bob UI header in `_MFC.CPP` (leaf control
    wrappers before composites; +case-alias symlink); make bob data headers
    self-contained (package.h->uniqueid/movement, nodebob.h->uniqueid,
    missman2.h->savegame, _mfc.h->enumbits.m/flyinit.h); fix per-file MSVC-isms
    (member-fn-ptr `&Class::`, for-scope hoists, temp->non-const-ref via by-value
    params, `IconsUI : unsigned int`, static-member-via-type `::`). Key gotchas:
    A-macros (`TextOut`->`TextOutA`) clobber member methods (don't define both);
    VTS_* must be string literals not NULL; CFile must be fwd-declared before CWnd
    (else `CFile*` -> `int*` under -fpermissive); generated wrapper headers
    (rscrlbar.h) lack include guards (add for unity).
  - **Compile the MFC module with `-ISRC/MFC`** (resource.h). afxwin/streams are
    NOT included by the other 13 modules, so the whole MFC effort was
    regression-free.
  - **NEXT**: survey + build the other standalone MFC `.cpp` TUs (beyond the
    _MFC.CPP unity); they now have the full MFC layer available. Then Phase 2
    (link the `bob` ELF): assemble GRAPHICS/GRAFPASM + 3D/LSTRASM + HARDWARE asm,
    resolve undefined symbols (GentleBankData, BAD_RV, DirectPlay/DirectShow
    stubs), reconstruct bfrefs.g for BFIELDS.

- **2026-06-09 (19)**: **MFC unity landscape mapped + shared prelude; `_AFX.CPP`
  also clean (2 of 7 MFC unities archived).** The MFC module has **7 unity TUs**
  (BOB.DSP): `_MFC`(core dialogs/views/map) ✓, `_AFX`(controls/font) ✓, and the
  campaign UI: `_TOOL`(9 frags), `_FULL`(42), `_SA`(49), `_LW`(20), `_RAF`(20).
  The ~133 "standalone MFC .cpp" are actually these unities' fragments.
  - **Shared preludes `SRC/MFC/bob_mfc_pre.h` + `bob_mfc_post.h`** (extracted from
    _MFC.CPP, applied to all 7 unities): pre = F_BATTLE + files.g + cstring before
    stdafx; post = bob UI/data/frame headers after _mfc.h (control wrappers ->
    composites -> redit/fullpane/MainFrm/MIGView). **Must live in SRC/MFC** so
    quote-include resolution (resource.h etc.) matches the inline form. Effect:
    _AFX 164->0, _RAF 841->27, _TOOL 529->99, _LW 1175->152, _FULL 2376->340.
  - **libbob_mfc.a now archives `_MFC.CPP` + `_AFX.CPP`.** Added CY/CURRENCY (OLE),
    COleDispatchDriver(LPDISPATCH) ctor; include guards on generated wrappers
    RSCRLBAR.H/REDIT.H (unity double-include); ~70 case-alias .cpp symlinks for the
    unity fragments.
  - **NEXT (campaign-UI unities, ~27-340 each)**: the remaining errors are
    cross-unity dialog references (_RAF instantiates LWRouteMain/RAFDiaryDetails;
    `::Place`) + per-fragment bits (info_airgrp/info_waypoint incomplete,
    `PT_LWTOTAL` non-constexpr enum-arith, IdList& temp-binds, resource.h IDC_*
    redef). Recipe: add the cross-referenced dialog header to bob_mfc_post.h (test
    it doesn't regress _MFC/_AFX), or fix per-fragment. Then wire each clean unity
    into SRC/MFC/CMakeLists.txt. Then Phase 2 (link the ELF).

- **2026-06-09 (20)**: **`_RAF.CPP` COMPILES CLEAN (28 -> 0); shared fixes cascade
  to every campaign unity.** `libbob_mfc.a` now archives `_MFC + _AFX + _RAF`.
  Five roots — mostly in shared headers, so the others fell for free:
  **_SA 2072->46, _LW 152->37, _FULL 340->276, _TOOL 99->96** (no _MFC/_AFX regress).
  1. **info_airgrp/info_waypoint were forward-decl-only everywhere.** Their full
     defs (`infoitem.h` lines 100-346) are gated on `#ifdef BFNUMBER_Included` and
     use `EventVal` bit-field members. `infoitem.h` is first reached during
     `_mfc.h` (via persons2.h) *before* post.h, so `INFOITEM_INCLUDED` was set with
     the body skipped. Fix in **bob_mfc_pre.h**: include `uniqueid.h` then
     `bfnumber.h` (defines BFNUMBER_Included + EventVal; pulls only bfenum.h, NOT
     the deferred-corrupt bfrefs.g) ahead of the _mfc.h chain — the bit-field
     module enters the build narrowly, through the front door.
  2. **Dialog-layout temp->non-const-ref** (rdialog.h): the `DialBox(DialBox&)`
     copy ctor, `DialList(DialBox& d,...)`, `HTabBox(...,IdList&,Edges&,...)` and
     VTabBox variants took non-const refs but are always passed temporaries. Made
     them `const` (+ `HTabBox::titles` -> `const IdList*`). The protected internal
     `DialList(d0..d7)` then tied with the public ctor at 8 args -> disambiguated
     with a `ChildrenTag` enum (`DialList(CHILDREN,...)`). The 10 cascading
     "expected primary-expression before '('" were just fallout of `new
     LWRouteMain`/`LWReviewAircraft`/`RAFDiaryDetails` (undefined classes).
  3. **Cross-unity dialog classes** used before their own fragment's .cpp: added
     `infoitem.h` + `LWRouteM.h`/`LWRevAc.h`/`RAFDryD.h` to bob_mfc_post.h.
  4. **`PT_LWTOTAL = PT_HE59 - PT_GER_FLYABLE`** (RAFDryD.h:37) invoked the MATHABLE
     runtime `operator-` in an enum initializer -> cast both operands to `(int)`.
  5. **`ON_EVENT_RANGE` undefined** (only ON_EVENT existed) -> no-op macro
     (afxwin.h); for-scope `i` hoist (RAFRevCl.cpp:163).

- **2026-06-09 (21)**: **`_LW.CPP` (37->0) and `_SA.CPP` (46->0) COMPILE CLEAN —
  5 of 7 MFC unities archived** (`libbob_mfc.a` = _MFC+_AFX+_RAF+_LW+_SA). Only
  `_TOOL` (82) and `_FULL` (275) remain. Same playbook: undefined cross-unity
  dialog classes + a few per-fragment MSVC-isms. Roots:
  - **More cross-unity dialog headers -> bob_mfc_post.h**: AcUnit.h (AircraftUnit
    +TypesToList), WPDialog.h, RAFRevAs/RAFRevAc.h, LWTaskSm/LWDiaryD/LWDiary.h
    (for _LW); SquadDtl/GrpGesch/AfDetl.h, Load.h (LSD_State enum), MapFltLw.h
    (for _SA). Again the bulk of each unity's errors were "expected
    primary-expression" cascades behind one undefined `new <Dialog>`.
  - **GDI gaps (compat)**: added ExtCreatePen geometric styles (PS_GEOMETRIC/
    PS_ENDCAP_*/PS_JOIN_*), hatch styles (HS_*), the `LOGBRUSH` struct
    (compat_wingdi.h), and a 4-arg `CPen(int,int,const void*,int)` ctor (afxwin.h)
    for `CPen penf(PS_GEOMETRIC+..,THK,&logbrush,0)` in clock.cpp.
  - **DialBox copy ctor was `protected`** but afdossr.cpp materialises a DialBox
    temp from a `cond ? DialBox(..) : *ND` ternary in non-derived code (MSVC
    allowed the protected access). Moved the copy ctor into the public section
    (kept the default ctor protected). Widens access only; no regression.
  - per-fragment for-scope `i`/`actype` hoists (LWRevCl, LWTaskFr, LWTaskSm,
    lwdirect). **NEXT: _TOOL (82) then _FULL (275); then Phase 2 (link ELF).**

- **2026-06-09 (22)**: **ALL 7 MFC UNITIES COMPILE CLEAN — the MFC module is
  DONE.** `libbob_mfc.a` archives _MFC + _AFX + _RAF + _LW + _SA + _TOOL + _FULL.
  - **_TOOL (82->0)**: it's the top-level toolbar/navigator, so it instantiates
    ~every campaign dialog -> added ~37 dialog headers to bob_mfc_post.h. The rest
    were genuine MFC-layer gaps added to **afxwin.h**: CWnd virtual handlers the
    fragments forward to via `Base::` (OnInitMenu/OnInitMenuPopup/OnSetFont/
    OnCancelMode/OnFinalRelease/PreSubclassWindow/OnChildNotify/OnCharToItem/
    OnAmbientProperty), CWnd `IsZoomed`/`WinHelp`/`m_pCtrlSite`/`m_nIDHelp`/static
    `WindowFromPoint`; CObject `IsKindOf`; CDC `SelectObject(CPen&)`; CList `SetAt`;
    `ON_WM_CANCELMODE`/`ON_WM_CHARTOITEM` map macros. (CRToolBar : CDialog : CWnd,
    so the CWnd additions resolve the `CRToolBar::OnInitMenu` base calls too.)
  - **_FULL (275->0)**, the biggest:
    * **~95 member-function-pointer table entries** written bare (MSVC extension):
      `{IDS_x,&screen, SomeMemberFn}` where the field is `SelProc`/`Proc` (=
      `Bool (RFullPanelDial::*)(...)`). GCC needs `&RFullPanelDial::`. Fixed with a
      guarded perl across 8 fragments (fplayout/fpconfig/fullpane/commsac/credits/
      Radio/Sdetail/TwoDPref): prefix a name only when it's a value reference —
      `(?<![:\w])NAME(?!\s*\()` — which skips the `RFullPanelDial::NAME(){...}`
      definitions living in those same files.
    * **~50 more cross-unity dialog headers** (CSQuickLine=SQUICKUN.H, CREdtBt,
      the C*-named game-option dialogs APILOT/SCAMP/SDETAIL/SFLIGHT/SGAME/SSOUND/
      SVIEWER, service/session, EndDayR*, GameSelt, SController, SMission, TwoDPref,
      SideSel/PhsDscr/EndDy*) -> bob_mfc_post.h.
    * **Win32 gaps -> compat/bob_dx_extra.h**: display-settings consts
      (DM_BITSPERPEL/DM_DISPLAY*, ENUM_CURRENT_SETTINGS, DISP_CHANGE_*, CDS_TEST);
      Shell AppBar API (APPBARDATA, ABM_*/ABE_*/ABS_*, SHAppBarMessage); version-
      resource (VS_VERSION_INFO/VS_FIXEDFILEINFO/RT_VERSION + FindResource/
      LoadResource/LockResource/GlobalSize).
    * per-fragment: `static currmode=` implicit-int -> `static int`; for-scope
      `i`/`m`/`wave` hoists.
  - No regressions: all 7 unities verified 0 at each step.

- **2026-06-09 (23)**: **Phase 2 link surface scoped.** All 15 module libs build
  (53 unity .o covering ~302 game TUs). A trial whole-archive link of every
  `libbob_*.a` (`ld --whole-archive ... --allow-multiple-definition`) yields
  **574 distinct undefined symbols** (12k refs). Breakdown:
  - **Deferred project TUs are the dominant gap.** The undefined globals/methods
    are defined in TUs explicitly skipped in module CMakeLists:
    * AI: **MSGAI.CPP** (defines `ArtInt Art_Int;` @114 + many AirStruc::/ArtInt::
      methods), **USERMSG.CPP** — deferred: need missman2.h, incomplete Model/anim
      types, rchatter.h ordering.
    * MISSMAN: **PACKAGES.CPP / SAVEGAME.CPP / UIMSG.CPP**.
    * 3D: **OVERLAY.CPP** (needs CDC/CFont + GDI GetGlyphOutline).
    * SRC/MFC **STUB3D.CPP / BOBFRAG.CPP** are not in any `_*.CPP` unity yet
      (they hold the `Aircraft_Formations`/`Anim_Control`/`fastMath` tentative defs
      — currently only `extern`-referenced everywhere → `U`).
    Un-deferring these (same self-containment grind as MFC) resolves the bulk.
  - **External stubs still needed (~bounded):** DirectX creation entrypoints
    (DirectDrawCreateEx, DirectInputCreateA, DirectSoundCreate, DirectDraw/Sound
    EnumerateA, CLSID_DirectMusicSegment), DirectPlay SP GUIDs (DPSPGUID_*),
    CRT/file-system (`_findfirst`/`_findnext`/`_findclose`, FindFirstFileA/
    FindNextFileA, **fopen_nocase** — case-insensitive open matters on Linux),
    known stragglers BAD_RV / BOB_GUID / GentleBankData.
  - **ASM still to convert** (MASM->nasm; MATRASM already done): HARDWARE
    PRO/PROLOG/HARDPASM, 3D LSTRASM, GRAPHICS GRAFPASM, FILES CDROM. (MEDITOR
    TPAINTWL is editor-only, out of the 302.)
  - **NEXT**: un-defer MSGAI/USERMSG first (largest symbol contributor) → then
    PACKAGES/SAVEGAME/UIMSG, OVERLAY, STUB3D/BOBFRAG → add external stubs → convert
    asm → add the `bob` add_executable + AfxWinMain-style entry → iterate the link.
    bfrefs.g reconstruction (BFIELDS) is independent and can land anytime.

- **2026-06-09 (24)**: **AI module's deferred TUs un-deferred — MSGAI.CPP (27->0)
  and USERMSG.CPP (1548->0) now build into libbob_ai.a.** These hold `ArtInt
  Art_Int;` and many AirStruc::/ArtInt:: methods. Recipe (the generic
  "deferred-TU" playbook for the rest of Phase 2):
  - **Reproduce the implicit MSVC PCH**: both TUs assumed a force-included PCH.
    Added a `#if defined(BOB_LINUX)` prelude with dosdefs.h (base types
    Bool/SLong/ShapeNum/NULL — USERMSG had *no* dosdefs at all → 1548 errors), the
    F_COMMON/F_GRAFIX/F_BATTLE file-enum group + files.g, then world/ai/model/anim/
    3dcom/planetyp/persons2/aaa/transite/overlay/globrefs.
  - **`class ViewPoint;` fwd-decl before worldinc.h** (worldinc uses `ViewPoint*`
    before 3dcom.h declares it) — recurring ordering fix.
  - **INSTANCEAI made variadic** (ai.h:318 `(name,trgtype,...)`): the 3rd `options`
    arg is unused in the body; BoB calls it 2-arg, MSVC allowed the short call.
  - **ArtInt data block -> `protected:`** (ai.h): the INSTANCEAI handler classes
    derive `: public ArtInt` and read `ACArray`/`ACARRAYSIZE`, which were private.
  - **member-fn call / for-scope**: `FindFormpos0` needed `()` on both sides of a
    compare; `auto`-typed inline decls for leaked loop vars (nf, newwp); i-hoists.
  - Trial whole-archive link: **574 -> 554** undefined symbols; full build clean,
    no regression from the shared ai.h edits. NEXT deferred TUs: MISSMAN
    PACKAGES/SAVEGAME/UIMSG, 3D OVERLAY, MFC STUB3D/BOBFRAG.

- **2026-06-09 (25)**: **MFC `STUB3D.CPP` builds (31->0) into libbob_mfc.a** —
  the 3D-subsystem stub/glue (defines `fastMath`, Master_3d, the View3d draw
  loop). The "MFC-context standalone TU" recipe (also applies to BOBFRAG and any
  other `stdafx`-including non-fragment TU): wrap its own includes with
  `#if BOB_LINUX #include "bob_mfc_pre.h"` *before* stdafx (F_BATTLE enum group →
  FIL_3D_KEYBOARD_TABLE) and `#include "bob_mfc_post.h"` *after* its includes
  (the whole RFullPanelDial/rdialog/rlistbox/resource.h dialog ecosystem — adding
  just `fullpane.h` cascades into undeclared DialBox/CRListBox/IDD_FULLPANE).
  Compat gaps filled (reusable across the remaining TUs):
  - **afxmt.h**: real CSyncObject/CEvent/CMutex/CCriticalSection/CSemaphore/
    CSingleLock/CMultiLock (no-op single-thread stubs) + it now `#define`s
    `__AFXMT_H__` — bob headers gate threading members on that (stub3d.h's
    StaticTimeProc/TimeProc were `#ifdef __AFXMT_H__`, hence "no declaration
    matches"/"has no member" until defined).
  - **mmsystem.h**: multimedia-timer API (LPTIMECALLBACK, TIME_*, timeSetEvent/
    timeKillEvent — no-op; timeBeginPeriod/EndPeriod already in compat_winbase.h).
  - **compat_wingdi.h**: SetSystemPaletteUse + SYSPAL_*.
  - **afxwin.h**: AfxBeginThread stub + `AFX_CDECL` (empty).
  - **dinput.h**: IDirectInputDevice_{GetDeviceData,GetDeviceState,Acquire,Unacquire}
    C-macro → C++ method wrappers.
  - **compat_types.h**: MSVC `i64`/`ui64` integer-literal suffixes as UDLs;
    `WINBASEAPI`/`WINUSERAPI`/`WINGDIAPI` empty (raw Win32 redeclarations in code).
  STUB3D is caller-heavy glue, so the whole-archive undefined count rose 554->585
  (it references the 3D/Miles/DDraw subsystems it drives) — expected; it's one of
  the 302 TUs that must be in the final binary. Full build clean. **BOBFRAG.CPP
  prelude added but still 135 errors (H2H multiplayer: H2HPlayerInfo, MMC,
  CRCombo/CRStatic, squad types) — left WIP, not yet wired into CMake.**

- **2026-06-09 (26)**: **MISSMAN `PACKAGES.CPP` builds (62->0) into
  libbob_missman.a** (8 standalone TUs now). Two macro-gated declaration blocks
  needed include-order fixes (the recurring "PCH provided the macro first"
  pattern) — both macros are *include guards*, so the fix is to include the
  header earlier, not `#define` the macro:
  - **ranges.h before package.h**: `RANGES_Included` gates Profile::AddAttackWP /
    InsertWpBetween / AddNumerousEscorts (else "no declaration matches").
  - **package.h before bfnumber.h**: `PACKAGE_INCLUDED` gates
    EventVal::MakeIcptGRExpr / MakeAngWorldPos.
  Plus mytime.h (SECSPERMIN/HR01), missman2.h (MMC), for-scope hoists
  (sq/find/j/i), `=+`->`=` (MSVC unary-plus on EventVal ambiguous in GCC), and a
  leaked for-scope `sm` replaced by its value `squadlist.Max()`.
  - **INT3 (DOSDEFS.H, BOB_LINUX): `((void)0)` -> `{}`.** BoB writes bare `INT3`
    with no trailing `;` (relied on MSVC `_asm{}` block form); an expression-form
    needs `;` and broke `else INT3 else`. Empty-block is a statement needing no
    `;`. Full build re-verified clean (no `if()INT3;else` breakage in built TUs).

- **2026-06-09 (27)**: **MISSMAN `SAVEGAME.CPP` builds (63->0)** into
  libbob_missman.a (9 standalone TUs now). Pure application of the settled recipe:
  a BOB_LINUX prelude before missman2.h/savegame.h with cstring.h
  (CSTRING_Included → SaveData::lastsavegame/lastreplayname/lastpackname + full
  CString), bfnumber.h (BFNUMBER_Included → info_itemS body via persons2.h),
  ranges.h then package.h (PACKAGE_INCLUDED → `typedef CampaignZero Campaign` +
  `class MissMan`, both gated in missman2.h), mymath.h (Math_Lib), node.h
  (Attacks), compat_winuser.h (::MessageBox/MB_OK). Then for-scope hoists:
  `entry` (7 identical find-loops, replace_all), `i` (×3), `u`. No shared-header
  edits → no regression. (The 8 earlier "void[int] subscript" errors were just
  cascades of the undeclared `entry`.)

- **2026-06-09 (28)**: **MISSMAN `UIMSG.CPP` builds (105->0) — MISSMAN module
  COMPLETE (all 10 standalone TUs).** UIMSG had only 4 includes and leaned hardest
  on the PCH (ItemPtr/AirStrucPtr/ItemBasePtr/mobileitem all undeclared). Same
  recipe: a BOB_LINUX prelude after dosdefs.h with F_* + files.g (FIL_NULL),
  uniqueid/cstring/bfnumber/ranges/package (the four gate macros), a ViewPoint
  fwd-decl + worldinc/airstruc (world item types), persons2 (Persons2),
  planetyp/FlyInit (PT_*, NAT_RAF/NAT_LUF/PT_BADMAX), mymath (Math_Lib), mytime
  (SECSPERDAY), missman2 (MMC), miles (_Miles), ../mfc/resource.h (IDS_GROUP_10).
  `LoadResString(int)` reproduced inline locally (only needs CString::LoadString)
  to avoid pulling the rdialog.h dialog ecosystem. One for-scope `i` hoist.
  Trial whole-archive link: **585 -> 473** undefined symbols. Full build clean.

- **2026-06-09 (29)**: **MFC `BOBFRAG.CPP` builds (135->0)** into libbob_mfc.a
  (the H2H-multiplayer frag-screen; defines `Aircraft_Formations`). Key lesson on
  **include ORDER for an MFC-context TU that also uses game-base types**: the
  game-base headers (esp. worldinc.h, which defines the `item` base + ITEMSIZE)
  must precede `bob_mfc_post.h`, because post.h pulls infoitem.h whose
  `info_itemS()` ctor does `Status.size=ITEMSIZE` — needs `item`/ITEMSIZE first.
  Final order: pre.h → stdafx → bob.h → [worldinc, ranges, package, missman2,
  nodebob, winmove, comms] → post.h → BoBFrag.h. (post.h must still precede
  BoBFrag.h so the CRCombo/CRStatic/CRListBox control wrappers are declared; and
  winmove.h before comms.h for NUMRADIOMESSAGES.) Reduced 135->67 (control
  wrappers/m_IDC_ members) ->21 (game types) ->1 (ordering) ->0.

- **2026-06-09 (30)**: **3D `OVERLAY.CPP` builds (330->0) — the LAST deferred TU.
  Every game TU now compiles into a module lib.** OVERLAY is the map/overlay UI
  (MFC-context: afxwin/afxctl). Four roots:
  1. **incomplete CString (~80)**: `<afxwin.h>` defines `__AFX_H__`, flipping bob
     headers to their "MFC present -> CString is forward-decl" branch. Fix: an
     inline pre.h-style block (only this TU sits in SRC/3D, off the MFC path) that
     `#include`s cstring.h BEFORE afxwin.h so bob's own CString is complete first.
  2. **MapScr screen-state table (~220)**: a table of `SelRtnPtr` member-fn
     pointers (`{IDS_x,0,SEL_n,MapScr::SelectFromX}`) written bare — same MSVC
     extension as _FULL's SelProc. Guarded perl prefixed `&MapScr::` to the 36
     erroring member fns (NOT followed by `(`, so defs/calls are skipped). Caught
     and reverted one over-match: `MapScr::OptionList` is a nested *type*, not a fn
     (`MapScr::OptionList *popt` / `MapScr::OptionList escOpt,termOpt,...` — the
     bad `&` had cascaded into all the `*Opt` "undeclared" errors).
  3. **GDI glyph API (4)**: GetGlyphOutline/GLYPHMETRICS/MAT2/FIXED/GGO_* stubbed
     in compat_wingdi.h (returns 0 -> blank overlay text for now; NDEBUG drops the
     asserts). A real path can rasterise via FreeType/SDL_ttf at runtime.
  4. **missing FIL_ enum constants**: discovered the **files.g F_* selector rule** —
     the F_* names are include GUARDS, so defining F_COMMON/F_GRAFIX *suppresses*
     F_COMMON.G/F_GRAFIX.G. Define **only F_BATTLE** -> pulls COMMON+GRAFIX+SOUNDS,
     skips just F_BATTLE.G. (Applied the same correction to UIMSG's prelude.)
  Plus four for-scope hoists. OVERLAY -> libbob_3d.a. Trial link: **473->465**.

- **PHASE 1 COMPLETE — every game TU compiles.** 14 module libs (61 objects).
  **PHASE 2 REMAINING (link the ELF):** 465 undefined symbols left — external
  stubs (DirectX/DirectPlay creation entries, CRT `_findfirst`/`FindFirstFileA`/
  `fopen_nocase`, BAD_RV/BOB_GUID/GentleBankData) + 6 MASM->nasm conversions
  (HARDWARE PRO/PROLOG/HARDPASM, 3D LSTRASM, GRAPHICS GRAFPASM, FILES CDROM) +
  the `bob` add_executable with an AfxWinMain-style entry → iterate link to zero →
  runtime bring-up (SDL2/GL/OpenAL). bfrefs.g reconstruction (BFIELDS) independent.

### Inline-asm conversion recipe (validated)
At each `_asm`/`__asm`/`#pragma aux` site add a `#if defined(BOB_LINUX)` branch
*before* the `__MSVC__`/`__WATCOMC__` one (BOB_LINUX is checked first):
- **register-arg routine** kept in nasm -> GCC wrapper:
  `__asm__ volatile("call X..." : "=a"(r) : "a"(),"d"(),"b"(),"c"() : "esi","edi","cc","memory")`
- **FPU/bit/math** routine -> portable C with `__builtin_*` (sqrt/sin/cos/lrint/
  fabs/clz/ctz); fixed-point 64-bit ops via `long long`/`int64`. Keep the
  original under `#elif`/`#else` so the Windows build is untouched.

### ASM porting recipe (validated)
1. Translate each MASM `.ASM` -> `<name>.nasm`: `SEGMENT`->`section`, `_X<name>`
   public label -> `X<name>` (no leading underscore on Linux ELF), `@@local`->
   `.local`, `dword ptr ds:[sym]`->`[sym]`, hex `1Fh`->`0x1F`, scratch `dd ?`->
   `.bss resd`. Add `section .note.GNU-stack noalloc noexec ...`.
2. In `H/DOSDEFS.H`, stop defining `__MSVC__` under `__GNUC__` (define a
   `BOB_LINUX` path instead) so GCC skips the MSVC `_asm{}` blocks.
3. At each `#ifdef __MSVC__` inline-asm site add `#elif defined(BOB_LINUX)` with
   a GCC `__asm__ volatile("call X...": "=a"(r): "a"(),"d"(),"b"(),"c"() :
   "esi","edi","cc","memory")` wrapper (omit ebx/ebp from clobbers when the
   routine preserves them; pass `&ref` for reference args).
