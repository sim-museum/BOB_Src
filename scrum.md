# Scrum — Battle of Britain Linux Native Port

Scrum project management for completing the native Linux port of Rowan's *Battle of
Britain* (2000). This file is the **single source of agile truth**: backlog, sprints,
ceremonies, and Definition of Done. It rides on top of the engineering record — the
running technical log is `PORT.md` (newest-first, with evidence) and the high-level state
snapshot is `STATUS.md`. When a backlog item moves, update its checkbox here and append the
evidence to `PORT.md`.

---

## 1. Product Vision

> A faithful, native Linux build of *Battle of Britain* with **ALL functionality** — every mode
> (Quick Mission, full Campaign, Multiplayer) playable the way it was on Windows: faithful flight,
> cockpit, lighting, AI, weapons, and effects; the complete front-end; joystick + mouse + keyboard;
> save/load; replay; intro video. Boots to its real menu on one window with **no `BOB_*` env vars**.
>
> **Original DoD (Quick-Mission slice) — ✅ MET 2026-06-17.** Vision now extended to *all* functionality
> (PO, 2026-06-17): multiplayer is **in scope**; only MIDI music stays environment-blocked (icebox).

The constraint that shapes everything: **game sources stay unedited** (rare root-cause UB fixes are a
documented exception — e.g. R1.3a–e operator-delete/index bounds, each logged with ASan evidence). Work
lives in `SRC/compat/` + `#if BOB_LINUX` boot scaffolds. The port maps Win32/MFC/DX7/DInput/DSound onto
SDL2 + OpenGL + OpenAL.

---

## 2. The Epic

> **EPIC: Port ALL of BoB to native Linux.**
>
> Every Windows feature running faithfully on SDL2+GL+OpenAL — the full game, not a slice.

Decomposes into **Releases R1–R7** (see §6 backlog): R1 real boot/one window ✅, R2 play a mission ✅,
R3 faithful flight (render fidelity), R4 the campaign, R5 control & sim depth, R6 complete front-end &
media, R7 multiplayer. Each Sprint delivers a **shippable increment** (a `./bob` that does demonstrably
more of the real game than the last). The §7a roadmap maps releases → sprints to the end state.

---

## 3. Roles

| Scrum role | Who | Responsibility |
|---|---|---|
| **Product Owner** | The user (`curator`) | Orders the backlog, accepts increments, owns "done". |
| **Scrum Master** | Claude (this agent) | Removes blockers, keeps ceremonies, protects the no-edit-game-code + no-real-email conventions. |
| **Development Team** | Claude (autonomous per CLAUDE.md working mode) | Builds increments in `SRC/compat/`, logs evidence in `PORT.md`. |

Working agreement (from `CLAUDE.md`): work autonomously, don't pause for confirmation, pick
the idiomatic Linux/POSIX path, only stop on a hard blocker. Anonymous repo — commit as
`curator <noreply@anthropic.com>`.

> **PO standing approval (2026-06-17):** the Product Owner approves every sprint in advance —
> "just keep working." The Scrum Master (Claude) therefore makes per-sprint planning/scope
> calls itself (incl. the R1.1b direction below) without pausing for acceptance, and runs the
> ceremonies as a log rather than a gate. Increments are still demoed + recorded in `PORT.md`.

---

## 4. Definition of Ready (DoR)

A backlog item is *Ready* to enter a sprint when:
- It has a clear acceptance criterion that is **observable by running `./bob`** (a trace
  line, a frame dump, a non-black ground %, an exit code, an audible/visible state).
- Its reproduction command (env vars + data path) is written down.
- Dependencies (earlier stories) are done or explicitly stubbed.

## 5. Definition of Done (DoD)

A backlog item is *Done* when **all** hold:
1. Implemented in `SRC/compat/` (or a `BOB_*` boot scaffold) — **no game-logic edits**.
2. `cd build && ninja bob` links clean.
3. Default `./bob` still exits 0 (no regression to the safe default path).
4. The acceptance criterion is demonstrated with reproducible evidence.
5. A dated entry is appended to `PORT.md` (newest on top) with the evidence.
6. Any new diagnostic is **env-gated and default-off**.
7. `STATUS.md` "What works" / roadmap updated if the item changes product state.

**Increment-level DoD (per sprint):** the above for every story in the sprint **plus** the
sprint's increment is independently runnable and represents a faithful step toward the
no-env-var end state.

---

## 6. Product Backlog

Ordered by Product-Owner value/risk. Estimates in **story points** (Fibonacci; 1≈trivial,
13≈multi-session research grind). Status: ☐ To Do · ◐ In Progress · ☑ Done.

### Already shipped (Sprint 0 — pre-Scrum increments, accepted)
These landed before formal Scrum and are the baseline increment. Listed for velocity
calibration and so we don't re-do them.

| ID | Story | Pts | Status |
|---|---|---|---|
| S0.1 | Build/link all 16 modules + MFC → ELF; default `./bob` exits 0 | 8 | ☑ |
| S0.2 | 3D flight + cockpit on real GL (`BOB_BOOT_FRONTEND`) | 13 | ☑ |
| S0.3 | Landscape ground via FBO RTT (default-on, 51%→99% non-black) | 13 | ☑ |
| S0.4 | FULL_RES land textures + fluffy clouds by default | 5 | ☑ |
| S0.5 | Audio DirectSound→OpenAL (engine loop + effects) | 13 | ☑ |
| S0.6 | Keyboard flight input (SDL→DIK→DInput→command) | 8 | ☑ |
| S0.7 | HUD info bar + unit-factor (`SetUnits`) divide-by-zero fix | 3 | ☑ |
| S0.8 | Front-end: hosted R\* OLE controls, config forms, RLE8 backgrounds | 13 | ☑ |

**Baseline velocity reference:** ~25–30 pts of accepted work per "session-sprint".

### Release 1 — "Real boot, one window" (Phases 1–2)

| ID | Story (As a player / dev, I want…) | Pts | Status |
|---|---|---|---|
| R1.1a | **Unify the window — infrastructure.** *Verified already done:* one `SDL_CreateWindow`/GL context, one event pump, both present paths (`present_surface` flight + `bob_gdi_present` front-end) swap the same `g_win`. | 3 | ☑ |
| R1.1b | **Unify the window — control flow.** Merge the two mutually-exclusive `InitInstance` forks (`BOB_BOOT_FRONTEND` vs `BOB_FRONTEND`) + collapse the `Run()` wait-strategy branches into one continuous menu→fly→menu app. **Blocked-by R1.3** (flight-under-real-init corrupts the heap). Re-estimated up; carried to Sprint 2. | 8 | ☐ |
| R1.2 | **Diagnose the gating heap corruption:** get a memory tool (valgrind memcheck under `SDL_VIDEODRIVER=dummy`/software-GL, or ASan on suspect TUs) onto the `InitPreferences` combat path; capture the first invalid write. | 13 | ☑ |
| R1.3a | **Fix `SetPilotedAcAnim`** scalar `delete`→`delete[]` (alloc-dealloc-mismatch). Verified gone under ASan; default flight unregressed. | 3 | ☑ |
| R1.3b | **Bound `Reg3dConv`** scancode/shiftstate indices + terminator read (2-byte WRITE 1207 B past `KeyMap3d`). Verified gone under ASan. | 2 | ☑ |
| R1.3c | **Pin bilinear after `InitPreferences`** (trilinear default → `CopyMapToSurface` NULL-deref). Verified: loader SEGV gone, InitPreferences reaches live flight. | 2 | ☑ |
| R1.3d | **Transient double-free** in `RemoveDeadListFromWorld` — `TransientItem::operator delete` re-ran the destructor (delete-expression idiom) → anim buffer freed twice. Fixed to `::operator delete(obj)`. ASan: gone; flight 90s clean. | 8 | ☑ |
| R1.4 | **Land real init via `InitPreferences()`:** wired as default; per-feature `BOB_*` forces retired (gated `!initPrefs` / kept as overrides). `BOB_NOINITPREFS` reverts. | 5 | ☑ |
| R1.5 | **Regression sweep:** verified — default flight reaches View3d interactive, 90s clean, 92% non-black frame, OpenAL engine loop + effects, HUD on, no feature env vars. | 3 | ☑ |

### Release 2 — "Play a mission" (Phase 3)

| ID | Story | Pts | Status |
|---|---|---|---|
| R2.1 | **Menu "Fly" drives the real mission load** (`LoadSetPiece`) instead of the synthesized scramble. **☑ DONE (satisfied by R1.1b 4.1/4.2)** — real menu Fly → `StartFlying→Launch3d→new Inst3d→Persons_2.LoadSetPiece`; `SetUpHotShot` found unsafe on BoB data (missing `IDS_CONFIGIGNORED`), our pre-flight is the faithful index-based equivalent. | 13 | ☑ |
| R2.2 | **Mission-end → debrief** path runs through the game's own flow. **☑ DONE** — EXITKEY (Alt+X) → `CloseWindow(IDOK)` → `OnFlyingClosed(IDOK)` → (`gamestate=HOT`) `quickmissiondebrief` renders. `BOB_AUTOQUIT=debrief`. | 8 | ☑ |
| R2.3 | **Latent uninitialized-state bug grind** uncovered by the real mission loop. **☑ DONE (stress-validated, NO bugs found)** — 4-mission chain + `BOB_QM_INDEX=0..7` variety: all reach flight, 0 crashes. The feared grind was pre-empted by the upstream fixes (InitPreferences R1.4, combat-corruption R1.3a–d, DD7 teardown 4.3b, SetIndex guard 4.3c). | 13 | ☑ |
| R2.4 | **Campaign continuity:** menu → mission → fly → debrief → next, no env vars. **☑ DONE (chain proven)** — `BOB_REFLY=N` chains missions; verified two consecutive fly→debrief→fly→debrief cycles in one process, both debriefs render, 2nd `StartFlying` clean (DD7 fix holds across cycles). Cosmetic "no env vars" packaging deferred. | 8 | ☑ |

> **Scope expansion (2026-06-17, PO):** the goal is now a **Linux port of ALL functionality** — every
> BoB feature, faithfully. DirectPlay multiplayer comes **in scope** (was iceboxed). Only MIDI music stays
> iceboxed (hard environment blocker). Backlog below re-groomed to that end. Releases R3–R7 are the path.

### ✅ Release 2.5 — "First human pilot" (2026-06-17, accepted)
The DoD increment shipped (bare `./bob` boots + plays, no env vars) and a human flew it. Field fixes:
| ID | Story | Pts | Status |
|---|---|---|---|
| R2.5a | **Gun-fire crash (R1.3e)** — `operator delete` double-free on the 7 sibling item classes (combat spawns/frees them); extends the R1.3d fix. ASan: 7772→0 double-frees. | 5 | ☑ |
| R2.5b | **Menu click hit-boxes** offset right of the text (measured at `fh`, drawn at `oleH`); aligned. | 2 | ☑ |

### Release 3 — "Faithful flight" (render fidelity) — **NEXT**
*The in-flight view should match the Windows/Wine reference. First-pilot reports drive the top items.*
| ID | Story | Pts | Status |
|---|---|---|---|
| R3.1 | **Scene lighting** — ☑ **DONE (2026-06-17).** Root cause (gated `BOB_TRACE_LIGHT` dump): the `BOB_STARTFLYING` preflight set a **PRE-DAWN** time (curTime 23400 < dawn 25200 → twilight ambient `2c2c2c`), unlike `SetQuickState`'s `HR14`. Fixed: preflight sets `currtime=HR14` + clear sky → daylight (`ambient ffffff`, frame mean 63→111). **Pilot confirmed "much better lighting!"**, instruments readable; env-free path verified bright. The lighting *system* was correct (time-of-day driven). | 8 | ☑ |
| R3.2 | **Cloud depth / draw-order** — fluffy-cloud billboards paint over the cockpit. ◐ **SPIKE — needs dedicated focused work (2 failed real-time attempts).** Findings: (1) `draw_fvf` sends only x,y for pre-transformed RHW geometry, so the screen-z never reaches GL → depth can't differentiate (the first `BOB_ZTEST` no-op). (2) Sending the RHW z + depth-test is right *in principle* but the z-mapping through `glOrtho(...,-1,1)` is inverted (near→larger window-z), and getting GEQUAL/clear-value/FBO-depth-interaction wrong **blanks the scene** (everything depth-rejected). Needs: a correct RHW-z→window-z mapping for the back buffer, a per-frame back-buffer depth clear, verified against the FBO-composited terrain (which writes no depth). A careful spike, NOT a live patch. **Re-est 8→13.** **Spike starting point (derived while debugging):** the standard D3D-pre-transformed-in-GL mapping is `glOrtho(0,w,h,0, 1,0)` (near=1,far=0) + send the RHW z (3 comps) + `GL_LEQUAL` + clear depth to **1.0** — NOT the `(…,-1,1)`+GEQUAL+clear-0 I tried (that blanked the scene). Also honour ZWRITE so the sky/terrain backdrops (far) don't depth-write over the near cockpit; A/B vs Wine. | 13 | ◐ |
| R3.3 | **Cloud sprite fidelity** — ☑ **DONE (2026-06-17).** The cloud 4444 textures drew correctly blended (`BOB_TRACE_CLOUDA`: blend=1, SRC_ALPHA/INV_SRC_ALPHA) but ALL alpha textures were filtered `GL_NEAREST` (right for 1-bit keyed masks). Clouds have **smooth 4-bit dithered alpha** → NEAREST showed the dither as a hard white checkerboard. Fix: split alpha filtering by depth — smooth alpha (4444/32-bit) → `GL_LINEAR` (soft clouds); 1-bit/keyed → `GL_NEAREST` (no fringe). Clouds now soft/fluffy; masked cockpit textures unchanged. | 5 | ☑ |
| R3.4 | **Rear-view mirror horizon UVs** — `InfiniteStrip` garbage v-texcoords (compat sanitiser or game-side) so the mirror shows the horizon, not a flat edge texel. | 5 | ☐ |
| R3.5 | **Trilinear mipmaps** — ☑ **DONE (2026-06-21).** The `CopyMapToSurface` NULL-deref was a missing **attached mip-level chain**: for `HINT_TRILINEAR` the game makes a `DDSCAPS_COMPLEX|DDSCAPS_MIPMAP` texture and walks `GetAttachedSurface(DDSCAPS_MIPMAP)` to upload each level; compat created no sub-levels → NULL target. Fixed in `bob_video.cpp`: `make_surface` builds the `dwMipMapCount` sub-surface chain (`GLSurface7::mip`), `GetAttachedSurface` returns + AddRefs each level (DX semantics; prevents the mid-walk free), `SURF_Release` tears it down, `upload_texture` auto-generates GL mips (+anisotropy) for chained surfaces only. Bilinear pins (R1.3c) lifted → trilinear is the faithful default; `BOB_BILINEAR`/`BOB_NOMIP` A/B. `BOB_FILTER=2` SIGSEGV→clean flight; default boots `filtering=2`, 88.8% non-black, no stripes. | 5 | ☑ |
| R3.6 | **Terrain detail combiner + over-tiling** — ◐ **ADDRESSING DONE (2026-06-21).** `DEV_SetTextureStageState` (was a no-op) now captures `D3DTSS_ADDRESS/U/V`; `draw_fvf` applies the game's per-stage MIRROR/CLAMP/WRAP as the GL wrap mode at draw time (matches D3D sticky sampler state) instead of GL_REPEAT everywhere → stops terrain over-tiling. `BOB_NOADDR` reverts. The 2-stage detail-texture *combiner* (multi-texture blend) is the remaining sub-item (separate from addressing). | 5 | ◐ |
| R3.7 | **In-flight effects** — smoke/contrails/tracers/muzzle-flash/explosions/flak render faithfully (transient sprites). | 5 | ☐ |
| R3.8 | **Render regression sweep** — A/B each in-flight view vs Wine `bob.exe`; pixel-truth + long-session render stability. | 3 | ☐ |
| R3.9 | **Ground-impact crash (field fix)** — ☑ **DONE (2026-06-17).** Pilot reported a crash on hitting the ground. ASan: `LandMapNumRecord::Reset`/`~LandMapNumRecord` freed `new UByte[]` landscape-tile buffers (body/palette/alpha, from `FixLbmImageMap`) with scalar `delete` → new[]/delete mismatch corrupting the heap on every landscape-texture free (10× in flight; low flight streams tiles → the ground rush trips it). Fixed → `delete[]`; bonus `SetPilotedAcAnim` scalar-new → `new[1]` matching `animptr::Delete`'s `delete[]` (1× teardown). Same bug family as R1.3a/e. ASan 10→0 / 1→0; bare `./bob` 0. Repro `BOB_AUTOFLY=dive`. Commit `ceb3083`. | 5 | ☑ |

### Release 4 — "The campaign"
*Play the strategic Battle-of-Britain campaign, not just Quick Missions (the empty campaign screen the pilot hit).*
| ID | Story | Pts | Status |
|---|---|---|---|
| R4.1 | **Campaign front-end bring-up** — ◐ **IN PROGRESS (2026-06-21).** Campaigns flow now navigable to the phase-select screen. (1) Side-select (RAF/LW) wired via the faithful `SideSelectOutlines` polygon hit-areas → real `OnSelectRlistbox` nav (`FULLPSYS.CPP`); the art-region screen was a dead end (no text menu). (2) `campaignselect` crash fixed: `CSCampaign` AddString's into columns 0–3 without AddColumn (relies on the persisted OCX column count); host now `ensureColumns()` auto-creates them (`bob_ole_rlistbox.cpp`). **campaignselect renders** — the 4 BoB phases (Convoys/Eagle Attack/Critical Period/Blitz) + portrait + description. Remaining: `Begin → campaignentername` (enter-name screen), then into the campaign proper. | 8 | ◐ |
| R4.2 | **Strategic map screen** — ◐ **TERRAIN RENDERS (2026-06-21).** `LaunchMap → CMIGView::UpdateBitmaps` now draws the real strategic map (SE England + Channel + France, RAF sectors A–E/Y/Z, city labels, No.11 Group) via C-GDI `StretchDIBits`/`FillSolidRect` implemented on R6.1, driven by a map-paint tick (`g_bob_map_active`) that also keeps the window live. Verified `/tmp/r42_map.png`, no regression. **Render confirmed correct for the strategic zoom** — `DrawIconTest` runs (zoom 2.0, 1052 items) but `m_zoom<ZOOMTHRESHOLDDETAIL` means only *dynamic* units (raids/squadrons/waypoints) get icons at this zoom, and those need `Todays_Packages` populated by the campaign day (→ R4.3). Remaining: `CMainFrame` toolbars/bars, scroll/zoom + click (scramble/intercept), and the dynamic icons. **Icon spike (2026-06-24):** root-caused why no unit icons render — the icon path (`UpdateBitmaps→DrawIcons→DrawIconTest→MaskIcon`, on R6.1's blit) IS wired + reached, but `DrawIcons` gets an **empty `inter` clip rect (0,0,0,0)** → the world-rect cull rejects every item (`raw_p` large, `survived_cull=0`). Items exist; the gap is the **`CMapDlg` scroll/world-transform** the headless `UpdateBitmaps` shim doesn't set up. So R4.2 icons is a **coordinate-system subsystem** (drive the real CMapDlg paint; consider adopting MA's further-along map view), not a one-liner. Banked; trace reverted (game code pristine). | 13 | ◐ |
| R4.3 | **Campaign mission flow** — ◐ **CORE LOOP CLOSED (2026-06-24).** The full campaign mission cycle runs in one process, no crash: strategic map → **intercept a raid** (`BOB_CAMPAIGN_FLY` → `NewPackage` scrambles a real interceptor) → **briefing** (`bobfrag`) → **Fly** → **flight** (cockpit on real GL; reuses the QM `StartFlying`/Launch3d bridge) → **mission end** (`BOB_AUTOQUIT=debrief` → EXITKEY → `OnFlyingClosed(IDOK)`) → **back to the strategic map** (`OnFlyingClosed` routes by gamestate=COMMANDER → campaign branch `NextMission`+`StartUpMapWorld`+`LaunchMap`). One fix earlier: set `MMC.playersquadron` at Fly time (a just-scrambled interceptor is outside the briefing's flyable-status gate). One deferral: headlessly fast-forwarding the *post-mission* sim SIGSEGVs in `GetCruiseAt`/`Plane_Type_Translate[bad ptype]` (post-mission SAG `type` uninit — **R4.5 grind**); guarded off (`g_campfly_flown`) so the returned map is stable (139→124), faithful to real play (returned map is paused). Remaining: post-mission SAG-uninit (R4.5) for day-advance + next mission; in-cockpit Continue/Quit dialog; briefing widget population (R6.3). _(earlier same day: a mission flies; briefing reached; interception scaffold.)_ | 13 | ◐ |
| R4.4 | **Campaign save/load** — ☑ **DONE — FULLY CLICK-DRIVEN (2026-06-24).** Save a campaign and load it back entirely through the real UI: **save persists (S28) → load restores state (S29) → load screen lists saves (S30) → load enters the map (S31) → file-row click selects a save (S32)**. Last piece (S32): the OCX file-row-click eventsink is a no-op on Linux, so a targeted `BOB_LINUX` bridge (mirrors R5.3b SController) — `CLoad::bob_file_clicked`→`OnSelectRlistboxfile`, `g_bobCLoad` registry, `OleHost::rowAtY`→`HostRListBox::GetRowFromY`, routed by `bob_ole_click`. Verified by genuine clicks: click "Auto Save" → `OnSelectRlistboxfile filename='Auto Save.bsr'` → click "Load" → `LoadGame=1 → LaunchMap → strategic map active`, no crash. Earlier-session ASan fix (deserialise terminator) + 3 `fakefile`-path twins (Save/Load/CLoad) + `DoLoadGame`-is-clean finding. No regression. _(2nd targeted OCX bridge → MA general eventsink worth adopting next.)_ | 8 | ☑ | Save (S28) + **load (S29)**: `CFiling::LoadGame -> OK, currtime 26660->32180` (loaded state = saved state). The deserialise crash was ASan-pinned to `MIGView.cpp:2210` — the `raidnumentries[r]` terminator loop overrunning the array because **loaded packages lack the terminator** `RecostRaidList` sets at runtime (R4.5 family; **NOT** ConvertPtrUID — its null-handling is already safe via address-cast operators, hypothesis disproven). Fixed: re-run `RecostRaidList()` on loaded packages in `PackageList::LoadGame` (MAPCODE.CPP) + `if(ac)` guard the 3 unguarded `ac->SetDraw()` in SetVisibilityFlags (world not rebuilt at deserialise-time). ASan-verified the overflow is gone. **S30: lists the save** (`CLoad::MakeFileList` path fix, 3rd twin). **S31: load → campaign map** — the loadgame "Load" item is `DoLoadGame` = `LoadGame`(S29) + `LaunchMap`, NOT the R4.2-blocked `CFiling::OnOK`; so loading enters the strategic map on the restored state (`[loadgo] LoadGame=1 currtime=32180 → LaunchMap done`; `doc/reference/loadgame-into-map-2026-06-24.png`). **Save→load→list→enter-map all work.** **Only remaining R4.4 UI:** the OCX **file-row-click** → `OnSelectRlistboxfile` (RListBox-host row-click via `GetRowFromY` + targeted CLoad bridge / MA eventsink) so the user clicks the save instead of the `BOB_LOAD_GO` scaffold setting `selectedfile`. _(S28 save persists, S29 load restores state.)_ | 8 | ◐ | With the campaign running (R4.3), **a campaign save now lands on disk** — `SAVEGAME/Auto Save.BSR`, 225KB of real `bos<<Miss_Man`. Root cause of "no saves ever": `CFiling::SaveGame` used the corrupted `fakefile` savegame path that `LoadGame` was already `#if BOB_LINUX`-bypassed for, but `SaveGame` wasn't — fixed (mirror the bypass: write `savegame/<fname>`). `BOB_CAMPAIGN_SAVE` scaffold triggers it. **Load = two scoped gaps (deferred):** (1) `CLoad` file-list enumerates `FIL_SAVEGAMEDIR` via the same `fakefile` path → list empty (adopt MA's CLoad render+click fix); (2) `LoadGame`→`bis>>Miss_Man`→`PackageList::SetVisibilityFlags` SIGSEGVs at `*ConvertPtrUID(uid)` (deref-before-NULL-check; world not rebuilt at deserialise-time — **R4.5 family**, gdb-pinned). No regression (scaffolds gated). | 8 | ◐ |
| R4.5 | **Campaign uninit-state grind** — ◐ **CAMPAIGN SIM RUNS (2026-06-22).** ASan (build-asan under `BOB_MAP_TIMER`) was the oracle: caught two raid-generator heap bugs — (1) `new[]`/`delete` mismatch in `Profile::ReorderPackage` (PACKAGES.CPP:5828 → `delete[]`, cleared all 12 alloc-dealloc-mismatches); (2) raid-list terminator off-by-one in `RecostRaidList` (PACKAGES.CPP:5881, `RemakeRaidList` can clobber the only terminator → `MoveAllSAGs` reads off the array end → garbage raids → NULL SAG waypoints → the crash). Fixed (`+2` guard slot). The strategic-map day now advances 90s with NO crash (`d42dbd2`). No regression. Remaining: residual lower-severity ASan reads + wiring dynamic raid icons/toolbars. _(earlier: entered the grind, pinned the crash chain.)_ Breakthrough: the campaign sim RUNS (init ✓, raid SAGs created+tabulated ✓, `as` non-NULL) — the earlier "not deployed" theory was wrong. The crash is an uninit-state bug **inside the SAG movement AI**: `movecode=AUTOSAG_FOLLOWWP` → `SAGMovementFollowWP`→`CruiseToWp` derefs a NULL (likely the waypoint at `wpref` not in `pItem`, or the squadron) — an incompletely-created raid. This is the multi-bug grind through the raid-data lifecycle (SAG→waypoints→squadron→target), each a locate-NULL+fix. Pinned the first at SAGMOVE.CPP:2021. | 8 | ◐ |

### Release 5 — "Control & sim depth" (input + flight/AI fidelity)
| ID | Story | Pts | Status |
|---|---|---|---|
| R5.1 | **Joystick** — ✅ **DONE — PO FLY-TEST PASSED (2026-06-22): "joystick works".** Full DirectInput→SDL_Joystick path in compat (enumerate, GetCaps, EnumObjects, SetDataFormat offset-learning, GetDeviceState + **buffered GetDeviceData** reading real SDL axes/buttons/POV, QueryInterface). Keystone fixes: distinct non-zero device + object-type GUIDs (BOBGUID made them all-zero → CreateDevice(joystick) returned the keyboard; axes misclassified as buttons), and the **buffered GetDeviceData joystick branch** that the flight loop (`Analogue::PollPosition`) actually reads — the last missing piece. Default flight mapping injected into runtimedevices (axis 0=aileron/1=elevator/2=rudder/3=throttle; `BOB_NOJOYDEFAULT` off). Calibration telemetry under `BOB_TRACE_JOY` (per-axis min/max + LIVE line). PO follow-up "throttle maxes at 90%" → **diagnosed faithful** (`KEYFLY.CPP:319` `MAXSAFETHROTTLE=90` boost-cutout, gated on `FD_SPINS`; 100%+ = pull boost cut-out `FK_BOOSTCUTOUT`); game code left unedited. Keyboard flight unregressed. `58bdd4a`+. | 5 | ✅ |
| R5.2 | **In-flight mouse** — ✅ **DONE (2026-06-23).** Full DirectInput→SDL mouse in compat (`bob_video.cpp`): EnumDevices(MOUSE)/CreateDevice(GUID_SysMouse)/EnumObjects(2 rel axes+3 buttons)/SetDataFormat offset-learning/buffered GetDeviceData reading `SDL_GetRelativeMouseState`/GetCaps. Distinct non-zero `GUID_SysMouse` (R5.1 keystone). Default mapping (`ANALOGUE.CPP` BOB_LINUX scaffold) axis0→AU_UI_X, 1→AU_UI_Y enables the in-3D UI cursor (was `-0x8000` disabled); `BOB_NOMOUSEDEFAULT`/`BOB_NOMOUSE` off. **Keystone fix:** `DIDEV_EnumObjects` now honours the DIDFT filter — the controls-config's `EnumObjects(DIDFT_AXIS+DIDFT_POV)` counted buttons as axes → `firstaxes` underflow → OOB write clobbering `connecteddevices[]` (SIGSEGV); joystick alone stayed under, mouse tipped it over. Flight requests all types → R5.1 unregressed. Verified: offsets 912/916=AU_UI_X/Y, axisvalues track injected motion (sign+ratio), still=−0x8000. Pending PO mouse fly-test. `BOB_MOUSEFLY`/`BOB_TRACE_MOUSE`. | 3 | ✅ |
| R5.3 | **Full keymap + controls-config UI** — ◐ **FORM RENDERS + REBIND INTERACTIVE (2026-06-23).** Clicking a hosted device/axis combo now reassigns the control via a targeted `BOB_LINUX` bridge (`SController::bob_combo_changed` → genuine `OnTextChanged*` handler → `ChangedAxesCombo`/`RemakeAxes`; the OCX eventsink is a no-op on Linux). Verified: cycling the Stick combo changes its value AND recomputes the other combos (freed axes become available options) — real interdependent reassignment; persists via `PreDestroyPanel→SetAxisConfig`. No regression. Remaining: in-game *keyboard* rebind list. _(earlier: form renders + CString-varargs fix.)_ The front-end **Controls config screen** (`SController` device/axis-assignment form) now renders as a complete, readable form (tab bar + labels + hosted device/axis combos: "First Joystick: Logitech Extreme 3D", "First Joystick Axis 0 & Axis 1", …) — was a SIGSEGV; validates the R5.2 EnumObjects fix end-to-end (`BuildEnumerationTables` is its path). **Bonus game-wide fix:** root-caused the garbled `%s` text to the **CString-in-varargs Itanium-ABI bug** (`CSprintf("%s",CString)` passed by-ref on GCC vs by-value on MSVC) and fixed it in compat `CString::FormatV` (numeric formats keep trusted `vsnprintf`; `%s` formats discriminate CString-by-ref vs char* via CStringData-header validation + `/proc/self/maps`-guarded reads). `BOB_CONFIGSCREEN` scaffold. No regression (bare 0; flight + GFX/Sound forms clean). Remaining: rebind *interaction* (click-to-reassign + persist) + full keymap screen. | 5 | ◐ |
| R5.4 | **Flight-model / damage / collision verification** — A/B vs Wine (handling, stalls, damage, ground collision, fuel/engine management). | 8 | ☐ |
| R5.5 | **AI combat verification** — enemy AI engages/fires/dies faithfully; scoring + diary. | 8 | ☐ |

### Release 6 — "Complete front-end & media"
| ID | Story | Pts | Status |
|---|---|---|---|
| R6.1 | **Front-end blit subsystem** — ◐ **CORE DONE (2026-06-21).** Built `bob_gdi_blit.cpp`: bitmap registry + DIB decoder (8/24/32/4/1-bit) + raster-op blit (SRCCOPY/AND/PAINT/INVERT) for `bob_blit`/`bob_stretchblit`. Wired pixel-backed `CBitmap` + memory-DC `CDC::SelectObject`/`BitBlt`/`StretchBlt` + `CreateDIBitmap` (`afxwin.h`/`compat_wingdi.h`) — the path `LoadInstances`/`MaskIcon`/`UpdateBitmaps` need. Verified: icon sheet decodes (1408×1024, dumped perfect) + blits to the framebuffer (`BOB_BLIT_TEST`). Remaining: drive `MaskIcon` from the hosted-combo/button OnDraw so dropdown arrows/box-art show on the front-end (the data + blit now exist). **Unblocks R4.2.** | 8 | ◐ |
| R6.2 | **Font / DPI fidelity** — ◐ **multi-line text fixed (2026-06-21).** The huge-overlapping-text bug was `bob_ole_draw_panel` setting the font height = the control's BOX height — right for single-line labels/combos, but a tall multi-line text control (campaign PhaseDescription `CRStatic`) drew its font at the full box height. Fixed: cap the font at the single-line (16-DLU) height for multi-line boxes; single-line controls unchanged. campaignselect description now readable; config screens unregressed. Remaining: native-DLU base-font pass + multi-line word-wrap to fill the box. | 5 | ◐ |
| R6.3 | **Remaining UI screens** — ◐ **OPTIONS TABS + DEBRIEF VERIFIED (2026-06-23).** All six options config tabs render as readable forms (GFX, More GFX, Controls, Sound, 2D, Sim — via `BOB_CONFIGSCREEN`), and the mission **debrief/readyroom** renders correct text ("54 Squadron", "Eagles Attack", "11:00") — both unblocked by the game-wide CString-varargs fix (R5.3). Remaining: loadout, diary, other debrief variants. | 8 | ◐ |
| R6.4 | **Replay** — record + playback through the menu (the debrief Replay option). | 8 | ☐ |
| R6.5 | **Save/load round-trip** — all game state (settings/campaign/replay) binary-compatible. | 5 | ☐ |
| R6.6 | **Intro Smacker / cutscenes** — Smacker video → libsmacker. | 5 | ☐ |

### Release 7 — "Multiplayer" (the long tail — now in scope)
| ID | Story | Pts | Status |
|---|---|---|---|
| R7.1 | **DirectPlay → sockets** — session create/join, lobby/readyroom, state sync. (Large; slice into sub-stories at planning.) | 21 | ☐ |
| R7.2 | **H2H + co-op missions** — play a multiplayer mission end-to-end over the socket transport. | 13 | ☐ |

### Icebox (environment-blocked — not schedulable until the environment changes)
| ID | Story | Why parked |
|---|---|---|
| ICE.1 | **MIDI music** (`midiOut*` → soft-synth) | No 32-bit fluidsynth / system soft-synth + proprietary `.DIR` music archive. Needs a 32-bit ALSA-seq synth installed first; sound effects + engine already work. |

---

## 7. Sprint Plan

Sprints are scoped to ~one focused work-session each, ~20–30 pts (calibrated to Sprint-0
velocity). Each sprint ends with a **shippable increment**: a `./bob` that does strictly
more of the real flow.

### Sprint 1 — "One window, real init begins" → *Increment: unified-window build*
- **Sprint Goal:** the front-end and flight share one continuous GL window, and the heap
  corruption that blocks real init is **diagnosed** (first invalid write captured).
- **Committed:** R1.1 (8), R1.2 (13). *Total 21 pts.*
- **Increment demo:** `./bob` opens one window, navigates menu, enters flight on the *same*
  window; a `PORT.md` entry pins the memory-tool finding for the corruption.
- **Risk:** R1.2 is research — if valgrind can't run under GL, the fallback (ASan on suspect
  TUs / `-fpack-struct` A-B build) is the Scrum-Master's blocker-removal task.

### Sprint 2 — "Real init lands" → **CLOSED 2026-06-17 (partial; Release 1 slips to Sprint 3)**
- **Delivered (PO-accepted, 7 pts):** R1.3a/b/c — setup-layer heap corruption fixed & ASan-verified;
  InitPreferences reaches live flight; default flight unregressed. Increment = the corruption-fix build.
- **Not delivered:** R1.4/R1.5 — blocked by **R1.3d** (transient double-free, discovered mid-sprint;
  InitPreferences-specific, gates init-as-default). PO chose to ship the verified increment and split
  R1.3d into a focused Sprint 3 rather than grind an uncertain multi-session bug.
- **Carried to Sprint 3:** R1.3d (8), R1.4 (5), R1.5 (3).

### Sprint 3 — "Crack the transient double-free, land init" → *Increment: env-free boot to flight* — **NEXT**
- **Sprint Goal:** fix R1.3d so combat under InitPreferences is heap-clean, then land InitPreferences as
  the default init (R1.4) with the regression sweep (R1.5). **Ships Release 1.**
- **Committed (16 pts):** R1.3d (8), R1.4 (5), R1.5 (3).
- **Tooling ready:** `BOB_ASAN` build (consider adding `-g` for line numbers on the destructor aliasing);
  valgrind memcheck for the uninitialised-value cross-check.
- **Note:** the original Sprint-3 mission-loop work (R2.1/R2.2) shifts one sprint later.

### Sprint 4 — "Real menu→flight transition (StartFlying bring-up)" → *Increment: flight via the game's own screen flow* — **ACTIVE (2026-06-17)**
- **Sprint Goal:** R1.1b — make the game's own menu→flight path stand up flight **in one process**:
  `LaunchScreen(&quickmissionflight) → RFullPanelDial::StartFlying() → flybox/Rtestsh1 →
  Start3d(7) → Launch3d → View3d interactive`, replacing the synthesized `BOB_BOOT_FRONTEND`
  scaffold's direct `Inst3d/View3d` construction. **SM decision (standing PO approval):** the
  faithful path, not a throwaway scaffold — it *is* R2.1's seam, so the work carries forward.
- **Committed (13 pts):** R1.1b, delivered as increments —
  - **4.1** ☑ **DONE (2026-06-17).** Factored the QM pre-flight; `BOB_STARTFLYING` drives
    `LaunchScreen(&quickmissionflight) → StartFlying() → flybox/Rtestsh1`, then (compat has no
    message-map dispatch, so the `WM_GETSTRING` post is swallowed) feeds the two `Start3d` paint
    bits + calls the game's own public `Rtestsh1::Launch3d(wasrunning)` — exactly what
    `OnGetString` does. **Accepted:** faithful Spitfire cockpit (HUD/mirror/tower ATC), frame 120
    88.7% non-black, 60s+ no crash, bare `./bob` still exits 0. Evidence: PORT.md + `/tmp/sf_frame.png`.
  - **4.2** ☑ **DONE (2026-06-17).** `BOB_STARTFLYING=click` + `BOB_AUTOCLICK="0,1,2"` navigates the
    genuine `OnSelectRlistbox → CSQuick1 → CheckForMissingMission → FragFly2 → StartFlying` path into
    flight. Crash root-caused: `CSQuick1::SetIndex(currquickfamily=-1)` (the `INT3` range-guard doesn't
    halt on compat → NULL `GetAt`). Fix (boot scaffold, no game edit): click-mode pre-flight does
    world-init only + resets `currquickmiss=-1` so the game's own `CSQuick1` ctor initialises the QM
    screen. **Accepted:** faithful cockpit via real clicks, frame 150 90.7% non-black, no crash; 4.1
    unregressed; bare `./bob` exits 0. Evidence: PORT.md + `/tmp/sf42b.png`.
  - **4.3** ◐ **PLUMBING DONE, blocked → 4.3b.** Built the full return path: `BOB_AUTOQUIT` injects
    F12 (`KEY_CONFIGMENU`) → `View3d::CloseWindow(IDCANCEL)` → `WM_COMMAND` captured by compat
    (`CWnd::PostMessage`→`bob_capture_wm_command`, live-flight-only) → main-thread Run loop drains via
    `bob_process_flight_close` → game's own `Rtestsh1::OnCancel` + `OnFlyingClosed`→`LaunchScreen`. The
    close fires correctly (default-off, no spurious closes, bare `./bob` 0). **Blocked:** the teardown
    `~View3d → Lib3D::CloseDown` SIGSEGVs on a NULL vtable — `pDD7->SetCooperativeLevel` (idx 20) with
    pDD7's vtable zeroed (object freed mid-teardown; draw thread already stopped via `WaitEndDraw`). =
    a **compat DD7/D3D7 device-release refcount bug** in the never-before-run 3D shutdown.
  - **4.3b** ☑ **DONE (2026-06-17).** Compat COM bug: `GLDD7` had no refcount — `DD_Release` freed on
    the first `Release`, so `CloseDown`'s balanced `getRefCount(pDD7)` freed pDD7 mid-teardown →
    `SetCooperativeLevel` use-after-free. Fixed: real `int ref` (init 1) + `DD_AddRef`/`DD_Release`
    (free at 0), matching the surface model. **Flight now shuts down clean**; the full chain
    `OnCancel→OnFlyingClosed→LaunchScreen(options3d)` runs. No regression (scaffold 91.7% non-black,
    bare `./bob` 0). Evidence: PORT.md + `/tmp/sf43b_bt.log`.
  - **4.3c** ☑ **DONE (2026-06-17).** Traced (combo-host SetIndex print): the crash was `SetIndex(3)` on a
    2-item Off/On `CSDetail` detail combo (post-flight `Save_Data` field out of range) — the **INT3-guard-
    doesn't-halt class** (3rd recurrence, cf. CSQuick1). Fix (general, compat-side): the RCombo host
    **refuses an out-of-range `SetIndex`** (honours the game's guard intent) — fixes the whole class for
    every hosted combo. **Accepted:** full **fly→F12→exit→options3d menu** round-trip renders in one
    process (options3d 800×600 99.97% non-black, `back in front-end`); no regression. Evidence: PORT.md +
    `/tmp/bobgdi.png`.

  **✅ R1.1b COMPLETE — the menu↔flight control-flow window merge is done (Sprint 4 goal met):** menu→flight
  (forced + real-click) and flight→menu (clean teardown + return to a rendered front-end screen), one process.
- **Risk (spike-flagged):** a "cascade of uninitialised-UI failures" in the dialog/paint compat —
  treat as an onion (per the Sprint-2/3 retro); each fix logged in `PORT.md`, ASan available.
- **Increment demo:** `BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_STARTFLYING=1 ./bob` enters flight through
  the game's own `StartFlying` screen flow on the front-end window.

### Sprint 2 (original plan, superseded by the CLOSED block above)
- **Sprint Goal:** `InitPreferences()` is the real default init; flight runs faithfully with
  **no `BOB_*` Save_Data forces**.
- **Committed (PO-approved, 16 pts):** R1.3 (8), R1.4 (5), R1.5 (3). R1.1b deferred to Sprint 3.
- **R1.3 broken out (from the R1.2 diagnosis):**
  - R1.3a — `shape::SetPilotedAcAnim`: scalar `delete` → `delete[]` (match `SetAnimData`'s `new[]`).
  - R1.3b — `keytests::Reg3dConv`: bound the index / size the table to the real `FileNum` range.
  - R1.3c — InitPreferences's trilinear default → `CopyMapToSurface` NULL-deref: pin `filtering`=BILINEAR
    in the boot scaffold after `InitPreferences` (the deep trilinear mipmap-upload fix stays deferred → R3.5).
- **PO decision (Sprint-2 planning):** R1.3a/b done as a **minimal documented game-code exception** to the
  "sources stay unedited" rule — root-cause UB fixes (~2 lines), each logged in PORT.md with ASan evidence.
  (Rejected: compat-side global-operator mitigation — invasive, leaves the UB latent.)
- **Increment demo:** `BOB_RUN_INIT=1 BOB_DRIVE_C=… ./bob` reaches flight with terrain/
  clouds/cockpit/sound/HUD live and zero feature-forcing env vars.
- **Ships Release 1.**

### Sprints 1–7 (Releases 1–2 + DoD) — ✅ CLOSED
Sprints 1–4 (Release 1, R1.1b window merge), 5–6 (Release 2, play-a-mission + campaign continuity),
7 (env-var-free default boot → **DoD met**) are done — see the Burndown table (§9) and PORT.md. The
game boots + plays a Quick Mission end-to-end, no env vars; a human pilot has flown it.

### Sprint 9 — "Faithful flight II" → *Increment: render-fidelity polish + flight stability* — **ACTIVE (2026-06-17)**
- **Sprint Goal:** finish the Release-3 render-fidelity tail and harden in-flight stability. **Ships Release 3.**
- **Opened with a field fix banked:** ☑ **R3.9 ground-impact crash** (5 pts) — pilot-reported crash on
  hitting the ground; root-caused (landscape-tile `new[]`/`delete` mismatch, ASan 10→0) + fixed +
  committed (`ceb3083`) before the planned stories. Standing PO approval.
- **Committed (planned, ~23 pts):** R3.4 mirror-horizon UVs (5), R3.5 trilinear mips (5), R3.6 terrain
  detail/over-tiling (5), R3.7 in-flight effects (5), R3.8 render regression sweep (3). The R3.2 cloud-depth
  spike (13, carried from Sprint 8) runs as a **dedicated focused spike** within/after this sprint — it
  needs a Wine A/B and is *not* a live patch (Sprint-8 retro).
- **Starting story — R3.4 (mirror-horizon UVs):** well-scoped, known root cause (PORT.md: `InfiniteStrip`
  horizon quads carry garbage v-texcoords, `v≈-2.4e24`, clamped to one edge texel → flat mirror), compat-side
  (no game edit), headlessly verifiable (`BOB_MIRROR` + `BOB_DUMP_RTT` → mirror FBO variance > 0).
- **Increment demo:** `BOB_MIRROR` rear-view mirror shows the horizon/sky backdrop (not a flat edge texel);
  in-flight stability holds a long session; bare `./bob` exits 0.

---

## 7a. Forward roadmap — to "all functionality" (regroomed 2026-06-17)
~20–25 pts/sprint (calibrated to velocity). Each sprint = a shippable, demoable increment. Releases
pulled in priority order; first-pilot fidelity (R3) leads because it's what's visibly wrong today.

| Sprint | Release | Goal / committed stories | ~Pts |
|---|---|---|---|
| **8 (NEXT)** | R3 | **Faithful flight I** — scene lighting (R3.1) + cloud depth/draw-order (R3.2) + cloud sprite fidelity (R3.3). The two visible first-pilot bugs + clouds. | 19 |
| 9 | R3 | **Faithful flight II** — mirror UVs (R3.4), trilinear mips (R3.5), terrain detail/over-tiling (R3.6), effects (R3.7), regression sweep (R3.8). **Ships Release 3.** | 23 |
| 10 | R4 | **Campaign I** — campaign front-end (R4.1) + strategic map screen (R4.2). | 21 |
| 11 | R4 | **Campaign II** — mission flow + day/period progression (R4.3) + save/load (R4.4). | 21 |
| 12 | R4 | **Campaign III** — uninit-state grind (R4.5) + campaign regression. **Ships Release 4.** | 13 |
| 13 | R5 | **Control & sim** — joystick (R5.1) + in-flight mouse (R5.2) + keymap/config UI (R5.3). | 13 |
| 14 | R5 | **Sim fidelity** — flight-model/damage/collision (R5.4) + AI combat (R5.5). **Ships Release 5.** | 16 |
| 15 | R6 | **Front-end I** — blit subsystem (R6.1) + font/DPI (R6.2) + remaining UI screens (R6.3). | 21 |
| 16 | R6 | **Front-end II + media** — replay (R6.4) + save/load round-trip (R6.5) + intro Smacker (R6.6). **Ships Release 6.** | 18 |
| 17–19 | R7 | **Multiplayer** — DirectPlay→sockets (R7.1, sliced) + H2H/co-op (R7.2). **Ships Release 7.** | 34 |
| — | ICE | **MIDI music** — blocked until a 32-bit soft-synth exists in the environment; revisit then. | 8 |

**End state (all functionality):** every BoB mode — Quick Mission, full Campaign, Multiplayer — playable
natively on Linux with faithful flight/cockpit/AI/effects, the complete front-end, joystick+mouse+keyboard,
save/load, replay, and intro video. Only MIDI music remains env-gated. **~9–11 sprints out.**

> **Re-planning discipline:** spike fidelity/architecture unknowns before committing points (Sprint-1
> retro). R3.2 (cloud depth) and R5.4 (flight model) are A/B-against-Wine spikes, not fixed-size fixes.

---

## 8. Ceremonies

Adapted to an autonomous single-agent cadence (a "session" = a sprint):

- **Sprint Planning** (start of session): pull the top Ready stories up to the velocity
  budget; write the Sprint Goal here.
- **Daily Standup** (per work-batch): not a meeting — a one-line progress note. Surfaces in
  the agent's running narration; blockers escalate immediately (autonomous working mode).
- **Sprint Review** (end of session): demo the increment by running `./bob`; Product Owner
  accepts against the DoD. Record acceptance in `PORT.md`.
- **Sprint Retrospective** (end of session): one "what to improve" note appended below.

---

## 9. Burndown / Velocity Tracking

| Sprint | Committed pts | Done pts | Increment shipped? | Notes |
|---|---|---|---|---|
| 0 (pre-Scrum) | — | ~76 | ✅ baseline | Calibration baseline. |
| 1 | 21 | 16 | ✅ accepted | PO-accepted as Done (Sprint Review, 2026-06-17). R1.2 (13) done; R1.1 split → R1.1a (3) done, R1.1b (8, re-est.) blocked-by R1.3, carried. Tooling: `BOB_ASAN` build + valgrind memcheck (cross-validated R1.2) added. |
| 2 | 16 | 7 | ⚠️ partial | Shipped R1.3a/b/c (setup corruption fixed; InitPreferences reaches flight). R1.3d (transient double-free, NEW) gates init-as-default → R1.4/R1.5 + R1.3d split to Sprint 3. **Release 1 slips to Sprint 3.** PO-accepted the corruption-fix increment. |
| 3 | 16 | 16 | ✅ **Release 1 shipped** | R1.3d (transient double-free root-caused + fixed), R1.4 (InitPreferences = default init), R1.5 (regression: faithful flight, no feature env vars). Full commit delivered. |
| 4 | 13 | 13 | ✅ **R1.1b shipped** | **R1.1b control-flow window merge DONE (2026-06-17).** Faithful path: the game's own `LaunchScreen(quickmissionflight)→StartFlying→Rtestsh1→Launch3d` (4.1), reached by **real menu clicks** (4.2, fixed CSQuick1), and the **return path** (4.3) F12→`CloseWindow`→`OnCancel`→`OnFlyingClosed`→menu — with the **DD7 refcount teardown fix** (4.3b, real compat COM bug) + the **out-of-range-SetIndex guard** (4.3c). Full **menu→fly→menu in one process on one window**. Cross-port notes synced with MiG Alley. No regression (bare 0; 4.1/4.2 ~89-92% non-black). R2.1/R2.2 → Sprint 5. |
| 5 | 21 | 21 | ✅ **R2.1+R2.2 shipped** | **"Play a mission" (2026-06-17).** R2.1 done (real menu Fly already drives `LoadSetPiece` via R1.1b; `SetUpHotShot` unsafe on BoB data → our pre-flight is the faithful equivalent). R2.2 done (EXITKEY/Alt+X → IDOK → `OnFlyingClosed` → `quickmissiondebrief` renders, artnum 27924; F12→IDCANCEL→options3d intact). The game's own **menu→fly→exit→debrief** runs end-to-end, one process. No regression. R2.3/R2.4 → Sprint 6. |
| 6 | 21 | 21 | ✅ **R2.4 + R2.3 — Release 2 done** | **Campaign continuity + mission-loop stress (2026-06-17).** R2.4: `BOB_REFLY=N` chains missions — 2+ consecutive fly→debrief→fly→debrief cycles in one process (debriefs render; 2nd `StartFlying` clean). R2.3: 4-mission chain + `BOB_QM_INDEX=0..7` variety, **0 crashes** (grind pre-empted by upstream fixes). **Release 2 ("play a mission") complete** — the game's own menu→mission→fly→debrief→next loop runs end-to-end + survives stress. |
| 7 | ~16 | ~16 | ✅ **DoD MET** | **"No env vars" — DEFINITION OF DONE (2026-06-17).** inc1 mission setup game-driven; inc2 real click-through flies with NO `BOB_STARTFLYING` (boot device-init + always-on bridge + frame-based auto-quit fixing a black-flight bug); inc3 **bare `./bob` from the install dir boots the real title screen, ZERO env vars**, mouse+keyboard playthrough flies → debrief → menu. **First human pilot flew it.** Field fixes (R2.5): gun-fire double-free crash (R1.3e, 7 sibling classes) + menu click-offset. Safe fallback + `BOB_NO_RUN`/`BOB_RUN_INIT` preserved. |
| 8 | 19 | 13 | ✅ **closed — 2 wins** | **Faithful flight I (2026-06-17).** ☑ R3.1 scene lighting (pilot-confirmed — pre-dawn time bug), ☑ R3.3 cloud checkerboard (smooth-alpha → GL_LINEAR, pilot can re-test). R3.2 clouds-over-cockpit re-scoped 8→13 to a dedicated **Sprint-9 depth spike** (2 failed real-time attempts: RHW-z not sent to GL; then a wrong z-mapping blanked the scene — needs careful work, default rendering unaffected/reverted). PO standing approval; closed per the onion rule (bank verified value, spike the unknown). |
| 9 | 23 | 5 | ◐ **active** | **Faithful flight II (2026-06-17).** Banked field fix ☑ **R3.9 ground-impact crash** (`ceb3083`). ☑ **R3.4 mirror RE-DIAGNOSED** on real GL (renders flat sky, not garbage-UV; RTT→RTT dump tooling; niche → re-pointed; `68c0f85`). ◐ **R3.2 depth-sort spike** (pilot: "landscape shows through cockpit"): built the gated `BOB_ZDEPTH` path with real-GL A/B — correct z-mapping `glOrtho(0,w,h,0,0,-1)`, force depth-write, **opaque-only** write (clouds stay painter's-ordered → no blur streak); at-altitude clean/non-blanked; **awaiting pilot A/B of the terrain-behind-cockpit case** (`29dc873`). Real GL display (`:0`) is the big unlock this sprint. |
| 10 | ~21 | 18 | ✅ **3 wins + R4 opened** | **Faithful flight III → Campaign I (2026-06-21).** ☑ **R3.5 trilinear mipmaps** (`94fd95c`) — built the compat attached mip-chain; trilinear is the faithful default again (was the R1.3c bilinear pin). ☑ **R3.6 per-stage addressing** (`49c3ed4`) — honour D3DTSS_ADDRESS MIRROR/CLAMP/WRAP (was GL_REPEAT-everywhere over-tiling). ☑ **R3.2 re-A/B'd** on real GL — forced depth-write regresses the prop, stays gated. ☑ **R4.1 campaign front-end** (`0789383`) — wired the SideSelect polygon hit-areas (RAF/LW nav) + fixed the campaignselect hosted-listbox NULL-column crash; the campaign front-end (side-select → phase-select → enter-name) is navigable, `LaunchMapFirstTime` runs. **R4.2 strategic-map render is next.** |
| 32 | ~5 | 5 | ★ **R4.4 DONE — save/load fully click-driven** | **File-row-click bridge (2026-06-24).** Wired the last R4.4 piece: clicking a save in the load list selects it via the genuine `OnSelectRlistboxfile`, so save/load is fully click-driven (no scaffold). The OCX Select eventsink is a no-op on Linux → targeted `BOB_LINUX` bridge (mirrors R5.3b): `CLoad::bob_file_clicked`→protected `OnSelectRlistboxfile`, `g_bobCLoad` registry (set in OnInitDialog), `OleHost::rowAtY`→`HostRListBox::GetRowFromY`, routed by `bob_ole_click` (~40 lines, 5 files). Verified by genuine clicks (`BOB_CLICKXY`): click "Auto Save" row → `filename='Auto Save.bsr'` → click "Load" → `LoadGame=1 → LaunchMap → strategic map active`, no crash. **R4.4 save/load COMPLETE** (save→list→select→load→map all via the UI). No regression (targeted shim + default-virtual; bare boot + map clean). 2nd OCX bridge → MA general eventsink the adopt-target. |
| 31 | ~5 | 4 | ◐ **R4.4 load → campaign map** | **Menu-driven load completes (2026-06-24).** Loading a save from the load screen now restores the campaign + enters the strategic map. Spike: the "Load" menu item is `DoLoadGame` = `CFiling::LoadGame`(S29) + `LaunchMap` — NOT the R4.2-blocked `CFiling::OnOK` CMainFrame path; so only `selectedfile` + LoadGame + LaunchMap are needed (all work). The one OCX gap is file selection (`OnSelectRlistboxfile` on the no-op eventsink); pre-seeding fails (setup overwrites `selectedfile` with the player name; the Load nav re-inits CLoad and wipes it). `BOB_LOAD_GO` scaffold sets `selectedfile` (what a row-click sets) + runs DoLoadGame's body directly. Verified: `LoadGame=1 currtime=32180 → LaunchMap done → strategic map active`, no crash (`doc/reference/loadgame-into-map-2026-06-24.png`). Save→load→list→enter-map all work; remaining: the OCX file-row-click bridge. No regression (gated; bare boot + map clean). |
| 30 | ~5 | 4 | ◐ **R4.4 load screen lists the save** | **CLoad file-list enumeration fixed (2026-06-24).** The loadgame screen now LISTS the campaign save ("Auto Save"). Root cause: `CLoad::MakeFileList`'s `_findfirst` search path used the corrupted `fakefile` savegame path — the **3rd twin** of the S28/S29 SaveGame/LoadGame bug (compat `_findfirst`/`_findnext` are real — opendir+fnmatch — only the path was wrong). Fixed: relative `savegame/<wildcard>` for FIL_SAVEGAMEDIR (BOB_LINUX), case-insensitively resolved. Verified: `listempty=0` + screen capture shows "Auto Save" in the file list (`doc/reference/loadgame-lists-save-2026-06-24.png`; MA's row-at-`y=-rowheight` render bug N/A to BoB). Save→load→list now all visible. Remaining R4.4 UI: click→load (OCX eventsink — R5.3b bridge/MA adopt). No regression (BOB_LINUX path fix + gated trace; bare boot + map clean). |
| 29 | ~8 | 8 | ★ **R4.4 save/load round-trip works** | **Load restores campaign state (2026-06-24).** Completed the load half: `CFiling::LoadGame -> OK, currtime 26660->32180` (loaded = saved state). First disproved the Sprint-28-retro "general ConvertPtrUID sentinel" hypothesis (its null-handling is already safe — `info_*Ptr` conversions are pure address casts, no memory read; crashes are genuine). Then ASan-pinned the deserialise crash to `MIGView.cpp:2210` — the `raidnumentries[r]` terminator loop overrunning because loaded packages lack the terminator `RecostRaidList` sets at runtime (R4.5 family). Fixed: re-run `RecostRaidList()` on loaded packages in `PackageList::LoadGame` (MAPCODE.CPP, BOB_LINUX) + `if(ac)` guard the 3 unguarded `ac->SetDraw()` in SetVisibilityFlags. ASan-verified the overflow is gone (remaining ASan noise = pre-existing benign FILEMAN/odr twins). No regression (BOB_LINUX load-path fix + harmless NULL check; bare boot + normal map clean). **R4.4 save/load core done** (UI file-list enumeration remains). |
| 28 | ~8 | 5 | ◐ **R4.4 campaign save persists** | **First save on Linux (2026-06-24).** Unblocked the long-gated save/load (campaign now runs → produces state). **A campaign save lands on disk** (`SAVEGAME/Auto Save.BSR`, 225KB real `Miss_Man`). Root cause of "no saves ever": `SaveGame` used the corrupted `fakefile` savegame path that `LoadGame` was already BOB_LINUX-bypassed for but `SaveGame` wasn't — fixed (mirror the bypass). `BOB_CAMPAIGN_SAVE` scaffold. Load characterized into two scoped gaps (deferred): CLoad file-list enumeration (same path bug; adopt MA's CLoad fix) + `LoadGame` deserialisation crash (`SetVisibilityFlags` `*ConvertPtrUID(uid)` deref-before-NULL-check, R4.5 family, gdb-pinned). No regression (scaffolds gated; bare boot + normal map clean). Cross-port: BoB now matches MA's save side. |
| 27 | ~5 | 3 | ◐ **R4.2 icon spike (root-caused)** | **Why the strategic map shows no icons (2026-06-24).** Spiked the dynamic-icon render path (gold-standard refs show squadron/raid icons + route lines). Found the path is wired + reached (`UpdateBitmaps→DrawIcons→DrawIconTest→MaskIcon` on R6.1's blit) but `DrawIcons` receives an **empty `inter` clip rect (0,0,0,0)** → the visible-world-rect cull rejects *every* item (`raw_p` large = items exist, `survived_cull=0=drawn`). The terrain renders because per-block `StretchDIBits` uses the block rect directly, not `inter`. Root: the icons are positioned by the **`CMapDlg` scroll/world transform** the headless `UpdateBitmaps(&dc,fullrect)` shim never sets up → R4.2 icons is a coordinate-system subsystem (drive the real CMapDlg paint; candidate to adopt MA's map view), not a one-liner. No code shipped (trace added/measured/reverted — game code pristine; build + campaign loop unchanged). Precise characterization banked. |
| 26 | ~8 | 6 | ★ **R4.3 campaign mission CYCLE closes** | **Fly → mission-end → back to map (2026-06-24).** Closed the campaign mission loop: after flying, end the mission (`BOB_AUTOQUIT=debrief` → EXITKEY → `OnFlyingClosed(IDOK)`) and **return to the strategic map**. Spike: `OnFlyingClosed` routes by `gamestate` — the campaign flight is COMMANDER(5), so the **else/campaign branch** (`NextMission`+`StartUpMapWorld`+`LaunchMap`) runs (no new code). Verified on `:0`: flight → `flight close (IDOK)` → `LaunchMap done` → `back in front-end (InThe3D=0)`. The never-run-on-Linux campaign return (CMainFrame/toolbar ops, StartUpMapWorld, LaunchMap) runs clean. One deferral: fast-forwarding the *post-mission* sim SIGSEGVs in `GetCruiseAt`/`Plane_Type_Translate[bad ptype]` (post-mission SAG `type` uninit — **R4.5 grind**, precisely characterized via gdb); guarded off (`g_campfly_flown`) so the returned map is stable (139→124), faithful (returned map is paused in real play). No regression (guard is `!g_campfly_flown`; normal map sim + bare boot unaffected). **Release 4 core loop closed.** |
| 25 | ~8 | 8 | ★ **R4.3 a campaign mission FLIES** | **Campaign briefing → Fly → cockpit (2026-06-24).** Closed the campaign loop to actual flight: from the mission briefing (Sprint 24), drove **Fly** → the campaign mission flight renders in the cockpit on real GL. Spike finding: `bobfrag`'s Fly item navigates to the **same `quickmissionflight` screen QM uses** (`StartFlying` InitProc), so the campaign flight **reuses the proven QM Launch3d bridge** — not a new subsystem. Extended the scaffold (`BOB_CAMPFLY_GO`) to trigger the Fly nav; one fix needed — a just-scrambled interceptor is `PS_ACTIVE_MIN` (outside the briefing's flyable-status window) so `playersquadron` stayed -1 and `FragFly2` blocked Fly → set `MMC.playersquadron` from the package at Fly time. Verified on `:0`: intercept → bobfrag → `Fly→StartFlying→Launch3d→InThe3D=1` → frame 120 cockpit (gunsight, instruments, scrambled sqn on the runway). No regression (Fly gated behind `BOB_CAMPFLY_GO`; bare boot clean). Capture: `doc/reference/campaign-mission-cockpit-2026-06-24.png`. **Release 4 flyable end-to-end (intercept slice).** |
| 24 | ~8 | 5 | ◐ **R4.3 campaign mission briefing reached** | **Campaign map → mission briefing (2026-06-24).** With the strategic-map sim running (R4.5), drove the campaign's own map→mission seam to the **mission briefing** (`bobfrag`), bypassing the CRToolBar/mission-folder OCX subsystem (a separate ~13pt piece). Mapped the seam (`OnClickedFrag2 → LaunchFullPane(&bobfrag, UIR_FRAG)`), then found a fresh day has only the **AI opponent's** packages — so a player package must be *created*. New `BOB_CAMPAIGN_FLY` scaffold performs the faithful **interception** (`OnClickedRbuttonauthorise` equivalent): find an airborne enemy raid (`Squad::instance!=0` — a 0 instance SIGSEGVs `NewPackage`), `Todays_Packages_NewPackage(inst, RAF intercept profile)` **scrambles a real interceptor**, `LaunchFullPane(&bobfrag)`. Verified headless: intercept → `NewPackage→packnum=3` → **bobfrag briefing renders** (montage + Back/Sim Config/**Fly**), **no crash**. Added `bob_gdi_dump_to` (deterministic one-shot capture). No regression (all gated behind `BOB_CAMPAIGN_FLY`; bare boot enters Run() clean; normal map advances). Reviewed gold-standard Wine captures (`/run/media/m/BEA6-BBCE/bob`) — confirm the R4.2 dynamic-icons + toolbar tail. Remaining R4.3: briefing widget population + **Fly→StartFlying** campaign flight + debrief→next-day. |
| 23 | ~3 | 3 | ◐ **R6.5 load screen + save/load mapped** | **Load-game screen + cross-port (2026-06-23).** `loadgame`/`CLoad` reaches + renders (RAF/LW/Back/Load menu + hosted file-list), empty (no saves). Mapped the dependency: `CFiling::SaveGame` serialises campaign `Miss_Man`+map → save/load is gated on the campaign (R4.3), not independent. BoB shares the `CLoad` class with MiG Alley; MA already did the file-list render + click->DoLoadGame (S12-S14) -> ports near-directly once saves exist. Eventsink deferred (adopt MA general one when wiring the CLoad click). `BOB_CONFIGSCREEN=load`. No regression. |
| 22 | ~5 | 5 | ◐ **R5.3 controls rebind interactive** | **Controls rebind (2026-06-23).** Clicking a hosted device/axis combo now reassigns the control. The OCX eventsink (`ON_EVENT`) is a no-op on Linux, so a combo cycle changed only the display; a general eventsink would touch every dialog (vtable/macros) -- high risk for a power-user feature. Targeted bridge instead: `BOB_LINUX SController::bob_combo_changed(ctrlId)` dispatches to the genuine `OnTextChanged*` handler (X-macro list of 26 combos, compiler-validated); registers in `OnInitDialog`, called from `bob_ole_click`. Verified: cycling Stick combo changes its value AND recomputes the other combos (freed axes appear as options) -- genuine interdependent reassignment; persists via unedited `PreDestroyPanel→SetAxisConfig`. Fixed `BOB_CONFIGSCREEN` starving click injection. No regression (bare 0; flight clean). |
| 21 | ~5 | 5 | ◐ **R5.3 controls form + game-wide text fix** | **Controls config screen + CString-varargs fix (2026-06-23).** Brought up the `SController` Controls form via a `BOB_CONFIGSCREEN` scaffold — it renders as a complete device/axis-assignment form (was a SIGSEGV), validating the R5.2 EnumObjects fix in the front-end. Root-caused the garbled combo text to the **CString-in-varargs Itanium-ABI bug** (game pervasively passes `CString` to `%s` without a cast → by-ref on GCC, by-value on MSVC) and fixed it in compat `CString::FormatV`: numeric formats keep `vsnprintf`; `%s` formats discriminate CString-by-ref vs char* via CStringData-header validation + `/proc/self/maps`-guarded reads (bounded blast radius — only already-broken `%s` formats change). Combos now read real device/axis names. No regression (bare 0; flight, GFX, Sound forms clean). Remaining R5.3: rebind interaction + keymap screen. |
| 20 | ~3 | 3 | ✅ **R5.2 in-flight mouse** | **In-flight mouse (2026-06-23).** DirectInput→SDL mouse device in compat (mirrors R5.1 joystick): EnumDevices/CreateDevice(GUID_SysMouse)/EnumObjects(rel X,Y+buttons)/SetDataFormat/buffered GetDeviceData(`SDL_GetRelativeMouseState`)/GetCaps. Default maps mouse→AU_UI_X/AU_UI_Y, enabling the in-3D UI cursor (was disabled). **Found+fixed a real R5.1-era bug:** `DIDEV_EnumObjects` ignored the DIDFT filter → controls-config miscounted buttons as axes → `firstaxes` underflow → OOB write SIGSEGV (joystick alone stayed under the bound; the mouse tipped it over). Now honours `DIDFT_AXIS/BUTTON/POV`; flight (all types) unregressed. Verified offsets=AU_UI_X/Y + axisvalues track injected motion; bare 0; joystick flight unregressed. Pending PO mouse fly-test. |
| 19 | ~5 | 5 | ◐ **R4.5 hardening** | **Campaign sim hardened + driveable (2026-06-22).** With the sim running, drove the day (`BOB_MAP_TIMER=N` un-pause + fast-forward) and confirmed it's alive: clock advances, raids generate (worlditems 1052→1111). Fixed a 3rd ASan-found campaign bug: negative-index OOB read in `WhereToReassignProduction` (NODEBOB.CPP:7107, `bestsq==-1` → `production[-1]`; `bb04166`). Remaining ASan residue is non-sim (front-end layout overflows — likely odr-artifacts of the unity-build twins; FILEMAN dir-list). Next: dynamic raid icons (needs sim-time/scroll), `CMainFrame` toolbars, R4.3 mission flow. |
| 18 | ~8 | 8 | ★ **CAMPAIGN SIM RUNS** | **R4.5 unblocked (2026-06-22, `c46f2c2`+`d42dbd2`).** Per the prior plan, ran `build-asan` under the day-start sim — ASan was the oracle. Fixed two raid-generator heap bugs: (1) `new[]`/`delete` mismatch in `ReorderPackage` (PACKAGES.CPP:5828); (2) raid-list terminator off-by-one in `RecostRaidList` (PACKAGES.CPP:5881). ASan re-validated: alloc-dealloc 12→0, heap-overflow 14→4, the fatal `MoveAllSAGs` reads gone. The strategic-map campaign day now advances 90s with **no crash** (was SIGSEGV on frame 1). No regression (bare 0, QM flies). Remaining R4.5 tail: lower-severity reads (`SetVisibilityFlags`, FILEMAN, `PositionRListBox`) + dynamic raid icons/toolbars. |
| 17 | ~8 | 5 | ◐ **R4.5 root triangulated** | **Crash pinpointed (2026-06-22 cont.).** Flushed gated traces corrected the mapping: `movecode=2`=`AUTOSAG_WAITTAKEOFF` (fresh raid), so the crash is `SAGDecisionWaitTakeOff` (SAGMOVE.CPP:1204) `waypoint->ETA` with the SAG's `waypoint` member NULL. Triangulated: both `waypoint` and `ConvertPtrUID(wpref)` are NULL because the raid's flight-plan **waypoint items aren't in `pItem`** — `AutoLWPackages` builds the SAGs but not their waypoints on Linux. Ruled out the `ReassignTo`/dangling-`wpref` theory (never ran). Next: trace `AutoLWPackages` waypoint creation (PACKAGES.CPP:2379/2616/4686), ideally with `-O0 -g`/ASan on the campaign TUs. All traces reverted; map render stable. |
| 16 | ~8 | 5 | ◐ **R4.5 grind entered** | **Campaign-sim BREAKTHROUGH (2026-06-22, `7c16628`/`R4.5`).** Instrumented `MoveAllSAGs` → the sim RUNS (init ✓, raid SAG created+tabulated, `as` non-NULL, `localplayer=1`); overturned the "not deployed" theory. The crash is an uninit-state bug in the SAG movement AI: `movecode=AUTOSAG_FOLLOWWP` → pinned to `GetCruiseToWp` (SAGMOVE.CPP:1364) `despos=wp->World` with `wp=ConvertPtrUID(wpref)=NULL` — the raid's flight-plan waypoint isn't in `pItem`. Node-tree-rebuild hypothesis tested + ruled out. This is the R4.5 grind (raid-data lifecycle); next: trace `AutoLWPackages` waypoint creation. Map render stable. |
| 15 | ~13 | 3 | ◐ **R4.3 root-cause** | **Campaign-sim crash pinned (2026-06-22, `e9a8b85`).** Deep-dived the `MoveAllSAGs` SIGSEGV across 3 hypotheses (NULL-guard, `WipeAll`, gdb fault analysis). Root: `ConvertPtrUID(uid)=pItem[uid][0]` and `pItem[uid]`=NULL — the day's raid SAGs were never created/tabulated; the headless click-through reaches the map but doesn't run the full campaign-start/deployment (OOB deploy + LW directive AI raid generation). Both candidate fixes ruled out + reverted. Carried as a dedicated campaign-deployment investigation; map render stays stable (`BOB_MAP_TIMER` gated off). |
| 14 | ~13 | 5 | ◐ **R4.3 clock spike** | **Campaign clock wired (2026-06-21, `08be1cc`).** A `BOB_LINUX` `CMapDlg::bob_drive_timer()` forwarder driven from the map tick replaces the dead MFC `WM_TIMER` → `OnTimer`/`StartOfDay`. Pinned the sim crash to `SAGMOVE.CPP:970` (`ConvertPtrUID(Todays_Packages[p][s].instance)`=NULL → the day's raid packages exist but their SAG items aren't deployed in the world). Gated OFF (`BOB_MAP_TIMER`); map render stable. Carried: campaign deployment so the squadron UIDs resolve, then the sim + briefing→fly→debrief→next-day. |
| 13 | ~13 | 8 | ◐ **R4.2 map terrain** | **Strategic map renders (2026-06-21, `e7600f7`).** Built C-GDI `StretchDIBits`/`FillSolidRect` on R6.1 + a map-paint tick (`g_bob_map_active`/`bob_map_paint_begin`); `CMIGView::UpdateBitmaps` now draws the real strategic map — SE England + Channel + France, RAF sectors A–E/Y/Z, city labels, No.11 Group (`/tmp/r42_map.png`). Window kept live by the tick. No regression (QM flies, map gated off). Remaining R4.2: unit icons need campaign data populated (→ R4.3 day/mission setup; `DrawIcons` path runs, no units placed yet at first entry), `CMainFrame` toolbars, scroll/zoom + click. |
| 12 | ~8 | 8 | ✅ **R6.1 blit subsystem** | **GDI blit (2026-06-21, `89c0d20`).** Built `bob_gdi_blit.cpp` (bitmap registry + DIB decoder 8/24/32/4/1-bit + ROP blit SRCCOPY/AND/PAINT/INVERT) and wired pixel-backed `CBitmap` + memory-DC `CDC::SelectObject`/`BitBlt`/`StretchBlt` + `CreateDIBitmap`. Verified: icon sheet decodes (1408×1024, dumped perfect) + blits to framebuffer (`BOB_BLIT_TEST`). No regression. **Unblocks R4.2** (map tiles via `StretchDIBits`, now implementable on `bob_dib_decode`+`bob_stretchblit`). |
| 11 | ~13 | 5 | ✅ **R6.2 + R4.2 spike** | **Campaign II — font fix + map spike (2026-06-21).** ☑ **R6.2 multi-line font** (`cece1d0`) — the giant overlapping campaign-description text was `bob_ole_draw_panel` setting the font = the control's box height; capped multi-line boxes to the single-line dialog font (single-line controls unchanged). ◐ **R4.2 strategic-map SPIKED** — fully characterized: enter-name `Begin → LaunchMapFirstTime → StartUpMapWorld` (world data load, runs) → `CMIGView::LaunchMap` destroys the front-end panel and hands off to a **parallel UI** (`m_mapdlg` scrolling map view, driven by `OnIdle`→`InvalidateAnotherItem`, + `CMainFrame` toolbars) that isn't wired for compat rendering — a ~13-pt subsystem equivalent to the whole front-end bring-up. No crash (idle→SDL_QUIT). Carried as the dedicated next sprint. |
| **12–19** | — | — | — | **See §7a roadmap** → R4.2 strategic-map subsystem (next), R4.3+ mission flow / save-load, R5 control & sim, R6 front-end & media, R7 multiplayer. |

Update the **Done pts** column at each Sprint Review; that's the running velocity. Cumulative done: ~133 pts
across Sprints 0–10 (Releases 1–2 + DoD + most of R3 + R4.1). Remaining to all-functionality: ~157 pts (§7a) —
R3 tail (effects/mirror, pilot-gated), R4.2+ campaign, R5 control & sim, R6 front-end & media, R7 multiplayer.

---

## 10. Retrospective Log
*(Newest on top. One improvement note per sprint.)*

- _Sprint 32 (R4.4 done — save/load fully click-driven):_ **The bounded bridge that S31 deferred took ~40
  lines and finished the whole feature — and the prior sprint's "blocker" was partly a phantom.** S31 worried
  the menu re-init would wipe `selectedfile` even after a row-click; S32 just *built the row-click bridge and
  tried it* — and the genuine flow worked first time, because the row-click sets `selectedfile` on the live
  screen (unlike S31's pre-seed that setup overwrote). Lesson: **a hazard observed under a workaround doesn't
  always transfer to the real path — verify the genuine interaction before assuming the blocker generalises.**
  The implementation reused three existing pieces cleanly (`GetRowFromY` already on the host; the R5.3b
  registry pattern; `bob_ole_click`'s hit-test loop), which is why it was small — a payoff of the earlier
  targeted-bridge investment. Meta-note on the whole R4.4 arc (S28→S32, five sprints): save/load went from
  "no save ever lands" to "fully click-driven" by finding **one bug class repeated at four sites** (the
  `fakefile` path: Save/Load/CLoad-enumeration) **plus two real fixes** (the ASan deserialise-terminator,
  the eventsink bridge). Worth flagging forward: BoB now has **two** targeted OCX-eventsink bridges (R5.3b
  controls + S32 CLoad) — the threshold the cross-port notes set for adopting MA's *general* `ma_eventsink.cpp`
  is now met; doing so would retire both bridges and pre-wire every future dialog's events.

- _Sprint 31 (R4.4 load → campaign map):_ **Finding the CLEAN seam beat fighting the messy one — and when a
  UI-state subtlety resisted, driving the game's own action body directly was the faithful shortcut.** Two
  load-completion paths exist: `CFiling::OnOK` (CMainFrame/toolbar ops — R4.2-blocked) and `DoLoadGame`
  (`LoadGame` + `LaunchMap` — both work). Spotting that the *menu* "Load" routes to `DoLoadGame`, not
  `OnOK`, meant the completion was already unblocked — no R4.2 needed. The friction was elsewhere: the OCX
  file-row click (which sets `selectedfile`) is a no-op, and every workaround to inject it (pre-seed before
  launch; fire the Load menu via `OnSelectRlistbox`) was defeated by CLoad's state churn (setup overwrites
  `selectedfile` with the player name; the menu nav re-inits and wipes it). The traces nearly hid this
  (`default='Bob'`, `selectedfile=''`) until the per-file match trace named it. Lesson: when a screen's
  state keeps resetting under you, stop poking the UI and **call the underlying action's body directly**
  (set `selectedfile` + run `LoadGame`+`LaunchMap`) — it proved the functional outcome (load → map) without
  the OCX eventsink, and scoped the *only* genuine gap (the row-click bridge) precisely. Process caution
  worth repeating: I spent too long iterating injection hacks before tracing the actual `selectedfile`
  lifecycle — when two or three workarounds fail the same way, trace the *invariant they're all fighting*
  (here, "who owns `selectedfile` across setup/re-init") before trying a fourth.

- _Sprint 30 (R4.4 load screen lists the save):_ **The same root-cause bug surfaced a THIRD time at a third
  call site — symmetric-twin bugs cluster, and once you've named the pattern, hunt every sibling
  proactively.** The corrupted `fakefile` savegame path bit `SaveGame` (S28), then `LoadGame` (S29's
  precondition was the original LoadGame patch), and now `CLoad::MakeFileList` (S30) — three independent
  call sites of one stateful-global path bug, each found reactively when its screen failed. The lesson
  (sharpening S28's "check the twin"): when a platform path/serialisation quirk hits one site, `grep` the
  *whole* family at once (`fakefile(FIL_SAVEGAMEDIR`, here) and fix them together — the cost of finding the
  3rd reactively (build+run+capture cycles) far exceeds the grep. Process win: the trace nearly misled me
  (`path='Bob'`) until I noticed `MakeFileList` *reuses the path buffer* for `filename` mid-function — a
  reminder that **a diagnostic that prints a reused buffer lies**; trust the orthogonal signal (`listempty=0`)
  and the pixel capture over a single mutable variable. Scoped well: shipped the visible win (load screen
  lists the save, completing save→load→list) and banked the click→load eventsink as the last R4.4 UI piece
  rather than chaining into the OCX-event subsystem at the end of a long session.

- _Sprint 29 (R4.4 save/load round-trip):_ **Reading the contract before coding the "obvious" general fix
  saved a wrong multi-site change — and ASan turned a 3rd-recurrence "family" crash into a precise one-line
  root cause.** The Sprint-28 retro's tempting idea was a blanket `ConvertPtrUID` safe-sentinel to kill the
  recurring `*ConvertPtrUID`-NULL crashes in one move. Ten minutes reading `WorldInc.h` disproved it:
  the pointer conversions are pure address casts (no memory read), so `ConvertPtrUID` is *already* NULL-safe
  and the guards work as written — a sentinel would have masked real bugs (exactly the R4.5 warning). The
  actual crash was something else entirely: ASan named it `MIGView.cpp:2210`, a heap-overflow in the
  raid-list terminator loop on *deserialised* packages — the **R4.5 `RecostRaidList` family**, re-applied at
  the load boundary. Lessons: (1) **a 3rd appearance of a "family" is a cue to find the shared invariant,
  not to add a shared guard** — here the invariant is "packages carry a terminated raid list," which the
  deserialiser broke, so re-running the runtime rebuild (`RecostRaidList`) on load was the faithful fix;
  (2) **ASan + `-g` is the decisive oracle for "is this NULL-deref or buffer-overflow?"** — gdb's bare
  backtrace looked like the NULL family; ASan proved it was an overflow, redirecting the fix. (3) the
  symbol-hunt for `PackageList::LoadGame` only resolved because the ASan frame carried the real file
  (`BFIELDS/MAPCODE.cpp`) — instrumented builds pay for themselves in navigation, not just detection. Kept
  well: verified the fix under a rebuilt ASan (overflow gone), and reused the `g_campfly_flown` post-action
  fast-forward guard so the round-trip demo is crash-clean without masking the separate R4.5 sim grind.

- _Sprint 28 (R4.4 campaign save persists):_ **"The same bug, on the sibling function" — fixing one half
  of a symmetric pair often leaves the other half latent; check the twin.** `LoadGame` got a `BOB_LINUX`
  savegame-path bypass months ago; `SaveGame` — its mirror — was never given it, so every campaign autosave
  silently went to a corrupted path. The fix was three lines once spotted, and it produced the first save on
  Linux. Lesson: when a function has an obvious twin (save/load, encode/decode, push/pop) and one was
  patched for a platform quirk, **grep the twin for the same pattern** — the asymmetry is a reliable bug
  smell. Second, the sprint stayed honest about scope: the *save* worked, but the *load* exposed two further
  layers (the CLoad file-enumeration hitting the *same* path bug, and a `SetVisibilityFlags`
  `*ConvertPtrUID`-NULL deref that's the R4.5 family). Rather than chase both into a multi-session grind, I
  shipped the save (a real, demonstrable milestone — a feature that never worked now works) and characterized
  the load into two scoped, evidence-backed gaps, with MiG Alley's CLoad work flagged as the adopt target.
  Reinforces the running theme: ship the verified increment, bank the precise characterization, don't let a
  deeper layer hold a working milestone hostage. Watch-item: the load deserialisation crash is the **third**
  appearance of the `ConvertPtrUID`-NULL family (R4.5 sim, R4.3 post-mission, R4.4 load) — a general
  `ConvertPtrUID` safe-sentinel vs. the per-site upstream fix is now worth a deliberate decision.

- _Sprint 27 (R4.2 strategic-map icon spike):_ **A two-counter trace beat hours of code-reading — and
  stopping at "root-caused, not half-built" was the right discipline after three delivery sprints.** The
  question "why no map icons?" had many plausible answers (blit stubbed? icon sheet unloaded? zoom gating?
  no data?). Reading the code narrowed it slowly; the **empirical trace settled it in two runs**: `raw_p`
  large + `survived_cull=0` + `inter=(0,0,0,0)` proved *items exist, the clip rect is empty* — a coordinate
  bug, not a data or blit bug. Lesson (recurring): for "why doesn't X render", **instrument the exact
  predicate (items-found vs survived-cull vs drawn) before theorising** — the counts collapse the
  hypothesis space instantly. Second lesson: the spike revealed R4.2 icons is the same *parallel-UI*
  subsystem Sprint 11 flagged (the `CMapDlg` scroll/world transform, which the terrain shim bypasses), so
  the right move was to **bank the precise root cause and ship nothing** rather than bolt a coordinate hack
  onto the terrain path — keeping game code pristine (trace reverted). A spike-only sprint is a legitimate
  increment when it converts an open "make icons work" into a scoped, evidence-backed subsystem story (and
  flags MA's further-along map view as an adopt candidate, mirroring the cross-port wins).

- _Sprint 26 (R4.3 campaign mission cycle closes):_ **Separating "the path works" from "a deeper sim bug it
  exposes" kept a real milestone from being held hostage by an onion.** The campaign return path
  (`OnFlyingClosed`→campaign branch→`LaunchMap`) worked on the *first* try — gamestate routing meant zero new
  code. But continuing to fast-forward the post-mission sim immediately SIGSEGV'd in the **R4.5 SAG-AI grind**
  (`GetCruiseAt`/`Plane_Type_Translate[bad ptype]`). The instinct to "fix the crash before claiming done"
  would have sunk the sprint into a multi-session ASan hunt. Instead: gdb pinned the crash to a *distinct
  onion layer* (post-mission SAG `type` uninit, not the return path), and the right move was a **faithful
  guard** — in real play the returned map is *paused*, so not headlessly fast-forwarding it isn't a hack,
  it's correct behaviour. That banked the milestone (cycle closes, 139→124) while characterizing the
  deferred bug precisely for R4.5. Lessons: (1) when a working path exposes a pre-existing deeper bug,
  **attribute the crash to the right layer** (gdb backtrace) before deciding scope — don't let layer-N's bug
  block layer-(N-1)'s milestone; (2) a test-harness artifact (fast-forwarding a normally-paused sim) is a
  legitimate thing to gate off, and doing so *faithfully* (matching real play) is better than a NULL-guard
  that masks the root cause and violates the no-edit-game-logic discipline. Watch-item: R4.3's remaining
  tail (day-advance, next mission) is **blocked on the R4.5 post-mission SAG-uninit** — the two stories have
  merged at the sim layer, so the next campaign push should run the post-mission world under ASan.

- _Sprint 25 (R4.3 a campaign mission flies):_ **Spiking the seam before committing scope turned a feared
  "campaign→3D mission-load subsystem" into a one-fix reuse — and prior infrastructure paid off massively.**
  The worry was that flying a campaign mission needed a whole new campaign→3D translation. The 10-minute
  spike (read `bobfrag`'s Fly menu item) showed it navigates to the **same `quickmissionflight`/`StartFlying`
  screen QM uses** — so the **already-proven R1.1b Launch3d bridge** carries the campaign mission into the
  cockpit unchanged. The entire sprint then reduced to (a) triggering the Fly nav and (b) **one** real fix
  (player-squadron at Fly time, because a just-scrambled interceptor sits outside the briefing's
  flyable-status gate). Lessons: (1) **read the target seam before sizing** (DoR "spike unknowns first") —
  it repeatedly converts "new subsystem" into "reuse + small fix" (cf. Sprints 11–14); (2) **investment in a
  general bridge compounds** — the QM flight bridge built three releases ago just delivered campaign flight
  for free; (3) **dummy-SDL proves logic, `:0` proves pixels** — the headless run confirmed `InThe3D=1` (nav +
  world build) and the `:0` run confirmed the cockpit rasterises, a clean split that kept iteration fast.
  Process watch-item carried from Sprint 24: dummy-SDL `bob` ignores SIGTERM and shares `/tmp` dump paths —
  always `timeout -s KILL` + private `BOB_DUMP_PATH`, and prefer an in-engine `_exit(0)`-after-dump for
  deterministic captures.

- _Sprint 24 (R4.3 campaign mission briefing):_ **The "drive the public seam, not the OCX UI subsystem"
  pattern carried the campaign forward again — and reading the data first turned a guess into the faithful
  action.** Rather than build the CRToolBar + mission-folder dialogs (a ~13pt subsystem) to reach the
  briefing, I drove the game's own public `LaunchFullPane(&bobfrag, UIR_FRAG)` — the same play as
  `BOB_CONFIGSCREEN`/`BOB_STARTFLYING`. But the first attempt (just highlight a package + launch) was wrong:
  a *scan of `Todays_Packages`* on the running map showed a fresh day holds only the **AI opponent's**
  packages, so a player mission must be **created**. That reframed the task as "perform the game's
  interception action" (`NewPackage` on a live raid squadron) — the genuine player experience, not a façade.
  Two debugging tells mattered: the `inst=0` SIGSEGV pinned the lifecycle subtlety (raids spawn into `pItem`
  only at takeoff time → guard on `Squad::instance!=0`), and a *re-scan-each-paint* loop beat the timing
  race (the first scans legitimately find no airborne raid). Process: I burned real time fighting the test
  harness (dummy-SDL `bob` ignores SIGTERM; multiple stuck instances corrupt the shared dump PPM) — the fix
  was a **deterministic in-engine one-shot capture** (`bob_gdi_dump_to` + `_exit(0)`) instead of racing a
  per-frame dump. Lesson: when a headless capture is flaky, make the *engine* emit the artifact at the exact
  state you want, rather than sampling from outside. The PO's gold-standard Wine captures arriving mid-sprint
  also re-confirmed the R4.2 tail (dynamic map icons + toolbars) is real and next.

- _Sprint 22 (R5.3 controls rebind):_ **Choosing a targeted bridge over the "correct" general mechanism
  was the right risk call.** The textbook fix for the dead OCX eventsink is to implement
  `BEGIN_EVENTSINK_MAP`/`ON_EVENT` generically — but that means member-fn-ptr dispatch, per-class maps,
  and a `CWnd` vtable change touching *every* dialog: a large, broad-blast-radius change to land a
  power-user rebind feature whose defaults already work. A scoped `BOB_LINUX` scaffold on the one screen
  that needs it (`SController::bob_combo_changed`, an X-macro list the compiler checks) got the same user
  outcome with the blast radius of a single file. Lesson (mirrors S21's blast-radius framing): when the
  "proper" fix is general infrastructure but only one caller needs it today, a targeted bridge is often
  the better sprint-sized move — and the X-macro-mirrors-the-event-map trick keeps it from rotting. The
  recompute-the-other-combos evidence (freed axes reappearing elsewhere) was the tell that it's the real
  logic, not a façade — pick a verification that only passes if the deep path actually ran.

- _Sprint 21 (R5.3 controls form + CString-varargs fix):_ **A scaffold-to-render the target screen turned a
  "make the UI work" story into a game-wide correctness win.** Forcing the Controls screen directly
  (`BOB_CONFIGSCREEN`) — instead of grinding the menu nav to reach it — got it on screen in minutes and
  immediately exposed the real defect: not a layout/combo bug but the **CString-in-varargs ABI mismatch**
  affecting *every* `CSprintf("%s",CString)` in the game. Banking-vs-fixing tension resolved by the
  *bounded-blast-radius* insight: because `%s`-formats were already 100% broken, a FormatV change scoped to
  only `%s`-formats can't regress a working (numeric) screen — which flipped a scary "touches all text"
  change into a safe one, worth doing now rather than deferring. Lesson: a cheap "jump straight to the
  artifact" scaffold pays for itself by surfacing the true root cause fast; and quantifying a fix's blast
  radius (what it can/can't break) is what licenses doing the bigger, higher-value fix in-sprint. Validated
  with a standalone ABI repro + cross-screen regression captures (controls/gfx/sound) before committing.
  flushed out a latent bug in the *original* pattern.** Reusing the DirectInput→SDL device shape for the
  mouse was fast, but enabling it SIGSEGV'd — and the root cause was an R5.1 shortcut (`EnumObjects`
  ignored the DIDFT type filter) that the joystick alone happened to survive (it stayed one slot under
  the config's `firstaxes` bound; the mouse's extra objects tipped it negative → OOB write). Lesson: when
  a second client exercises shared compat code, treat the first client's "it works" as *under-tested*, not
  proven — honour the real API contract (here, the enum filter) rather than the narrowest thing that
  passed. gdb's faulting-pointer value (garbage `m_pchData=3`) + tracing the index arithmetic pinned it
  fast. Kept well: trace-and-revert for the game-side `axisvalues` proof (game code stays pristine);
  diagnostics env-gated; every default-on path re-swept (bare 0, joystick unregressed).

- _Sprints 13–14 (R4.2 map render → R4.3 live-sim spike):_ **The R6.1→R4.2 bet paid off spectacularly —
  the blit subsystem lit up the whole strategic map in one increment — and the R4.3 spike correctly
  stopped a whack-a-mole.** Building R6.1 first meant R4.2's terrain render was a thin C-GDI shim
  (`StretchDIBits`→`bob_stretchblit`) + a paint tick: the map drew on the first try. Then the R4.3
  clock-drive immediately surfaced the real shape of the campaign loop — the live sim (`MoveAllSAGs`)
  is *systematically* built on deployed SAGs (an `if(as)` guard cleared one deref only to hit the next),
  so it's a deployment-subsystem dependency, not a bug to patch. Lesson reinforced: when a spike shows a
  story is a whole subsystem (campaign raid/SAG lifecycle, or the `CRToolBar` toolbars), bank the precise
  characterization and don't grind game-code guards that mask the cause. Kept well: every visible
  increment gated so the default path stays stable (the map renders with `BOB_MAP_TIMER` off); reverting
  the guard rather than committing a half-measure.

- _Sprint 12 (R6.1 GDI blit):_ **Spiking R4.2 first paid off by revealing the real dependency — a
  blit subsystem — which is itself an independently-valuable, cleanly-verifiable story.** Rather than
  grind the map subsystem blind, the Sprint-11 spike found that both the map *and* front-end icons gate
  on the same stubbed GDI bitmap path; building that (R6.1) is verifiable in isolation (decode the icon
  sheet → dump it → self-test blit to framebuffer) without needing the map wired. Lesson: when a big
  story (R4.2) blocks on a foundational gap, extract and ship the foundation as its own story — it
  de-risks the big one and delivers standalone value. Kept well: a gated self-test (`BOB_BLIT_TEST`)
  proves an infrastructure increment end-to-end even when its in-game consumer isn't wired yet.

- _Sprints 10–11 (R3 render tail → R4 campaign):_ **Real-GL access turned three "awaiting-pilot" /
  deferred items into shipped fixes in one session, and the campaign onion peeled cleanly the same way
  combat did.** R3.5 (trilinear) and R3.6 (addressing) were just *latent compat gaps* (missing mip chain;
  dropped sampler state) that a frame-dump A/B exposed immediately once a real display was available —
  the same unlock the Sprint-8/9 retros predicted. R4.1 confirmed the **onion pattern** holds for UI, not
  just memory bugs: each campaign screen revealed the next blocker (side-select had no text menu → polygon
  hit-areas; then campaignselect crashed on a NULL listbox column → persisted-column auto-create), each a
  small targeted fix reusing the game's *own* data (the real `SideSelectOutlines` polygons, the real
  `OnSelectRlistbox` nav). Lesson reinforced: **spike a big story before committing points** — R4.2 looked
  like "render the map" but the spike showed it's a *parallel UI subsystem* (map view + CMainFrame
  toolbars), so it was banked as a dedicated sprint instead of half-built. Kept well: every fix verified on
  real GL with a before/after capture; no game-logic edits (compat + boot scaffold only); bare `./bob` 0
  after each.

- _Sprints 5–7 + first-pilot (Release 2 → DoD → first human flight):_ **Shipping the whole loop fast
  validated the architecture, and the first human pilot found in one sortie what no headless test had:
  a gun-fire crash (combat exercises code paths the scaffolds never did) and render-fidelity gaps
  (clouds-over-cockpit, dark lighting) that frame dumps under-weight.** Lessons: (1) **a real player is the
  best fuzzer** — the gun-fire double-free was the R1.3d bug class on sibling classes, latent until *combat*
  ran; get a human (or a combat-exercising auto-test like `BOB_AUTOFLY=shoot`) on it early each release.
  (2) **Don't live-patch render fidelity** — the cloud-depth BOB_ZTEST attempts failed because the FBO-RTT
  terrain has no shared depth; visual bugs need a spike + Wine A/B, not real-time guessing (now R3.2/R3.8).
  (3) **Re-groom when scope expands** — the PO widening to "all functionality" (multiplayer in) is a backlog
  event; R3–R7 roadmap (§7a) is the response. Kept well: the env-var-free packaging + safe-default fallback;
  ASan as the combat-corruption oracle (7772→0 double-frees confirmed the fix).

- _Sprint 4:_ **The spike's "faithful path" call was right, and the trigger-agnostic bridge made the
  next increment cheap to reach.** 4.1 (force `LaunchScreen` + bridge to `Launch3d`) shipped real
  menu→flight fast; refactoring the bridge to fire on `Rtestsh1::THISTHIS` (not on *how* we got there)
  meant the 4.2 click-path spike was a tiny addition that immediately surfaced the true blocker (the
  CSQuick1 config-form combo crash) instead of hiding it. Lesson, reinforcing Sprint 1/2/3: **spike the
  click-driven path early** — it located the exact R2.1 seam (front-end config bring-up) in one run.
  Also: when porting a Win32 message flow with no compat dispatch, deliver the handler directly via the
  game's own *public* method (here `Rtestsh1::Launch3d`, what `OnGetString` calls) — faithful, minus the
  dead message hop. Watch-item: `INT3` range-guards don't halt on compat, so game-code "can't happen"
  asserts fall through to the UB they were guarding — a recurring trap for the config grind ahead.

- _Sprint 3:_ **Splitting R1.3d out (Sprint 2 decision) paid off — focused, it cracked in one session.** The
  multi-session "uncrackable" transient double-free fell quickly once given (a) `-g` line numbers on the ASan
  build and (b) undivided focus instead of being one item in a crowded story. Lesson: when a bug resists,
  the unblock is often *better tooling + isolation*, not more effort — and that's worth a deliberate
  re-scope. Kept well: invest in tooling early (the `-g` addition turned an inlined `??:?` stack into the
  exact `operator delete` line). Estimation note: R1.3d came in at ~8 as estimated; the corruption onion
  bottomed out at four bugs total — the Sprint-1 spike lesson would have sized R1.3 closer to 15 up front.
- _Sprint 2:_ **A "fix the corruption" story hid layers; fixing one peeled back the next.** R1.3 looked
  like one bug but was three (setup mismatch, OOB write, trilinear) plus a deeper combat-loop double-free
  that only surfaced *once the setup fixes let combat run*. Lesson: corruption stories are onion-shaped —
  estimate them as spikes, not fixed-size fixes, and expect each fix to reveal the next under the tool.
  Kept well: ASan-verify each fix immediately (caught that R1.3b's write was fixed but a read remained);
  A/B with/without the trigger to classify bugs (default-vs-InitPreferences split told us which bug
  actually gates the goal). Process win: surfacing the scope fork to the PO instead of silently grinding
  an uncertain multi-session bug — banked verified value and kept the default path stable.
- _Sprint 1:_ **Estimate the work before committing the points.** R1.1 ("unify the window", 8 pts) was
  mis-sized in two directions at once — the infra was *already done* (over-estimated) while the real
  remaining work (control-flow merge) is bigger *and* blocked-by a later story (R1.3). A 1-pt spike to read
  the seam before Sprint Planning would have caught both. Action: add a Definition-of-Ready "spike unknowns
  first" check for architecture-touching stories. Upside: ASan-as-tool (when valgrind is absent) was a clean
  unblock for the critical-path story — keep the "verify the tool exists before committing the approach" habit.

---

## 11. Risk Register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Heap corruption (R1.2/R1.3) is a deep `-fpack-struct` grind | High | Blocks Release 1 | Memory tooling first; A/B `-fpack-struct` per-TU; class-match prior overrun fixes. |
| Real mission loop unearths a long tail of uninit-state bugs | High | Slows Release 2 | Budgeted as R2.3 (13 pts); each bug a logged sub-task, same pattern as `SetUnits`/volume fixes. |
| valgrind can't run under real GL | Medium | Slows R1.2 | `SDL_VIDEODRIVER=dummy` / software GL fallback (already noted in STATUS.md). |
| Game-code edit pressure when a fix "wants" to live in game logic | Medium | Violates core constraint | Hard rule: solve in `SRC/compat/` or boot scaffold; cap the compat layer instead. |
| No joystick hardware to verify R3.1 | Medium | Can't accept R3.1 | Implement against `SDL_NumJoysticks`; mark accepted-on-hardware. |
| MIDI music environment blocker | Certain | Music absent | Iceboxed; excluded from DoD; revisit only after environment change. |
```
