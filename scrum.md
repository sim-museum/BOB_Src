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
| R3.9 | **Floating light/dark grey square during campaign dogfight (PO 2026-08-28)** — screenshots `Screenshot From 2026-08-28 21-40-46.png` and `21-44-32.png`. An untextured grey quad appears intermittently in the 3D view. ⚠️ **Likely the same family as the cockpit/mirror RTT work**: an untextured or unbound quad reads as flat grey. Suspects, in order — a render-target surface drawn as geometry (`BOB_DUMP_RTT` dumps each RTT FBO), the `InfiniteStrip` horizon backdrop (known garbage `v` texcoords, R3.4), and a sprite whose texture failed to bind. `BOB_TRACE_RTT` / `BOB_CHECK_SURF` are the existing instruments. <br>◐ **CANARY ADDED (2026-08-29): `BOB_TRACE_GREY`.** A flat grey square IS an untextured quad, and the draw path has TWO ways to produce one, needing different fixes: `t == NULL` → texturing disabled, quad draws in vertex colour; `t && !t->glTex` → texturing ENABLED bound to texture 0 because `upload_texture` produced nothing, so GL samples white (the quiet one). Both are now reported, in `draw_fvf` AND `DEV_DrawIndexedPrimitiveVB`, keyed PER SURFACE with a `TABLE FULL` notice — not per call (a per-call trace in this codebase once wrote 24.7 M lines and starved the run) and never silent when full (that exact silence caused an R3.8 finding to be retracted). <br>⚠️ **First run: 0 untextured 3D draws — and that is NOT evidence of absence.** It was a QUICK-MISSION flight (`BOB_BOOT_FRONTEND=1`, dump at frame 400); the PO saw the square **during a CAMPAIGN dogfight** and says it appears *"sometimes"*. Wrong scenario and a short window. The existing `[texfail] summary: 0 uploads bailed` agrees, which is consistent but equally scoped. **Next: drive the campaign to 3D (the R4.3 loop) with `BOB_TRACE_GREY=1` over a long combat soak, and only then treat silence as meaningful.** | 5 | ☐ |
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
| R7.1 | **DirectPlay → sockets** — session create/join, lobby/readyroom, state sync. (Large; slice into sub-stories at planning.) | 21 | ☐ |
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

**Provenance:** the PO says this shipped in an official BoB patch. Before implementing, check the
patch-level executables/data under the Wine install for the acquisition rule and the exact key —
matching the patch beats inventing our own geometry. Gold-standard behaviour, as always, is what the
real game does, not what seems reasonable.

**Open questions to settle from the patch, not by choice:**
* Is the cell exactly 1/3 x 1/3 of the view, or an angular cone (e.g. ±10 deg)? A grid cell scales
  with FOV; a cone does not. These differ a lot at the FOV extremes.
* "Closest" by slant range, or by angular distance from view centre?
* Enemies only, or any aircraft (the PO says "bogie", which implies unidentified/hostile)?
* Does it re-acquire when the target leaves the cell, or hold until broken?

**Which key:** `S` is not currently in `keymaps.h` as a padlock binding; the acquisition needs its
own `KeyName`/`KeyMap` entry rather than stealing `PADLOCKTOG`'s.

**Done when** pressing the bound key with a bogie in the centre cell locks the NEAREST one, pressing
it with an empty centre cell does nothing observable, and a gate asserts BOTH arms — the empty-cell
no-op is the half that will silently rot.

**Points:** 8
