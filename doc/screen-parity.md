# Screen parity vs the Windows gold standard (Release SP)

Sprint 123 (2026-07-25), updated Sprint 124 (2026-07-26). Gold standard: the PO-supplied
Wine captures of the Windows build (BDG 0.99 patched), `/run/media/admin/BEA6-BBCE/bob/`
— **19 PNGs** dated 2026-06-24 (the backlog says 17; two are near-duplicate side-select
shots — flagged for the PO below). Native captures land in `doc/parity/` (1024x768 GDI
framebuffer / 800x600 GL dumps, this repo).

**S124 (2026-07-26): the S123 root-cause 1 (resource-version delta) is CLOSED.** The
native build now reads the INSTALLED build's PE resources (`English/TEXT/boblang.dll`
= the BDG 0.99 data, per the SM oracle ruling): DIALOG rects, DLGINIT captions with
faithful IDS→string-table resolution, template-driven label-static hosting, and a
template-membership draw filter. Every config screen's label set/rows/order now comes
from the BDG oracle. `BOB_NO_PE_RSRC` reverts to the S123 source-.rc behaviour.
Fresh captures: `native-*-2026-07-26.png` (verdicts updated below).

**S126 (2026-07-27): the sequential property-stream reader LANDED** (the item 3 "honest
full fix" below): every hosted R\* control's genuine `DoPropExchange` now replays its
persisted DLGINIT property stream (real `CPropExchange` reader in `afxwin.h`; stream
layout validated against all 1280 R\*-class bags). Design-time FontNums, fore/shadow/
back colors, ResourceNumbers, columns now load — screens picked up gold's authored
colors exactly (phase-select date `(183,250,255)` = a pixel-exact color match with the
full-res gold shot; Controls' cyan row labels; QS yellow combos + cyan description).
The #16 duplicate date is settled by a covered-static erase emulation of Windows'
dirty-region repaint (a static ≥90% covered by a sibling hosted listbox is skipped —
the settled Windows state). Gates: `BOB_NO_PROP_STREAM` reverts to the S125 spot-fix
decode, `BOB_NO_DLGINIT_PROPS` the whole design-prop layer, `BOB_NO_COVER_ERASE` the
erase emulation. **New acceptance bar (adopted from MA note 16): a headless SDL-dummy
capture must be byte-identical (`cmp`) to a real-GL capture of the same recipe — PASSES
on mainmenu (first try)** — catches the environment-dependent uninitialized-PX class
without a display. Fresh captures: `native-*-2026-07-27.png` (13 changed screens).

**Capture harness (SP.1, this sprint):** `BOB_SHOT=<n>` + `BOB_SHOT_PATH=<file>` —
deterministic one-shot capture: after n front-end ticks (or map paints) the GDI
framebuffer is dumped and the process exits. Works headless (`SDL_VIDEODRIVER=dummy`).
`BOB_CONFIGSCREEN` gained `game`/`mission`/`views`/`flight`/`quick` targets. Flight
frames use the existing `BOB_DUMP_FRAME`/`BOB_DUMP_PATH`/`BOB_EXIT_AFTER_DUMP`
(real GL, `:0`, display-lock protocol). Common env prefix for every recipe below:

```
cd "<drive_c>/Program Files/Rowan Software/Battle Of Britain"
E: BOB_RUN_INIT=1 BOB_DRIVE_C=<drive_c> BOB_FRONTEND=1 BOB_OLE_DRAW=1 [SDL_VIDEODRIVER=dummy]
```

**Side-by-side composites** (gold left, native right — adopted from the Julia Racer QA note
`doc/QA_METHOD_GOLD_PARITY_from-julia-racer.md`, its pt 6): `doc/parity/sbs-*.jpg` for the seven
key screens (mainmenu, config-gfx, quickshots, phaseselect, strategic-map, cockpit, sim-views).

## Verdict key
- **MATCH** — layout + art + text agree (resolution/AA differences ignored)
- **CLOSE** — same screen, same structure; named cosmetic deviations
- **PARTIAL** — screen reachable/renders but with structural deviations
- **GAP** — not natively reachable/capturable yet

## Per-shot inventory (gold timestamp order)

| # | Gold shot (16-xx-xx) | Screen | Native recipe (env beyond E) | Native capture | Verdict |
|---|---|---|---|---|---|
| 1 | 47-32 | Main menu (`title`) | `BOB_SHOT=40` | `native-mainmenu-2026-07-25.png` | **CLOSE** — art + menu placement match (menu now anchored at the game's own ListX/ListY=210,220). Deviations: last item "Website" vs gold "BDG 0.99" (BDG-patched string table); our stencil font vs gold's rounded gold face (R6.2 font pass). |
| 2 | 47-38 | Quick Shots select (`quickview`/CSQuick1, Scenario page) | `BOB_STARTFLYING=click BOB_AUTOCLICK=0 BOB_SHOT=220` | `native-quickshots-2026-08-02.png` | **PARTIAL → CLOSE (S128: PAGE TABS now render)** — background montage, mission combos, description (word-wrapped, S127), Back/Fly present, AND the page-tab row now draws: **✓Scenario / Parameters / Luftwaffe / RAF** with their selection-tick / radio icons (S128 hosts the genuine `CRRadioCtrl` for IDC_RRADIO — the tab captions come from its own OnDraw; `bob_make_rradio`). Remaining deviations: font face (R6.2); clicking a tab to switch pages (#3) needs the RRadio click→`OnSelectedRradio`→`QuickMissionParameters` page-switch wiring (distinct open item — see root cause 4). |
| 3 | 47-45 | Quick Shots — **player-flight editor** (Squadron/Aircraft/Duty/Callsign, name box, "Return to Player") | reached by a player-line click (`CSQuickLine`/`QuickParameters`, SQUICKUN.CPP) — not scripted yet | — | **GAP — mapping CORRECTED (S129).** This gold shot is NOT the "Parameters" page tab: it has no tab row and a "Return to Player" button — it is the per-flight player editor (Squadron/Aircraft/Duty/Callsign), reached by clicking the player's flight line, not by the tab. **S129 built the tab-navigation it was assumed to need** (RRadio click → `OnSelectedRradio` → `LaunchDial` page switch) and the **Parameters _tab_ (mission params: Target Area / T.D. / Weather / Time / Name) now renders and is reachable by a genuine click** — a real new QS page, just not this gold shot. Remaining for #3: reach the `CSQuickLine` player-flight editor + render its Squadron/Aircraft/Duty/Callsign combos. **S130 spike: that screen is the QS order-of-battle** (`QuickMissionBlue`/`Red`, reached via the RAF/Luftwaffe tabs — clickable since S129) and **SIGSEGVs** — root-caused to a ternary copy-from-`*(DialBox*)NULL` for inactive flight slots (`fullpane.cpp:215`; benign-on-MSVC/faults-on-GCC UB; shared-doc §8q). Fix needs a game-code UB-exception in the variadic panel builders (deferred). |
| 4 | 47-57 | "Initialising 3D" loading screen (Dover art + red progress) | transient (drawn inside `StartFlying`→`Launch3d`) | — | **GAP (by design)** — our Launch3d bridge feeds the `Start3d` paint bits directly, so the progress screen never paints headlessly. Low value: transient screen. |
| 5 | 48-09 | In-flight cockpit (Spitfire Mk I, on runway) | `BOB_BOOT_FRONTEND=1 BOB_DUMP_FRAME=150 BOB_DUMP_PATH=<p> BOB_EXIT_AFTER_DUMP=1` on `:0` under the display lock | `native-cockpit-2026-07-25.png` | **CLOSE** — cockpit frame, gunsight, instrument panel, HUD info line ("4ft Hdg 242 Speed 0Kts" matches gold's readout), mirror, clouds. Deviations: prop rendered as a static dark blade vs gold's blur disc; our white HUD bar + Tower ATC line vs gold's red-only info line; 800x600 vs gold ~1830x1080. |
| 6 | 55-25 | PC Config — More GFX (`options3d2`) | `BOB_CONFIGSCREEN=gfx2 BOB_SHOT=70` | `native-config-moregfx-2026-07-26.png` (sbs: `sbs-config-moregfx.jpg`) | **CLOSE (S124: label-for-label gold)** — all 12 rows labeled exactly as gold: Filtering/Smoke Effects/Texture Quality/**Town and forest raises**/Routes/A-C Shadows/Item Shadows/Horizon Distance/**Detail Level**/G Effects/Injury Effects/White Outs (BDG captions via PE DLGINIT + IDS string-table resolution). Deviations: combo values (settings state), one stray box artifact at G Effects, tab-row spread, font face. |
| 7 | 55-41 | PC Config — GFX (`options3d`) | `BOB_CONFIGSCREEN=gfx BOB_SHOT=70` | `native-config-gfx-2026-07-26.png` | **CLOSE (S124: BDG row set + captions)** — gold's row set incl. the BDG-added **3D Resolution / Campaign Resolution / Map Screen** rows, captions exact (**Gamma Level**, **Weather Detail**). Deviations: Campaign Resolution / Map Screen rows are label-only (the 2000 game code has no combo member to bind — BDG-code delta, unfixable data-side), our "BoB Linux OpenGL backend" driver string (faithful for the port), tab-row spread, font. |
| 8 | 55-52 | PC Config — Controls (`controloptions`/SController) | `BOB_CONFIGSCREEN=control BOB_SHOT=70` | `native-config-controls-2026-07-27.png` | **CLOSE (S127: "&&" escape fixed)** — device line, axes/buttons count, Use For FF / Gun Fire / Buffet / Aerodynamic / Airframe rows, Dead Zone / Mode / Flip headers, and the Stick/Rudder/Throttle/Prop Pitch/View Pan/Zoom/**Cockpit & UI**/Gunner row labels all render. The BDG label "Cockpit && UI" now renders "Cockpit & UI" (Windows '&' accelerator-prefix processing in `CDC::DrawText`, S127; combo device names' literal '&' untouched — they draw via ExtTextOut; `BOB_NO_AMP_ESCAPE` reverts). Deviations: a few mid-grid rows unlabeled, Calibrate/checkbox art not drawn, font face. |
| 9 | 55-59 | PC Config — Sound (`soundoptions`) | `BOB_CONFIGSCREEN=sound BOB_SHOT=70` | `native-config-sound-2026-07-26.png` | **CLOSE (S124: structurally 1:1 with gold)** — Sound Driver header + wide driver combo + all 7 labeled rows in gold order (3d SFX Volume/SFX Processing/UI SFX Vol/Ambient SFX Vol/Radio Chatter Volume/Engine Volume/SFX Quality). The overlapped top label + stray top/bottom combos are GONE (template filter: BDG dropped the source's music combos). Deviations: combo values (settings state), "Default" vs gold "Primary Sound Driver" (faithful port string), font. |
| 10 | 56-15 | Sim Config — Flight (`flightoptions`) | `BOB_CONFIGSCREEN=flight BOB_SHOT=70` (`sim` also works) | `native-sim-flight-2026-07-26.png` | **CLOSE (S124: row-for-row gold)** — FLIGHT OPTIONS header row + all 10 rows labeled incl. the previously-missing Engine Management / Prop Pitch Control and gold's **109 Fuel Capacity** (BDG). Deviations: combo values (settings state), font. |
| 11 | 56-21 | Sim Config — Game (`gameoptions`) | `BOB_CONFIGSCREEN=game BOB_SHOT=70` | `native-sim-game-2026-07-26.png` | **CLOSE (S124)** — all 10 rows labeled+paired (Weapons/Vulnerable To Fire/Ground Collisions/Midair Collisions/Complex A.I. Pilots/Accel Off/Target Size/Auto Canopy/Text Info/**Aircraft Names**). Deviations: combo values (settings state), font. |
| 12 | 56-30 | Sim Config — Mission (`missionoptions`) | `BOB_CONFIGSCREEN=mission BOB_SHOT=70` | `native-sim-mission-2026-07-26.png` (sbs: `sbs-sim-mission.jpg`) | **CLOSE (S124 proof screen — labels/rows match gold)** — all 6 label+combo rows exactly as gold (LW Skill Modifier/RAF Skill Modifier/Luftwaffe Tactics/Luftwaffe Intell/Map Plotting/Auto Vectoring; values match bar RAF Skill = settings state). Root cause was NOT a resource delta alone: SMissionConfigure DDX-binds no statics — the S124 template-driven static hosting creates them from the PE DIALOG template. Deviations: tab-row spread, font. |
| 13 | 56-45 | Sim Config — Views (`vieweroptions`) | `BOB_CONFIGSCREEN=views BOB_SHOT=70` | `native-sim-views-2026-07-26.png` | **CLOSE (S124)** — 9 rows labeled+paired with gold captions (Auto External, View Mode Select, Padlock When Visible, Info Line, Head Up Display …); the S123 overlapped Info Line/Camera Colour pair is gone (template filter). Deviations: combo values, font. |
| 14 | 56-53 | Campaign side-select (RAF/Luftwaffe) | `BOB_AUTOCLICK=1 BOB_SHOT=250` | `native-campaign-sideselect-2026-07-25.png` | **CLOSE** — art identical, RAF/Luftwaffe/Back hit-polygons live. Deviation: captions tiny vs gold's large gold face (the screen has no textlist coords; captions are art-adjacent). |
| 15 | 57-36 | Campaign side-select (near-duplicate of #14) | same | same | same — **flag for PO: #14/#15 duplicates** (19 vs "17" count). |
| 16 | 57-47 | Campaign phase select (`campaignselect`) | `BOB_AUTOCLICK=1,1 BOB_SHOT=380` | `native-campaign-phaseselect-2026-07-27b.png` | **CLOSE (S127: description now WORD-WRAPS — the multi-line phase blurb wraps within its box, paragraph break preserved, instead of one clipped line off the right edge; `BOB_NO_WORDWRAP` reverts). S126: duplicate date GONE — covered-static erase emulation; date color pixel-exact vs gold `(183,250,255)`; tabs gold-faced from the persisted stream. Remaining: tab face size (R6.2)**. _S125: tab row spreads as gold._ Earlier history: **PARTIAL → improved (S125: tab row spreads as gold)** — the phase tabs now spread across the full row with gold's exact column layout: the DLGINIT bag persists the listbox's authored columns (`A0..A3`=180px, `C2/C3`=right-aligned) which the empty-bag host boot had lost; Convoys/Eagle Attack left-aligned, Critical Period/Blitz right-aligned to their column ends, matching gold's spacing (`BOB_NO_DLGINIT_PROPS` reverts). Remaining deviations: duplicated date heading top-left (RStatic `IDC_RSTATICDATE` 1227 — in BDG's template WS_VISIBLE with a design caption resolving to the date; on Windows it is covered by the tab-row listbox's background re-blit after the first repaint and never invalidated, while our panel redraws every control every frame — paint-model story, deferred), description unwrapped (R6.2), tab font small vs gold's large face (R6.2). |
| 17 | 57-55 | Campaign enter-name (`campaignentername`) | `BOB_AUTOCLICK=1,1,1 BOB_SHOT=520` | `native-campaign-entername-2026-07-27.png` | **CLOSE (S126: date/labels in gold's large gold face + authored colors from the persisted stream; S125: REdit hosted + gold line layout)** — the name edit is the genuine hosted `CREditCtrl` (S125 REdit host: caption/word-list machinery live, blocking-keys + `CommsPlayerName` reach it), and the line layout now matches gold structurally: "Commander Bob" adjacent (IDC_ROLE right-aligned per its persisted alignment byte), "Luftwaffe <phase>" adjacent (IDC_SIDE right / IDC_PERIOD left), date centred. Phase/date strings differ only by selected phase (state). Deviations: caret line not drawn in the capture (flash-timer state), font face (R6.2), typed-input not yet harness-verified headlessly (no key-injection harness; deferred with the caret check). |
| 18 | 58-08 | Strategic map + LW Directives dialog | directives dialog not natively reachable yet | — | **GAP** — the OOB info-dialog subsystem renders Bases etc. (S113-S117); the LW Directives dialog is a toolbar dialog not yet wired. Map behind it: see #19. |
| 19 | 58-19 | Strategic map (LW campaign, raids/routes) | `BOB_AUTOCLICK=1,1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=700..1200` | `native-strategic-map-2026-07-25.png` | **CLOSE** — terrain, sectors A–E/Y/Z, city labels, No.11 Group, full unit-icon layer (green RAF / blue / yellow), footer event log + date-clock ("10 July 11:54 x1"), both LW toolbar rows, right ruler. Deviations: no raid stacks/route lines in the capture (fresh Convoys day, paused-start; gold is 12 August Eagle Attack at x300 with live raids), ruler band plain vs gold's wooden art, accel transport buttons not in shot. |

## Systemic root causes (ranked, for SP.2/SP.3 continuation)

1. **Resource-version delta — ☑ CLOSED S124 (2026-07-26).** SM oracle ruling (PO can
   overturn): parity is judged against the gold shots as-is = the **BDG 0.99 patched
   build**. The native build now reads the installed build's PE resources at runtime
   (`bob_resources.cpp` DIALOG/DLGINIT enumerators + `bob_dlgtemplate.cpp` PE-first
   load): BDG rects/rows, DLGINIT captions with the genuine control's IDS→string-table
   resolution (`WM_GETSTRING` equivalent), template-driven hosting of non-DDX label
   statics, and a template-membership draw filter (source-only controls that BDG's
   templates drop are not drawn — the Windows dialog manager would never create them).
   `BOB_NO_PE_RSRC` reverts. Also closes the packaging resource-root blocker for
   dialogs (the .rc text parse remains only as fallback). Residual BDG-**code** deltas
   (not fixable from data): title item "BDG 0.99" vs "Website" (FULLPSYS string list),
   "BDG" config tab, combo members the 2000 source never binds (Campaign Resolution /
   Map Screen rows are label-only).
2. **Fixed S123:** unscoped control-rect lookup (labels took rects from other
   dialogs' templates — scrambled forms); menu lists ignoring the game's per-resolution
   `ListX/ListY` anchor (Back/Begin/Fly rows drawn top instead of bottom); runtime
   `ShowWindow` visibility ignored (demo/disabled ghost statics drawn).
3. **DLGINIT design-prop loss — ◐ two slices closed S125 (2026-07-26).** The genuine
   controls load design-time layout props in `DoPropExchange` from the persisted DLGINIT
   property stream; our hosts boot from an EMPTY `CPropExchange`, losing them. Landed
   (`BOB_NO_DLGINIT_PROPS` reverts): **(a)** RListBox column widths/aligns (`A0..A8`
   shorts + `C0..C8` longs, the last 54 bytes of a version&4 bag) — fixed #16's tab-row
   spread; **(b)** RButton `m_alignment` (packed in the persisted ResourceNumber's bits
   24..31, anchored after the design caption; applied to artless caption buttons only)
   — fixed #17's scattered lines. Remaining in this class (S126 candidate: a real
   sequential property-stream reader feeding each host's genuine `DoPropExchange`):
   FontNum/FontNum2 (gold's large tab/heading faces), fore/shadow colors, RStatic
   numeric ResourceNumber (the faithful caption-resolution path — would settle #16's
   duplicate date), REdit props.
4. **Remaining render classes:** ~~multi-line word-wrap for description statics (R6.2)~~
   **☑ CLOSED S127** (`CDC::DrawText` DT_WORDBREAK; only multi-line-height boxes wrap so
   single-line config labels are untouched; `BOB_NO_WORDWRAP` reverts — #2 QS + #16 phase
   descriptions wrap); ~~accelerator-escape "&&"→"&" in label draw (#8)~~ **☑ CLOSED S127**
   (Windows '&' prefix processing in the static `DrawText` path, DT_NOPREFIX-aware; combos
   keep literal '&' via ExtTextOut; `BOB_NO_AMP_ESCAPE` reverts). Still open: page-switching
   dialogs (`MoveWindow` offsets not tracked — the remaining half of #3, now that S128
   draws the QS page tabs); native-DLU font face/size pass (R6.2); dirty-region repaint
   model (S126 covered-static erase covers #16); LW Directives dialog wiring (#18);
   headless key-injection harness (#17 typed-input + caret verification).
6. **QS page-tab captions — ☑ CLOSED S128.** The Quick-Shots page tabs (Scenario/
   Parameters/Luftwaffe/RAF, `IDC_RRADIO`) are a `CRRadioCtrl` — a 6th hosted R\* control
   type (`bob_ole_rradio.cpp`, factory CLSID `5363BA22-…`). The genuine control's OnDraw
   draws the tab captions + per-tab selection tick (MaskIcon) onto the panel. Same hosting
   pattern as RButton/REdit; one compile-compat fix in the genuine RRADIOC.CPP (MaskIcon
   temp-`CPoint&` bind, mirrors the existing RBUTTONC.CPP `_mip00` fix).
5. **Parent-rect clipping (MA note 17 mechanism #2) — assessed N/A for BoB (S127).** MA
   clips template controls parked fully outside the dialog client rect. BoB's dead controls
   are already removed by the S124 template-membership filter, and no out-of-client-rect
   stray draws across the 14-screen headless sweep — no current BoB symptom; adopt the
   distinct filter if one surfaces.

## PO questions

- Gold folder holds **19** shots, not 17; #14/#15 are near-duplicates of the same screen.
  Confirm the canonical set (inventory above covers all 19).
- ~~Resource-version delta (root cause 1): which oracle?~~ **Resolved by SM ruling S124
  (standing approval): oracle = the BDG 0.99 build (the gold shots as-is); implemented.**
  PO can overturn cheaply: `BOB_NO_PE_RSRC=1` reverts the whole PE layer to the 2000
  source-.rc data, and every BDG-vs-source delta above is tagged per-deviation.
