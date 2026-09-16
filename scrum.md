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
> ⚠️ **ID COLLISION — read this before quoting an R3.x number.** The four PO defects filed
> 2026-08-28 (`R3.6` icon art, `R3.7` CloseLoggedChild crash, `R3.8` no aircraft list, `R3.9` grey
> square) reuse IDs that Release 3 already had for render work (`R3.6` terrain combiner, `R3.7`
> in-flight effects, `R3.8` render sweep, `R3.9` ground-impact crash). Both sets are live in this
> file. Filing them on top of existing numbers was a mistake made when they were entered; they are
> NOT renumbered here because those IDs have already been reported to the PO under these names, and
> silently changing an identifier the PO is using is worse than the collision. **Disambiguate by
> date: the 2026-08-28 rows are the PO defects.**

| R3.6 | **PO 2026-08-28: two CAMPAIGN icons are drawn where the "X" (exit) icon belongs, upper-right** — video `/home/admin/Videos/260828_bob.mp4`. Two icons that belong at the BOTTOM of the screen appear at the top right, in the system box. ⭐ **The PLACEMENT and the HIT-TEST are both correct — it is the ART that is wrong.** Measured from the same session's log: `[sysbox] panel 34x50 DLU -> 51x81 px at (965,8): 3 controls drawn` (screen 1024 wide ⇒ `1024-51-8 = 965`, i.e. correctly flush upper-right), and `[sysbox] click (996,41) consumed` — so the exit control **is** there and **does** take the click. What is painted on it is not its own icon. ✅ **PO CONFIRMED: clicking there DOES execute the "X" function.** So the control, its position and its behaviour are all correct — **only the icon art is wrong**. That narrows this to art assignment alone. ⛔ **THE ART-GUARD SUSPECT IS ELIMINATED (2026-08-28, measured).** This entry pointed at
`DIALCLASS::OnGetFile`'s widened guard (`filenum>0x6600 && filenum<0x7200`, original `<0x7100`
still commented directly above it) on the MA S315 precedent. It is not the path:
`OnGetFile` is **never called at all**. Instrumented BOTH branches (`BOB_TRACE_ART`) and ran a full
German campaign in which `[sysbox] panel 34x50 DLU -> 51x81 px at (965,8): 3 controls drawn` — the
sysbox demonstrably painted — with **0 `[art]` and 0 `[icon]` calls**.
Two traps worth keeping:
* the first instrument covered only the `else` branch (the guard), and icons return EARLIER via
  `if (filenum>=0x10000) { IconsUI(filenum); return; }`. **Tracing the branch you did not take
  proves nothing about the branch you did.** Both are traced now.
* `OnGetFile`'s only references are `ON_MESSAGE(WM_GETFILE, OnGetFile)` — and `ON_MESSAGE` is a
  **no-op macro** in this compat layer, so the handler is unreachable. Same class as MA PO-67,
  where `RMdlDlg` never inherited `OnRowanMessage` and `WM_GETFILE`/`WM_GETGLOBALFONT` silently
  went nowhere. **Narrowing the guard would have changed nothing and "verified" whatever the next
  screenshot happened to show.**

⛔ **THE LAYOUT IS CORRECT TOO (2026-08-29, measured).** `BOB_TRACE_SYSBOX` dumps what dialog 823
(`IDDT_SYSTEM`) actually hosts:

```
[sysbox-ctl] dlgId=823 ctrlId=1003 visible=1 dlu=(0, 0,34,30)   IDC_FILES     -- the "X", TOP
[sysbox-ctl] dlgId=823 ctrlId=1001 visible=1 dlu=(0,30,17,20)   IDC_THUMBNAIL -- bottom-left
[sysbox-ctl] dlgId=823 ctrlId=1002 visible=1 dlu=(16,30,18,20)  IDC_ZOOMIN    -- bottom-right
```

So: the right three controls, the right ids, `IDC_FILES` on top at full width exactly as this entry
describes, correct panel origin (965,8), and the PO has confirmed the click behaves. **Nothing about
identity, geometry or hit-testing is wrong.**

⭐ **REMAINING HYPOTHESIS — VISIBILITY, not art and not layout.** All three report `visible=1`. The
PO's words are *"two campaign icons that should be at BOTTOM of screen are at upper right, where the
X icon should be displayed"* — which fits a sysbox correctly showing its X **with THUMBNAIL and
ZOOMIN also drawn beneath it**, when the real game shows those two elsewhere (or not at all) in this
context. `host->visible` honours the game's runtime `ShowWindow` (SP.2/S123), so the question is
whether gold ever calls `ShowWindow(SW_HIDE)` on 1001/1002 here and this port misses it.

**Next step is a GOLD COMPARISON, not more code reading:** capture the same screen under Wine and
count the icons in that 51x81 px box at (965,8). If gold shows one, this is a missed hide; if gold
shows three, the PO is describing the ART on them and the search moves to `host->draw()`'s
`SetDIBitsToDevice` source per control.

_Sprint limit reached (4). Two suspects eliminated by measurement; do not start from the art guard
or the layout again._

**Start at the icon/art assignment, not the layout:** `IDDT_SYSTEM` is 34x50 DLU with `IDC_FILES` (exit) on top and `THUMBNAIL`/`ZOOMIN` beneath, drawn as a whole panel by `bob_ole_draw_panel` (`MAINFRM.CPP:1401`). ⭐ **Sister-port precedent — MA S315:** widening `CRToolBar::OnGetFile`'s art guard from `0x6800..0x7100` to `0..0xFFFF` admitted two art files the real game never draws (`0x6607`, `0x660c`), putting the WRONG icons on toolbar buttons. Check bob's equivalent art guard / `FileNum` resolution for the same over-admission before touching any geometry. | 5 | ☐ |
| R3.7 | ☑ **FIXED & GATED (2026-08-28, commit `3c0a9cf`).** Root cause was **not** the use-after-free first supposed: `MAINFRM.CPP:1795` passed `root` (an `RDialog*`) to `CloseLoggedChild(**int**)` — the slot INDEX. It compiles silently because the build carries `-fpermissive` *and* `-w`. On i386 the pointer is truncated and used as a subscript: `root=0xb2efe10` = 187,727,376 → the read lands ~750 MB past the array, giving the log's `fault_addr=0x3bf95c60`. It also explains why the S108 re-entrancy guard never fired — `i >= 0 && i < 256` is false for a truncated pointer. Fixed by passing `i`. Gate `tools/bob_dialslots.sh`: PASS arm green on all five assertions; `CONTROL=1` (`BOB_R37_REVERT=1`) reproduces the SIGSEGV on the same path. Two earlier controls poisoned slot *contents* and came up green — they tested a wrong theory and never reached the line; the gate now asserts the close path was REACHED. Follow-up: **R3.10**. <br>_Original report:_ 🔴 **CRASH: accepting an "Intercept Offered" dialog kills the game (PO 2026-08-28)** — the PO ran the German campaign, then the British campaign, and the window disappeared. **Fully resolved from the session log — this is a real, reproducible backtrace, not a mystery:** <br>`[titleglyph] (655,57) -> IDOK (accept) on dialog 0xb2efe10 (16InterceptOffered)` <br>`[dlgclose] toolbar 1 child 12 closed (root=0xc525d10, asked=0xb2efe10)` <br>`=== CRASH: signal 11 fault_addr=0x3bf95c60 ===` <br>`CRToolBar::CloseLoggedChild(int)` (`rdiallog.cpp:434`) ← `bob_close_logged_dialog` (`MainFrm.cpp:1796`) ← `bob_oob_click_title_glyph` (`:1836`) ← `bob_map_click_oob` (`:1473`) ← `bob_frontend_tick` ← `OnIdle`. Registers: `eax=0c525d10` (root), `ebx=edi=0b2efe10` (the dialog asked to close), `esi=0x0c` (**child 12** — the one the line above says was just closed). ⭐ **The child is closed and then still walked** — a use-after-free in the sibling walk during removal. `CloseLoggedChild` has **previous form**: `CLAUDE.md` records a *"CloseLoggedChild recursion"* as one of three real game-code bugs already fixed there. Same function, new failure. **Repro: campaign map → accept an Intercept Offered dialog via its title glyph.** | 8 | ☐ |
| R3.8 | **Pre-3D-entry campaign screen shows NO aircraft list (PO 2026-08-28)** — screenshot `Screenshot From 2026-08-28 21-39-10.png`. Before entering 3D the screen should list the aircraft you can fly, or join as gunner; it shows only the background art and the `Back / Sim Config / Fly` menu row. **Compare against the gold standard** before assuming the list is missing rather than mis-drawn — the whole panel may be rendering with an empty hosted listbox, which is a different fix from the list never being populated. ⛔ **`bob_ole_count_hosted()` DOES NOT EXIST** — this entry named a non-existent instrument when it
was filed (2026-08-28). Do not go looking for it. The working equivalent is `BOB_TRACE_SYSBOX`
(added for R3.6, `SRC/RLISTBOX/bob_ole.cpp`), which prints one line per hosted control per dialog —
`dlgId`, `ctrlId`, `visible`, and its DLU rect — once each, rather than the per-frame firehose of
`BOB_TRACE_OLE`. Run the campaign recipe through to the briefing and read off what the pre-3D
dialog hosts: **zero rows means the list was never populated; rows present with a drawn rect means
it is populated and mis-drawn.** Those are different fixes.

⛔ **THE "ZERO CONTROLS" READING BELOW WAS AN INSTRUMENT FAULT — RETRACTED (2026-08-29).**
`[panel] dialog=0x986e300 dlgId=? registry=59 matched=22`: the briefing pane hosts **22** controls,
not zero. The per-control trace deduped on `ctrlId` alone into a 64-entry table and **stopped
recording once full** — `IDD_LWDIRECTIVES` contributes 49 ids by itself, so every dialog reached
later printed nothing and the briefing looked empty. A trace that goes quiet when it runs out of
room reports "absent" for "I stopped looking". Fixed: key on `(dlgId,ctrlId)`, 1024 entries, and it
now prints `TABLE FULL` rather than falling silent.

**The open question is therefore back to the original one** — is `IDC_RLIST_UNITDETAILS` among those
22 and drawn empty (population failure, `maxsquadoption == 0`), or absent from them (creation
failure)? `BOBFRAG.CPP:225-255` is the filler: four columns (Unit/Aircraft/Duty/Callsign) then one
row per `squadinfo.currfrag->squadoptions[i]`, `i < maxsquadoption`.

⭐ **ANSWERED (2026-08-29): THE LIST IS FULLY POPULATED WITH CORRECT DATA. THE DEFECT IS IN
PAINTING THE ROWS.** Every link in the chain measured good:

| link | evidence |
|---|---|
| created & hosted | `[sysbox-ctl] dlgId=1164 ctrlId=1481` (`IDC_RLIST_UNITDETAILS` on `IDD_BOBFRAG`) |
| visible | `visible=1` |
| drawn with a real rect | `dlu=(5,42,449,86)` |
| fill loop runs | `[fraglist] maxsquadoption=7 currsquadoption=0 side=1` |
| rows carry real data | `[fragrow] 0: unit='S1/III (7)' ac='Ju87' duty='Dive Bomb' call=' Checkerboard III'` … `4: unit='J3/I (1)' ac='Me109' duty='High' call=' Panther I'` |
| `CString::LoadString` | properly backed by the PE resource loader (`cstring_impl.cpp:571`) |

Seven squadron options with correct units, aircraft types, duties and callsigns are added to a
visible, drawn listbox — and the PO sees an empty panel. **So look at how the RListBox host PAINTS
its rows**, not at the campaign data:
* `bob_ole_draw_panel` clips each control to its own rect before `host->draw()` — check the DLU→px
  conversion for a 449-wide control on this pane (a wrong `pxPer100` would clip the rows away);
* row text colour vs the panel art behind it;
* whether the host's row renderer draws the header row only.

⚠️ **NOT the same defect as R3.6.** That one has correct data and correct geometry with VISIBILITY
as the surviving suspect; this one has correct data and a drawn control with ROW PAINTING as the
surviving suspect. Two "missing UI content" reports, two different mechanisms — do not fix one and
assume the other follows.

_Two superseded readings, kept so the mistakes are not repeated:_
_(1) "the briefing hosts zero controls" — a truncated trace, retracted above._
_(2) the conclusion that followed from it:_
⭐ ~~**MEASURED (2026-08-29): NEVER POPULATED, not mis-drawn.**~~ — wrong twice over: it was right
about "not mis-drawn" only by luck, and it is now measured to be exactly backwards. German campaign driven to the briefing
and STOPPED there (no `BOB_CAMPFLY_GO`, so the screen is actually up — the first attempt flew
straight through to 3D and its empty dump proved nothing).

`[campfly] LaunchFullPane(bobfrag, UIR_FRAG) hipack=6 hisquad=0` then
`[fullpane] ENTER page=1 startscreen=0x8453be0 reqres=3` — the briefing pane enters, and hosts
**ZERO** OCX controls. Every dialog that hosts anything in that run is a campaign-SELECTION screen:

| dlgId | name | hosted controls |
|---|---|---|
| 1032 | `IDD_LWDIRECTIVES` | 49 |
| 1191 | `IDD_CAMPNAME` | 5 |
| 823 | `IDDT_SYSTEM` | 3 |
| 289 | `IDD_SCAMPAIGNSELECT` | 3 |
| 1040 | `IDD_SIDESELECT` | 3 |
| 1043 | `IDD_PHASEDESCRIPTION` | 1 |

The fullpane path is not a separate draw route — `FULLPSYS.CPP:540,571` call `bob_ole_draw_panel`,
the same function the trace sits in — so a hosted aircraft list WOULD have appeared. It does not
exist to be drawn.

**So the fix is population/creation, not layout or art.** Next: find what builds the flyable-aircraft
list for `UIR_FRAG` and why it produces nothing — `hisquad=0` in the same line is a suspicious
neighbour, given R4.3 already had to set `MMC.playersquadron` at Fly time because a just-scrambled
interceptor falls outside the briefing's flyable-status gate. | 5 | ☐ |
| R3.9 | **Floating light/dark grey square during campaign dogfight (PO 2026-08-28)** — screenshots `Screenshot From 2026-08-28 21-40-46.png` and `21-44-32.png`. An untextured grey quad appears intermittently in the 3D view. ⚠️ **Likely the same family as the cockpit/mirror RTT work**: an untextured or unbound quad reads as flat grey. Suspects, in order — a render-target surface drawn as geometry (`BOB_DUMP_RTT` dumps each RTT FBO), the `InfiniteStrip` horizon backdrop (known garbage `v` texcoords, R3.4), and a sprite whose texture failed to bind. `BOB_TRACE_RTT` / `BOB_CHECK_SURF` are the existing instruments. <br>◐ **CANARY ADDED (2026-08-29): `BOB_TRACE_GREY`.** A flat grey square IS an untextured quad, and the draw path has TWO ways to produce one, needing different fixes: `t == NULL` → texturing disabled, quad draws in vertex colour; `t && !t->glTex` → texturing ENABLED bound to texture 0 because `upload_texture` produced nothing, so GL samples white (the quiet one). Both are now reported, in `draw_fvf` AND `DEV_DrawIndexedPrimitiveVB`, keyed PER SURFACE with a `TABLE FULL` notice — not per call (a per-call trace in this codebase once wrote 24.7 M lines and starved the run) and never silent when full (that exact silence caused an R3.8 finding to be retracted). <br>⚠️ **First run: 0 untextured 3D draws — and that is NOT evidence of absence.** It was a QUICK-MISSION flight (`BOB_BOOT_FRONTEND=1`, dump at frame 400); the PO saw the square **during a CAMPAIGN dogfight** and says it appears *"sometimes"*. Wrong scenario and a short window. The existing `[texfail] summary: 0 uploads bailed` agrees, which is consistent but equally scoped. **Next: drive the campaign to 3D (the R4.3 loop) with `BOB_TRACE_GREY=1` over a long combat soak, and only then treat silence as meaningful.** <br>◐ **RUN 2 (2026-08-29): campaign → 3D on REAL GL, full 10 minutes, `InThe3D=1` — still 0 untextured draws.** Note `tools/bob_combat_soak.sh` could NOT have been reused for this: it runs `SDL_VIDEODRIVER=dummy`, so `draw_fvf` never executes and the canary cannot fire there at all. <br>⚠️ **But run 2's silence is also only partial evidence, because the canary filtered `!is2D`.** That assumed a square floating in the 3D view must be 3D geometry — it need not be: an RTT blit or an overlay quad carries `BFVF_XYZRHW` and would have been excluded from the very trace written to find it. **Widened to include 2D-flagged draws** (still keyed per surface, so it cannot flood; `is2D` is now reported per hit). Re-run before drawing any conclusion. <br>**If it stays silent with 2D included**, the square is not an untextured quad at all and the search moves to the other two suspects in this entry — an RTT surface drawn as geometry (`BOB_DUMP_RTT`) and the `InfiniteStrip` horizon (R3.4's garbage `v` texcoords). <br>⭐ **RUN 3 (2026-08-29) — A HIT, AND ONLY BECAUSE THE `!is2D` FILTER WAS DROPPED:** <br>`[grey] draw_fvf NO texture (texturing disabled)  surf=(nil) prim=6 count=4 fvf=3c4 is2D=1` <br>Exactly one untextured draw in a 10-minute real-GL campaign flight, and it is **a 4-vertex triangle fan in SCREEN SPACE — i.e. a quad**, drawn with texturing disabled. `fvf=0x3c4` decodes as `XYZRHW|DIFFUSE|SPECULAR|TEX2`: **the vertex format declares TWO texture coordinate sets and nothing is bound**, so it paints flat. A screen-space untextured quad is precisely "a floating light/dark grey square". <br>⚠️ The earlier `!is2D` filter would have hidden this permanently — the assumption "a square floating in the 3D view must be 3D geometry" was wrong, and two runs reported a clean 0 under it. <br>**Next: a one-shot `backtrace()` at that site (added) names the caller.** The fix depends on which it is — a draw that should have bound a texture and did not, versus a draw that should not be happening at all. Do NOT simply skip untextured quads: that would hide a legitimately-untextured overlay if one exists. <br>⛔ **RUN 4 — THE HIT IS A FALSE LEAD, AND THE CANARY WAS HIDING THE REST.** `addr2line` on the backtrace: <br>`draw_fvf ← DEV_DrawPrimitiveVB ← Lib3D::RenderPlainPolyList (LIB3D.CPP:15022) ← Lib3D::EndScene ← COverlay::LoaderScreen (OVERLAY.CPP:2924) ← View3d::MakePassive ← Launch3d ← bob_frontend_tick` <br>It is the **LOADING SCREEN**, drawn through `RenderPlainPolyList` — *plain* = untextured **by design**, and it happens at load, not in a dogfight. Exactly the "legitimately-untextured overlay" this entry warned about one line above. <br>⚠️ **AND IT WAS MASKING EVERYTHING ELSE.** The canary deduped on the surface pointer, which is `(nil)` for EVERY texturing-disabled draw — so all of them collapsed to a single entry and only the FIRST was ever printed. The loader screen took that slot at startup and silently swallowed every later untextured draw, including whatever the PO sees. Two runs' "1 hit" actually meant "one of an unknown number". Same failure family as the R3.8 trace that stopped recording once its table filled. **Fixed: keyed on the DRAW (`fvf`/`prim`/`count`), 128 entries, `TABLE FULL` when exhausted.** Re-run before drawing any conclusion. <br>◐ **RUN 5, with the keying repaired: still exactly ONE distinct untextured signature — the loader screen's (`fvf=3c4 prim=6 count=4 is2D=1`). So in this scenario the grey square is NOT an untextured `draw_fvf` quad.** <br>**SPRINT LIMIT (4) REACHED. State of the three suspects:** <br>1. ~~untextured quad in the FVF draw path~~ — **eliminated for this scenario** by a sound instrument (both draw entry points, 2D and 3D, keyed per draw, table-full announced). <br>2. **RTT surface drawn as geometry** — untested. `BOB_DUMP_RTT` dumps each FBO. <br>3. **`InfiniteStrip` horizon** (R3.4's garbage `v` texcoords) — untested. <br>⚠️ **ONE SCOPE CAVEAT THAT MUST NOT BE FORGOTTEN:** the run drives the campaign into 3D and soaks for 10 minutes, but it is NOT established that actual COMBAT occurred, and the PO's words are *"during campaign dogfight… can SOMETIMES be seen"*. Elimination (1) is therefore sound for "campaign flight" and not proven for "campaign dogfight". Before spending a sprint on suspects 2 or 3, get a run where combat demonstrably happens (`BOB_TRACE_ACM` shows the ACM tree engaging) and re-run the canary there — it is cheap and it is the difference between eliminating a suspect and eliminating a scenario. | 5 | ☐ |
| R3.4 | **Rear-view mirror horizon UVs** — ☑ **CLOSED 2026-09-15 (S5), against the gold.** The filed cause (`InfiniteStrip` garbage v-texcoords) was wrong; the defect was MIRROR-1's 5:2 projection aspect against a square FBO, fixed 2026-09-13. Cockpit confirmation at last: `~/gold standard/bob/bob_convoy_campaign.mp4` (real `bob.exe` under Wine, same Luftwaffe Convoys campaign) shows a live mirror — sky, horizon, tan ground, ~1100 distinct colours, sd 33-42 — and the port now measures sd ~35 with ~1000 distinct against its old flat 19. `doc/reference/mirror-gold-vs-port-2026-09-15.png`. The gold mirror is a **circle** (50x50, w/h 1.000), which also settles MIRROR-1's open question: aspect 1.0 is right, not merely workable. | 5 | ☑ |
| ASPECT-1 | 🔴 **The 3D view is stretched 1.333x horizontally on a widescreen display (found 2026-09-15 by R3.4 S5; affects every flight the PO makes).** The Bf 109's round rear-view mirror measures **50x50 (w/h 1.000)** in the gold at 800x600 and **192x144 (w/h 1.333)** in the port at 1920x1080, stable in every frame of both. 1.3333 = (16/9)/(4/3): the port builds a **4:3** projection and rasterises it into a 16:9 window, so every circle in the game — reticle, dials, sun, mirror — is 33% too wide and the horizontal FoV is wrong to match. Machinery: `DOSDefs.H` `FULLW 25600`/`FULLH 19200` (a hardcoded 4:3 virtual space), `HARDWIN.CPP:253` integer `virtualXscale`, `WIN3D.CPP:3433` `aspectRatio=FoV*(window_height/window_width)`. The integer division alone predicts only 3.7% at 1920x1080, so the prime suspect is the projection not being rebuilt after `ChangeDisplaySettings 1024x768 -> 1920x1080`. **S1: trace `window_width/window_height/aspectRatio/FoV` at `Set3dWindow` and at the 3D projection setup, once per mode change, and A/B against `BOB_FORCE_MODE=1440x1080` (4:3) where the disc must measure 1.000.** Do not change the aspect before that trace says which mechanism is in force. | 5 | ☐ |
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
| R4.2 | **Strategic map screen** — ◐ **TERRAIN + UNIT ICONS RENDER (2026-06-25, S34).** The strategic map now draws the full **unit-icon layer** (green RAF squadron/airfield, blue fighter, yellow LW raid markers over SE England, matching the Wine gold ref). Root cause (measured via `BOB_TRACE_ICONS`, correcting the S27 attribution): `UpdateBitmaps` calls `DrawIcons(pDC,inter)` **per terrain block** with `inter=block∩bounds` (a Windows paint-region optimization); the headless paint has no per-region paint rect so every `inter`→`(0,0,0,0)`, and the world-rect cull lands ~2.16M units off every item (`scan=1238 cull_pass=0 drawn=0`). Fix (1 line, `#if BOB_LINUX`): draw icons **once over the full client bounds** (the game's own pre-optimization call, still DEADCODE) → `cull_pass=768 drawn=99`. Icons need the campaign sim populated (zoom 2.0 < `ZOOMTHRESHOLDDETAIL` → dynamic units), so it rides R4.3/R4.5. Bare `./bob` 0 (map-only, behind `g_bob_map_active`). Remaining: CMainFrame toolbars/bars + scroll/zoom + click (scramble/intercept). _(earlier: terrain renders 2026-06-21; icon spike root-caused 2026-06-24.)_ | 13 | ◐ |
| R4.3 | **Campaign mission flow** — ◐ **CORE LOOP CLOSED (2026-06-24).** The full campaign mission cycle runs in one process, no crash: strategic map → **intercept a raid** (`BOB_CAMPAIGN_FLY` → `NewPackage` scrambles a real interceptor) → **briefing** (`bobfrag`) → **Fly** → **flight** (cockpit on real GL; reuses the QM `StartFlying`/Launch3d bridge) → **mission end** (`BOB_AUTOQUIT=debrief` → EXITKEY → `OnFlyingClosed(IDOK)`) → **back to the strategic map** (`OnFlyingClosed` routes by gamestate=COMMANDER → campaign branch `NextMission`+`StartUpMapWorld`+`LaunchMap`). One fix earlier: set `MMC.playersquadron` at Fly time (a just-scrambled interceptor is outside the briefing's flyable-status gate). One deferral: headlessly fast-forwarding the *post-mission* sim SIGSEGVs in `GetCruiseAt`/`Plane_Type_Translate[bad ptype]` (post-mission SAG `type` uninit — **R4.5 grind**); guarded off (`g_campfly_flown`) so the returned map is stable (139→124), faithful to real play (returned map is paused). Remaining: post-mission SAG-uninit (R4.5) for day-advance + next mission; in-cockpit Continue/Quit dialog; briefing widget population (R6.3). _(earlier same day: a mission flies; briefing reached; interception scaffold.)_ | 13 | ◐ |
| R4.4 | **Campaign save/load** — ☑ **DONE — FULLY CLICK-DRIVEN (2026-06-24).** Save a campaign and load it back entirely through the real UI: **save persists (S28) → load restores state (S29) → load screen lists saves (S30) → load enters the map (S31) → file-row click selects a save (S32)**. Last piece (S32): the OCX file-row-click eventsink is a no-op on Linux, so a targeted `BOB_LINUX` bridge (mirrors R5.3b SController) — `CLoad::bob_file_clicked`→`OnSelectRlistboxfile`, `g_bobCLoad` registry, `OleHost::rowAtY`→`HostRListBox::GetRowFromY`, routed by `bob_ole_click`. Verified by genuine clicks: click "Auto Save" → `OnSelectRlistboxfile filename='Auto Save.bsr'` → click "Load" → `LoadGame=1 → LaunchMap → strategic map active`, no crash. Earlier-session ASan fix (deserialise terminator) + 3 `fakefile`-path twins (Save/Load/CLoad) + `DoLoadGame`-is-clean finding. No regression. _(2nd targeted OCX bridge → MA general eventsink worth adopting next.)_ | 8 | ☑ | Save (S28) + **load (S29)**: `CFiling::LoadGame -> OK, currtime 26660->32180` (loaded state = saved state). The deserialise crash was ASan-pinned to `MIGView.cpp:2210` — the `raidnumentries[r]` terminator loop overrunning the array because **loaded packages lack the terminator** `RecostRaidList` sets at runtime (R4.5 family; **NOT** ConvertPtrUID — its null-handling is already safe via address-cast operators, hypothesis disproven). Fixed: re-run `RecostRaidList()` on loaded packages in `PackageList::LoadGame` (MAPCODE.CPP) + `if(ac)` guard the 3 unguarded `ac->SetDraw()` in SetVisibilityFlags (world not rebuilt at deserialise-time). ASan-verified the overflow is gone. **S30: lists the save** (`CLoad::MakeFileList` path fix, 3rd twin). **S31: load → campaign map** — the loadgame "Load" item is `DoLoadGame` = `LoadGame`(S29) + `LaunchMap`, NOT the R4.2-blocked `CFiling::OnOK`; so loading enters the strategic map on the restored state (`[loadgo] LoadGame=1 currtime=32180 → LaunchMap done`; `doc/reference/loadgame-into-map-2026-06-24.png`). **Save→load→list→enter-map all work.** **Only remaining R4.4 UI:** the OCX **file-row-click** → `OnSelectRlistboxfile` (RListBox-host row-click via `GetRowFromY` + targeted CLoad bridge / MA eventsink) so the user clicks the save instead of the `BOB_LOAD_GO` scaffold setting `selectedfile`. _(S28 save persists, S29 load restores state.)_ | 8 | ◐ | With the campaign running (R4.3), **a campaign save now lands on disk** — `SAVEGAME/Auto Save.BSR`, 225KB of real `bos<<Miss_Man`. Root cause of "no saves ever": `CFiling::SaveGame` used the corrupted `fakefile` savegame path that `LoadGame` was already `#if BOB_LINUX`-bypassed for, but `SaveGame` wasn't — fixed (mirror the bypass: write `savegame/<fname>`). `BOB_CAMPAIGN_SAVE` scaffold triggers it. **Load = two scoped gaps (deferred):** (1) `CLoad` file-list enumerates `FIL_SAVEGAMEDIR` via the same `fakefile` path → list empty (adopt MA's CLoad render+click fix); (2) `LoadGame`→`bis>>Miss_Man`→`PackageList::SetVisibilityFlags` SIGSEGVs at `*ConvertPtrUID(uid)` (deref-before-NULL-check; world not rebuilt at deserialise-time — **R4.5 family**, gdb-pinned). No regression (scaffolds gated). | 8 | ◐ |
| R4.5 | **Campaign uninit-state grind** — ◐ **POST-LOAD SIM ADVANCES (2026-06-25, S37).** The load-boundary reference-audit (PO-chosen) landed one systemic fix: `ConvertPtrUID` (`PERSONS2.CPP`) now honors its own `assert(tmpUID in [1,IllegalSepID])` on Linux — returns the same null-ref it already gives for UID==0 when the UID is out of range (the R1.3b/4.3c compat-non-halting-assert class), retiring the **whole garbage-UID fatal family** (S36 `CountFormationSize`, S36-exposed `SquadTarget`, …) in one place. **Post-load campaign sim now advances**: loaded save (currtime 32180) fast-forwards to 62540+, 1000+ cycles, raids processing (`worlditems 1238→1218`), no crash. Transparent for valid UIDs (fires only on garbage → zero normal-play change); bare `./bob` 0; fresh-campaign sim unregressed. Caveat (banked): garbage UIDs resolve to NULL (a stale loaded raid may not perfectly re-acquire its target — minor fidelity), the per-reference deserialise restoration (`FixupAircraft`/`SetTargetUIDs` gaps) is the faithful follow-up. Remaining: post-*mission* `GetCruiseAt`/`Plane_Type_Translate` path; toolbars/interaction. _(S36 formation-pointer fix; S35 `BOB_POSTLOAD_FF` repro + root-cause; campaign sim runs 2026-06-22.)_ | 8 | ◐ |

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
| R6.2 | **Font / DPI fidelity** — ◐ **multi-line text fixed (2026-06-21).** The huge-overlapping-text bug was `bob_ole_draw_panel` setting the font height = the control's BOX height — right for single-line labels/combos, but a tall multi-line text control (campaign PhaseDescription `CRStatic`) drew its font at the full box height. Fixed: cap the font at the single-line (16-DLU) height for multi-line boxes; single-line controls unchanged. campaignselect description now readable; config screens unregressed. **☑ multi-line word-wrap DONE (S127, 2026-08-02):** `CDC::DrawText` now implements real DT_WORDBREAK — the phase-select/QS descriptions wrap within their boxes (paragraph breaks preserved), ≥2-line-box guard keeps single-line labels intact; plus '&' accelerator escape ("Cockpit && UI"→"Cockpit & UI", #8). `BOB_NO_WORDWRAP`/`BOB_NO_AMP_ESCAPE` revert. Remaining: native-DLU base-font face/size pass (gold's large tab/heading faces). | 5 | ◐ |
| R6.3 | **Remaining UI screens** — ◐ **OPTIONS TABS + DEBRIEF VERIFIED (2026-06-23).** All six options config tabs render as readable forms (GFX, More GFX, Controls, Sound, 2D, Sim — via `BOB_CONFIGSCREEN`), and the mission **debrief/readyroom** renders correct text ("54 Squadron", "Eagles Attack", "11:00") — both unblocked by the game-wide CString-varargs fix (R5.3). Remaining: loadout, diary, other debrief variants. | 8 | ◐ |
| R6.4 | **Replay** — record + playback through the menu (the debrief Replay option). | 8 | ☐ |
| R6.5 | **Save/load round-trip** — all game state (settings/campaign/replay) binary-compatible. | 5 | ☐ |
| R6.6 | **Intro Smacker / cutscenes** — Smacker video → libsmacker. | 5 | ☐ |

### Release 7 — "Multiplayer" (the long tail — now in scope)
| ID | Story | Pts | Status |
|---|---|---|---|
| R7.1 | ◐ **DirectPlay → sockets** — session create/join, lobby/readyroom, state sync. **TWO GATES PASS (2026-08-28):** `tools/bob_mp_connect.sh` drives Multi-Player → DirectPlay → lobby through the real menus (with a `BOB_NO_DPLAY=1` control that must stay on the main menu), and `tools/bob_mp_packet.sh` has two processes discover a session and exchange a packet on port 47624 (with a no-host control that must find 0 sessions). ⚠️ Both gates were labelled **R6.1/R6.2** in their own banners — pointing at "Front-end blit" and "Font / DPI fidelity" — and are relabelled R7.1 here. A gate that announces the wrong story ID sends the next reader to the wrong backlog row. <br>◐ **THE UI JOIN PATH IS PLUMBED END TO END — IT NEEDS TESTING, NOT BUILDING (2026-08-29).** Traced: `RFullPanelDial::GetSessions` (`FULLPANE.CPP:1243`) → `_DPlay.UIGetSessionListUpdate()` (`COMMS.CPP`) → `lpDP4->EnumSessions(&desc, 0, EnumSessionsCallback, NULL, DPENUMSESSIONS_AVAILABLE\|DPENUMSESSIONS_ASYNC)`. The compat `EnumSessions` (`bob_dplay.cpp:227`) **does invoke that callback** — it broadcasts `MSG_PROBE`, collects `MSG_OFFER` replies, builds a `DPSESSIONDESC2` with the host's session name and calls `cb(&sd,&tmo,0,ctx)` per session. So a client SHOULD be able to see a hosted session in the real UI list. **Nobody has run it.** <br>⭐ **DONE — `tools/bob_mp_uijoin.sh`, and the JOIN LIST IS POPULATED (2026-08-29):** <br>`the game enumerated sessions at all  yes` / `sessions the JOIN list would show  1` / `PASS`. <br>A `dplay_probe host` runs, the game is driven through Multi-Player → Join Game, and its OWN list (`_DPlay.SessionList`, via `GetSessions` → `UIGetSessionListUpdate` → `EnumSessions` → `EnumSessionsCallback`) comes back holding the hosted session. `BOB_TRACE_SESSIONS` (added in `COMMS.CPP`) prints the names and count. <br>⭐ **THE LOBBY PAINTS A SESSION-LIST CONTROL THAT NOTHING FILLS UNTIL "JOIN GAME" IS CHOSEN.** Measured with `BOB_TRACE_SYSBOX`: the lobby (artnum 27920) is `dlgId=923` with `ctrlId=1692` (header, dlu 0,0,390x16) and `ctrlId=2132` (the list, dlu 0,20,391x142) — and reaching that screen produces **zero** session traces. The enumeration hangs off the menu ACTION (`FPLAYOUT.CPP`: `{IDS_JOINGAME,&selectsession,&RFullPanelDial::GetSessions}`), not off the screen. **A player who stops at the lobby sees an empty list box** — the "painted but inert" shape S82 booked for this front end, and the reason this gate asserts on list CONTENTS rather than on the screen appearing. <br>✅ **CONTROL VERIFIED (2026-08-29): with no host the enumeration STILL RUNS and returns 0.** <br>`the game enumerated sessions at all  yes` / `sessions the JOIN list would show  0` / `CONTROL OK`. <br>That distinction is the whole value of the arm: "ran and found nothing" is evidence about the host, while "never ran" is evidence about the drive. An earlier cut of this gate conflated them and would have printed `CONTROL OK` for a run that never reached the code — it now exits **2 (INCONCLUSIVE)** in that case instead. So the pass (1 session) and the control (0 sessions) differ by exactly one thing: whether a host is up. <br>⚠️ **The working drive is `BOB_AUTOCLICK="2,2"`, found by TRYING three candidates and letting the gate's INCONCLUSIVE verdict reject the wrong ones.** `2,3` — the sequence the menu order suggests — never reaches the code at all. Do not re-derive it by reading `FPLAYOUT.CPP`. <br>_(superseded plan:)_ **Next: a two-process UI gate** — process A hosts through the menus (as `bob_mp_connect.sh` already reaches the lobby), process B drives Join Game and asserts its session list is NON-EMPTY, with the existing no-host control (0 sessions) as the negative arm. `bob_mp_packet.sh` already runs two processes, so the harness exists. ⚠️ **Assert on the LIST CONTENTS, not on reaching the screen** — a Join screen renders identically whether the list has one entry or none, which is exactly the "painted but inert" failure S82 recorded for the front end. <br>⚠️ One semantic gap to keep in mind: the game passes `DPENUMSESSIONS_ASYNC`, and the compat **ignores the flags word entirely** — it blocks up to 400 ms and calls back synchronously. That is stricter than DirectPlay (which returns immediately and fills in the background), so it should be safe, but if the UI expects to poll a list that fills later, the timing differs from Windows. _(Large; slice into sub-stories at planning.) | 21 | ☐ |
| R7.2 | **H2H + co-op missions** — play a multiplayer mission end-to-end over the socket transport. | 13 | ☐ |

### Release SP — "Screen parity vs the Windows gold standard" *(PO-added 2026-07-25)*
*Gold standard: PO-supplied captures of the Windows build running under Wine —
`/run/media/admin/BEA6-BBCE/bob/` (17 PNGs, taken 2026-06-24). Every native screen must
match its gold shot. Formalizes R3.8's "A/B vs Wine" into a PO-curated, full-product
screen sweep with the gold shots as the fixed oracle.*
| ID | Story | Pts | Status |
|---|---|---|---|
| SP.1 | **Gold-shot inventory** — ☑ **DONE (S123, 2026-07-25).** All shots mapped (NB: the folder holds **19** PNGs, not 17 — two near-dupe side-selects, flagged for PO) with scripted repro recipes; `doc/screen-parity.md` verdict table (MATCH/CLOSE/PARTIAL/GAP + named deviations) + 15 native captures in `doc/parity/` (3 BEFORE/after pairs). New deterministic capture harness `BOB_SHOT`/`BOB_SHOT_PATH` (headless, private path); `BOB_CONFIGSCREEN` gained game/mission/views/flight/quick. | 3 | ☑ |
| SP.2 | **Front-end parity** — ◐ **BDG-ORACLE PE RESOURCES LANDED (S124)** after S123's 3 systemic fixes. S124: the port reads the INSTALLED build's PE resources (`boblang.dll` = BDG 0.99) at runtime — DIALOG rects/rows, DLGINIT captions with the genuine IDS→string-table resolution, template-driven hosting of non-DDX label statics (the Mission-tab root cause), template-membership draw filter (source-only controls BDG dropped aren't drawn). Every config tab now CLOSE with gold label sets ("Town and forest raises", "109 Fuel Capacity", "Gamma Level", BDG's extra GFX rows). `BOB_NO_PE_RSRC` reverts. _S123: (1) dialog-SCOPED control-rect lookup; (2) menu lists at the game's `ListX/ListY` (`BOB_NO_LISTXY` reverts); (3) runtime `ShowWindow` honored._ _S125: REdit hosted (#17 CLOSE — genuine `CREditCtrl`, gold line layout) + DLGINIT design-prop slices (`bob_dlg_columns`/`bob_dlg_resnum`): authored listbox columns → #16 tab-row spread per gold; RButton alignment byte → #17/#16 label placement; `BOB_NO_DLGINIT_PROPS` reverts._ _S126: **full sequential property-stream reader** — every hosted R\*'s genuine `DoPropExchange` replays its DLGINIT bag (layout validated vs all 1280 bags); authored fonts/colors land gold-exact (phase-date pixel-exact color); #16 duplicate date settled via covered-static erase emulation → #16 CLOSE; dummy==GL `cmp` bar adopted+passing; `BOB_NO_PROP_STREAM`/`BOB_NO_COVER_ERASE` revert._ Remaining: word-wrap (R6.2), `MoveWindow` page tracking, font face size mapping (FontNum→px, e.g. the Controls "4 axes" line draws large), "&&" escape, QS tab captions/recipe (#3), Directives dialog (#18), key-injection harness (#17 typed input/caret). | 13 | ◐ |
| SP.4 | **Host `CRSpinBut` — the 8th (and last) R\* control type.** The LW Directives grid's ~50 numeric spinner boxes and their values (gold #18: Bomber Allocation 40/30 %, per-aircraft gruppen counts, Resting counts) don't draw because `CRSpinBut` (`SRC/H/LWDIRECT.H`) has no host — the only R\* type still unhosted after S140. Follow the §8p new-control-type recipe (new host TU + genuine OCX in the build + CLSID in the factory); expect the two recurring OCX compile traps (§8t). Closes the last named deviation on #18 and is the prerequisite for any *editable* directives. *(S141)* | 6 | ☐ |
| SP.5 | **Dismiss a logged dialog headlessly — ANSWERED by MA note 29 §2, ready to implement.** Blocks capturing the strategic map *under* an auto-opened dialog, which is what gold **#19**'s remaining raid-stack/route deviation needs (an active Eagle Attack day is reachable since S141, but the Directives dialog covers the map). **The trigger: call `CloseLoggedChild(<INDEX>)` / `CloseLoggedChildren()` directly** — a capture scaffold must not care who opened the dialog, and S110 already de-bugged that machinery. Do **not** route through `OpenDirectivetoggle`: BoB's *is* a genuine toggle (MSCTLBR.CPP:378), so S141's "OpenXxx is ensure-open" read was wrong for BoB — the second stacked frame points to an **index mismatch** (the game's auto-open logs under a different child index than `DIRECTIVES`), which is exactly the state-dependent branch a scaffold must avoid. Verify the index first. *(S141, unblocked S142)* | 5 | ☐ |
| SP.6 | **Drive the genuine `OnLButtonDown` instead of recomputing its inputs (MA note 29 §1 — adopt MA's structure).** Two symptoms, one cause: our synthesized click *recomputes* the hit-test (S141 resolves the column via `GetColFromX`) and fires Select itself, so the control's own `m_iRowSel`/`m_iColSel` are never set — gold **#16** shows the selected phase tab WHITE, ours leaves all four gold. **MA is structurally immune to the S141 bug for this reason:** `CRListBoxCtrl::MaMouse` hands the point to the control's own `OnLButtonDown`/`OnLButtonUp` and reads `m_iRowSel`/`m_iColSel` back out, so both event args are whatever the genuine control decided and there is nowhere a column *could* be hardcoded. Adopting that shape fixes the highlight, retires S141's recompute, and removes the bug class rather than the bug. Touches the shared click path — gate with the A/B byte-identical sweep. *(S141, re-scoped S142)* | 5 | ☐ |
| SP.22 | **The port cannot read the installed build's saved settings — `settings.cfg` fails to parse (S150, MEASURED).** `[prefs] exists=1 … successfulLoad=0`: `SaveData::InitPreferences` finds `SAVEGAME/settings.cfg`, fails to deserialise it, and falls back to factory defaults (the game would also show its `IDS_CONFIGIGNORED` box). **Consequences beyond parity:** every run starts at factory defaults and `SavePreferences()` writes a file the next run rejects — the player's preferences are silently discarded. **It also reclassifies four parity rows:** #6/#9/#10/#11's "combo values (settings state)" are **our** deviation, not user state, because gold parsed the same file under Wine. Lead to test (not a conclusion): we build `-fpack-struct=1`, so an MSVC-written `SaveData` dump is a plausible layout mismatch — measure where the stream diverges (size? a version field? the first member that reads wrong) before changing anything. *(S150)* | 8 | ☐ |
| SP.23 | **Dialog teardown — root cause FOUND (S154), fix not yet landed.** ⭐ The real no-op is **`CWnd::DestroyWindow() { return TRUE; }`** (afxwin.h:855) — a textbook *stub that returns SUCCESS and hides a subsystem*, the class this port's notes say to grep `{ return TRUE; }` for. The whole chain runs correctly (`LWDirectives::OnCancel` → `RDialog::OnCancel` → `EndDialog(IDCANCEL)` → `RDialog::EndDialog` walks children → `DestroyWindow()`) and the last call lies. **S108's note said the no-op was `CDialog::OnCancel` — right about the symptom, wrong about the location** (`Rowan::CDialog` adds only a constructor, so `OnCancel` resolves to `RDialog::OnCancel`, which does real work); that misdirection is plausibly why this sat unexamined ~45 sprints. **Remaining, measured:** the teardown hook now fires but releases **1 control, not 184** — `DestroyWindow` reaches the panel/window, while the hosts belong to the **contained** dialog. Next step: release for the destroyed node's descendants (walk `fchild`/`sibling`), needing a safe `CWnd*`→`RDialog*` check first. All of it is behind `BOB_DLG_TEARDOWN`, **default-off**. *(S153, advanced S154)* | 5 | ☐ |
| ~~SP.23-orig~~ | ~~**The port has NO dialog teardown — the real root of the host leak (S153, measured).** compat's `CDialog::OnCancel` and `EndDialog` are **no-ops** (afxwin.h:1133/1135), so closing a logged child destroys nothing: `RDialog::DestroyPanel` never runs, dialog objects are never freed, and every re-open allocates a fresh dialog **plus ~184 fresh hosted controls**, all retained. Measured on the Directives dialog: 184 → 1656 in ordinary cycling, **181,424** when a scaffold closes it every paint, with tens of thousands still being *drawn*. **Delicate:** S108's re-entrancy guard exists precisely because our `OnCancel` does NOT clear the slot (the directive dialogs' cancel handlers form a toggle loop that recursed to stack overflow on Windows-faithful behaviour), so implementing teardown must preserve or replace that guard. `bob_ole_release_dialog()` + its `DestroyPanel` hook are already in place and will fire the moment teardown lands. *(S153)* | 8 | ☐ |
| SP.21 | **Gold #18's two remaining extra controls: the "Sweeps" LABEL and the "Escort 1:1" row.** S150 removed the stray 11-spinner sweep row (dead `SWEEPSNDECOYS` controls left in the template); these two are what remain. **Do not assume the same answer:** the sweep row was dead code, but `IDC_ESCORT_PROPORTION` (id 1680, y=195) is **live** — `LWDirectives` drives it via `SetIndex(escortproportion/.19)` — so gold hiding it needs a different explanation (BDG template delta? a state-dependent hide?). Measure with the per-control dump before theorising; the S146 lesson was that a plausible id guess named entirely the wrong controls. *(S150)* | 3 | ☐ |
| ~~SP.20~~ | ☑ **DONE (S152) — VERIFIED user-settings state, retired.** Read the actual differing values instead of theorising: on #11, ours are the game's **coded factory defaults** in every case (`targetsize = TS_MEDIUM`; `\|= GD_GROUNDCOLLISIONS`; `GD_UNLIMITEDARM` never set; complex-AI flag untouched) and gold's are none of them — gold's set is coherent as a player's chosen configuration. So the differences are genuine **user state in the gold capture**, out of scope for a port verdict. _The original annotation was right; S150's confident "it's ours" was the error._ | 3 | ☑ |
| ~~SP.20-orig~~ | ~~**Verify or retire the "combo values (settings state)" deviations (#6, #9, #10, #11).** The S149 audit closed three state mismatches that a recipe token could fix; these four are the same *shape* — a difference attributed to state — but the state lives in the installed build's saved settings, not in a drive recipe, so they are not a one-token fix. Establish which they are: capture with the gold build's settings (or diff the settings the two builds actually hold) and either match them or state positively that the values are user-settings and therefore out of scope for a parity verdict. Current honest status is "plausibly benign, **not verified**". *(S149)* | 3 | ☐ |
| SP.19 | **Never rebuild while a gate run is queued or in flight (S148, self-inflicted).** S148's sweep came back **13/14** — the first non-byte-identical result in seven sprints — and the cause was not code: I rebuilt twice while `gates148` sat in the `gl-lock` queue, so the 14 sequential `bob` invocations straddled binaries and `config-control` (4th recipe) was captured mid-swap. Proof: three fresh runs on the final binary are identical to each other **and to the pre-S142 baseline**; only the sweep frame differs. **A gate whose inputs can change under it is not a gate.** Make `gates*.sh` stamp the binary's mtime/hash at start and again at end, and fail loudly if they differ; and treat "queued" as "running" for the purposes of touching the tree. *(S148)* | 2 | ☐ |
| SP.18 | **Scaffold logging must print transitions, not polls (S148, self-inflicted).** Making `bob_oob_close_dialogs` repeatable in S147 without making its logging conditional turned six per-toolbar lines into per-paint spam: one capture run produced a **3.5-million-line log**, which is noise *and* enough stderr I/O to drag a run that has to reach a particular game state. Fixed there (log only when a pass actually closed something). **The general rule — "filter, don't cap" applies to scaffold logging, and a one-shot hook made repeatable needs its logging revisited in the same change.** Sweep the other repeatable/`static done` hooks for the same shape while doing SP.15. *(S148)* | 2 | ☐ |
| SP.17 | **Audit all 19 parity verdicts for STATE mismatches (S141 + S148 found two).** A verdict can be right about the port and still not be *earned* by its evidence, if the native capture and the gold shot are in different game states. Twice now: **#16** compared our Convoys phase-select against a gold showing **Eagle Attack** (S141), and **#19** compared a fresh-Convoys map against a **12-Aug Eagle-Attack** gold from S123 until S148. Both were caught incidentally, which is the worrying part. Now that captures can be state-predicated (SP.16) and self-describing (SP.9), re-check each row: does the recipe put the port in the state the gold shot shows — same phase, day, side, settings, screen? Cheap per row, and it protects every verdict the epic rests on. *(S148)* | 5 | ☐ |
| SP.16 | **Arm captures on a GAME-STATE PREDICATE, not on paints (S147 — the limit of SP.7).** SP.7 fixed drift caused by *queueing* by arming the shot from the drive. S147 found the next layer: with the dismiss scaffold suppressing every blocking prompt, the campaign runs **faster**, so "150 paints after arming" no longer lands at the same game state — the same recipe that gave 12 Aug 07:52 in S146 gave **13 Aug** twice in S147. **The scaffold changed the very quantity the timing depended on.** Fix: let a recipe say *what state to capture* (`BOB_SHOT_WHEN=date==12aug && packages>0`, or a `bob_shot_arm_when()` predicate evaluated per paint) instead of counting anything. This is what gold #19's clean capture actually needs, and it generalises to every future timing-sensitive parity shot. *(S147)* | 5 | ☐ |
| SP.15 | **Sweep BoB's drive hooks for one-shot statics (MA note 29 §3 — I had the bug in a hook written AFTER reading the warning).** `bob_oob_close_dialogs` was guarded by a function-local `static int done`, so the dismiss fired **once per process** and every dialog the game opened later sailed past it — the Mission Folder, then the "Take over?" intercept prompt. I was writing that up as "the game keeps re-opening dialogs"; part of that was real (the Directives popup genuinely re-arms — measured as an oscillation) and part was my own scaffold. Fixed there; **five more `static int done` hooks remain in MAINFRM.CPP** (lines ~1375/1445/1458/1482/1521) and each needs the same judgement: is once-per-process right for this hook, or must it re-arm? MA's smell test: *a drive counter declared inside the block it drives can only ever run once*. *(S147)* | 3 | ☐ |
| SP.14 | **Do BoB's MAP OOB dialogs accept real clicks, or are they render-only? (MA note 31 §3).** MA found its OOB dialogs painted perfectly and ignored every click — the clues were scaffold-only exercise and an uncalled `ma_tabs_hit`. **BoB cannot currently claim better:** every drive this session (phase select, directives accept, dismiss) went through `BOB_*` scaffolds, which is exactly that evidence pattern. BoB does have a genuine click route (`bob_ole_click` + S33 eventsink + S92 toolbar clicks) and S129 proved real clicks switch the QS tabs, so the front end is not render-only — but the **map OOB dialogs** are untested. Test a real click into an open OOB dialog; if it does nothing, mirror the paint walk for hit-testing (same tree, same offsets, children first) so hit rects cannot drift from drawn rects, give an open dialog first refusal, and swallow in-dialog misses instead of leaking them to the map. *(S146, from MA note 31 §3)* | 5 | ☐ |
| SP.13 | **`CRButtonCtrl::OnLButtonUp` derefs NULL via the stubbed `WM_GETHINTBOX` (MA note 31 §2).** `phintbox=(CDialog*)GetParent()->SendMessage(WM_GETHINTBOX,..); phintbox->ShowWindow(SW_HIDE);` — `ON_MESSAGE` is an empty macro, so the SendMessage returns 0 and the next line dereferences it. Guarded by `if (phintbox)` on entry, so it bites only once that member is set non-NULL — which is why firing events through the sink has never hit it, and why **wiring a real title-bar click would**. Note the *same* idiom in `CRListBoxCtrl::OnLButtonDown` **does** re-check, so one codebase holds both the safe and unsafe spelling: check each `SendMessage(WM_GETHINTBOX` site individually. §8i family. *(S146, from MA note 31 §2)* | 3 | ☐ |
| SP.12 | **Make the eventsink walk base classes (S144 finding).** `bob_evt_fire` matches `type_info` **exactly** (bob_eventsink.cpp:39) and every call site passes `typeid(*dlg)`, so an `ON_EVENT` registered on a BASE class can never fire for a derived object. That kills `ON_EVENT(RDialog, IDJ_TITLE, 3=OK, OnOK)` and its Cancel/Help siblings — the engine's **title-bar ✓ / ✕ / ?** buttons — for every real dialog in the game, since all of them are derived. It stayed invisible because every previously-wired event was registered on the same class that received it. S144 worked around it by firing under `typeid(RDialog)` (the derived override still runs, via the implicit virtual). The general fix is a base-class walk in the sink; it changes dispatch for every existing registration, so it needs the byte-identical sweep as its gate. Shared as §8z. *(S144)* | 5 | ☐ |
| SP.11 | **Complete the LW orders flow (drive `OnOK`), which is what CREATES the day's raids — gold #19.** S143 established that #19's missing raid stacks are not a clock/accel problem: the Directives ⇄ DirectiveResults pair is a **closed loop on cancel** (each one's `OnCancel` opens the other), so the map cannot be uncovered by dismissing, and suppressing the day-start popup doesn't break it. That is faithful — the LW player must *issue orders*, not escape. The exit is `OnOK`, and `DirectiveResults::OnOK` calls **`LWDirectivesResults::MakeLWPackages(dr, true)`** (DIRRSULT.CPP:207) — the call that builds the day's packages. Drive the genuine OK handlers (eventsink, per MA note 29 §1) on both dialogs, then let the map run: that should give both a *clear* map and the raid stacks/routes gold #19 shows. Supersedes S141's "needs an active day" framing of #19. *(S143)* | 5 | ☐ |
| SP.10 | **Hosted controls LEAK when a dialog is re-opened.** Found incidentally by the S143 state banner: driving the Directives dialog through a few open/close cycles took dialog 1032's hosted-control count from **184 to 1656** — each re-open re-creates the controls and the old hosts are never released. Not a scaffold-only problem: the game re-opens that dialog on its own (`DirectiveResults::OnCancel`, the day-start path), so a long campaign session leaks in normal play, and every leaked host also gets *drawn* (the banner showed 334 drawn of 1656). Find the destroy path (`PreDestroyPanel`/`DestroyPanel` → the `bob_ole` side-table) and release hosts with their dialog. **This is exactly the kind of defect the banner exists to surface** — nobody was looking for it. *(S143)* | 5 | ☐ |
| SP.9 | **Every parity capture must EMIT the state it claims to capture (FF note 15).** FF spent a sprint on a deviation whose recorded mechanism was invented, because their capture never logged which view was actually in force — so "the recipe didn't fire", "something switched it back" and "the renderer is wrong" were indistinguishable. **This applies live to BoB:** the S142 Directives capture was run *without* `BOB_TRACE_OLE` (the trace is per-control-per-frame and starved the run), so the run emitted no record of which campaign phase, which dialog id, or which art set was actually up. S142's verdict is corroborated only because the state happens to be *rendered into the pixels* (the clock reads "12 August 08:45" and the allocation values are Eagle-Attack-specific) — that is luck, not method. Add a one-line, always-on capture banner to `BOB_SHOT`: screen/dialog id actually up, art file **resolved** (not requested), phase/date, and the recipe steps that actually fired. Cheap, and it converts "looks wrong" into "looks wrong in the state we intended". *(S142, from FF note 15)* | 3 | ☐ |
| SP.8 | **Gold #18: suppress the two rows gold doesn't show.** _S144 investigation (no code yet):_ the extra row is the **"Sweeps" / fighter-sweep target row** — `IDC_FIGHTERSWEEP*` (1103, 1107, 1109, 1114, 1115, 1119, 1120, 1124, …) — and it is **dead in the shipped game**: `LWDIRECT.CPP:1626` guards that branch with `INT3; //This should not happen. Patrols removed.` So gold is right to show nothing, and the question is only *which* mechanism the Windows build uses to drop it (BDG template membership, per S124, is the first candidate — check `bob_dlg_in_template(IDD_LWDIRECTIVES, 1103)`; note our control creation is DDX-driven, so a control absent from the template is still *created*, just filtered at draw). Beware the standing watch-item: `INT3` does not halt on compat, so that "can't happen" branch falls through. _S146 probe result:_ the S124 **template filter is working correctly** on this dialog — `[tmpl] dlg=1032: 167 in template, 17 absent`, and the banner shows exactly `167/184` drawn, so the 17 non-template controls ARE suppressed. **No hosted control on that dialog has an id in the 1103-1130 sweeps band**, so the `IDC_FIGHTERSWEEP*` id guess above is wrong for the row we actually draw. The extra row is therefore *in* the BDG template and hidden by some other mechanism (runtime `ShowWindow`/enable, or a per-row draw rule) — widen the probe to dump every hosted id + rect on that dialog and identify the row by its y-band before theorising further. ** With the spinners hosted (S142) the Directives grid matches gold number-for-number, but we additionally draw a **"Sweeps" spinner row** and an **"Escort 1:1" combo row** that gold shows nowhere — and Sweeps overprints the "Ground Attack Gruppen"/"Escort Gruppen" section headers. Same class as S123's runtime-`ShowWindow` and S124's template-membership filter (a control the Windows dialog manager creates but the game hides in this state), so check those two mechanisms before anything else. Found by the S142 capture; the only remaining *layout* deviation on #18. *(S142)* | 3 | ☐ |
| SP.7 | **Audit one-shot drive counters + absolute `BOB_SHOT` timing (MA note 29 §3).** `if (++n == N)` / `static int done` on a function-local static fires **once per process**, which capped MA's campaign at one flyable mission and read for the port's whole life as a *game* limitation. BoB has at least `bob_oob_open_directives`'s `static int done` and every `BOB_AUTOCLICK`/map-drive counter. Sweep them; make per-occurrence hooks re-arm on the state transition that ends the occurrence. **Same note, second half — felt directly in S142:** an absolute `BOB_SHOT=<tick>` cannot be aimed at something whose arrival time varies; adding 85 spinners slowed the run enough that the capture tick moved. Arm the capture **from the drive** (fire N ticks after the dialog opens) instead of from an absolute idle count. *(S142)* | 5 | ☐ |
| SP.3 | **Flight / map parity** — ◐ **BOTH CAPTURED + VERDICTED (S123).** Cockpit vs gold: CLOSE (structure/instruments/HUD readout match; prop-blur + HUD style deviations named). Strategic map vs gold: CLOSE (terrain/sectors/icons/footer/toolbars/clock match; raid-stacks/routes absent in the fresh-day capture, ruler art plain, Directives dialog = GAP). Remaining: LW Directives dialog reachability, raid-day capture, deviation fixes. | 13 | ◐ |

### Release P — "Mine the patch changelists and the docs for bugs we still have" *(PO-added 2026-08-25)*

> **PO:** *"check the ~/sgl/TUE patch changelists for ma and bob, and check whether any bug fixes
> listed in these patch changelists are bugs that need to be fixed in the ma or bob linux
> codebase"* … *"do the same with any bugs mentioned in ~/sgl/TUE ma or bob documentation, either
> that distributed with the games or provided later by the user communities"*.

⭐ **The point that gives this teeth: WE COMPILE THE SOURCE, THE ORACLE IS A PATCHED BINARY.** Our
parity oracle is the **BDG 0.99** build (`boblang.dll`, S124), and the BDG patch line runs 0.70 →
0.99 with a documented fix history — `DOC/BDG_99_patch_manual_summary.md` already names *"0.98:
fixed crash issues"* and *"0.97 to 0.973: bug fixes, including texture corruption and waypoint
errors"*. Every bug those patches fixed **in the EXE** is by default **still live here** (we build
the pre-patch sources) while being **absent from the gold shots**. So this is both a list of
already-diagnosed bugs we have never looked for, **and** a reason some recorded parity deviations
may be patch differences rather than port defects.

| # | Story | Pts | Acceptance criterion | Status |
|---|---|---|---|---|
| P1 | Establish what patch level our SOURCE is. | 3 | Written answer with evidence: does `SRC/` already contain the 0.9x fixes, or is it the pre-patch tree? **Do this first** — it decides whether the list is long or empty. | 🔨 **NEW — do first.** |
| P0 | Inventory the corpus, extract every named bug/fix. | 5 | A table in `doc/patch-bugs.md`: source → version → symptom → implication. Sources: `DOC/BDG_99_patch_manual_summary.md`, `DOC/BDG Patch Battle of Britain Manual_1.pdf`, `DOC/patch 0.99Manual.pdf`, `DOC/Patch 0.98 manual.pdf`, `DOC/BattleOfBritainOnlineDocumentation.txt`, `DOC/GettingStartedAndTroubleshootingGuide.pdf`, `DEBUG/THU_graphics_glitches.txt`. | 🔨 **NEW** |
| P2 | Triage each: live / already-fixed / N/A / data-only. | 8 | Every row gets a verdict **from evidence** (a grep, a run, `git log -L`), never from the description. Patch items shipping only DATA (art, missions, `bdg.txt`) are N/A to a source port and marked so. | 🔨 **NEW** |
| P3 | Fix the live ones, highest-impact first. | 13 | Each fix gated or measured like any other item. | 🔨 **NEW** |
| P4 | Re-examine screen-parity verdicts in the light of P1. | 5 | Any deviation explained by a patch difference is re-marked with the item cited. **An oracle we mis-attribute is worse than no oracle** — and `doc/screen-parity.md` currently reads 18 CLOSE / 1 GAP on that assumption. | 🔨 **NEW** |

**Release P total: 34 pts.** P1 first, then P0 → P2 → P3/P4.
⭐ **`DEBUG/THU_graphics_glitches.txt` is worth reading on day one** — a community-recorded list of
*graphics glitches*, which is exactly the class our parity gates argue about.

### Icebox (environment-blocked — not schedulable until the environment changes)
| ID | Story | Why parked |
|---|---|---|
| ICE.1 | **Music assets** (`MUSIC/*.xmi` absent from the install) | **RE-SCOPED 2026-07-19 — the code path is DONE, the *assets* are missing.** Both original premises were false (see `doc/CROSS-PORT-FROM-REVIEW-2026-07-19.md`): (a) 32-bit FluidSynth **is** installed (`libfluidsynth3:i386` 2.4.8, plus `libfluidsynth-dev:i386`) and needs no ALSA-seq/system soft-synth — it renders in-process; (b) `.DIR` is **not** a proprietary archive but a 640-byte plain filename index (BoB's `MUSIC/DIR.DIR` is byte-identical to MiG Alley's, md5 `d27ecb89639958b6b3576a5646856924`) that `SRC/FILES/FILEMAN.CPP`'s `namenumberedfile`/`opennumberedfile` layer already reads. Also note BoB's music is **DirectMusic**, not `midiOut*` (`SRC/HARDWARE/MUSIC.CPP`; `midiOut*` only appears in the dead Miles/SoundFont code). The synth path is now implemented and proven (`SRC/compat/bob_music.cpp`, S121). **What remains is an asset condition:** `MUSIC/`, `MUSICMED/` and `MUSICLOW/` each contain only `DIR.DIR` — 0 `.xmi` on disk (likely CD-resident, as with MiG Alley). The engine degrades silently, as designed. Re-open as a normal story once the `.xmi` payload is located. |

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

### Sprint 126 — "Property-stream reader lands + the GL gate returns" → *Increment: bag-faithful hosted controls, capture-proven* — **✅ CLOSED 2026-07-27 (8/8 pts; see §9 row 126 + PORT.md S126)**
- **Sprint Goal:** finish + prove the S126 WIP (salvage `9105e25`) — the persisted property-stream
  reader feeding every hosted R\* control's genuine `DoPropExchange` — with the full capture-diff
  sweep and revert gates; restore the real-GL default-run DoD gate (GLX probe: **HEALED**, NVIDIA
  direct rendering back); adopt MA note 16's byte-identical `cmp`(dummy, GL) capture bar; process
  MA note 16 (§2 residual PX-defaults checks a/b).
- **Committed (~8 pts):**
  - **S126.1 (5)** — SP.2 slice: property-stream reader completion. Build clean; re-run the
    14-recipe headless capture sweep vs the S125 references (diffs must be surgical + explained);
    verify the revert gates (`BOB_NO_PROP_STREAM` → S125 spot-fix behaviour, `BOB_NO_DLGINIT_PROPS`
    → whole layer off, `BOB_NO_COVER_ERASE` → keep the covered static); update
    `doc/screen-parity.md` verdicts (#16 duplicate-date settled-state emulation).
  - **S126.2 (2)** — DoD gate restore: default `./bob` exits 0 on `:0` (flock-wrapped) + the new
    acceptance bar — a headless SDL-dummy `BOB_SHOT` capture byte-identical (`cmp`) to a GL-run
    capture of the same screen at the same idle (catches the uninitialized-PX-garbage class
    without a display; MA note 16 §1).
  - **S126.3 (1)** — Cross-port: apply MA note 16 §2 residual checks (a: every control-creation
    path runs `DoPropExchange`; b: bag-omitted members still get PX defaults), reply as BoB note
    17 in BOTH copies of the shared lessons doc (byte-identical).
- **Increment demo:** phase-select/enter-name/config screens render with bag-faithful fonts,
  colors, columns and the settled-state date heading; dummy==GL `cmp` passes; bare `./bob` exits
  0 on real GL.

### Sprint 127 — "Label-render fidelity" → *Increment: descriptions word-wrap, labels escape '&'* — **✅ CLOSED 2026-08-02 (8/8 pts; see §9 row 127 + PORT.md S127)**
- **Sprint Goal:** close the Release-SP "remaining render classes" items that live in the one
  compat method rendering every R\* static label (`CDC::DrawText`): multi-line word-wrap for
  description statics (R6.2) and the '&'/"&&" accelerator escape (#8). Adopt MA note 17's
  `CDC::DrawText DT_WORDBREAK` shared find on the BoB side.
- **Committed (~8 pts):** S127.1 (5) DT_WORDBREAK word-wrap with the ≥2-line-box regression
  guard; S127.2 (2) '&' accelerator-prefix escape (DT_NOPREFIX-aware, static path only);
  S127.3 (1) cross-port note + real-GL DoD gates + dummy==GL `cmp`.
- **Delivered:** #8 '&&' + #16 word-wrap deviations retired (both already CLOSE), #2 improved (parity 15 CLOSE / 1 PARTIAL / 3 GAP
  of 19). Build clean; 14-recipe headless sweep 14/14; surgical diffs, no config-label wrap
  regression; flight frame-150 95.2% non-black on `:0`; dummy==GL `cmp` byte-identical on
  mainmenu + the changed phaseselect. MA note 17 mechanism #2 (parent-rect clip) assessed N/A.
- **Increment demo:** `BOB_AUTOCLICK=1,1 BOB_SHOT=380` (phase-select) shows the phase blurb
  wrapped in its box; `BOB_CONFIGSCREEN=control BOB_SHOT=70` shows "Cockpit & UI".

### Sprint 128 — "Host the last front-end control type" → *Increment: Quick-Shots page tabs render* — **✅ CLOSED 2026-08-02 (6/6 pts; see §9 row 128 + PORT.md S128)**
- **Sprint Goal:** close #2's last PARTIAL deviation (blank page-tab row) and meet the #3
  prerequisite by hosting the `CRRadioCtrl` — the one front-end R\* control type BoB had not
  yet hosted (`IDC_RRADIO`, the QS Scenario/Parameters/Luftwaffe/RAF tabs).
- **Committed (~6 pts):** S128.1 host CRRadio (new host TU + factory CLSID + build integration
  + the genuine control's MaskIcon compile-compat fix; verify tabs render + DoD gates).
- **Delivered:** #2 PARTIAL→CLOSE (parity 16 CLOSE / 0 PARTIAL / 3 GAP of 19); 6th hosted R\*
  type; 9/9 regression sweep; dummy==GL `cmp` byte-identical on the QS screen; flight + safe
  default gates pass. #3's remaining half (tab-click page-switch + `MoveWindow` page visibility)
  named as a distinct open item.
- **Increment demo:** `BOB_STARTFLYING=click BOB_AUTOCLICK=0 BOB_SHOT=220` shows the QS page-tab
  row (✓Scenario / Parameters / Luftwaffe / RAF with selection ticks).

### Sprint 129 — "Quick-Shots tab navigation" → *Increment: click a tab, switch the page* — **✅ CLOSED 2026-08-02 (4/4 pts; see §9 row 129 + PORT.md S129)**
- **Sprint Goal:** make the S128 QS page tabs interactive — a click switches the panel page —
  and reach the gold #3 Parameters/player screen.
- **Committed (~4 pts):** S129.1 RRadio click hit-test (`OleHost::onButtonClick`) + eventsink
  wiring in `bob_ole_click` → `OnSelectedRradio` → `LaunchDial`; verify the page renders.
- **Delivered:** bidirectional tab nav works (Parameters ↔ Scenario, both render); the
  Parameters _tab_ (mission params) renders on a genuine click — a real new reachable QS page.
  **Scope correction:** gold #3 (`16-47-45`) turned out to be the player-flight editor
  (`CSQuickLine`), a different screen than the Parameters tab — #3 stays GAP with its true path
  identified; parity unchanged 16 CLOSE / 0 PARTIAL / 3 GAP. All gates pass (7/7 regression,
  dummy==GL `cmp` byte-identical, flight + safe-default).
- **Increment demo:** navigate to QS, click the Parameters tab (`BOB_CLICKXY="215,214,191"`) →
  the mission-parameters form (Target Area/T.D./Weather/Time/Name) renders.

### Sprint 130 — "gold #3 OOB spike" — **◐ SPIKE, banked 2026-08-02 (3 pts; §9 row 130 + PORT.md S130)**
- Root-caused the QS order-of-battle SIGSEGV (null-DialBox copy in a variadic panel ternary);
  fix is a game-code UB-exception deferred. No code shipped.

### Sprint 131 — "Per-face font registry" → *Increment: data/labels render Arial* — **✅ CLOSED 2026-08-02 (8/8 pts; §9 row 131 + PORT.md S131)**
- **Sprint Goal:** adopt inbound MA note 26 to fix the pervasive "font face" deviation — bob_gdi
  drew every face in the one art TTF, so data/label rows rendered in the Rowan art face not Arial.
- **Committed (~8 pts):** S131.1 EnumFontFamilies availability (§1), S131.2 per-face registry (§2),
  S131.3 combo black-fill (§3) + verify/gates.
- **Delivered:** §2 per-face registry (4 kinds × regular/italic; Intel ART / Arial SANS / Times
  SERIF / Courier MONO) threaded through the DC's `CFont`, + italic (gold's italic combo values).
  §1 and §3 verified **N/A for BoB** (already requests English faces; combos already translucent).
  ART screens `cmp` byte-identical; controls dummy==GL byte-identical; 14/14 sweep; flight + safe
  default pass. The "font face" deviation retired across the config/campaign screens. Cross-port
  §8r; MA notes 26/27 processed. `BOB_NO_FONTFACE` reverts.
- **Increment demo:** `BOB_CONFIGSCREEN=control BOB_SHOT=70` — labels/values render in Arial
  (values italic) = gold's scheme; `BOB_TRACE_FONT` shows the requested faces.

### Sprint 132 — "Kill the QS order-of-battle crash" → *Increment: the RAF/Luftwaffe tabs no longer SIGSEGV* — **✅ CLOSED 2026-08-02 (6/6 pts; §9 row 132 + PORT.md S132)**
- **Sprint Goal:** fix the S130-root-caused crash (null-`DialBox` copy in the QS OOB panel
  builders) so the RAF/Luftwaffe tabs — clickable since S129 — stop crashing.
- **Committed (~6 pts):** S132 the game-code UB-exception fix + regression + gates.
- **Delivered:** null-reference-safe `DialBox` copy ctor (RDIALOG.H, BOB_LINUX) — inactive slot →
  empty leaf DialBox; `AddChildren` renders it as empty `RDEmptyP`. Two layers (ctor deref + the
  uninit `diallist` behind copy-elision). RAF-tab exit 0; 13/13 sweep; core change byte-identical
  on working screens; flight + OOB dummy==GL + safe default all pass. **Honest:** crash fixed +
  screen unblocked; the `CSQuickLine` content (gold #3 fields) doesn't paint yet (deferred).
- **Increment demo:** `BOB_STARTFLYING=click BOB_AUTOCLICK=0 BOB_CLICKXY="215,458,191"` (RAF tab)
  loads the OOB screen without crashing (previously SIGSEGV).

### Sprint 133 — "QS order-of-battle flight-lines render" → *Increment: the RAF/Luftwaffe tabs show the player flight row* — **✅ CLOSED 2026-08-02 (6/6 pts; §9 row 133 + PORT.md S133)**
- **Sprint Goal:** close the nested-dialog-render gap S132 named — make the `CSQuickLine`
  flight-line content paint on the QS order-of-battle (RAF/Luftwaffe) tabs, which S132 unblocked
  structurally but left blank.
- **Committed (~6 pts):** S133 nested `DialList` panel draw walk + regression + gates; verify MA
  note 28 (OOB listbox black-fill) against BoB's campaign-map OOB dialogs.
- **Delivered:** `bob_fp_draw_nested`/`bob_nested_walk` (FULLPSYS.CPP, BOB_LINUX, default-on,
  `BOB_NO_QS_NESTED` reverts) walk the panel's child `RDialog` tree and draw each nested dialog's
  hosted controls, **synthesizing** the vertical row stacking (the game's headless layout gives no
  real rects — probe `BOB_TRACE_OOBTREE`: nested nodes have viewsize height 0 / full-screen
  GetWindowRect) while reusing the existing template-rect column positioning. RAF tab now shows the
  flight row (piloted-flag icon + Patrol/Altitude/Skill → Spitfire IA/1/Veteran). **MA note 28
  verified N/A** — the campaign-map Bases dialog already composites its squadron lists over the
  translucent panel (no black fill). Gates: config-gfx2 + QS-Scenario **byte-identical** on/off (no
  regression on flat screens); mainmenu dummy==GL byte-identical; flight 94.9% non-black; safe
  default exit 0.
- **Honest:** single-flight rows validated; multi-row stacking synthesized (`rowStep=40`), not yet
  captured with >1 flight. Renders the OOB **list**; gold #3 proper (the per-flight *editor* via a
  flight-line click) stays GAP with that click the only remaining step.
- **Increment demo:** `BOB_STARTFLYING=click BOB_AUTOCLICK=0 BOB_CLICKXY="180,458,191;220,458,191"`
  (RAF tab) → the player flight row renders (Spitfire IA / 1 / Veteran).

### Sprint 134 — "gold #3 mapping re-correction (spike)" — **◐ SPIKE, banked 2026-08-02 (2 pts; §9 row 134 + PORT.md S134)**
- Chasing S133's stated "flight-line click → editor" remaining-step, found it was itself a
  mis-mapping. gold #3's "Return to Player" is `IDC_RETURNTOPLAYER`, which lives in **`IDD_BOBFRAG`**
  (`class BoBFrag`, BOBFRAG.CPP) — the mission **briefing** (pilot roster + Squadron/Aircraft/Duty/
  Callsign + formation callsign buttons + Back/Sim Config/Fly footer), not a QS `CSQuickLine`/
  `QuickParameters` sub-editor. It is reached via the **mission-fly / campaign-intercept flow**
  (`{IDS_FLY,&bobfrag,CheckForMissingMission}`; or `BOB_CAMPAIGN_FLY`) — the QS **Scenario-page**
  "Fly" is `{IDS_FLY,&quickmissionflight,FragFly2}` → straight to flight (verified: hangs under the
  SDL-dummy capture driver, which has no GL). No code shipped (investigation only; game pristine).
  **Next sprint (scoped):** reach BoBFrag headlessly (campaign seam) + render its roster — likely
  reuses the S133 nested-panel draw + the RButton callsign buttons. #3 stays GAP, now mapped to its
  true screen + reach-path.

### Sprint 135 — "Render the mission briefing (gold #3)" → *Increment: the BoBFrag briefing renders with its roster* — **✅ CLOSED 2026-08-02 (6/6 pts; §9 row 135 + PORT.md S135)**
- **Sprint Goal:** reach the `IDD_BOBFRAG` mission briefing (gold #3, per the S134 re-mapping)
  headlessly + render its flight roster, flipping #3 off GAP.
- **Committed (~6 pts):** S135 a reliable `BOB_BOBFRAG` reach scaffold + verify the render + gates.
- **Delivered:** `BOB_BOBFRAG` scaffold (FULLPSYS.CPP, BOB_LINUX, default-off) — QS click-mode
  pre-flight seeds `quickdef` → `LaunchScreen(&quickmission)` inits CSQuick1 → `LaunchScreen(&bobfrag)`;
  stops at the briefing (no Rtestsh1 → no flight), so `BOB_SHOT` captures it. The briefing renders
  closely matching gold #3: crashed-109 + pink-cloud + two-He111 background, the roster listbox
  (`CRListBoxCtrl` id=1481: **Unit / Aircraft / Duty / Callsign → 54 Squadron / Spitfire / Patrol /
  Trumpet**), Back/Sim Config footer, exit 0. Gates: safe default exit 0; mainmenu dummy==GL
  byte-identical; flight 94.9% non-black (scaffold is env-gated → no regression).
- **#3 GAP → PARTIAL.** Deviations named: "Return to Player" RButton not drawn (front-end RButton
  hosting is the follow-on); name edit (`CREditCtrl` 1923) created-not-drawn; Fly footer item gated.
- **Increment demo:** `BOB_BOBFRAG=1 BOB_SHOT=120` → the briefing with the flight roster
  (`doc/parity/native-quickshots-bobfrag-2026-08-02.png`).

### Sprint 136 — "Render the briefing's Return-to-Player button" → *Increment: template-driven button hosting; gold #3's key element renders* — **✅ CLOSED 2026-08-02 (5/5 pts; §9 row 136 + PORT.md S136)**
- **Sprint Goal:** close the most visible S135 deviation on gold #3 — the "Return to Player" button
  — by extending template-driven hosting to non-DDX buttons.
- **Committed (~5 pts):** S136 `bob_dlg_enum_buttons` + template-button hosting + regression A/B.
- **Delivered:** gold #3's "Return to Player" (`IDC_RETURNTOPLAYER`=2146) is a template-only button
  no DDX binds, so it was never created. Extended the S124 template-driven static hosting to
  non-DDX **buttons** (`bob_dlg_enum_buttons` in bob_dlgtemplate.cpp + a K_RBUTTON pass in
  `bob_ole_host_template_statics`; `bob_make_rbutton` already renders a hosted RButton) — the button
  now draws top-left with its caption, matching gold #3's key element. `BOB_NO_TEMPLATE_BUTTONS`
  reverts. **Regression:** config (gfx2/game/mission/control/sound) + QS-Scenario + mainmenu +
  phase-select all **byte-identical** on/off (the hosting is inert wherever no non-DDX template
  button draws); safe default exit 0; flight 94.9% non-black.
- **#3 stays PARTIAL** (improved) — remaining: the "Bob" pilot name box (a `CREdtBt` slot, not yet
  front-end-hosted) + the Fly footer item (gated). Button art is the tickbox icon vs gold's bezel.
- **Increment demo:** `BOB_BOBFRAG=1 BOB_SHOT=120` → the briefing now shows "Return to Player".

### Sprint 137 — "Reach the LW Directives dialog (gold #18)" → *Increment: the Directives dialog is reachable + renders* — **✅ CLOSED 2026-08-02 (5/5 pts; §9 row 137 + PORT.md S137)**
- **Sprint Goal:** bring up the LW Directives dialog (gold #18, the last-but-one GAP) — reach it
  headlessly + render it — moving #18 off GAP.
- **Committed (~5 pts):** S137 open scaffold + TB_MISC OOB paint extension + regression + capture.
- **Delivered:** the Directives dialog (`LWDirectives`/`IDD_LWDIRECTIVES`) lives on the **misc**
  toolbar (TB_MISC), which `bob_map_paint_oob` didn't walk. Added `bob_oob_open_directives`
  (`MiscToolBar().OpenDirectivetoggle(NULL)`, null-safe — the ctor builds a default results from
  `MMC.directives.lw.current`), a `BOB_MAP_DIRECTIVES` trigger, and extended `bob_map_paint_oob` to
  render TB_MISC logged children via a full recursive walk (`bob_oob_paint_tree_deep`, fchild+sibling).
  The dialog opens (exit 0, no crash) + renders its frame + "Rest All" + standby reminder. **#18 GAP
  → PARTIAL.** Regression: Bases OOB (TB_MAIN path unchanged) still renders; TB_MISC paint inert when
  no misc dialog logged (no map regression); safe default exit 0.
- **Honest:** the dense allocation grid doesn't show — gold #18 is 12 Aug Eagle Attack (active
  gruppen); mine is 10 July Convoys where the game shows the standby state (grid hidden until an
  active phase). Same fresh-day-vs-Eagle-Attack state gap as #19; confirming the grid renders through
  the deep walk once active is the follow-on.
- **Increment demo:** `BOB_AUTOCLICK=1,1,1,1 BOB_MAP_TIMER=8 BOB_MAP_DIRECTIVES=1 BOB_SHOT=900`.

### Sprint 139 — "Footer-listbox clip fix" → *Increment: clipped last footer/tab columns render* — **✅ CLOSED 2026-08-03 (3/3 pts; §9 row 139 + PORT.md S139)**
- **Sprint Goal:** fix the S138-diagnosed footer draw bug so gold #3's "Fly" (and any other clipped
  last column) renders.
- **Committed (~3 pts):** S139 the `bob_draw_menu` clip-width widen + A/B regression.
- **Delivered:** the footer/tab `CRListBoxCtrl` lays its columns at its own internal widths but
  `ExtTextOut`-clips each to the passed `rcBounds`; `bob_draw_menu`'s tight `total` (re-measured text
  widths) clipped the last column off (bobfrag's "Fly", the QS Scenario "Fly", config tab edges).
  Widened the listbox clip to the remaining screen width — column positions are internal + hit-rects
  come from `wids[]`, so nothing moves, previously-clipped columns just appear (`BOB_NO_FOOTER_CLIP`
  reverts). **General fix:** bobfrag footer now Back/Sim Config/Fly (gold #3); QS Scenario footer
  now Back/Fly (gold #2); config tab-row / phase / campsel reveal clipped char edges. A/B verified
  each diff is a benign clipped-edge reveal (mainmenu byte-identical; all diffs ≤ a few px in the
  footer/tab band). Gates: safe default exit 0; flight 94.9% non-black.
- **#3 nearer CLOSE** — only the "Bob" name box (`CREdtBt`) remains; #2 footer improved.
- **Increment demo:** `BOB_BOBFRAG=1 BOB_SHOT=120` → footer shows Back / Sim Config / Fly.

### Sprint 140 — "Host CREdtBt — the briefing name box (gold #3 → CLOSE)" → *Increment: the "Bob" pilot name box renders; #3 CLOSE* — **✅ CLOSED 2026-08-03 (6/6 pts; §9 row 140 + PORT.md S140)**
- **Sprint Goal:** render gold #3's last missing element (the "Bob" pilot name box) by hosting the
  `CREdtBt` control type — flipping #3 PARTIAL → CLOSE.
- **Committed (~6 pts):** S140 host the 7th R\* control type (`CREdtBtCtrl`) + OCX compile-compat +
  regression.
- **Delivered:** BoBFrag's pilot slots `IDC_PILOT_0..14` are `CREdtBt` (edit-button), DDX-bound;
  `OnInitDialog` does `SetCaption(playerslotname)` and the player's slot is "Bob". Hosted the genuine
  `CREdtBtCtrl` (new `SRC/REDTBT/bob_ole_redtbt.cpp` + OCX `REDTBTC.CPP` in the build + `CLSID_REdtBt`
  wired) mirroring the REdit host, with two CREdtBt-specifics: caption is *stock* (`SetProperty(DISPID_CAPTION)`)
  → `InternalSetText` (compat `SetText` is a no-op); OnDraw's `captiontext` member is only refreshed
  in handlers → refresh from `InternalGetText()` in `draw()`. **Two OCX compile-compat fixes** (§8p
  class, BOB_LINUX-guarded): the `IconsUI` forward-decl underlying type (`:int`→`:unsigned int` to
  match uiicons.h; GCC rejects the mismatch MSVC ignored) + the `MaskIcon(CPoint&)` temp-bind (name
  the temp, cf. RRADIOC/RBUTTONC). The "Bob" box now renders = gold #3. **Gates (gl-lock):** safe
  default exit 0; mainmenu dummy==GL byte-identical; flight 94.9% non-black (CREdtBt only instantiates
  on BoBFrag → other screens unaffected).
- **#3 PARTIAL → CLOSE** (parity **17 CLOSE / 1 PARTIAL / 1 GAP**). With S136 (Return-to-Player) +
  S139 (Fly footer) + S135 (roster), every gold-#3 element now renders.
- **Increment demo:** `BOB_BOBFRAG=1 BOB_SHOT=120` → the briefing shows the "Bob" name box.

### Sprint 141 — "Choose the campaign phase (gold #18 → CLOSE)" → *Increment: the campaign phase is selectable; Eagle Attack's Directives allocation grid renders* — **✅ CLOSED 2026-08-08 (8/8 pts; §9 row 141 + PORT.md S141)**
- **Sprint Goal:** reach an active campaign phase headlessly so gold #18's allocation grid renders —
  the last non-by-design parity deviation, and the same state gap #19 names.
- **Committed (~8 pts):** S141.1 column-aware listbox Select + `#ID[:COL]` recipe token (3);
  S141.2 Eagle-Attack map + Directives capture & verdict (4); S141.3 gates/docs/cross-port (1).
- **Delivered — the grid was never a render gap.** BoB models a tab row as the **columns of one
  `CRListBoxCtrl`**; `CSCampaign::OnSelectRlistCampaigns(row, column)` picks the campaign phase from
  the **column**, and our hosted-listbox click passed a hardcoded `0` there while resolving the row
  faithfully. Every campaign the port had ever run therefore started in phase 0 (Convoys, 10 July —
  a standby day with nothing to allocate). Resolving the column through the genuine control's own
  `GetColFromX` (`colAtX` beside `rowAtY`; `BOB_NO_LIST_COL` reverts) makes the phase selectable:
  the select screen now reads **12th August – 23rd August** with the Eagle Attack narrative, and on
  that day **the game opens the Directives dialog itself** (S137's `BOB_MAP_DIRECTIVES` scaffold is
  no longer needed) with the full grid drawing through S137's deep TB_MISC walk — structurally 1:1
  with gold #18 down to the Missions column's 1/1/1/0/0/0.
- **Recipe grammar `#ID[:COL]` adopted from MA S62/S63** — the click point is resolved from the
  control's own drawn rect + column walk (`bob_ole_ctrl_point`), never fixed pixels.
- **#18 PARTIAL → CLOSE** (parity **18 CLOSE / 0 PARTIAL / 1 GAP**; the one GAP is #4, by design).
- **Honest, carried:** (a) the ~50 numeric spinner boxes don't draw — `CRSpinBut` is the 8th R\*
  type and the only unhosted one (follow-on story, §8p recipe); (b) **#19's raid-stack deviation is
  NOT retired** — that needs the map without the dialog over it, and `OpenDirectivetoggle` opens a
  *second* dialog rather than closing a game-opened one. Banked as its own story + a question to MA.
- **Gates (all `gl-lock`):** build clean; 14-recipe headless sweep 14/14 exit 0; **A/B on the same
  build (default vs `BOB_NO_LIST_COL=1`) 14/14 BYTE-IDENTICAL** — the shared click primitive is
  surgical; safe default (`BOB_NO_RUN`) exit 0; Eagle-Attack phase select **dummy==GL
  byte-identical**; flight frame-150 on `:0` **98.6% non-black**.
- **Increment demo:** `BOB_AUTOCLICK=1,1,#1000:1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=1100` → the Eagle
  Attack Directives grid over the strategic map, clock "12 August 10:45 x1".

### Sprint 142 — "Host CRSpinBut — the last R\* control type" → *Increment: the Directives grid's numbers render and match gold #18* — **✅ CLOSED 2026-08-08 (8/8 pts; §9 row 142 + PORT.md S142)**
- **Sprint Goal:** close the last named deviation on gold #18 by hosting `CRSpinBut` — the 8th and
  only remaining unhosted R\* control type (backlog SP.4).
- **Committed (~8 pts):** S142.1 host the control + build wiring (6); S142.2 capture, verdict,
  gates, docs (2).
- **Delivered — the R\* set is now COMPLETE (8/8 types hosted).** The LW Directives dialog is mostly
  *made of* this type — **85 DDX-bound spinners** — which is exactly why S141 could render its
  labels, headers and Missions column while every allocation number stayed blank. The dialog drives
  them through `CRSpinButExtra`'s chained `Clear()->MakeNumList()->SetIndex()`, which decomposes
  into dispids 7 / 5×N / setprop 2 — all routed by the host, so the path is right by construction.
- **Verified against gold number-for-number:** Morning 40, Mid-day 30; Reconn 0/1; the Ground-Attack
  matrix; Size per target "1 Gruppe" ×6; Missions 1/1/1/0/0/0; Escort 2/1, 1/0, 2/0; %Free 100 ×6;
  Resting 1/3/2/2 and 3/1 — every value agrees, with the spin-arrow art drawn.
- **Three traps:** the §8t `MaskIcon` temp-bind (patched as *bytes* after a text edit silently
  re-encoded three `£` literals — §8w); `CWnd::ReleaseCapture` **missing from compat** (the first
  hosted control ever to call it); and ⭐ **`m_bDrawing` is a STATIC flag cleared only inside
  `DrawBitmap`** — one draw taking the black-fill branch would have latched it and silently killed
  the other 84 spinners for the rest of the process. Neutralised host-side, no game-code edit.
- **Honest, booked not patched:** a NEW deviation this capture exposed — we draw a **"Sweeps" row
  and an "Escort 1:1" row that gold doesn't show**, Sweeps overprinting the section headers
  (template-membership / `ShowWindow` class → **SP.8**).
- **Inbound MA note 29 received + acted on** (commit `26a2a3f`): §2 unblocks SP.5 with the exact
  dismiss trigger, §1 re-scopes SP.6 to MA's structurally-better "drive the genuine `OnLButtonDown`"
  shape, §3 opens SP.7; shared-doc `§8v` collision resolved (BoB's → `§8w`, convention in `§8x`).
- **Gates (all `gl-lock`):** build clean; sweep 14/14 exit 0 and **14/14 BYTE-IDENTICAL vs the
  pre-S142 binary** (the new type only instantiates on LWDirectives, so every other screen must be
  untouched — and provably is); safe default (`BOB_NO_RUN`) exit 0; phase select **dummy==GL
  byte-identical**; flight frame-150 **98.7% non-black**.
- **Increment demo:** `BOB_AUTOCLICK=1,1,#1000:1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=900` → the Eagle
  Attack Directives grid with all its allocation numbers.

### Sprint 143 — "Capture what you claim, then see under the dialog" → *Increment: every parity capture records its own state; gold #19 re-diagnosed on evidence* — **✅ CLOSED 2026-08-08 (8/8 pts; §9 row 143 + PORT.md S143)**
- **Sprint Goal:** make parity captures self-describing (SP.9, from FF note 15), then use that to get
  the strategic map out from under the Directives dialog for gold #19 (SP.5, unblocked by MA note 29).
- **Committed (~8 pts):** S143.1 capture-state banner (3); S143.2 dismiss + #19 verdict (5).
- **Delivered (S143.1, in full):** every `BOB_SHOT` dump emits an always-on state line — where,
  tick, art, resolution, campaign phase, date/time, **`autoclick=<fired>/<total>`**, and per-dialog
  hosted/drawn control counts. The autoclick pair needed the drive-step counter made observable, so
  a recipe step that silently fails to fire can no longer look like a render bug.
- **The banner paid for itself three times in four runs:** (1) it killed **three** successive wrong
  theories about one behaviour — S141's "opens a second instance", S142's "index mismatch", and the
  implicit "it's one dialog" — by showing a two-dialog *stack* (`DIRECTIVERESULTS` 5 under
  `DIRECTIVES` 6), which also proved **S137 had captured DirectiveResults, not the grid**; (2) it
  exposed a **control leak** nobody was hunting (184 → 1656 hosted controls across re-opens → SP.10);
  (3) it made the #19 re-diagnosis possible.
- **Delivered (S143.2, mechanism yes / goal no — stated plainly):** the dismiss scaffold works per
  dialog and the day-start popup can be suppressed via the game's own toolbar toggle (driven as a
  genuine Clicked event, since the handler is `protected`). But the map still cannot be uncovered:
  Directives ⇄ DirectiveResults form a **closed loop on cancel**, measured oscillating 6→5→6→5.
  **That is faithful** — the LW player must *issue* orders, not escape them.
- **⭐ Which re-diagnoses gold #19.** Its long-standing deviation implied "reach an active day, let
  the clock run". False: the loop's exit is `OnOK`, and `DirectiveResults::OnOK` calls
  **`MakeLWPackages`** — the call that *creates the day's raids*. #19's raid stacks are downstream of
  completing the orders flow, not of the clock. **SP.11** names the fix; S141's framing is superseded.
- **Honest:** #19 is **not** closed and no raid-map capture was obtained. A plausible-but-wrong
  blocker was replaced with an evidenced one plus a named next step. Parity tally unchanged.
- **Gates (all `gl-lock`):** build clean; sweep 14/14 exit 0 and **14/14 BYTE-IDENTICAL against
  the pre-S142 binary** (spans S142+S143 — the banner is stderr-only and the scaffolds are
  default-off, so no rendered screen may change, and none does); safe default (`BOB_NO_RUN`)
  exit 0; phase select **dummy==GL byte-identical**; flight frame-150 **98.7% non-black**.
- **Increment demo:** any `BOB_SHOT` recipe now prints `[shot-state] …` before the dump;
  `BOB_MAP_NODIRECTIVES=30` suppresses the day-start popup.

### Sprint 144 — "Accept the orders flow" → *Increment: the title-bar OK works at all; the strategic map is capturable on an active campaign day* — **⚠️ CLOSED PARTIAL 2026-08-08 (6/8 pts; §9 row 144 + PORT.md S144)**
- **Sprint Goal:** complete the LW orders flow (SP.11) so gold #19's map — and its raids — can be
  captured; plus suppress the two rows gold doesn't show on #18 (SP.8).
- **Committed (~8 pts):** S144.1 drive the orders flow (5); S144.2 the #18 extra rows (3).
- **⭐ Delivered, and bigger than the story: `bob_evt_fire` matches `type_info` EXACTLY.** No
  base-class walk, and every call site passes the derived type — so **any `ON_EVENT` registered on a
  base class is dead.** That includes `ON_EVENT(RDialog, IDJ_TITLE, 3=OK, OnOK)` and its
  Cancel/Help siblings: the engine's **title-bar ✓ / ✕ / ?** buttons, on every dialog in the game.
  No dialog had ever been able to receive a title-bar OK. Invisible until now because every
  previously-wired event was registered on the same class that received it. Scaffold works around it
  by firing under `typeid(RDialog)` (the derived override still runs via the implicit virtual);
  general fix booked as **SP.12**, shared as **§8z**.
- **Delivered (S144.1):** the faithful exit from S143's cancel-loop works — `BOB_MAP_ACCEPTDIR`
  fires the genuine title-bar OK and the stack clears (`open-index=-1`). With S143's suppression,
  this produced **the first clear strategic-map capture on an active campaign day**.
- **NOT delivered, and the banner is why we know:** that capture is **not** a valid gold #19 oracle.
  It looks like a clean map; `[shot-state]` says `phase=0 date=1250121600` — the run had drifted to
  **15 August at x20**, three days past the 12 August a verdict would have claimed, phase field
  reset, no unit icons. Second sprint running that FF note 15's rule caught a verdict before it was
  made — and the first time it caught one of ours mid-flight.
- **#19's remaining conditions, now exact:** (a) **arm the capture from the drive** (SP.7) — an
  absolute `BOB_SHOT=<tick>` cannot be aimed at a state whose arrival time varies, which is precisely
  how this drifted; (b) the raid path needs `dr->dirresults[0].targets[0]`. **[S145 correction:** that
  array is *compacted* — `RefreshMissions` runs `k=0` through the recon line then i=1..7, each call
  returning the next `k` (LWDIRECT.CPP:628-643) — so index 0 is the **first allocated line**, not a
  fixed RECON slot, and `FillTargetLists` writes no `dirresults` at all. **[S146: that hypothesis was wrong too** — the trace shows
  the guard passing with real values (`13178 / 11766 / 13435` for Airfields/Docks/RDF).
  `dirresults` was fine; `LWDirectives::OnOK` was simply never called, because the OK went to the
  panel wrapper. Five readings of this path, five wrong.**]**
- **S144.2 (SP.8) — investigated, deliberately not started** (so 6/8, not a padded 8): the extra row
  is the fighter-**Sweeps** row (`IDC_FIGHTERSWEEP*`), **dead in the shipped game** —
  `LWDIRECT.CPP:1626` guards it with `INT3; //This should not happen. Patrols removed.` Gold is right
  to show nothing; the open question is only which mechanism drops it. Ids recorded on SP.8.
- **Also:** the SP.10 host leak reproduced twice more (184 → 368 → 1656).
- **Gates (all `gl-lock`):** sweep 14/14 exit 0 and **14/14 BYTE-IDENTICAL vs the pre-S142 binary**
  (now spanning S142+S143+S144); safe default (`BOB_NO_RUN`) exit 0; phase select **dummy==GL
  byte-identical**; flight frame-150 **98.7% non-black**.
- **Increment demo:** `BOB_AUTOCLICK=1,1,#1000:1,1,1 BOB_MAP_TIMER=8 BOB_MAP_NODIRECTIVES=25
  BOB_MAP_ACCEPTDIR=32 BOB_SHOT=1200` → the strategic map, no dialog over it.

### Sprint 145 — "Arm the capture, then accept" → *Increment: captures fire relative to the drive, so state drift is gone* — **⚠️ CLOSED PARTIAL 2026-08-08 (5/10 pts; §9 row 145 + PORT.md S145)**
- **Sprint Goal:** fix the capture-timing drift that blocked gold #19 (SP.7), then use it to build
  the day's raids and judge #19 (SP.11 cont.).
- **Committed (~10 pts):** S145.1 drive-armed capture (5); S145.2 raids + #19 verdict (5).
- **✅ Delivered (S145.1, SP.7):** `BOB_SHOT_AFTER=<n>` captures n paints after a scaffold calls
  `bob_shot_arm()` (first-arm-wins). **Proven on the first run:** `[shot] ARMED by
  accept-directives` → capture at `phase=1 date=1250035200`, the correct 12 Aug Eagle Attack state,
  where the identical recipe under absolute ticks had drifted to 15 Aug. From MA note 29 §3.
- **❌ Not delivered (S145.2) — decisive negative, not a stall.** `BOB_TRACE_DIR` in
  `LWDirectives::OnOK` printed **nothing** while the accept reported success and the dialog closed.
  **S144's workaround had been running the wrong handler.** Structural cause: the logged child is an
  RDialog **panel wrapper**, not the dialog — `LWDirectives::Make` returns
  `MakeTopDialog(..., DialBox(FIL_D_LWDIRECTIVES, new LWDirectives(dirres)))`. Firing OK at the
  panel ran `RDialog::OnOK` → `EndDialog`, so `MakeLWPackages` was never reached **and it looked
  like success**.
- **Two corrections issued, both to S144, both mine:** (1) **§8z in the shared doc** claimed firing
  under the base type "still reaches the derived override" — it does not; corrected and re-synced,
  since MA had been told to act on that note. (2) The raid-guard reading ("index 0 is the RECON slot
  `FillTargetLists` never fills") was wrong — `dirresults[]` is *compacted* by `RefreshMissions`.
- **#19 next step, from evidence:** drive the panel's `dial` (the `RowanDialog`), not the panel. The
  capture side is solved, so #19 is one correctly-addressed handler away.
- **Gates (all `gl-lock`):** sweep 14/14 exit 0 and **14/14 BYTE-IDENTICAL vs the pre-S142 binary**
  (now spanning S142→S145); safe default (`BOB_NO_RUN`) exit 0; phase select **dummy==GL
  byte-identical**; flight frame-150 **98.7% non-black**.
- **Increment demo:** `BOB_MAP_ACCEPTDIR=32 BOB_SHOT_AFTER=120` → capture lands in the intended
  state regardless of queueing or frame rate.

### Sprint 146 — "Reach the real dialog" → *Increment: the LW orders flow completes; the day's raids are built, fly and land* — **⚠️ CLOSED PARTIAL 2026-08-08 (5/8 pts; §9 row 146 + PORT.md S146)**
- **Sprint Goal:** drive the panel's contained dialog so `MakeLWPackages` runs and gold #19's raids
  exist (SP.11), and settle the #18 extra-row mechanism (SP.8).
- **Committed (~8 pts):** S146.1 reach the real dialog + raids (5); S146.2 the #18 rows (3).
- **⭐ Delivered (S146.1).** A trace named the object S145 could only describe: `RDialog::OnOK`
  reported `rtti=8RDEmptyD` — the logged child is an **empty placeholder panel**, the real dialog is
  its `fchild`. Descending one level produced the whole chain in one run:
  `child rtti=12LWDirectives -- firing OK` → `[dir] guard=13178` → `child rtti=16DirectiveResults`
  → `MakeLWPackages`.
- **Gold #19's deviation is substantively resolved: the raids EXIST.** Route lines across the
  Channel, raid markers, the game's Mission Folder listing **R001 / 36 / Dive Bomb / T/O 08:53 /
  ToT 09:59 / Tangmere AF**, and the footer log reporting **"Geschwader Landed [R002]", "[R005]",
  "Geschwader Landing [R005]"** — built, flown, returning. Banner confirms `phase=1
  date=1250035200` (12 Aug, Eagle Attack).
- **A fifth wrong reading retired by the same trace:** S145's "`RefreshMissions` never ran headlessly"
  was false — the guard passed with real values (`13178/11766/13435` = Airfields/Docks/RDF, matching
  the grid's 2/1/2). The guard was never the problem; `LWDirectives::OnOK` was never called.
  **Five readings of one control path, five wrong; the trace that settled it was twelve lines.**
- **#19 NOT marked CLOSE — no unobstructed capture yet.** Two dialogs covered the map in turn, each
  teaching something: the Mission Folder is logged on a **different toolbar** (so
  `MiscToolBar().LoggedChild()` truthfully said −1 while it covered half the screen — dismiss now
  sweeps all `TB_TOTAL` toolbars), and with the suppression flag omitted the Directives dialog
  re-armed before the shot. One composition of existing flags remains; left to the next sprint
  rather than claimed here.
- **SP.8 probe refutes my own guess:** `[tmpl] dlg=1032: 167 in template, 17 absent` with exactly
  `167/184` drawn — **the S124 filter works**; and **no** hosted control on that dialog has an id in
  the `IDC_FIGHTERSWEEP*` band. So the extra row is *in* the template, hidden another way. Next
  probe named on SP.8; no sixth guess.
- **Gates (all `gl-lock`):** sweep 14/14 exit 0 and **14/14 BYTE-IDENTICAL vs the pre-S142 binary**
  (spans S142→S146; every S146 change is a default-off scaffold or an env-gated trace); safe default
  (`BOB_NO_RUN`) exit 0; phase select **dummy==GL byte-identical**; flight frame-150 **98.7%**.
- **Increment demo:** `BOB_MAP_ACCEPTDIR=32 BOB_SHOT_AFTER=150` → the orders flow completes and the
  map carries live raids.

### Sprint 147 — "Guard the derefs, find why the shot keeps missing" → *Increment: 5 NULL derefs fixed; the capture-timing model shown to be wrong* — **⚠️ CLOSED PARTIAL 2026-08-08 (5/8 pts; §9 row 147 + PORT.md S147)**
- **Sprint Goal:** judge gold #19 with an unobstructed capture (2), fix the `WM_GETHINTBOX` derefs
  MA warned about (3), and identify #18's stray row (3).
- **✅ Delivered (SP.13):** swept **every** `SendMessage(WM_GETHINTBOX)` site rather than the two
  I'd read — RCOMBO 0/4 unguarded, RLISTBOX 1/4, **RBUTTON 4/4**. `CRButtonCtrl`, the control a
  title bar *is*, had no guarded site at all. Five derefs fixed with the codebase's own safe
  spelling; six-line diff. Note 19 to MA corrected — "one safe, one unsafe" understated it.
- **⭐ Delivered (unplanned): my own scaffold had MA's one-shot-static bug**, in a hook written
  *after* reading their warning. `bob_oob_close_dialogs` was `static int done`-guarded, so the
  dismiss fired once per process and every later dialog (Mission Folder, then a **"Take over? —
  Dover CH under attack"** intercept prompt) sailed past it — while I wrote it up as "the game keeps
  re-opening dialogs". Fixed; five sibling hooks booked as **SP.15**.
- **⭐⭐ And the fix exposed the limit of SP.7.** With prompts suppressed the campaign runs
  **faster**: the recipe that landed 12 Aug 07:52 in S146 landed **13 August** twice here, at both
  340 and 150 paints. **The scaffold changed the quantity the timing depended on.** Captures must be
  armed on a **game-state predicate**, not paints → **SP.16**, which is what #19 actually needs.
- **❌ Not delivered: #19 is still not judged, and I stopped rather than iterate.** Six attempts
  across three sprints, each yielding a real finding, none yielding a comparison frame. The raids
  demonstrably exist; a parity verdict is a claim about a comparison and I don't have one.
- **SP.8 probe widened** to dump every drawn control + rect (S146's id guess matched nothing); not
  yet run to conclusion.
- **Gates (all `gl-lock`):** sweep 14/14 exit 0 and **14/14 BYTE-IDENTICAL vs the pre-S142 binary**
  (spans S142→S147 — the five deref guards are unreachable on today's paths, and provably change
  no rendered screen); safe default (`BOB_NO_RUN`) exit 0; phase select **dummy==GL
  byte-identical**; flight frame-150 **98.7% non-black**.
- **Increment demo:** five latent NULL derefs gone; `BOB_MAP_CLOSEDLG` now re-arms.

### Sprint 148 — "Aim at a state, not a moment" → *Increment: a state-predicate capture trigger; gold #19 judged on a like-for-like frame* — **✅ CLOSED 2026-08-08 (7/7 pts; §9 row 148 + PORT.md S148)**
- **Sprint Goal:** build the capture trigger S147 proved was needed (SP.16), then use it to judge
  gold #19.
- **✅ SP.16 delivered and proven on first use.** `BOB_SHOT_WHEN=clear` captures the first map paint
  with **no logged child on any toolbar**, AND-able with `BOB_SHOT_DATE` and `BOB_SHOT_TIME_LT`.
  Seven counter-based attempts across S145–S147 missed; the first state-based one hit:
  `phase=1 date=1250035200` — a clean **12 August 20:58 x300** strategic map.
- **⭐ #19's verdict is now made on a like-for-like frame.** Since S123 it compared a *fresh-Convoys*
  capture against a *12-Aug Eagle-Attack* gold. Everything structural agrees on the new frame
  (terrain, sectors A-E/Y/Z, London, No.11 Group, city labels, full unit-icon layer, footer log,
  date-clock at **x300**, both toolbar rows, ruler) — CLOSE, finally earned rather than inherited.
- **The raid-stack difference is TIMING, measured:** gold 06:31 (raids outbound, routes + two raid
  stacks); our first *clear* paint 20:58 ("Geschwader Landed", routes gone). **The map is covered
  precisely while raids fly and clear precisely when they are not** — S146 captured ours *with*
  routes at 07:52 behind the Mission Folder, so the port renders them; no single frame has held
  both. `BOB_SHOT_TIME_LT` asks for the morning window; if no clear morning paint exists the target
  needs a dismiss-then-shoot primitive (named, not guessed).
- **Gates (all `gl-lock`):** sweep 14/14 exit 0; safe default exit 0; **dummy==GL byte-identical**;
  flight **98.7% non-black**. A/B vs pre-S142: 13/14 from the sweep, **14/14 after re-verification**.
- **⚠️ The 14th frame was my own process error, not code.** `config-control` differed in a 96x16
  band; two innocent explanations were available (live device state; the port's run-to-run variance
  class) and both would have been easy to write down. Measured instead: **three fresh runs on the
  final binary are identical to each other and to the pre-S142 baseline** — so the odd frame is the
  *sweep capture*, taken while I rebuilt twice with `gates148` queued, straddling binaries mid-sweep.
  **A gate whose inputs can change under it is not a gate** → **SP.19**.
- **Increment demo:** `BOB_SHOT_WHEN=clear BOB_SHOT_DATE=<day>` → a frame provably in the state the
  verdict claims.

### Sprint 149 — "Trust the gate, then audit the verdicts" → *Increment: a self-certifying gate suite; a third state-mismatched verdict fixed* — **✅ CLOSED 2026-08-08 (7/7 pts; §9 row 149 + PORT.md S149)**
- **Sprint Goal:** make the gate prove its own inputs held still (SP.19), then use it to audit the
  epic's foundation for capture-vs-gold state mismatches (SP.17).
- **✅ SP.19:** gate suite promoted to versioned `tools/bob_gates.sh` (the scratchpad+`sed` copy had
  failed twice), takes an output dir + optional baseline, does its own A/B, and **hashes the binary
  before and after — exit 2 with a loud banner if it moved.** First run: *"binary unchanged
  (md5=a8eccc5e…) — gate valid"*, 14/14 byte-identical.
- **✅ SP.17: a third verdict was resting on a state mismatch.** Gold #17 is **Eagle Attack, 12–23
  Aug**; recipe `1,1,1` never selected a phase, so we captured Convoys. With `#1000:1` it matches
  gold line for line. All three campaign rows (#16, #17, #19) shared one root — no phase token.
- **⭐ The finding is how they read before:** none was invisible. #17's row said "phase/date differ
  only by selected phase (state)". They were **recorded as deviations and reasoned past**, because
  *"that's just state"* sounds like a reason to stop looking. Now written into the parity doc:
  **a state difference the recipe CAN fix is a defect in the test; one it cannot is a finding about
  the port** — the doc had been merging the two.
- **Deliberately NOT claimed:** #6/#9/#10/#11's "combo values (settings state)" are the same shape
  but live in the installed build's saved settings, not a recipe token → **SP.20**, status
  *plausibly benign, not verified*.
- **Gates:** sweep 14/14 exit 0; safe default exit 0; dummy==GL byte-identical; flight **98.6%**;
  **A/B 14/14 vs pre-S142** (spans S142→S149); **gate self-certified valid**.
- **Increment demo:** `gl-lock tools/bob_gates.sh <out> <baseline>` → gates + A/B + a validity verdict.

### Sprint 150 — "Dump the geometry, don't guess the ids" → *Increment: gold #18's stray row removed; the dismiss hook de-duplicated* — **✅ CLOSED 2026-08-09 (6/8 pts; §9 row 150 + PORT.md S150)**
- **Sprint Goal:** identify and suppress #18's stray row (SP.8), sweep the scaffold logging (SP.18),
  and settle the four "settings state" deviations (SP.20).
- **✅ SP.8 — solved by geometry, after a guess had failed.** S146's `IDC_FIGHTERSWEEP*` guess
  matched **nothing**; the widened per-control dump put the six real rows at y=317..447 (12 controls
  each) and an extra **11-control row at y=229** whose ids are the `...7` members of the same
  families — index 7 of `for (i=1;i<8;i++)`, i.e. **SWEEPSNDECOYS**, the branch the game marks dead
  (`INT3; //Patrols removed.`). **The feature was cut; its controls stayed in the BDG template.**
  Suppressed by measured id; the section headers read cleanly again. `BOB_NO_SWEEPROW_SKIP` reverts.
- **Residual booked, NOT assumed to share the cause (SP.21):** the "Sweeps" label and the
  "Escort 1:1" row remain — but `IDC_ESCORT_PROPORTION` (1680) is **live code**, so gold hiding it
  needs a different explanation than "cut feature".
- **✅ SP.18 — found a redundancy, not just noise.** All 16 scaffold hooks audited; exactly one
  repeatable hook logs (`bob_oob_close_dialogs`), and S148 had silenced only its first loop. The
  tail turned out to be **superseded** by S146's all-toolbar sweep — the stack was walked twice and
  the dismiss policy had two homes. Deleted rather than quietened.
- **◐ SP.20 — instrumented, deliberately not concluded.** `SAVEGAME/settings.cfg` exists (1976 B)
  and `InitPreferences` reads it, falling back to factory defaults when `successfulLoad` is false.
  Parses → the four rows' differences are genuine user state (out of scope); fails → we show
  defaults against gold's saved values and the deviation is **ours**. `BOB_TRACE_PREFS` prints it;
  the measurement was still queued on the shared display at sprint close, so the story carries with
  the instrument in place rather than a guess written down.
- **◐ SP.20 measured: `[prefs] exists=1 … successfulLoad=0`** — the port finds the install's
  `settings.cfg` and **fails to parse it**, falling back to factory defaults. Real defect, fixed in
  S151. **[S151 CORRECTION: this entry originally went on to conclude "gold parsed the same file, so
  #6/#9/#10/#11's differences are ours" and reclassified four rows. That was wrong** — the file's
  own stamp says `Jul 19 2026`, i.e. **our port wrote it**, and the gold shots (2026-06-24) predate
  it; and with the load fixed those four screens are **byte-identical** to the factory-default
  captures. Cause unknown again; SP.20 reopens.**]** Booked **SP.22** (8 pts).
- **Gates (all `gl-lock`, via `tools/bob_gates.sh`):** sweep 14/14 exit 0; safe default exit 0;
  **dummy==GL byte-identical**; flight **98.6% non-black**; **A/B 14/14 byte-identical vs pre-S142**
  (correctly so — the sweep-row suppression is scoped to dlg 1032, which no swept recipe renders);
  **binary hash unchanged — gate valid**.
- **Increment demo:** `BOB_AUTOCLICK=1,1,#1000:1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=1100` → the Directives
  grid with clean section headers.

### Sprint 151 — "Settings survive a rebuild; and a retraction" → *Increment: `settings.cfg` loads again; S150's conclusion about four parity rows withdrawn* — **✅ CLOSED 2026-08-09 (5/8 pts; §9 row 151 + PORT.md S151)**
- **Sprint Goal:** make `settings.cfg` parse (SP.22) and confirm what that does to #6/#9/#10/#11.
- **✅ SP.22 root cause, measured:** `SaveData`'s deserialiser gates on `strcmp(date,date2)==0`
  where **both are `"Rowan Savegame: " __DATE__`** — settings load only if written by a binary
  compiled the **same calendar day**. The original's own design (preferences die on every rebuild);
  its authors never felt it because they didn't rebuild daily. **We do** — so every `ninja bob`
  silently discarded the player's preferences, and `SavePreferences()` wrote a file the next run
  rejected.
- **The fix was already in the file, for savegames.** `operator>>(BIStream&, Campaign&)` has a
  `BOB_LINUX` block — *"the version string embeds the build `__DATE__` … load anyway — the binary
  save format is what matters (/Zp1-packed, our -fpack-struct read matches)"*. Never applied to
  `SaveData`. Applied now with the same reasoning; `BOB_NO_PREFS_LOAD` reverts. Verified on four
  config screens: loads, exit 0, no garbage.
- **⭐ RETRACTION, and it matters more than the fix.** S150 measured `successfulLoad=0` and concluded
  gold had parsed the same file, so #6/#9/#10/#11's differences were ours — reclassifying four rows.
  **Both unchecked assumptions were false:** the file's stamp reads `Jul 19 2026`, so **our own port
  wrote it** and the 2026-06-24 gold shots never saw it; and with the load fixed those screens are
  **byte-identical** to the factory-default captures, so loading changes no pixel. Rows reverted to
  **cause unknown**; SP.20 reopens; PORT.md + parity doc corrected in place.
- **The rule this cost: a measurement licenses only the claim it measures.** `successfulLoad=0`
  proves *we* don't load the file — nothing about what gold did. Two cheap checks (the file's own
  date stamp; a pixel diff) refuted the story built on it.
- **Gates (`tools/bob_gates.sh`):** sweep 14/14 exit 0; safe default exit 0; **dummy==GL
  byte-identical**; flight **98.6% non-black**; **A/B 14/14 byte-identical vs pre-S142**; binary hash
  unchanged — gate valid. **That 14/14 is corroboration, not a formality:** settings now load, and
  nothing renders differently — exactly what the pixel diff predicted, because the saved file holds
  default values. Had the struct layout been wrong, this is where garbage would have shown up.
- **Increment demo:** `BOB_TRACE_PREFS=1 BOB_CONFIGSCREEN=gfx2` → *"version string differs … loading
  anyway"*, settings applied instead of discarded.

### Sprint 153 — "Prune the host table" → *Increment: the leak's real root cause found — the port never destroys a dialog* — **⚠️ CLOSED PARTIAL 2026-08-09 (4/8 pts; §9 row 153 + PORT.md S153)**
- **Sprint Goal:** stop the hosted-control leak (SP.10) and clear #18's residual controls (SP.21).
- **◐ SP.10 re-scoped by measurement.** Wrote `bob_ole_release_dialog()` + a `DestroyPanel` hook —
  then measured **0 releases** on a run that reached **181,424** hosted controls. The hook is right
  and unreachable: compat's `CDialog::OnCancel`/`EndDialog` are **no-ops**, so closing a logged
  child destroys nothing and **the port has no dialog teardown at all**. Every re-open allocates a
  fresh dialog plus ~184 hosts and keeps both.
- **Half of it was already recorded:** S108 documented the same no-op while fixing a stack overflow
  and moved on. Nobody asked what *else* "never destroys the window" implies. **A note that explains
  one symptom is not the same as a note that scopes the defect.**
- **→ SP.23 (8 pts)** with the delicate part flagged: S108's re-entrancy guard depends on the slot
  NOT clearing, so teardown must preserve or replace it. Hook left in place, documented dormant.
- **SP.21 not started** (budget went to the root-cause hunt); carries.
- **Gates:** sweep 14/14 exit 0; safe default exit 0; dummy==GL byte-identical; flight 98.6%;
  **A/B 14/14 byte-identical vs pre-S142**; binary hash unchanged — valid.

### Sprint 154 — "Implement dialog teardown" → *Increment: the missing teardown located — a `{ return TRUE; }` stub — hooked default-off* — **⚠️ CLOSED PARTIAL 2026-08-09 (4/8 pts; §9 row 154 + PORT.md S154)**
- **Sprint Goal:** implement the dialog teardown S153 proved absent (SP.23).
- **⭐ Root cause found: `CWnd::DestroyWindow() { return TRUE; }`.** The chain runs correctly all the
  way (`OnCancel` → `RDialog::OnCancel` → `EndDialog` → walks children → `DestroyWindow`) and the
  last call lies. The `{ return TRUE; }` class the port's own notes say to grep for.
- **Corrects a misdirecting note:** S108 named `CDialog::OnCancel` as the no-op. `Rowan::CDialog`
  adds only a constructor, so that resolves to `RDialog::OnCancel`, which works. **A note with the
  wrong location reads as a closed question** — this one closed it for ~45 sprints.
- **Honest: hooked, not fixed.** Three wrong hook sites before this, each caught by measurement in
  one run (`DestroyPanel` never reached; compat `::CDialog` wrong class; `DestroyWindow` fires but
  releases **1 control, not 184** — it reaches the panel, the hosts belong to the *contained*
  dialog). Next step named (walk descendants; needs a safe `CWnd*`→`RDialog*` test) and
  **deliberately not guessed at a fourth time**.
- **Default-off** behind `BOB_DLG_TEARDOWN`; **gates 14/14 byte-identical**, so the partial state
  ships safely.
- **Design note for the finisher:** release hosts + clear the slot, but do **not** free the dialog
  object — ownership varies and freeing the wrong one is worse than retaining it.

### Sprint 155 — "Trace, don't guess" → *Increment: dialog teardown lands; 181,424 hosted controls → 184, default-on* — **✅ CLOSED 2026-08-09 (8/8 pts; §9 row 155 + PORT.md S155)**
- **Sprint Goal:** finish SP.23 by instrumenting the destroy call graph instead of guessing a fifth
  hook site.
- **✅ One trace settled it.** `BOB_TRACE_DESTROY` printed, per `DestroyWindow`: object RTTI, hosts
  naming it as parent, and the same per descendant. Result: `RDEmptyD hosts_here=0` →
  `child LWDirectives hosts=184`. **DestroyWindow gets the PANEL; the controls belong to the dialog
  inside it.** Uniform across every dialog (DirectiveResults 3, LWMissionFolder 23, TakeOverOffered
  6 **×4 instances in one campaign day** — the leak in ordinary play). The trace also proved the
  `dynamic_cast` guard sound.
- **✅ Fix:** release the destroyed node **and** its `fchild`/`sibling` descendants, bounded.
  **181,424 → 184 hosted controls** (exactly one dialog's worth — no accumulation), tracked dialogs
  12 → 6, same end state.
- **✅ Flipped DEFAULT-ON on evidence:** the measurement; the campaign cancel-toggle path (S108's
  stack-overflow territory) exercised without incident; and **a full gate suite run with teardown
  enabled: 14/14 byte-identical**. `BOB_NO_DLG_TEARDOWN` reverts.
- **Deliberately still not freeing the dialog object** — ownership varies; freeing the wrong one is
  worse than retaining it. Reclaims control storage + removes the use-after-free hazard.
- **Banked §8-BoB155 (synced to MA):** the panel-wrapper has now cost two unrelated 3-sprint hunts
  (S144-146 driving the handler, S153-155 releasing resources) and **both failure modes report
  success**. Rule: print `typeid(*p).name()` before driving or destroying.
- **Gates, twice:** with teardown enabled *and* on the shipped default — both **14/14
  byte-identical**, safe default exit 0, dummy==GL byte-identical, flight 98.6%, binary hash
  unchanged, gate valid.

### Sprint 158 — "The message map is real" → *Increment: 16 dead WM_* routes dispatch (default-off); blocked on §8-MA84 file-block pairing* — **⚠️ PARTIAL 6/8 CLOSED 2026-08-09 (§9 row 158 + PORT.md S158)**
- **Sprint Goal:** implement the compat message dispatch found dead in S157 so the 16 routes reach the handlers that already exist.
- **✅ Mechanism lands.** `DECLARE_MESSAGE_MAP` declares a per-class static member, `BEGIN_MESSAGE_MAP` defines it, `ON_MESSAGE` registers a thunk onto the game's own `MSG2_*` adapter (which already normalises 0/1/2-arg, void/non-void handlers — the Windows branch of that macro does the same). All 16 `WM_USER` routes + `WM_COMMANDHELP` register; 86 handler rows, 148 probes.
- **✅ Base-class walk demonstrated, not asserted:** `DISPATCHED 0x402 → CRToolBar (depth 1)`. That is the §8z failure mode avoided. `WM_GETARTWORK → 27922` matches the banner's `fe_art=27922` — a real art FileNum.
- **✅ Prediction → measurement → fix, in that order.** §7d predicted derived dialogs would miss (declared map base `CDialog` vs real base `RowanDialog`, no rows of their own). Census confirmed it (same ids both dispatched and unhandled). *Then* the `dynamic_cast` probe fallback was implemented → **`unhandled=0`**.
- **⚠️ Blocked: §8-MA84.** With all routes live the run dies — `FILEMAN.CPP: Opened file block (6d12) again without closing!` — MA's documented hazard, hit from the other side. The `WM_GETFILE`/`WM_RELEASELASTFILE` protocol has **never run** in this port, so its bookkeeping has never had to balance. Hypothesis (not concluded): the probe scan picks the *first* matching class and `MSG2_*` are non-virtual, so a derived dialog can get `RDialog`'s handler where an override exists. → **S159**.
- **✅ Nothing ships broken:** dispatch default-off (`BOB_MSG_DISPATCH`); **gates on the shipped default 14/14 byte-identical**, safe default exit 0, dummy==GL byte-identical, flight 98.7%, binary hash unchanged.
- **⭐ Would have shipped as a silent success.** The first implementation compiled, defined 146 register functions and registered **nothing** — `ON_MESSAGE` is stubbed in **two** layers and `GLOBDEFS.H` wins. Caught by counting call sites *before* testing behaviour (`chain` 296 / `add` **0**). Checking for the registrar objects instead would have misled — GCC inlines their ctors into `_GLOBAL__sub_I`, so `nm | grep runinst_` reads zero even when it works.
- **Cascade worth noting for estimation:** each un-stubbing surfaced the next hidden gap — 2 missing `CString operator+` overloads (call sites constant-folded away), an undeclared `WM_COMMANDHELP`, then the file-block protocol. **A stub returning a compile-time constant suppresses the very link errors that would reveal the rest of the missing implementation.**

### Sprint 157 — "What actually drives it?" → *Increment: a click driven from a real SDL event, end to end; the message dispatcher found to be an allowlist of 3* — **✅ CLOSED 2026-08-09 (8/8 pts; §9 row 157 + PORT.md S157)**
- **Sprint Goal:** cash S156's retro — audit every capability's drivers, classify **shallow**
  (substitutes an input → proves the real path) vs **deep** (substitutes a call → proves nothing
  above it), and close the biggest gap. Audit lives in `doc/scaffold-audit.md`.
- **✅ Found: layer (1) of the click path had never been executed by any test.** Every existing
  driver (`BOB_CLICKXY`, `BOB_AUTOCLICK`, `BOB_MAP_CLICK`) enters at `bob_gdi_get_click` or later, so
  the SDL event handler + its logical→drawable scaling were never run.
- **✅ Built `BOB_SDL_CLICK` / `BOB_MAP_SDLCLICK`** — push a **real `SDL_MOUSEBUTTONDOWN`** into the
  queue (`bob_sdl_push_click()`, framebuffer→logical, inverse of the handler's own scaling); the
  normal poll loop consumes it. Map trigger reuses the settled-after-6-paints **state** rule, not a
  tick count (S148).
- **✅ The negative result explained 40 sprints of harness design.** Headless it never arrived;
  instrumenting `pump_events` at calls #0/#100/#10000 printed **nothing** — under
  `SDL_VIDEODRIVER=dummy` `SDL_CreateWindow` fails, no window exists, and the pump is never called.
  The headless harness **cannot** reach layer (1); every driver entering below it was a necessity.
  Explicitly **not** claimed: anything about whether real mouse input works.
- **✅ Proven on real GL, whole chain, one run:** `pump_events #0` → `SDL event POLLED (712,499)` →
  `bob_gdi_get_click CONSUMED` → `oobclick … ctrl id=2600` (**`IDC_RBUTTONREST`**). Nothing bypassed
  but the physical mouse — and it re-proves S156's fix via the real path, not injection.
- **✅ Second finding — `CWnd::SendMessageA` is an allowlist of three** (`WM_GETFILE`,
  `WM_GETGLOBALFONT`, `WM_GETSTRING`) and returns 0 for everything else; `SendMessageToDescendants`
  is `{}`; `ON_MESSAGE` expands to nothing. MA's §8-MA83 class. The game sends **20** `WM_*` types;
  `BOB_TRACE_MSG` (deduped, per S142's 70 MB lesson) caught **4 firing in one ordinary run**:
  `WM_GETARTWORK`, `WM_GETXYOFFSET`, `WM_RELEASELASTFILE`, `WM_GETX2FLAG`. Corroborated by the port
  already hand-delivering two of these routes at individual call sites (MAINFRM.CPP:1417,
  FULLPSYS.CPP:1198) — two local workarounds, one missing subsystem.
- **Deliberately NOT claimed:** that any specific parity/backlog symptom is *caused* by the dead
  dispatcher. `WM_SELECTTAB` (4 real handlers, prime suspect for SP.6 / gold #16's tab highlight) did
  not fire in this recipe → dead-by-inspection only. S151's rule.
- **Method note banked:** `grep -o "WM_[A-Z_]*"` excludes digits and silently truncated
  `WM_GETX2FLAG` to a plausible `WM_GETX`; the runtime census caught it. §8k(3)/§8m(2)'s class.

### Sprint 156 — "The dialogs were never clickable" → *Increment: OOB map dialogs accept real clicks; Rest zeroes the directive grid* — **✅ CLOSED 2026-08-09 (8/8 pts; §9 row 156 + PORT.md S156)**
- **Sprint Goal:** answer SP.14 / MA note 31 §3 — do BoB's map OOB dialogs accept genuine clicks?
  **Answer: they did not, and had not since S113.**
- **✅ Found by reading the dispatch, not by a failing test.** `bob_frontend_tick`'s map click path
  was `bob_map_click_toolbars(...)` → `bob_map_select(...)`: toolbar buttons, then unit selection,
  **with no branch into an open dialog**. Every OOB panel built across S113–S155 was *render-only*.
  Nothing failed, because the only thing that ever drove those dialogs was
  `bob_oob_accept_directives`/`bob_oob_close_dialogs` — which call **`bob_evt_fire` directly**,
  skipping hit-testing and dispatch. MA note 31 §3's heuristic — *a capability only ever exercised
  through scaffolding is evidence the real path is missing* — is what prompted the look.
- **⚠️ Corrected mid-sprint (see §10):** the first version of this entry blamed
  `BOB_AUTOCLICK`/`bob_ole_ctrl_point`. Both are **shallow** scaffolds — they synthesize a
  *coordinate* and fall into the same dispatch a real click uses — and they act on front-end panels,
  not map OOB dialogs. The distinction (**shallow = substitutes an input, proves the real path;
  deep = substitutes a call, proves nothing above it**) is the actual lesson and is what §8-BoB156
  now carries.
- **✅ Fix (`bob_map_click_oob`, MAINFRM.CPP):** an open dialog gets **first refusal**; walks each
  toolbar's logged children *and* descendants (§8-BoB155 — controls live on the contained dialog);
  swallows in-dialog misses so a click on dialog background cannot select a unit behind it.
  `BOB_NO_OOB_CLICK` reverts. Hit rects are the hosts' own last-drawn screen rects, so they cannot
  drift from what was painted (the drift MA had to engineer around).
- **✅ Proven end-to-end with a noise-floor-controlled A/B**, one binary (`d9fcef96…`, hash checked
  before *and* after both runs):
  | run | result |
  |---|---|
  | click (712,499) | `[oobclick] consumed by toolbar 3 child 6 ctrl id=2600` = **`IDC_RBUTTONREST`** |
  | click vs no-click | **4,742 px differ** |
  | no-click vs no-click (**noise floor**) | differs only in a **16×8 box** at (649–665, 248–256) — a clock field |
  | signal outside the noise box | **4,666 px** — grid rows y320/346/372, the button y493–505, footer y649–767 (**0 noise px in every one**) |
- **✅ The handler genuinely ran** — not a repaint: before/after crops show the directive grid going
  from `1 / 0 / 1` with yellow totals `1 1 1` to **all zeros in every column and total**. Resting all
  squadrons clears their allocations, and all 85 spinners re-render the new data.
- **✅ Latent defect found by self-review, not by symptom.** The new call linked only because
  `_MFC.CPP` includes `MainFrm.cpp` (line 79) before `fullpsys.cpp` (line 105) — same unity TU, so an
  in-body `extern int` inherited C linkage per `[dcl.link]/6`. Correct by include order, not by
  construction; reordering or splitting the unity build would break it with an undefined reference
  pointing nowhere near the cause. Now declared `extern "C"` at file scope. **Banked §8-BoB156b**
  (MiG Alley has the identical unity layout).
- **Process:** three self-inflicted measurement faults this sprint, all one family — *plumbing that
  didn't reach the thing being measured*: `2>/dev/null` swallowing the trace being grepped; a control
  run launched from the wrong cwd (`Can't find ROOTS.DIR`, dead before init); and a first diff taken
  against an **S150-era binary**, which would have bundled six sprints of change into "the click did
  this". See §10.

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

> ⚠️ **This table stops at 158 and the sprint sections in §7 stop earlier still.** Sprints
> **159–205 are recorded in `PORT.md`** (newest first), which `CLAUDE.md` names as the running
> engineering log; the S206 row below is added on top rather than backfilling 47 rows from memory,
> which is exactly the kind of reconstruction §8-MA/BoB notes keep warning about. Read `PORT.md`
> for anything after 158. Backfilling this table from PORT.md is a real, schedulable chore.

| Sprint | Committed pts | Done pts | Increment shipped? | Notes |
|---|---|---|---|---|
| **208** | ~5 | 5 | ✅ **The same stub MA lost its replay feature to — we have it too, and measured why it cannot bite yet** | **(2026-08-25)** Answering **§8-MA139**. MA lost its **entire** replay feature to `static inline BOOL SetEndOfFile(HANDLE h) { (void)h; return TRUE; }`: `OpenRecordLog` opens the record file `OPEN_ALWAYS` and empties it through that call, so with the stub the file accumulated every flight ever flown and playback — which starts at the **first** block — read a stale `numframes=0` and could never advance. **We have the identical stub (`compat_winbase.h:550`) and the identical call site (`COMMS/REPLAY.CPP:146`, same comment).** But *"the same code exists"* is not a verdict: `BOB_TRACE_RECLOG` printed **0** lines over a full GATE 5 campaign flight to the Bf 109 cockpit, and there is **no `replay.dat` anywhere under `drive_c`** — two independent signals that `OpenRecordLog` is never reached, with the instrument provably on the claim's path (§8-MA138). ⭐ **Precisely why:** every `StartRecordFlag=TRUE` in `TRANSITE.CPP` is gated on `Save_Data.gamedifficulty[GD_GUNCAMERAONTRIGGER]` **and** on the player firing — an opt-in option plus a trigger pull, and no gate shoots. **The feature is switched off, not broken** (MA's S90 rule arriving back in the other port). **Fixed anyway** with the real Win32 semantic (`ftruncate` at the current pointer; `BOB_NO_TRUNCATE=1` reverts) — *latency is a reason to fix a success-reporting stub, not to skip it*: one returning FALSE is found the first time the feature runs, one returning TRUE waits and then fails years later as "the replay doesn't work". **Shipped deliberately without a gate** — nothing to assert until something records, and asserting a property the port has never had is asserting a wish. Also checked and unaffected: `FULLPANE.CPP:2548`'s `OPEN_ALWAYS` never calls `SetEndOfFile`. **Known gap logged: BoB's entire replay subsystem is unexecuted code**, same class as the ACM tree (S201). Gates: **GATE 5 PASS 9/9**, safe default exit 0; no behaviour changed. |
| **207** | ~8 | 8 | ★ **Row 7 of the mission briefing's roster could not be clicked — answering MA's §8-MA137 the day it arrived** | **(2026-08-24)** MA asked whether we bound a hit test by a control's rect while its paint uses its own metric. **We do.** Measured: `IDD_BOBFRAG`/`IDC_RLIST_UNITDETAILS`, the briefing's unit-details roster, rect 673x139, `contentH=162` — *rows 0..6 fit, content has 0..7, **row 7 UNCLICKABLE***. ⚠️ **Nearly filed N/A by reading:** S173 clips each control's draw to its rect, so paint and hit test "agree by construction" — but that clip is on the `SetDIBitsToDevice` **art** path and does not bound row **text**. The tidy N/A argument was about a different code path than the question; a ten-minute probe (`OleHost::contentH()` vs the hosted rect, printed only on overflow — filtered, not capped) settled it. **Both halves were broken and only one is obvious:** `bob_ole_click` refused the click, and `bob_ole_ctrl_point_rc`'s row scan — bounded by `sh` — hit S199's refusal branch, so the row was unreachable by a player **and unnameable by a test**, which is exactly why nothing had ever failed over it. Mirror image of MA, whose resolver produced the right coordinate and whose hit test threw it away. Fixed with `OleHost::hitH` (the height paint covered, from the control's own `GetListHeight()`), used by both; **deliberately separate from `sh`**, which also builds the dialog's *swallow* region — widening that changes which clicks the map stops seeing, a far larger blast radius than making a drawn row answer. A/B on one binary: `hit-tested to 162` vs `BOB_NO_DRAWH=1` → `139`. **Also: report a defect in the units the player feels it in** — the probe first said "23px of rows painted outside the rect", which nobody can act on; it now names the row. Third inbound MA note in a row that looked N/A and paid. |
| **206** | ~8 | 8 | ★ **The raid was never stuck — the tape ran out. S204 withdrawn; the campaign's strategic layer runs end to end, and GATE 7 locks it in** | **(2026-08-24)** S204 asked *why does a `PS_FORMING` LW squadron never execute a waypoint?* New `BOB_TRACE_FOLLOWWP=<n>` prints the only three things that can hold a squadron there — escortee / no waypoint / never arrives — per squadron, filtered to LW packs. **None of them.** `d2wp` shrinks 2,538,720 → 1,558,264 cm in nine frames at ~110,000 cm/frame, exactly `moved`: the raid is navigating correctly. The finding was in the **call counter** — `call=10` after 420 s, against the ~23 frames it needs. **Mechanism:** the campaign clock is driven from the map PAINT and does not run in 3D flight (guarded on `!InThe3D`), so the strategic sim crawls at ~real time; every soak set `BOB_CAMPAIGN_FLY` and spent its wall clock in the cockpit. **Same binary, recipe the only variable:** flight → 0 waypoint executions / status 14 / no `SetRAFIntercept` / 0 interceptor packages; map-only → **37 / status 19 (`PS_OUTGOING`) / `SetRAFIntercept` reached for the first time in this port's life / 52 AM_INTERCEPT sightings**. The raid flies BombRendezvous → DogLeg → IP → target → egress, 656 → 15,000 ft; the RAF creates interceptor packages that reach `PS_ENEMYSIGHTED`. **The strategic layer of the campaign works.** Aircraft-level `AUTO_COMBAT` stays 0 and that is *correct* — those codes belong to expanded SAGs near the player, and a map-only run has none; whether the ACM tree works is still open → **S207**. **GATE 7** (`tools/bob_strategic_soak.sh`) asserts the five properties + crash-freedom, wired into `bob_gates.sh`; `STARVE=1` re-runs S204's own recipe and must go red, reproducing its numbers exactly. ⚠️ **The first control was WRONG and is recorded, not deleted:** it starved the run via the fast-forward multiplier and the gate **passed anyway** — refuting a mechanism already written into the gate header, before publication. ⚠️ **False PASS found while wiring GATE 7 in:** `bob_gates.sh` read GATE 5's status two commands and one `if` later, so `${PIPESTATUS[0]}` was that `if`'s echo — **`campaign: PASS` printed unconditionally** and GATE 5 never contributed to `gates_fail`. Fixed. ⭐ **And with that fixed GATE 5 was RED, and had been since `e186da2` (S195) created it:** its "Convoys phase" assertion greps for `phase=0`, a field printed only when a `BOB_SHOT` capture fires, while the gate's own recipe sets `BOB_SHOT=99999` — **unpassable on the day it was written**, hidden by the false PASS ever since, so nine sprints of "the campaign is gated" rested on eight assertions. Re-keyed to `[campphase] whichcamp=%d`, emitted unconditionally in `FULLPANE.CPP` where the choice commits. Predicted the campaign itself was fine; measured `whichcamp=0`, GATE 5 **9/9 PASS**. **No game behaviour changed:** the diff is env-gated instrumentation, one new gate, one gate-runner fix, one re-keyed assertion, and the retraction of a wrong finding from three documents asserting it as fact. Cross-port **§8-BoB206**. |
| **158** | ~8 | 6 | ⚠️ **PARTIAL — the message map is REAL (16 dead routes dispatch, default-off); blocked on §8-MA84 file-block pairing** | **(2026-08-09)** S157 found `SendMessageA` an allowlist of 3 with `ON_MESSAGE` expanding to nothing. Implemented: per-class static registrar (`DECLARE_MESSAGE_MAP` declares, `BEGIN_MESSAGE_MAP` defines, `ON_MESSAGE` registers a thunk onto the game's own `MSG2_*` adapter — the same thing the Windows branch of that macro does). All 16 `WM_USER` routes + `WM_COMMANDHELP` registered. **Base walk demonstrated:** `0x402 → CRToolBar (depth 1)`, i.e. a base-registered handler reached from a derived object — the §8z trap avoided *and shown*; `WM_GETARTWORK → 27922` matches the banner's `fe_art`. **§7d prediction confirmed then fixed in that order:** declared map bases aren't the real bases (`LWDirectives` declares `CDialog`, derives `RowanDialog`, 0 rows of its own) → census showed the same ids dispatched *and* unhandled → `dynamic_cast` probe fallback added → **`unhandled=0`**. ⚠️ **Then §8-MA84:** `FILEMAN.CPP: Opened file block (6d12) again without closing!` — the `WM_GETFILE`/`WM_RELEASELASTFILE` protocol has never run in this port, so its bookkeeping never had to balance; hypothesis is the probe scan picking the first match over a non-virtual override → S159. ⭐ **Nearly a silent success:** first attempt compiled, defined 146 registrars, registered **nothing** (`ON_MESSAGE` stubbed in TWO layers, `GLOBDEFS.H` wins) — caught by counting call sites before testing behaviour (`chain` 296 / `add` 0); checking the registrar *objects* would have misled (GCC inlines their ctors into `_GLOBAL__sub_I`). **Cascade:** each un-stubbing exposed the next gap — 2 missing `CString operator+` (call sites constant-folded away), undeclared `WM_COMMANDHELP`, then the file protocol. Default-off; **gates 14/14 byte-identical, flight 98.7%, hash valid**. |
| **157** | ~8 | 8 | ★ **A click driven from a REAL SDL event, end to end — and the message dispatcher is an allowlist of 3** | **(2026-08-09)** Cashed S156's retro by auditing what actually drives each capability (`doc/scaffold-audit.md`), splitting scaffolds **shallow** (substitutes an *input* → everything downstream is production code) vs **deep** (substitutes a *call* → proves nothing above it). **Finding 1:** every existing click driver (`BOB_CLICKXY`, `BOB_AUTOCLICK`, `BOB_MAP_CLICK`) enters at `bob_gdi_get_click` or later, so **layer (1) — the SDL event handler + logical→drawable scaling — had never been executed by any test**. Built `BOB_SDL_CLICK`/`BOB_MAP_SDLCLICK` to push a **real `SDL_MOUSEBUTTONDOWN`** (state-triggered, not tick-counted, per S148). **The negative result explained 40 sprints of harness design:** headless it never arrived, and a trace on `pump_events` calls #0/#100/#10000 printed **nothing** — under `SDL_VIDEODRIVER=dummy` `SDL_CreateWindow` fails, no window exists, the pump never runs. The headless harness *cannot* reach layer (1); entering below it was a necessity, not an oversight. Explicitly **not** claimed: anything about whether real mouse input works. **On real GL the whole chain ran in one go:** `pump_events #0` → `SDL event POLLED (712,499)` → `get_click CONSUMED` → `oobclick … id=2600` (`IDC_RBUTTONREST`), and the resulting **grid crop is byte-identical to S156's injected click** while differing from no-click — same game-state change via two independent entry points. **Finding 2:** `CWnd::SendMessageA` handles **3** message ids and returns 0 for the rest; `SendMessageToDescendants` is `{}`; `ON_MESSAGE` expands to nothing (MA §8-MA83 class). Game sends **20** `WM_*` types; `BOB_TRACE_MSG` (deduped — S142's 70 MB lesson) caught **4 firing in one ordinary run**: `WM_GETARTWORK`, `WM_GETXYOFFSET`, `WM_RELEASELASTFILE`, `WM_GETX2FLAG`. Corroborated by the port already hand-delivering two of those routes at single call sites (MAINFRM:1417, FULLPSYS:1198) — two workarounds, one missing subsystem. **Deliberately not claimed:** that any specific parity symptom is *caused* by it; `WM_SELECTTAB` (4 real handlers, prime SP.6/#16 suspect) didn't fire in this recipe → dead-by-inspection only. **Method note:** `grep -o "WM_[A-Z_]*"` excludes digits and silently truncated `WM_GETX2FLAG` → plausible-looking `WM_GETX`; the runtime census caught it. |
| **156** | ~8 | 8 | ★ **BoB's map OOB dialogs were RENDER-ONLY — now they take real clicks; Rest zeroes the directive grid** | **(2026-08-09)** SP.14 / MA note 31 §3, answered by *reading the dispatch*: `bob_frontend_tick`'s map click path went `bob_map_click_toolbars` → `bob_map_select` with **no branch into an open dialog**. Every OOB panel built S113–S155 was render-only, and nothing ever failed because the only driver those dialogs ever had was `bob_oob_accept_directives`/`bob_oob_close_dialogs`, which call **`bob_evt_fire` directly** (skipping hit-test + dispatch) — MA's heuristic that *a capability only exercised through scaffolding is evidence the real path is missing* is what prompted the look. **Corrected mid-sprint:** the first write-up blamed `BOB_AUTOCLICK`/`bob_ole_ctrl_point`, which are **shallow** (synthesize a coordinate into the real dispatch) and target front-end panels, not map dialogs — the real distinction is **shallow (substitutes an input, proves the path) vs deep (substitutes a call, proves nothing above it)**. Fix `bob_map_click_oob`: open dialog gets **first refusal**, walks each toolbar's logged children **and descendants** (§8-BoB155), swallows in-dialog misses so a background click can't select a unit behind the dialog; `BOB_NO_OOB_CLICK` reverts. Hit rects are the hosts' own last-drawn rects → cannot drift from what was painted. **Proven with a noise-floor-controlled A/B on ONE binary** (`d9fcef96…`, hashed before *and* after): click at (712,499) → `consumed by toolbar 3 child 6 ctrl id=2600` = **`IDC_RBUTTONREST`**; click vs no-click **4,742 px**; **no-click vs no-click differs only in a 16×8 clock field** (649–665, 248–256) → **4,666 px of signal, 0 noise px in every affected band**. **Handler genuinely ran, not a repaint:** grid goes `1/0/1` + totals `1 1 1` → **all zeros**, all 85 spinners re-rendering new data. ⭐ **Latent defect caught by self-review:** the new call linked only because `_MFC.CPP` includes `MainFrm.cpp` (79) before `fullpsys.cpp` (105) — same unity TU, so an in-body `extern int` inherited C linkage via `[dcl.link]/6`. Correct by *include order*, not construction; now `extern "C"` at file scope, banked **§8-BoB156b**. **Process:** 3 self-inflicted measurement faults, one family (plumbing not reaching the measurement): `2>/dev/null` eating the grepped trace; control run from the wrong cwd (`Can't find ROOTS.DIR`); first diff taken against an **S150-era binary**. |
| **155** | ~8 | 8 | ★ **Dialog teardown lands — 181,424 hosted controls → 184, default-on** | **(2026-08-09)** Applied S154's own retro: instrument instead of a 5th hook-site guess. `BOB_TRACE_DESTROY` printed per-`DestroyWindow` RTTI + host counts + descendants, and one run answered it: `RDEmptyD hosts_here=0` → `child LWDirectives hosts=184` — **DestroyWindow gets the PANEL, the controls belong to the contained dialog**. Uniform across all dialogs; `TakeOverOffered` appeared as **4 distinct instances in one campaign day** (leak in ordinary play). Fix: release the node **and** its bounded `fchild`/`sibling` descendants. **181,424 → 184** (exactly one dialog's worth), tracked dialogs 12 → 6, end state unchanged. **Flipped default-ON on evidence**: measurement + the S108 cancel-toggle path exercised clean + **a full gate suite WITH teardown on returning 14/14 byte-identical**. `BOB_NO_DLG_TEARDOWN` reverts. Still does not free the dialog object (ownership varies). Banked **§8-BoB155** — the panel wrapper has now cost two unrelated 3-sprint hunts and both failure modes *report success*. |
| **154** | ~8 | 4 | ⚠️ **PARTIAL — the missing teardown is `CWnd::DestroyWindow() { return TRUE; }`; hooked default-off, not finished** | **(2026-08-09)** ⭐ The chain executes correctly (`LWDirectives::OnCancel` → `RDialog::OnCancel` → `EndDialog(IDCANCEL)` → walks children → `DestroyWindow()`) and the final call **reports success and destroys nothing** — the *"stub that returns SUCCESS and hides a subsystem"* class the port's notes say to grep `{ return TRUE; }` for (cf. MA S68's `DrawIcon`). **Corrects S108**, which named `CDialog::OnCancel` as the no-op: `Rowan::CDialog` adds only a constructor, so that resolves to `RDialog::OnCancel`, which does real work — the stub is 3 calls further down, and the wrong location read as a closed question for ~45 sprints. **Honest:** 3 hook sites were wrong before this (DestroyPanel never reached, 0 releases at 181,424 hosts; compat `::CDialog` wrong class; `DestroyWindow` fires but frees **1 control not 184** — it reaches the panel, hosts belong to the contained dialog). Next step named (walk `fchild`/`sibling`, needs a safe `CWnd*`→`RDialog*` test), **deliberately not guessed a 4th time**. Default-off (`BOB_DLG_TEARDOWN`); **gates 14/14 byte-identical**. |
| **153** | ~8 | 4 | ⚠️ **PARTIAL — the host "leak" is really that the port NEVER DESTROYS A DIALOG** | **(2026-08-09)** Booked as "prune the host table" (S143 banner: 184 → 1656 across re-opens). Wrote `bob_ole_release_dialog()` + `DestroyPanel` hook, measured **0 releases** on a run reaching **181,424** hosted controls, tens of thousands still *drawn*. Root cause: compat's `CDialog::OnCancel`/`EndDialog` are **no-ops** (afxwin.h:1133/1135) → closing a logged child destroys nothing → `DestroyPanel` unreachable → **every re-open allocates a fresh dialog + ~184 hosts and retains both**. **S108 had documented the same no-op** while fixing a stack overflow and moved on — a note explaining one symptom is not a note scoping the defect. → **SP.23 (8 pts)**, flagged delicate because S108's re-entrancy guard depends on the slot not clearing. Hook kept, documented dormant, fires when teardown lands. SP.21 carries. **Gates:** 14/14 byte-identical, valid. |
| **151** | ~8 | 5 | ★ **`settings.cfg` loads again (preferences no longer die on every rebuild) — and S150's conclusion about four parity rows RETRACTED** | **(2026-08-09)** Root cause measured: `SaveData`'s deserialiser gates on `strcmp(date,date2)` where **both embed `__DATE__`**, so settings load only if written by a binary compiled the same calendar day — the original's own design, harmless to authors who didn't rebuild daily, corrosive to a port that rebuilds constantly (every `ninja bob` discarded preferences; `SavePreferences()` wrote a file the next run rejected). **The fix already existed in the same file for Campaign saves** (`BOB_LINUX`: *"version string embeds the build `__DATE__` … load anyway — the binary format is what matters, /Zp1 vs -fpack-struct"*) and had simply never been applied to `SaveData`. Applied; `BOB_NO_PREFS_LOAD` reverts; verified on four config screens. **⭐ RETRACTION:** S150 had concluded from `successfulLoad=0` that gold parsed the same file and therefore #6/#9/#10/#11's combo differences were ours. **Both assumptions false** — the file's stamp is `Jul 19 2026` (**our port wrote it**; gold shots are 2026-06-24), and with the load fixed those screens are **byte-identical** to factory defaults (loading changes no pixel). Rows → cause unknown, SP.20 reopens, docs corrected in place. **Rule: a measurement licenses only the claim it measures.** |
| **150** | ~8 | 6 | ★ **Gold #18's stray row was a CUT FEATURE's orphaned controls — found by dumping geometry after an id guess failed** | **(2026-08-09)** S146 guessed `IDC_FIGHTERSWEEP*`; the probe matched **nothing**. The widened per-control dump answered it: six real rows at y=317..447 (12 controls each) plus an extra **11-control row at y=229** whose ids are the `...7` members of the same families = index 7 of `FillTargetLists`' `for(i=1;i<8;i++)` = **SWEEPSNDECOYS**, which the game marks dead (`INT3; //Patrols removed.`, LWDIRECT.CPP:1626). **The fighter-sweep feature was cut; its controls stayed in the BDG template** and our DDX/template hosting drew them over the "Ground Attack Gruppen"/"Escort Gruppen" headers. Suppressed by measured id (`BOB_NO_SWEEPROW_SKIP` reverts); headers clean, six real rows untouched. **Residual → SP.21, explicitly not assumed to share the cause:** the "Sweeps" label + "Escort 1:1" row remain, but `IDC_ESCORT_PROPORTION` (1680) is **live code**. **SP.18:** all 16 scaffold hooks audited — one repeatable hook logs, and its tail was **redundant** (S146's all-toolbar sweep superseded the S143 misc-only loop: stack walked twice, policy in two places). Deleted, not quietened. **SP.20:** instrumented (`BOB_TRACE_PREFS`); measured `successfulLoad=0` (real defect, fixed S151) — but the conclusion drawn from it about #6/#9/#10/#11 was **retracted in S151** (our own port wrote that file; loading it changes no pixel). |
| **149** | ~7 | 7 | ★ **Self-certifying gate suite (SP.19) + a THIRD state-mismatched verdict found and fixed (SP.17)** | **(2026-08-08)** Gate promoted to versioned `tools/bob_gates.sh` — the scratchpad+`sed` arrangement had failed twice (a silent rename mismatch overwrote a baseline; recipes drifted from the prose). It now takes an outdir + baseline, A/Bs itself, and **hashes `build/bob` before and after, exiting 2 loudly if it moved** — S148's mixed-binary 13/14 can never again be read as code. First run self-certified: *binary unchanged (md5=a8eccc5e…) — gate valid*, **14/14 byte-identical**. **SP.17 audit:** gold #17 is **Eagle Attack 12–23 Aug**; recipe `1,1,1` never chose a phase → every capture was Convoys. `#1000:1` makes it match gold line for line (`phase=1 autoclick=4/4`). **All three campaign rows (#16/#17/#19) shared one root: no phase token.** ⭐ **And none was invisible** — #17's row literally said "phase/date differ only by selected phase (state)". They were recorded as deviations then reasoned past. Parity doc now states the distinction it had been merging: **a state difference the recipe CAN fix is a defect in the test; one it cannot is a finding about the port.** #6/#9/#10/#11's "combo values (settings state)" deliberately NOT claimed (settings-resident, not recipe-resident) → SP.20, *plausibly benign, not verified*. |
| **148** | ~7 | 7 | ★ **State-predicate capture trigger (SP.16) — and gold #19 judged on a like-for-like frame at last** | **(2026-08-08)** `BOB_SHOT_WHEN=clear` fires on the first map paint with **no logged child on ANY toolbar** (asking every toolbar deliberately — S146 showed one can report -1 while another holds the dialog), AND-able with `BOB_SHOT_DATE` / `BOB_SHOT_TIME_LT`. Rationale: S145's `BOB_SHOT_AFTER` fixed **queueing** drift; S147 found the layer beneath — suppressing prompts makes the campaign run faster, so one recipe gave 12 Aug in S146 and 13 Aug twice in S147 at two paint counts. No counter survives the harness changing the sim rate; only a state description does. **Worked first use after seven counter attempts failed:** clean **12 Aug 20:58 x300** map, `phase=1 date=1250035200`. **#19 re-judged like-for-like** (it had compared a fresh-Convoys capture to a 12-Aug gold since S123): terrain, sectors, labels, No.11 Group, unit-icon layer, footer log, x300 clock, toolbars, ruler all agree → CLOSE, earned. **Raid-stack difference = TIMING, measured:** gold 06:31 raids outbound; our first clear paint 20:58, raids landed. **The map is obstructed precisely while raids fly** (the game prompts) — S146 has ours *with* routes at 07:52 behind a dialog. Evidence `doc/parity/native-strategic-map-eagle-2026-08-08.png` + refreshed `sbs-strategic-map.jpg`. |
| **147** | ~8 | 5 | ⚠️ **PARTIAL — 5 NULL derefs fixed; the capture-timing model shown to be wrong (and my own scaffold carried MA's one-shot bug)** | **(2026-08-08)** **SP.13:** swept every `SendMessage(WM_GETHINTBOX)` site instead of the two I'd read — RCOMBO **0/4** unguarded, RLISTBOX **1/4**, **RBUTTON 4/4**; `CRButtonCtrl` (what a title bar *is*) had no guarded site at all. Five derefs fixed with the codebase's own safe spelling, 6-line diff; note 19 to MA corrected ("one safe, one unsafe" understated it). **Unplanned ⭐:** `bob_oob_close_dialogs` was `static int done`-guarded — MA note 29 §3's trap, in a hook I wrote *after* reading the warning — so the dismiss fired once per process and the Mission Folder + a **"Take over? Dover CH under attack"** prompt sailed past while I blamed the game. Fixed; 5 sibling hooks → **SP.15**. **⭐⭐ Then the fix exposed SP.7's limit:** with prompts suppressed the campaign runs faster, so the recipe that gave 12 Aug 07:52 in S146 gave **13 Aug** twice, at both 340 and 150 paints — **the scaffold changed the quantity the timing depended on**. Captures need a **game-state predicate** → **SP.16**. **#19 still not judged** after six attempts across three sprints; stopped deliberately rather than grind more paint-count guesses. **Gates:** sweep 14/14 + byte-identical vs pre-S142; safe default exit 0; dummy==GL byte-identical; flight 98.7%. |
| **146** | ~8 | 5 | ⚠️ **PARTIAL — the LW orders flow COMPLETES; gold #19's raids are built, fly and land** | **(2026-08-08)** A trace named what S145 could only describe: `RDialog::OnOK rtti=8RDEmptyD` — the logged child is an **empty placeholder panel**, the real dialog is its `fchild`. Descending one level ran the whole chain: `child rtti=12LWDirectives` → `[dir] guard=13178` → `child rtti=16DirectiveResults` → **`MakeLWPackages`**. **Result:** route lines across the Channel, raid markers, Mission Folder listing **R001 / 36 / Dive Bomb / Tangmere AF**, footer log **"Geschwader Landed [R002]/[R005]"** — raids built, flown, returning, at the correct `phase=1 date=1250035200`. **Fifth wrong reading retired:** S145's "`RefreshMissions` never ran" was false — guard passed with `13178/11766/13435` (Airfields/Docks/RDF = the grid's 2/1/2); `LWDirectives::OnOK` was simply never called. **#19 NOT closed** — no unobstructed capture: the Mission Folder is logged on a **different toolbar** (dismiss now sweeps all `TB_TOTAL`), and the Directives dialog re-armed when the suppression flag was omitted; one flag composition remains. **SP.8 probe refutes the id guess:** `167 in template / 17 absent`, exactly `167/184` drawn — the S124 filter works, and no hosted id falls in the `IDC_FIGHTERSWEEP*` band, so the extra row is in-template and hidden another way. **Gates:** sweep 14/14 + **14/14 BYTE-IDENTICAL vs pre-S142** (spans S142→S146); safe default exit 0; dummy==GL byte-identical; flight 98.7%. |
| **145** | ~10 | 5 | ⚠️ **PARTIAL — drive-armed captures land in the intended state (SP.7 done); S144's "accept" proven to have run the WRONG handler** | **(2026-08-08)** ✅ **SP.7:** `BOB_SHOT_AFTER=<n>` fires n paints after `bob_shot_arm()` (first-arm-wins); accept/suppress scaffolds arm it. Proven first run: `[shot] ARMED by accept-directives` → `phase=1 date=1250035200` (correct 12 Aug Eagle Attack), where the same recipe on absolute ticks had drifted to 15 Aug. From MA note 29 §3. ❌ **SP.11:** `BOB_TRACE_DIR` in `LWDirectives::OnOK` printed **nothing** while the accept "succeeded" — the logged child is an RDialog **panel wrapper**, not the dialog (`LWDirectives::Make` → `MakeTopDialog(..., DialBox(FIL_D_LWDIRECTIVES, new LWDirectives(dr)))`), so OK ran `RDialog::OnOK`→`EndDialog`: panel closed, `MakeLWPackages` never reached, **looked like success**. **Two S144 claims corrected at source**, including **§8z in the shared doc** (re-synced to MA, who had been told to act on it). Fourth mechanism claim in this thread produced by reading and overturned by measurement. #19 next step: drive the panel's `dial`. |
| **144** | ~8 | 6 | ⚠️ **PARTIAL — the title-bar OK works for the FIRST TIME (every base-registered event had been dead); map capturable on an active day, but #19 still not a valid oracle** | **(2026-08-08)** ⭐ `bob_evt_fire` matches `type_info` **exactly** (bob_eventsink.cpp:39) with no base-class walk, and all call sites pass the derived type — so **any `ON_EVENT` registered on a BASE class can never fire**, including `ON_EVENT(RDialog, IDJ_TITLE, 3=OK, OnOK)` + Cancel/Help, i.e. the engine's **title-bar ✓/✕/? buttons on every dialog in the game**. Invisible for the port's whole life because every previously-wired event was registered on the same class that received it. Worked around by firing under `typeid(RDialog)` (derived override still runs — `RDialog::OnOK` implicitly overrides compat's virtual `CDialog::OnOK`); general fix = **SP.12**; shared **§8z** (MA: this is likely why Player Log `?`/`✓` do nothing). **SP.11:** the faithful exit from S143's cancel-loop now works (`BOB_MAP_ACCEPTDIR` → `open-index=-1`), and with S143's suppression gave **the first clear strategic-map capture on an active campaign day**. **But it is NOT a valid #19 oracle and the state banner is how we know:** `phase=0 date=1250121600` — drifted to **15 Aug at x20**, 3 days past what a verdict would have claimed, phase reset, no unit icons. Remaining conditions: arm the capture **from the drive** (SP.7), and satisfy the raid path — **[corrected S145:** `dirresults[]` is a *compacted* list built by `RefreshMissions` (k=0 → recon line → i=1..7), so index 0 is the first *allocated* line, not a RECON slot, and `FillTargetLists` writes none of it; the live hypothesis is that `RefreshMissions` never ran headlessly — to be settled by a trace.**]** **SP.8 investigated, not started** (hence 6/8): the extra row is the fighter-**Sweeps** row, **dead code** — `INT3; //Patrols removed.` SP.10 leak reproduced twice (184→368→1656). **Gates:** sweep 14/14 exit 0 + **14/14 BYTE-IDENTICAL vs pre-S142** (spans S142+S143+S144); safe default exit 0; dummy==GL byte-identical; flight 98.7% non-black. |
| **142** | ~8 | 8 | ★ **Hosted `CRSpinButCtrl` — the 8th and LAST R\* control type; the Directives grid's numbers render and match gold #18 value-for-value** | **(2026-08-08)** `CRSpinBut` was the only R\* type never hosted, so every wrapper `InvokeHelper` on it was a silent no-op — and the LW Directives dialog is mostly *made of* it (**85** DDX-bound spinners, LWDIRECT.CPP:149+), which is exactly why S141 rendered that screen's labels, headers and Missions column while every allocation number stayed blank. Hosted via the §8p recipe (new `SRC/RSPINBUT/bob_ole_rspinbut.cpp`, `RSPINBTC.CPP` into the bob_rlistbox lib, `CLSID_RSpinBut` c3270e66… wired), dispids from the WRAPPER. Routing is right by construction: the dialog drives them through `CRSpinButExtra`'s chained `Clear()->MakeNumList()->SetIndex()` = dispids 7 / 5×N / setprop 2. **Values match gold number-for-number:** Morning 40, Mid-day 30; Reconn 0/1; Ground-Attack matrix (Airfields 1/0/0/0, Docks 0/1/0/0, RDF 1/0/0/0, rest 0); "1 Gruppe" ×6; Missions 1/1/1/0/0/0; Escort 2/1, 1/0, 2/0; %Free 100 ×6; Resting 1/3/2/2 + 3/1 — with the red spin-arrow art. **Three traps:** §8t `MaskIcon` temp-bind (patched as BYTES — a text-tool edit had silently re-encoded three `£` literals, §8w); **`CWnd::ReleaseCapture` absent from compat** (first hosted control ever to call it); ⭐ **`m_bDrawing` is a STATIC flag cleared only inside `DrawBitmap`** — one black-fill draw would have latched it and silently killed the other 84 spinners for the rest of the process; neutralised host-side, no game-code edit. **Gates (gl-lock):** sweep 14/14 exit 0 + **14/14 BYTE-IDENTICAL vs the pre-S142 binary**; safe default (`BOB_NO_RUN`) exit 0; **dummy==GL byte-identical**; flight frame-150 **98.7% non-black**. **Honest:** a NEW deviation surfaced — we draw a "Sweeps" row + "Escort 1:1" row gold doesn't show, Sweeps overprinting the section headers (→ SP.8). Harness cost: `BOB_TRACE_OLE` is per-control-per-frame → 70 MB log starved three capture attempts. Evidence `doc/parity/native-strategic-directives-spin-2026-08-08.png`, `sbs-strategic-directives.jpg`. Inbound MA note 29 handled in `26a2a3f`. |
| **141** | ~8 | 8 | ★ **The campaign PHASE is selectable — Eagle Attack reached, gold #18's Directives allocation grid renders; #18 PARTIAL → CLOSE (parity 18 CLOSE / 0 PARTIAL / 1 GAP, the GAP by design)** | **(2026-08-08)** The grid was never a render gap. BoB models a tab row as the **columns of one `CRListBoxCtrl`**: `CSCampaign::OnInitDialog` `AddString`s each phase into its own column and `OnSelectRlistCampaigns(row, column)` (`VTS_I4 VTS_I4`) picks the phase from the **column** — which `bob_ole_click` hardcoded to `0` while resolving the row faithfully via the genuine `GetRowFromY`. So **every campaign the port had ever run started in phase 0** (Convoys, 10 July — a standby day with nothing to allocate), which is why S137's Directives dialog came up empty and read as a screen needing more work. Fixed symmetrically: `OleHost::colAtX` + `HostRListBox::colAtX` → the genuine `GetColFromX` (walks `m_sizeList`); `BOB_NO_LIST_COL` reverts. Also added a **metrics-resolved recipe token** `BOB_AUTOCLICK=#ID[:COL]` (`bob_ole_ctrl_point`) — adopted from **MA S62/S63** so no drive recipe encodes fixed pixels. **Result:** phase select reads "12th August - 23rd August" + the Eagle Attack narrative; on that day **the game opens Directives itself** (S137's `BOB_MAP_DIRECTIVES` no longer needed) and the full grid draws through S137's deep TB_MISC walk — Bomber Allocation / Reconn / Mission Timing / Attached+Detached Escort / Ground Attack Gruppen / **Missions column reading gold's 1/1/1/0/0/0** / Escort Gruppen / Resting / Rest All, footer "Aircraft Quota Allocated", clock "12 August 10:45 x1". **Also corrected a silent STATE mismatch on #16** — gold #16 *is* the Eagle Attack phase; every native capture had been Convoys. **Gates (gl-lock):** sweep 14/14 exit 0; **A/B default vs `BOB_NO_LIST_COL=1` 14/14 BYTE-IDENTICAL**; safe default (`BOB_NO_RUN`) exit 0; phase select **dummy==GL byte-identical**; flight frame-150 **98.6% non-black**. **Honest:** the ~50 numeric spinner boxes don't draw (`CRSpinBut` = 8th R\* type, only unhosted one → SP.4); **#19's raid-stack deviation NOT retired** — no headless way to dismiss a game-opened OOB dialog, `OpenDirectivetoggle` stacks a second (→ SP.5); selected-tab highlight not mirrored (→ SP.6). Evidence `doc/parity/native-strategic-directives-eagle-2026-08-08.png`, `sbs-strategic-directives.jpg`, `native-campaign-phaseselect-eagle-2026-08-08.png`. Cross-port §8u + note 18. |
| **140** | ~6 | 6 | ★ **Hosted `CREdtBtCtrl` (7th R\* type) — the "Bob" briefing name box renders; gold #3 PARTIAL → CLOSE** | **(2026-08-03)** BoBFrag's pilot slots `IDC_PILOT_0..14` are `CREdtBt` (edit-button), DDX-bound; `OnInitDialog`'s `SetCaption(playerslotname)` sets the player's "Bob". Hosted the genuine `CREdtBtCtrl` (new `SRC/REDTBT/bob_ole_redtbt.cpp` + OCX `REDTBTC.CPP` in the bob_rlistbox lib + `CLSID_REdtBt` wired) mirroring the REdit host — with two CREdtBt-specifics: caption is *stock* (`SetProperty(DISPID_CAPTION)`→`InternalSetText`; compat `SetText` is a no-op) and OnDraw's `captiontext` member is refreshed only in handlers → refreshed from `InternalGetText()` in `draw()`. **Two OCX compile-compat fixes** (BOB_LINUX-guarded, §8p class): `IconsUI` forward-decl `:int`→`:unsigned int` (match uiicons.h) + `MaskIcon(CPoint&)` temp-bind (name the temp, cf. RRADIOC/RBUTTONC). **Gates (gl-lock):** safe default exit 0; mainmenu dummy==GL byte-identical; flight 94.9% non-black (CREdtBt only instantiates on BoBFrag → other screens unaffected). **#3 CLOSE** — with S135 roster + S136 Return-to-Player + S139 Fly footer, every gold-#3 element renders. Parity **17 CLOSE / 1 PARTIAL / 1 GAP**. Evidence `doc/parity/native-quickshots-bobfrag-2026-08-02.png`. |
| **139** | ~3 | 3 | ★ **Footer-listbox clip fix — clipped last footer/tab columns render (gold #3 "Fly", gold #2 "Fly")** | **(2026-08-03)** The footer/tab `CRListBoxCtrl` lays columns at its own internal widths but `ExtTextOut`-clips each to the passed `rcBounds`; `bob_draw_menu` passed a tight `total` (re-measured text widths) that clipped the last column (bobfrag "Fly", QS Scenario "Fly", config tab edges). Widened the listbox clip to the remaining screen width — positions are internal + hit-rects come from `wids[]`, so nothing moves, previously-clipped columns appear (`BOB_NO_FOOTER_CLIP` reverts). bobfrag footer = Back/Sim Config/Fly (gold #3); QS footer = Back/Fly (gold #2). **A/B verified** each of 4 screens' diffs is a benign clipped-edge reveal (mainmenu byte-identical; diffs ≤ a few px in the footer/tab band, e.g. QS "Fly" newly visible, gfx2 "C…" fully revealed). Gates: safe default exit 0; flight 94.9% non-black. #3 nearer CLOSE (only the "Bob" name box remains). |
| **137** | ~5 | 5 | ★ **LW Directives dialog (gold #18) now REACHABLE + renders — #18 GAP → PARTIAL** | **(2026-08-02)** The Directives dialog (`LWDirectives`/`IDD_LWDIRECTIVES`) is on the **misc** toolbar (TB_MISC), which `bob_map_paint_oob` never walked → unreachable. Added `bob_oob_open_directives` (`MiscToolBar().OpenDirectivetoggle(NULL)`, null-safe: the ctor builds a default `LWDirectivesResults` from `MMC.directives.lw.current`) + a `BOB_MAP_DIRECTIVES` trigger, and extended `bob_map_paint_oob` to render TB_MISC logged children via a full recursive walk (`bob_oob_paint_tree_deep`, fchild+sibling — dense nested grids the fchild-only Bases walk misses). The dialog opens (exit 0, no crash) + renders its frame + "Rest All" + standby reminder. **Gates (gl-lock):** Bases OOB (TB_MAIN, unchanged) still renders; TB_MISC paint inert when no misc dialog logged (no map regression); safe default exit 0. **Honest:** the dense allocation grid doesn't show — gold #18 is 12 Aug Eagle Attack (active gruppen), mine is 10 July Convoys where the game shows the standby state (grid hidden until active); same state gap as #19. Evidence `doc/parity/native-strategic-directives-2026-08-02.png`. |
| **136** | ~5 | 5 | ★ **Template-driven BUTTON hosting — gold #3's "Return to Player" button now renders** | **(2026-08-02)** `IDC_RETURNTOPLAYER` (2146) is a template-only button no DDX binds → never created on Linux (our creation is DDX-driven). Extended the S124 template-driven static hosting to non-DDX **buttons**: `bob_dlg_enum_buttons` (bob_dlgtemplate.cpp, K_RBUTTON) + a button pass in `bob_ole_host_template_statics` creating `CLSID_RButton` for each unbound template button (`bob_make_rbutton` already renders art+caption; the DDX-bound tickbox proved the path). The briefing's "Return to Player" now draws top-left with its caption = gold #3's key element. `BOB_NO_TEMPLATE_BUTTONS` reverts. **Gates (gl-lock):** config (gfx2/game/mission/control/sound) + QS-Scenario + mainmenu + phase-select **all byte-identical** on/off (inert where no non-DDX template button draws — surgical); safe default exit 0; flight frame-150 94.9% non-black. **#3 stays PARTIAL** (improved) — remaining: "Bob" name box (`CREdtBt` pilot slot, unhosted) + Fly footer item (gated); button art is the tickbox icon vs gold's rounded bezel. Evidence `doc/parity/native-quickshots-bobfrag-2026-08-02.png`. |
| **135** | ~6 | 6 | ★ **The mission BRIEFING (gold #3, `IDD_BOBFRAG`) RENDERS with its flight roster — #3 GAP → PARTIAL** | **(2026-08-02)** Built on S134's re-mapping (gold #3 = the BoBFrag briefing). New `BOB_BOBFRAG` scaffold (FULLPSYS.CPP, BOB_LINUX, default-off) is the reliable headless reach: QS click-mode pre-flight seeds `quickdef` → `LaunchScreen(&quickmission)` inits CSQuick1 → `LaunchScreen(&bobfrag)`, stopping at the briefing (no Rtestsh1 → no flight) so `BOB_SHOT` captures it. The briefing renders closely matching gold #3: crashed-109 + pink-cloud + two-He111 background, the roster listbox (`CRListBoxCtrl` id=1481: **Unit / Aircraft / Duty / Callsign → 54 Squadron / Spitfire / Patrol / Trumpet**), Back/Sim Config footer, exit 0. **Gates (gl-lock):** safe default exit 0; mainmenu dummy==GL byte-identical; flight frame-150 94.9% non-black (scaffold env-gated → no regression). **#3 GAP → PARTIAL.** Deviations named: "Return to Player" (`IDC_RETURNTOPLAYER`) is an RButton, not hosted in the front-end (follow-on); name edit (`CREditCtrl` 1923) created-not-drawn (template/rect); Fly footer item gated (`CheckForMissingMission`). Evidence `doc/parity/native-quickshots-bobfrag-2026-08-02.png`. |
| **134** | ~2 | 2 | ◐ **SPIKE — gold #3 mapping RE-CORRECTED to `IDD_BOBFRAG` (the mission briefing), reach-path scoped** | **(2026-08-02)** Following S133's "flight-line click → editor" remaining-step, found it was a mis-mapping: gold #3's "Return to Player" (`IDC_RETURNTOPLAYER`) is in **`IDD_BOBFRAG`** (`class BoBFrag`, BOBFRAG.CPP) — the mission **briefing** (pilot roster + Squadron/Aircraft/Duty/Callsign + formation callsign buttons + Back/Sim Config/Fly footer), not a QS sub-editor. Reached via the **mission-fly / campaign-intercept flow** (`{IDS_FLY,&bobfrag,CheckForMissingMission}`; or `BOB_CAMPAIGN_FLY`) — the QS Scenario-page "Fly" (`FragFly2`) goes straight to flight (verified: hangs under the SDL-dummy capture driver, no GL). No code shipped (game pristine; investigation only). #3 stays GAP, now mapped to its true screen + reach-path; next sprint = reach BoBFrag headlessly + render its roster (likely reuses the S133 nested draw + RButton callsigns). |
| **133** | ~6 | 6 | ★ **QS order-of-battle flight-lines RENDER — nested `DialList` draw walk; RAF/Luftwaffe tabs show the player flight row (Spitfire IA / 1 / Veteran)** | **(2026-08-02)** S132 fixed the crash so the OOB tabs load; the `CSQuickLine` content stayed blank because `bob_ole_draw_panel(pdial[d])` only draws controls whose `parentDlg==pdial[d]`, and the QS OOB is a `DialList` of `CSQuickLine` rows each with its own `parentDlg`. **Probe (`BOB_TRACE_OOBTREE`) found the game's layout is unusable headlessly** — nested nodes have `viewsize` height 0, `GetWindowRect`=full-screen, `OnGetXYOffset`=(0,0) (MoveWindow/OnSize stubs). **Fix (`FULLPSYS.CPP`, BOB_LINUX, default-on, `BOB_NO_QS_NESTED` reverts):** `bob_fp_draw_nested`/`bob_nested_walk` walk the panel's child `RDialog` tree (`fchild`/`sibling`), draw each nested dialog's hosted controls via `bob_ole_draw_panel`, **synthesizing** the vertical row stacking (identical rows → each content child one `rowStep` lower; `BOB_QS_ROWSTEP`) and reusing the existing template-rect column layout. RAF tab now shows the flight row (piloted-flag icon + Patrol/Altitude/Skill → Spitfire IA/1/Veteran). **Also: MA note 28 verified N/A** (map Bases OOB already composites correctly, no black fill). **Gates (gl-lock):** config-gfx2 + QS-Scenario `cmp` **BYTE-IDENTICAL** on/off (flat panels have no `fchild` → zero regression); mainmenu dummy==GL byte-identical; flight frame-150 94.9% non-black; safe default exit 0. **Honest:** single-flight rows validated; multi-row stacking synthesized (not yet captured with >1 flight); renders the OOB **list**, gold #3 proper (the per-flight *editor* via a flight-line click) stays GAP with that click the only step left. Evidence `doc/parity/native-quickshots-oob-raf-nested-2026-08-02.png`. Cross-port §8s. |
| **132** | ~6 | 6 | ★ **S130 QS order-of-battle crash FIXED — null-reference-safe `DialBox` copy ctor; RAF/Luftwaffe tabs no longer SIGSEGV** | **(2026-08-02)** The crash S130 root-caused (and S129's tab-nav exposed): `QuickMissionBlue/Red` build a variadic `DialList` where inactive flight slots pass `(count>k)?DialBox(temp):*(DialBox*)NULL`; the ternary's prvalue copy-constructs a DialBox from null → the copy ctor derefs address 0. **Fix (RDIALOG.H, one method, BOB_LINUX):** null-ref-safe copy ctor → empty leaf DialBox (`dial=NULL`) for the null case; `AddChildren` already renders a null-dial child as empty `RDEmptyP`. **Two layers (gdb):** (1) the ctor deref; (2) the copy left `diallist[]` uninit (stock ctor relied on copy-elision; a real copy has none) → `AddChildren` recursed into garbage → fixed by copying `diallist` explicitly. **Gates (gl-lock):** RAF-tab click exit 0 (was SIGSEGV); 13/13 sweep; mainmenu/controls/phase `cmp` byte-identical to S131 (core change transparent; strat diff = sim-clock timing variance, two S132 runs also differ); flight 95.2% non-black; OOB dummy==GL byte-identical; safe default exit 0. **Honest:** fixes the crash + unblocks the screen; the `CSQuickLine` flight-line CONTENT (gold #3 fields) doesn't paint yet (nested-dialog-render gap). #3 stays GAP, crash blocker gone. Cross-port §8q addendum. |
| **131** | ~8 | 8 | ★ **Per-face font registry — the pervasive "font FACE" deviation CLOSED (MA note 26 §2); data/labels render Arial (italic), ART screens byte-identical** | **(2026-08-02)** `bob_gdi_font` drew every face in one art TTF (Intel.ttf) → data/label rows in the Rowan art face, not Arial (the deviation on nearly every gold shot). Adopted MA note 26 §2: an 8-slot per-FACE registry (4 kinds × regular/italic) — ART=Intel (unchanged load), SANS=LiberationSans, SERIF=LiberationSerif, MONO=LiberationMono; `CFont::bobFaceKind` classifies the CreateFont face name + captures `bItalic`; `CDC::bobSetFace` threads face+italic through the DC's selected font; the front-end menu sets ART explicitly. **Beyond note 26:** honoured the italic flag → gold's italic combo values match. **N/A for BoB (verified):** §1 Japanese-branch (a `BOB_TRACE_FONT` dump showed the game already requests Arial/Courier/Intel — not a Japanese system), §3 combo-fill (already skipped via `m_FirstSweep=TRUE`); MA note 27's listbox-fill warning heeded. **Gates (all gl-lock):** 14/14 sweep; **mainmenu (ART) `cmp` BYTE-IDENTICAL** on vs off (art screens unregressed); controls **dummy==GL byte-identical**; flight 95.2% non-black; safe default exit 0. Config/campaign data/labels now = gold's Arial-italic scheme; `BOB_NO_FONTFACE` reverts. Cross-port: shared-doc §8r. |
| **130** | ~5 | 3 | ◐ **SPIKE — gold #3 (QS order-of-battle) root-caused to a null-DialBox-copy SIGSEGV; banked** | **(2026-08-02)** With S129's tab-nav, the RAF/Luftwaffe tabs reach `QuickMissionBlue`/`Red` (the QS OOB with the `CSQuickLine` flight editors = gold #3's Squadron/Aircraft/Duty/Callsign) — a never-run-on-Linux screen that SIGSEGVs. gdb: `QuickMissionBlue (fullpane.cpp:215)`. **Root cause:** the variadic `DialList` uses `ND=*(DialBox*)NULL` as a null terminator; `DialList`+`AddChildren` are null-safe (`&ND==0`, `for(i;diallist[i];i++)`), but the per-slot ternary `(count>k)?DialBox(temp):ND` mixes a **prvalue** temp and the **lvalue** `ND` → the conditional is a prvalue → the `:ND` branch **copy-constructs a DialBox from `*(DialBox*)NULL`** (benign-on-MSVC/faults-on-GCC UB). Only inactive slots hit it (`initind=6` → line 215). **Faithful fix is game-code** (name the true-branch DialBox locals so the ternary yields a reference, across the `QuickMission*` builders — a UB-exception change) → deferred; not compat-fixable (copy precedes the list). Honest S129 interaction noted: tabs 0/1 (Scenario/Parameters) render fine; tabs 2/3 now reach this crash. No code shipped (game pristine; gdb/trace only). Cross-port: shared-doc §8q (MA uses the same RDIALOG.H). #3 stays GAP, now exactly root-caused. |
| **129** | ~4 | 4 | ★ **QS tab navigation works — RRadio click → page switch; Parameters tab renders; gold #3 mapping corrected** | **(2026-08-02)** Built on S128's hosted CRRadio: the QS page tabs are now interactive. New `OleHost::onButtonClick(localX)` (HostRRadio maps click X → tab index, `SetCurrentSelection`) + a multi-button branch in `bob_ole_click` that fires the genuine `Selected(idx)` event (dispid 1, VTS_I4) via the S33 eventsink → `CSQuick1::OnSelectedRradio` → `QuickMissionParameters/Desc` → `LaunchDial` (the standard panel-nav, not MoveWindow — why it was tractable). **Result:** clicking Parameters switches to the mission-params page (Target Area/T.D./Weather/Time/Name — a real previously-unreachable QS page); clicking Scenario switches back; bidirectional, verified by genuine clicks. **Gold #3 mapping corrected:** the `16-47-45` gold shot is the per-flight PLAYER editor (Squadron/Aircraft/Duty/Callsign, "Return to Player"; `CSQuickLine`), not the Parameters tab — so #3 stays GAP with its true path (a flight-line click) identified, and the nav machinery it needed is built. Parity unchanged 16 CLOSE / 0 PARTIAL / 3 GAP (interactive-UI + mapping progress, not a verdict flip). **Gates (all under gl-lock):** build clean; 7/7 regression; both nav directions render; safe default exit 0; flight frame-150 95.2% non-black on `:0`; **dummy==GL `cmp` BYTE-IDENTICAL on the QS Parameters page**. |
| **128** | ~6 | 6 | ★ **Host `CRRadioCtrl` — Quick-Shots page tabs render (#2 PARTIAL→CLOSE); 6th hosted R\* control type; parity 16 CLOSE / 0 PARTIAL / 3 GAP** | **(2026-08-02)** `CSQuick1` binds `IDC_RRADIO` as a `CRRadio` and `AddButton()`s the page tabs (Scenario/Parameters/Luftwaffe/RAF), but that control had no host → blank tab row (#2's last deviation, #3's prerequisite). Hosted the genuine `CRRadioCtrl` (`SRC/RRADIO/bob_ole_rradio.cpp`, `HostRRadio : CRRadioCtrl, OleHost`) mirroring the REdit/RButton pattern: boot + `applyDesignProps` (persisted DLGINIT bag → FontNum/Cols/ColW) + `draw` (`m_FirstSweep=TRUE` skips artwork+black-fill → genuine OnDraw) + dispid routing (5 AddButton BSTR, 6 Clear, 1-4 props, stock ForeColor). Registered `CLSID_RRadio` in the factory + `bob_make_rradio` + build integration (RRADIOC.CPP + host TU + include dir). One compile-compat fix in the genuine RRADIOC.CPP (MaskIcon temp-`CPoint&` bind → named local, mirrors RBUTTONC.CPP `_mip00`). **Result:** the tab row renders each caption + its selection-tick/radio icon (MaskIcon art path works, no crash); `[ole] created CRRadioCtrl … AddButton "Scenario"/…`. **Gates:** build clean; 9-recipe sweep 9/9; safe default exit 0; flight frame-150 95.2% non-black on `:0`; **dummy==GL `cmp` BYTE-IDENTICAL on the changed QS screen**. **#2 PARTIAL→CLOSE.** #3 (Parameters page) prerequisite met — remaining half is the tab-click page-switch (`OnSelectedRradio`→`QuickMissionParameters`) + page-visibility (`MoveWindow`), a distinct open item. |
| **127** | ~8 | 8 | ★ **Label-render fidelity: DT_WORDBREAK word-wrap + '&' accelerator escape in `CDC::DrawText` — #8/#16 deviations retired, #2 improved; parity 15 CLOSE / 1 PARTIAL / 3 GAP** | **(2026-08-02)** Two contained wins in the single compat method rendering every R\* STATIC label (`afxwin.h CDC::DrawText`). ☑ **S127.1 (5) word-wrap:** the genuine `CRStaticCtrl::OnDraw` draws prose via `DrawText(..., DT_LEFT+DT_WORDBREAK)` but compat ignored it → the phase-select/QS training descriptions ran off the right edge as one clipped line. Real greedy word-wrap (fits box width via `bob_gdi_text_width`, honours explicit `\n`, DT_CENTER/RIGHT per line, clips to box); **≥2-line-box guard** so single-line config labels never wrap (our stencil font is wider than gold's — wrapping a fitting label would spill into the next row). #16 phase (already CLOSE since S126) + #2 QS descriptions now wrap fully (paragraph breaks preserved). `BOB_NO_WORDWRAP` reverts. ☑ **S127.2 (2) '&' escape:** Windows accelerator-prefix processing ("&&"→"&", DT_NOPREFIX-aware) → BDG "Cockpit && UI" renders "Cockpit & UI" (#8); combos keep literal '&' (they draw via ExtTextOut, not DrawText). `BOB_NO_AMP_ESCAPE` reverts. ☑ **S127.3 (1) cross-port + gates:** MA note 17's `CDC::DrawText DT_WORDBREAK` shared find now implemented BoB-side (outbound note appended, shared doc synced); MA note 17 mechanism #2 (parent-rect clipping) assessed **N/A** (S124 membership filter covers BoB's dead controls; no out-of-bounds stray in the 14-screen sweep). **Gates:** build clean; 14-recipe headless sweep 14/14 exit 0; surgical diffs (controls 447px = just the removed '&'; Sound long labels stay single-line — no wrap regression); safe default `./bob` exit 0; flight frame-150 on `:0` 95.2% non-black exit 0; **dummy==GL `cmp` BYTE-IDENTICAL on mainmenu AND the changed phaseselect** (word-wrap is backend-independent). |
| **126** | ~8 | 8 | ★ **Property-stream reader landed + capture-proven; GLX healed — all real-GL DoD gates PASS; dummy==GL byte-identical bar adopted (first-try pass)** | **(2026-07-27)** ☑ S126.1 (5): the salvaged reader (`9105e25` — real `CPropExchange` replaying each hosted R\*'s DLGINIT bag through its genuine `DoPropExchange`, all 5 control types) verified: 14-recipe headless sweep all exit 0; mainmenu pixel-identical; 13 screens changed *toward gold* — authored design colors land exactly (phase-date `(183,250,255)` = pixel-exact vs the full-res gold PNG; Controls cyan labels; QS yellow combos); #16 duplicate date GONE via the new covered-static settled-state emulation (`BOB_NO_COVER_ERASE` reverts) → #16 **PARTIAL→CLOSE**, #17 improved. Revert gates capture-verified (`BOB_NO_PROP_STREAM` == S125 modulo the independently-gated erase, 3338px in one bbox; `BOB_NO_DLGINIT_PROPS` whole-layer). ☑ S126.2 (2): GLX **HEALED** (probe first, per MA) — default `./bob` exit 0 on `:0`; flight frame-150 96.6% non-black exit 0 (the S125-blocked gate); **new bar: SDL-dummy capture `cmp`-identical to the real-GL capture — PASSES first try** (catches the uninit-PX class headlessly). ☑ S126.3 (1): MA note 16 §2 residual checks applied — both PASS (all 5 host ctors run unattached `DoPropExchange` → PX defaults written on every creation path incl. template statics; stock members member-initialized); **BoB note 17** written (stream layout + COLORREF-convert-once + art-FileNum trap + settled-state emulation + cmp-bar result), shared doc synced byte-identical (md5 `68e921a8…`), message file delivered to `~/ma/port/`. No regression: bare `./bob` 0; sweep 14/14. |
| **125** | ~8 | 8 | ★ **#17 enter-name CLOSE (REdit hosted, gold line layout) + #16 tab-row spread fixed — DLGINIT design-prop slices; sprint closed across two sessions** | **Session 1 (2026-07-26, interrupted by session limit → salvage `ac873f6`):** REdit host (`SRC/REDIT/bob_ole_redit.cpp` + registration) — the genuine `CREditCtrl` hosted for CampaignEnterName's IDC_NAME (word-list machinery, blocking keys, `CommsPlayerName`); note 14 (PE-parser design) delivered to MA; lessons doc +96 synced. **Session 2 (2026-07-26, close-out):** GLX still wedged machine-wide (`X_GLXCreateNewContext` BadValue at `glxinfo -B` level) → the default-run DoD gate on `:0` stayed **BLOCKED**; headless proxy evidence instead: bare run under SDL-dummy boots clean through `InitInstance()` into `Run()` (no crash — the salvaged WIP has no startup regression). **#16 root-caused + fixed (tab row):** the genuine controls load design-time layout props in `DoPropExchange` from the DLGINIT bag; our hosts boot from an EMPTY `CPropExchange` — CSCampaign's tab listbox lost its authored columns (`A0..A3`=180px, `C2/C3`=right-aligned, decoded from boblang.dll RT240 and verified to reproduce gold's spread exactly) and the host's per-draw `Shrink()` tight-packed the rest. Landed: bag-column extraction (`bob_dlg_columns`, last-54-bytes anchor of version&4 bags) + host recreation of the authored columns + Shrink suppression for bag-column controls; RButton `m_alignment` from the persisted ResourceNumber's top byte (`bob_dlg_resnum`, caption-anchored; artless caption buttons only after a first-cut regression on art buttons) → **#17 lines snap to gold** ("Commander Bob" adjacent, "Luftwaffe/phase" adjacent, date centred): verdict **PARTIAL→CLOSE**; #16 **PARTIAL improved** (spread fixed; duplicate date = Windows dirty-region repaint effect, documented + deferred with the S126 property-stream-reader story). `BOB_NO_DLGINIT_PROPS` reverts (verified pixel-identical to pre-fix modulo pre-existing run variance). **Regression:** 14 headless recipes (11 screens + side/phase/entername reruns + strategic map) all exit 0; capture diffs surgical (8 config tabs/menu/QS pixel-identical; residual diffs 4px/68px caption-shadow shifts). Flight reach: GL-blocked. |
| **124** | ~8 | 8 | ★ **BDG-oracle PE resources land — the S123 resource-delta root cause CLOSED; every config tab now CLOSE vs gold** | **PE `.rsrc` DIALOG+DLGINIT thin slice (2026-07-26; PO re-scope: finish well before the session limit).** Planning notes / SM rulings (standing approval, PO can overturn): **(a) oracle = the gold shots as-is = the BDG 0.99 patched build** — parity judged against BDG data, not the 2000 checkout's .rc; BDG-vs-source deltas tagged per-deviation so the ruling flips cheaply (`BOB_NO_PE_RSRC=1` reverts wholesale); **(b) sprint scoped to ~8 pts**: minimal PE extraction + ONE proof screen (Sim-Config Mission — doubling as the SP.2 missing-labels root-cause), then close. Delivered on the existing `bob_resources.cpp` loader (found: boblang.dll already parsed for LoadString!): (1) DIALOG (RT5) + DLGINIT (RT240) enumerators (offset-based, packing-safe); (2) `bob_dlgtemplate.cpp` PE-FIRST load feeding the same rect/caption tables (.rc = fallback only, never overwrites PE); (3) **template-driven static hosting** — the real Mission-tab root cause: `SMissionConfigure` DDX-binds 0 statics, and on Windows the dialog manager creates EVERY template item (`bob_ole_host_template_statics` in `CDialog::Create`); (4) **template-membership draw filter** (source-only controls BDG dropped aren't drawn — killed Sound's overlapped label + stray combos, QS page-ghost combos); (5) **faithful IDS→string-table caption resolution** (the genuine `CRStaticCtrl` `WM_GETSTRING` path → "Town and forest raises"/"Gamma Level"/"109 Fuel Capacity" exact). Verdicts: #6-#13 all **CLOSE** (were PARTIAL x5), #2 improved. No regression: bare 0; all 11 headless screens exit 0; strategic map clean. **Deferred to S125:** parser generalization + MA handoff design doc + outbound note 14 (PO session-budget constraint); enter-name edit hosting; tab wrap; Directives; QS tab recipe. |
| **123** | ~20 | 11 | ★ **Release SP opened — SP.1 done + 3 systemic parity fixes** | **Gold-standard screen sweep (2026-07-25).** ☑ SP.1 (3): all 19 gold shots (not 17 — flagged) mapped + scripted repro + 15 native captures + `doc/screen-parity.md` verdicts; `BOB_SHOT` one-shot capture harness; `BOB_CONFIGSCREEN` +game/mission/views/flight/quick. ◐ SP.2 (~8): dialog-scoped rect lookup (unscrambled the config forms), menu lists at the authored `ListX/ListY` (Back/Begin/Fly bottom-left per gold), `ShowWindow` visibility (ghost statics gone). ◐ SP.3: cockpit + strategic map captured, both CLOSE. Headline: gold = **BDG 0.99 patched resources** vs our source-checkout .rc — label deltas are data-level, PE-`.rsrc`-parser story scoped, PO oracle question posed. No regression (bare 0; flight frame-150 on `:0`; campaign map clean). Cross-port: shared doc §8e (sync ✓) + note 13 delivered (MA/FF/Julia Racer). |
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
| 45 | ~5 | 5 | ★ **R4.5 POST-MISSION DEBRIEF NO LONGER LOOPS — missing-asset infinite loop fixed; debrief degrades gracefully** | **(2026-06-27)** The S44-revealed `dial640` debrief flooded stderr forever on a missing `artwork/dial640/title.bmp` (**12.9M log lines**). The hand-off (Bash was down → designed-not-run) blamed `opennumberedfile`'s `#else` branch; the **first real run disproved it** — a `BOB_MB_BT` `.bmp`-caption backtrace in the compat `MessageBox` showed the loop is the **`#ifdef __MSVC__`** CD-retry path, which is **live on Linux** because `DOSDEFS.H:104` defines `__MSVC__` for GCC. The compat `MessageBox` returns `IDOK` (never `IDCANCEL`) → `while(!retval)` never breaks. **Fix 1** (`opennumberedfile`, `#if BOB_LINUX`): return NULL for a missing file (one deduped warning) instead of the unbreakable dialog loop; **not** via `ReallyEmitSysErr` (it `_exit()`s on Linux). **Fix 2** (`makefileblock`): guard the NULL handle before `getfilesize()`/`readfileblock(NULL)` → empty block → `getdata()` NULL → `RDialog::DoPaint`'s own `if(pData && pData[0]=='B')` skips the missing background (graceful degradation). Verified `:0`: log **12,972,192 → 51 lines**, debrief renders + flow continues (`LaunchMain artnum=27402`), **0 SEGV/clamp**, `exit 0`. Bare `./bob` 0. Only other caller (`MINFILE.CPP`) not compiled. Closes the S35→S44 post-mission loop: fly → return → debrief, no crash, no flood. |
| 44 | ~8 | 8 | ★★ **R4.5 POST-MISSION CRASH FIXED — stale Package.dat (scaffold bypassed OnClickedFrag2's save); sim advances past the whole squadnum family** | **Breakthrough (2026-06-27).** A decode trace showed the out-of-range squadnums came **straight out of the stream** (base-90 encode/decode is symmetric), and `SAVEGAME/Package.dat` is the **Apr-18 shipped template** — never written by a run. The live `Package.dat` write lives in `CMainToolbar::OnClickedFrag2` (MAINTBAR:681); the `BOB_CAMPFLY_GO` scaffold launches `bobfrag` directly, **bypassing that save**, so post-mission reloaded stale data. **Fix:** the scaffold now writes the live `Package.dat` before flying (mirrors OnClickedFrag2). Verified `:0`: post-fly squadnums valid (112/113/126/76 <148), **0 clamps, 0 SEGV** — the post-mission sim advances past `GetCruiseAt` AND `SAGDecisionPreCombat`. The S43 `operator[]` guard stays as defense-in-depth. Next layer (S45, NOT a crash): the post-mission `dial640` debrief loads a missing `title.bmp` and the asset handler loops — fix the archive/path + a missing-asset guard. Bare `./bob` 0. **The milestone the S35→S44 grind drove at: the campaign mission loop no longer crashes on return.** |
| 43c | — | — | ✎ **R4.5 S43 CORRECTION** | **(2026-06-27)** Compile-time reveal: `SQ_MAX=148` (not ~145). Both raid squadnums (210 *and* 160) are >148 → out of range; the S39 clamp hit only 160 by luck of the OOB-read value. So it's **the whole intercepted raid package (pack 2)** with corrupted squadnums post-mission, not "3 phantom squads." The `operator[]` fix stands (removes the OOB-read crash) but neutralizes the entire intercepted raid to default squadrons — bigger caveat than stated. S44 reframed: find why the rebuild brings pack 2 back with out-of-range squadnums (deserialization/offset defect on the engaged package), NOT "delete phantom squads." |
| 43 | ~8 | 6 | ★ **R4.5 ROOT-CAUSED — phantom out-of-range-squadnum raid squads; systemic squadnum-funnel fix** _(see S43c correction — it's the whole raid pkg, not 3 squads)_ | **Root cause + funnel fix (2026-06-27).** A `FixupAircraft` trace showed the corrupt SAGs (uid 4612–4614) are pack 2 (enemy LW raid) squads 4–6 with **`squadnum=160` out of range** (`SQ_MAX≈145`) — 3 phantom raid squads, not a stale type (S42's `type=squadnum` reinit was a no-op 160→160). Landed the systemic fix: `NodeData::operator[]` honors its own `assert(sq<SQ_MAX)` (the S37 ConvertPtrUID move) → returns a neutral Squadron for an out-of-range squadnum → retires the whole squadnum-OOB family in one place (the GetCruiseAt clamp goes **17→0**). The sim still SEGVs at `SAGDecisionPreCombat` (the SAME phantom squads' `target` field, unchanged from S39), so the faithful fix is excluding the phantom squads at the source (FixupAircraft has the safe context) → **S44**. Flight unregressed; bare `./bob` 0. Honest: ships the root cause + a correct funnel fix, doesn't yet advance the observable sim (source fix does). |
| 42 | ~8 | 4 | ◐ **R4.5 type-source localized — `type` is a squadnum (`SetSquad`); corruption is the post-mission rebuild, not creation** | **Type-source localization (2026-06-27).** Found the SAG `type` is set at `Profile::Squad::SetSquad` (`PACKAGES.CPP:5159`, `a->type=squadnum`) — so the corrupt `type=160` is a **stale/invalid squadnum** whose `Node_Data[160]` is bad post-mission. New `BOB_CAMPFLY_NOFLY` probe (default-off) showed the pre-takeoff map sim never ticks the fresh interception SAGs (a `WAITTAKEOFF` squadron isn't in `SagBAND` until takeoff), so the corruption is the **post-mission `StartUpMapWorld` rebuild re-adding the just-flown player squadron to the band with a stale `type`**, not a creation bug. Faithful fix (next pass): reinit the returning squadron's SAG `type` in the rebuild (S36 `FixupAircraft` shape) or don't re-band a consumed squadron — needs a rebuild-path trace to pin the write. Banked; bare `./bob` 0; S39 clamp + post-load unchanged. Continued characterization (cf. S35/S38). |
| 41 | ~8 | 5 | ◐ **R4.5 post-mission SAG-state capture — definitive: no safe skip predicate; fix is the type/target-source (reframe + reusable trace)** | **Empirical invariant (2026-06-27).** Built the `BOB_TRACE_SAG` capture the S40 retro scoped; ran the full post-mission repro on `:0`. The corrupt SAGs (uid 4612/4613/4614 — the just-scrambled player pack, `WAITTAKEOFF`) carry a stable garbage `type`=160 but are **byte-identical to healthy neighbours** in every SAFE base field (`size=12 deaded=0 deadtime=0 movecode=2`) → **no safe predicate exists**, closing the S40 "skip the SAG" direction with proof. Reframed as a **two-garbage-field family** (`type` S39-handled; `target` = ~8 `IllegalBAND` INT3 declared-bound sites that don't halt on Linux). Per-method honouring confirmed fragile (`SAGDecisionPreCombat` derefs `a` post-check, line 2801). Faithful fix = the **type/target-source** (why the post-mission rebuild leaves the returning player-package SAG with garbage fields) — scoped for the next pass. `BOB_TRACE_SAG` kept (default-off, reusable). Bare `./bob` 0; S39 clamp + post-load advance unchanged. Spike: definitive answer + redirect, no behavioural fix (cf. S35/S38). |
| 40 | ~8 | 3 | ◐ **R4.5 SAG-skip — negative result (reverted), next approach scoped** | **Funnel-skip attempt (2026-06-25).** Tried the S39-retro SAG-level skip in `MoveAllSAGs` (skip the broadly-corrupt post-mission SAG so it reaches no crashing method). Reverted: the predicate `((info_grndgrp*)as)->type.Evaluate()` is itself unsafe — on `:0` the skip moved the crash *into* `MoveAllSAGs` on the render/move thread **during flight** (`type` isn't safely evaluable on every SAG subtype in the band; the grndgrp cast reads the wrong offset for air-group/in-construction SAGs). Sound funnel idea, wrong invariant. Reverted to the clean S39 state (flight + post-mission advance intact); bare `./bob` 0. Next: empirically capture the corrupt SAG's raw state for a SAFE predicate (Status.deaded/movecode, not a complex-field Evaluate()), or the type-source fix (why the rebuild leaves it corrupt) — its own focused pass. |
| 39 | ~8 | 7 | ★ **R4.5 post-mission GetCruiseAt fixed — advances one layer** | **Plane_Type_Translate bounds-honor (2026-06-25).** Landed S38's candidate: bounds-honor `Plane_Type_Translate[ptype]` in the 3 `info_grndgrp` move methods (`SAGMOVE.CPP`, S37 pattern) — clamp an out-of-range `ptype` (garbage post-mission `type`) so the OOB read can't yield a garbage `PlaneInit*`. Verified on `:0` (gdb, S38 repro): `GetCruiseAt` SEGV gone; sim advances to `SAGairgrp::SAGDecisionPreCombat` (via `DecideSAG→MoveAllSAGs`). Transparent for valid types; bare `./bob` 0; post-load sim unregressed (→51740). **Systemic finding:** every post-mission crash is the same broadly-corrupt SAG hit through a different method → per-method clamps are whack-a-mole; S40 = a SAG-level skip in `MoveAllSAGs` (or the type-source fix). |
| 38 | ~8 | 5 | ◐ **R4.5 post-mission crash reproduced + root-caused (spike)** | **Post-mission sim (2026-06-25).** Built the post-mission repro (`BOB_POSTMISSION_FF` overrides the post-fly `g_campfly_flown` guard); on `:0`, drove the full campaign loop (load→intercept→fly→mission-end→return→advance). gdb on the release build pinned the crash: advancing the post-mission world SEGVs in `info_grndgrp::GetCruiseAt` (SAGMOVE.CPP:1481) — `ptype=type.Evaluate()` is garbage on a post-mission ground-group SAG → `Plane_Type_Translate[ptype]` OOB → garbage `PlaneInit*` deref (refines S26's `Plane_Type_Translate[bad ptype]`; same systemic shape as S37, array+enum not UID). Bonus: ASan exposed a *separate* in-flight 3D-render UAF (`AddLensObject`/`SunItemAnim` anim-buffer, R1.3a/R3.9 family) that hides the post-mission crash under ASan → used gdb instead. Banked fix candidates (S39): bounds-honor `Plane_Type_Translate[ptype]` (18 sites) or faithful type-source restore. Bare `./bob` 0; spike (default-off toggle only). |
| 37 | ~8 | 8 | ★ **R4.5 POST-LOAD SIM ADVANCES — reference-audit, one systemic fix** | **ConvertPtrUID bounds-honor (2026-06-25).** PO chose the load-boundary reference-audit. Found the post-load fatal crashes are one family (an incompletely-restored deserialised reference → garbage UID → `ConvertPtrUID` indexes `pItem[]` OOB → SEGV). Fix (`PERSONS2.CPP`, `#if BOB_LINUX`): honor `ConvertPtrUID`'s own `assert(tmpUID in [1,IllegalSepID])` — return the null-ref it already gives for UID==0 when out of range (R1.3b/4.3c compat-non-halting-assert class). Retires the whole garbage-UID fatal family in one place. **Post-load sim now advances**: currtime 32180→62540, 1000+ cycles, raids processing, no crash. Transparent for valid UIDs; bare `./bob` 0; fresh-campaign unregressed. Caveat banked: garbage UIDs → NULL (minor target-reacquire fidelity gap); per-reference restoration is the faithful follow-up. |
| 36 | ~8 | 8 | ★ **R4.5 post-load formation crash FIXED — grind advances one layer** | **Formation-pointer fix (2026-06-25).** Landed S35's candidate fix: `FixupAircraft` (`MAPCODE.CPP`) resets each loaded SAG's `fly.leadflight`/`nextflight`/`expandedsag` to NULL (`//save` raw pointers → stale; the SAG AI re-links at runtime — S29 invariant-restore pattern, one layer deeper). ASan-verified via `BOB_POSTLOAD_FF` repro: `CountFormationSize` SEGV **gone**; the sim advances to the next genuine crash (not a side-effect): `SAGExecuteWaypoint (SAGMOVE.CPP:1837)→ConvertPtrUID(SquadTarget(s))` garbage package-target UID (`AM_LWPACKS` branch) — the S15–17 SAG-target family, now post-load (banked for S37). Bare `./bob` 0; load-path only; release+ASan clean. A real fix this sprint (vs S35's spike). |
| 35 | ~8 | 5 | ◐ **R4.5 post-load crash root-caused (spike)** | **Post-load SAG sim (2026-06-25).** Built the post-load ASan repro (`BOB_POSTLOAD_FF` + `BOB_MAP_TIMER` fast-forwards the loaded+`LaunchMap`-rebuilt world). First fatal crash is **not** `GetCruiseAt`/`Plane_Type_Translate` (post-*mission*) but a SEGV in the radio-chatter intel path: `MoveAllSAGs→RAFDetectLW::SetDetectionLevel→...→ArtInt::CountFormationSize (MSGAI.CPP:2367)` walking a garbage `trg->fly.leadflight`. Root: `flight_ctl`'s `leadflight`/`nextflight`/`expandedsag` are `//save`-serialized raw pointers → a loaded raid aircraft carries a stale pointer (S29 `RecostRaidList` class, deeper). Candidate fix banked (NULL loaded AirStrucs' formation pointers at the load boundary; deferred — risks save/AI regression). Reusable repro infra; corrects the prior characterization. Bare `./bob` 0; spike-only (game code pristine bar the default-off toggle). |
| 34 | ~8 | 8 | ★ **R4.2 strategic-map unit icons render** | **Icon-render fix (2026-06-25).** The campaign strategic map now draws the full unit-icon layer (green RAF squadron/airfield, blue fighter, yellow LW raid markers over SE England — matches the Wine gold ref). Measured the bug with new `BOB_TRACE_ICONS` (correcting S27's attribution — the scroll/world transform is fine, the terrain tiles prove it): `UpdateBitmaps` calls `DrawIcons(pDC,inter)` per terrain block with `inter=block∩bounds`, a Windows paint-region optimization; the headless paint has no per-region rect → every `inter`=(0,0,0,0) → the world-rect cull lands ~2.16M units off every item (`scan=1238 cull_pass=0 drawn=0`). Fix (1 line, `#if BOB_LINUX`): draw icons once over the full client bounds (the game's own pre-optimization call, still DEADCODE) → `cull_pass=768 drawn=99`. Visual capture (`doc/reference/strategic-map-icons-2026-06-25.png`) matches the gold ref. Bare `./bob` 0; map-only (behind `g_bob_map_active`). Remaining R4.2: toolbars + scroll/zoom/click. |
| 33 | ~8 | 8 | ★ **General OCX eventsink adopted — 2 targeted bridges retired** | **Cross-port infra (2026-06-25).** Adopted MiG Alley's general `ma_eventsink.cpp` design (renamed `bob_*`): the game's own `BEGIN_EVENTSINK_MAP`/`ON_EVENT` maps now drive control events via RTTI dispatch (`bob_evt_fire(dlg,&typeid(*dlg),id,dispid)`), retiring the R5.3b SController combo bridge + the R4.4 CLoad file-row bridge. Two BoB deltas: a `(LPCTSTR,short)` `bob_evt_call` overload (combo handlers), and `__LINE__`→**`__COUNTER__`** in the registrar name (BoB's unity builds concatenate .cpp → `__LINE__` collided; MA per-TU never hit it — flagged back). Full rebuild clean (83 EVENTSINK TUs); bare `./bob` 0; verified via genuine clicks (`BOB_TRACE_OLE`): CLoad row → `evt_fire id=1062 type=5CLoad HANDLER CALLED` (listempty=0), SController combo → `evt_fire id=2150 type=11SController HANDLER CALLED`. No regression. |
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

- _Sprint 158 (a stub returning a constant deletes the evidence of its own gap):_ **The most valuable
  discipline this sprint was verifying the mechanism was WIRED before testing whether it WORKED.**
  The first implementation compiled cleanly, defined 146 registration functions, and registered
  nothing — `ON_MESSAGE` was stubbed in two layers and the one I fixed lost. Run as-is it would have
  dispatched nothing, changed no pixel, and read as "those routes are dead for another reason".
  Two `objdump` counters (`bob_msgmap_chain` 296, `bob_msgmap_add` **0**) settled it in seconds,
  where a behavioural test would only have said "no effect" without saying which half failed.
  **Rule: when you build a registration mechanism, count the registrations first.** And check the
  right symbol — looking for the registrar *objects* reads zero even when it works, because GCC
  inlines their constructors into `_GLOBAL__sub_I`; that near-miss would have been a confident wrong
  conclusion.
  **Second: predict, measure, then fix — never fix speculatively.** §7d wrote down, before the run,
  that derived dialogs would miss because the declared map bases aren't the real bases, *and* the
  fallback that would fix it. The census confirmed the prediction, and only then was the probe
  fallback built. Cost: one run. Had it been built up front it would have been unfalsifiable
  complexity; had the prediction not been written down, the census result would have needed a second
  run to interpret.
  **Third: a fix that surfaces new failures is working.** Sixteen routes answering 0 suppressed not
  just behaviour but *link errors* — the optimizer deletes branches guarded by a compile-time
  constant, so two missing `CString operator+` overloads and an undeclared `WM_COMMANDHELP` had been
  invisible for the port's whole life. Estimate accordingly: "implement the dispatcher" is not
  bounded work, because every route restored makes more never-executed code reachable.

- _Sprint 157 (the negative result was the useful one):_ **The test that failed to fire taught more
  than the one that worked.** `BOB_MAP_SDLCLICK` was built to prove layer (1); headless, it produced
  nothing — and chasing *why* found that under `SDL_VIDEODRIVER=dummy` the pump never runs at all, so
  no headless test in this project's history could ever have exercised the SDL layer. That reframed
  the whole audit: the drivers weren't sloppy, they were **constrained**, and the untested layer was
  a consequence of the harness, not of carelessness. **When a new test comes back empty, the first
  question is whether the harness can reach the thing at all** — before concluding anything about the
  code. Had I skipped that and gone straight to "the SDL path is broken", it would have been the same
  shape of error as S150's retracted conclusion.
  **Second: state what a measurement does NOT cover, in the same breath as the result.** The headless
  null result says nothing about whether real mouse input works; the real-GL run says nothing about
  the physical mouse/X server. Writing both bounds down at the time is what stops a later sprint from
  reading the note as broader than it was — the failure mode behind S150→S151.
  **Third: two independent entry points producing a byte-identical outcome is the cheapest strong
  evidence available.** The real-SDL click and S156's injected click zero the directive grid to the
  same bytes. Neither alone rules out a scaffold artifact; together they do.

- _Sprint 156 (nothing was failing — that was the problem):_ **The bug with no symptom is the one a
  DEEP scaffold hides.** The OOB dialogs had been render-only since S113 and no test, gate or capture
  ever complained. **And I got the reason wrong on the first pass**, inside the same sprint: I blamed
  `BOB_AUTOCLICK`/`bob_ole_ctrl_point`, then found on re-reading that both merely synthesize a
  *coordinate* and fall into the same dispatch a real click uses — and that they act on front-end
  panels, not map dialogs. The actual culprit was `bob_oob_accept_directives`, which calls
  `bob_evt_fire` **directly on the dialog**. The refined rule is worth more than the original:
  **a scaffold that substitutes an INPUT proves the real path; one that substitutes a CALL proves
  nothing above the point where it enters** — and the deeper it enters, the more impressive the
  evidence it generates, because it is driving the working part of the system. S144–S146 used that
  deep scaffold to build, fly and land whole raids, which felt like proof the dialog worked.
  **Standing check for the rest of the port: for each capability, name its drivers and classify each
  shallow or deep. All-deep ⇒ unproven.** That is a concrete, answerable question — it is now
  SP.24/S157. Note the correction also proves the check's value on itself: the first statement of the
  lesson was plausible, self-consistent, and wrong, and only re-reading the code caught it.
  **Second — the recurring fault this sprint was not in the code, it was in the measurement plumbing:**
  three times (stderr to `/dev/null` then grepped; a run from the wrong cwd that died before init; a
  diff against a six-sprint-old binary) the apparatus failed to reach what it claimed to measure.
  These join the same list as S148's two-binary gate and the monitor that matched its own echo. The
  gate suite already hashes the binary because of S148; **the general lesson is that a measurement
  needs its own sanity check — a hash, a control run, a known-nonzero baseline — and the noise-floor
  run is exactly that check.** Here it earned its keep: without it, "4,742 pixels changed" would have
  been a claim resting on an assumption of determinism this codebase specifically does not warrant.
  **Third, on self-review:** the linkage defect had no symptom and would have surfaced only on a
  future unity-build change, far from the cause. It was found by asking "*why* did this link?" about
  code that already worked. Worth repeating when a change works first try for a reason not fully
  understood.

- _Sprint 155 (trace, don't guess):_ **The previous sprint's retro was the whole plan, and it worked
  on the first attempt.** S154 ended with "when the second guess is wrong, stop guessing and
  instrument"; S155 opened by writing one trace that printed RTTI, host ownership and descendants
  per destroy. It produced the answer, the safety proof for the `dynamic_cast`, and the shape of the
  fix in a single run — after four hook-site guesses across two sprints had produced nothing.
  **Retro items are only worth writing if the next sprint actually executes them**; this is the
  first time in this thread that one was cashed immediately, and it was the cheapest sprint of the
  four.
  **Second: the flip to default-on was earned, not assumed.** Three independent pieces of evidence —
  the leak measurement, the specific dangerous path (S108's cancel toggle) exercised clean, and a
  full gate suite run *with the flag on* — before changing what every player gets. A default-off flag
  is a way to make a risky change measurable, not a way to avoid deciding.
  **Third, the standing pattern:** two subsystems in this port were hidden behind `{ return TRUE; }`
  stubs, and the wrapper-vs-dialog confusion has now cost six sprints across two unrelated tasks.
  Both are *searchable* — grep for success-returning stubs; print RTTI before acting on a dialog.
- _Sprint 154 (implement dialog teardown):_ **Four hook sites, three wrong, and the measurement
  caught each one in a single run — but the real lesson is that I should have instrumented the call
  graph once instead of probing it four times.** S146 already taught me to print the RTTI of an
  object before driving it; I had the rule and reached for `grep` and inference anyway. Probing is
  seductive because each attempt is cheap; four cheap attempts cost more than one trace would have.
  **When the second guess is wrong, stop guessing and instrument.**
  **The find itself is a repeat offender the notes warned about:** `{ return TRUE; }`. Two subsystems
  in this port have now been hidden behind a success-returning stub (MA's `DrawIcon`, our
  `DestroyWindow`). That is a standing grep, not a one-off.
  **And the most transferable bit: S108's note named the wrong location.** It was right that
  something was a no-op and wrong about which thing, and because it *sounded* like a diagnosis
  nobody re-opened it for ~45 sprints. **A note that misidentifies a mechanism is worse than
  silence** — it converts an open question into a closed one. Worth a pass over old notes that
  assert a mechanism without showing the measurement that established it.
- _Sprint 153 (prune the host table):_ **The fix I wrote was correct and fired zero times, and that
  was the useful result.** I had a plausible model (the host map is never pruned), wrote the obvious
  fix, and it built clean — the point at which it is very tempting to call a sprint done. The
  instrument said 0 releases and 181,424 hosts, and chasing *why* found the actual defect: this port
  has **no dialog teardown at all**, because compat's `OnCancel`/`EndDialog` are no-ops. **Verify
  that your fix executed, not just that it compiled** — "correct code in an unreachable place" looks
  identical to "fixed" in every artefact except a measurement.
  **And the humbling part: half of this was already in our own notes.** S108 wrote down "our Linux
  `CDialog::OnCancel` is a no-op, so the loop recurses forever" while fixing a stack overflow, then
  moved on. The fact was recorded; its *scope* never was. **When a note explains a symptom by citing
  a broad behaviour ("X never happens"), ask what else depends on X** — that sentence had been
  sitting in the tree for ~45 sprints with a second, larger defect hiding inside it.
- _Sprint 151 (settings survive a rebuild; and a retraction):_ **The seventh mechanism claim this
  thread has overturned by measuring — and the first where the measurement was RIGHT and the story
  built on it was wrong.** That is the more dangerous shape: `successfulLoad=0` was a real number,
  and it lent unearned credibility to two unchecked inferences (that gold had read the same file,
  and that reading it would change what the screens show). Both were refuted in minutes — by the
  file's own date stamp, and by a pixel diff. **Rule: a measurement licenses only the claim it
  measures.** Write down what was observed and what was inferred as separate sentences; the second
  kind needs its own evidence.
  **Second, the good half:** the fix for a defect I'd called novel had been sitting in the same file
  for dozens of sprints, applied to `Campaign` saves with the reasoning already written out. Before
  designing an accommodation, grep for the same problem elsewhere in the tree — this port has solved
  most of its own problems once already.
  **Third, on damage control:** the retraction touched PORT.md, two board entries and five parity
  rows, because the wrong conclusion had been propagated eagerly the moment it looked good. Good
  news travels fast and needs correcting everywhere it went; that is an argument for holding a
  pleasing conclusion for one extra check before writing it into five places.
- _Sprint 150 (dump the geometry, don't guess the ids):_ **The probe that dumps everything beat four
  sprints of naming candidates.** S146 guessed an id family for #18's stray row and matched nothing;
  widening the same probe to print every drawn control's id and rect identified the row, its
  provenance and its fix in one run — because the answer was in the *geometry* (an extra row above
  the headers, families ending in 7), not in any name I could have thought of. **When a targeted
  probe finds nothing, widen it before re-aiming it: "no match" is usually evidence the filter is
  wrong, not that the thing is absent.**
  **And the answer was a nice piece of software archaeology:** the sweep row is a *cut feature's*
  orphaned controls. The logic was removed (`INT3; //Patrols removed.`) but the dialog template kept
  the widgets, so a faithful port renders a feature the shipped game doesn't have. Worth remembering
  as a category — "we draw more than gold" can mean the original build cut something without
  cleaning the resource.
  **Third: SP.18 was booked as a logging fix and delivered a deletion.** Reading the hook to make it
  quieter revealed its tail had been superseded two sprints earlier and never removed — the stack
  was being walked twice, and the dismiss policy had two places to drift apart. The logging noise
  was a symptom of the duplication, not the defect.
- _Sprint 149 (trust the gate, then audit the verdicts):_ **"That's just state" was doing a lot of
  load-bearing work.** Three parity verdicts rested on the native capture and the gold shot being in
  different game states, and in every case the row *said so* — and stopped there. The phrase reads
  like an explanation, so nobody asked the next question, which turned out to be a single recipe
  token in all three cases. **New rule, now in the parity doc: when a deviation is attributed to
  "state", say which kind — state the recipe can set (a defect in the TEST) or state it cannot (a
  finding about the PORT).** Merging them let three verdicts look earned when they were not.
  **Second, on gates: I made the harness prove itself before trusting what it said about the port.**
  S148 spent real effort deciding whether a 13/14 was a regression, live device state, or an uninit
  read — when it was none of those, it was my rebuild landing mid-sweep. A gate that cannot detect
  its own inputs changing is not evidence, and the fix was ten lines of `md5sum`. Cheap insurance
  for something every subsequent conclusion leans on.
  **Third, on scope honesty:** four rows in the same "state" class were left explicitly unverified
  rather than swept into the win. Three fixed and four honestly open is a better position than seven
  quietly assumed.
- _Sprint 148 (aim at a state, not a moment):_ **Seven attempts with counters, one with a
  description, and the description worked immediately.** The whole #19 hunt across four sprints was
  a slow discovery that I was specifying the wrong thing: `BOB_SHOT=<tick>` says *when*, and every
  time I fixed one source of when-drift (queueing → S145; the harness changing the sim rate → S147)
  another appeared, because "when" was never what I cared about. `BOB_SHOT_WHEN=clear` says *what*,
  and it cannot drift by construction. **Generalise: if a trigger keeps needing recalibration, the
  problem is usually that it names a proxy rather than the condition.**
  **The second thing this sprint bought is honesty about an old verdict.** Gold #19 had been CLOSE
  since S123 on a comparison between a *fresh-Convoys* native capture and a *12-Aug Eagle-Attack*
  gold — i.e. the verdict was right about the port but not actually earned by the evidence. It is
  now earned. Worth a sweep of the other 18 rows for verdicts resting on state mismatches nobody
  noticed, since S141 already found the same problem on #16.
- _Sprint 147 (guard the derefs, find why the shot keeps missing):_ **I spent three sprints blaming
  the game for something my own harness was doing, twice.** First the dismiss that only ever ran once
  (MA had warned about exactly that class, and I wrote the hook *after* reading the warning); then
  the capture timing, where suppressing the game's prompts made the campaign run faster and moved the
  very state I was trying to photograph. Both times the symptom was "the game keeps doing X" and both
  times the answer was "your scaffold does Y". **Rule: when a scaffold and the thing it observes are
  in the same loop, changing the scaffold changes the measurement — treat every harness change as a
  potential perturbation of the state you are capturing, and re-measure the state, not the timing.**
  **Second, on the sweep:** MA's warning named one call; checking *all twelve* cost ten minutes and
  found four more, including a control with a 0/4 record. Cheap thoroughness on a named bug class
  beats sampling — and my first note to MA, sent from the two sites I'd read, was wrong in a way
  that would have under-warned them.
  **Third, on stopping:** six capture attempts, each producing a genuine finding, none producing the
  comparison frame. I closed 5/8 with #19 unjudged rather than take a seventh guess at a paint count.
  The evidence that the raids exist is overwhelming; the *verdict* needs a comparison, and SP.16 —
  arm on a state predicate — is the thing that unblocks it. Knowing which one you are short of is
  the difference between stopping and giving up.
- _Sprint 146 (reach the real dialog):_ **Twelve lines of trace ended a five-sprint argument.** Five
  successive readings of one control path — mine, all plausible, all from the source — were wrong,
  and the thing that settled it printed one line: `rtti=8RDEmptyD`. The object we had been driving
  for three sprints was an *empty placeholder panel*, and the dialog we wanted was one `fchild`
  away. **The rule is no longer "instrument before theorising" but something stricter: in this
  engine, print the RTTI of any object before you drive it.** A "dialog" here is usually a panel
  wrapping the real thing, and every symptom of that — the OK that closes without acting, the
  handler that never fires — looks exactly like a logic bug somewhere else.
  **The corollary that keeps recurring: a scaffold must never assume a single container.** Three
  times now the same shape has bitten — a dialog *stack* (S143), a dialog logged on a *different
  toolbar* (S146), and a popup that *re-arms* itself. Each looked like "it didn't close"; each was
  "you closed the one you were looking at".
  **On sizing and honesty:** 5/8, and #19 stays open despite the raids demonstrably flying, because
  no unobstructed capture exists. It would have been easy to show the raid screenshot and call the
  deviation closed. The parity verdict is a claim about a *comparison*, and I do not have one yet.
- _Sprint 145 (arm the capture, then accept):_ **The instrumentation kept working; my reasoning kept
  not.** SP.7 landed and proved itself in one run — the same recipe that drifted three campaign days
  under an absolute tick landed exactly on 12 August once armed from the drive. Then a two-line trace
  demolished the previous sprint's centrepiece: S144's "accept" workaround had been firing OK at an
  RDialog *panel wrapper*, running the base `OnOK` and closing the panel while the derived
  `LWDirectives::OnOK` never ran — **and reporting success the whole time.** That is the worst
  failure shape available: a workaround that silently performs the base behaviour. Rule to carry:
  **before driving any handler, verify that the object you are holding is the thing that implements
  it** — in this engine a "dialog" is usually a panel wrapping the real dialog.
  **The count is now four:** four mechanism claims in this thread arrived at by reading the source
  and overturned by measuring. Two of them I published to the sibling port before checking, and one
  had to be retracted from the *shared* doc. So the standing rule gets sharpened from "instrument
  before theorising" to something with teeth: **do not put a mechanism into a cross-port note until a
  trace has printed it.** A wrong note costs the other port a sprint, and they cannot see our
  uncertainty.
  Sizing: 5/10, and the 5 is real. Overcommitting because two stories "obviously" chain is its own
  small lesson — SP.11 depended on an S144 result that turned out not to exist.
- _Sprint 144 (accept the orders flow):_ ⭐ **An entire event class had been dead since the port
  began, and the only reason it surfaced now is that we finally fired an event nobody had fired
  before.** `bob_evt_fire` matches types exactly; every `ON_EVENT` on a base class — including the
  title-bar ✓/✕/? on *every* dialog — could never fire. Every previously-wired event happened to be
  registered on the class that received it, so exact matching and correct matching were
  indistinguishable for ~140 sprints. **Generalise: a lookup that has only ever been exercised on
  its easy case is untested, not proven** — the sibling of "when a second client exercises shared
  compat code, treat the first client's 'it works' as under-tested" (Sprint 22). Worth auditing any
  other exact-match table in the port the same way.
  **Second, and this one is about honesty of process rather than code:** the sprint produced a
  beautiful clean map capture that a year of habit would have compared straight to gold #19 — and
  the state banner said `phase=0`, three days of drift, no units. **Instrumentation added one sprint
  earlier caught a bad verdict from the sprint that followed it.** That is the whole argument for
  FF note 15 in one data point, and it is why SP.7 (arm the capture *from the drive*) moved from
  "nice hygiene" to the thing that actually blocks #19.
  **Third, sizing honesty:** closed 6/8, not 8/8. SP.8 was investigated (the row is dead code —
  `INT3; //Patrols removed.`) and deliberately left unimplemented rather than rushed in at the end;
  the points follow the work, not the intent.
- _Sprint 142 (host CRSpinBut — the last R\* type):_ **The §8p recipe held for the eighth and final
  time, and the one thing it did NOT warn about was the expensive one.** Dispids-from-the-wrapper,
  copy-the-nearest-host, expect-two-compile-traps: all still true, and the control was rendering
  within one build. What the recipe doesn't cover is **per-control static state**: `m_bDrawing` is a
  *class-wide* reentrancy flag cleared only inside `DrawBitmap`, so a single draw down the
  black-fill branch would have latched it and silently stopped the other 84 instances for the rest
  of the process — presenting as "the new host doesn't work" with nothing in any log. Add to the
  recipe: **before driving a genuine `OnDraw`, grep the control for `static` members and for early
  `return`s that skip a flag reset.** The fix (clear it host-side) cost two lines once seen.
  Second, cheaper lesson, and it is the same one twice: **"no output" cost three capture attempts.**
  `BOB_TRACE_OLE` is per-control-per-frame; 85 new controls × ~1000 ticks = a 70 MB log that starved
  the run past its own timeout. That is "filter, don't cap" (§8i) arriving from the other direction —
  the trace wasn't *capped* and starved, it was *uncapped* and drowned — and the answer is the same
  one: a predicate, not a budget. Then `pkill -f` matched its own shell (the pattern was in that
  shell's own command line) and killed the relaunch it was clearing the way for. Both were harness
  self-harm, neither touched the port, and together they cost more wall-clock than the feature did.
  Third: **verify a lesson before banking it.** A grep that returned nothing was one command away
  from being written into the shared cross-port doc as "grep goes silent on ISO-8859 files"; the
  real cause was a reset cwd. Re-running it with the blamed variable actually isolated is what
  caught it — and the hunt did surface a genuine trap next door (the silent `£` re-encode, §8w),
  which is only worth having because it was checked rather than assumed.
- _Sprint 141 (choose the campaign phase, #18 → CLOSE):_ ⭐ **A hardcoded event argument spent four
  sprints disguised as a rendering gap.** S137 looked at an empty Directives dialog and correctly
  concluded "state gap — the grid is hidden until an active phase"; S141 found *why* the port could
  never be in an active phase: the hosted-listbox click fired `Select(row, column)` with the column
  **hardcoded to 0**, and BoB picks the campaign phase from that column. Nothing was uninitialised
  and nothing errored — the value was simply always the same wrong constant, so the symptom surfaced
  four screens away and got written down as a screen that needed more work. **This is the §8i family
  wearing a new coat** (stubbed/ignored argument → plausible-looking downstream defect), and it
  extends the standing rule: when a screen looks *state-starved*, audit what SELECTS the state before
  touching what draws it. The tell here wasn't run-to-run variance (the giveaway for the uninit
  flavour) but **invariance** — the campaign started on 10 July every single time, across every
  capture, for four sprints. An always-identical value in something the user is supposed to choose
  deserves the same suspicion as a value that changes when it shouldn't.
  Second lesson, cheaper: **adopting MA's `#ID[:COL]` recipe rule cost nothing because it was done
  before the first pixel was written down.** The temptation was to hardcode "click (419,36)" — the
  point we'd just measured — and move on. Retro-fitting that rule cost MA a sprint; applying it
  pre-emptively cost about ten lines. Adopt a sibling port's *process* lessons at the moment you'd
  otherwise commit the mistake, not after.
  Third, a gate-hygiene one: **the safe-default gate first reported `exit=124` and it was the
  harness, not the port.** Bare `./bob` on a real data dir correctly enters the interactive `Run()`
  loop and never exits — the standing gate is `BOB_NO_RUN=1` (link-only safe default). A gate that
  is *mis-specified* rather than failing is the expensive kind: it reports a number that looks like
  a regression and invites a hunt through a clean diff. Two rules banked: read how the previous
  sprint actually invoked a gate before re-implementing it, and never wrap an interactive app in a
  bare `timeout` and read its exit code as a verdict (cf. the standing "no timeout on interactive
  apps" rule, and MA's `timeout -k`).
- _Sprint 140 (host CREdtBt, #3 → CLOSE):_ **The new-control-type recipe (§8p) held, and its two
  known compile traps were the whole cost.** Hosting the 7th R\* type went exactly as the checklist
  predicts — mirror the nearest host (REdit), wire CLSID + CMake, DDX auto-instantiates — and the
  only friction was the two OCX compile-compat fixes the note already names (IconsUI enum underlying
  type, MaskIcon temp-bind), both one-liners with a precedent to copy. Lessons: (1) when a genuine
  OCX won't compile, check the sibling control TUs that already compile for the exact GCC fix before
  reasoning it out — RRADIOC/RBUTTONC had the MaskIcon pattern verbatim; (2) read the control's OWN
  draw for how it sources its text — CREdtBt draws a `captiontext` member it refreshes only in
  handlers, so a host that calls OnDraw directly must refresh it, a subtlety invisible until you read
  OnDraw; (3) a stock vs custom Caption dispid decides the setter (`InternalSetText` vs a control
  `SetCaption`) — check `SetProperty(...)` in the wrapper .cpp, don't assume; (4) a screen that took
  six sprints (S130→S140) to reach CLOSE was mostly reuse — the crash fix, the reach scaffold, and
  five already-hosted control types; only one genuinely new control (CREdtBt) remained.

- _Sprint 139 (footer clip fix):_ **A one-screen deviation was a shared-primitive bug — fixing it
  at the primitive fixed several screens at once.** #3's missing "Fly" looked local, but the clip was
  in `bob_draw_menu`'s width handed to every hosted footer listbox, so the same one-line widen
  surfaced the QS Scenario "Fly" (gold #2) and clipped config-tab edges too. Lessons: (1) when a
  widget renders "N-1 of N," suspect a width/clip handed in from the caller before the control's own
  draw — the control was drawing all N, the caller clipped one; (2) prove a global draw-width change
  safe by diffing the *bounding box* of changed pixels per screen, not just cmp — every diff being a
  few px in the footer band (and a legible revealed glyph) is the evidence it reveals, not shifts;
  (3) keep positions and hit-rects decoupled from the clip so widening it is provably inert to
  layout — the fix moved nothing, it only stopped hiding.

- _Sprint 137 (reach the Directives dialog):_ **A "GAP" screen can be two different gaps —
  reachability and content — and the reach is often the cheap half.** #18 read as one blocker but was
  really: (a) the dialog was on a toolbar the paint loop never walked (TB_MISC vs TB_MAIN), and (b)
  its content only populates in an active campaign phase. Fixing (a) — mirror the existing Bases
  open+paint onto the misc toolbar — made the screen reachable + framed in one sprint; (b) is the
  same fresh-day-vs-Eagle-Attack state gap as #19, not a render bug. Lessons: (1) when a dialog is
  "not reachable," check which toolbar/owner logs it before assuming a missing subsystem — the OOB
  paint may just not walk that owner; (2) distinguish "renders nothing" from "renders the game's
  empty state" — my capture's "standby" message is the faithful Convoys-day state, so calling it
  PARTIAL (not a broken render) is the honest verdict; (3) a null-safe `Make(NULL)` (ctor fills the
  default) is the cheapest deterministic reach — no need to synthesize the results struct.

- _Sprint 136 (template button hosting):_ **When a control is missing, first ask "is it even
  created?" — DDX-driven creation silently drops template-only controls.** gold #3's "Return to
  Player" wasn't a draw bug; the button was never instantiated because `BoBFrag::DoDataExchange`
  doesn't `DDX_Control` it (on Windows the dialog manager creates every template item regardless).
  The fix was the exact sibling of S124's template-static hosting — a one-enum, one-loop extension
  to buttons — because `bob_make_rbutton` already existed. Lessons: (1) trace whether a missing
  widget was *created* before theorising about *drawing* it — "created CR…Ctrl" in the OLE trace is
  the tell; (2) a global creation change needs a byte-identical A/B across unrelated screens
  (config/QS/menu) as the regression proof — the change is only safe because it's inert wherever no
  non-DDX template button draws; (3) reuse compounds — the roster listbox (already hosted) and now
  the button both came almost free, so "render screen X" keeps shrinking to "host the one control
  type X adds + a reach."

- _Sprint 135 (render the briefing):_ **A direct-launch scaffold beats fighting the nav — when
  you can seed the state the screen needs.** S134 burned two runs on the fragile campaign/click
  reach (the campaign-fly found no LW package; the QS "Fly" hung in flight bring-up). S135 landed
  the screen in one build by asking "what state does BoBFrag actually read?" — `quickdef`, which
  `CSQuick1` populates — and reproducing exactly that (pre-flight → launch quickmission → launch
  bobfrag), stopping short of the Fly that starts flight. Lessons: (1) to reach a data-driven
  screen headlessly, seed its data via the smallest real prerequisite (launch the screen that fills
  the static it reads), don't replay the whole click chain; (2) stop the scaffold one step before
  the expensive/ fragile action (here, Fly→flight) — the capture wants the screen, not the
  transition; (3) the roster came for free from the genuine `CRListBoxCtrl` — the S133/earlier
  hosting work compounds, so a "new screen" is often mostly already-built controls + a reach; (4)
  name the missing pieces precisely (RButton-not-hosted vs template-filtered vs gated) so the
  follow-on is a checklist, not a re-investigation.

- _Sprint 134 (gold #3 re-mapping spike):_ **Follow the resource id to the screen, not the
  narrative to the screen.** #3 had been mapped by S129 to the "Parameters" tab, re-mapped to a
  `CSQuickLine` editor, and S133 inherited "a flight-line click reaches it" — all plausible stories,
  none verified against the actual control. One `grep IDC_RETURNTOPLAYER` settled it in a minute:
  the button is in `IDD_BOBFRAG`, the briefing. Lessons: (1) when a screen has a distinctive control
  (a named button, a unique caption), grep the *resource id* to the owning dialog before theorising
  about the nav path — the id doesn't lie, the recollection does; (2) a capture attempt is a cheap
  disambiguator — the QS "Fly" hanging under the dummy driver instantly proved that Fly ≠ briefing
  (it's flight bring-up), narrowing the reach-path; (3) it's fine — good, even — to *un*-claim a
  remaining-step you stated last sprint once you learn it was wrong; the honest correction is worth
  more than defending the earlier guess.

- _Sprint 133 (nested OOB render):_ **Probe the runtime geometry before trusting the game's own
  layout — then synthesize what the stubs don't provide.** The obvious plan was "walk the tree and
  read each row's rect." A 20-line `BOB_TRACE_OOBTREE` probe killed that in one run: every nested
  node reports `viewsize` height 0 and full-screen `GetWindowRect` because `MoveWindow`/`OnSize`
  are compat stubs — the game never computes the row layout headlessly. The winning move was to
  stop trying to recover a layout that doesn't exist and instead *synthesize* the one invariant I
  did know (identical rows stack vertically), reusing the per-control template-rect positioning the
  config panels already trust. Lessons: (1) when porting a layout-driven screen, dump the runtime
  rects first — a stubbed layout engine silently returns zeros, not errors; (2) `cmp`(on, off) on
  the *flat* screens (config/Scenario byte-identical) is the cheap proof a new draw path is inert
  everywhere it shouldn't fire — an early-return on `!fchild` plus a byte-identical A/B beats
  eyeballing five screens; (3) measure the sibling's inbound note before adopting it — MA note 28's
  black-fill fix dissolved on a single BoB capture (map OOB already composites), exactly as the
  note itself warned; (4) separate "list renders" from "editor reached" honestly — #3's list is now
  populated, the flight-line click to its editor is the named next step.

- _Sprint 132 (null-DialBox crash fix):_ **Fix the crash at the smallest shared layer, and expect
  a second layer behind the first.** The whole class of `(cond)?DialBox(...):*(DialBox*)NULL`
  ternaries across the panel builders was fixable by ONE null-safe copy ctor rather than rewriting
  each builder — because `AddChildren` already handled the resulting `dial==NULL` child. But
  stopping there just moved the crash: the copy left `diallist[]` uninitialised (the stock ctor
  silently depended on copy-elision), so the fix wasn't done until gdb showed the second SIGSEGV
  and I copied `diallist` too. Lessons: (1) when a crash is a value-category / UB quirk, the fix
  usually belongs in the shared primitive (the copy ctor), not the many call sites — one guarded
  method beats N rewrites; (2) a "benign on MSVC" copy ctor that reads members can hide a
  *second* dependency on copy-elision — verify by re-running, don't assume the first fix is
  complete; (3) `cmp`(pre, post) on working screens is the proof a core-header change is
  transparent — mainmenu/controls/phase byte-identical said more than any eyeball; (4) separate
  "no crash" from "renders" honestly — the screen loads now, but its content paint is a named,
  deferred follow-on, not a claimed win.

- _Sprint 131 (per-face font registry):_ **Trace what the code actually asks for before porting
  the sibling's fix — the diagnosis transfers even when the patch doesn't.** MA note 26's headline
  was a Japanese-branch trap; a 10-minute `BOB_TRACE_FONT` dump showed BoB already requests
  Arial/Courier/Intel (English), so §1 was a no-op and §3 was already handled — the real gap was
  only §2 (the registry). Porting §1 blindly would have added dead code chasing a bug BoB doesn't
  have. Lessons: (1) the empirical trace both scoped the sprint down AND de-risked it (I knew the
  classification worked before writing the registry); (2) the byte-identical `cmp`(on, revert) on
  the ART title screen was the safety net that let me ship a font change touching every screen —
  it proved the art screens were untouched in one command; (3) going one step past the sibling
  note (honouring `bItalic`) was cheap once the registry existed and closed the last visible gap
  (gold's italic values); (4) heed the sibling's *warning* as much as its fix — note 27 said "don't
  skip the listbox fill", so I checked and left it alone.

- _Sprint 130 (gold #3 OOB spike):_ **A cheap probe with the machinery you just built can
  reveal the next real bug — and knowing when to bank beats grinding.** One click on the RAF tab
  (using S129's nav) turned "reach gold #3" from a UI story into a precise crash root-cause: the
  QS order-of-battle SIGSEGVs on a ternary that copies a `DialBox` from `*(DialBox*)NULL`. The
  disciplined outcome was to root-cause it exactly (list-side null-safe, ternary value-category
  the culprit), write the shared-doc note (§8q — MA shares `RDIALOG.H`), and bank it rather than
  attempt a multi-site game-code UB fix late in the session. Lessons: (1) a bug in never-run game
  code that only fires on *sparse* data (inactive flight slots) hides behind full-complement test
  data — vary the inputs; (2) verify where the null-safety actually is — `AddChildren` looked
  guilty but was fine; the copy happened one layer up in the builder; (3) a spike that produces an
  exact root cause + a reusable cross-port note is a legitimate increment even with no code
  shipped — the next session starts with the fix, not the diagnosis.

- _Sprint 129 (QS tab navigation):_ **Sample the gold shot before assuming what a story is —
  a backlog label can mis-map the target.** #3 was logged as the "Parameters/player page,"
  reachable "once the tab captions render"; wiring the tab-click and rendering the Parameters
  tab was the whole assumed fix — but sampling gold `16-47-45` showed it is a *different* screen
  (the player-flight editor with "Return to Player," no tab row), reached by a flight-line click.
  The sprint still delivered real value (bidirectional tab navigation + a new reachable page),
  and the honest move was to correct the mapping and keep #3 GAP rather than claim a match that
  the pixels don't support. Lessons: (1) the de-risking find came early — reading
  `QuickMissionParameters()` showed it uses `LaunchDial` (the standard panel-nav), not the hard
  `MoveWindow` page-switch I'd feared, which turned a scary story into a small one; (2) reuse the
  existing seam — the click→`bob_evt_fire`→eventsink path was already there for combos/lists, so
  RRadio needed only one new virtual (`onButtonClick`) and one branch; (3) a sprint that advances
  interactivity + corrects a mapping is a legitimate increment even when it moves no verdict —
  name it as such rather than inflate the count.

- _Sprint 128 (host CRRadioCtrl):_ **A "missing caption" can be a whole un-hosted control
  type — check what the wrapper binds before assuming a data/font gap.** #2's blank tab row
  looked like another caption-resolution deviation; the one-line cause was that `IDC_RRADIO`
  is a `CRRadio` and BoB had never hosted that type, so every wrapper call was a no-op. Once
  identified, the fix was pure pattern-application — the 5th→6th hosted control mirrored REdit
  almost verbatim (boot/applyDesignProps/draw/dispatch), which is the payoff of having built
  the hosting seam five times. Lessons: (1) the established host pattern makes a new R\* type a
  ~1-hour job, not a subsystem — the investigation (dispids from the wrapper, CLSID, OnDraw
  requirements) is the real work, the code writes itself; (2) reuse the sibling fix verbatim —
  the MaskIcon temp-`CPoint&` bind was already solved in RBUTTONC.CPP, so grepping for the
  prior fix beat re-deriving it; (3) knowing where a story STOPS is as valuable as delivering
  it — #3 needs the page-switch/MoveWindow mechanism, so S128 shipped the prerequisite (tabs
  render) and named the remaining half rather than half-wiring a click that couldn't paint.

- _Sprint 127 (label-render fidelity):_ **When you implement a Windows drawing flag the game
  already passes, scope the guard to the case the game actually needs — not the flag's full
  generality.** `DT_WORDBREAK` is passed by EVERY R\* static (labels and descriptions alike),
  but only the tall description boxes should visibly wrap; blindly wrapping on the flag would
  have regressed every config label (our font is wider than gold's, so a label that fits on
  Windows spills a line here). The ≥2-line-box guard turned a risky global change into a
  zero-regression one — verified by the Sound tab's long labels staying single-line and the
  surgical 447px controls diff. Lessons: (1) a one-line grep ("who calls DrawText?") proved
  combos/buttons use ExtTextOut, making the '&'-escape safe before writing it — cheap scoping
  beats defensive special-casing; (2) the dummy==GL `cmp` bar (S126) paid off again — it
  confirmed the wrap renders byte-identically on both backends in one command; (3) adopting a
  sibling-port note (MA 17's DT_WORDBREAK) is highest-value when the shared find maps to a
  single method you own — implement + reply in the same sprint while the context is loaded.

- _Sprint 126 (property-stream reader proven + GL gates return):_ **Probe the environment gate
  BEFORE planning around it, and verify a "wrong-looking" render against the primary oracle
  before debugging it.** The GLX probe (10 seconds) turned two sprints of "GL-blocked" carry-over
  into three passing DoD gates; and the half-hour spent chasing a "color channel swap" dissolved
  the moment the ORIGINAL gold PNG was sampled instead of the JPEG side-by-side composite — the
  "cyan" date was pixel-exact gold. Lessons: (1) a salvaged WIP's missing piece is usually the
  *verification*, not more code — the whole S126 diff needed zero fixes, only the sweep + gates
  it was interrupted before running; (2) adopt the sibling port's acceptance bar cheaply while
  the surface is fresh (the dummy==GL `cmp` cost one run and is now a standing net for the
  garbage class); (3) revert-gate verification by capture-diff (gate ON == old reference within
  a named bbox) is fast and decisive — three gates verified in one loop.

- _Sprint 124 (BDG-oracle PE resources):_ **Look for the loader you already have before designing the
  one you think you need.** The story was scoped ~8-13 pts as "write a PE `.rsrc` parser"; the first
  hour found `bob_resources.cpp` already parsing the PE resource tree of the exact BDG module
  (boblang.dll, loaded for LoadString since the earliest sessions) — the delivered slice was two
  enumerators on top of it, and the sprint's real work turned out to be the CONSUMER side. Lessons:
  (1) **the "missing labels" had two independent causes** (no DDX binding → control never created;
  AND caption data) — fixing only the planned one (data) would have shipped zero visible labels on
  the proof screen; walking the runtime path end-to-end (DDX → create → applyDesignProps → draw)
  before coding found the second. (2) **fidelity comes from mimicking the platform's mechanism, not
  the artifact**: "create every template item" and "resolve captions via WM_GETSTRING/LoadString"
  each replaced a would-be heuristic with what Windows actually does, and gold text snapped into
  place ("Town and forest raises"). (3) **a thin sprint under a hard session budget works**: one
  proof screen, verdict-table updates as the demo, everything else explicitly deferred with names.
  (4) **headless runs don't need the display lock** — queueing on a sibling's flock for a
  SDL-dummy capture wasted the first attempt; the protocol's intent is the GL display, match the
  intent not the letter (real-GL runs still lock).

- _Sprint 123 (Release SP opened — gold-shot inventory + 3 systemic parity fixes):_ **Build the
  oracle-driven capture loop FIRST; the fixes then pick themselves.** The sprint's leverage came from
  spending the first hours on a deterministic one-command capture per screen (`BOB_SHOT`) and viewing
  every gold shot before touching code — the three fixes that followed (scoped rect lookup, `ListX/ListY`
  anchors, `ShowWindow`) were each visible as a *pattern across many screens*, not a single-screen bug.
  Lessons: (1) **when a reference and the port disagree on CONTENT, check data provenance before
  render code** — the gold build runs BDG-0.99-patched resources; half the label "bugs" were
  resource-version deltas (the S44 provenance lesson, now on the render side). (2) **the engine often
  ships the layout data you're synthesizing** — `FullScreen::ListX/ListY` was authored per-resolution
  placement sitting unread while we hand-centred menus; grep for the data before writing a heuristic.
  (3) **an infrastructure asymmetry is a bug magnet**: the scoped rect table existed since S94 but only
  the toolbar path used it — when a fix lands, sweep the OTHER call sites of the thing it replaced.
  (4) **don't widen hit-rects past what's drawn** (declined the tab-row spread): the first-pilot
  hit-box lesson held firm against a cosmetic win. (5) **verdict tables with named deviations beat
  prose** — MATCH/CLOSE/PARTIAL/GAP per shot made the remaining SP.2/SP.3 backlog self-evident and
  PO-reviewable. (6) Honest count: ~11 of ~20 pts — the BDG-resource question gates the rest; posing
  it crisply to the PO IS the deliverable, not grinding label-by-label against the wrong oracle.

- _Sprint 44 (R4.5 post-mission crash FIXED — stale Package.dat):_ **Five sprints of "fix the garbage
  field" were treating the symptom; the actual bug was that the data was garbage because we were reading
  the wrong file — and the validation-methodology instinct (check the file's timestamp / where it comes
  from) cracked what per-field bounds-honoring never could.** S41–S43 honored bound after bound on values
  that were out of range; S44 asked *why* they were out of range, traced them to `DecodePackage` reading
  them straight from the stream, and found `Package.dat` was the Apr-18 shipped template the scaffold never
  overwrote. Lessons: (1) **when the data is garbage, question the source before guarding the consumer** —
  a stale/wrong input file produces "corruption" that no amount of consumer-side bounds-honoring fixes
  faithfully; the S37/S39/S43 guards were all real UB-removal but none addressed the input. (2) **check
  provenance: a file timestamp is a one-line test that reframes a multi-sprint grind** — `ls -la
  Package.dat` (Apr 18, never written) was the whole answer, available since S38. (3) **a scaffold must
  reproduce the real flow's side effects, not just its happy path** — `BOB_CAMPFLY_GO` drove the
  map→mission seam but skipped `OnClickedFrag2`'s save; the bug lived in what the scaffold *omitted*,
  matching the cross-port lesson MA and I keep relearning (the integration risk is in the flow you bypass).
  (4) **the symmetric-codec check ruled out a whole hypothesis cheaply** — confirming base-90 encode/decode
  are inverse functions proved the decode was faithful, redirecting from "deserialization bug" to "wrong
  data." (5) **fixing a crash often just reveals the next gate** — the missing `title.bmp` is genuine
  forward progress (the flow now reaches a screen it never used to), not a regression; framing it as the
  S45 layer keeps the win honest. (6) **defense-in-depth and the real fix coexist** — the S43 guard stays
  (don't SEGV on a bad file) alongside S44 (feed the right file); belt and braces is correct here.

- _Sprint 43 (R4.5 root-cause + squadnum-funnel fix):_ **Measuring both the corrupt AND the matching
  field killed a plausible-but-wrong fix before it shipped, and the real root turned out one level deeper
  than the symptom.** The `FixupAircraft` trace I added to verify S42's `type=squadnum` reinit immediately
  showed `type_was=160 squadnum=160` — the reinit was a no-op, the squadnum *itself* is the defect. Without
  that trace I'd have committed a do-nothing "fix." Lessons: (1) **trace the fix's own assumption, not just
  the bug** — S42 assumed type was stale vs a valid squadnum; one trace line (`type==squadnum`) falsified
  it and redirected to the real root (out-of-range squadnum). (2) **find the funnel and honour the engine's
  own bound there** — `NodeData::operator[]`'s `assert(sq<SQ_MAX)` is the single choke every squadnum
  lookup passes through; fixing it there is the S37 move (one change, whole family) vs S39's per-method
  clamp. (3) **a fix that doesn't move the observable needle can still be correct and worth shipping** — the
  funnel fix removes a latent OOB-read corruptor and is defense-in-depth, even though `SAGDecisionPreCombat`
  (a *different* field of the *same* phantom squads) still crashes; I framed that honestly rather than
  overclaiming "sim advances." (4) **when every field of an object is garbage, stop fixing fields** — type,
  squadnum, and target are all garbage on the 3 phantom squads, so the convergent fix is at the source
  (exclude the phantom squads), not the Nth field; that's the S44 scope, deliberately not rushed at session
  end (the S40 discipline). (5) **manage the repro, not just the code** — the per-frame loop trace drowned
  the post-mission signal and starved the timeout; splitting it to `BOB_TRACE_SAG_LOOP` made each slow
  verify actually reach the phase under test.

- _Sprint 42 (R4.5 type-source localization):_ **A probe that returns "nothing happened" can still be a
  positive result if you read what the silence means.** The `BOB_CAMPFLY_NOFLY` probe was built to catch
  the clamp firing pre-flight; it caught *zero* SAG ticks instead — which, rather than a dud, is the
  finding: a `WAITTAKEOFF` squadron isn't in the movement band until takeoff, so the corrupt post-mission
  SAGs being *in* the band is itself the anomaly, pointing at the rebuild. Lessons: (1) **localize the
  write before attempting the fix** — `SetSquad:5159` (`type=squadnum`) reframed "garbage type" as "stale
  squadnum reference," which is a different and more tractable bug than "uninitialised memory." (2) **an
  experiment that doesn't fire still constrains the hypothesis space** — no pre-flight clamp + no pre-flight
  ticks together rule out creation-time corruption and isolate the rebuild. (3) **name the probe's
  limitation in the bank** — NOFLY *implies* rebuild-corruption but doesn't *capture* the stale write; the
  honest follow-up (a `StartUpMapWorld` SAG-band trace) is now written down so the next pass starts there.
  (4) **respect the budget under autonomy** — two characterization sprints (S41 definitive-negative, S42
  localization) on a known multi-session grind is real progress; grinding the rebuild dig at the window's
  end is exactly the masking-risk the S35/S40 retros warn against, so banking the precise next probe is the
  disciplined close.

- _Sprint 41 (R4.5 post-mission SAG-state capture):_ **Doing exactly the measurement the prior retro
  scoped turned a "try the skip again with a better predicate" hope into a *proof* that no such predicate
  exists — and that proof is more valuable than another reverted attempt.** S40 guessed the skip needed a
  "safe field" and banked finding it; S41 measured it and found the corrupt SAGs are byte-identical to
  healthy ones in *every* safe field. Lessons: (1) **a negative result earns its keep when it's
  load-bearing** — "no safe predicate exists (here's the byte-identical capture)" permanently closes a
  direction two sprints chased, vs S40's "this particular predicate was unsafe" which left the door open.
  (2) **measure the discriminator, not just the bug** — dumping the corrupt SAG alone (S40's plan) would
  have shown `type`=160 and tempted another `type`-based skip; dumping corrupt *and* healthy side-by-side
  is what proved no field separates them. The control group is the experiment. (3) **keep the spike's
  infrastructure when it's reusable** — unlike S40's full revert, `BOB_TRACE_SAG` stays (default-off): the
  type/target-source pass needs exactly this trace, and the S35/S38 repro toggles already set the
  precedent. (4) **the reframe is the deliverable** — recognizing the crash as a *two-garbage-field family*
  (`type` + `target`, each an engine-declared bound that doesn't enforce on Linux) and that per-method
  honouring is fragile (the `a`-deref-after-check at 2801) is what tells the next session to go for the
  source, not the 9th clamp. (5) **honour the working agreement under autonomy** — with the PO away and the
  source-dig genuinely open-ended, the disciplined move was to ship the definitive spike and pivot to an
  unimpeded story, not burn the window chasing a fix the evidence says needs its own focused pass.

- _Sprint 40 (R4.5 SAG-skip — negative result):_ **A sound strategy with the wrong primitive — and
  verifying on `:0` caught it before it shipped.** The funnel-skip was the right idea (S39's retro), but I
  reached for the corruption signal I already had (`type` out of range) without checking it was *safe to
  read* on every SAG the loop touches — it wasn't (the `(info_grndgrp*)` cast reads `type` at the wrong
  offset for other SAG views, and a complex-field `Evaluate()` can deref). The skip turned a post-mission
  crash into a *flight* crash. Lessons: (1) **a guard predicate must be safe on the whole set it filters,
  not just the bad element it targets** — I validated the predicate against the corrupt SAG, not against
  the healthy in-flight SAGs it would also run on; for a hot loop over a heterogeneous set, the predicate's
  domain is the loop, not the bug. (2) **verify the fix on the same path that exercises everything** — the
  `:0` repro flies *and* advances post-mission, so it surfaced the flight regression immediately; a
  post-mission-only test would have shipped a flight crash. (3) **negative results are sprint output worth
  committing** — recording "type-predicate skip is unsafe, here's why, here's the safe-invariant /
  type-source path" stops the next attempt (mine or MA's) from repeating it. (4) **know when an approach
  needs a fresh run, not another patch** — the safe predicate wants an empirical SAG-state capture, and
  forcing it at the tail of an 8-sprint session is how the *previous* sprint's good fix gets undone;
  reverting to S39-clean and scoping the next pass is the disciplined close.

- _Sprint 39 (R4.5 post-mission GetCruiseAt fix):_ **Landing the fix immediately revealed it was the wrong
  altitude — the verification's *next* crash taught more than the fix itself.** The S38 candidate
  (bounds-honor the array index) worked exactly as designed: `GetCruiseAt` stopped crashing. But the very
  next layer (`SAGDecisionPreCombat`, then the `MoveAllSAGs` loop) made plain that all the post-mission
  crashes are *one broadly-corrupt SAG* hit through different methods — so clamping each method is
  whack-a-mole, and the real fix is one level up (skip the corrupt SAG, or fix why it's corrupt). Lessons:
  (1) **ship the small verified fix AND read the next failure before declaring the approach** — the
  per-method clamp is a legitimate, low-risk advance, but the next backtrace reframed the *strategy* from
  "bounds-honor 18 sites" to "one SAG-level skip"; that re-frame is the sprint's real value. (2) **the
  post-mission grind rhymes with post-load (S35→S37)**: spike → point-fix → systemic funnel-fix, and
  recognizing the rhythm means S40 can jump straight to the funnel (`MoveAllSAGs`) instead of grinding
  methods. (3) **a fix that's destined to become dead code can still be worth shipping** — once the
  SAG-level skip lands the corrupt SAG won't reach `GetCruiseAt`, but the clamp is cheap defense-in-depth
  and unblocked the verification that produced the strategic insight. (4) restraint at session end:
  identified the `MoveAllSAGs` choke point but did NOT rush the SAG-level skip (needs a reliable
  "is-this-SAG-corrupt" invariant) — banked it scoped rather than guess at the end of a long session.

- _Sprint 38 (R4.5 post-mission crash spike):_ **Matching the tool to the bug — not defaulting to ASan —
  was the unlock; ASan's own instrumentation hid the target crash behind an earlier one.** The post-mission
  repro needed a real flight (GL `:0`), and under ASan the run died first in an in-flight render
  use-after-free (a *separate* anim-buffer-lifetime bug whose race ASan's slowdown widens) — never
  reaching the post-mission phase. Switching to **gdb on the release build** (which flies past the UAF at
  native speed) got the post-mission backtrace cleanly. Lessons: (1) **ASan is not always the right
  oracle** — for a crash gated behind a timing-sensitive earlier path, an instrumented build can change
  *which* bug fires first; keep gdb-on-release in the kit for "crashes only at full speed / after a long
  setup." (2) **a hard-to-reach repro is worth building as infrastructure** — `BOB_POSTMISSION_FF` + the
  load→fly→return chain is reusable for the whole post-mission grind (as `BOB_POSTLOAD_FF` was for
  post-load). (3) **the systemic shape repeats** — post-mission `GetCruiseAt` is S37 again (a stale field,
  here a ground-group's `type`, indexes a table OOB), which means the S37 bounds-honor pattern is the ready
  candidate; recognizing the repeat turns "new deep crash" into "known pattern, scoped fix." (4) **spikes
  surface bonus bugs — log them, don't fold them in** — the in-flight lens/sun-anim UAF is real and
  distinct; banking it as its own item keeps the post-mission story clean. Honest scope: 5 of ~8 pts as
  characterization (repro + root cause + bonus finding); the multi-site fix + GL re-verify is its own pass.

- _Sprint 37 (R4.5 post-load reference-audit):_ **The PO's "step back and audit" call was right — one
  systemic fix cleared a family that was shaping up as a multi-sprint point-fix grind.** Two prior sprints
  were converging on "fix each stale reference in `FixupAircraft`" (formation pointers S36, target UIDs
  next, …). Auditing the family instead revealed the shared failure *mechanism* sits one level below all
  of them: every variant ends at `ConvertPtrUID` indexing `pItem[]` with a garbage UID. The function's own
  `assert` already declares the bounds — honoring it on compat (the established R1.3b/4.3c
  non-halting-assert pattern) retired the whole fatal family at once, and the post-load sim went from
  "crashes on the first detection" to "advances the full day." Lessons: (1) **when point-fixes share a
  funnel, fix the funnel** — N stale references all crash at *one* unchecked deref; guarding that deref
  (per its own contract) beats N restorations *for crash-removal*. (2) **distinguish crash-removal from
  fidelity-restoration and be honest about which you shipped** — this stops the crash but resolves stale
  UIDs to NULL, so a loaded raid may not re-acquire its target; that's a real (banked) fidelity gap, not a
  complete fix, and the retro/PORT say so plainly. (3) **re-litigating a prior "no" is sometimes correct**
  — the S28/S29 retros rejected a "blanket `ConvertPtrUID` sentinel," but that was a *fake-valid* sentinel
  masking a *different* bug (the terminator overflow); returning NULL for a UID the game itself declares
  illegal is a categorically different, contract-honoring change. Reading *why* the prior decision was made
  let me tell "same mistake" from "superficially similar but sound." (4) **the audit's by-product is the
  next backlog** — the per-reference deserialise gaps (`targetindex`/`SGT` staleness) are now precisely
  scoped faithful follow-ups, and the post-*mission* `GetCruiseAt` path is confirmed still separate.

- _Sprint 36 (R4.5 post-load formation-pointer fix):_ **The previous sprint's banked candidate fix landed
  cleanly because S35 had already done the hard part — locating the exact field and the right hook.** S35
  resisted the temptation to fix blind at session end; S36 implemented it in ~10 lines at the
  natural `FixupAircraft` hook (where the engine *already* does post-load pointer reconversion for
  waypoints/squadrons — it just omitted the formation pointers). Lessons: (1) **a deferred fix with a
  precise root cause is cheap to resume** — the cost of S35's "characterize, don't force it" was one
  sprint's latency, and it bought a low-risk, correctly-placed fix instead of an end-of-session guess;
  (2) **the right fix location is "where similar work already happens"** — `FixupAircraft` reconverting
  the other deserialised pointers was the tell that formation pointers belonged there too, not at the
  crash site; (3) **ASan-grind discipline: verify the fix eliminated the target crash AND classify the
  next one before claiming victory** — the new SEGV (`SAGExecuteWaypoint→ConvertPtrUID(SquadTarget)`) is
  on a *different* code path (target-UID, not formation-pointer), confirming my NULL reset didn't cause
  it; reading that backtrace immediately is what distinguishes "advanced the grind" from "broke
  something." (4) **the multi-bug grind is real and finite** — post-load is now CountFormationSize (fixed)
  → SquadTarget (next); each layer is a stale-deserialised-reference of the same family, so the eventual
  systemic fix may be "audit every `//save` raw pointer/UID for a missing post-load reconvert" rather than
  N point-fixes. Worth weighing at S37: one more point-fix vs. a load-boundary reference-audit pass.

- _Sprint 35 (R4.5 post-load sim crash spike):_ **Reproducing under the right tool first paid off twice —
  it corrected a wrong banked characterization AND localized the real bug in one run.** The backlog
  carried the post-mission/post-load crash as the `GetCruiseAt`/`Plane_Type_Translate` family (from the
  S26 post-*mission* gdb pin). Building a dedicated post-*load* repro (`BOB_POSTLOAD_FF`) and running it
  under ASan showed the post-load first-crash is a *different* bug entirely — a SEGV in the radio-chatter
  intel path (`CountFormationSize` walking a stale `fly.leadflight`). Lessons: (1) **two superficially
  similar crashes ("the SAG sim crashes after a mission/load") can be distinct bugs** — don't let one
  gdb pin stand in for a whole family; build the specific repro before assuming. (2) **investing in a
  reusable repro toggle is worth a sprint slot** — `BOB_POSTLOAD_FF` (like the ASan oracle itself, and
  `BOB_TRACE_ICONS` last sprint) turns a vague "it crashes somewhere post-load" into a one-command
  deterministic ASan trace, and it'll serve the rest of the R4.5 grind. (3) **the `//save` annotation in
  the struct was the smoking gun** — once ASan pointed at `fly.leadflight`, reading the field's save-format
  comment explained *why* it's stale (serialized raw pointer), turning "garbage pointer" into a precise,
  fixable root cause (S29 `RecostRaidList` class). (4) **knowing when to stop**: the fix is a load-boundary
  formation-pointer reset that risks save-format/AI regression — an end-of-session blind edit there is
  exactly the masking the S26/S29 retros warn against, so banking the precise root cause + the repro is
  the honest increment (cf. S15/S17/S27 spike-only sprints). Cost note: spent the slot's budget on
  investigation, delivered 5 of ~8 pts as characterization — appropriate for a genuine multi-layer grind.

- _Sprint 34 (R4.2 strategic-map unit icons):_ **Re-measuring a banked "root cause" beat trusting it — the
  S27 spike correctly found the symptom (empty `inter` clip rect) but mis-attributed the cause (the
  CMapDlg scroll/world transform "never set up"), which would have sent this sprint down a 13-pt
  rebuild-or-adopt-MA's-map-view path.** Re-running with a fresh `BOB_TRACE_ICONS` trace showed the
  transform is *fine* — the terrain tiles render with the same `m_scrollpoint`/`m_zoom`, and the world
  rect was non-degenerate, just centered far from every item because it was derived from a `(0,0,0,0)`
  per-block `inter`. The actual fix was **one line** (restore the game's own pre-optimization single
  `DrawIcons(pDC,bounds)` call), not a subsystem. Lessons: (1) **a spike's root cause has a shelf life** —
  when you return to a banked finding, re-verify the *attribution* with a live measurement before
  committing scope to it; the symptom ("empty clip rect") was durable but the *why* wasn't. (2) **find
  the working sibling and ask why it differs** — the terrain tiles rendering correctly was the key
  disproof of "transform not set up"; when X is broken but a sibling Y on the same machinery works,
  the bug is in what's *unique* to X (here, the per-block `inter` vs the tiles' direct rect), not the
  shared transform. (3) **the original game code is a fix oracle** — the DEADCODE single-call `DrawIcons`
  was the intended pre-Windows-optimization path; the Linux-faithful fix was to restore it, not invent
  one. (4) Counting the funnel (`scan`→`cull_pass`→`drawn`) localized the failure stage in one run, the
  same predicate-instrumentation lesson as S27 — pays off again. Cross-port: MA fixed the same empty-clip
  class differently (`GetBoundsRect→DCB_RESET→GetClientRect` fallback in its single-call DrawIcons);
  noted the architecture difference back to MA.

- _Sprint 33 (general OCX eventsink adopted):_ **Adopting the sister port's proven infrastructure paid
  off fast — but "proven on MA" is not "compiles on BoB", and the delta was a build-shape mismatch, not
  a logic bug.** The eventsink design ported almost verbatim (3 files, ~90 lines) and both targeted
  bridges fell out cleanly because the genuine `BEGIN_EVENTSINK_MAP`s already named the right handlers —
  the bridges had been hand-mirroring those maps all along. The one real fight was the **unity build**:
  BoB `#include`s several `.cpp` into one TU, so MA's `__LINE__`-based registrar name collided across
  concatenated files (`BobEvtAuto_120` twice). The fix (`__COUNTER__` via an `_IMPL` indirection so it's
  captured once, not re-incremented per textual use) is a generally-useful idiom for any per-TU-unique
  generated symbol. Lessons: (1) **when porting cross-build infrastructure, the integration risk lives in
  the build model** (unity vs per-TU, link flags, static-init), not the algorithm — sanity-check those
  before assuming a drop-in; (2) **a no-op-macro layer hides latent referencing bugs** — the 83
  `BEGIN_EVENTSINK_MAP` TUs compiled as no-ops for months; turning them real surfaced every stale handler
  reference at once (here, none were stale — the fallback template + complete DECLARE coverage held), so
  budget a full rebuild as part of "turn the macro real", not a follow-up; (3) **verify the deep path
  fired, not just that it built** — `[evt_fire] ... HANDLER CALLED` with the concrete RTTI type name
  (`5CLoad`, `11SController`) is the signal that proves RTTI dispatch resolved, which a green build can't.
  Cross-port discipline held: the `__COUNTER__` finding went straight back to MA (latent there until it
  unity-builds), mirroring how MA's ASan + eventsink came to BoB.

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

### Release 6 — Multiplayer (PO 2026-08-28)

> PO: *"get multiplayer working"*. Already declared **in scope** by the §1 vision
> (*"multiplayer is in scope"*, 2026-06-17); this makes it a release with stories.

**This is a BACKEND gap, not a feature to write.** The game's own multiplayer code is present and
compiled: `class DPlay` (shared with MiG Alley), the `Aggrgtor` packet layer
(`SRC/COMMS/Aggrgtor.cpp` — `allpackets.player[n].IDCode`, `PIDC_PACKETERROR`,
`PIDC_DUMMYPACKET`, `ReservePackets[n].GetTemp()/GetCurr()`), and a **populated multiplayer UI**:
`LOCKER.CPP` fills `RESCOMBO(DEATHMATCH0,8)` (:184) and `RESCOMBO(TEAMPLAY0,8)` (:187) and reads
`_DPlay.GameIndex` (:264). The main menu's **item 2 is "Multi-Player"** (`BOB_DUMP_MENU`, S316)
and it navigates. What is missing is the transport underneath.

⚠️ **THE GAP, CORRECTED (S323).** An earlier note in MA's PO-76 said "nothing defines
`DirectPlayCreate`" — true but IRRELEVANT, because **the game never calls it**.
`DPlay::CreateDPlayInterface()` (`SRC/COMMS/Comms.cpp:807`) builds the object through **COM**:

```c
res = CoCreateInstance( CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER,
                        IID_IDirectPlay4A, (LPVOID*)&lpDP4 );
```

and compat's `CoCreateInstance` (`SRC/compat/objbase.h`) is a blanket stub — `*ppv = NULL;
return E_NOINTERFACE;` for **every** CLSID. So: `CoCreateInstance → E_NOINTERFACE` ⇒
`CreateDPlayInterface() FALSE` ⇒ `UIMultiPlayInit() FALSE` ⇒ `StartCommsSession() FALSE` ⇒ the
not-connected box. **The single entry point to implement is `CoCreateInstance(CLSID_DirectPlay)`
returning a socket-backed `IDirectPlay4A`**; the vendored `dplay.h` is exactly the vtable to fill.

⭐ **CROSS-PORT — WRITE THE SHIM ONCE.** MiG Alley has the *same* `DPlay` class, the *same*
`Aggrgtor`, **the same `CoCreateInstance(CLSID_DirectPlay, …, IID_IDirectPlay4A)` call and the same
blanket stub** — verified in both trees. Whichever port
implements the socket-backed vtable first, the other adopts it — as with RLE8 decode and the D3D7
refcount fix. Keep `doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md` == `~/ma/port/BOB_PORT_LESSONS.md` in
sync.

| ID | Story | Pts | ☐ |
|---|---|---|---|
| R6.1 | **Connectivity gate FIRST, before any UI work.** ✅ **DONE for the FRONT DOOR (2026-08-28).** `tools/bob_mp_connect.sh` is green: clicking Multi-Player creates the DirectPlay object, enumerates a provider, and **reaches the lobby (artnum 27920)** instead of the not-connected box — i.e. `StartCommsSession()` now returns TRUE. `SRC/compat/bob_dplay.cpp` (new) subclasses the game's own `IDirectPlay4` so the compiler lays out the 53-entry vtable; the 49 stub overrides were GENERATED from `SRC/H/DPLAY.H` rather than typed. Hooked into the existing `bob_com_create_instance` dispatcher. **Negative control is real and runs every time:** `BOB_NO_DPLAY=1` restores `E_NOINTERFACE` and the gate REQUIRES that arm to stay on the main menu — measured, it does. ⬜ **The packet half moves to R6.2**: two processes exchanging a packet needs `Open`/`Send`/`Receive`, which this step deliberately does not implement. | 8 | ◐ |
| R6.2 | **`IDirectPlay4` over sockets in `SRC/compat/`** — ◐ **TRANSPORT WRITTEN, NOT YET PROVEN END-TO-END (2026-08-28).** 17 methods now real (was 4): `InitializeConnection`, `Open` (CREATE binds UDP / JOIN targets the host), `EnumSessions` (probe → offer → callback; **no host ⇒ empty list and `DP_OK`, which is the honest answer, not an error**), `CreatePlayer`/`DestroyPlayer`, `Send`/`Receive`/`GetMessageCount`, `Close`, `GetCaps`, `Get`/`SetSessionDesc`. 36 stubs remain and still log themselves. **The method set was OBSERVED, not chosen from the header** — `BOB_TRACE_DPLAY=1` made the game name each one. Plain UDP on one socket, and deliberately **no sequencing/retransmission**: the game's own `Aggrgtor` already does reserve packets and loss handling, and a second protocol beside it would be the harder bug. A `pump()` on `Receive`/`GetMessageCount`/`EnumSessions` lets a host answer probes while merely idling — no extra thread. ✅ **PACKET PROVEN (2026-08-28), `tools/bob_mp_packet.sh`:** two processes, discovery → join → a packet across. `control: no host -> 0 sessions | host RECEIVED the client's packet | client found the session and sent` — *"RECEIVED 22 bytes from pid 1: hello from the client"*. It drives the object through the **same COM entry point the game uses**, needs **no display** (pure sockets, so it never queues behind `gl-lock`), and its **solo arm is a real negative control**: discovery must come back EMPTY with no host, so a passing host/join arm means a packet actually crossed. ⬜ **The UI path is still unproven, but NOT for the reason I recorded.** ⚠️ **CORRECTION (S319):** I wrote that the lobby's `Create Game` click "does not fire", from an unmeasured guess that `BOB_AUTOCLICK` advances per screen PAINT. **Both halves are wrong.** The dispatch is `++t % 120` — one step per 120 **TICKS** — and measured, **Create Game DOES fire**: `click (175,747) -> menu item 1` → `[dplay] InitializeConnection (TCP/IP provider selected)` → a NEW lobby screen (a different `27920` instance). My original test was truncated by a short timeout and a `head -14`, and I attributed my own truncation to the product. (The wrong "per screen PAINT" claim came from a stale comment at `FULLPSYS.CPP:1498`, now corrected in place.) **What is actually left:** the create-game screen needs further navigation before it reaches `Open(DPOPEN_CREATE)` — enumerate it with `BOB_DUMP_MENU` and drive it, exactly as S316 did for the Fly path. The one real constraint remains `gl-lock` serialising the display, so host+client cannot both run the front end; the transport gate sidesteps that by needing no display at all. ✅ **Player-ID gap CLOSED (R6.3, same day):** each object used to start `nextPid` at `DPID_SERVERPLAYER`, so host and client both allocated **pid 1** — packets still crossed, but every player was indistinguishable and the `Aggrgtor` addresses its packets BY pid. The host now owns the id space: `MSG_JOIN` → host mints an id and replies `MSG_ASSIGN`; the client waits briefly for it and `CreatePlayer` honours it. Verified — `RECEIVED 22 bytes from pid 2` (was pid 1), and **the probe now ASSERTS it**, failing if a client ever gets `DPID_SERVERPLAYER`. ⭐ Found by READING the R6.2 trace, not by a failure: a two-node echo cannot expose an id collision, so the passing gate was hiding it. | 13 | ◐ |
| R6.3 | **The lobby screens work** — ✅ **THE GAME HOSTS A SESSION THROUGH ITS OWN MENUS (2026-08-28).** `Multi-Player → Create Game → Continue` now produces: `host bound to UDP 47624` · `Open(CREATE) session "BoB"` · `CreatePlayer → pid 1` · `CreateGroup → gid 2` · `CreatePlayer → pid 3` · `AddPlayerToGroup 3 → 2`, then sits in the lobby awaiting players (rc=124 = correct host behaviour). **No unimplemented method remains on the host path.** Screens enumerated with `BOB_DUMP_MENU`, as S316 did for Fly — the create-game screen is `0 Back · 1 Continue · 2 Load Game`. Added this sprint: the **group family** (`CreateGroup`/`AddPlayerToGroup`/`EnumGroups`/`EnumGroupPlayers`, with real bookkeeping so enumeration answers truthfully rather than returning `DP_OK` and recording nothing) and **`SendEx`** (forwards to `Send`; the game calls it every frame once a session is live). ⬜ Remaining: a second instance JOINING through the UI — blocked only by `gl-lock` serialising the display, which is exactly why the transport gate needs none. | 8 | ◐ |
| R6.4 | **Two clients fly the same mission** — the `Aggrgtor` packet layer carries positions both ways; measure and RECORD packet rate and observed desync. | 13 | ☐ |
| R6.5 | **Soak + drop-out** — a peer leaving mid-mission must not crash or hang the survivor. ASan-clean over the soak. | 8 | ☐ |

⚠️ **Do not start at the UI.** The lobby renders and navigates already; it is the transport that
returns FALSE. Starting at the screens would produce motion without progress — the R1 lesson
(S313–S317), where three real defects sat underneath a harness that looked like a navigation
problem.

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

### R3.10 (from R3.7, 2026-08-28) — audit the conversions that `-w` hides

R3.7 was a `RDialog*` silently truncated into an `int` **slot index**
(`MAINFRM.CPP:1795`). It compiled because the build carries `-fpermissive`
(invalid conversion → warning) *and* `-w` (warning → nothing). Recompiling the
`_MFC.CPP` unity TU with warnings on emits **7** such conversions; R3.7 was one.

Triage of the other six — all size-preserving on i386, none currently suspected
of being a live defect:

| site | conversion | assessment |
|---|---|---|
| `fullpsys.cpp:2377` (×2) | `LONG*` → `LONG_PTR*` | both 4 bytes here; a type-pun, not a truncation |
| `MainFrm.cpp:1197` | `HTASK` (`void*`) → `DWORD` | 32-bit target, value preserved |
| `MIG.cpp:311` | fn-ptr → `void*` | benign on i386 |
| `afxwin.h:450`, `resource.h:121` | `int` → `LPCSTR` | the `MAKEINTRESOURCE` idiom |

**The dangerous class is specifically pointer → integer where the result is used
as an INDEX, COUNT or SIZE** — that is what made R3.7 a wild read rather than a
harmless reinterpretation. This audit is about finding more of *that*, not about
zeroing the warning count.

⚠️ Do **not** simply drop `-w`: this is 1990s Win32 source and the noise floor is
enormous. The useful move is a one-off build with `-Wconversion-null -Wall` piped
through a filter for pointer→integer conversions, run over ALL unity TUs (only
`_MFC.CPP` has been examined), with each hit triaged for how the value is USED.

**Done when** every pointer→integer conversion in the tree is listed with a
use-site assessment, and any that reaches an index/count/size is fixed or has a
recorded reason it is safe.

☑ **DONE (2026-08-29).** All **98** unity TUs recompiled with `-w` swapped for `-Wall` and every
`invalid conversion` collected. **13** in the whole tree; exactly **ONE** is pointer→integer:

| site | conversion | use | verdict |
|---|---|---|---|
| `MFC/MainFrm.cpp:1197` | `void*` (`HTASK`) → `long unsigned int` (`DWORD`) | `CFrameWnd::OnActivateApp(bActive, hTask)` — passed straight to the base handler, never indexed or sized | **safe on i386** (both 32-bit, value preserved); revisit on any 64-bit port |

The other 12 are not this bug class: six `int → Angles` (a typed-scalar constructor), two
`int → LPCSTR` (the `MAKEINTRESOURCE` idiom), two function-pointer casts, and `long int* → int*`
twice at `fullpsys.cpp:2377` (a pointer-to-POINTER pun, same width here — not a truncation).

**So R3.7 was the only live instance of the class, and it is fixed. The tree is clean of
pointer-as-index.**

⚠️ **THE AUDIT REPORTED A CLEAN TREE TWICE BEFORE THIS, BOTH TIMES WRONGLY:**
1. **"0 across 98 TUs"** — the grep used `.` for GCC's quote characters, which are Unicode `‘ ’`
   (3 bytes each in UTF-8), so the pattern matched nothing at all.
2. **"0 pointer→integer"** — the regex captured the ALIAS name (`HTASK`), which contains no `*`;
   the pointer is visible only in the `{aka ‘void*’}` expansion.

Both were caught by re-running the sweep against a TU already known to emit 6. **A zero from a wide
audit is the most comfortable possible result and the easiest to be wrong about — run it against a
known positive before believing its silence.**

### R5.x (PO 2026-08-28) — centre-square padlock on the "S" key

PO: *"during a dogfight, imagine the forward view as a 3x3 grid. When you press the 'S' key, you
padlock the closest aircraft in the center square of that imaginary grid, if there is a bogie within
that square, otherwise the keypress has no effect. This was added as a bob enhancement in one of the
patches. It turns a corkscrewing effort to outturn a locked up bandit in your rear quadrant while
taking potshots from his pals in a melee into a manageable repeated boom-and-zooms into a furball."*

**This is NOT the existing padlock.** The engine already has one — `VM_InPadlock` / `VM_OutPadlock`
(`SRC/H/viewsel.h:83`) toggled by `PADLOCKTOG`, bound to **Enter** (`SRC/H/keymaps.h:1183`, also
joystick `A1_b2`/`A3_b2`). That one cycles/toggles a lock on whatever the engine considers current.
What the PO describes is a **narrow acquisition rule**, and the narrowness is the whole point:

1. project candidate aircraft into forward-view screen space;
2. keep only those inside the **centre cell of a 3x3 split of the view** (i.e. |x| and |y| within
   1/6 of the view width/height about the centre — confirm against the patch, do not assume);
3. of those, lock the **nearest**;
4. **if the centre cell is empty, do nothing at all** — no nearest-overall fallback, no cycling.

⚠️ Step 4 is the requirement most likely to be "improved" into uselessness. A padlock that grabs
*something* whenever you press it is the behaviour the PO is escaping: it locks a bandit in the rear
quadrant and forces the corkscrew. The value is that the key is a **deliberate, aimed** acquisition —
you point the nose at who you want, then press. Silence on an empty centre cell is a FEATURE.

**Provenance — RESOLVED (2026-08-29).** It is a **BDG** feature (the community Battle of Britain
Developer's Guide patch), not a Rowan one. `bdg.txt` in the install carries the settings by name:

```
DRAW_PADLOCK_CENTER_BOX=OFF      <-- the PO's centre square, by name
BOB_PADLOCKFIX=OFF
PADLOCK_OVERRIDES_TRACKIR=OFF
NO_HEAD_BOBBING_WHILE_PADLOCK=OFF
```

* The installed executable **is** the BDG 0.99 build and contains those strings (3 occurrences
  each), so the behaviour is implemented **in the patched binary**.
* **None of these keys appears anywhere in `SRC/`** (checked with `grep -a`, which matters — plain
  `grep` silently skips ~46 % of this tree). So there is nothing dormant to switch on: the port must
  IMPLEMENT it. What the patch gives us is an oracle, not code.

⭐ **AND THE ORACLE CAN BE MADE TO DRAW ITS OWN ANSWER.** Setting `DRAW_PADLOCK_CENTER_BOX=ON` in
`bdg.txt` and running the BDG build under Wine renders the acquisition region on screen. That
settles, by measurement rather than by choice, the questions this entry lists below:
* **cell or cone** — capture the drawn box at two different FOVs. A 1/3 x 1/3 grid cell scales with
  FOV; an angular cone does not. One screenshot pair decides it.
* **exact extent** — measure the box in pixels against the viewport, rather than assuming "one
  third".

Do this BEFORE writing any geometry. Gold-standard behaviour is what the real game does, and here
the real game will literally draw it for us.

#### R5.x-S1 (2026-08-29) — the oracle RUNS. Recipe recorded; one obstacle left before the capture.

**The BDG build launches under Wine and reaches its own window.** Two things had to be got right,
neither obvious, both now written down so the next attempt starts here instead of rediscovering them:

1. **The system `wine` cannot run it.** `wine-10.0` is a 64-bit loader and the prefix is 32-bit:
   `wine: '/home/admin/sgl/TUE/BattleOfBritain/WP' is a 32-bit installation, it cannot support
   64-bit applications.` Use a 32-bit loader from Lutris —
   `/home/admin/.local/share/lutris/runners/wine/lutris-8.0-x86_64/bin/wine` — with `WINEARCH=win32`.
2. **Wine's Mono installer blocks the launch.** Suppress it with
   `WINEDLLOVERRIDES="mscoree=d;mshtml=d"`, else a `Wine Mono Installer` window takes the session
   and `bob.exe` never gets there.

Working command:

```bash
cd "$GD" && DISPLAY=:0 WINEPREFIX=/home/admin/sgl/TUE/BattleOfBritain/WP WINEARCH=win32 \
  WINEDEBUG=-all WINEDLLOVERRIDES="mscoree=d;mshtml=d" \
  /home/admin/.local/share/lutris/runners/wine/lutris-8.0-x86_64/bin/wine bob.exe
```

With that, `bob.exe` runs indefinitely (killed only by `timeout`) and creates a **fullscreen
1920x1080 window titled `BoB` at (0,0)**.

**`DRAW_PADLOCK_CENTER_BOX` is now `ON`** in the install's `bdg.txt` (line 58), backup at
`/tmp/bdg.txt.backup-162307`. It was `OFF`. **Left ON deliberately** so the capture can be taken
without re-editing; restore from the backup if the PO wants the shipped config back.

⚠️ **BLOCKER — a modal sits BEHIND the fullscreen window.** On first run the imported-settings
dialog appears: *"Your pre-BDG 0.96 configuration file has been imported."* (279x101 at 854,505).
It is in the window tree but **obscured by the fullscreen `BoB` window**, so it cannot be seen or
clicked: `xdotool` `Return` and a synthetic click at its OK position both left it standing, and an
`x11grab` of its rectangle returns the game's black, not the dialog. Until it is dismissed the
front-end does not proceed.
**Next attempt should try, in order:** run the game WINDOWED (a BDG/`bdg.txt` or registry setting)
so the modal is reachable; or `xdotool windowraise` the dialog before clicking; or dismiss it once
by hand — it is a first-run-only dialog, so one manual OK likely clears it for every later run.

#### R5.x-S2 (2026-08-29) — four ways past the modal tried, all failed. **This needs 30 seconds of PO time, and that is now the cheapest path.**

The modal is REAL and LIVE, not a stale window: `_NET_WM_PID` on the dialog and on the `BoB` window
both return the running `bob.exe` (pid 1400570). Its X id repeating across runs is just Wine
allocating ids deterministically, not a leftover.

Tried, in order, all unsuccessful:
1. **Wine virtual desktop** (`wine explorer /desktop=bobdesk,1280x1024 bob.exe`) — the usual trick
   for trapped modals. `bob.exe` **exits immediately and silently** under it (log holds only the
   harmless `winemenubuilder` line). Not viable, and worth recording so it is not retried.
2. **`xdotool windowraise` + `windowactivate`** on the dialog — it stays behind the fullscreen
   surface.
3. **Synthetic `Return` and a click at the OK position** — dialog still standing afterwards.
4. **Moving the dialog to the FREE SECOND MONITOR** (`windowmove 2100,300`, where nothing covers
   it) — an `x11grab` of its own rect there is still **uniformly one colour**.

That last result is the informative one: even with nothing on top of it, the dialog captures as a
flat colour. Consistent with Wine's **DirectDraw fullscreen-exclusive** mode suppressing ordinary
window painting — the same class of trap as MA-S233 (a capture tool reporting black for a rendering
app), so the pixels cannot be trusted either way and the window-tree facts above are the only solid
evidence.

**RECOMMENDED NEXT STEP — PO, ~30 seconds, once ever.** Run the command below, click **OK** on
*"Your pre-BDG 0.96 configuration file has been imported."*, then quit. It is a FIRST-RUN dialog, so
one manual dismissal should clear it for every later automated run, after which the two-FOV capture
can proceed unattended:

```bash
cd "/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
DISPLAY=:0 WINEPREFIX=/home/admin/sgl/TUE/BattleOfBritain/WP WINEARCH=win32 \
  WINEDLLOVERRIDES="mscoree=d;mshtml=d" \
  /home/admin/.local/share/lutris/runners/wine/lutris-8.0-x86_64/bin/wine bob.exe
```

`DRAW_PADLOCK_CENTER_BOX=ON` is already set in `bdg.txt`, so the box will be drawn once a flight is
reached.

**Still outstanding, unchanged:** capture the drawn box at TWO different FOVs (a 1/3x1/3 grid cell
scales with FOV, an angular cone does not) and measure its extent in pixels against the viewport.
No geometry should be written before that pair of screenshots exists.


⛔ **NOT DONE BY ME, DELIBERATELY — THIS ONE NEEDS THE PO (2026-08-29).** The measurement means
editing `bdg.txt` in the PO's own Battle of Britain install and then RUNNING the BDG 0.99 build under
Wine. That is their game directory: the run can rewrite `settings.cfg` (S151 records that this build
rejects its own saved settings on a date mismatch and silently reverts to factory defaults) and
touch saves. **I am not modifying a working install to take a measurement**, and the change is a
one-line edit for the PO.

**What the PO would do, and what to capture:**
1. in `bdg.txt`, set `DRAW_PADLOCK_CENTER_BOX=ON` (it currently reads `OFF`);
2. fly, padlock a bogie, and screenshot the forward view **twice — once at each FOV extreme**
   (`FOV_SMALL = 25` and `FOV_LARGE = 80` are both already in that file);
3. send both shots.

**Why two FOVs and not one:** a 1/3 x 1/3 GRID CELL scales with the field of view; an angular CONE
does not. One screenshot shows a box and settles nothing; the pair settles which of the two the
patch implements, which is the open question this whole item turns on. Measure the box in pixels
against the viewport in each.

◐ **CONFIG READ (2026-08-29): `bdg.txt` carries NO dimensions for the box — only the on/off flag.**
113 lines; the padlock entries are all boolean (`DRAW_PADLOCK_CENTER_BOX`, `BOB_PADLOCKFIX`,
`PADLOCK_OVERRIDES_TRACKIR`, `NO_HEAD_BOBBING_WHILE_PADLOCK`). The geometry is compiled into the
patch, so **draw-and-measure is the only route short of disassembly** — which is what the plan above
already says, now confirmed rather than assumed.

◐ **THE EXE HAS A FIFTH PADLOCK SETTING THAT `bdg.txt` DOES NOT LIST (2026-08-29):
`DEATH_BREAKS_PADLOCK`.** Present in the executable's strings, absent from the shipped 113-line
config. **So the config is NOT an inventory of the patch's padlock behaviour** — reading only
`bdg.txt` under-counts what BDG implements, which matters when deciding what "match the patch" means.

⛔ **The key question cannot be answered from the executable's strings.** Searched for binding text:
the exe carries `KEY_CONFIGMENU`, `KEY_JOYSTICKCONFIG`, `KEY_TOGGLE_DESC_TEXT` and similar, but
nothing tying a key to the padlock acquisition. So "which key, and what happened to `SUICIDE`"
still needs either the draw-and-measure route or disassembly — it is not sitting in a string table.

⚠️ **DO NOT MISTAKE `EPI_RADIUS = 50.0` / `EPI_Y_RADIUS = 10.0` FOR THE BOX.** They sit two lines
away in the same numeric block and look exactly like an acquisition region. They belong to
`ENEMY_POSITION_INDICATOR` (line 56), a different aid entirely. Anyone skimming for "the padlock
numbers" will find these first and they are the wrong ones.

**Open questions to settle from the patch, not by choice:**
* Is the cell exactly 1/3 x 1/3 of the view, or an angular cone (e.g. ±10 deg)? A grid cell scales
  with FOV; a cone does not. These differ a lot at the FOV extremes.
* "Closest" by slant range, or by angular distance from view centre?
* Enemies only, or any aircraft (the PO says "bogie", which implies unidentified/hostile)?
* Does it re-acquire when the target leaves the cell, or hold until broken?

**Which key — ⚠️ CORRECTED (2026-08-29): `S` IS ALREADY TAKEN.** The original note here said "`S` is
not currently in `keymaps.h` as a padlock binding", which was true and misleading. Measured with
`grep -a` (plain `grep` silently skips ~46 % of this tree):

```
KeyMap(SUICIDE,       s, norm)     <-- PLAIN S IS BOUND
KeyMap(SPINRECOVERY,  s, ShiftL)
KeyMap(ASCII_S,       s, ShMsg)
```

So binding the centre-square padlock to plain `S` **displaces `SUICIDE`**. Three possibilities, and
they must be told apart from the BDG patch rather than chosen:
1. BDG rebound `S` and moved `SUICIDE` elsewhere — then match what it did;
2. the PO's `S` carries a modifier this port has not recorded;
3. BDG binds it only while in a padlock-capable view, so the two coexist by context.

⚠️ The acquisition still needs its own `KeyName`/`KeyMap` entry rather than stealing `PADLOCKTOG`'s
(Enter) — but "which key" is now an open question with a wrong answer already written down once.
**Check the BDG executable's key table before assigning anything.**

**Done when** pressing the bound key with a bogie in the centre cell locks the NEAREST one, pressing
it with an empty centre cell does nothing observable, and a gate asserts BOTH arms — the empty-cell
no-op is the half that will silently rot.

**Points:** 8


---

## R3.8 — SPRINT (2026-09-02/03): ⭐ **ROOT CAUSE FOUND. The list is PAINTED, then WIPED by the frontend repaint.**

The question this entry has carried for four sprints — never populated, or populated and not drawn —
is answered, and the answer is a third thing neither branch predicted: **it is drawn correctly and
then erased.**

**Reproduced in the gate first.** `tools/bob_convoy_campaign.sh` already captures a framebuffer PPM
(`BOB_SHOT`), and that capture is the PO's screenshot exactly: background art, the
`Debrief / Back / Sim Config / Fly` row, and **no aircraft list**. So this is reproducible headlessly
and needs no PO session to work on.

**The chain, measured end to end:**

| link | evidence |
|---|---|
| hosted, visible, real rect | `[sysbox-ctl] dlgId=1164 ctrlId=1481 visible=1 dlu=(5,42,449,86)` |
| not skipped | `[skip] dlgId=1164 hosted=22 DREW=12 \| not-visible=6 not-in-template=4 no-DLU-rect=0` |
| rows laid out | `[extent] dlg=1164 ctrl=1481 rect=(27,88 673x139) contentH=162 -- rows 0..6 fit` |
| **pixels actually written** | `[pix] dlg=1164 ctrl=1481 rect=(27,88 673x139) content-px-just-after-draw=93541` |
| final frame | that same rect is **bare sky art** |

93,541 content pixels in a 673x139 box (93,547 px) means essentially the WHOLE control was painted
into the framebuffer. It is then lost.

**What wipes it — from the log, in order after the control paints:**

```
[frontend] painted screen artnum=27917 + dials + menu + presented
[frontend] LaunchMain painted artnum=27917 res=1024 + menu + presented
[frontend] painted screen artnum=0   + dials + menu + presented
```

`FULLPSYS.CPP:~1985` repaints the whole screen and presents: `DoPaint` (background art) →
`pdial[0..2]` and **their** hosted controls → `bob_draw_menu` → `bob_gdi_present`. It redraws hosted
controls **only for the three dial panels**. `IDD_BOBFRAG` is not a dial, so its controls are painted
by another path, then this repaint covers them and presents. The final frame is exactly what this
function draws — art + menu — which is why the PO sees art + menu.

**The fix (next sprint):** the frontend repaint must also redraw the hosted controls of whatever
non-dial dialogs are open, at their own origins, before `bob_gdi_present`. The origin is the part to
get right — the briefing drew at px (27,88) via its own path, and guessing an origin would move the
list rather than restore it. A dialog registry already exists (`[shot-state] ... dialogs=6 dlg=254:1/1
dlg=942:8/8 ...`), which is where the enumeration should come from. **Not attempted this sprint on
purpose:** the diagnosis is solid, a placement guess is not, and this pane has already cost sprints
to wrong turns.

**Instruments added (both env-gated, off by default):**
* `bob_gdi_rect_content(x,y,w,h)` (`SRC/compat/bob_video.cpp`) — counts non-background pixels inside
  a rect of the GDI framebuffer. This is the measurement that separated "never painted" from
  "painted then covered"; nothing existing could tell those apart.
* `BOB_TRACE_PIX=<dlgId>` (`SRC/RLISTBOX/bob_ole.cpp`) — samples that rect right after each control
  draws, restricted to ONE dialog.
* `BOB_TRACE_PAINTORDER=1` (`SRC/MFC/MAINFRM.CPP`) — the paint-tree visit order, node + artnum.

⚠️ **My first cut of `BOB_TRACE_PIX` deduped across all dialogs in a 64-entry table and went silent
before reaching the briefing** — dlg 1032 draws 156 controls by itself. That is the SAME instrument
fault that produced this entry's retracted "zero controls" reading. It is now filtered at the source
by dialog id. An instrument that stops recording when it fills reads exactly like a screen with
nothing on it.

**Gates: 13 pass.** `soak`, `r1` and `settings_nav` did not run — all three print
`REFUSING TO RUN: bob is already running (pid 2341852)`. That process has been alive **2 days**; it
is not this sprint's doing and safe-kill deliberately will not touch it, per the standing rule that
a stray bob may be the PO's own game. **It needs the PO's word before anything kills it.**


## R3.8 — SPRINT 2 (2026-09-03): 🔴 **MY OWN SPRINT-1 ROOT CAUSE IS RETRACTED. The briefing DOES render the list.**

Sprint 1 concluded the list was "painted, then buried by the frontend repaint". **That conclusion was
wrong, and the error was in the comparison, not the measurement.** I compared a pixel probe taken
*while the briefing pane was drawing* against the recipe's framebuffer capture — which fires at tick
99999, **after `Launch3d`, by which time the briefing has closed.** Two different screens. A capture
that cannot contain the thing you are looking for is not evidence that something removed it.

**What the briefing actually looks like.** Armed a capture on the dialog itself
(`BOB_DUMP_AFTER_DLG=<dlgId>:<n>`, dumping the PRESENTED frame n presents after dlg 1164 draws) and
shot the briefing while it is up. With the port **stock**, it renders:

```
Unit         Aircraft Duty        Callsign
S1/III (7)   Ju87     Dive Bomb   Checkerboard III
S1/III (8)   Ju87     Dive Bomb   Checkerboard III
S1/III (9)   Ju87     Dive Bomb   Checkerboard III
S1/III (10)  Ju87     Dive Bomb   Checkerboard III
J3/I (1)     Me109    High        Panther I
J3/I (2)     Me109    High        Panther I
J3/I (3)     Me109    High        Panther I
```

— exactly the seven squadron options S(2026-08-29) measured as populated, with the squadron/callsign
labels and "Return to Player" beside them. **On this path the feature works.**

The frame that matches the PO's screenshot (art + `Debrief / Back / Sim Config / Fly`, no list) is
the **next** screen, after the briefing closes and Fly is taken — captured at n=2, where it looks the
same with and without any change. It is a different screen, not a broken briefing.

**So R3.8 is not "the list fails to draw".** The open question is now: *what does the PO's route do
differently?* Candidates, in order: a different screen reached from Debrief rather than the frag
pane; a real-GL/interactive path the headless recipe does not exercise; or a campaign state where
`maxsquadoption` really is 0 (this recipe always has 7). **The next sprint should get the PO's exact
route** rather than test more of the one that works.

**The fix I wrote is DEFAULT OFF.** `bob_ole_replay_panels()` redraws already-drawn panels at their
OWN recorded origins before the present (never a guessed origin, which would move a control rather
than restore it). It is sound in shape and does change the final frame — it brings the system box
back into the post-briefing frame — but it fixes a burial that was never demonstrated, so it ships
disabled: `BOB_PANELREPLAY=1` enables it.

**Kept, and worth keeping** (all env-gated, off by default): `bob_gdi_rect_content()`,
`BOB_TRACE_PIX=<dlgId>`, `BOB_TRACE_PAINTORDER=1`, and `BOB_DUMP_AFTER_DLG=<dlgId>:<n>` — the last is
the one that settled this, because it is the only way to photograph a transient screen at the moment
it is up.

⚠️ **A second instrument fault, mine, worth recording**: I first "verified" with a metric that counted
pixels differing from the rect's dominant colour. Over cloudy background art that scores ~93,000
either way, so it reported the list present in both arms when one of them plainly had none. The
metric could not fail. The visual check is what caught it.

**Gates: same 13 pass / 3 blocked as sprint 1** (`soak`, `r1`, `settings_nav` refuse while the
2-day-old pid 2341852 is alive — still awaiting the PO's word).


## R3.4 — SPRINT (2026-09-03): ⭐ **THE MIRROR IS OFF BY DEFAULT, AND WHEN TURNED ON IT RENDERS A FLAT WASH**

R3.4 was filed as "`InfiniteStrip` garbage v-texcoords so the mirror shows the horizon, not a flat
edge texel". **That attribution is not supported.** Measured instead, on real GL:

**1. The mirror is not rendered at all under the shipped defaults.** `RenderMirror`
(`3DCODE.CPP:6458`) is gated on `Save_Data.cockpit3Ddetail[COCK3D_SKYIMAGES]` — the "Reflections"
setting — which `SAVEGAME.CPP:2497` **clears by default**, and which `DecDetailLevel()` clears third
when reducing detail. Across three flights with it off, `BOB_TRACE_RTT` shows the mirror surface
**never once receiving a `SetRenderTarget`**: only the 256x256 landscape composite did (40 binds,
one FBO). A texture that is never rendered into can only sample stale content — which is exactly
"a flat edge texel".

**2. It is NOT the RTT probe and NOT the FBO.** Added a one-line report of the probe's verdict:
`[rtt] CheckIfTextureCanBeRenderTarget -> 0x00000000 (DD_OK) => mirror/land RTT ENABLED`. So
`F_TEXTURECANBERENDERTARGET` is set and `LIB3D.CPP:4427` would bind the mirror. The gate is the
setting, upstream of all of it.

**3. Turned on (`BOB_MIRROR=1` — an existing switch, see below), the mirror DOES render:** two FBOs
appear (`128x128 tex=42` and `256x256 tex=43`) with **20 render-target binds each**. So the 128x128
surface is the mirror, and the whole RTT path works.

**4. …and what lands in it is a near-uniform LIGHT GREY** — `BOB_DUMP_RTT`: 19 unique colours,
mean rgb (213.5, 213.6, 214.0), no horizon, no terrain, no sky gradient. The landscape RTT beside it
in the same frame has 264 colours and real structure, so the dump path is sound. **R3.4 therefore
reduces to: the mirror FBO is bound and rendered, but the scene is not drawn into it.**

🔗 **POSSIBLE LINK TO R3.9 (the PO's floating grey square) — hypothesis, not a claim.** The mirror
texture is a flat LIGHT GREY 128x128. R3.9's suspect 2 is "an RTT surface drawn as geometry", and the
PO reports "a floating light/dark grey square… sometimes" in a dogfight. A cockpit mirror surface
painted with this texture would read as exactly that, and "sometimes" fits a mirror that is only in
view at certain angles. R3.9's own canary work eliminated the untextured-quad path, which is
consistent: this quad IS textured — with a flat grey texture. **Worth testing together next.**

⚠️ **I duplicated an existing facility and removed it.** I added a `BOB_SKYIMAGES` override in
`SAVEGAME.CPP` before finding that `MIG.CPP:669` already has **`BOB_MIRROR`**, which forces the same
flag at QM boot — the right place, and the reason my earlier override appeared to do nothing (the
settings load runs after it). A second env var for the same switch is worse than none: reverted, and
`BOB_MIRROR` is what this entry should use.

**Kept:** the probe-verdict trace in `LIB3D.CPP` (env-gated on the existing `BOB_TRACE_RTT`).

**Next sprint:** with `BOB_MIRROR=1`, find why the scene does not reach the mirror FBO — the
`RENDERTARGET_MIRROR` branch sets `clipVal2D`, clears `LANDSCAPE_TEXTURE` from
`globTextureTypeFlags` and applies `mirrorRect` as the viewport (`LIB3D.CPP:4427`); a wrong
`mirrorRect` or an empty visible-shape list would both produce a flat clear. Dump `mirrorRect` and
the shape count for the mirror pass first.


## R3.4 — SPRINT 2 (2026-09-03): the mirror pass is fully exercised, and the flat grey is CHARACTERISED

Sprint 1 left "the mirror FBO is bound and rendered, but the scene is not drawn into it". Sprint 2
measured that claim and **it was wrong in its second half — the scene IS drawn.** What is true:

| question | measurement |
|---|---|
| is the mirror bound? | yes — 20 `SetRenderTarget` binds per flight, `128x128 fbo=1 complete=1` |
| is the viewport right? | yes — `mirrorRect L=0 T=0 R=128 B=128 -> viewport x=0 y=0 w=128 h=128` |
| does anything draw into it? | **yes — 138-140 primitive draws land on the mirror surface every pass** |
| does the FBO start blank? | **yes — pure black at bind time** (mean 0, 1 colour) |
| what comes out? | **uniform grey: range 209-215, no row gradient, 19 colours** |
| textures failing? | no — `[texfail] summary: 0 uploads bailed`; the `[grey]` canary reports only the known loader-screen quad |
| fog? | ruled out — `Lib3D::EnableFogging` is entirely dead code |
| colour clear? | no — the mirror pass clears ZBUFFER only, and the compat `Clear` is faithful |

So the mirror's geometry lands and **paints a flat fill**. Black-before / grey-after is the pair that
proves it: the draws are what turn it grey.

**`InfiniteStrip` is ELIMINATED as the cause** — R3.4's filed attribution. `BOB_NO_STRIP=1` (new A/B
switch, off by default) makes the mirror *more* uniform, not less (19 colours → 2). The strip was
contributing the only variation there was.

**What the pass actually draws, read from the code** (`3DCODE.CPP:6526+`): viewer rotated 180° from
the aircraft heading, `SetProjectionMatrix(mirrorFoV, 5/2, NEARZ, RANGE_FAR_MIRROR)`, then
`RenderMirrorLandscape()` + `GetMirrorObjects()`. And `RenderMirrorLandscape` (`LANDSCAP.CPP:620+`)
draws the **HORIZON geometry** under `LF_AMBIENT` ambient-only lighting, translated
`-(viewer_y + 500)`, with `view_dist = RANGE_FAR_MIRROR` — it does not draw terrain tiles.

**Leading hypothesis for the next sprint (stated, not claimed):** the mirror is showing the
ambient-lit HORIZON DOME filling the frame — which would be a flat wash by construction — because
the terrain tiles are not part of the mirror pass. Two cheap discriminators: (1) attribute the 140
draws between `RenderMirrorLandscape` and `GetMirrorObjects`; (2) note the projection is built with a
**2.5 aspect ratio for a square 128x128 target**, which is worth checking on its own.

**New instruments, all env-gated and default-off:** the `mirrorRect`/viewport report and the
per-target draw counter (`BOB_TRACE_RTT`), a bind-time FBO dump (`BOB_DUMP_RTT_BIND`) — the one that
settled black-before/grey-after — and `BOB_NO_STRIP` for the strip A/B.


## R3.4 — SPRINT 3 (2026-09-03): the mirror is filled by the AMBIENT-LIT HORIZON, and one of my own readings was wrong

**A correction first.** I split the mirror pass's ~140 draws with phase markers around
`RenderMirrorLandscape()` and `GetMirrorObjects()`, and got `landscape=8 objects=0 other=132`. The
obvious reading — "132 unrelated draws are landing in the mirror because the target is never
released" — **is wrong**. `Lib3D::EndScene()` is where the deferred poly lists are actually rendered
(`RenderTLPolyList` / `RenderPlainPolyList` / `RenderPolyList` / `RenderTPolyList`): the collection
calls only BUILD the lists, so the 132 are the mirror's OWN geometry, flushed after my markers had
already reset. A batching renderer cannot be attributed by wrapping the calls that submit to it.

**What the sprint does establish.** `RenderMirrorLandscape` lights the horizon geometry with
`landAmbientColamb`, and that colour is now printed:

```
[rtt] mirror landscape ambientRGB=0xe5e5e5 (R229 G229 B229)
```

The mirror reads back as **(213.5, 213.6, 214.0)** — the same neutral grey at ~93% brightness. Two
independently-measured neutral greys, one the ambient the pass lights with and one the result, is
strong support for: **the mirror is filled by the ambient-lit HORIZON geometry, not by the scene.**
Stated as support, not proof — the 229 → 213 difference is unexplained (texture or shading
modulation), and identical hue is not identity.

That also fits what the code does: `RenderMirrorLandscape` (`LANDSCAP.CPP:620+`) draws the horizon
points with `view_dist = RANGE_FAR_MIRROR`, translated `-(viewer_y + 500)` — **it never draws terrain
tiles**. A mirror showing only an ambient-lit dome is flat by construction.

**Standing summary of R3.4 after three sprints** — everything eliminated by measurement, not opinion:

| eliminated | how |
|---|---|
| `InfiniteStrip` (the filed cause) | `BOB_NO_STRIP=1` makes the mirror MORE uniform (19 colours → 2) |
| the RTT probe / FBO | probe returns `DD_OK`; FBO `complete=1`, 20 binds/flight |
| the viewport | `mirrorRect L0 T0 R128 B128 → x0 y0 w128 h128` |
| "nothing is drawn into it" | black at bind, grey after; 138-140 draws land |
| texture upload failure | `0 uploads bailed`; the grey canary sees only the loader quad |
| fog | `EnableFogging` is entirely dead code |
| a colour clear | the pass clears ZBUFFER only; compat `Clear` is faithful |

**Next (sprint 4, the last on this item):** the remaining question is why the horizon dome fills the
frame — the projection is built with a **2.5 aspect ratio for a square 128x128 target**, and the
object matrix is offset `-(viewer_y + 500)`. Check those two numbers before anything else; if the
dome is simply drawn too close/too large, that is the whole defect.


## R3.4 — SPRINT 4 (2026-09-03): two more candidates eliminated; **SPRINT LIMIT REACHED**, and the item needs a GOLD reference to proceed

**Eliminated this sprint:**
* **the `-(viewer_y + 500)` offset** — the mirror path is the only landscape render that offsets the
  viewer height (both normal paths at `LANDSCAP.CPP:530` and `:993` use plain `-viewer_y`), so it
  looked like a real candidate. `BOB_MIRROR_YOFF=0` (new A/B, default 500 = shipped) produces an
  **identical** mirror: mean (213.5, 213.6, 214.0), range 209-215. Not it.
* **the 2.5 aspect ratio on a square target** — eliminated by reasoning, not measurement, and marked
  as such: a rear-view mirror is a wide, short shape, so rendering at 2.5 into a square texture that
  is then mapped onto a wide cockpit quad is the ordinary way to do it.
* **the pitch sign** (`BOB_MIRROR_PITCH`, default −1 = shipped) — the code's own comment says the
  pitch "needs to be made to point down not up", so a wrong sign would face the sky. Flipping it
  DOES change the content (19 → 47 colours, range 204-217) but the result is still a flat grey of
  the same mean. It affects what the mirror sees; it is not the defect.

**The structural finding that should drive the next sprint.** Listing the calls each landscape path
makes:

| main render (`:530+`) | mirror render (`:672+`) |
|---|---|
| `DrawHorizon`, `GeneratePointData`, `DistDrawClouds`, `FlushAsBackground`, `InfiniteStrip`, `DoRain`, `SetAmbientLighting` … | `UpdateHorizTexture`, `VisibleCheck`, `BeginPoly`/`EndPoly`, `Translate` |

**`RenderMirrorLandscape` draws the horizon band and NO TERRAIN TILES.** A mirror fed only an
ambient-lit horizon is flat by construction — which is consistent with everything measured across
four sprints (ambient `0xe5e5e5` → result `~213`).

⚠️ **And that raises the question this item cannot answer from inside the port: what SHOULD the
mirror show?** A rear-view mirror in a fighter, looking backward and level at altitude, showing sky
and horizon is not obviously wrong. The reported defect may be less "the mirror is flat" than
"nothing ever appears IN it" — and `GetMirrorObjects` contributing no visible draws would be the
real complaint. **I cannot claim that from the phase counters**: sprint 3 established that all
geometry is batched to `Lib3D::EndScene`, so `landscape=8 objects=0` does not mean the object list
was empty.

**Recommendation to the PO: settle it against the gold before spending a fifth sprint.** R3.8
(render regression sweep) exists precisely to A/B in-flight views against Wine `bob.exe`. One gold
capture of the cockpit mirror answers whether a flat sky-grey mirror is correct behaviour, whether
aircraft should appear in it, or whether terrain should. Every internal candidate that could be
eliminated by measurement now has been; the next question is about the TARGET, not the code.

**Instruments left behind, all env-gated and default-off:** `BOB_TRACE_RTT` (probe verdict,
`mirrorRect`/viewport, per-target draw counts, ambient colour), `BOB_DUMP_RTT_BIND`, `BOB_NO_STRIP`,
`BOB_MIRROR_YOFF`, `BOB_MIRROR_PITCH`.


## R3.2 — SPRINT (2026-09-03): 🔴 **THE SHIPPED DEPTH SORT IS DELETING COCKPIT INSTRUMENTS**

**First, the entry was stale.** R3.2 reads "SPIKE — needs dedicated focused work (2 failed real-time
attempts)", but the spike LANDED: `zdepth = is2D && !getenv("BOB_NO_ZDEPTH")` is **default ON**
(marked S119). So this sprint measured the shipped state rather than re-implementing it.

**And the shipped state has a cost nobody had measured.** A/B of a QM cockpit flight, same frame:

| | depth ON (shipped) | depth OFF (`BOB_NO_ZDEPTH=1`) |
|---|---|---|
| lower instrument panel | **three instrument bezels MISSING** | all three present |
| pixels differing | 112,313 — **99k of them in the bottom quarter** of the screen | |
| where they differ | depth-ON is DARKER (35,39,25) | (42,46,31) |

The cockpit is authored in SUBMISSION order, so a gauge drawn later with a farther RHW z loses the
`LEQUAL` test to the panel drawn before it, and vanishes.

**Two experiments, both conclusive:**
1. `BOB_ZDEPTH_PAINTER=1` — keep painter's order within the OPAQUE cockpit (`GL_ALWAYS` + still
   write depth) so later parts win while the depth buffer still populates for clouds. **Did not
   restore the bezels.**
2. `BOB_ZD_ALLPAINT=1` — extend that to translucent geometry as well. **Restores the cockpit
   completely: 191 pixels from the depth-OFF frame.**

So the missing bezels are **smooth-alpha cockpit parts being depth-rejected**, and the useful
distinction is NOT opaque-vs-translucent (which is what the code currently splits on) but
**world-vs-cockpit**.

⚠️ **This is a live trade-off in the PO's build, and the PO should decide it:** the depth sort stops
clouds painting over the canopy, and costs three instruments on the panel. `BOB_NO_ZDEPTH=1` trades
back. Neither is right; the fix is to depth-test the WORLD and leave cockpit/HUD geometry in
painter's order.

**Next sprint:** the world quads are already identifiable — `g_devTex[0]->isRTT` marks the
FBO-composited landscape, and the existing `[cloudz]` instrument buckets on exactly that. The open
question is which class the CLOUD sprites fall into (they are not the FBO landscape), because they
are the geometry the depth test exists to reject. Classify a cloud draw first, then split on
world-vs-cockpit instead of on alpha.

Both new switches are **default OFF**; the shipped path is untouched by this sprint.


## R3.2 — SPRINT 2 (2026-09-03): the missing instruments are FIXED by a depth split; the cloud half is NOT yet verified

**Two candidates eliminated first, both by measurement:**
* **world-vs-cockpit via `isRTT`** — the split sprint 1 proposed. `BOB_TRACE_CLOUDZ` in a real
  cockpit flight reports **`WORLD/terrain(isRTT) n=0`**: not one draw is classified as the
  FBO-composited world, all 628,000 land in the other bucket. The discriminator does not exist on
  this path, so that plan is dead.
* **stale back-buffer depth** — R3.2's own note listed "a per-frame back-buffer depth clear" as a
  requirement, and the game only z-clears its FBO targets. Added one (`BOB_ZDEPTH_CLEAR=1`,
  back buffer only). The bezels stay missing: 15k px from the broken frame, 94k from the good one.
  Not the cause.

**What does work.** The missing bezels are TRANSLUCENT cockpit decals rejected by the opaque panel
drawn before them at a nearer z — in painter's order they composite on top, under `LEQUAL` they lose.
Clouds are translucent too, so opacity cannot separate them — but **depth can**: the cockpit is near,
the clouds far, and the RHW z spans the full 0..1 across this pass. `BOB_ZD_NEARFREE=<z>` lets a
translucent draw whose nearest z is below the threshold skip the test (cockpit decal) while a far one
keeps it (cloud).

At `BOB_ZD_NEARFREE=0.5` **the three instrument bezels come back** — confirmed visually, and the
frame moves from 48k px away from the good cockpit to 27k.

⚠️ **AND THE CLOUD HALF IS UNVERIFIED — the test frame cannot check it.** Cropping the canopy post in
all three arms shows clouds correctly BEHIND it in `depth-OFF` too, i.e. **this frame never exhibited
the bug the depth sort exists to fix**, so it cannot discriminate. The region diff is consistent with
that reading and not with a claim of success: in the sky/canopy rows the near-free arm sits 196 px
from depth-OFF and 12,299 px from depth-ON.

**So the honest state:** a fix that restores the cockpit and a property that has not been tested.
`BOB_ZD_NEARFREE` ships **default OFF**.

**Next sprint:** get a frame where clouds demonstrably overlap the canopy with `BOB_NO_ZDEPTH=1`
(a cloudier QM, or fly INTO the cloud layer — `MissManCampSky().Layer[0]` gives the altitudes, and
`BOB_AUTOFLY` can climb). Only then can the near-free threshold be A/B'd on the property that
matters, and only then should it be defaulted on.


## R3.2 — SPRINT 3 (2026-09-03): the cloud half CANNOT BE VALIDATED HERE — the original bug does not reproduce

Sprint 2 left one question: does the `BOB_ZD_NEARFREE` split still keep clouds off the canopy? To
answer it needs a frame where clouds demonstrably overlap the cockpit **with the depth sort OFF** —
i.e. where the original defect is visible. Two flights were flown to find one:

| run | scenario | result |
|---|---|---|
| `BOB_AUTOFLY=dive`, depth OFF, frame 1600 | climbing, 10,379 ft | canopy occludes correctly; a cloud sits in the sky, not over the frame |
| `BOB_AUTOFLY=dive`, depth OFF, frame 3000 | 10,476 ft | same |

**The clouds-over-cockpit defect did not reproduce in ANY arm.** So the near-free rule cannot be
validated against it on this machine's available scenarios, and — worth saying plainly — **it is not
established that the defect still exists at all**. It may need particular weather (an overcast layer
low enough to fly INTO; `MissManCampSky().Layer[0]` carries the altitudes and this QM's layer was
never entered), or it may already be fixed by other work.

**Consequence for the item:** `BOB_ZD_NEARFREE` restores the three cockpit instruments (sprint 2,
visually confirmed) and its risk to clouds is UNTESTED because the risk case could not be staged.
It stays default OFF. **Before another sprint, R3.2 needs a scenario that shows the bug** — the PO
saw it, so the fastest route is asking which mission/weather, rather than flying more sorties blind.

---

### 🔗 A LEAD FOR R3.9 (the PO's floating grey square), found while doing this

Every cockpit capture across this session — four flights, different altitudes, different depth
settings — contains a **flat grey ellipse pinned to the SAME screen rows, y 140-190**:

| capture | grey-blob bbox |
|---|---|
| `cl_off` (dive, 10,379 ft) | x 156-314, **y 140-189** |
| `cl_off2` (dive, 10,476 ft) | x 48-293, **y 140-190** |
| `r32_on` (level QM) | x 62-314, **y 140-190** |
| `r32_off` (level QM) | x 63-314, **y 140-190** |

A world object would move as the aircraft climbs and manoeuvres; **this is pinned to screen
coordinates**. Cropped, it is a hard-edged grey lens/ellipse floating in the sky — the same character
as the PO's report ("a floating light/dark grey square … sometimes").

**Best hypothesis: an aircraft SHADOW polygon drawn in the sky rather than on the ground** — a shadow
is an ellipse, `DETAIL3D_AIRCRAFTSHADOWS` is on by default, and R3.9's own marker-texture list
already names `shadow` / `lshad`. ⚠️ Note it is TEXTURED, not an untextured draw: R3.9's `[grey]`
canary reports only the loader quad, which is consistent with a shadow texture rather than a missing
bind — and explains why four sprints of untextured-quad hunting never found it.

**R3.9 is at its 4-sprint limit**, so this is recorded rather than pursued. The next sprint on it
should start here: toggle `DETAIL3D_AIRCRAFTSHADOWS` and see whether the ellipse goes.


## R3.9 — SPRINT (2026-09-03): the shadow hypothesis is REFUTED, and I RETRACT the "screen-pinned" claim I made for it

Last sprint I recorded a lead: a flat grey ellipse appearing in every cockpit capture, "pinned to the
SAME screen rows (y 140-190)", with the hypothesis that it is an aircraft SHADOW polygon drawn in the
sky. Both halves were tested this sprint.

**1. The shadow hypothesis is refuted.** `BOB_TRACE_SHADOW` first confirmed shadows are live and
reaching the draw (`detail=1`, added for non-player aircraft; the player's own is correctly skipped
because `dopiloted=0` and `ac == ControlledAC2`). Then `BOB_NO_ACSHADOW=1` (new, test-only,
default off) suppressed them entirely:

| | grey-blob pixels in the region |
|---|---|
| shadows ON | 10,312 |
| shadows OFF | **10,288** |

Unchanged, and the ellipse is still there in the crop. **It is not an aircraft shadow.**

**2. ⚠️ And my "pinned to screen coordinates" claim does not hold.** Re-reading my own table: the
blob's **x** moves substantially between captures (156-314, 48-293, 62-314, 63-314) — only **y** is
stable, and stable y is exactly what level-ish flight at similar altitude produces. I treated "same
rows" as "screen-pinned" and drew a conclusion the data did not support. A distant world object would
look like this. The lead was overstated and is withdrawn.

**3. It is also unconfirmed that this ellipse is even the PO's defect.** The PO reported a "floating
light/dark grey SQUARE"; this is a hard-edged grey LENS. Similar in character, not the same shape,
and I should not have equated them.

**What remains true and useful:** there IS a hard-edged flat-grey ellipse in the sky in ordinary
cockpit flight, it survives disabling shadows, and R3.9's earlier work already eliminated untextured
draws — so it is textured geometry. Plausible identities not yet tested: a barrage BALLOON (grey
ellipsoid, and this is Battle of Britain), or a cloud billboard drawn with hard edges. **The next
sprint should identify the object rather than the mechanism** — dump the draw that covers those
pixels (its texture and vertex extent) instead of guessing from shape.

Convoy campaign gate: PASS (the only code added is an env-gated early return, default off).


## R3.6 (PO 2026-08-28, the icon art) — ⭐ **ROOT CAUSE FOUND AND FIXED: the marker-x rule was substituting a CAMPAIGN icon for the exit icon**

R3.6 was already narrowed by earlier work to "the placement and the hit-test are correct — only the
ART is wrong", with the `OnGetFile` art-guard eliminated. This sprint found what actually assigns it.

**The evidence, straight out of the system box's own draw sequence:**

```
[sysbox-ctl] dlgId=823 ctrlId=1002 visible=1 dlu=(16,30,18,20)
[fileman] marker-x: 'xi_bases.bmp' -> 'i_bases.bmp' (FIL_x placeholder substituted)
[ole] getfile 0x6a9c -> BM 48x48 8bpp
[ole] draw panel ctrl id=1002 at (989,56) 27x32
```

The system box's control asks for `xi_bases.bmp`; that file does not exist; the **marker-x rule**
(`FILEMAN.CPP`, S180b) strips one `x` and accepts `i_bases.bmp` — **the campaign BASES icon** — which
is then drawn on the exit control. That is precisely the PO's report: *campaign icons drawn where the
X belongs*.

**Why the rule did it.** It was written for a real problem — MASTER.FIL spells a family of names with
a stray `x` (`buxtton1`, `xtitleb`, `gripx`, …) and the retail build substitutes it away, so the map
toolbar had been missing its art. Its guard only ensured it "cannot change any path that resolves
today" — but it never checked that the substituted name is *semantically* the same art. A missing
icon is a missing icon; a DIFFERENT icon is worse, because it looks deliberate.

**The fix, and why it is safe.** The rule's own evidence names the family it verified — `button1`,
`button2`, `titleb`, `grip`, `b_all1`, `b_all2`, "checked 6 for 6 against artwork/AXART". It is now
restricted to that family. Measured over a full campaign session **the rule fired exactly ONCE, and
that once was the wrong one** — its intended beneficiaries never trigger on this path — so
restricting it removes the defect and takes nothing away. `BOB_MARKERX_ANY=1` restores the old
behaviour for A/B.

**Verified:** marker-x substitutions in a full campaign run go **1 → 0**; the system box still draws
all 3 of its controls (`[sysbox] panel 34x50 DLU -> 51x81 px at (965,8): 3 controls drawn`); the
convoy campaign gate still PASSES.

⚠️ **Scope, stated honestly:** this removes ONE wrong icon, and the PO reported TWO. The remaining
one is not this mechanism (only one substitution ever fires). The exit control will now show no art
rather than the wrong art — better, but the "X" itself is still not being drawn, and finding its real
source is the next step on this entry.

Gates: the usual `soak`/`r1`/`settings_nav` refuse while pid 2341852 is alive; everything that ran, passed.


## R3.6 — SPRINT 2 (2026-09-03): ⭐⭐ **THE SECOND MECHANISM FOUND AND FIXED — an id collision — and R3.6 is now fully explained**

Sprint 1 fixed one wrong icon (marker-x). The PO reported TWO. This sprint found the other, and it is
a straightforward bug:

**`bob_ole_rbutton.cpp`'s `kBtnIcon` forcing table matched on `ctrlId` ALONE.** Control ids are only
unique WITHIN a dialog, and the SYSTEM BOX (dlgId **823**) numbers its buttons **1001/1002/1003** —
the same ids the files toolbar uses. So the system box's `TOOLBAR_HIDE` and `CLOSE1` buttons were
force-fed the toolbar's **THUMB** and **SAVE** icons. That is exactly *"two CAMPAIGN icons are drawn
where the X (exit) icon belongs"*, and it is why every earlier sprint measured the placement and the
hit-test as correct while only the ART was wrong.

**Fixed** by giving each entry the dialog it was written for, measured from a campaign run:
**826** = strategic-map toolbar, **942** = files toolbar, **960** = title bar
(`BOB_BTNICON_ANYDLG=1` restores the old match). Verified:

| | before | after |
|---|---|---|
| dlg 823 id=1001 (`TOOLBAR_HIDE`) | forced 0x10000 (ICON_THUMB) | **0x6a99, its own art** |
| dlg 823 id=1003 (`CLOSE1`) | forced 0x10002 (ICON_SAVE) | **0x6aa0, its own art** |
| dlg 826 id=1827 (map toolbar) | 0x10007 | 0x10007 — unchanged |
| dlg 942 id=1001 (files toolbar) | 0x10000 | 0x10000 — unchanged |

Convoy campaign gate: PASS.

### ⚠️ And the reason the X still will not appear — worth the PO knowing

Following the art through to the data, **the correct icons are simply not in this source drop**:

```
MASTER.FIL:   i_basexs.bmp   FIL_xICON_TOOLBAR_HIDE
              xi_bases.bmp   FIL_xICON_SCREENSIZE
              i_baxses.bmp   FIL_xICON_CLOSE1
```

All three are `i_bases.bmp` **with a stray `x` at a different position** — the marker-x placeholder
family. Strip the x and every system-box button loads the campaign BASES icon; six more entries share
the same file. And the sprite sheets (`iconnum.g`, `iconnum2.g`) contain no `CLOSE1`,
`TOOLBAR_HIDE` or `SCREENSIZE` entry either.

**So the exit button can be blank or wrong; with this data it cannot be right.** Sprint 1's marker-x
restriction chose blank, which is the better of the two — a missing icon reads as missing, a wrong
icon reads as deliberate.

**A real route to the X exists though:** the port already paints `? / tick / X` glyphs itself for
title bands (`bob_oob_paint_title_glyphs`, S174). Pointing that at the system box's close control
would draw a genuine X without needing the absent art. **That is the next sprint on this entry.**


## R3.6 — SPRINT 3 (2026-09-03): ✅ **THE X IS DRAWN. R3.6 is closed.**

Sprint 2 ended with the system box's buttons correctly no longer showing campaign icons, but blank —
because the art `MASTER.FIL` maps them to is `i_bases.bmp` under a marker-`x`, and the sprite sheets
carry no `CLOSE1` entry.

**They do carry a real X, though: `ICON_CROSS`** — already used by the title-band glyph painter
(S174). The system box's close control is now mapped to it, so the X is drawn **from art that
actually exists** rather than from a filename that does not:

| | before | after |
|---|---|---|
| dlg 823 id=1003 (`CLOSE1`) | 0x6aa0 — a file that returns nil | **0x10022 — `ICON_CROSS`, a sheet icon** |

**Verified in pixels:** a campaign-map capture shows the **red X drawn in the upper-right**, at the
system box's own position (965,8). Convoy campaign gate: PASS.

**Only that one control is mapped, deliberately.** The sheet has no `TOOLBAR_HIDE` or `SCREENSIZE`
equivalent, and picking a near-match (`ICON_ZOOM` for "screensize", say) is exactly the habit that
created this defect — an icon that is merely *plausible* reads as intentional and misleads. Those two
stay blank until real art is identified.

### R3.6 — the complete story, three sprints

1. **marker-x** was substituting `i_bases.bmp` (the campaign BASES icon) for any failed filename;
   restricted to the six names its own evidence verified.
2. **`kBtnIcon` matched on `ctrlId` alone**, and the system box shares ids 1001/1003 with the files
   toolbar — so it was force-fed the toolbar's THUMB and SAVE icons. **That was the "two campaign
   icons".** Each entry now carries its dialog (826 map toolbar / 942 files toolbar / 960 title bar).
3. **The close button now draws `ICON_CROSS`.**

The PO's report — *"two CAMPAIGN icons are drawn where the X (exit) icon belongs"* — is answered in
full: the two wrong icons are gone and the X is there. Earlier sprints had established the placement
and hit-test were already correct, so no other work is outstanding on this entry.


## R3.9 — SPRINT (2026-09-03): ⭐ **THE GREY SHAPE IS IDENTIFIED — it is a SPRITE, and here is its texture**

The previous sprint ended by saying the next one should *identify the object, not reason from its
shape* — because reasoning from shape had produced two wrong answers (an aircraft shadow; "pinned to
screen coordinates"). This sprint did that.

**New instrument: `BOB_PIXPROBE="x,y"`** names every pre-transformed primitive whose screen bbox
covers a given pixel, reports its bound texture (size, bpp, isRTT, colour-key) — and now **writes
that texture out as a PPM**, so the object is identified by looking at it.

**Probing the ellipse at (180,165)**, one draw matches its footprint exactly:

```
[pixprobe] (180,165) prim=6 count=4 bbox=(30,17)-(330,317) tex=128x128 bpp=16 isRTT=0
```

— a 128x128 texture drawn as a ~300x300 screen quad, over the ellipse's own span (x 48-314).

**And the texture is the answer**: a **beige/tan ELLIPSE on a pure GREEN background** — green being
the colour-key. So the "floating grey square/lens" is a **SPRITE**, drawn from artwork that is
literally an ellipse.

**That explains why four sprints missed it:** R3.9 hunted untextured quads (`[grey]` canary) and then
shadow geometry. This is neither — it is a normal, textured, colour-keyed sprite. An untextured-draw
canary can never see it, and disabling aircraft shadows correctly changed nothing.

**What remains:** which game object places that sprite. Candidates now that the ART is known: a
ground/blob shadow drawn through a path other than `Add_Shadow`, a distant barrage balloon, or a
smoke/cloud puff. Identifying the source file would settle it — a recursive scan of the installed
art for 128x128 BMPs timed out and is not worth brute-forcing; better to log the texture's creation
path (the surface is created from a named file somewhere) and read the name.

⚠️ Whether this sprite is even the PO's defect is still unconfirmed — the PO said "square", this is a
lens — but it IS a real flat grey shape floating in the sky in ordinary flight, and it is now a known
quantity rather than a mystery.

Convoy campaign gate: PASS. The probe is env-gated and off by default.


## R3.9 — SPRINT (2026-09-03, cont.): the sprite's DRAW PATH and TEXTURE IDENTITY are pinned down

Building on the identification of the artwork (a beige ellipse on a green colour-key), this sprint
answered *how* it reaches the screen and *which* texture it is.

**Draw path — by backtrace, not inference:**

```
draw_fvf  <-  DEV_DrawPrimitiveVB  <-  Lib3D::RenderTPolyList  <-  Lib3D::EndScene
          <-  ThreeDee::render3d   <-  ThreeDee::render
```

So it is a **world-space TRANSPARENT (alpha-blended) polygon**, batched into the transparent list and
flushed at `EndScene` — not an overlay, not a HUD element, not a render-target blit. Every one of
those had been a live suspect in this entry at some point.

**Texture identity** — the port already carries `uniqueTextID` down to the GL layer for exactly this
kind of question, and the probe now reports it:

```
uniqueTextID=0x07ad  ->  type=TIT_LOADED  index=1965  masked=2  isLand=0
```

A **loaded, colour-keyed texture, index 1965**, not a landscape tile and not a plain-coloured
material — consistent with the green-key artwork dumped last sprint.

⚠️ **One loose thread worth recording rather than guessing at:** `3DDEFS.H` declares
`MINMAX(ImageMapNumber, 0, 1023)`, yet this index is **1965** — outside the image-map range even
though `TIT_MASK_INDEX` (0x07FF) permits it. Either the index is not an ImageMapNumber, or the range
in the header is stale. **That is the thread the next sprint should pull**, because whichever it is
names the object.

⚠️ I also corrected my own probe: I printed the type as `id >> 11`, which mixes in
`TIT_MASK_SECONDARY`. The type is `id & 0xC000` (`TIT_MASK_TYPE`). The conclusion (TIT_LOADED) was
right because the top bits are zero, but the field as printed was wrong and is fixed.

Convoy campaign gate: PASS. The probe stays env-gated (`BOB_PIXPROBE`, `BOB_PIXPROBE_BT`), off by default.


## R3.9 — SPRINT (2026-09-03, 4th this cycle): **the sprite's texture is NAMED — image map 525 (dir 2, file 13)**

The loose thread from last sprint was that `uniqueTextID`'s index (1965) sat outside the documented
`ImageMapNumber` range of 0..1023. **Resolved, and the header was not stale:** `hTextureMap` is a
**runtime texture-table SLOT** (`LIB3D.CPP:7405` allocates from the top of `textureTable` as textures
upload), not an identity. So `uniqueTextID` can never name artwork — a dead end, now closed rather
than left as a suspicion.

**The identity is on the material's ImageMap pointer, which the port already captures
(`g_lib3d_map0`).** Added `bob_imagemap_number_of()` — a reverse lookup that walks
`GetImageMapPtrDontLoad(dir<<8|file)` over the table and returns the composed number for a pointer
(public accessor only, and it never loads, so it cannot change what is resident). The probe now
reports it:

```
[pixprobe] (180,165) ... tex=128x128 masked=2 isLand=0 uniqueTextID=0x07ad
[pixprobe]   imagemap number = 525 (dir 2, file 13)
```

**So the floating grey shape is image map 525** — a 128x128, colour-keyed, beige-ellipse-on-green
sprite, drawn as a world-space transparent billboard.

**R3.9's state after this cycle — everything by measurement:**

| question | answer |
|---|---|
| untextured quad? | **no** — the `[grey]` canary sees only the loader screen |
| an aircraft shadow? | **no** — suppressing shadows leaves it unchanged |
| screen-pinned? | **no** — my own claim, withdrawn; its x moves between frames |
| an overlay / HUD / RTT blit? | **no** — backtrace puts it in `RenderTPolyList` from `render3d` |
| what IS it? | a world-space transparent SPRITE, **image map 525 (dir 2, file 13)**, colour-keyed |

**The last step, for whenever this resumes:** turn dir 2 / file 13 into a filename. `SuperMap` carries
no directory name, so it needs the image-map LOADER (`LoadImageMap`) — log the path it opens for
dir 2 and the answer falls out. That is a small, well-defined piece of work, and it is the difference
between "image map 525" and naming the object on screen.

⚠️ Still unconfirmed that this sprite is the PO's reported defect (they said "square"; this is a
lens), and that caveat has now survived four sprints — worth asking the PO directly rather than
assuming.

Convoy campaign gate: PASS. All probe code is env-gated and off by default.


## R3.7 (in-flight effects) — SPRINT (2026-09-03): 🔴 **BLOCKED BEFORE THE EFFECTS: the guns never fire**

R3.7 asks whether tracers, muzzle flash, smoke and flak render faithfully. **The story cannot be
started, because nothing fires.** Measured, not assumed:

| attempt | result |
|---|---|
| `BOB_AUTOFLY=shoot` as shipped (tap DIK 0x39 down+up in one tick) | gun ammo **2800 at frame 900 and 2800 at frame 2500** — not one round |
| the key HELD instead (down for 20 ticks, then up) | ammo **still 2800** |
| driving the game's own `KeyFake3d(SHOOT, …)` directly, bypassing the scancode leg | ammo **still 2800** |
| the gate it reaches | `[fire] KeyPress3d(SHOOT)=0 secondary=0 ShootDelay=0` — **the action is never seen** |

`KEYMAPS.H` binds `KeyAll(SHOOT, space)` and the scaffold pushes DIK 0x39, so the binding is right;
and the SAME scaffold's other keys work (throttle `0x0B` and trim `0xC7` fly the aircraft in
`BOB_AUTOFLY=dive`, view `0x40` switches views). So this is specific to the SHOOT action, not to
synthetic input in general.

⚠️ **Scope, stated carefully: this shows two SYNTHETIC paths fail.** Whether a human pressing space in
a real session fires the guns is NOT established — it cannot be tested headlessly, and I should not
report "the port cannot shoot" on this evidence. **The PO can settle it in one sortie**: fly a quick
mission, hold the trigger, and see whether the ammo counter moves.

**If it does not move for the PO either, this is a significant gameplay gap** — a combat flight sim
that cannot fire — and it outranks R3.7's fidelity question entirely.

**Instruments left behind (env-gated, off by default):** `BOB_TRACE_FIRE=1` reports the firing gate
(`KeyPress3d(SHOOT)`, the secondary key, `ShootDelay`, `frametime`), and `bob_fake_shoot()` drives
the action bit directly. The `BOB_AUTOFLY=shoot` scaffold now HOLDS the key rather than tapping it
down-and-up in a single tick — a one-tick tap was a race, not an input, regardless of this defect.

Convoy campaign gate: PASS.


## R3.6 FOLLOW-UP (2026-09-03): 🔴 **MY OWN FIX CAUSED A LOG FLOOD — the PO hit it as a hang**

PO: *"start bob -> campaign -> german -> begin -> ... -> fly, tried mousing -> hang"*. Caught the
process live: **state R at 78% CPU**, not deadlocked — and the session log was growing at
**~101 lines/sec**, with **51,842 of 51,904 lines (99.9%)** being one message, 6 MB in nine minutes:

```
[fileman] missing file 6a99=...\artwork\axart\i_basexs.bmp -- returning NULL
[fileman] missing file 6a9c=...\artwork\axart\xi_bases.bmp -- returning NULL
```

**Those are the system box's two icons, and this is a consequence of R3.6 sprint 1.** Before that
sprint the marker-x rule silently (and wrongly) resolved them to the campaign BASES icon, so they
never reported a miss. Restricting the rule made them honest misses — correct — but exposed a latent
defect in the reporting.

**The latent defect:** the dedup kept a SINGLE `lastwarned` slot, which two ALTERNATING missing files
defeat completely — each call differs from the last, so both log every frame. One missing file would
have been quiet; two are a flood. Replaced with a 128-entry set plus a "table full" notice, the same
shape this codebase uses elsewhere. **Verified: a full campaign run now emits 2 missing-file lines,
down from 51,842.** Convoy gate: PASS.

⚠️ **What I have NOT established** is that the flood is the whole of the PO's hang. It is certainly a
real defect and certainly mine to fix; but the process was busy in `hrtimer_nanosleep` at 78% CPU,
and 101 lines/sec of stderr is not obviously enough to stall a frame loop on its own. **The PO should
retry the same path on this build**; if it still hangs, the flood was a passenger and the hang needs
its own diagnosis — the live-process method used here (state, %CPU, wchan, log growth rate) is the
one to repeat, because it distinguished "spinning" from "deadlocked" in seconds.

## MIRROR-1 S4 (Opus 5, 2026-09-13) — ⭐⭐ ROOT CAUSE AND FIX: the mirror's projection used a 5:2 aspect against a SQUARE render target

S3 proved the cull is a fault (124 of 961 cells forward, 2 of 961 in the mirror, same grid, same
frame) and named S4's question: which view is in force when the mirror's `VisibleCheck` runs.

**First, a correction I nearly shipped as a finding.** Reading `3DCODE.CPP:6547` I saw
`//TempCode JON 2Nov00 g_lpLib3d->Rotate(MATRIX_VIEWER,AXIS_HEADING,...)` commented out and was
about to report "the view is never reversed". The **live** heading rotate is three lines above it
(`-hdg + ANGLES_180Deg`); the commented line is a superseded duplicate. A commented-out line that
looks like the answer is not the answer — read the whole block.

**The instrument.** `VisibleCheck` culls with `ClipCodeFBLRTB`, which returns one bit per plane, so
a HISTOGRAM of the flags each pass already computes names the guilty plane with no new geometry.
Added to both horizon passes (`BOB_TRACE_RTT=1`):

    [clip] FORWARD of 1024 cells: inside=94  FRONT=497 BACK=56  LEFT=367 RIGHT=387 TOP=29  BOTTOM=1
    [clip] mirror  of 1024 cells: inside=0   FRONT=527 BACK=219 LEFT=387 RIGHT=367 TOP=485 BOTTOM=486

⭐ **The mirror's cull is VERTICAL.** LEFT/RIGHT are the forward pass's own numbers mirrored
(387/367 against 367/387) — so the horizontal field is right and the 180° reversal works. TOP and
BOTTOM go from 29/1 to 485/486: essentially every cell is rejected above or below. Note also
`inside=0`, not 2 — the "drawn=2" of S2/S3 is the pass's own four-corner test, which is more
permissive than "all corners inside".

⭐⭐ **The cause.** `_SetProjectionMatrix` builds `h = aspect * w` and puts `h` in `_22`, so the
second argument IS the vertical scale. The forward pass calls the 3-arg overload, which supplies
the screen's `aspectRatio`. The mirror calls the 4-arg one with a hardcoded **`5.f/2.f`** — the
aspect of a wide mirror STRIP, which is what a rear-view mirror looks like in the cockpit — but this
port renders the mirror into a **square 128x128 FBO**. A 2.5x vertical scale puts `sy` outside `±w`
for almost every horizon cell.

**A/B, and the mirror FBO CAPTURED in both arms — not merely counted:**

| aspect | inside | TOP | BOTTOM | horizon drawn | mirror FBO |
|---|---|---|---|---|---|
| 2.5 (shipped) | 0 | 481 | 482 | 2 | mean 79.4 **sd 0.78** distinct 20 — uniform olive |
| **1.0** | 24 | 10 | 5 | **41** | mean 100.6 **sd 43.21** distinct 693 — **sky, horizon line, ground** |

`bobmirror_ab.png` is the pair. The left half is the PO's flat mirror; the right half is a rear
view. This is the capture the item has needed since June — every earlier sprint measured counts.

**Shipped:** default aspect is now **1.0**; `BOB_MIRROR_ASPECT=<float>` A/Bs it and `=2.5` restores
the old behaviour. Verified with the default and no override: `mean=100.5 sd=43.33 distinct=694`.
Low risk regardless — the mirror is still gated on the Reflections setting (`BOB_MIRROR`), off by
default, so the PO opts in.

**Not claimed.** That 1.0 is the *ideal* value. It is the FBO's own shape and it demonstrably works;
whether the geometrically right number is 1.0 or the aspect of the cockpit mirror QUAD the texture
is finally mapped onto is a display-side question this sprint did not touch. A cockpit capture was
attempted (`BOB_SHOT=520`) and did not land in the window the run had; the FBO capture stands on its
own and the cockpit view is the first thing to look at next.

**MIRROR-1: 4 sprints — at cap, and rotating off with the defect fixed rather than parked.**

## R3.7 S2 (Opus 5, 2026-09-13) — ⚠️ S1's probe was DESTROYING the event it measured; fixed, and the binding is now proven from the live table

R3.7 was parked as "blocked on the PO: fly a sortie and see whether the ammo moves". Two of its
three legs turn out to be answerable here, and one of them was self-inflicted.

⚠️ **The instrument was the bug — in part.** `keytests::KeyPress3d` is **consume-on-read**:

    bool keytests::KeyPress3d(KeyVal3D keyval)
    { return BITRESET(keymap->bitflags, keyval+1); }     // clears the bit, returns its OLD value

That is faithful to the original (`btr` + `setc al`) and correct for a oneshot. But S1's `[fire]`
trace called it to REPORT the gate:

    bool k1 = Key_Tests.KeyPress3d(SHOOT), k2 = ...;     // eats the hit bit
    ...
    if (Key_Tests.KeyPress3d(SHOOT) || ...)              // the real gate, two lines below, now reads 0

So **with `BOB_TRACE_FIRE=1` the guns could not fire even if the key had been pressed**, and S1's
headline measurement `[fire] KeyPress3d(SHOOT)=0` was partly its own damage. Added
`keytests::KeyPeek3d` (non-consuming, `BITTEST`) and the trace now uses it. Exactly the banked
[[measuring-can-hide-the-bug]] shape, and worth re-reading before adding any probe to a
consume-on-read API.

**It was not the whole cause.** Re-run with the corrected probe: `[fire] KeyPress3d(SHOOT)=0
secondary=0 ShootDelay=0 frametime=4` — the hit bit is still never set. Honest position: the probe
defect is real and fixed; it did not unblock the item.

⭐ **What IS now settled, and S1 asserted it without measuring.** S1 said *"KEYMAPS.H binds
KeyAll(SHOOT, space) ... so the binding is right"* — read from the header, not from the running
game. Dumped the LIVE table (`BOB_DUMP_BINDINGS`, 550 entries):

    0x39,0,112      <- space, shiftstate 0, action 112 = SHOOT (KeyName(56,SHOOT), KeyVal = 56*2)
    0x39,6,576
    0x39,7,1310

The binding is correct **in the running game**. That eliminates the hypothesis, and it also surfaces
something S1 could not have seen from the header: **space is bound to two OTHER actions at
shiftstates 6 and 7.**

**S3 — two candidates left, one run splits them.** `Inst3d::OnKeyDown` sets the hit bit from
`commonkeymaps->mappings[keynum]`, so either
  (a) the synthetic key never reaches `OnKeyDown` at all, or
  (b) it arrives with `currshifts` not 0, selecting action 576 or 1310 instead of SHOOT.
Trace `OnKeyDown` — scancode in, `currshifts`, and every index it sets. Do not add a probe that
calls `KeyPress3d`.

⚠️ **The PO sortie is still worth doing and is NOT superseded:** everything above concerns the
synthetic path. Whether a human pressing space fires the guns remains untested, and that is the
question that decides whether this is a harness limitation or a shipped gameplay gap.

**R3.7: 2 sprints.**

## R3.7 S3 (Opus 5, 2026-09-14) — ⛔ the scaffold never pulled the trigger: "the guns never fire" is not evidenced

S2 fixed the probe that was consuming the keypress and proved the binding correct in the live table,
leaving two candidates: the synthetic key never reaches `Inst3d::OnKeyDown`, or it arrives with the
wrong shift state. S3 answers with a third possibility neither sprint considered.

**`OnKeyDown` is never called at all** — `BOB_TRACE_KEY` (which already existed; no new probe needed)
printed **0 lines for every key** across a 190 s autofly run. Instrument proven able to speak first:
the format string is in the binary and `BOB_LINUX` is defined by `CMakeLists.txt:50`. That is not a
SHOOT-specific fault, and it is not the shift state — the scaffold does not use that path at all. It
pushes `kb_push()` (the DirectInput buffer) and calls `bob_fake_shoot()`.

⛔ **And neither of those runs either.** Both autofly branches are gated on `g_bob_flight_active`, and
instrumenting the gate itself rather than inferring it:

    [fire] autofly shoot branch: g_bob_flight_active=0 cnt=1
    [fire] autofly shoot branch: g_bob_flight_active=0 cnt=2
    [fire] autofly shoot branch: g_bob_flight_active=0 cnt=3
    [fire] KeyPress3d(SHOOT)=0 secondary=0 ShootDelay=0 frametime=4

The trace reports on CHANGE as well as on the first ticks, so a single 0 is not a sample — the flag
never became 1 for the whole run. Meanwhile `frametime=4` shows TRANSITE's firing code executing, so
the aircraft IS flying as far as the sim is concerned while the port-side flag says otherwise.

⭐ **So the trigger was never pulled.** R3.7 S1's headline — *"BLOCKED BEFORE THE EFFECTS: the guns
never fire"*, with its table of "ammo 2800 at frame 900 and 2800 at frame 2500" — measured a scaffold
that does nothing in this configuration. S1's own careful wording was *"two SYNTHETIC paths fail"*;
the truth is that **neither synthetic path ever executed**. The port has NOT been shown unable to
fire, and that claim should not be repeated.

**Not claimed:** that S1's runs had the flag at 0 too. They used the same recipe, which makes it the
parsimonious reading, but it was not measured then and cannot be now. What IS established is that the
evidence on file does not support the conclusion drawn from it.

**S4:** make the gate true before testing anything about firing. `g_bob_flight_active` is set at
`FULLPSYS.CPP:1463` and `:1575` — find which flight-entry path this recipe misses, reach a flight
through the path that sets it, and only then ask whether the trigger produces rounds. Until then
neither "the guns fire" nor "the guns do not fire" is supported.

⚠️ **The PO sortie remains the cleanest answer and is still not superseded** — one quick mission,
hold the trigger, watch the ammo counter.

**R3.7: 3 sprints.**

## R3.7 S4 (Opus 5, 2026-09-14) — ✅ THE GUNS FIRE. The blocker was the scaffold's gate, and R3.7 can finally start.

S3 found both autofly branches gated on `g_bob_flight_active` and that flag reading 0 all run. S4
asks why, fixes it, and re-runs the measurement in S1's own currency.

**Why the flag was 0.** It is set in exactly two places, `FULLPSYS.CPP:1463` and `:1575`, and **both
are synthetic bridges into the 3-D**. A flight entered by the game's OWN path — the frontend
auto-click reaching Fly, which is what `BOB_BOOT_FRONTEND` produces — sets neither. Confirmed rather
than assumed: S3's log contains no `[frontend] (bridge)` and no `[startfly]` line, so neither bridge
ran, while the sim was demonstrably flying. **The flag records "we bridged into the 3-D", not "a
flight is live"** — and every autofly branch hangs off it.

**The fix.** `Inst3d::InThe3D()` is the game's own answer to the question the flag was trying to
approximate. Exposed it as `bob_in_the_3d()` (STUB3D.CPP) and the pump now takes the game's word:

    if (!getenv("BOB_NO_INTHE3D") && bob_in_the_3d()) g_bob_flight_active = 1;

**MEASURED, same recipe as S1, and closed in S1's own metric — the ammo counter:**

    [fire] autofly shoot branch: g_bob_flight_active=1 cnt=1
    [fire] KeyPress3d(SHOOT)=1 secondary=1 ShootDelay=0 frametime=4
    [fire] GATE PASSED #0   gunammo=2800 (started 2800, spent 0)
    [fire] GATE PASSED #1   gunammo=2787 (spent 13)
    [fire] GATE PASSED #5   gunammo=2735 (spent 65)
    [fire] GATE PASSED #200 gunammo=200  (spent 2600)
    [fire] GATE PASSED #400 gunammo=0    (spent 2800)

✅ **2,800 rounds down to zero.** S1's table read *"gun ammo 2800 at frame 900 and 2800 at frame
2500 — not one round"*; the same counter now empties the magazine. **The port can fire. It always
could.** Three sprints of "BLOCKED BEFORE THE EFFECTS: the guns never fire" measured a test scaffold
that was switched off.

**What this retires.** The claim itself, and the PO sortie that was being held as the tie-breaker —
it is no longer needed for THIS question (it remains the only way to test a human's trigger, but the
port's firing path is now demonstrated).

**R3.7 can finally begin.** Its actual story — do tracers, muzzle flash, smoke and flak render
faithfully — was never reachable while nothing fired. With rounds leaving the guns, that comparison
against the gold is now a normal sprint.

**R3.7: 4 sprints — at cap, rotating off UNBLOCKED rather than parked.**

## MP / cross-port (Opus 5, 2026-09-14) — ⭐⭐ BoB holds every multiplayer frame too: the csync gate is a SHARED Rowan defect

MA's EPIC M harness-validity S3 (same day) found MA multiplayer rendering nothing because
`STUB3D.CPP`'s only live `render()` call is gated on `!_DPlay.Implemented || _DPlay.csync`, and csync
measured 0 on 87 of 87 host samples. BoB is the same engine lineage, so the gate was checked here
rather than assumed — and BoB carries it verbatim, at `STUB3D.CPP:1682/1730/1739`.

**Measured with the identical probe, over BoB's own two-instance harness:**

    host    130 samples   ALL  DPlay.Implemented=1 csync=0 -> held (waiting/resync)
    client  130 samples   ALL  DPlay.Implemented=1 csync=0 -> held (waiting/resync)

⭐⭐ **Both ports hold every multiplayer frame.** Neither game renders a world in a two-instance
session; both exchange packets steadily while doing it. That is now two shared Rowan comms defects
found in this codebase pair — the other is MP-2 S19's `Joining` flag, where only a JOINING peer
announces itself, identical in both.

**A difference that looked like the answer and is not.** BoB sets `csync=true` at 3-D entry
(`STUB3D.CPP:1234`) where MA has no equivalent — which read like BoB's advantage until the code
around it is taken whole: it is set true, then `WaitEndDraw(D_YES)` twice, then set **false** again
at `:1240`. It is a transient for the mode change, not a persistent enable. So MA is not missing a
setter BoB has; both depend on the comms sync phase completing, and in both it does not.

**Where the fix lives, for whoever takes it.** In MA the chain is visible: `csync=true` is reached
only inside the second sync phase (`WINMOVE.CPP:4478`), which runs only from
`if (!csync && synched) { if (!SecondSyncPhase()) return false; … }` (`:299`). So `synched` is the
upstream term. The same structure should be checked in BoB before assuming it matches.

⚠️ **Worth the PO knowing plainly:** on this evidence multiplayer in BOTH ports connects, joins,
seats players, enters the 3-D and exchanges state — and shows a waiting screen rather than a world.
Everything previously reported as "multiplayer works" was measured on comms and entity state, never
on a rendered frame.

## MP / cross-port S2 (Opus 5, 2026-09-14) — ✅ BoB stalls in the SAME phase as MA: `InitSyncPhase` never succeeds in either port

The previous cross-port entry measured BoB holding every multiplayer frame on the same `_DPlay.csync`
gate as MA (130/130 samples), and explicitly said the chain BELOW that gate should be verified in BoB
rather than assumed to match MA's. MA's MP S4 then traced its stall to `InitSyncPhase`. This verifies
BoB with the identical probe (`BOB_TRACE_SYNC=1`).

**MEASURED, BoB's own two-instance harness:**

    host    130 samples  [sync] synched=0 csync=0     InitSyncPhase FAILED ~60/s
    client  129 samples  [sync] synched=0 csync=0     InitSyncPhase FAILED ~60/s

✅ **Same phase, both ports.** `InitSyncPhase` never succeeds, so `synched` never becomes TRUE,
`SecondSyncPhase` never runs, `csync` stays 0, and the render gate holds every frame. The assumption
was right — and it was still worth the run, because the two ports differ in a way that only the
measurement shows.

⭐ **The difference: the RETRY RATE.** BoB fails **60 times a second** — once per frame, a polite
per-frame retry. MA's host fails **~840,000 times a second**. Same logical stall, but MA additionally
spins inside the frame, which is a separate MA-side defect (an unbounded retry loop where BoB has a
bounded one) and explains the CPU burn in MA's harness runs. Worth carrying back to MA as its own
item; it is not part of the shared sync bug.

**Where this leaves multiplayer across the pair.** Both games: discover ✅, join ✅, seat both players
✅, enter the 3-D ✅, exchange state ✅, **sync ❌ → render ❌**. One function in each port, the same
one, standing between the current state and a working multiplayer picture.

**Next (either port, the work is shared):** `MA_TRACE_DPLAY` / the BoB equivalent already report the
shim's DirectPlay traffic. Ask whether the aggregate sync packet `InitSyncPhase` waits on is ever SENT
and ever DELIVERED. MP-6 S2/S3 already found and fixed one packet the shim dropped because it ignored
a receive filter the game depends on — check that fix's shape first.

## MP S5 cross-port (Opus 5, 2026-09-14) — BoB had MA's missing local delivery too, and now reads the SAME next gate

MA's MP S5 (ma 77299a7) found its DirectPlay shim delivering nothing locally: a send from one local
player to another, or to a group with a local member, went on the wire and nowhere else, so the
host's game half never received the sync packet its own in-process aggregator produced. BoB's shim
(`SRC/compat/bob_dplay.cpp`) carries the same `Send` and the same one-`myPid` assumption — the two
shims share their ancestry — so both fixes were ported: `localPids[]` records EVERY player this
process creates, and a local copy is queued when the destination is a local player or a group with a
local member (`BOB_NO_LOOPBACK=1` reverts).

**MEASURED, two-instance session (`BOB_TRACE_AGG=1`), and the defect was real here too:**

    host    [agg] loopback 9/s  from=1 to=2 (local player or group)
    client  [agg] loopback 2-3/s  from=4 to=2

Same topology as MA: an in-process player 1 addressing group 2, whose traffic previously never
reached this side's own game half. The two-instance gate still PASSES (both sides in 3-D, random
list clears), so nothing regressed.

⚠️ **BoB still does not sync**, exactly like MA: `synched=0 csync=0`, `InitSyncPhase FAILED 60/s`.
The gate probe was ported as well rather than assuming the two ports fail alike, and they do:

    bob  host    [agg] GATE num=1 CurrPlayers=4  IDCodes: 194 196 194 ...  (DUMMY=196)
    bob  client  [agg] GATE num=1 CurrPlayers=4
    ma   host    [agg] GATE num=1 CurrPlayers=4

⭐ **`CurrPlayers=4` in a TWO-player session, character for character the same as MA's.** So the
remaining defect is shared engine bookkeeping, not a port difference, and whichever port it is
solved in should fix both. MA's entry names the two counting sites to check (`CountPlayers()`
recounting from `H2H_Player[].status` vs a bare `CurrPlayers++` per allocated player).

**BoB MP: 1 sprint this pass (cross-port). Rotating off.**

## MP S6 cross-port (Opus 5, 2026-09-14) — the count fix lands (4 → 2), but BoB still does not sync: only ONE dummy reaches the aggregate

MA's MP S6 (ma 0c707a8) fixed two things behind the shared `num != CurrPlayers` gate: the shim looped
a group send back to its own sender (so each side processed its own entry announcement and joined
itself), and `AddPlayerToGame` counted with a bare `CurrPlayers++` on top of a `CountPlayers()` that
had already counted everyone. Both ported here (`BOB_LOOPBACK_SELF=1`, `BOB_MP_NORECOUNT=1` revert).

**MEASURED, two-instance session:**

    before   [agg] GATE num=1 CurrPlayers=4
    after    [agg] GATE num=1 CurrPlayers=2      (42/42 samples, both sides)

⭐ **The count is right now** — the half of the gate this sprint owned. MA cleared the whole gate on
the same two fixes and reached `synched=1`.

⚠️ **BoB does not**, and the difference is the OTHER half: `num` stays **1**. The aggregate packet
carries a dummy for slot 1 only (`IDCodes: 194 196 194 …`, DUMMY=196) on **every one of 42 samples**,
where MA's alternated and reached 2 between samples. So one of BoB's two players never gets its
dummy packet into the aggregate, persistently.

**Next sprint, and the instrument already exists in MA:** port the `[agg] aggregator received …
mapped-to-slot …` census (ma AGGRGTOR.CPP) and ask whether slot 0's dummy is never SENT, never
received by the aggregator, or received and not mapped to a slot. MA's census answered exactly that
question in one run.

**BoB MP: 2 sprints this pass. Rotating off.**

## MP S7 (Opus 5, 2026-09-14) — the host's OWN player stops feeding the aggregator after the first few seconds

S6 left BoB with the right player count but `num=1`: the aggregate carries a dummy for one player
only. MA's aggregator census was ported to answer which player, and whether its packets are never
sent, never received, or received and not mapped (`BOB_TRACE_AGG=1`).

**MEASURED, host, one second per line:**

    [agg] aggregator received  1 msg/s  right-size 1  mapped-to-slot 1  from: 3
    [agg] aggregator received  3 msg/s  right-size 3  mapped-to-slot 3  from: 4
    [agg] aggregator received 10 msg/s  right-size 10 mapped-to-slot 10 from: 3 4
    [agg] aggregator received  8 msg/s  right-size 8  mapped-to-slot 8  from: 4      <- and 4 only, thereafter

⭐ **Every packet that arrives is the right size and maps to a slot, so nothing is being dropped or
mis-addressed. The host's own player (pid 3) simply stops sending** after the first few seconds,
while the remote client (pid 4) keeps going. That is exactly one dummy in the aggregate, which is
what the gate sees.

The chain explains itself: a player sends its dummy only AFTER it receives an aggregate packet, so
the host's game half has stopped receiving them. The loopback is still firing at 8–9/s
(`loopback from=1 to=2`), the shim's queue never overflows (0 "queue full"), and the ids are the same
shape as MA's (aggregator pid 1, game player pid 3, group 2 = {3,4}). So the packet is being queued
and not delivered to the caller that wants it.

**Next: port MA's InitSyncPhase receive census** (`[agg] InitSyncPhase received N msg/s aggID=… From
seen: …`), which names what that caller actually gets. In MA the same instrument showed another
caller stealing the traffic, and BoB has more receive sites than MA does.

**BoB MP: 3 sprints this pass.**

## MP S8 (Opus 5, 2026-09-14) — the host's game half gets ONE of every NINE looped-back packets; two candidate causes ruled out by measurement

S7 found the host's own player feeding the aggregator for a few seconds and then stopping, which
means its `InitSyncPhase` stopped receiving the aggregate packet. S8 ported MA's receive census to
see what that caller actually gets.

**MEASURED, same session, the two sides side by side:**

    host    [agg] loopback 9/s  from=1 to=2          (the aggregator's packet IS queued locally)
    host    [agg] InitSyncPhase received 1 msg/s  aggID=1  From seen: 1
    host    [agg] InitSyncPhase got-agg-packet 0-2/s   no-packet 58-60/s
    client  [agg] InitSyncPhase received 9-11 msg/s aggID=1  From seen: 1 3
    client  [agg] InitSyncPhase got-agg-packet 8-9/s

⭐ **The packet is queued nine times a second and the caller that wants it sees one.** The client,
which has no local aggregator competing for its queue, sees all of them and clears its half. So the
loss is host-local and it is a DELIVERY problem, not a send problem.

**Two candidate causes tested, both REFUTED:**

1. ⛔ **The aggregator's own drain stealing group traffic.** MA's MP-6 S4 rule (a KNOWN group is
   routed by MEMBERSHIP; only an unknown id falls through to the permissive catch-all) was missing
   here, so this looked certain. Ported it — `BOB_MP_LOOSEGROUP=1` reverts — and re-ran: the host
   still sees **1-2 msg/s**, unchanged. The rule is kept for parity with MA's measured behaviour and
   because the two-instance gate still passes, but **it fixed nothing here and is not credited.**
2. ⛔ **Shim queue overflow.** ⚠️ S7 reported "0 queue full" — that claim was UNSOUND: the drop
   message is gated on `BOB_TRACE_DPLAY`, which that run did not set, so it was measured with the
   instrument switched off. The drop counter now also reports under `BOB_TRACE_AGG`, and with the
   instrument proven able to speak the answer is a real **zero drops on both sides**.

**S9 (next pass): name the thief.** MA has `ma_recv_caller`, a tag each call site sets before
`Receive` so a delivery can be attributed; **BoB has no equivalent** (`grep recv_caller` finds
nothing). Add it, tag every BoB receive site, and print which caller takes each looped-back aggregate
packet. MA's MP-6 S2 solved the identical class of question in one run with exactly that instrument.

**BoB MP: 4 sprints this pass (S5–S8). AT THE CAP — rotating the item off.**

## R21 defect (2) S1 (Opus 5, 2026-09-14) — ⭐ the black band is the terrain loop RUNNING OUT OF TILES, and the prediction lands on the pixel

The PO's zoomed campaign map is black from x≈1190 of 1852 while the squadron icons keep drawing
across the whole width. R21 recorded that mismatch as "two layers, one extent each, disagreeing" and
left it there.

**The terrain loop's own bound** (`MIGVIEW.CPP`, `BOB_TRACE_MAPEXT=1`):

    [mapext] rect=(0,0)-(1024,768) scroll=(500,800) zoom=2 zsq=256 area=(8,8) sstart=(1,3) end=(6,7)

so the terrain can cover `(areax − scroll.x/zsq) × zsq − (scroll.x mod zsq)` pixels and no more. With
the default scroll that is 1280 px into a 1024 px pane — wider than the pane, which is why the
harness's own screens have never shown the defect.

⚠️ **Fitting that formula to the PO's 1190 px needs one free parameter (their scroll), so it is not
evidence.** Instead a scroll override was added (`BOB_MAP_SCROLL="x,y"`, a test hook, not a fix), the
prediction was written down first, and then it was run:

* set `scroll.x = 1124` → `sstartx = 4`, `(8−4)×256 − 100` = **terrain must end at x = 924**.

**MEASURED, colour profile across row y=300 of the capture:**

| x | RGB | what |
|---|---|---|
| 860 | 140,124,60 | brown land |
| **924** | **127,147,137** | the terrain STOPS, to the pixel |
| 976 | 232,199,70 | the scale ruler |

⭐ **924, exactly as predicted.** The terrain's right edge is the tile loop's arithmetic, nothing else.

⭐ **And the colour at 924 names the second half of the answer.** `RGB(127,147,137)` is the map
code's own `FillSolidRect` backdrop — the one it paints only when **more than four tiles have not
loaded yet**. It is not an out-of-extent fill. So when the tiles ARE loaded, as in the PO's session,
nothing paints the uncovered area at all and it stays **BLACK**, while the overlay — which is not
bounded by the tile grid — keeps drawing its icons over it. That is the PO's frame exactly, including
"the icons cover ground the terrain does not".

**The fix is a choice, and it should be made deliberately:** clamp the scroll so the pane is always
covered (what the original almost certainly does, and it needs checking against the gold), or paint
the uncovered region unconditionally instead of only as a loading backdrop. The first is correct; the
second is safe. **Not shipped this sprint** — no fix goes in on the strength of a mechanism alone
when the gold can settle which behaviour is right.

**R21(2): 1 sprint.**

## R21 defect (2) S2 (Opus 5, 2026-09-14) — ⚠️ S1's experiment used an ILLEGAL scroll; the game DOES clamp, and the clamp is skipped on any page but 0

S1 explained the PO's black band as the terrain loop running out of tiles, and predicted the edge to
the pixel by setting `scroll.x = 1124` through a new test hook. S2 checked two things S1 did not.

**1. The gold never shows it.** Sampling the PO's own gold video (`bob_convoy_campaign.mp4`, one frame
every 4 s) and keeping the frames that are actually the strategic map: **13 consecutive map frames,
black area 0.0%**. Whatever the original does, it does not leave black on the map.

⭐ **2. The game clamps the scroll, and S1's experiment violated that clamp.** `CMIGView::UpdateScrollbars`
(`MIGVIEW.CPP:3693`):

    m_scrollpoint.x = min(m_scrollpoint.x, m_size.cx - rect.right);
    m_scrollpoint.x = max(m_scrollpoint.x, 0);

With `m_size.cx = 256*4*zoom - 5` = **2043** at zoom 2 and a 1024-wide client, the largest legal
scroll is **1019** — and S1 set **1124**. ⚠️ **So S1 demonstrated the mechanism in a state the game's
own rules forbid, and its write-up does not say so.** The correction matters because the clamp is
exactly sized to prevent the defect: at the PO's ~1852-wide window the limit is 2043−1852 = 191,
which leaves the terrain covering 8×256 − 191 = **1857 px of an 1852 px pane** — i.e. the clamp
guarantees coverage with 5 px to spare.

⭐ **So the defect is not "the map can be scrolled past its edge". It is that the clamp did not hold
in the PO's session** — and the code says where that can happen:

    void CMIGView::UpdateScrollbars() {
        if (m_currentpage != 0) { hide both scrollbars; }      // <-- returns without clamping
        else { ...the clamp above... }

**On any page other than 0 the scroll is never clamped at all.** The PO's frames have the Messages
dialog open over the map, which is a page change. That is the prime suspect and it is one trace away.

**S3:** report `m_currentpage`, `m_size`, the client rect and `m_scrollpoint` on every map paint
(extend `BOB_TRACE_MAPEXT`), reach the PO's state (campaign → convoys → Messages), and see whether
the scroll exceeds `m_size − client` while `m_currentpage != 0`. If it does, the fix is to clamp
regardless of page rather than to touch the drawing at all.

**R21(2): 2 sprints.**

## R21 defect (2) S3 (Opus 5, 2026-09-14) — ✅ FIXED: the only clamp in the game never runs on the paint path, so the scroll is now bounded where it is USED

S2 found the game's scroll clamp (`CMIGView::UpdateScrollbars`) and noted it sits in the else-branch
of `if (m_currentpage != 0)`, making the page the prime suspect. S3 instrumented that function to
find out which branch runs.

⭐ **It runs NEITHER. `UpdateScrollbars` was called ZERO times in a 900-tick map session** — the map
opens, paints and scrolls without the clamp ever executing. Its callers are the scrollbar handlers,
`OnSize` and the zoom path, and none of them fired here. So the page hypothesis was not even reached:
**nothing bounds `m_scrollpoint` on the path that paints the map.**

That is the defect. An unbounded scroll leaves the tile loop short of the pane, and the remainder is
unpainted — black in the PO's frames, because the grey-green fill in that code is only a
"tiles still loading" backdrop (S1) and not an out-of-extent fill.

**FIX** (`MIGVIEW.CPP`, the tile-draw path, `BOB_NO_MAPCLAMP=1` reverts): apply **the game's own
bound** — `0 <= scroll <= size − client` — at the point of use, so it holds however the scroll got
there.

**VERIFIED with a stated prediction.** Setting the scroll to 1124, which S2 showed is illegal at a
1024-wide client (limit 2043 − 1024 = **1019**):

| | before the fix | after |
|---|---|---|
| scroll reported | 1124 | **1019**, exactly the game's limit |
| terrain coverage | 924 px of a 1024 px pane | **1280 px**, pane fully covered |
| colour at x=924 | 127,147,137 (the loading backdrop) | brown land, continuous to the ruler at 976 |

⚠️ **What this does not do:** it treats the symptom at the paint, not the cause of `UpdateScrollbars`
never being called. That function also sizes and shows the scrollbars, so whatever keeps it from
running may be hiding those too. **S4 should ask why it never runs** — but the map now cannot show a
black band regardless of the answer, which is what the PO reported.

**R21(2): 3 sprints.**

## R20 S1 (Opus 5, 2026-09-14) — one real-GL campaign flight, and the square does NOT appear; the instrument speaks, and says there was only ONE untextured draw shape

R20 has been waiting on "one display slot" since S409, which measured that the 3-D draw path does not
execute under `SDL_VIDEODRIVER=dummy`. S1 spent that slot: a full campaign flight on real GL
(`BOB_TRACE_GREY=1`, `BOB_DUMP_FRAME=400`), reaching `InThe3D=1` and dumping a 1920x1080 frame.

**RESULT — the defect did not reproduce.**

* The captured frame is a clean Hurricane cockpit over the Channel: canopy, gunsight glass and
  reticle, sea, sky, the artificial horizon, and the in-flight menu panel at the top centre.
* **No dark square.** The one grey box in the frame is that menu panel, measured at RGB
  ≈(165,165,165) against a sky of (138,169,197) — nothing like the PO's (40,52,52).
* The trace reported exactly **one distinct untextured draw shape** in the whole flight:
  `prim=6 count=4 fvf=3c4 is2D=1, flat colour 0xffff0000 = R255 G0 B0`.

⭐ **And the instrument is not the reason.** It dedups on `(fvf, prim, count)`, not on the surface —
S397 already fixed the earlier version that collapsed every texturing-disabled draw onto one entry —
so "one line" here means **one distinct shape occurred**, not "one was recorded". A red 2-D quad is
not the PO's square, and nothing else untextured was drawn.

**So the square needs a state this flight did not reach**, and the PO's own note says which:
*"when I pressed printscreen (may have hit a nearby button first)"*, and their evidence file is named
`inflight_gfx_blacksquare.png` — **the GFX dialog raised over the 3-D**. A dialog over the flight view
is exactly the kind of state that adds an overlay quad, and it is also consistent with the square's
colour matching the ground band rather than any sky or cockpit shade.

**S2:** fly the same campaign flight and RAISE THE GFX DIALOG over it (the PO's screenshot names the
screen), then read the `[grey]` lines. One more display slot, with a specific state this time rather
than a general flight.

**R20: 1 sprint this pass.**

## R20 S2 (Opus 5, 2026-09-14) — ⛔ the mirror hypothesis is NOT supported; two real-GL flights, no square

S1 flew a campaign sortie on real GL and found no dark square. S2 tested the best remaining
hypothesis before spending more slots on guesses.

**The hypothesis.** The PO's square is ONE flat colour, RGB (40,52,52), and their own frame's ground
band is (34,44,48) — six units apart. BoB renders the rear-view mirror by drawing the landscape into
a 128x128 render target (`BOB_MIRROR` gates it, and MIRROR-1 measured the pass running 4,136 times in
a 70 s flight). **A landscape-shaded quad drawn in the world instead of in the cockpit is exactly a
flat dark square floating in the sky**, and it would explain the colour matching the ground.

**MEASURED: the same campaign flight with `BOB_MIRROR=1`.** The frame is indistinguishable from S1's
— same cockpit, same sea and sky, same in-flight menu panel, **no dark square** — and the `[grey]`
trace again reports exactly one distinct untextured draw shape, the same red 2-D quad.
**Hypothesis not supported.**

⚠️ **One number in this sprint is worthless and I am saying so rather than quoting it.** A colour
search for "pixels within 25 of (40,52,52)" in the upper half returned **137,704**, which looks like a
finding and is not: the bounding box spans the full frame width and the matches are **the sea**,
which is that colour. A colour window wide enough to catch a shade is wide enough to catch a whole
ocean. The visual check is what settles this frame, not that count.

**Where this leaves R20.** Two real-GL campaign flights, two frames, no reproduction, and the one
instrument that could name the draw says only one untextured shape exists in a normal flight. The
PO's evidence file is named `inflight_gfx_blacksquare.png` and their note says *"may have hit a
nearby button first"* — **so the reproduction depends on an action only they can name.**

**S3 should not be another guessed state.** Ask the PO what was pressed just before that screenshot,
or whether the square is still there in the current build. The instrument is ready and one informed
run would finish it; three uninformed ones would not.

**R20: 2 sprints this pass.**

## R21 defect (2) S4 (Opus 5, 2026-09-14) — the map clamp is regression-checked: parity 8/8 byte-identical, campaign gate green

S3 shipped the paint-path scroll clamp on the strength of one A/B. S4 checks it against the suites
before it reaches the PO, because a bound applied on the paint path touches every map draw.

| gate | result |
|---|---|
| `bob_parity.sh` (screens vs references) | **PASS — 8 screens byte-identical** |
| `bob_convoy_campaign.sh` (Luftwaffe Convoys, map → directives → Fly) | **PASS — Fly reached 3D, no fatal error** |

⚠️ **What the parity gate does and does not cover, stated so the green is not over-read.** Its eight
screens are the main menu and the config tabs — **the campaign map is not among them**, so those
byte-identical results say the change did not leak into the front end, not that the map is right. The
map's own coverage is the campaign gate above, which drives the real screen and reaches 3-D.

⚠️ **The full `bob_gates.sh` suite did NOT complete** — my 900 s timeout killed it mid-run (exit 143)
after it had written its captures. That is a harness limit of mine, not a failure of the suite, and it
is recorded rather than glossed: the two gates that bear on this change were then run individually and
both passed.

**R21(2): 4 sprints — AT THE CAP. The defect the PO reported is fixed, verified against the value
that used to break it, and regression-checked.**

## R21 defect (3) S1 (Opus 5, 2026-09-14) — the stray dialog fragment does not appear in a campaign-map session either

R21's third defect is *"a title bar with `? ✓ ✗` at roughly (0,18)-(232,40), with SECTOR W beneath —
a second dialog drawn at the origin with no body"*. S1 looked for it with the census that would
catch it.

**MEASURED, campaign map reached headlessly, `BOB_DUMP_HITTARGETS=1`:**

    [hittargets] menu rects: n=2
    [hittargets]   menu[0] = (35,710 76x54)
    [hittargets]   menu[1] = (111,710 83x54)
    [hittargets] panel 1 hosted controls:
    [hittargets]   (0 hosted controls for this dialog)

**No dialog at the origin, and nothing near (0,18)-(232,40).** The map capture from the same session
has the map drawn to its own left edge with a `SECTOR E` label there — the label the PO saw *beneath*
their fragment is normal map furniture; what they had **above** it was a dialog title bar, and this
session has none.

⭐ **A pattern worth naming, because it is now three items.** R20 (the floating square), R21(3) (this
fragment), and the PO's GFX-dialog-over-flight state all reproduce only in a session state the
harness does not reach, and in all three the PO's own note points at an interaction they made. The
port's own screens are reproducible headlessly and these are not, which is itself information: **they
are states reached by clicking, not states the game enters on its own.**

**S2 — build the detector instead of hunting the state.** A dialog whose title bar draws at the
origin, or whose body has zero extent, is a condition the port can NOTICE: one line at the dialog
draw when `rect.left == 0 and rect.top < 64 and body height == 0`. Then the next time the PO sees it,
the log names the dialog instead of a screenshot starting another search. Cheaper than guessing
states, and it also covers R20's class.

**R21(3): 1 sprint.**

## R21 defect (3) S2 (Opus 5, 2026-09-14) — ⭐ the stray fragment is NAMED: it is an `RDEmptyD` placeholder panel, drawn at (44,20) with no body

S1 could not reproduce the PO's fragment and proposed building a detector instead of hunting the
state. S2 built it, proved it, and it found the thing on the first ordinary run.

**The detector** (`bob_ole_draw_panel`, always on): a panel asked to draw near the top-left corner
that draws **zero controls** is exactly *"a title bar at the origin with no body"*, so say so, once
per dialog.

⚠️ **It reported nothing — and a check that has never fired cannot be distinguished from one that
cannot fire.** So the thresholds were made overridable (`BOB_ORIGIN_DLG_X/Y`) purely to prove it can
speak. With them widened it printed immediately:

    [origin-dialog] class=8RDEmptyD dialog=0x9fc77f0 drawn at (44,20) with ZERO controls drawn

⭐ **And that is the PO's fragment.** Their title bar sits at **(0,18)-(232,40)**; this panel draws at
**(44,20)** with nothing in it. **My default x limit of 2 was set from their screenshot's left edge
and would have missed it by 42 px** — the reason to prove an instrument rather than trust its
silence, twice over in one sprint.

⭐ **What it is.** `RDEmptyD` is the engine's **placeholder panel**: `MAINFRM.CPP:2053` already
records *"the logged child is a PLACEHOLDER PANEL (traced: rtti=RDEmptyD), not the dialog …
the real dialog hangs off the panel as `fchild`"*. So the port is drawing a **frame and title bar for
a panel whose content lives in a child**, and with the map behind it that is a title bar with
`SECTOR W` showing through — the PO's picture exactly.

**The default limits are now x ≤ 64, y ≤ 64**, so the condition is reported in ordinary runs.

**S3:** decide what a placeholder panel should draw. It should not paint a title bar where its child
is not, so either suppress the frame when `fchild` carries the content, or draw the placeholder at
the child's position. Check first whether the PO's *visible* chrome comes from this panel or from the
child's own title bar drawn at the parent's origin — the two want different fixes.

**R21(3): 2 sprints. The fragment is identified from a normal session, without needing the PO's
steps.**

## R21 defect (3) S3 (Opus 5, 2026-09-14) — the placeholder hosts NOTHING, so its chrome cannot come from the control path

S2 named the fragment as an `RDEmptyD` placeholder panel drawn at (44,20) with zero controls. S3 asks
the obvious follow-up before proposing a fix: are its controls being SKIPPED (a filter problem, which
would be fixable there) or does it genuinely host none?

**MEASURED, the skip census for that exact object:**

    [skip] dlgId=-1 dialog=0x9072bb0 hosted-for-this-dialog=0 DREW=0 |
           not-visible=0 dead-sweep-row=0 not-in-template=0 no-DLU-rect=0

⭐ **Zero hosted, zero drawn, and zero skipped by any of the four filters.** The panel is empty in
every sense — nothing was dropped on the way to the screen. So **no control path can be drawing the
PO's `? ✓ ✗` title bar**, and the fix is not in the control filters.

**What that leaves.** A panel's own ART is blitted separately from its hosted controls
(`SetDIBitsToDevice`, the path S173's clip works on). A placeholder with a title-bar art file and no
controls would paint exactly what the PO photographed: chrome with the map showing through where a
body should be. In this session it paints nothing visible, so either it has no art assigned here or
the art draw is skipped — and those are different bugs.

**S4:** trace the panel-ART blit for this object — does `RDEmptyD` get an art FileNum, and is it
drawn? That is one trace on the path that is left, now that the control path is ruled out by
measurement rather than by argument.

**R21(3): 3 sprints.**

## R21 defect (3) S4 (Opus 5, 2026-09-14) — the fragment's position is the SCREEN'S OWN dial slot, and the report now names it

S3 ruled out the control path: the placeholder hosts nothing and nothing was filtered. S4 asks where
(44,20) comes from, because a position that is not chosen by a control must be chosen by a layout.

**It is `dials[1]` of the current screen.** `bob_fp_repaint` draws each panel at
`fp->m_currentscreen->resolutions[res].dials[d].X/Y`, so the detector now records which slot and
which screen it was painting:

    [origin-dialog] class=8RDEmptyD dlgId=-1 dial-slot=1 screen-art=27922 drawn at (44,20) with ZERO controls drawn

⭐ **So the chain is complete: the screen declares a dial slot at (44,20); the panel placed in it is
an `RDEmptyD` placeholder whose real content hangs off it as `fchild`; and the panel's own
`DoPaint()` runs regardless of whether anything is in it.** In this session that paints nothing
visible; in the PO's it painted a title bar with the map showing through.

⭐ **And the report is now actionable without reproducing the state.** The next time the PO sees the
fragment, one grep gives the screen's art id and the slot index — which screen, which dial, which
class. Three sprints ago this was a screenshot with no way in.

**S5 (the fix, and it is a choice):** suppress the frame when a placeholder's content lives in a
child, or draw the placeholder where its child is. The second is only right if the child is meant to
be in that slot; the first is safe either way. Decide with the gold: the reference screens for this
port show what a dial slot looks like when its panel is empty.

**R21(3): 4 sprints — AT THE CAP. From "a title bar in a screenshot with no reproduction" to
"screen-art 27922, dial slot 1, class RDEmptyD", entirely from ordinary runs.**

## R3.9 S5 (Opus 5, 2026-09-14) — ⭐⭐ THE SPRITE IS NAMED: image map 525 is `MskMap16\THREAT01.X8`, and its ONLY draw site is the HUD

The 2026-09-03 cycle ended with *"turn dir 2 / file 13 into a filename — log the path the image-map
LOADER opens"*. That is what this sprint does, and the answer arrives with a second answer attached.

**The trace now prints the path, from the game's own composer.** `LoadImageMap` computes a `FileNum`
and hands it to a `fileblock`; `fileman::namenumberedfile()` is the function that turns that number
into a path, so the S309 probe now calls it (`LessFail`, so a bad number cannot assert inside a
diagnostic) and prints what is about to be opened:

    [imagemap] ptr=0xdaa94680 dir=2 file=13 ImageMapNumber=0x020d FileNum=39693 (16bit,
               off8=38912 off16=39680) path=...\mskmap16\THREAT01.x8

⭐ **Image map 525 is `MskMap16\THREAT01.X8`** — confirmed present on disk. And the number alone
could never have said so: **dir 2 has TWO file lists**, `maskmap\` at `off8=38912` and `mskmap16\` at
`off16=39680`, and the same `file=13` names a different file in each. Only the FileNum actually used
resolves it, which is why four sprints of arithmetic on `ImageMapNumber` could not finish this.

⭐ **And `THREAT01NO` has exactly ONE reference in the whole source tree:**

    SRC/3D/OVERLAY.CPP:7805   ImageMapDesc* pmap = Image_Map.GetImageMapPtr(THREAT01NO);   // COverlay::DoThreat()

`DoThreat()` is called from the overlay pass at `OVERLAY.CPP:680`, immediately after
`LoadIdentity(MATRIX_OBJECT/VIEWER/PROJECTION)` and `GiveHint(HINT_2DRENDER)`, and only when
`Save_Data.gamedifficulty[GD_HUDINSTACTIVE]` is set. **It is a HUD instrument — a threat indicator,
drawn in 2D, screen-pinned by construction.**

⚠️ **Which contradicts what this item measured in September, and the contradiction is the defect.**
The earlier cycle established by backtrace that the shape is drawn from `RenderTPolyList` inside
`render3d`, that it is NOT screen-pinned, and that its x moves between frames. Both observations can
be true at once in exactly one way: **the HUD's transparent poly is QUEUED under identity matrices
and FLUSHED later, inside the 3D pass, under the WORLD matrices.** A 2D instrument drawn with a
world transform is precisely "a grey shape floating in the sky that drifts between frames".

**S6, two experiments, both cheap and both falsifiable:**
1. Turn `GD_HUDINSTACTIVE` off. If the shape goes with it, the identity is settled end to end.
2. Trace the matrix state at `BeginPoly` (queue) and at `RenderTPolyList` (flush) for this material.
   If they differ, the fix is to flush the 2D list with the matrices in force when it was queued.

*(And the four-sprint caveat can now be put to the PO as a question with a name in it: "the object is
the threat indicator from the HUD — do you have HUD instruments switched on?" — better than asking
them to describe a square again.)*

The probe is inside the existing `BOB_TRACE_IMAGEMAP` block, so nothing changes with the trace off;
the campaign flight that produced the line above ran 240 s with no crash.

**R3.9: 1 sprint this pass. The shape has a filename and a single draw site.**

## R3.9 S6 (Opus 5, 2026-09-14) — ⭐⭐ the "floating grey shape" is the THREAT INDICATOR, drawn where it belongs; and S5's mechanism is ruled out by a `#define`

S5 named the texture (`MskMap16\THREAT01.X8`) and its one draw site (`COverlay::DoThreat`), and
proposed a mechanism for the contradiction — a 2D HUD quad queued under identity matrices and flushed
later under the world matrices. S6 tested both halves.

⛔ **The mechanism is impossible in this build, and reading settles it without a run.** The deferred
path is the hardware-T&L branch in `Lib3D::PrepareForClipping` — the one that stores
`sPolygon->transMat = transMatPos` and defers the transform to `RenderTPolyList`. It sits under
`#ifndef NO_HARD_TNL`, and **`LIB3D.CPP:134` is `#define NO_HARD_TNL`.** Transparent polys therefore
take the software path: `CombineMatrices()` and a CPU transform **at submission**, so a poly carries
the matrices in force when it was built. *(Confirming it: that branch calls `CombineTranspMat()`,
whose only definition in the tree is commented out — it would not link if it were compiled.)*

⭐ **And the positive half is better than the hypothesis it replaces.** `DoThreat()` runs in an
ordinary campaign flight (no special difficulty setting needed), and the new `[dothreat]` probe
prints the rect it occupies:

    [dothreat] surface=1920x1080 SCX=3.000 SCY=1.688 quad=(30.0,16.9)-(330.0,316.9)

**A 300 x 300 screen-fixed box in the top-left corner.** R3.9's pixel probe sampled the grey shape at
**(180,165)** — **inside that box**.

⭐⭐ **So the object is not stray and is not lost in world space: it is the threat indicator,
rendered at its intended position and size.** The comment above the function has said so since 2000
("display threat indicator in the top left corner of the screen"); four sprints of treating it as an
unidentified floating sprite were chasing an instrument. **The defect the PO sees is in how it LOOKS
— a large pale box over the sky — not in where it is.**

⚠️ **One scaling oddity, recorded but not yet a defect claim.** Both axes divide by 640:
`SCX = physicalWidth/640`, `SCY = physicalHeight/640`. The X factor is a clean 3.0 at 1920; the Y
factor is 1.688, and the quad then uses `sRADIUS` (an X-scaled length) for BOTH its width and its
height, so the box stays square while its ORIGIN moves by the Y factor. That is self-consistent, but
it means the indicator's top-left Y drifts with aspect in a way its size does not.

**S7 goes to the gold, which is what this item has been missing:** capture the same view under Wine
at the same resolution and compare the top-left 330 x 320 corner. Either the original draws the same
box — and the PO's complaint is that a HUD instrument they did not expect is on screen — or it draws
something different there, and the difference IS the defect. Also print the threat COUNT the function
computes (`cnt` from the `MAX_THREATS` scan), because a scope with no blips on it is exactly a plain
pale box.

**R3.9: 2 sprints this pass. The unidentified sprite is identified, sited, and demoted to an
instrument.**

## R3.9 S7 (Opus 5, 2026-09-14) — the scope is NOT blank (33 contacts), the off-by-one is not one, and ⚠️ **this item has no gold to be compared against**

S6 sited the object and asked three questions. S7 answers all three, two of them by reading.

**1. The scope has content.** The blip count is now printed rather than guessed:

    [dothreat] blips=33 (GetThreatData last index 32; 0 blips = background sprite only)

**33 contacts** in an ordinary convoy-campaign flight, drawn as red/blue sticks over the sprite. So
"a scope with no blips on it is exactly a plain pale box" — S6's own suggestion — is **wrong**, and
the pale box the PO sees is not an empty instrument.

**2. `if (++cnt>0)` is not an off-by-one**, which is what it looks like. `ViewPoint::GetThreatData`
starts `count=-1` and PRE-increments before each store, so it returns the LAST INDEX; `++cnt` turns
that into a count, and with no contacts it yields 0 and the loop is skipped. Checked by reading, at
zero run cost, before it could become a sprint.

⚠️ **3. There is no BoB gold for this.** The gold set is **19 screenshots and one 185 s video**;
sampled every screenshot and five points across the video, **every one is the front end or the
campaign map** — Directives, Target List, the phase-select screen, the strategic map. **Not one
in-flight 3D frame exists.** So R3.9 cannot be settled the way MA's GOLD3D-1 was this morning, and
any sprint that plans to "compare with the gold" here should stop and read this line first.

⭐ **Which leaves the art, and it is the sharpest remaining hypothesis.** `THREAT01.X8` is a 128×128
**colour-keyed** sprite (the pixel probe recorded `masked=2`) stretched over a 300×300 screen box. If
the key is honoured, the scope is a ring and a few marks over the sky; **if it is not, the whole
texture paints as an opaque 300×300 panel in the top-left corner — which is precisely "a floating
grey square".**

**S8:** dump the decoded `THREAT01` texels and count how many are the key colour, then read the same
region out of a rendered frame. A sprite that is 80% key on disk and 0% transparent on screen is the
defect, and both numbers come from one run.

**R3.9: 3 sprints this pass. Two candidate explanations closed, one sharpened, and the item's oracle
situation is now on the record.**

## R3.9 S8 (Opus 5, 2026-09-14) — ⭐ THREAT01 is a two-tone STENCIL with an alpha plane: what it looks like is decided entirely by the alpha, and the loader and the drawn material disagree about masking

S7 narrowed the item to the art. S8 measured it, at load time, from the game's own loader:

    [imagemap] ... path=...\mskmap16\THREAT01.x8
    [imagemap]   128x128 isMasked=0 alpha=yes  index0=0 of 16384 (0.0%)  distinct=2

⭐ **Two distinct byte values in the whole 128×128 body.** This is not a picture — it is a
**stencil**: one value for the scope, one for everything around it, with an **alpha plane** carrying
the actual shape. Neighbouring maps from the same directories, measured in the same run, show what
normal art looks like:

| map | size | distinct | index 0 |
|---|---|---|---|
| `THREAT01.x8` | 128×128 | **2** | 0.0% |
| `BRIST2.x8` | 128×128 | 138 | 0.0% |
| `HURWING.x8` | 1024×1024 | 254 | 22.2% |

⭐⭐ **So the whole appearance of this sprite is decided by the alpha plane.** Honour it and the scope
is a thin ring over the sky; ignore it and a 300×300 box of ONE flat colour lands in the top-left
corner — **which is precisely a "floating grey square", and explains why four sprints of looking for
geometry found none: there is no wrong geometry, only a missing transparency.**

⚠️ **And the two masking flags do not agree.** The loaded map reports **`isMasked=0`**, while R3.9's
pixel probe recorded the DRAWN material as **`masked=2`**. One of those decides whether the sprite is
composited with transparency. That disagreement is now the shortest thread in the item.

**S9:** follow the alpha plane to the screen. `LIB3D.CPP:6686/6865` builds the texture from
`body + palette + alpha`, and the port then uploads that surface through `bob_video`'s GL path
(`BOB_DUMP_TEX` already dumps uploaded surfaces). Dump the THREAT01 upload and count how many texels
are transparent: **if the disk art is an alpha stencil and the uploaded surface has no transparency,
the defect is in the upload and the fix is there.** This is the GPU-hop rule — the CPU-side numbers
are all in hand and none of them can show what the texel became.

**R3.9: 4 sprints this pass — AT THE CAP, and the item has gone from "an unidentified floating grey
shape" to "a named sprite, a named draw site, a measured two-tone stencil and one disagreement left
to resolve".**

## R3.7 S5 (Opus 5, 2026-09-14) — ⭐⭐ **THE TRACERS RENDER.** First positive result the item has ever had, with a control

S4 unblocked this item by proving the guns fire; four sprints before that were spent on a scaffold
that was switched off. S5 asks R3.7's actual question for the first time: **does anything appear?**

**MEASURED, one firing frame and one control frame at the same frame index:**

| run | bright orange/yellow pixels in the sky right of the gunsight |
|---|---|
| `BOB_AUTOFLY=shoot` (magazine emptying, `Gun Ammo: 1695` on the HUD) | **1,518** |
| same recipe, guns silent | **0** |

⭐ **A tracer streak, drawn in the sky, gone when the guns are not firing.** The capture
(`parity/r37_s5_tracers_260914.png`, 1920×1080, real GL) also shows the gunsight reticle and its
reflector glass rendering, the gyro instrument, the panel and the HUD ammo readout counting down
through the burst. **R3.7 is no longer blocked and no longer hypothetical.**

⚠️ **The recipe matters, and the wrong one says "nothing fires".** The campaign path under
`SDL_VIDEODRIVER=dummy` — the recipe every other BoB item in this tree uses — **never arms the shoot
branch**: one `[fire]` line, `KeyPress3d(SHOOT)=0`, no gate at all. The working recipe is S1's, on
REAL GL: `DISPLAY=:0 BOB_BOOT_FRONTEND=1 BOB_AUTOFLY=shoot`. **Written down here because this is the
fourth harness in one session whose silence looked like a defect.**

⛔ **One blind alley closed by reading, at no run cost.** `SMK_TRACER` looks like the tracer
mechanism and is not: `3DCOM.CPP:27769` is `case SMK_TRACER: break;` — empty — the other reference is
commented-out old code, and **nothing in the tree ever PRODUCES the value.** It is a dead enum, so the
empty case is not a missing draw. Whatever draws these tracers is elsewhere, and it works.

**S6, now that something is on screen:** this camera cannot see the muzzles (the Spitfire's guns are
in the wings, out of the cockpit view), so the next capture should be an external view during the
same burst — muzzle flash, wing smoke and the rounds' departure are all in frame there. Then the
comparison is against the PO's description rather than against nothing.

**R3.7: 1 sprint this pass. The item finally has a picture.**

## R3.7 S6 (Opus 5, 2026-09-14) — ⭐ tracers confirmed from OUTSIDE; ⚠️ **the muzzle flash does not render**, measured against a control

S5 saw tracers from the cockpit and noted the Spitfire's wing guns are out of that frame. S6 built
the two harness pieces that were missing and took the capture from outside.

**Two harness fixes, both of which hid the answer:**
* `shoot` and `view<hex>` were **mutually exclusive** — the shoot test is `strstr` and comes first,
  so `BOB_AUTOFLY=view40shoot` fired and never changed view. The shoot branch now accepts a
  `view<hex>` anywhere in the mode string and taps it once before the burst (`shootview40`).
* **`BOB_DUMP_ON_FIRE=1`** dumps the first frame in which the trigger is actually HELD. The burst is
  20 ticks on in every 60, so a fixed frame index lands in the quiet phase **two times in three** —
  and an effects capture that misses the burst is indistinguishable from an effect that is not drawn.
  *(The first attempt of this sprint dumped nothing at all, because the dumper's early-return guard
  did not know about the new variable. Fifth harness silence of the session.)*

**MEASURED — one frame with the trigger held (ammo 2475, mid-burst) and one control frame at the
same index with the guns silent, same camera, bright orange/yellow pixels:**

| region | firing | control |
|---|---|---|
| sky (rounds in flight) | **25** | **0** |
| wings (muzzle flash) | 198 | **191** |

⭐ **Tracers render from the external view as well** — 25 hot pixels in the sky, none at all in the
control, a round visible in flight above the aircraft.

⚠️ **The muzzle flash does NOT.** The wing band differs by **7 pixels out of ~191**, and that band is
full of the aircraft's orange-brown camouflage, its roundels and a hangar on the horizon. Firing four
wing guns produces no emission there the control does not already have. **That is a real, visible
gap in R3.7's subject: the rounds are drawn, the guns that fire them are not.**

*(Capture filed: `parity/r37_s6_external_firing_260914.png`, 1920×1080, real GL, trigger held.)*

**S7:** find the muzzle-flash draw. The gun positions are known to the sim (`Guns`/`TRANSITE`
fires from them), so the question is whether an effect is spawned at all or spawned and not drawn —
the same fork R3.9 had to resolve, and the same two instruments answer it: a counter at the spawn
site, then a pixel at the draw.

**R3.7: 2 sprints this pass. One effect confirmed working, one confirmed missing — both measured.**

## R3.7 S7 (Opus 5, 2026-09-14) — ⭐⭐ ROOT CAUSE: the muzzle-flash bit is computed every frame and **read nowhere**

S6 measured no emission at the wings while the guns fired. S7 traced the chain, and it ends in mid-air.

**The chain, all of it original Rowan code:**

    TRANSITE.CPP:4073   acadptr->cannonshooting = 1;   // LT_CANNON
    TRANSITE.CPP:4075   acadptr->gunshooting    = 1;   // LT_BULLET
    3DCOM.CPP:17570     if (adptr->gunshooting)  adptr->muzzleflash = !adptr->muzzleflash;
    3DCOM.CPP:17579     if (adptr->cannonshooting) adptr->cannonflash = !adptr->cannonflash;

**And then nothing.** Searching the whole tree (`grep -a`, both filename cases, `.CPP`/`.cpp`/`.H`):

| bit, same bitfield in `ANIMDATA.H` | references | has a READER |
|---|---|---|
| `lighttoggle` | 7 | **yes** — `3DCOM.CPP:24082  if (adptr->lighttoggle)` |
| `hassmoked` | many | **yes** — `3DCOM.CPP:24830, 25051, 25060` |
| **`muzzleflash`** | **4** | **NO — every one is a write** |
| **`cannonflash`** | **4** | **NO — every one is a write** |
| `rearshooting` | 1 | no — the declaration only |

⭐ **The two neighbours in the same byte are read; the two flash bits are not.** That is not a search
artefact — it is the same grep, over the same struct, in the same file, finding readers for the bits
that have them. **The engine computes a per-frame muzzle-flash flicker that nothing draws**, which is
exactly what S6 measured in pixels: 198 hot pixels at the wings while firing, 191 with the guns
silent.

⚠️ **This is very probably UPSTREAM, not a port regression.** Every line above carries an original
`//RJS 25Aug00` comment, and the port has not touched any of them. So the honest reading is *"the
1999 code sets a flash flag whose consumer is absent from this source tree"* — which could mean the
consumer was lost before the source we have, or that the shipped game does not draw one either.

⛔ **And BoB cannot settle that from its gold**, because R3.9 S7 established there is no in-flight
frame in the gold set at all — 19 screenshots and a 185 s video, every one front-end or campaign map.
**The only oracle that could answer "does the original flash?" is the Wine build**, which is the same
artefact GMOBJ-1 has been waiting for in the FreeFalcon tree.

**S8, and it is a choice for the PO rather than a bug to fix quietly:**
1. **Ask.** "The guns fire and the tracers draw; the original's muzzle-flash flag has no consumer in
   this source. Do you see a flash under Wine?" One answer closes the item either way.
2. **Or implement one**, flag-gated, since the flicker is already computed: draw a short bright quad
   at the gun positions while `muzzleflash` is set. That is a FEATURE, not a repair, and it must be
   labelled as one.

**R3.7: 3 sprints this pass. Two effects examined: tracers work, the flash has no draw path.**

## R3.7 S8 (Opus 5, 2026-09-14) — ⚠️ S7's "read nowhere" was reached by the wrong method; measured properly, the flash byte is still never addressed

S7 concluded the muzzle-flash bit has no consumer **from a name search**. S8 found the reason that is
not sufficient, and then did the measurement anyway.

⚠️ **The correction.** `3DCOM.CPP:1988` reads the anim data **by BYTE OFFSET supplied by the shape
file**:

    val = GlobalAdptr[ptr->animoffset];       // nobytes 1/2/4

**A `.3do` can therefore read the byte holding `muzzleflash` without any C++ identifier naming it**,
and every grep in S7 would miss it. "No reader by name" is not "no reader" in an engine whose
animation is data-driven.

⭐ **MEASURED instead, in a full firing sortie (real GL, the S6 recipe):**

    [animoff] AircraftAnimData: aclightclock2 at 140, so the
              lighttoggle/hassmoked/MUZZLEFLASH/cannonflash byte is offset 141
    [animoff] shape reads anim byte offset 191 (nobytes=2)

**One offset addressed in the whole flight — 191 — and the flash byte is 141.** The shapes in this
sortie never read the byte the flash lives in.

⚠️ **Scope, stated because it is narrower than it looks.** This is *one* of the interpreter's
anim-reading sites; `3DCOM.CPP:2311` reads `animoffsrc` and there may be others. So what is
established is **"no shape addressed offset 141 through the `animoffset` path during a firing
flight"** — much stronger than a grep, and not yet the whole claim.

**S9:** instrument the remaining anim-read sites (`animoffsrc`, and anything else indexing
`GlobalAdptr`) the same way. If none of them touches 141 either, *"the flash flag has no consumer"*
is established by measurement and S7's question to the PO — implement one as a labelled FEATURE, or
leave it as the original behaves — is the whole of what is left.

**R3.7: 4 sprints this pass — AT THE CAP. Tracers proven working, the flash proven absent from the
picture, and its absence now being proven at the mechanism rather than in a text search.**

## R3.8 S1 this pass (Opus 5, 2026-09-14) — the pre-3D screen hosts 22 controls and draws 12: **10 are lost to FILTERS, not to an empty list**

The backlog entry sets the fork: *"zero rows means the list was never populated; rows present with a
drawn rect means it is populated and mis-drawn. Those are different fixes."* Ran the campaign recipe
to the pre-3D screen with `BOB_TRACE_SYSBOX` and `BOB_TRACE_SKIP`, and the answer is **neither**.

    [skip] dlgId=1164 hosted-for-this-dialog=22 DREW=12 | not-visible=6 not-in-template=4 ...
    [skip] dlgId=1032 hosted-for-this-dialog=184 DREW=156 | dead-sweep-row=11 not-in-template=17 ...

⭐ **The pre-3D dialog (1164) hosts 22 controls and paints 12.** Six are skipped because `visible` is
false and **four because the dialog TEMPLATE has no entry for them**, so the port has no rect to draw
them in. The list is populated; a filter is eating it.

**What the drawn twelve are** (`BOB_TRACE_SYSBOX`, DLU rects):

    1481  (5,42,449,86)      a 449x86 pane -- the list's own frame
    2146  (67,15,80,17)      a title
    2200  (49,134,96,16)     2201 (5,153) 2202 (105,152)
    2203  (370,138,96,16)    2204 (379,146)  2205 (389,155)  2206 (398,164)
    2207  (407,172,96,16)    2208 (417,181)  2209 (426,189)

⭐ **2203–2209 are seven 96×16 rows stepping (+9,+8) each** — a diagonal cascade, which is what a
squadron drawn in echelon looks like, and 96×16 DLU is a text row. **This is the aircraft list, and
it is being hosted with sensible geometry.**

⚠️ **The 17 + 4 "not-in-template" losses are the thread**, and they are not unique to this screen —
the campaign map (1032) loses 17 the same way. A control the game creates but the dialog template
does not describe gets no rect and is dropped silently. **That is a systematic 21-control loss across
two screens in one run.**

*(Recorded because it has bitten before: an earlier attempt at this item reported "hosts zero
controls" — an artefact of a 64-entry dedup table that filled and went quiet. It hosts 22.)*

**S2:** the skip counters COUNT but do not NAME. Print the `ctrlId` of every skipped control with its
reason, then compare the four "not-in-template" ids on 1164 against the aircraft rows. If the four
are rows 2210+, the list is being truncated by a missing template entry and the fix is in the
template parse; if they are something else, the visible-flag six are the next suspects.

**R3.8: 1 sprint this pass. The item's central question is answered with a number.**

## R3.8 S2 (Opus 5, 2026-09-14) — the aircraft list is ctrls **2203–2214: seven drawn, five marked NOT VISIBLE**; and the ids were already in last sprint's log

S1 ended with *"the skip counters COUNT but do not NAME — S2 should print the ctrlId of every skipped
control"*. ⚠️ **They already do.** `bob_skip_name()` emits a `[skipid]` line per (dialog, control,
reason) under the same `BOB_TRACE_SKIP` that produced S1's totals, and those lines were sitting in
S1's own log. *(Sixth time this session that the next question was already answered by output
already on disk — the standing rule is to read every tag in the log before proposing an instrument.)*

**The pre-3D dialog's ten losses, named:**

    [skipid] dlgId=1164 ctrl=1479 not-in-template      [skipid] dlgId=1164 ctrl=2145 not-visible
    [skipid] dlgId=1164 ctrl=1923 not-in-template      [skipid] dlgId=1164 ctrl=2210 not-visible
    [skipid] dlgId=1164 ctrl=2061 not-in-template      [skipid] dlgId=1164 ctrl=2211 not-visible
    [skipid] dlgId=1164 ctrl=2123 not-in-template      [skipid] dlgId=1164 ctrl=2212 not-visible
                                                       [skipid] dlgId=1164 ctrl=2213 not-visible
                                                       [skipid] dlgId=1164 ctrl=2214 not-visible

⭐⭐ **Line them up with the drawn ones and the list appears.** S1 measured the drawn rows as
**2203–2209** — seven 96×16 DLU rows. The not-visible ones are **2210–2214**. **So the aircraft list
is controls 2203…2214 — twelve rows — of which seven are drawn and five are switched off by the
`visible` flag.** The list is not absent and not unpopulated: **it is half hidden.**

**The campaign map's losses are periodic**, which is worth recording next to it: on dlgId 1032 the
skipped ids run `2220, 2230, 2240 … 2320` (not-in-template) interleaved with
`2227, 2247, 2267 …` (dead-sweep-row) — **one id per 10, i.e. a repeating per-row group**, so those
17+11 losses are one structure failing twelve times, not 28 unrelated controls.

⚠️ **And the PO sees NO list, while seven rows do draw.** That is the next fork: either the seven
draw with **empty captions** (a populated-but-blank list looks exactly like a missing one), or they
draw text that lands somewhere invisible.

**S3:** print each drawn row's caption at the draw site — one line per control, id and string. An
empty string on 2203–2209 means the rows exist and the model behind them is empty, which is a
different fix again from the `visible` flag on 2210–2214.

**R3.8: 2 sprints this pass.**

## R3.8 S3 (Opus 5, 2026-09-14) — ⭐⭐⭐ **THE AIRCRAFT LIST IS THERE**: ten callsigns, real text, drawn at real rects

S2 left the fork: the seven-to-ten drawn rows either carry **empty captions** (a blank list looks
exactly like a missing one) or draw text somewhere invisible. S3 read the captions at the draw site —
`HostREdtBt::draw()` assigns `captiontext = InternalGetText()` immediately before the game's own
`OnDraw`, so the string is right there.

**MEASURED (`BOB_TRACE_DRAWID=1`, one line per control):**

    [caption] dlgId=1164 ctrl=2200 REdtBt text="Bob" (len=3)
    [caption] dlgId=1164 ctrl=2201 REdtBt text=" Tomato 2" (len=9)
    [caption] dlgId=1164 ctrl=2202 REdtBt text=" Tomato 3" (len=9)
    [caption] dlgId=1164 ctrl=2203 REdtBt text=" Tomato 4" (len=9)
    [caption] dlgId=1164 ctrl=2204 REdtBt text=" Tomato 5" (len=9)
    [caption] dlgId=1164 ctrl=2205 REdtBt text=" Cabbage Leader" (len=15)
    [caption] dlgId=1164 ctrl=2206 REdtBt text=" Cabbage 2" (len=10)
    [caption] dlgId=1164 ctrl=2207 REdtBt text=" Cabbage 3" (len=10)
    [caption] dlgId=1164 ctrl=2208 REdtBt text=" Cabbage 4" (len=10)
    [caption] dlgId=1164 ctrl=2209 REdtBt text=" Cabbage 5" (len=10)

⭐⭐ **Ten aircraft, two sections — Tomato and Cabbage — with the player as "Bob" at the top.** They
are `HostREdtBt` controls (the name buttons you click to take an aircraft or give it to a pilot),
each **144×26 px**, drawn at real screen rects inside the window: (93,237), (27,268), (177,267),
(575,244) … (659,327).

⭐ **So R3.8's premise does not hold on this build.** *"Before entering 3D the screen should list the
aircraft you can fly, or join as gunner; it shows only the background art and the Back / Sim Config /
Fly menu row."* — **the list is present, populated and painted.** Something between the PO's
2026-08-28 screenshot and today has fixed it; the item has been carrying a diagnosis
("`bob_ole_count_hosted()` does not exist", "zero rows means never populated") for a symptom that no
longer reproduces.

⚠️ **Not closed by me.** The PO's screen may differ — a different campaign, a squadron with no
aircraft assigned, or a resolution this recipe does not use. And *fixed in dev is not fixed in the
AppImage they run*. **S4 is a question, not a run: show the PO this capture and ask whether their
pre-flight screen still looks bare.**

*(Third item today whose PO-reported symptom fails to reproduce on the current build — TERRAIN-1 in
both its scenarios, PO-37 at the gold's own resolution, and now R3.8. Worth a single batched question
to the PO rather than three separate ones.)*

**R3.8: 3 sprints this pass, from "zero rows or mis-drawn?" to ten callsigns on screen.**

## R3.2 S1 this pass (Opus 5, 2026-09-14) — the cloud/cockpit draw order is CORRECT in a real-GL frame; and the same frame is R3.9's first capture from our own build

R3.2 is a 13-point spike — *"fluffy-cloud billboards paint over the cockpit"* — with two failed
attempts behind it. Before spending the spike, the cheapest thing is to ask whether the premise still
holds. Captured a real-GL cockpit frame (`BOB_BOOT_FRONTEND=1`, frame 1400, 1920×1080,
`parity/r32_s1_cockpit_clouds_260914.png`).

⭐ **In this frame the order is right.** The canopy pillars, the coaming, the gunsight and its
reflector glass all occlude the cloud deck behind them; no cloud paints over any cockpit surface.

⚠️ **Scope, and it is a real limit.** The aircraft is **parked** — `Speed 0 Kts, Alt 4 ft`;
`BOB_AUTOFLY=throttle` does not get this airframe rolling — so this is a ground-level view of clouds
*above* the aeroplane, not a cockpit *inside or beside* a cloud. **The defect may need the billboard
close to or straddling the near plane, which this frame cannot produce.** R3.2 is not cleared by it;
what is established is that the ordinary case is ordered correctly.

⭐⭐ **And the same frame answers a different item for free.** There is a flat grey ellipse at the
top left, and R3.9 S6 measured the threat indicator's quad at exactly **(30,17)–(330,317)** at this
resolution. **The ellipse is inside it.** Measured against the sky in the same rows:

| region | mean rgb | R−B | stdev |
|---|---|---|---|
| the ellipse (inside the threat quad) | **[161,162,160]** | **+1** | 27.0 |
| sky, same rows | [189,195,203] | −14 | 7.1 |
| sky, right of frame | [142,179,223] | **−81** | 33.5 |

**A neutrally grey patch (R ≈ G ≈ B) in a blue sky, inside the threat indicator's rect.** That is the
PO's *"floating light/dark grey square"*, captured from our own build for the first time, and it is
exactly what R3.9 S8 predicted a **two-tone stencil drawn without its alpha plane** would look like.
*(R3.9 is at its 4-sprint cap this pass; recorded here so the evidence is not lost, and its S9 can
open with a picture instead of a hypothesis.)*

**S2 for R3.2:** get the aeroplane INTO the weather before spending the spike — the cloud deck sits
above a parked aircraft, so the test needs altitude. The BoB campaign path reaches 3D in flight
(`bob_combat_soak.sh`'s recipe) but runs under `SDL_VIDEODRIVER=dummy`, where `draw_fvf` never
executes; the real-GL path starts on the runway. **Bridging those two is the prerequisite for R3.2,
and it is the same gap R3.7 S5 hit.**

**R3.2: 1 sprint this pass — premise partly checked, and the spike deliberately not started.**

## R3.2 S2 (Opus 5, 2026-09-14) — ⭐⭐ the aeroplane ROLLS: the wheel brakes were never released, and that unblocks R3.2, R3.7 and R3.9 on real GL

S1 could only test clouds ABOVE a parked aeroplane, because every real-GL capture this session reads
`Speed 0 Kts, Alt 4 ft` after a minute at full throttle. S2 asks why, and the answer is one key.

⭐ **BoB maps the wheel brakes to comma and stop** (`KEYMAPS.H:1185-6`,
`LEFTWHEELBRAKE`/`RIGHTWHEELBRAKE` = DIK 0x33/0x34), and `BOB_AUTOFLY=throttle` **never touched
them** — it taps full throttle and nothing else, so the aircraft sits at full power with the brakes
on. Released once at `cnt=60` (`BOB_NO_BRAKE_TAP=1` reverts):

| | HUD readout at frame 2000 |
|---|---|
| before | `Alt 4ft  Speed 0Kts  Power 70` |
| **after** | `Alt 5ft  Hdg 269  **Speed 80Kts**  Power 70` |

⭐⭐ **80 knots on the takeoff roll, from a standstill.** *(Released ONCE on purpose: they toggle, so
a second tap puts them straight back on — the exact trap MA's own autofly records, where tapping at
two checkpoints "released the brakes and put them back, which looks like the brakes never
released".)*

⭐ **This is a harness fix with three items behind it.** R3.2 needs the cockpit *in* the weather, not
under it; R3.7's muzzle-flash capture was taken on a parked aircraft; R3.9's grey square is reported
*"during a campaign dogfight"* and the only real-GL path available started on the runway and stayed
there. **All three were blocked on the same key.**

⚠️ **Not airborne yet** — `Alt 5 ft` at frame 2000, and the heading has drifted to 269, so the roll
needs more runway and probably some elevator. **S3: hold the throttle longer and add a rotate input,
then take R3.2's real measurement — a cockpit frame at altitude, in or beside a cloud.**

**R3.2: 2 sprints this pass. The spike is still unstarted, and the thing that was stopping it from
even being tested is fixed.**

## R3.2 S3 (Opus 5, 2026-09-15) — ⭐⭐ AIRBORNE on real GL at last: the campaign recipe minus `SDL_VIDEODRIVER=dummy`. And in that frame the clouds still do not paint over the cockpit

S2 got the aeroplane rolling but not flying — no keyboard elevator exists (`ELEVATOR_BACK/FORWARD`
are dead-coded in `KEYMAPS.H:70-71`; pitch is an analogue axis), so the runway start cannot reach
altitude from the harness. S3 took the other route.

⭐⭐ **The campaign recipe already reaches a flight IN THE AIR — it was only ever run headless.**
`bob_combat_soak.sh` sets `SDL_VIDEODRIVER=dummy`, where `draw_fvf` never executes and no capture is
possible. **Dropping that one variable and running the same clicks on `DISPLAY=:0` gives:**

    Alt 4854ft   Hdg 256   Speed 190Kts   Power 70   Gun Ammo. 1000

**A cockpit at 4,854 ft and 190 knots, on real GL, with a dumped frame**
(`parity/r32_s3_airborne_realgl_260915.png`). **That is the bridge R3.2, R3.7 and R3.9 have all been
blocked on** — the campaign path for the flight, the real-GL path for the pixels, in one run.

**MEASURED in that frame — bright, near-neutral (cloud-coloured) pixels by band:**

| band | count | share |
|---|---|---|
| above the horizon (where the clouds are) | 4,552 | 0.2% |
| the cockpit structure (lower 28%) | 1,470 | 0.1% |
| gunsight glass, mean rgb | **[52,62,61]** | dark |

⭐ **No evidence of clouds painting over the cockpit here either.** The 1,470 bright pixels in the
cockpit band are consistent with the gunsight's own chrome barrel and reflector rim, and the glass
itself reads dark, not cloud-filled. **Two frames now — parked and airborne — with the order correct.**

⚠️ **Still not a clearance.** This aircraft is at 4,854 ft over the sea with the cloud deck above it;
the PO's defect plausibly needs the aeroplane **inside or level with** a cloud, which neither frame
has. **What changed is that it is now reachable:** the campaign flight runs for minutes on real GL,
so a later frame — or a climb — can put the cockpit in the deck.

**S4:** dump several frames across the campaign flight rather than one, and score each for
cloud-coloured pixels inside the cockpit silhouette. The first frame where that count rises above the
chrome baseline is R3.2's repro — and if none does across a whole sortie, the item should be put to
the PO as "not reproducible on this build" alongside the other three.

**R3.2: 3 sprints this pass. The spike is still unstarted; its blocker is gone.**


## R3.2 S4 (Opus 5, 2026-09-15) — ⭐⭐ the negative result is EXPLAINED, and it is geometric: **this flight is 2,700 ft below the lowest cloud in the game**

S3 scored one airborne frame and found no cloud over the cockpit. S4 does it across a whole sortie —
and then asks the question three sprints of negatives should have asked first: **could this recipe
ever have put the cockpit in a cloud?**

**The recipe is a script now.** `tools/bob_realgl_flight.sh` is S3's reconstruction written down: the
campaign drive from `bob_combat_soak.sh` minus `SDL_VIDEODRIVER=dummy`, on `DISPLAY=:0`, dumping
every Nth 3D frame. Three items (R3.2, R3.7, R3.9) depend on this path and it existed only as prose.

**And the run can now SAY it got airborne.** New `BOB_TRACE_HUD` prints the numbers
`COverlay::DrawTopText` is about to draw. S3 had to read `Alt 4854ft` off a dumped frame by eye,
which no script can do. This run: **295 samples, 4,386-5,332 ft, 152-182 kt, 59 frames.**

**SCORED** (`tools/bob_cloud_score.py`): cloud-coloured (bright, near-neutral) pixels inside a
cockpit-structure mask built from the sequence itself.

| | |
|---|---|
| frames | 59 |
| cockpit mask | 659,886 px (31.8% of frame) |
| cloud px in the mask, 50 of 59 frames | **0** |
| the other 9 (all in the first 66 s) | 6 - 726, decaying monotonically as the light changes |

⚠️ **The mask rule had to be written so the defect could not erase itself.** "Dark in frame 0 AND
unchanged in every frame" would drop exactly the pixels a passing cloud lit up. The rule is *dark in
70% of frames*, which survives a cloud crossing. (Same family as the julia-racer census that counted
the list built before the rule it was measuring.)

⭐⭐ **WHY EVERY ONE OF THESE RUNS WAS ALWAYS GOING TO SAY NO.** The clouds are not where the
aeroplane is:

| | altitude | source |
|---|---|---|
| fluffy cloud sprites | **8,000 ft + noise** (0-8,566 ft more) | `LANDSCAP.CPP:9276` - `dummyitem.World.Y = FT_8000 + newY`, `newY = Noise(...)<<10`, `Noise` returns a `UByte` |
| stratus layer | **15,000 ft** | `MISSINIT.CPP:1506` `CloudLayer = 457200`; `SKY.H:72` - *"bottom layer in cm"* |
| **this flight, best case** | **5,332 ft** | `[hud]` |

**The campaign flight cruises 2,668 ft below the lowest cloud the game can place.** S1's parked
frame, S3's single airborne frame and S4's 59 are all measurements taken *under the weather*. None of
them is evidence about the PO's report, and the item must stop collecting them: **R3.2 needs a flight
at >= 8,000 ft**, which this recipe does not fly (the campaign sortie is a low-level convoy strike).

⭐ **UNEXPECTED, AND MEASURED: a grey ellipse pinned to the screen for the entire flight.** In every
frame there is a flat grey ellipse at **x 49-312, y 141-189** - *the same bounding box in frames
7,200, 20,400 and 35,400*, i.e. across 28,000 frames and ~8 minutes of flight at 180 kt. Scenery
cannot be static in screen space. **This is a candidate for R3.9 / PO-73 ("a grey box near the
cockpit")**, which has never had a reproduction; capture at
`parity/r32_s4_clouds_above_cockpit_260915.png`. Not chased here - R3.2 is at its cap - but it is
the first time that shape has been caught in a frame this project can measure.

**S5 (R3.9's, not R3.2's):** identify what draws the grey ellipse. It is static, ~263x48 px, upper
left, and survives a whole sortie - start from the draw order in that band rather than from the
report's word "drifting".

**R3.2: 4 sprints this pass - AT THE CAP. The item is not disproved; it has never been tested, and
now the reason is a number.**

## R3.9 S1 this pass (Opus 5, 2026-09-15) — the grey ellipse is MEASURED and FOUR candidates are eliminated, including the one this repo had written into the source as its hypothesis

R3.2 S4 caught a flat grey ellipse pinned to the same screen rows for a whole sortie. This sprint
A/Bs it. The real-GL flight is frame-deterministic (same recipe -> the same frame numbers and the
same pixel counts to within 0.4%), which is what makes single-arm comparisons trustworthy here.

**WHAT IT IS, measured** (frame 15,000 of a 260 s campaign flight, 1920x1080):

| property | value |
|---|---|
| screen rows | **y 141-189**, identical in every arm and every frame |
| horizontal extent | grows over the sortie: x 49-122 at f.6000, 49-273 at f.9000, **49-313** from f.12000 on |
| colour | **flat and uniform**: (146,146,146) under the cloudy sky, (103,123,143) with the sprite clouds suppressed |
| shading | none - no gradient, no texture, one colour across the whole shape |

⭐ **It is LIT by the scene** — the colour tracks the sky's illumination between arms — so it is not a
2-D blit pasted after the 3-D pass; it goes through the lighting path like world geometry.

**FOUR ARMS, four eliminations** (each a new test switch, default off):

| arm | switch | ellipse |
|---|---|---|
| aircraft shadows off | `BOB_NO_ACSHADOW` (already in the tree, never run) | **unchanged** (7,120 px vs 7,094) |
| sun object off | `BOB_NO_SUN` (new) | **unchanged** (7,091 px) |
| sprite cloud field off | `BOB_NO_FLUFFY` (new) | **still there** — the white puffs vanish, the ellipse does not |
| distant layer clouds off | `BOB_NO_STRATUS` (new) | **unchanged** |

⭐⭐ **The aircraft-shadow hypothesis is REFUTED, and it was the one written into the source.**
`Add_Shadow` carries a comment from 2026-09-03 reasoning that "an aircraft shadow IS an ellipse" and
that an unresolved ground height would leave it floating in the sky; `BOB_NO_ACSHADOW` was added then
to test it and **never could be run**, because no harness reached a real-GL cockpit in flight until
R3.2 S3. The switch has now been used for the purpose it was written for, and the answer is no.

⚠️ **AND ONE OF MY OWN MEASUREMENTS WAS WRONG FOR A SPRINT-TYPICAL REASON.** The first pass scored
the `BOB_NO_FLUFFY` arm as **0 grey pixels** — "the clouds off, the ellipse gone, case closed". The
detector was a fixed colour window (`100<r<170`, near-neutral). With the cloud field suppressed the
whole scene relights and the ellipse becomes (103,123,143) — `|r-b|` = 40, outside the window. The
object never moved; **my instrument stopped being able to see it**, and it said so as a zero. Caught
by looking at the picture. The replacement is relative: sample the ellipse row against the sky 55 px
above it in the SAME frame, which cannot be fooled by a global colour shift.

**S2:** the remaining candidates are the overlay/HUD layer and the horizon band. Test the overlay
first — it is one switch — and if the ellipse survives that too, bisect by draw order rather than by
guessing objects: dump the framebuffer at successive points in the frame and find the first dump that
contains it.

**R3.9: 1 sprint this pass.**

## R3.9 S2 (Opus 5, 2026-09-15) — a fifth elimination (not the cockpit), and **S1's "it grows over the sortie" is WITHDRAWN**: the ellipse is the same size in every frame

⚠️ **CORRECTION FIRST.** S1 reported the ellipse growing — x 49-122 at f.6000, 49-273 at f.9000,
49-313 from f.12000. **It does not grow.** Cropping the frames shows it at full size in the EARLIEST
capture (f.3000) and unchanged thereafter. The "growth" was the fixed colour window (`100<r<170`,
near-neutral) clipping a lit gradient differently as the scene's light changed — **the same
instrument that produced S1's false zero on the `BOB_NO_FLUFFY` arm, now producing a false TREND.**
One bad detector, two wrong facts, in the same item. A colour window is not a shape detector.

**Corrected description: a flat grey ellipse, ~265 x 48 px, at x 49-313 / y 141-189, present in every
frame of every arm from the first capture onward.**

**FIFTH ARM — the external view (`BOB_AUTOFLY=view40`, F6):** with the cockpit not drawn at all, the
ellipse is **in the same screen position**, and against the clean sky there it reads as a grey DISC
with a small dark mark at its centre. So it is not cockpit geometry, and the list of things it is not
now reads: aircraft shadow, sun, sprite clouds, layer clouds, cockpit.

**S3 — stop guessing objects.** Five arms have each cost a 260 s flight to eliminate one candidate,
and the remaining candidates are not enumerable by reading source. The next instrument is a DRAW-LIST
probe: report every shape the frame draws with its projected screen bbox, then look for the one whose
bbox is (49,141)-(313,189). That names the object in one flight instead of one per guess.

**R3.9: 2 sprints this pass.**

## R3.9 S3 this pass (Opus 5, 2026-09-15) — ⚠️ **I re-litigated a solved item**, and the one new thing is a picture of the sprite: a tan ellipse on green chroma

⚠️ **ORIENTATION FIRST, because this is the expensive mistake of the session.** R3.2 S4 described the
ellipse as *"a candidate for R3.9 / PO-73, which has never had a reproduction"*. **That was wrong.**
R3.9 **S5-S8 (2026-09-14)** had already named the sprite (`MskMap16\THREAT01.X8`, image map 525),
found its single draw site (`COverlay::DoThreat`), measured the quad it occupies
(`(30.0,16.9)-(330.0,316.9)` at 1920×1080 — the same bbox this sprint's pixel probe reports), counted
its 33 blips, and measured the art as a two-tone stencil with an alpha plane. **S1 and S2 of this pass
spent five 260-second flights eliminating shadows, the sun, sprite clouds, layer clouds and the
cockpit — a question that was closed the day before.**

The tell was in the tree and I walked past it twice: `BOB_NO_ACSHADOW` carried an R3.9 comment dated
2026-09-03, and `BOB_PIXPROBE` carried an R3.9 comment saying *"four sprints of R3.9 reasoned from the
shape of a grey ellipse and got the wrong answer twice"*. **Before designing an experiment, grep the
source for the ITEM'S OWN NAME** — this repo writes its sprint history into the code.

**WHAT IS ACTUALLY NEW — S8 asked for the uploaded texels; here they are.** `BOB_PIXPROBE` dumps the
bound texture, and rendered it is unambiguous:

* **a flat tan ellipse on a solid green field**, 128×128 — exactly the "distinct=2" stencil S8
  measured at load time, now visible as a picture rather than as a histogram.

⭐ **And that settles S7/S8's fork.** S8's branch *"if the key is not honoured the whole texture paints
as an opaque 300×300 panel"* is **not** what happens: on screen the green field is gone and the sky
shows through it, so the transparency IS honoured. **What paints is the stencil's second tone** — the
ellipse itself. The PO's grey shape is not a missing key; it is the scope's backing shape rendering
with none of the detail the alpha plane is supposed to add.

**LEADING CANDIDATE for that, stated as a candidate:** the uploaded surface has **no alpha channel**.
Every 128×128 upload in the run reports either `A=0x0000` (RGB565) or `A=0xf000` (ARGB4444), and the
dumped THREAT01 texels decode cleanly as 565 — which a 4444 buffer could not. The loader says
`alpha=yes` for this map. ⚠️ **Not proven**, because the join failed: see below.

⚠️ **Two instrument faults found and fixed on the way.**
* `[texfmt]` printed no surface pointer, so it could not be joined to `[pixprobe]`'s `tex=`. Added.
* `[texfmt]` was capped at **24 lines** and went quiet long before this sprite's upload — the
  full-table-goes-silent failure this project has hit three times. The cap is now
  `BOB_TRACE_TEXFMT=<n>`.
* **And even uncapped (463 lines) no `[texfmt]` line matches the pointer the draw binds.** So the
  surface the draw uses is not the surface that path uploads, and *that* is the next thread —
  `g_devTex[0]` at draw time vs `s` at upload time may be different objects around the same GL
  texture.

**S4:** join them. Print the GL texture NAME (`s->glTex`) in both traces instead of the surface
pointer — a GL name is the same integer on both sides of the hop — and then read THREAT01's alpha
mask directly. If it is `A=0x0000`, the fix is at the surface build, not in `DoThreat`.

**R3.9: 3 sprints this pass.**

## R3.9 S4 this pass (Opus 5, 2026-09-15) — the join works, and it immediately killed a false lead: `glTex=0` at probe time is the PROBE's position, not a missing upload

S3 could not join `[pixprobe]` to `[texfmt]`: the surface pointer the draw binds never appeared in the
upload trace. Both traces now print the **GL texture name**, which is the same integer on both sides
of the CPU/GPU hop.

**What the join says:** the 128×128 surface covering (180,165) reports **`glTex=0`**, while other
draws in the same probe report real names (2, 12, 20, 21, 24, …). Read alone, that is "the sprite was
never uploaded and binds texture 0", which would have been a clean root cause.

⛔ **It is wrong, and the control that says so ran in the SAME flight.** `BOB_TRACE_GREY`'s condition
is exactly `(!t || !t->glTex)` — the canary for this very fault — and it fires **twice in the whole
run**, on an unrelated red 2-D quad, never on this surface. Two readings of the same field inside the
same function disagree, so the field changes between them: `[pixprobe]` sits **before** the bind and
`[grey]` **after**, and the GL texture is created at bind time. **`glTex=0` in the pixel probe is a
statement about where the probe stands, not about the texture.**

⭐ **So the sprite IS textured when it paints**, and S3's "the uploaded surface has no alpha channel"
remains the live candidate rather than being replaced by a missing upload.

**METHOD:** this is the third instrument fault in three sprints on this item (a colour window that
produced a false zero, then a false trend; a table capped at 24 that went quiet; now a field read
before it is set). None of them was wrong about the game — all three were wrong about the
instrument. **When a new probe produces a clean, satisfying answer, look for a second instrument that
already measures the same thing and check they agree before writing it down.**

**S5:** read THREAT01's alpha mask where the surface is BUILT (`LIB3D.CPP:6686/6865`,
`body + palette + alpha`) rather than at upload, and print `dwRGBAlphaBitMask` for that specific map
by joining on the GL name the canary sees.

**R3.9: 4 sprints this pass — AT THE CAP.**

## R3.7 S1 this pass (Opus 5, 2026-09-15) — ⛔⛔ **S7 AND S8 ARE BOTH OVERTURNED: the muzzle-flash bit HAS a consumer, it is reached, and its branch is TAKEN**

S7 concluded *"the muzzle-flash bit is computed every frame and read nowhere"* from a name search.
S8 corrected the METHOD (the interpreter reads anim data by byte offset, so a grep cannot see it),
measured **one** read site, found only offset 191 addressed, and stated its own scope honestly:
*"`3DCOM.CPP:2311` reads `animoffsrc` and there may be others… what is established is that no shape
addressed 141 **through the `animoffset` path**"*. S9 does the rest of that list.

**Every site that indexes `GlobalAdptr` is now instrumented** — `flagname`, `animoffsrc`,
`birthtimeoffset`, `animoff` (twice) and `dic_ptr->flag` — each keyed by (site, offset) so a busy
site cannot drown a rare one. One firing sortie, real GL:

    [animoff] site=animoff        reads anim byte offset 141      <-- THE FLASH BYTE
    [animoff] site=dic_flag       reads anim byte offset 114, 119, 120, 122, 215, 216, 225, …
    [animoff] shape reads anim byte offset 172, 191               (S8's site, unchanged)

⭐⭐ **Offset 141 is read — by `shape::doswitch`** (`3DCOM.CPP:3806`), the interpreter's conditional:
it takes a byte, extracts a bit, and skips the next instruction when the bit does not match. So the
shape data DOES branch on the muzzle flash; S7's "no consumer" and S8's narrower version are both
wrong, and the reason is the one S8 itself named — there was more than one read site.

**And the branch fires.** New `BOB_TRACE_FLASHBIT` prints the switch's own parameters and the byte:

    [flashbit] doswitch on byte 141: nobits=1 bitoffset=2 expects value=1
    [flashbit] doswitch on byte 141: nobits=1 bitoffset=5 expects value=0
    [flashbit] byte141=0x04 -> bit 2 IS SET (expected 1) -> branch TAKEN     (x8)

`ANIMDATA.H:192-199` gives byte 141 as `lighttoggle:1, hassmoked:1, **muzzleflash:1**, rearshooting:1,
enginestart:1, ejected:1, cannonflash:1, pad:1` — so **bit 2 is `muzzleflash`** and bit 5 is
`ejected`. `byte141 = 0x04` is exactly muzzleflash set and nothing else.

⭐ **So the chain is intact up to the draw:** the flag is set, a shape tests it, the test passes, the
branch is taken. **The flash's absence from the picture (S6, measured against a control) is therefore
DOWNSTREAM of the branch** — in whatever the taken branch draws: its geometry, its image map, its
colour or its scale.

⚠️ **One thing NOT claimed.** The eight `byte141=0x04` lines are only printed when the bit is SET (the
probe's condition), so they do not show whether the flag ever CLEARS. "The flash flag is stuck on" is
a different statement and this run cannot make it.

**S2:** follow the taken branch. `doswitch` leaves `instr_ptr` pointing at the next shape
instruction — log what that instruction IS (opcode, image map, colour) for the 141/bit-2 switch, and
compare it with a branch that visibly renders. The question has moved from "is anything listening?"
to "what does the listener draw?", which is a much smaller search.

**R3.7: 1 sprint this pass.**

## R3.7 S2 this pass (Opus 5, 2026-09-15) — ⭐ the flash switch guards **`dosetmaterialtype`**, and that call ends in a lighting flag that can be silently CLAMPED

S1 established the chain up to the branch: the muzzle-flash bit is set, a `doswitch` tests it
(byte 141, bit 2, expects 1) and the branch is taken. S2 asks what the taken branch leads to.

**The interpreter's loop reads `*instr_ptr` as the next opcode**, so the probe prints it for the
141/bit-2 switch only:

    [flashbit] after the bit-2 switch the next opcode is 146
    [flashbit] after the bit-2 switch the next opcode is 6        (the not-taken path)

**Opcode 146 is `dosetmaterialtypeno`** (`SHPINST` in `SHPINSTR.H`, counted through the enum;
the probe's own printed constants — `dopointno=1 dopolygonno=2 doretno=6 doswitchno=32` — confirm the
numbering). So **the muzzle flash is a MATERIAL change, not a piece of geometry** — which is why four
sprints of looking for a missing mesh found nothing.

**And `dosetmaterialtype` (3DCOM.CPP:12341) does exactly one thing:**

    LIGHTFLAG lf = LIGHTFLAG(GetLightingType(ptr->flags));
    g_lpLib3d->SetObjectLighting(lf);

⭐ **`Lib3D::SetObjectLighting` (LIB3D.CPP:6287) opens with a CLAMP:**

    if ( lf > masterLightFlag )
        lf = masterLightFlag;

**A shape can therefore ask for a brighter lighting mode than the current master allows and be
silently downgraded** — no error, no trace, the polygons simply draw in whatever mode survives the
clamp. For an effect whose whole appearance IS its lighting mode, that is a candidate root cause of
exactly the right shape: the flash's geometry draws, in the wrong material.

⚠️ **Candidate, not conclusion.** Nothing yet shows what `GetLightingType(ptr->flags)` asks for at
this call site, what `masterLightFlag` is at that moment, or whether the clamp actually bites. Three
values, one print.

**S3:** print `requested / masterLightFlag / applied` inside `dosetmaterialtype` when the caller is
the flash switch. If requested > master, the clamp is the defect and the fix is to raise the master
for the effect (or to stop routing an emissive effect through a mode the master can veto). If they
agree, the flash's material is being set correctly and the search moves to what the following
polygons draw with it.

**R3.7: 2 sprints this pass.**

## R3.7 S3 this pass (Opus 5, 2026-09-15) — ⛔ the CLAMP is innocent, and the flash turns out to be an **alpha-blended white polygon**

S2 named `SetObjectLighting`'s `if (lf > masterLightFlag) lf = masterLightFlag` as a candidate: a
silent downgrade would leave the flash's polygons in the wrong material. S3 measured it, and it is
not the defect.

**Two prints, one flight** (`BOB_TRACE_FLASHBIT` + new `BOB_TRACE_LIGHTCLAMP`):

    [flashbit] guarded dosetmaterialtype: flags=0x4 -> lighting type 1      (x6)
    (no [lightclamp] lines at all)

⛔ **The clamp never fired.** `masterLightFlag` is initialised to `LF_SPECULAR` (7) in
`SetDriverAndMode`, the flash asks for **1**, and 1 < 7 — so the request is applied unchanged. S2's
candidate is dead; killed in one run rather than argued about.

⭐ **And lighting type 1 is `LF_LIGHTSOURCE`** (`LIB3D.H:310` — `LF_ALPHA, LF_LIGHTSOURCE, LF_VERTEX,
LF_FONT, LF_DEPTH, LF_AMBIENT, LF_LIGHTING, LF_SPECULAR`). That is exactly the right request for a
muzzle flash: *this object is a light source, do not shade it*.

⭐⭐ **What the port then does with it is the new thread.** `LF_LIGHTSOURCE` is implemented, in two
places, and both say the same thing:

    case LF_LIGHTSOURCE:                          // LIB3D.CPP:10652
        sVertex->color = 0xAAFFFFFF;              // white, alpha 0xAA...
        SetColAlpha( sVertex->color, globAlpha ); // ...immediately replaced by globAlpha
        sVertex->specular = 0x00000000;

    if (lightflag == LF_LIGHTSOURCE)              // LIB3D.CPP:12014
        sVertex->color = 0xAAFFFFFF;
    …
    sVertex->color.alpha = globAlpha;

**So the muzzle flash is a WHITE, ALPHA-BLENDED polygon whose visibility is `globAlpha` and the blend
state.** Not a mesh, not a texture, not a lighting mode that got downgraded — an alpha value.

**S4, and it is two numbers:** print `globAlpha` at the moment the flash's polys are built, and check
what the GL shim has bound for blending on that draw. `globAlpha = 0` would make the flash
mathematically invisible while every trace upstream — flag, switch, branch, material — reads correct,
which is exactly the shape of this item's whole history.

**R3.7: 3 sprints this pass.**

## R3.7 S4 this pass (Opus 5, 2026-09-15) — the alpha theory dies too: `globAlpha = 255`, and LF_LIGHTSOURCE geometry is drawn in QUANTITY

S3 reduced the flash to "a white, alpha-blended polygon whose visibility is `globAlpha` and the blend
state". S4 measures both, and neither is the fault.

**New `BOB_TRACE_LSPOLY` prints every vertex coloured by the `LF_LIGHTSOURCE` path** — the one the
flash's material selects:

    [lspoly] LF_LIGHTSOURCE vertex 1 at (  6.6,  11.8, 72.7501) globAlpha=255
    [lspoly] LF_LIGHTSOURCE vertex 2 at (  6.6, -12.8, 72.7507) globAlpha=255
    [lspoly] LF_LIGHTSOURCE vertex 3 at ( -6.6, -12.8, 72.7507) globAlpha=255
    [lspoly] LF_LIGHTSOURCE vertex 4 at ( -6.6,  11.8, 72.7501) globAlpha=255
    [lspoly] LF_LIGHTSOURCE vertex 5 at ( -1.0,   1.0,  0.0000) globAlpha=255
    …
    128 report lines (the trace prints the first 12, then every 5,000th)

⛔ **`globAlpha` is 255 — fully opaque.** The flash's polygons are not transparent. That kills S3's
candidate as cleanly as S3 killed S2's.

⭐ **And LF_LIGHTSOURCE geometry is emitted in bulk** — the every-5,000th counter reaches 128 lines in
one 200 s sortie, i.e. of order half a million vertices. **So this lighting mode is used by far more
than the muzzle flash** (lights, tracers, the sun, HUD elements all plausibly share it), which is why
counting it as a whole cannot answer the question. *The first quad it sees is a clean
(±6.6, −12.8…11.8) rectangle at z ≈ 72.75 — the right shape and place for a flash billboard, but
nothing here proves it IS one.*

**Where that leaves the item after four sprints this pass:** the flag is set, the switch tests it, the
branch is taken, the guarded instruction is `dosetmaterialtype`, it requests `LF_LIGHTSOURCE`, the
request is not clamped, the mode is implemented, the geometry is opaque, and vertices do flow through
that path. **Every link in the chain is now measured and sound** — which means the fault is in
something none of these instruments scopes to: WHICH geometry belongs to the flash, and where it
lands.

**S5 (next pass), and it is ~10 lines:** `g_bob_flash_guard_hot` already marks the guarded
`dosetmaterialtype`. Latch a *"flash material in force"* flag there, clear it at the next material
change or `doret`, and count/locate **only** the LF_LIGHTSOURCE vertices emitted while it is set. That
turns half a million vertices into the handful that are the flash, with their screen positions — one
flight, one number.

**R3.7: 4 sprints this pass — AT THE CAP.**

## R3.6 S4 (Opus 5, 2026-09-15) — ✅ VERIFIED on the current build: the system box's close button draws `ICON_CROSS`, and I re-made the exact misreading the fix was written to prevent

R3.6's three earlier sprints ended with a fix keyed on **(dialog, control)** rather than control id
alone, because *"control ids are only unique WITHIN a dialog, and the SYSTEM BOX (dlgId 823) numbers
its buttons 1001/1002/1003 — the same ids the files toolbar uses"*. S4 checks that on the build that
exists now.

**New `[btnart]` / `[btnall]` traces** print the dialog AND the control for every hosted button —
matched or not, because a trace that speaks only on a match cannot show a control the table misses.

    [sysbox] panel 34x50 DLU -> 51x81 px at (965,8): 3 controls drawn
    [btnall] dlg=823 ctrl=1001        [btnall] dlg=823 ctrl=1002        [btnall] dlg=823 ctrl=1003
    [btnart] dlg=823 ctrl=1003 -> ICON_CROSS = 0x10022

⭐ **The system box is dialog 823, it hosts exactly the three controls the fix expects, and its close
button takes the real X.** 1001 and 1002 take no icon, deliberately — the sprite sheet has no
TOOLBAR_HIDE or SCREENSIZE equivalent and *"guessing a near-match is exactly what produced this
defect"*. **R3.6 is fixed and now verified rather than assumed.**

⚠️ **AND I WALKED STRAIGHT INTO THE ORIGINAL MISREADING.** My first instrument printed
`m_NormalFileNum` at the DRAW, with no dialog id. Two values appeared right after the sysbox paint
line — `0x10004` and `0x10002`, i.e. `ICON_ZOOM` and `ICON_SAVE` — and I read them as the system
box's, which is exactly the PO's reported symptom. They are the **files toolbar's** (dlg 942,
ctrl 1003/1004/1005). **An art number without its dialog is unattributable in this codebase**, and
that is the very sentence the fix's own comment opens with.

**Two process notes, and this is the second time today:**
* **Read the SPRINT NOTES, not the backlog row.** R3.6's row still narrates the investigation
  ("the art-guard suspect is eliminated…"); the fix landed three sprints later. R3.9 cost five
  flights to the same mistake this morning.
* **An instrument that identifies a thing by one half of its key will confirm whatever you already
  believe.** The trace that settled this prints both halves, for every control, matched or not.

**R3.6: verified. The item can be closed on this evidence.**

## R3.4 S5 this pass (Opus 5, 2026-09-15) — ⭐⭐ the gold the item has been "blocked on the PO" for since June was **already in `gold standard/bob/`**, it says the mirror is FIXED, and the same picture exposes a much bigger defect: **the whole 3D view is stretched 1.333x horizontally**

R3.4 S4 (2026-09-03) ended: *"the next question is about the TARGET, not the code ... one gold
capture of the cockpit mirror answers whether a flat sky-grey mirror is correct behaviour"*, and
parked the item at its 4-sprint cap awaiting the PO.

⚠️ **That capture existed the whole time.** `~/gold standard/bob/` holds 19 stills **and
`bob_convoy_campaign.mp4` — 3:05 of the real `bob.exe` running under Wine** (`[wine_runner] Using
runner: lutris-5.7-11-x86_64 ... TUE/BattleOfBritain/battleOfBritain.sh` is legible in the frame),
Luftwaffe Convoys, the *same campaign the port's own flight harness flies*. This is the third time
this pass that "blocked on the PO" turned out to mean "nobody looked in the repo"
([[blocked-on-po-may-be-in-the-repo]]).

**What the gold says the mirror shows.** Sampling the Bf 109 cockpit segment (1 fps, t=60-150 s) at
the mirror disc:

| gold frame | mean | distinct colours | sd | change vs previous frame |
|---|---|---|---|---|
| seq_031 | 200.6 | 1135 | 33.1 | - |
| seq_033 | 158.8 | 1135 | 54.6 | 50.6 |
| seq_034 | 91.0 | 999 | 26.6 | 72.8 |
| seq_047 | 150.3 | 1095 | 42.2 | 65.0 |
| seq_069 | 187.1 | 1045 | 33.8 | 34.4 |

A **live** mirror: sky band, a horizon, tan ground with linear features, and the aircraft's own
structure, all of it moving with the aeroplane. So "a flat grey mirror" was never correct behaviour.

⭐ **And the port now does the same thing.** A 280 s real-GL German Convoys flight with `BOB_MIRROR=1`
(54 dumped 3D frames, HUD 5302 -> 4597 ft, so the run proved it was airborne before anything was
measured):

| port frame | mean | distinct | sd | d(prev) |
|---|---|---|---|---|
| f.3000 | 194.2 | 815 | 35.6 | - |
| f.6000 | 186.1 | 1867 | 37.8 | 11.2 |
| f.9000 | 193.3 | 891 | 34.9 | 10.6 |
| f.12000 | 190.7 | 1256 | 34.8 | 2.9 |
| f.15000 | 189.4 | 1610 | 35.2 | 4.1 |

Same order of structure as the gold (sd ~35 against 33-42; ~1000 distinct colours in both), and it
changes frame to frame. Compare with what R3.4 measured on 2026-09-03: **mean 213.5, range 209-215,
19 distinct colours.** `doc/reference/mirror-gold-vs-port-2026-09-15.png` is the pair, and both
discs show sky / horizon / tan ground / the same white graticule.

**R3.4 is therefore CLOSED, and it was MIRROR-1 S4 that closed it** (the 5:2 projection aspect
against a square FBO, fixed 2026-09-13). This sprint is the cockpit-side confirmation MIRROR-1 S4
said it could not get: *"a cockpit capture was attempted (`BOB_SHOT=520`) and did not land"*. The
filed cause -- `InfiniteStrip` garbage v-texcoords -- was wrong, as S3 had already shown.

⭐ **MIRROR-1's one open question is also answered by the gold.** S4 declined to claim 1.0 was the
*right* aspect rather than merely a working one: *"whether the geometrically right number is 1.0 or
the aspect of the cockpit mirror QUAD"*. **The gold mirror is a circle -- 50 x 50 px, w/h = 1.000, in
three separate frames.** A round mirror wants a square render target. 1.0 is right.

## ⭐⭐ THE FINDING THIS SPRINT DID NOT GO LOOKING FOR: the port's 3D view is 1.333x too wide

Measuring the mirror disc's bounding box to compare the two pictures produced this, and it is stable
in every frame of both runs:

| | disc bbox | w/h |
|---|---|---|
| gold (`bob.exe`/Wine, 800x600 window) | 50 x 50, 50 x 50, 55 x 55 | **1.000** |
| port (`build/bob`, 1920x1080 window) | 192 x 144 in f.6000, f.9000, f.12000 | **1.333** |

**1.3333 is exactly (16/9) / (4/3).** The mirror is cockpit geometry, so it goes through the same
projection as the world: the port computes a 4:3 projection and rasterises it into a 16:9 window.
Every circle in the game -- gunsight reticle, instrument dials, the sun, the mirror -- is 33% too
wide on a widescreen display, and the horizontal field of view is wrong to match. **This is on the
PO's screen every time they fly**, and it is the kind of defect that is invisible until something
known to be round is measured.

Where it comes from, as far as this sprint got without a rebuild:

* `DOSDefs.H`: `FULLW 25600`, `FULLH 19200` -- a hardcoded **4:3** virtual coordinate space.
* `HARDWIN.CPP:253`: `virtualXscale = (FULLW+window_width-1)/window_width` -- **integer** division.
* `WIN3D.CPP:3433`: `aspectRatio = FoV*(window_height/window_width)` where
  `window_width = VirtualWidth/virtualXscale`.
* The flight log shows `SDL2 window 1024x768` then `ChangeDisplaySettings 1024x768 -> 1920x1080`.
  Both of those are the *window*; whether `Set3dWindow` is re-run after the mode change, and what
  the two integer scales are at 1920x1080, decides between "the projection is built 4:3 and never
  recomputed" and "it is recomputed but the integer division mangles it". The arithmetic at
  1920x1080 (scales 14 and 18) predicts only a 3.7% error, **so the integer division alone does not
  explain 33.3%** -- which points at the projection not being rebuilt after the mode change.

**Filed as ASPECT-1 (5 pts) at the top of the BoB queue, with S1 named**: print
`window_width/window_height/aspectRatio/FoV` at `Set3dWindow` and at the 3D projection setup, once
per mode change, and A/B against a 4:3 forced mode (`BOB_FORCE_MODE=1440x1080`) -- the disc must
measure 1.000 there. Do not "fix" the aspect before that trace says which of the two mechanisms is
in force; a guess here changes the field of view of every view in the game.

**Honest limits of this sprint.** (1) The gold clip is a manoeuvring sortie and the port flight is
straight and level on autopilot, so the gold's larger frame-to-frame motion (8-73 against 3-11) is a
scenario difference and is not claimed as a defect. (2) The port's mirror graticule is sharper and
higher-contrast than the gold's, which is what 192 px of the same art looks like next to 50 px;
not claimed as a defect either. (3) Nothing was changed in the code this sprint.

**R3.4: closed after 5 sprints (4 on 2026-09-03 + this one), by evidence rather than by a fix.**

## ASPECT-1 S1 (Opus 5, 2026-09-15) — ⛔ **the 3D projection is INNOCENT**: it matches the GL viewport exactly, so the 1.333 stretch is somewhere else — and the measurement is sharper than the item was filed with

ASPECT-1 was filed (R3.4 S5, today) on a measurement that has now been repeated on a fresh build: the
Bf 109's round rear-view mirror is **50x50 px (w/h 1.000)** in the gold under Wine at 800x600 and
**192x144 (w/h 1.333)** in this port at 1920x1080, in every frame of both. The filed suspicion was
that a 4:3 projection is rasterised into a 16:9 window.

**S1 printed the projection's own aspect beside the surface it is drawn into** (`BOB_TRACE_ASPECT`,
in `Lib3D::_SetProjectionMatrix`, keyed on the (aspect, viewport) pair with a TABLE FULL notice):

    [aspect] projection aspect=1.7778 (Lib3D aspectRatio=1.7778)  GL viewport 1920x1080 = 1.7778
             -> horizontal stretch 1.0000

⛔ **1.0000. The projection is right.** `Lib3D::aspectRatio` is `dwRenderWidth/dwRenderHeight`
(`LIB3D.CPP:3712`) and it tracks the mode change; `_SetProjectionMatrix` builds `h = aspect*w` with
`_11 = w`, `_22 = h`, which is the correct widescreen matrix. **A round object drawn through this
projection comes out round.** The filed mechanism is eliminated.

⚠️ **And the file the item's analysis leaned on is DEAD CODE.** `SRC/HARDWARE/WIN3D.CPP` — the
`aspectRatio = FoV*(window_height/window_width)` with the integer `virtualXscale`, and
`viewdata.scalex = window_width/2` — **is not in the build**: `SRC/HARDWARE/CMakeLists.txt` compiles
only `_HARD.CPP`, and its own header says the legacy `Hardwin` files are commented out of the unity.
The same is true of `MATRIX.CPP`'s `POLYGON.viewdata.scalex` screen transform, which sits inside a
`/*DEAD … DEAD*/` block. I wrote a trace into `WIN3D.CPP` first and it never compiled — reverted.
[[stale-duplicate-sources]] again, and the FULLW/FULLH 4:3 reasoning in ASPECT-1's backlog row is
about code that does not run.

⭐ **The measurement, restated properly, and it says more than "stretched".** Relative to the window:

| | disc w | disc h | % of width | % of height |
|---|---|---|---|---|
| gold, 800x600 | 50 | 50 | 6.25% | 8.33% |
| port, 1920x1080 | 192 | 144 | 10.00% | 13.33% |

A 4:3 picture scaled to fill 1920x1080 would put the disc at **120x90**. The port draws **192x144** —
the same 1.333 distortion **and 1.6x larger in both dimensions**. So the port's cockpit is both
magnified and stretched with respect to the gold's, which is not what a wrong projection aspect does.

**S2, sharply.** The mirror is cockpit geometry, and R3.2 established that the cockpit reaches the
draw path as **screen-space `XYZRHW` vertices** (that is why the depth sort could delete instrument
bezels by their RHW z). Screen-space vertices do not pass through the projection matrix at all, so
their x and y come from whatever the cockpit path computes — and the one live candidate is the
cockpit's own layout against a fixed canvas. **Print, for one frame, the screen coordinates of the
mirror quad's four vertices together with the window size**, in `draw_fvf`/`DEV_DrawIndexedPrimitiveVB`
keyed on the 2D flag. If they are authored against 1024x768 and scaled per-axis to 1920x1080, the
scale ratio is 1.875/1.40625 = **1.3333**, which is the number — and 1024x768 is exactly the mode
this port boots in before the campaign resolution changes it.

⚠️ **The A/B arm of this sprint did NOT work, and it must not be re-used as written.**
`BOB_FORCE_MODE=1440x1080` applies at startup and the campaign resolution immediately overrides it:

    [vid] BOB_FORCE_MODE -> 1440x1080
    [vid] ChangeDisplaySettings 1440x1080 -> 1920x1080

Both arms therefore flew at 1920x1080 and both measure 192x144 — a control that controlled nothing. A
4:3 arm has to pin the CAMPAIGN resolution, not the boot mode.

**ASPECT-1: 1 sprint. The filed cause is eliminated by measurement and the search has moved to the
2D cockpit path.**

## ASPECT-1 S2 (Opus 5, 2026-09-15) — the control arm exists now, and it produces a **bit-identical** mirror: the distortion does not depend on the display mode at all

S1's 4:3 arm was overridden by the campaign resolution, so both arms flew 16:9. `BOB_PIN_MODE=WxH`
(new, in `bob_change_display_mode`) refuses every later mode change, which is what a 4:3 arm needs:

    [vid] BOB_FORCE_MODE -> 1440x1080
    [vid] ChangeDisplaySettings 1024x768 -> 1440x1080
    [centre] UI content 1024x768 in window 1440x1080 -> offset (208,156)
    [vid] BOB_PIN_MODE: refusing 1920x1080, staying 1440x1080

**The pin holds — and the picture does not change by a single pixel.**

| arm | mode the game ends up in | dumped frame | mirror disc |
|---|---|---|---|
| default | 1920x1080 | 1920x1080 | x 1698..1889, y 23..166, **1.333** |
| pinned 4:3 | **1440x1080** | 1920x1080 | x 1698..1889, y 23..166, **1.333** |

⭐ **Bit-identical bounding box across a genuine display-mode change.** That eliminates every
mechanism of the form "the picture is authored at one resolution and scaled per-axis to the window":
such a mechanism would have to move the disc when the mode moves. It does not move at all.

⚠️ **And the run exposes a second, independent defect on the way.** The front end's centring code
sees a **1440x1080** window in the same run in which the projection trace and the frame dump both
report **1920x1080**:

    [centre] UI content 1024x768 in window 1440x1080 -> offset (208,156)
    [aspect] ... GL viewport 1920x1080 = 1.7778

**The mode the game believes it has and the surface it actually renders into have diverged** —
`ensure_window` sets `g_scrW/g_scrH` and calls `SDL_SetWindowSize`, yet the drawable the 3D pass
reads (and `bob_gdi_screen_size` reports) stayed at the desktop size. A resolution change can be
accepted by the front end and ignored by the renderer. That is worth an item of its own: it is the
shape of the PO's old "setting campaign resolution at 1920x1080 has no effect" (S182), seen from the
other direction.

**S3:** find what actually sizes the 3D drawable. `Lib3D` takes `dwRenderWidth/dwRenderHeight` from
`windowRect` at device creation (`LIB3D.CPP:3680`) and the aspect it derives there is correct for
1920x1080, so something re-establishes the desktop size after `ensure_window` shrinks it.
`BOB_TRACE_VID=1` already prints deferred/off-thread resizes; add the caller to it and run the pinned
arm again. **Until the drawable itself is 4:3, no 4:3 control exists** — and the disc being
mode-invariant says the answer is not in the mode at all but in how the cockpit's own geometry is
sized.

**ASPECT-1: 2 sprints.**

## ASPECT-1 S3 (Opus 5, 2026-09-15) — the disc is **exactly square on any 4:3 canvas**, which separates the item into two numbers: a 4:3→16:9 stretch and a 1.6x cockpit magnification

S2 left the distortion mode-invariant and pointed at the cockpit's own geometry. S3 takes the
measurement apart arithmetically before spending another flight on it.

**The disc measures 192 x 144 px in a 1920x1080 frame**, i.e. **10.00% of the width and 13.33% of the
height**. Carry those two fractions onto a 4:3 canvas:

| canvas | disc becomes |
|---|---|
| 1024 x 768 (this port's BOOT window) | **102.4 x 102.4** |
| 800 x 600 (the gold's window) | **80.0 x 80.0** |

⭐ **Square to the first decimal, and square on ANY 4:3 canvas** — which is what "the cockpit is laid
out against a fixed 4:3 space and presented on a 16:9 drawable" predicts exactly, and is consistent
with S2's mode-invariance: the layout space does not move when the display mode does. It is the same
1.3333 the item was filed with, but located in the presentation of a fixed canvas rather than in the
projection matrix S1 exonerated.

⭐ **And the second number falls out cleanly, which the item had been conflating with the first.**
On that same 800x600 canvas our mirror is **80 px** where the gold's is **50 px**: the port's cockpit
is **1.6x magnified** with respect to the shipped game, independently of the stretch. A narrower
horizontal field of view, a different cockpit scale, or a different eye position would each do that,
and none of them is the aspect bug.

**So ASPECT-1 is two defects and they should be measured separately from here:**
1. **the stretch** — a 4:3 cockpit canvas presented on a 16:9 drawable (circles 33% too wide);
2. **the magnification** — the cockpit drawn 1.6x larger than the original at the same canvas.

⚠️ **Both of the above are arithmetic on one measured pair, not an instrumented observation**, and
this project's own record says not to bank that. **S4 is the instrument, and it settles both at
once:** print the screen-space coordinates of the mirror quad's four vertices in `draw_fvf` /
`DEV_DrawIndexedPrimitiveVB` for the 2D-flagged draws, together with the window size. If they arrive
as `XYZRHW` in a 1024x768 (or other fixed) range, statement (1) is confirmed at the source and the
fix is a per-axis scale; if they arrive already in window pixels, the canvas theory is dead and the
cockpit geometry itself is the suspect. **Either way the vertex range also gives the cockpit's own
scale**, which is statement (2).

**ASPECT-1: 3 sprints.**

## ASPECT-1 S4 (Opus 5, 2026-09-15) — ⭐⭐ the mirror quad MEASURED at the draw call: a **128x128 square texture on a 192x145 screen rectangle**, and the coordinates arrive already in final-frame pixels

S3 named the instrument and the tree already had it: `BOB_PIXPROBE="x,y"` (R3.9) reports every
pre-transformed primitive whose screen bbox covers a pixel, with its bound texture. Aimed at the
mirror's centre (1793,94) on a real-GL campaign flight with `BOB_MIRROR=1`:

    [pixprobe] (1793,94) covered by prim=6 count=4 fvf=0x3c4 bbox=(1698,22)-(1890,167)
               tex=0xd9da2ba0 128x128 bpp=16 isRTT=0 masked=1 uniqueTextID=0x07bd

⭐ **A 128x128 SQUARE, masked (chroma-keyed) texture — the round mirror art — drawn onto a
192 x 145 screen rectangle.** That is the ellipse, measured at the draw call rather than off the
frame buffer: the ratio is 1.324, and the bbox matches the 192x144 rim measured in the dumped frames
to a pixel.

⭐ **And `fvf=0x3c4` is `XYZRHW`: the vertices arrive PRE-TRANSFORMED, in final 1920x1080 frame
pixels.** So nothing downstream stretches them — the game itself computed a 192x145 rectangle for a
square texture. That also explains why S1's projection trace was innocent and S2's mode change moved
nothing: **the mirror never goes through the projection matrix at all.**

⭐ **The same probe names three more draws over that pixel, and one of them is the shape of the whole
answer:**

    bbox=(0,0)-(1920,164)      256x256 texture   <- the cockpit's top bar, full frame width
    bbox=(1681,0)-(1920,246)   256x256 texture   <- imagemap 547
    bbox=(960,0)-(1920,1080)   256x256 texture   <- imagemap 1007: EXACTLY the right HALF of the frame

**The cockpit is laid out in fractions of the window and painted from 256x256 art**: one quad covering
x 0.500–1.000 and y 0.000–1.000 of the frame. A layout expressed as fractions of a 16:9 window, when
the fractions were authored for 4:3, stretches every circle in it by exactly (16/9)/(4/3) = 1.3333.

**And the mirror quad fits that reading to a pixel.** Carry its 192x145 back onto a 4:3 canvas:

| authoring canvas | the quad would be |
|---|---|
| 640 x 480 | **64.0 x 64.4** |
| 800 x 600 | 80.0 x 80.6 |
| 1024 x 768 | 102.4 x 103.1 |

**Square to within half a pixel at every 4:3 size** — a 64x64 quad for a 128x128 texture at 640x480
being the tidiest of them.

**So ASPECT-1's mechanism is now located to a layer:** the cockpit's own layout computes screen
rectangles as **per-axis fractions of the window**, from art and coordinates authored for 4:3. Not
the projection (S1), not the display mode (S2), not a blit (this sprint: the vertices are already
final-frame).

**S5:** find where the cockpit turns its layout into those XYZRHW coordinates — the draw is
`prim=6 count=4` (a triangle fan quad) reaching `draw_fvf` through `DEV_DrawPrimitiveVB`, so a
one-shot `backtrace()` at the covering draw names the caller, exactly as R3.9's `BOB_PIXPROBE_BT=1`
did for the loader screen. **`BOB_PIXPROBE_BT=1` with the same probe point is the whole experiment**
— it is already implemented and needs no rebuild. The fix, once the site is known, is to scale the
layout by a single factor and centre it, rather than per-axis.

**ASPECT-1: 4 sprints — AT THE CAP for this pass, and the item is one backtrace from its fix site.**

## R3.8 S1 (Opus 5, 2026-09-15) — the in-flight A/B against Wine `bob.exe` finally EXISTS, and its first pass finds a new user-facing defect and a constraint that ASPECT-1 needs

R3.8 is *"A/B each in-flight view vs Wine `bob.exe`"* and had never been started because there was no
in-flight gold. There is now: `~/gold standard/bob/bob_convoy_campaign.mp4` (found for R3.4 today) is
the real game flying the same German/Luftwaffe Convoys campaign our `tools/bob_realgl_flight.sh`
flies, in the same Bf 109 cockpit. First sweep, gold at 800x600 against ours at 1920x1080, matched on
"straight and level at altitude over cloud":

| feature | gold | port | verdict |
|---|---|---|---|
| rear-view mirror | round, 50x50 px (6.3% of view width) | **192x145, w/h 1.32** | ASPECT-1 |
| mirror CONTENT | sky / horizon / ground, ~1,100 colours, sd 33–42 | sky / horizon / ground, ~1,000 colours, sd ~35 | ✅ parity (R3.4) |
| **gunsight reticle** | a **soft ivory painted ring** with four thick radial ticks, on the sight glass, ~39x42 px (**4.9%** of view width) | a **thin bright-orange vector circle** with four thin crosshairs, 141x142 (**7.3%** of view width) | 🔴 **different object — filed as SIGHT-1** |

🔴 **SIGHT-1 (new, 5 pts): the gunsight reticle is the wrong thing, the wrong colour and half again
too big.** `doc/reference/sight-gold-vs-port-2026-09-15.png` is the pair. The original's sight is
**artwork on the reflector glass** — ivory, soft-edged, thick ticks; ours is **line-drawn**, crisp,
bright orange, and 1.5x larger as a fraction of the view. This is on screen in every sortie and it is
what the player aims with, so it is a first-class parity defect rather than a cosmetic one.
**S1 for that item:** find whether the port draws it because the reflector's artwork failed to load
(in which case this is a texture/imagemap bug of the same family as R3.4's mirror) or because a
compat path substitutes a drawn graticule by design.

⚠️ **And a constraint ASPECT-1 must have before its S5.** The reticle in OUR build measures
**141 x 142 — round to within a pixel — in the same frame in which the mirror measures 192 x 145.**
Two objects a few degrees apart in the same cockpit, one round and one 1.32x wide. **So there is no
global cockpit stretch**, and ASPECT-1 S3's "a fixed 4:3 canvas presented on a 16:9 drawable" cannot
be the whole story.

⚠️ **But the reticle is NOT a clean control either**, and saying so matters: it is a different object
from the gold's, drawn by a line path rather than as a textured quad, so it may be produced in a
square space and be round for reasons that say nothing about the cockpit's geometry.
**ASPECT-1 S5 needs a round object that is definitely cockpit MODEL geometry** — an instrument dial
bezel is the obvious candidate, and `BOB_PIXPROBE` at its centre names its quad the same way it named
the mirror's.

**R3.8: 1 sprint. The gold comparison it was filed for is now runnable, and it paid on its first pass.**

## SIGHT-1 S1 (Opus 5, 2026-09-15) — the reticle is ART, not a drawn graticule; the game's own gunsight code is COMPILED OUT; and the measured difference is **colour**: the gold's ring is pure white, ours is orange

R3.8 S1 filed this from a picture and read it as *"ours is line-drawn, the gold's is painted"*.
S1 checks that, and it is **wrong in the useful direction**.

⭐ **1. The game's own gunsight code does not exist in this build.** `shape::dogunsight`
(`3DCOM.CPP:2383`) is wrapped in

    #ifndef _ACTIVEGUNSIGHT_
    #pragma warnmsg("**** MiG Alley GUN-SIGHT code removed //CSB ****")
    #else
    ... 200 lines of vector gunsight ...

and **`_ACTIVEGUNSIGHT_` is not defined anywhere in the tree** (one grep, one hit — the `#ifndef`
itself). So the opcode is an empty function. Whatever we see is not that.

⭐ **2. It is a textured cockpit quad, and the probe names it.** `BOB_PIXPROBE="960,540"` on a
real-GL flight:

    [pixprobe] (960,540) covered by prim=6 count=4 fvf=0x3c4 bbox=(883,463)-(1037,624)
               tex=... 128x128 bpp=16 masked=2  imagemap number = 784 (dir 3, file 16)

and the dumped texture (`doc/reference/sight-imagemap784-art-2026-09-15.png`) is exactly what is on
screen: **an orange ring with a four-arm cross, on a green chroma key**. So the port draws the art it
was given, correctly. My "line-drawn" reading is retracted.

⭐⭐ **3. The real difference is COLOUR, measured rather than described.** Ring pixels against the
glass they sit on, each within its own frame:

| | ring RGB | glass around it |
|---|---|---|
| **gold** (`bob.exe`/Wine) | **(254.8, 254.6, 253.8)** — pure white, saturated | (244.3, 244.1, 246.2) |
| **port** | **(199.1, 149.4, 74.8)** — orange | (144.4, 169.8, 192.8) |

**The original's reticle is WHITE on a near-white glass; ours is ORANGE on a duller one.**

⭐ **And that points somewhere specific: the blend.** A reflector gunsight is an *illuminated*
overlay. An orange graticule drawn **additively** over a bright sky saturates to white — which is
precisely the gold's (255, 255, 254) on a 244 background. Drawn **modulated/opaque**, the same art
stays orange, which is ours. **Same texture, different blend state** is the hypothesis, and it is the
same family as R3.7's white-opaque `LF_LIGHTSOURCE` polygons.

**S2:** print the blend/alpha state of the draw that carries imagemap 784 (the `BOB_TRACE_COL` and
alpha-state traces already exist in `draw_fvf`) and compare it with a known-additive overlay. Then
A/B an additive blend for that one material and re-measure the ring against the gold's (255,255,254).

⭐ **Two by-products for ASPECT-1, and both matter:**
* the reticle quad is **155 x 162 px — near square (0.957)** in the very frame where the mirror quad
  is **192 x 145 (1.32)**. Two textured cockpit quads, drawn by the same path, one square and one
  stretched. **Whatever ASPECT-1 is, it is not a global cockpit stretch** — this is the second
  independent witness.
* ours is 8.1% of the view width against the gold's 4.9% — **1.6x too large**, the same factor the
  mirror showed. ASPECT-1's "magnification" half now has two witnesses too, and they agree.

**SIGHT-1: 1 sprint. Root cause narrowed from "wrong object" to "right art, wrong blend", with the
colour measured on both sides.**

## SIGHT-1 S2 (Opus 5, 2026-09-15) — ✅ **FIXED: the reflector sight blends ADDITIVELY.** The shipped blend drew the gunsight DARKER than the sky behind it

S1 narrowed this to "right art, wrong blend" and named the experiment. S2 ran it.

⭐ **1. The blend, printed.** `BOB_TRACE_SIGHT=1` on a real-GL flight:

    [sight] imagemap 784 drawn with alphaBlend=1 src=0x302 dst=0x303
            (GL_SRC_ALPHA / GL_ONE_MINUS_SRC_ALPHA)

Ordinary alpha blending — so the orange art is composited over the sky as paint, not as light.

⭐⭐ **2. The A/B, four frames each, ring pixels on the reticle annulus against the sky inside it:**

| arm | ring RGB | ring lum | sky lum | ring − sky |
|---|---|---|---|---|
| **alpha (shipped)** | (192–205, 150–163, 86–99) | 143–156 | 167–198 | **−25 to −42 (DARKER)** |
| **additive** | (248–250, 246–247, 197–209) | 231–234 | 172–211 | **+24 to +60 (BRIGHTER)** |
| **gold** | (254.8, 254.6, 253.8) | — | 244 glass | bright |

**The shipped build draws the gunsight darker than the sky.** A reflector sight is a projected light;
the gold's is saturated white. Additive puts ours on the right side of the background and within a
few counts of the gold on R and G.

✅ **Shipped: additive is the default for imagemap 784.** `BOB_SIGHT_NOADD=1` restores the old blend;
`BOB_SIGHT_ADD=<imagemap>` applies the same treatment elsewhere. **Verified with no env set at all**
(the rule that a default only exercised through an override is not a default): ring (248.1, 246.1,
208.7) / (249.0, 246.9, 203.9) / (249.2, 246.7, 202.0), +23.6 / +54.6 / +55.6 against the sky, 64
frames, no crash. `doc/reference/sight-fix-gold-before-after-2026-09-15.png` is gold | before | after.

⚠️ **What is NOT claimed.** Our ring's blue channel is **197–209 against the gold's 254** — ours is
slightly warm because the source art is orange and additive adds that orange to the sky, while the
gold's saturates to neutral white. Closer in kind and direction, not identical. Whether the original
also drove the sight's brightness from a lamp setting is untested.

⛔ **And a self-inflicted detour worth recording, because it cost three flights.** The first version
of this hook called `bob_imagemap_number_of()` on **every 2D draw**. That helper scans
`MaxMapDirs x 256` entries through `Image_Map.GetImageMapPtrDontLoad`, so it (a) starved the frame
loop — "NO FRAMES" in both arms, which reads as a harness failure — and (b) **SEGV'd inside the
accessor, deterministically, 2 runs of 2** (`addr2line`: `GetImageMapPtrDontLoad <- draw_fvf <-
DEV_DrawPrimitiveVB <- Lib3D::RenderTPolyList`). Fixed by adding the forward lookup
`bob_imagemap_ptr_of(number)` (`IMAGEMAP.CPP`) and comparing pointers — O(1) per draw — and gating the
lookup on `g_bob_flight_active` so the accessor is never touched during boot. **A diagnostic that is
cheap in a one-shot probe can be fatal in a per-draw hook**; the crash was mine, not the game's.

⚠️ **One more, and it is the third time this year.** Re-applying the hook with
`open(p,'wb').write(s.encode('latin-1'))` where the new comment contained a non-latin-1 character
**emptied `bob_video.cpp` — 4,337 lines to 0.** The rule that saved it: `wc -l` the file immediately
after any failed edit. Recovered with `git checkout --` (the file's last commit was ASPECT-1 S2, so
only this hook's first draft was lost) and re-applied with the encode done **before** the open, and
the comment kept ASCII. [[shell-edit-commit-traps]].

**SIGHT-1: 2 sprints. Fixed and verified against the gold.**

## ASPECT-1 S5 (Opus 5, 2026-09-15) — ⭐⭐ **it is not an aspect defect at all: in the same frame our gunsight is ROUND and our mirror is 1.32 wide.** The item belongs with MIRROR-1

S4 measured the mirror quad at the draw call and left one question: is anything else in the cockpit
stretched? SIGHT-1 answered it as a by-product, and S5 measures both sides properly.

**Two round objects, two games, measured the same way:**

| object | gold (`bob.exe`/Wine, 800x600) | port (1920x1080) |
|---|---|---|
| **gunsight ring** | 42 x 42–44 px, **w/h 0.955–1.000** | 141 x 142, **w/h 0.993** |
| **rear-view mirror** | 50 x 50 px, **w/h 1.000** | 192 x 145, **w/h 1.324** |

⭐⭐ **In our own build, in one frame, a few degrees apart: the gunsight is round and the mirror is
1.32 wide. In the gold both are round.** Two textured cockpit quads through the same `draw_fvf` path,
both carrying a 128x128 SQUARE texture, and only one of them is stretched.

**So every negative result this item has collected now makes sense together:**

* S1 — the projection's aspect matches the GL viewport exactly (stretch 1.0000). Innocent, and it
  had to be: the gunsight proves the path is not stretching things.
* S2 — a genuinely pinned 4:3 display mode changes the mirror by **zero pixels**. Of course: the
  defect is not in the mode.
* S3 — "square on any 4:3 canvas" was arithmetic that fitted, and fitted for the wrong reason.
* S4 — the vertices arrive pre-transformed in final-frame pixels, so nothing downstream stretches
  them.

**ASPECT-1 is therefore a MIRROR defect, and its home is MIRROR-1's family** — the same quad whose
projection aspect MIRROR-1 S4 had to correct from a hardcoded 5:2 to 1.0 to make the reflection
render at all. The name "ASPECT-1" is now actively misleading and the row should say so.

⚠️ **The magnification half does NOT resolve the same way, and must not be folded in.** Scaling both
objects from the gold's 800x600 to our 1080-row frame: the gunsight ring should be ~76 px and we draw
**141** (x1.86); the mirror should be ~90 px tall and we draw **145** (x1.6). **Two different
magnification factors** — so it is not one wrong field of view either. That is a separate question
with its own measurement, and it is not answered here.

**S6:** print the **four screen vertices** of the mirror quad and of the gunsight quad in the same
frame (`BOB_PIXPROBE` already finds both draws; it currently reports only the bbox). If the mirror's
four corners form a 1.32:1 rectangle while the sight's form a square, the difference is in the
geometry those two draws are handed — and the mirror is built by `docreatemirror`/`UseMirror`, which
is code, whereas the sight is plain model geometry. That is the fork worth spending a sprint on.

**ASPECT-1: 5 sprints. Re-aimed from "the view is stretched" to "the mirror quad is", with both
halves measured in both games.**

## ASPECT-1 S6 (Opus 5, 2026-09-15) — the mirror quad is an **exact 192.000 x 144.000 axis-aligned screen rectangle** — 10.000% x 13.333% of the window — and it arrives through the ordinary polygon batch

S5 showed the distortion is per-quad. S6 prints the quads themselves (`BOB_PIXPROBE` now reports the
four screen vertices, not just the bbox).

**The mirror quad** (128x128 masked texture, `uniqueTextID` idx 1981):

    v0 = (1698.0,  22.5)    v1 = (1890.0,  22.5)
    v3 = (1698.0, 166.5)    v2 = (1890.0, 166.5)

**192.000 x 144.000, axis-aligned, ratio 1.33333 — exactly 4:3, to three decimals.** As a fraction of
the window that is **10.000% of the width by 13.333% of the height**.

**Its neighbour, the gunsight glass** (128x128, idx 1989), in the same frame:

    v0 = (1190.8, 288.9)    v1 = (1190.8, 780.6)    v2 = (729.2, 780.6)

**461.6 x 491.7, ratio 0.939** — nothing round about it, and taller than it is wide.

⭐ **So one cockpit polygon lands on a perfect 4:3 rectangle and the one beside it does not.** A
rounded-off 4:3 is not what perspective does to a quad; it is what a computed rect looks like.

⚠️ **And it is NOT drawn by a special path.** `BOB_PIXPROBE_BT=1` (extended this sprint: the existing
backtrace sat inside the texture-dump guard, so a draw whose texture has no CPU bits — the mirror's —
never produced one) names the caller:

    draw_fvf <- DEV_DrawPrimitiveVB <- Lib3D::RenderTPolyList <- Lib3D::EndScene
             <- ThreeDee::render3d <- ThreeDee::render <- View3d::drawloop

**The ordinary batched polygon list, exactly like every other cockpit polygon.** So nothing downstream
singles the mirror out: the 4:3 shape is already in the vertices handed to `Lib3D`.

**S7 — one print, and it forks cleanly.** The reflection polygon has its own shape opcode,
`shape::dodrawreflectpoly` (`3DCOM.CPP:4961`), which copies `newco[*vertexp++]` — the shape
interpreter's already-transformed vertices — into `BeginPoly`. Print those four `newco` entries for
this polygon. **If they are already 4:3 there**, the shape interpreter or the cockpit model is the
source and the search moves to `newco`'s producer; **if they are not**, something between that opcode
and `RenderTPolyList` reshapes them, and the list is short.

**ASPECT-1: 6 sprints. The defect is now a single polygon whose four screen coordinates are known to
a tenth of a pixel.**

## ASPECT-1 S7 (Opus 5, 2026-09-15) — ⛔ **`dodrawreflectpoly` never fires.** The mirror is an ORDINARY textured cockpit polygon, so its 4:3 shape is in the model or in the shape interpreter's transform

S6 left the fork: are the mirror's vertices already 4:3 where `shape::dodrawreflectpoly` copies
`newco[]` into `BeginPoly`, or does something reshape them later? `BOB_TRACE_MIRRORPOLY` (new, in
that opcode) prints the polygon's vertices once.

⛔ **It printed nothing.** A full real-GL flight with `BOB_MIRROR=1` — 82 HUD samples, the mirror
visibly rendering — produced **zero** `[mirrorpoly]` lines.

**And the silence is meaningful, not a dead instrument.** `dodrawreflectpoly`'s body is gated on
`Save_Data.cockpit3Ddetail[COCK3D_SKYIMAGES]`, and that flag is demonstrably ON in this run: the
mirror pass is gated on the same flag (`3DCODE.CPP:6475`), and with it off the mirror texture is
merely wiped — yet the disc measures ~1,000 distinct colours with sd 35 (R3.4 S5), which is real
reflected content. **So the opcode is not in the 109's cockpit shape at all.**

⭐ **Therefore the mirror is drawn as an ordinary textured polygon** that happens to carry the mirror
material — consistent with S6's backtrace, which showed it arriving through `RenderTPolyList` exactly
like every other cockpit polygon, and with S5, which showed the gunsight (also ordinary) coming out
round in the same frame.

**Where that leaves the item.** The 192.000 x 144.000 rectangle is either
(a) the shape's own geometry — the cockpit model really does hold a 4:3 mirror quad, in which case
the ORIGINAL draws the same quad round and the difference is in the transform; or
(b) introduced by the shape interpreter's transform of that polygon.
**S8 separates them by identifying the polygon by its MATERIAL rather than by an opcode** — the
mirror's material is recorded in `ThreeDee::mirrorMaterial` by `UseMirror` — and printing its
vertices both before and after the interpreter's transform.

⚠️ **Housekeeping, recorded because it stopped the sprint dead.** The build failed mid-sprint with
`fatal error: error writing to /tmp/cck1cUeQ.s: Disk quota exceeded`, and even `pwd` could not write.
**The session scratchpad had reached 5.4 GB of a 7.6 GB tmpfs**, almost all of it 1920x1080 PPM frame
dumps at ~6 MB each. Deleting them freed 4.4 GB (tmpfs back to 22%). The standing rule "never
materialise game data into /tmp" covers this — **frame dumps are game data** — and this sprint's
re-run wrote its frames under `/home` instead, where there are 2.2 TB free.

**ASPECT-1: 7 sprints. The reflection opcode is eliminated; the mirror is ordinary geometry and the
search is down to two possibilities.**

## ASPECT-1 S8 (Opus 5, 2026-09-15) — ⛔⛔ **`BOB_MIRROR` has never applied on the campaign path.** Every mirror measurement this project has taken ran with Reflections OFF

S7 found `dodrawreflectpoly` never fires. S8 tried to identify the mirror polygon by its material,
printing `ThreeDee::mirrorMaterial` from `UseMirror` — and **`UseMirror` never fires either.** Two
mirror opcodes, zero calls. That is not a coincidence; it is a switch.

⛔ **The switch is `BOB_MIRROR`, and it is in the wrong block.** `MIG.CPP` sets
`Save_Data.cockpit3Ddetail |= COCK3D_SKYIMAGES` inside

    if (getenv("BOB_BOOT_FRONTEND")) { ... if (fullMission) { ... BOB_MIRROR here ... } }

— the **Quick-Mission** boot path. But `tools/bob_realgl_flight.sh`, which R3.4, MIRROR-1 **and**
ASPECT-1 all use, drives the **CAMPAIGN** path and sets `BOB_FRONTEND`, not `BOB_BOOT_FRONTEND`.
**So the Reflections setting was never turned on in any of those flights**, and `RenderMirror`,
`UseMirror` and `dodrawreflectpoly` — all gated on it — correctly did nothing.

⚠️ **And the tell was in every single log.** `[boot] BOB_MIRROR: ... enabled` **never printed once**
in a campaign flight. I greped for it twice this session, saw nothing, and moved on both times
because the mirror was visibly there on screen. It was there because it is **painted cockpit
artwork** — a 128x128 masked imagemap on a quad — which is exactly what S7 and S8's zero traces say.

✅ **Fixed.** The same hook now also runs on the campaign Fly path (`FULLPSYS.CPP`, beside the
`[campfly] Fly` action), and it announces itself:

    [boot] BOB_MIRROR: reflections/mirror (COCK3D_SKYIMAGES) enabled (campaign path)

⚠️ **`UseMirror` STILL traces zero calls with Reflections on**, so a second question is now open and
it is a better one: **does the Bf 109's cockpit shape contain a `docreatemirror` opcode at all?** If
it does not, this aircraft has no modelled mirror in the port and everything measured is the painted
art.

🔴 **What this puts in doubt, stated plainly.** R3.4 S5 closed "the mirror shows a live horizon like
the gold" on content statistics taken in these flights. **Those flights had Reflections off.** Its
own numbers hinted at it — our frame-to-frame delta was **3–11** against the gold's **8–73**, which
S5 attributed to a scenario difference (straight-and-level against a manoeuvring sortie). That
explanation is no longer the simplest one: painted art versus a live reflection fits better.
**R3.4 should be re-opened, not quietly amended.**

**S9:** re-run R3.4's exact measurement now that the switch works, and in the same sprint dump the
Spitfire and Hurricane cockpit shapes for the `docreatemirror` opcode. Two results, one run plus one
headless scan: either the disc comes alive (and R3.4's conclusion survives on corrected evidence), or
it does not and the 109 has no mirror to show.

**ASPECT-1: 8 sprints. A switch that was never on is the reason two items' worth of measurements
need re-taking.**

## ASPECT-1 S9 (Opus 5, 2026-09-15) — ⛔⛔ **`docreatemirror` is never REACHED: the Bf 109 has no modelled mirror.** The disc is painted artwork, and R3.4 is refuted

S8 fixed the switch and `UseMirror` still traced zero calls, leaving two readings: the opcode is
absent from the 109's cockpit shape, or it is present and the gate is still false. S9 traces the
opcode's **entry, outside its gate**.

⛔ **Nothing. A full real-GL campaign flight, 26 frames dumped, Reflections confirmed on —**

    [boot] BOB_MIRROR: reflections/mirror (COCK3D_SKYIMAGES) enabled (campaign path)
    (no [aspect] docreatemirror REACHED line, ever)

**The opcode is not in the shape.** Which closes the whole family at once:

| trace | result | reading |
|---|---|---|
| `docreatemirror` entry (ungated) | **0 calls** | the 109's cockpit shape has no mirror object |
| `UseMirror` | 0 calls | consequence — only `docreatemirror` calls it |
| `dodrawreflectpoly` | 0 calls | consequence — no reflection polygon either |
| the disc on screen | 128x128 **masked imagemap** on a quad | **painted cockpit artwork** |

🔴 **So R3.4 is REFUTED, not merely in doubt.** Its S5 closed *"the mirror shows a live horizon, and
the port now matches the gold"* on content statistics — ~1,000 distinct colours, sd ~35 — taken from
**a painted texture in an aircraft that has no mirror**. Its own numbers carried the warning: our
frame-to-frame delta was **3–11** against the gold's **8–73**, which S5 explained as a scenario
difference. The simpler explanation was the right one. **R3.4 goes back to open.**

⚠️ **And what the gold shows is now the interesting question.** The gold's 109 mirror content swings
by a factor of two across frames (mean 91 → 200), which is a live reflection, not paint under
changing light. If the original's 109 has a working mirror and ours has no mirror opcode at all,
then **either the shape we load is not the one the original loads** (an LOD, a variant, or a
cockpit-file mismatch) **or the opcode is being skipped by our shape interpreter**. That is a
data-identity question, and it is the first thing S10 should settle — by dumping the opcode histogram
of the cockpit shape we load and looking for the mirror opcode's byte at all.

⭐ **What survives from this item's nine sprints, all measured:** the projection is correct and
matches the viewport; the display mode is irrelevant; the vertices arrive pre-transformed in final
screen pixels; the gunsight is round while the mirror quad is an exact 192.000 x 144.000; and the
mirror is painted art, not a reflection. **The remaining question is a data one, not a rendering
one.**

**ASPECT-1: 9 sprints, 5 in this pass — OVER THE CAP. Parked here deliberately**, with R3.4 re-opened
and S10 named as a data-identity check rather than more rendering work.

## R3.4 S6 (Opus 5, 2026-09-15) — ⭐⭐ **the data answers: `docreatemirror` is not in ANY shape this port loads.** The census walked every instruction at load time and found zero

ASPECT-1 S9 proved the interpreter never reaches `docreatemirror` and named the two readings: the
opcode is absent from the data, or it is present and a runtime branch skips it. That is a data
question, so it is asked of the LOADER, not the renderer.

⭐ **`BOB_OPCENSUS=1` (new, `SHAPES.CPP: FixInstrData`)** counts opcodes on the **load-time fixup
walk** — the one pass that visits every instruction of every shape file with no gate, no branch and
no runtime condition. If the byte is in the data this walk sees it.

⛔ **A full real-GL campaign flight, airborne at 5,302 ft, 97 HUD samples:**

    [opcensus]     1 instructions walked (shape 690): docreatemirror=0 dodrawreflectpoly=0
    [opcensus] 50000 instructions walked (shape 184): docreatemirror=0 dodrawreflectpoly=0
    docreatemirror IN SHAPE ...  : never printed

⭐ **And the instrument was made to prove it can speak first** (memory: *instrument-bookkeeping-lies*).
The first version printed only every 200,000 instructions and every mirror hit — it produced **no
output at all**, which is exactly what a census that never ran looks like. The heartbeat at
instruction 1 was added for that reason, and the re-run shows the walk covering 50,000+ instructions
across shapes 690 and 184 while still reporting zero.

**So the mirror opcode is not in the shapes, full stop** — and with it the whole chain:
`docreatemirror` → `UseMirror` → `SetMirrorSeen` → `ThreeDee::RenderMirror` (which bails on
`!mirrorSeen`, `3DCODE.CPP:6487`). Our 109 draws a **painted 192×144 disc** and no render-to-texture
mirror exists to fill.

⚠️ **The limit of this measurement, stated plainly:** BoB loads shapes on demand, so the census sees
the shapes THIS flight loaded — the same recipe, the same squadron (126) and the same cockpit
ASPECT-1 measured the disc in, which is the population the question is about, but it is not every
shape in the game. A flight in a different aircraft would need its own census.

⭐ **What this leaves R3.4 with.** The gold's 109 mirror content swings by a factor of two across
frames (mean 91 → 200) — a live reflection. Ours cannot be one. The remaining possibilities are
narrow and all about DATA rather than rendering: the original loads a different cockpit shape
(a variant or LOD we do not pick), or our shape files are not the ones the original ships. **S7 is a
file-level question** — compare the cockpit shape file our loader opens against what the gold
install holds, by name and by size, before writing another line of renderer code.

**R3.4: 6 sprints (2 in this pass). The rendering explanation is exhausted; the data explanation is
now the only one left standing.**

## R3.4 S7 (Opus 5, 2026-09-15) — ⭐⭐ **the mirror opcode IS in the data — in exactly 2 of 253 shapes, and they are the SPITFIRE and the HURRICANE. The Bf 109 we fly has none, which is historically correct.**

S6 said *"docreatemirror is not in the shapes THIS FLIGHT loaded"* and named the honest version of
that sentence as the next step: walk the whole archive. `BOB_OPCENSUS_ALL=1` (new, `3DCODE.CPP`,
right after `MaxShapeCount` is read) loads every shape the game ships before the flight starts.

⭐ **All 253 shapes, every instruction walked:**

    [opcensus] loading ALL 253 shapes...
    [opcensus] docreatemirror IN SHAPE 65  (instr #440)
    [opcensus] docreatemirror IN SHAPE 172 (instr #3822)
    [opcensus] loaded 253 of 253 shapes

**Two shapes carry it. `shapenum.g` names them:**

    const ShapeNum CPT1 = (ShapeNum)  65;
    const ShapeNum CPT4 = (ShapeNum) 172;

⭐⭐ **And `FLYINIT.CPP`'s `pit1 shp` field says who flies them:**

| cockpit shape | aircraft | modelled mirror |
|---|---|---|
| **CPT1** | **Spitfire A, Spitfire B, Defiant, Blenheim** | **YES** |
| **CPT4** | **Hurricane A, Hurricane B** | **YES** |
| CPT2 | **Bf 109**, He 59 | no |
| CPT3 | Ju 87 | no |
| CPT5 | Bf 110 | no |

**Only the RAF fighters have a modelled rear-view mirror — which is exactly right.** The Spitfire and
the Hurricane carried a mirror on the windscreen frame; the Bf 109E did not.

✅ **So the port is DATA-CORRECT and ASPECT-1 S9's finding is explained rather than merely confirmed.**
Our flight is the Luftwaffe *German Convoys* campaign — the gold video's own allocation screen lists
**Me109 / Me110 / Do17** — so we fly CPT2, there is no mirror object to create, `RenderMirror` bails
on `!mirrorSeen` (`3DCODE.CPP:6487`), and the disc on the windscreen is painted artwork. Nothing is
broken.

⚠️ **Which means R3.4's gold comparison was never comparing a live mirror.** Looking again at
`doc/reference/mirror-gold-vs-port-2026-09-15.png`: **both** discs carry the same white curved
markings. S5 read the gold's ~1,100 distinct colours and sd 33–42 as proof of a live reflection; on a
Luftwaffe flight it is the same painted texture ours draws, at 800x600 and with different lighting.
**S5's "the mirror shows a live horizon and the port now matches" is withdrawn a second time** — this
time with the reason, not just the doubt.

⭐ **And the item now has a PREDICTION, which is what it has lacked for six sprints.** Fly an **RAF**
mission — Spitfire or Hurricane — and `docreatemirror` must execute. The traces to watch already
exist (`[aspect] docreatemirror REACHED`, `UseMirror`, `dodrawreflectpoly`), all three have only ever
printed zero, and all three should fire. **If they do, the live mirror can finally be compared
against a gold frame of the same aircraft; if they do not, there is a real defect and it is in shape
65, not in the renderer.**

**S8:** fly the RAF side and state the prediction before the run.

**R3.4: 7 sprints (3 in this pass). The data question is answered; the port is exonerated for the
109; and the comparison has been aimed at the right aeroplane for the first time.**

## R3.4 S8 (Opus 5, 2026-09-15) — ⭐⭐ **THE PREDICTION HELD. In a Spitfire the mirror chain fires — three traces that had printed zero in every run this item has ever made.**

S7 ended with a prediction written down before the run: *"Fly an RAF mission — Spitfire or Hurricane
— and `docreatemirror` must execute."* The quick-mission boot (`BOB_BOOT_FRONTEND=1`) flies a
Spitfire, which is CPT1 = shape 65, one of the two shapes the archive census found carrying the
opcode.

⭐⭐ **One flight, and the whole chain comes alive:**

    [opcensus] docreatemirror IN SHAPE 65 (instr #440)          <- the shape is loaded
    [aspect]   docreatemirror REACHED (SKYIMAGES=1)             <- the opcode EXECUTES
    [aspect]   UseMirror: mirrorMaterial = 284 (imagemap dir 1 file 28),
               pos (0, -148, 219) normal (0, -32767, 0) fov 8192

**`docreatemirror REACHED`, `UseMirror` and the mirror render pass had each printed ZERO in every
previous run of this item and of ASPECT-1** — nine sprints of zeros, explained in S7 and now
converted into a positive result by changing the aeroplane rather than the code.

⭐ **And the Spitfire's mirror is a different object from the 109's disc**, as the data said it would
be: material **284** (imagemap dir 1, file 28), mounted at (0, −148, 219) with a normal straight up
(0, −32767, 0) and a 45° field of view — a real mirror object with a position, an orientation and a
render target, not a texture painted on the windscreen.

⚠️ **One observation, flagged rather than claimed.** The mirror pass reports

    [aspect] projection aspect=1.0000 (Lib3D aspectRatio=1.7778) GL viewport 1920x1080 -> horizontal stretch 1.7778

The "stretch" figure compares the pass's square projection against the MAIN viewport, which is not
the mirror's render target — MIRROR-1 established that a square projection is correct for a square
mirror FBO. **This is not evidence of a defect until the FBO's own dimensions are printed beside it**,
and saying otherwise would repeat exactly the mistake S5 made with the painted disc.

⭐ **What R3.4 now has, for the first time in eight sprints:** a live mirror, in a known aircraft,
with its material and mount identified — and therefore a comparison that can be made against a gold
frame **of a Spitfire**, which is the aircraft the reference must now come from. The Luftwaffe
convoy video cannot serve; it never shows one.

**S9:** capture the Spitfire mirror and score its content the way S5 tried to — but with the
frame-to-frame delta as the discriminator, since a LIVE mirror must change between frames and paint
cannot. S5's own numbers already carried that test (ours 3–11 against the gold's 8–73) and it was
explained away; run it again now that there is something live to measure.

**R3.4: 8 sprints (4 in this pass — AT THE CAP, parked here).** The mirror is alive, in the right
aeroplane, and the next sprint is a measurement rather than a search.

## ASPECT-1 S10 (Opus 5, 2026-09-15) — ⭐⭐ **the port's mirror WORKS: sky, horizon and terrain rendered into the 128x128 FBO, ~750 distinct colours.** Nine sprints were spent measuring a dead path in an aeroplane that has no mirror

R3.4 S8 produced a live mirror by flying a Spitfire. S10 photographs it. `BOB_DUMP_RTT_DIR=<dir>`
(new) turns the existing single-file RTT dump into a numbered sequence — and puts it under `/home`,
where frames belong on this box.

⭐⭐ **5,077 mirror renders from one Spitfire flight, and the picture is unambiguous:**

    sky band   RGB 129 162 184      (pale blue, upper third)
    ground band RGB  82  85  23     (olive green, below the horizon)
    mean 91.3   sd 62.3   ~740 distinct colours

`doc/reference/mirror-spitfire-live-2026-09-15.png` — **a rear view with a horizon across it.** No
ladder markings, no flat wash.

⛔⛔ **Set that against what this item has been measuring since June.** R3.4's own numbers for the
mirror were **mean 79.2, sd 0.71, FOUR distinct values** — a blank grey square — and MIRROR-1, R3.4
S1–S5, ASPECT-1 S1–S9 all chased why. **They were all flying the Bf 109, whose cockpit shape (CPT2)
carries no mirror opcode at all** (R3.4 S7). The renderer was never broken; the aircraft had no
mirror to render, and the grey square was an unfilled buffer nothing was asked to fill.

✅ **And the geometry question is closed with it.** The mirror render target is **128x128 — square**
(`LIB3D.CPP:2229`), and the pass sets a matching viewport:

    [rtt] MIRROR pass: mirrorRect L=0 T=0 R=128 B=128 -> viewport x=0 y=0 w=128 h=128

So **a projection aspect of 1.0000 is correct**, exactly as MIRROR-1 concluded, and S8's
"horizontal stretch 1.7778" was the trace comparing the mirror pass against the MAIN viewport —
a number about the wrong rectangle. Flagging it rather than reporting it was the right call.

⚠️ **What is NOT shown here.** The content changes only slowly (mean 90.6 → 92.0 over 4,000 frames,
frame-to-frame delta 0.3–0.8), which is what a straight-and-level quick mission should look like and
is therefore **not** evidence either way about responsiveness. A manoeuvring flight is needed before
anyone claims the mirror tracks the aeroplane.

⚠️ **And there is still no gold for this.** The PO's BoB video is a Luftwaffe campaign — Me109 /
Me110 / Do17 — so it can never show a Spitfire mirror. **Comparing this against gold needs a new
capture of the real game flying an RAF fighter**, which is a PO request, not a sprint.

**S11:** fly the Spitfire through a turn with the same dump and show the mirror content following the
horizon — the responsiveness test, which needs no gold at all.

**ASPECT-1: 10 sprints (1 in this pass). The mirror renders, the square target is confirmed correct,
and the item's long grey-square mystery is retired as a wrong-aircraft measurement.**

## ASPECT-1 S11 (Opus 5, 2026-09-15) — ⛔ **the responsiveness test could not run, and S10's explanation of its own numbers was WRONG: the Spitfire is PARKED, not flying**

S10 measured the mirror drifting only 90.6 → 92.0 over 4,000 frames and wrote that this "is what a
straight-and-level quick mission should look like". **It is not, and the reason matters.**

⭐ **`BOB_AUTOFLY=bank[:tick]` (new)** holds `AILERON_LEFT` (DIK 0xCB, `KEYMAPS.H:1250`), held rather
than tapped. It fires exactly as asked:

    [autofly] bank: holding AILERON_LEFT (DIK 0xCB) from tick 120

⛔ **And the horizon in the mirror does not move — because the aeroplane does not move.** With
`BOB_TRACE_HUD=1` on the same flight, 29 samples across the whole run:

    [hud] alt=4ft hdg=242 speed=0Kts thrust=70        <- first sample
    [hud] alt=4ft hdg=242 speed=0Kts thrust=70        <- last sample

**`BOB_BOOT_FRONTEND=1` puts the Spitfire on the ground at 4 ft and 0 knots and leaves it there.**
Heading 242 for the entire flight. A bank key cannot roll a parked aircraft, and a mirror cannot
track an attitude that never changes.

⛔ **So S10's sentence is corrected here rather than left standing.** The mirror's near-constant
content was never evidence about responsiveness in either direction — it is what a stationary
aeroplane's rear view looks like. **The measurement was fine; the story I told about it was not.**

⭐ **What survives S10, and is if anything stronger.** The mirror renders **sky (129,162,184) over
terrain (82,85,23) with a horizon**, ~740 distinct colours, into the 128x128 FBO — from an aircraft
sitting on a runway, which is exactly what a rear view should show there. That is a live render of
the world, and no painted texture in this game looks like it.

⚠️ **What the responsiveness test actually needs**, now that the boot scaffold is ruled out: the
real-GL CAMPAIGN recipe (`tools/bob_realgl_flight.sh`), which is proven to reach **5,302 ft airborne**
— but it flies the Luftwaffe convoys campaign, and R3.4 S7 established the Bf 109 has no mirror. **An
airborne RAF flight is the missing harness**, and it is the same thing R3.4 S9 will need.

**S12:** get the campaign recipe to fly an RAF squadron, then repeat this test. The instrument, the
bank input and the dump sequence are all in place; only the mission is wrong.

**ASPECT-1: 11 sprints (2 in this pass). A test that could not run, said so, and took one of my own
explanations down with it.**

## R3.4 S9 (Opus 5, 2026-09-15) — ⭐⭐⭐ **the airborne RAF flight exists — quick mission 7 — and the Spitfire's mirror TRACKS THE HORIZON: sky, clouds and a banked skyline, changing 14–58 per step**

S11 said the missing piece was "an airborne RAF flight" and called it a PO request. **It was in the
game the whole time.** `FULLPSYS.CPP:640` seeds the flight from `BOB_QM_INDEX`, and
`QMISS.CPP`'s table lists *"FAMILIARISATION : Free Flight"* — `PT_SPIT_A, 1, SKILL_REGULAR,
FT_1000` — **a lone Spitfire at 1,000 feet, already flying.** It is index 7.

⭐ **`BOB_QM_INDEX=7`, and everything this item has wanted for eleven sprints arrives at once:**

    [hud]    alt=962ft hdg=0 speed=283Kts thrust=70        <- AIRBORNE, unlike the parked default boot
    [aspect] docreatemirror REACHED (SKYIMAGES=1)
    [aspect] UseMirror: mirrorMaterial = 284 ... pos (0, -148, 219) fov 8192

⭐⭐ **And the mirror MOVES.** 64 renders of the 128x128 surface through the flight:

| frame | mean | sd | distinct colours | change vs previous |
|---|---|---|---|---|
| 0073 | 200.2 | 19.9 | 1563 | — |
| 0107 | 206.1 | 11.4 | 1124 | **14.7** |
| 0139 | 159.2 | 65.4 | 1317 | **58.0** |
| 0173 | 176.0 | 53.2 | 1257 | **55.6** |
| 0205 | 202.9 | 21.8 | 559 | **33.2** |

**Against 0.30–1.19 per step on the parked Spitfire (ASPECT-1 S11).** The content is not just live, it
is tracking the aeroplane — and `doc/reference/mirror-spitfire-airborne-banked-2026-09-15.png` shows
what it is tracking: **sky, two cumulus clouds, and a horizon running diagonally across the frame**
because the aircraft is banked. **Up to 1,563 distinct colours**, where the 109's dead mirror gave
four.

⚠️ **Read the surface tag before the numbers.** This run dumped two RTT surfaces and the larger one
(`rtt_f79e20`, **256x256**) is the LANDSCAPE render target, not the mirror. Measuring it first gave a
plausible-looking table that was about the wrong buffer entirely. The mirror is the 128x128 one
(`rtt_1df030`), and the table above is from that.

⚠️ **The flight ends on the ground** — `alt=962ft` to `alt=0ft` over the run, with the heading barely
moving (0 → 359), so `BOB_AUTOFLY=bank:200` did not produce a sustained turn before the aircraft flew
into the terrain. **The attitude change measured above is the descent, not a commanded turn**, which
is enough for "the mirror responds" and not enough for "the mirror tracks a roll".

⭐ **What this unblocks.** R3.4 can now compare a LIVE Spitfire mirror against gold — and ASPECT-1 S12
has its harness. **What it still cannot do is compare against the PO's video**, which is a Luftwaffe
campaign and can never show a Spitfire mirror. That request stands.

**S10:** keep the aeroplane flying (the bank tick needs to be early enough, and something must hold
the nose up) and show the horizon line rotating with commanded roll.

**R3.4: 9 sprints (4 in this pass — AT THE CAP, parked).** The mirror is alive, airborne, and
demonstrably responsive; the aircraft that carries it is the one the PO has never filmed.

## ASPECT-1 S12 (Opus 5, 2026-09-15) — ⛔ **the turn still did not happen, and a 47° "horizon rotation" I nearly reported was my estimator fitting noise.** Looking at the picture killed it

S11 named the missing piece as an airborne RAF flight; R3.4 S9 found it (`BOB_QM_INDEX=7`) and noted
the aeroplane flew into the ground before a turn developed. S12 tried to keep it flying.

⭐ **What was added:** the `bank` mode now does what the `dive` mode already knew — full throttle at
tick 20, Ctrl held at 30, and repeated nose-UP trim (Home) before and through the roll, so the
aircraft is powered and trimmed when the aileron arrives.

⛔ **It changed nothing measurable.** The flight still reads `alt=962ft hdg=0` to `alt=1ft hdg=359`
— down to the ground, heading barely moved. The added throttle and trim taps did not hold it up, and
the likeliest reason is the one MA's `BOB_KEYSEQ` documented at S105: **a synthetic trim key carries
no modifier state**, so pushing Ctrl down and then Home does not give the engine the shift-state it
needs — the same trap, in the other port.

⛔⛔ **And the part worth recording is my own near-miss.** I fitted a line to the sky/ground boundary
per column and got a horizon tilt swinging **−31° → +16°**, which looked exactly like a banking
aeroplane, and I was one step from reporting *"the mirror rotates through 47°"*. **Then I looked at
the two frames.** `doc/reference/mirror-spitfire-turn-attempt-2026-09-15.png` and its pair show a
**flat, horizontal band** in both — the mirror is full of cloud and haze, there is no crisp horizon to
fit, and the estimator was measuring noise. **The 47° is withdrawn before it ever became a claim.**

⭐ **What IS supported by these 126 mirror renders:** the content keeps changing (mean 196–205,
sd 13.8–27.1 across the run), consistent with R3.4 S9's finding that the mirror is live. **Nothing
here says anything about roll tracking, in either direction.**

**S13:** fix the trim input first (give the synthetic Ctrl+Home the modifier handling
`BOB_KEYSEQ`'s third field already does in MA), and prove the aeroplane holds altitude and changes
heading — **from the HUD, before any mirror pixel is measured.** A mirror test on an aircraft that is
not turning cannot answer anything, and that is now twice.

**ASPECT-1: 12 sprints (3 in this pass). A failed test, honestly failed, and a wrong number caught by
looking at the image.**

## ASPECT-1 S13 (Opus 5, 2026-09-15) — ⭐ **no synthetic key could ever be HELD: `kb_push` fed only the buffered DI queue, while every modifier is read from the immediate state array.** Fixed. And the BoB gold store is confirmed to contain **no RAF cockpit imagery at all**

**The blocker S12 left.** S11 and S12 both tried to fly a banking turn with `BOB_AUTOFLY=bank`, which
presses **Ctrl (DIK 0x1D) at tick 30 and holds it for the rest of the flight** so that Home (0xC7)
means *nose-up trim*. The trim never took, and S12's "47° horizon rotation" was a line fit to a hazy
flat band that was withdrawn on sight of the frames.

⭐ **Root cause, and it is general.** `kb_push` appends to `g_kbq` — the **buffered** DirectInput
queue drained by `GetDeviceData`. But `DIDEV_GetDeviceState` builds the 256-byte immediate DIK array
**purely from `SDL_GetKeyboardState`**, i.e. the *physical* keyboard:

```c
int n; const Uint8* st=SDL_GetKeyboardState(&n);
for (int sc=0;sc<n;sc++) if (st[sc]) { int dik=sdl_to_dik(sc); if(dik&&dik<256) d[dik]=0x80; }
```

So a synthetic **tap** works (the game sees the buffered event), and a synthetic **hold is invisible**
to every reader of the immediate state — which is exactly how modifiers are checked. Ctrl read as UP
on every frame of every autofly run this item has ever made.

**The fix** is an overlay array that `kb_push` maintains and `GetDeviceState` ORs in:

```c
static unsigned char g_kbSynth[256];
...  if (dik < 256) g_kbSynth[dik] = down ? 0x80 : 0x00;   /* in kb_push */
...  for (int dik=0;dik<256;dik++) if (g_kbSynth[dik]) d[dik]=0x80;   /* in GetDeviceState */
```

Builds clean. This unblocks every autofly mode that holds a key, not just `bank` —
[[no-synthetic-keys-under-wayland]] recorded that xdotool cannot reach the game window, so the env
hook IS the only input path, and half of it was not wired up.

⛔ **And a standing PO question is now answered, negatively.** ASPECT-1 has been waiting on "a gold
capture of the real game flying an RAF fighter". `~/gold standard/bob/` holds **19 PNGs and one
33 MB video**. The video is the Luftwaffe convoy campaign. **All 19 stills are front-end screens** —
options pages (Controls, GFX, Sound) and the campaign phase-selection page; not one is a cockpit.

**So ASPECT-1's mirror can never be pixel-compared against gold**, and no amount of waiting changes
that. The item must be validated on **internal consistency** instead — does the mirror image change
when the aeroplane rolls, and does it change in the right direction — which is what R3.4 S7–S9
already started (mirror live, changing 14–58 per step). **Filed so the next sprint stops budgeting
for a comparison that cannot exist.**

⭐ **By-product for other items:** the gold Controls page (`16-55-52.png`) shows the real game's
input bindings — Stick on "First Joystick Axis 0 & Axis 1", Throttle on "Axis 6", Rudder "Axis 5",
dead zones "Small", mode "Realistic". That is a **directly comparable gold for the options UI**, and
it is sitting unused. Worth its own item.

**ASPECT-1: 13 sprints. The input path is fixed; the gold-comparison premise is dead and the item
needs re-aiming at internal consistency.**

## ASPECT-1 S14 (Opus 5, 2026-09-15) — ⭐⭐ **two more input bugs, both found by S13's fix working**: the autofly tick was a PUMP not a frame (the whole schedule fired in the first instant), and the held Ctrl turned the aileron into **Ctrl+Left** — heading pinned at *exactly* 0 for an entire flight

S13 made a synthetic key HOLD visible to the immediate DIK state for the first time. This sprint flew
it, and the fix promptly exposed the next two faults in the same path.

⛔ **Fault 1: a tick was a PUMP.** The `BOB_AUTOFLY` schedules live in `pump_events()`, which runs far
more often than a frame is presented, so `bank:60` meant 60 **pumps** — a fraction of a second. Every
step (throttle at 20, Ctrl at 30, trim from 30, aileron at 60) fired essentially at once, at the
instant the flight began. **This is precisely MA's KEYHOLD-1 S3** — *"a tick was a PUMP, not a frame:
740 pumps passed between two frames"* — in the sibling port, never fixed here. [[shell-edit-commit-traps]]

**Fixed** with an ungated `g_bob_frames`, incremented in `bob_frame_tick()` at every swap site; the
bank schedule advances only when the frame counter moves. `BOB_AUTOFLY_PUMPTICKS=1` reverts. The
trace now reports both (`from tick 60 (frame 74)`).

⭐ **It works, and the measurement is unambiguous** — the same flight, same mission (`BOB_QM_INDEX=7`,
the airborne Spitfire), pump-paced versus frame-paced:

| | alt over the first five HUD samples |
|---|---|
| pump-paced (S12 behaviour) | 962 → 479 → **0 ft** — straight into the terrain |
| **frame-paced** | 962 → 1014 → 1102 → 1182 → **1205 ft** — a real 243 ft climb |

The nose-up trim had never actually had time to act. R3.4 S9's "962 ft → 0 ft" and S12's repeat of it
were both this, not a flight-model or trim problem.

⛔⛔ **Fault 2, and it is S13's fix biting.** With the climb finally working, the heading stayed at
**exactly 0 for all 14 samples of the flight** — with `AILERON_LEFT` held from tick 60. Cause: Ctrl
goes down at tick 30 and was **never released**, so the aileron press arrived as **Ctrl+Left**, a
different command. Before S13 the held Ctrl was invisible, so `0xCB` was a plain Left — *and the trim
did not work*. The two bugs had been masking each other.

**Releasing Ctrl before the aileron unpins the heading:**

| arm | climb | heading |
|---|---|---|
| Ctrl held throughout | **+243 ft** | **exactly 0, always** |
| Ctrl pulsed per trim | +49 ft | 0 → 358 → 359 → 356 → 358 |
| Ctrl held across trim, released before aileron | +48 ft | 0 → 358 → 359 → 357 → 358 |

⭐ **And the climb column is itself a measurement of S13's mechanism.** A Ctrl down *and* up inside one
pump leaves the immediate-state overlay back at 0, so `GetDeviceState` never sees it — the trim falls
back to the buffered path and the climb drops from 243 ft to ~48 ft. **A modifier must be held across
frames to be read.**

⛔ **What this sprint does NOT establish.** The turn still does not develop: heading moves **at most 4°**
and the aeroplane reaches the ground in every arm. Nose-up *trim* cannot hold this aircraft at 280+
kts; it needs **elevator**, not trim, and/or a start well above 962 ft. **ASPECT-1 still cannot show a
mirror responding to a roll**, and that remains the item's actual goal.

**S15:** replace the trim with a held elevator key and re-measure the heading; only once the HUD shows
altitude held AND heading swinging does a mirror pixel mean anything. (And per S13, there is no RAF
cockpit in the gold store, so the test is internal consistency, not pixel parity.)

**ASPECT-1: 14 sprints. The input path is now correct in three separate respects; the manoeuvre is
not yet.**

## ASPECT-1 S15 (Opus 5, 2026-09-15) — ⭐ **the aeroplane finally MANOEUVRES: a held elevator climbs 1,685 ft and swings the heading 180°, where trim managed 48 ft and 4°** — ⛔ **and not one mirror surface was captured, so there is still nothing to measure**

S14 left the item with a correct input path and an aeroplane that would not turn: nose-up *trim*
under a held Ctrl bought at most 243 ft of climb before the Spitfire mushed into the terrain at
280+ kts, and the heading never moved more than 4°.

⭐ **The fix is to stop trimming and fly it.** `KEYMAPS.H:1253` binds `ELEVATOR_BACK` (pull back =
nose up) to `J_movedown` = `DIK_DOWN` (0xD0). `BOB_AUTOFLY_ELEV=1` holds that key from tick 30 with
**no modifier at all**, so it cannot repeat S14's Ctrl+Left mistake. The trim path is left exactly as
it was, so the two are an A/B rather than a replacement.

**Two runs, same mission (`BOB_QM_INDEX=7`, the airborne Spitfire), read off the HUD:**

| arm | climb | heading | speed |
|---|---|---|---|
| trim, Ctrl held (S14) | +243 ft | **exactly 0, always** | 283 → 256 |
| trim, Ctrl scoped (S14) | +48 ft | 0 → 357 (≤4°) | 283 → 275 |
| **elevator held, run 1** | **+601 ft** | 0 → 353 → 314 → 309 → **297** | 283 → 108 |
| **elevator held, run 2** | **+1,685 ft** (962 → 2,647) | 0 → **233 → 240 → 210 → 176** | 283 → 81 |

**Run 2 swings the heading through roughly 180°** and climbs 1,685 ft. The traces confirm both inputs
landed on their own frames — `ELEVATOR_BACK … from tick 30 (frame 44)` and `AILERON_LEFT … from tick
60 (frame 74)`. The aeroplane stalls (283 → 81 kts) and eventually reaches the ground, which is what
a full held elevator at 70% thrust does with no pilot; it is not a defect and not the point. **The
point is that ASPECT-1 at last has a flight worth measuring a mirror against.**

⛔⛔ **And the mirror was not measured, because none of the dumps is the mirror.** The run wrote
**618 surfaces**. Every one of them is **256×256**, and all 618 carry a single surface tag:

```
618 P6 256 256 255 …      distinct surface tags: 1  (e13080)
```

**256×256 is the LANDSCAPE render target. The mirror is 128×128.** So the count is impressive and the
content is irrelevant — this is the third time this item has come within one step of scoring the
wrong surface, and the only reason it did not is that the sizes were checked before the pixels.
[[gate-frame-must-match-the-eye]]

⚠️ **A harness fault found on the way, worth writing down.** The dump is gated on **`BOB_DUMP_RTT`**;
**`BOB_DUMP_RTT_DIR` only chooses the destination.** Setting the directory alone produces a run that
looks correct, logs nothing unusual and writes zero files. The first S15 flight was lost to exactly
that. *A switch needs its own printed term* — the same lesson the FF harness taught.

**What S16 must establish first, before any more flying:** *does this aircraft have a mirror at all?*
R3.4 S7–S9 found `docreatemirrorno` in **shapes 65 (CPT1) and 172 (CPT4) only**, and **the S15 flight
log contains no mirror trace whatsoever** — no creation, no RTT bind, nothing. Quick mission 7's
Spitfire may simply never build one in this code path. That is a five-minute check against
`FLYINIT.CPP`'s `pit1 shp` mapping and it gates everything else in this item.

**ASPECT-1: 15 sprints. The manoeuvre is solved. The mirror evidence does not exist yet, and the next
sprint is a shape-id check, not a flight.**

---

# NEW GOLD STANDARD VIDEOS (PO, 2026-09-15) — three BoB captures, and one of them REVIVES a dead item

`~/gold standard/bob/` gained three videos tonight. All three are **1920×1080 ~60 fps screen
recordings of the whole desktop**, with the game running in a window — not clean full-screen
captures. That shapes every item below (see the caveat at the end).

| file | length | size |
|---|---|---|
| `260915_bob_raf_campaign.mp4` | 2m 48s | 17 MB |
| `260915_bob_german_campaign.mp4` | 3m 44s | 33 MB |
| `260915_bob_turkey_shoot_german.mp4` | 1m 34s | 23 MB |

## GOLDVID-BOB-1 — ⭐⭐ **the RAF campaign video UNBLOCKS ASPECT-1**, which S13 declared un-gradeable

⛔ **S13 recorded, correctly at the time:** *"the BoB gold store has no RAF cockpit imagery at all —
19 stills, all front-end screens, and one video which is a Luftwaffe campaign. So ASPECT-1's mirror
can never be pixel-compared to gold."* That is now **out of date**. `260915_bob_raf_campaign.mp4` is
RAF, so a Spitfire/Hurricane cockpit — and therefore **a real mirror** — should be on screen.

**First task is to confirm the premise before building anything on it:** step the video and find
frames where (a) the cockpit is an RAF type, and (b) the rear-view mirror is visible. R3.4 S7–S9
established `docreatemirrorno` lives in shapes **65 (CPT1) and 172 (CPT4) only**, so the mirror's
presence is aircraft-dependent even in an RAF sortie. **If no frame shows the mirror, say so and stop
— do not grade a mirror against a video that has none.**

**If the mirror IS visible, this is the acceptance test ASPECT-1 has never had:**
* does the real game's mirror show sky above and terrain below, as ours does (S10)?
* does its content **roll with the aircraft** in a turn? That is the discriminator paint cannot
  fake, and the entire point of S11–S15.
* rough content scale — our mirror changes 14–58 per step (R3.4 S9); what does the gold do?

## GOLDVID-BOB-2 — the German campaign video, a second Luftwaffe reference

`bob_convoy_campaign.mp4` (Aug) was the only campaign reference. A second one at a different date is
worth having for the parts of the front end and the campaign map that S-series work keeps grading by
eye. **Lowest priority of the three** — it is more of the same team we already have.

## GOLDVID-BOB-3 — "turkey shoot", 94 s, and the likely candidate for the FLICKER report

The PO's 2026-09-04 report was *"in bob appImage dogfight the screen flickered baddly"*, and
`tools/bob_vsync_pace.sh` exists because of it. A **turkey shoot is a dogfight**, and this is the
first gold video of one. **Check whether the gold capture flickers.** That decides which way the
defect points:
* gold steady, ours flickers → our present/vsync path, as the gate assumes;
* gold flickers too → it is in the original game and the gate is measuring the wrong thing.
The gate's own header says the control matters more than the fix; this video IS a control.

⚠️ **Caveat that applies to all three, measured not assumed.** These are desktop recordings with the
game windowed, so the game area is a sub-rectangle of the frame and the art is scaled. **Any pixel
comparison must locate the game window first** and must not assume our render resolution. Verified on
the sibling FreeFalcon captures tonight: large HUD digits read cleanly at this scale, small ones do
not. [[gate-frame-must-match-the-eye]]

**Status: 3 items filed, 0 sprints. GOLDVID-BOB-1 is the one that matters — it reopens a question
that was closed as unanswerable.**

## GOLDVID-BOB-1 S1 (Opus 5, 2026-09-15) — ⭐ **the RAF mirror EXISTS and we can see it at last** — ⛔ **and the aeroplane never leaves the ground, so the question the item exists for is still unanswered**

The item's first task was to confirm the premise before building on it. Both halves are now answered,
and they point opposite ways.

⭐ **Confirmed: the RAF cockpit has a mirror, and it is on screen.** At t=115–152 s the video shows a
Spitfire/Hurricane cockpit with a **circular, dark-rimmed mirror at the TOP-RIGHT of the canopy
frame**. R3.4 S7–S9's worry — that `docreatemirrorno` lives in shapes 65/172 only, so an RAF sortie
might still show no mirror — does not bite here. **This is the first sight anyone on this item has
had of the real game's mirror.**

⭐ **Its content is FLAT HORIZONTAL BANDS**, zoomed at 10×: blue sky at the top, a teal/haze band, a
pale horizon band, then tan and brown terrain bands. No detail, no objects — a banded gradient.

**That matters, because it retro-justifies a withdrawal.** ASPECT-1 S12 fitted a per-column line to
our mirror, got a "47° horizon rotation", and withdrew it on looking at the frames, which showed *"a
flat horizontal band in a hazy mirror"*. **The gold shows a flat horizontal band in a hazy mirror
too.** Our appearance is not a defect to be tuned away; it is what the original does.

⛔⛔ **But the aeroplane is PARKED for the entire video.** Every sampled frame from 115 s to 152 s
reads the same, and the game is closed by 160 s of a 168 s recording:

```
5 ft   Hdg 241   Speed 0 Kts
```

Heading wanders 240–242 (a view pan, not a turn), altitude 5 ft, airspeed zero throughout.

**So the discriminator this item exists for — does the mirror's content ROLL WITH THE AIRCRAFT in a
turn — cannot be answered from this video.** A parked aeroplane's mirror shows a level horizon
whether the code is right or wrong. Sprints S11 through S15 built an autofly harness precisely to get
our own Spitfire manoeuvring; the gold capture gives us nothing to compare that against.

⚠️ **The gold reproduces our own S11 failure.** S11 was withdrawn because *"the responsiveness test
could not run — the Spitfire is PARKED, not flying"*. The new gold has the identical problem. Worth
saying plainly rather than filing it as progress. [[parity-captures-must-record-their-state]]

⚠️ **A liveness test I ran and then discarded.** I diffed the mirror disc across five frames at a
fixed pixel box to see whether the gold's mirror updates. The numbers (10–107 mean abs diff) look
decisive and mean nothing: **the view pans between those frames**, so the fixed box stops containing
the mirror and the diff measures camera motion. A control patch of canopy/sky moved 68 by the same
measure. **Whether the gold's mirror is live or a static texture is NOT established.** Testing it
needs frames at a fixed view angle. [[gate-frame-must-match-the-eye]]

⭐ **By-product: the gold build is identified on its own title screen — `BDG 0.99`**, matching
`RUNNING.md`'s standing note. Its menu is Quick Shot / Campaigns / Multi-Player / Load Game / Replay /
PC Config / Sim Config / Credits / Quit / **BDG 0.99**.

**What would unblock this item, and it is one sentence to the PO:** *a capture of an RAF fighter
actually airborne and turning, with the mirror in frame.* 168 s of a parked aeroplane cannot do it,
and no amount of work on our side substitutes.

**GOLDVID-BOB-1: 1 sprint. The mirror is real and looks like ours. The roll question is exactly as
open as it was this morning.**
