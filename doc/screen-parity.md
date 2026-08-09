# Screen parity vs the Windows gold standard (Release SP)

> **Standing tally after S141 (2026-08-08): 18 CLOSE / 0 PARTIAL / 1 GAP of 19.** The one GAP
> (#4, the transient "Initialising 3D" loading screen) is **by design**. S141 closed #18 by making
> the campaign **phase** selectable — the hosted-listbox `Select(row, column)` event had its column
> hardcoded to 0, so every campaign started in phase 0 (Convoys) and the LW Directives allocation
> grid, which only has content on an active day, looked like a render gap. It also corrected a
> silent STATE mismatch on #16 (gold is the Eagle Attack phase; every native capture had been
> Convoys). Open named deviations are now control-art only: `CRSpinBut` unhosted (SP.4), no headless
> dialog-dismiss (SP.5, blocks #19's raid-stack check), listbox selection state not mirrored (SP.6).

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

**S131 (2026-08-02): the pervasive "font FACE" deviation is CLOSED** (MA note 26 §2).
`bob_gdi_font` rendered every face in the one art TTF (Intel.ttf), so data/label rows
drew in the Rowan art face instead of Arial. Now a per-FACE registry routes the game's
requested face + italic to its own metric-compatible TTF (Intel/FC-Glamour/Fusion→ART;
Arial→LiberationSans; Times→LiberationSerif; Courier→LiberationMono; italic→the italic
variant, matching gold's italic combo values). Threaded through the DC's selected `CFont`
(`CFont::bobFaceKind` classifies the CreateFont face name). Config/campaign screens
(#6–#13, #16, #17) now render data/labels in Arial (values italic) = gold's scheme; the
ART screens (title menu, headers) are `cmp`-verified byte-identical (`BOB_NO_FONTFACE`
reverts). MA note 26 §1 (Japanese-branch) N/A for BoB; §3 (combo fill) already handled;
MA note 27 (listbox-fill is load-bearing) heeded. Fresh captures: `native-config-*-2026-08-02.png`.

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
| 2 | 47-38 | Quick Shots select (`quickview`/CSQuick1, Scenario page) | `BOB_STARTFLYING=click BOB_AUTOCLICK=0 BOB_SHOT=220` | `native-quickshots-2026-08-02.png` | **PARTIAL → CLOSE (S128: PAGE TABS now render)** — background montage, mission combos, description (word-wrapped, S127), Back/Fly present, AND the page-tab row now draws: **✓Scenario / Parameters / Luftwaffe / RAF** with their selection-tick / radio icons (S128 hosts the genuine `CRRadioCtrl` for IDC_RRADIO — the tab captions come from its own OnDraw; `bob_make_rradio`). Remaining deviations: font face (R6.2); clicking a tab to switch pages (#3) needs the RRadio click→`OnSelectedRradio`→`QuickMissionParameters` page-switch wiring (distinct open item — see root cause 4). _(S139: the footer's "Fly" was clipped off — the same listbox-clip bug fixed for #3 — now renders, so the footer is genuinely Back/Fly per gold.)_ |
| 3 | 47-45 | Mission **briefing** (`IDD_BOBFRAG`/`BoBFrag`) — roster (Unit/Aircraft/Duty/Callsign), name box, "Return to Player", Back/Sim Config/Fly | `BOB_BOBFRAG=1 BOB_SHOT=120` (S135 scaffold: QS pre-flight → `quickmission` → `LaunchScreen(&bobfrag)`) | `native-quickshots-bobfrag-2026-08-02.png` | **CLOSE (S140: all elements now render).** Every gold-#3 element present: the crashed-Bf109 + pink-cloud + He111 background, the roster listbox (Unit/Aircraft/Duty/Callsign → 54 Squadron/Spitfire/Patrol/Trumpet), the **"Return to Player"** button (S136), the **"Bob" pilot name box** (S140 — hosted the 7th R\* control type `CREdtBtCtrl`, BoBFrag's `IDC_PILOT_*` slots; `SetCaption(playerslotname)` → the player's "Bob"), and the **Back / Sim Config / Fly** footer (S139). Deviations: the Return-to-Player button art is the tickbox-style icon vs gold's rounded bezel; font face (R6.2). _History:_ This gold shot is NOT the "Parameters" page tab: it has no tab row and a "Return to Player" button — it is the per-flight player editor (Squadron/Aircraft/Duty/Callsign), reached by clicking the player's flight line, not by the tab. **S129 built the tab-navigation it was assumed to need** (RRadio click → `OnSelectedRradio` → `LaunchDial` page switch) and the **Parameters _tab_ (mission params: Target Area / T.D. / Weather / Time / Name) now renders and is reachable by a genuine click** — a real new QS page, just not this gold shot. Remaining for #3: reach the `CSQuickLine` player-flight editor + render its Squadron/Aircraft/Duty/Callsign combos. **S130 spike: that screen is the QS order-of-battle** (`QuickMissionBlue`/`Red`, reached via the RAF/Luftwaffe tabs — clickable since S129) and SIGSEGV'd on a ternary copy-from-`*(DialBox*)NULL` for inactive flight slots (benign-on-MSVC/faults-on-GCC UB; shared-doc §8q). **S132: the crash is FIXED** — a null-reference-safe `DialBox` copy ctor (RDIALOG.H, BOB_LINUX) makes an inactive slot an empty leaf DialBox instead of derefing null; `AddChildren` already renders a null-dial child as an empty `RDEmptyP`. The RAF/Luftwaffe tabs now load without crashing (tabs render, RAF-selected). **S133: the flight-line content now RENDERS** — `bob_fp_draw_nested` (FULLPSYS.CPP, BOB_LINUX, default-on, `BOB_NO_QS_NESTED` reverts) walks the panel's nested `RDialog` tree and draws each `CSQuickLine` row's hosted controls (the compat draw walk previously reached only `parentDlg==pdial[d]`), synthesizing the row stacking because the game's layout gives no real rects headlessly (probe `BOB_TRACE_OOBTREE`). The RAF tab now shows the player flight row: piloted-flag icon + **Patrol / Altitude / Skill** headers over **Spitfire IA / 1 / Veteran** combos (`doc/parity/native-quickshots-oob-raf-nested-2026-08-02.png`). **Remaining for #3 (still GAP):** this renders the OOB **list**. **S134 mapping re-correction:** gold #3's "Return to Player" is `IDC_RETURNTOPLAYER`, which lives in **`IDD_BOBFRAG`** (`class BoBFrag`, BOBFRAG.CPP) — the mission **briefing** screen (pilot roster + Squadron/Aircraft/Duty/Callsign + formation callsign buttons + Back/Sim Config/Fly footer), NOT a QS `CSQuickLine`/`QuickParameters` sub-editor. It is reached via the **mission-fly / campaign-intercept flow** (`{IDS_FLY,&bobfrag,CheckForMissingMission}` FPLAYOUT.CPP:1408; or the `BOB_CAMPAIGN_FLY` map→frag seam) — *not* the QS Scenario-page "Fly", which is `{IDS_FLY,&quickmissionflight,FragFly2}` → straight to flight (hangs under the SDL-dummy capture driver, no GL). So the S133 "flight-line click → editor" remaining-step was itself wrong: gold #3 is the BoBFrag briefing, and the next sprint is **reach BoBFrag headlessly + render its roster** (likely leveraging the S133 nested-panel draw + the RButton callsign buttons). (Single-flight OOB rows validated; multi-row stacking synthesized, not yet captured with >1 flight.) **S135: the BoBFrag briefing now RENDERS** via the `BOB_BOBFRAG` scaffold (reliable reach: QS click-mode pre-flight seeds `quickdef` → `LaunchScreen(&quickmission)` inits CSQuick1 → `LaunchScreen(&bobfrag)`; stops at the briefing, no flight). Matches gold #3 closely: the crashed-109 + pink-cloud + two-He111 background, the roster **listbox** (`CRListBoxCtrl` id=1481: **Unit / Aircraft / Duty / Callsign → 54 Squadron / Spitfire / Patrol / Trumpet**), and the **Back / Sim Config** footer, exit 0. **Remaining deviations (→ still PARTIAL, was GAP):** (1) the "Return to Player" button (`IDC_RETURNTOPLAYER`) isn't drawn — it's an RButton, which BoB doesn't host in the front-end (buttons render via the separate menu path), so front-end RButton hosting is the follow-on; (2) the pilot **name edit** (`CREditCtrl` id=1923) is created but not drawn (template/rect filter); (3) the **Fly** footer item isn't shown (only Back/Sim Config — likely a `CheckForMissingMission`/playersquadron gate). Header reads "Unit" vs gold's cropped "…quadron". **S136: the "Return to Player" button now RENDERS** — `IDC_RETURNTOPLAYER` (2146) is a template-only button no DDX binds, so it was never created; extended the S124 template-driven hosting to non-DDX **buttons** (`bob_dlg_enum_buttons` + `bob_make_rbutton`, `BOB_NO_TEMPLATE_BUTTONS` reverts) → the button now draws top-left with its caption, matching gold #3's key element (its art is the tickbox-style icon vs gold's rounded bezel — minor). All config/QS/mainmenu/phase screens **byte-identical** on/off (the hosting is inert where no non-DDX template button draws). **Still PARTIAL** — remaining: the "Bob" pilot **name box** (a `CREdtBt` pilot slot, not yet front-end-hosted). **S139: the "Fly" footer item now RENDERS** — the footer `CRListBoxCtrl` lays its columns at its own internal widths but `ExtTextOut`-clips each to the passed `rcBounds` width, and `bob_draw_menu`'s tight `total` (re-measured text widths) clipped the 3rd column off. Widened the listbox clip to the remaining screen width (`bob_draw_menu`; column positions are internal + hit-rects come from `wids[]`, so nothing moves — previously-clipped columns just appear; `BOB_NO_FOOTER_CLIP` reverts). The footer now reads **Back / Sim Config / Fly** = gold. Only the name box remains. |
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
| 16 | 57-47 | Campaign phase select (`campaignselect`) | `BOB_AUTOCLICK=1,1,#1000:1 BOB_SHOT=520` (S141 — gold #16 is the **Eagle Attack** phase, so the recipe now selects it; the old `1,1 BOB_SHOT=380` captures the Convoys default) | `native-campaign-phaseselect-eagle-2026-08-08.png` (sbs: `sbs-phaseselect.jpg`) | **CLOSE — and S141 fixes a STATE mismatch that had gone unnoticed: gold #16 shows the *Eagle Attack* phase selected, while every native capture until now showed Convoys** (the port could not select a phase — see #18/S141: the hosted-listbox `Select(row,column)` event passed a hardcoded column 0). The native capture now matches gold's state: heading **"12th August - 23rd August"**, the Eagle Attack narrative, same paragraph break, same wrap. **New named deviation (was invisible while both were phase 0):** gold draws the *selected* tab in WHITE ("Eagle Attack") and the rest gold; ours leaves all four gold, because our synthesized click fires the genuine Select event but does not set the control's own `m_iRowSel`/`m_iColSel` the way `CRListBoxCtrl::OnLButtonDown` would — a small, well-scoped follow-on (mirror the genuine handler's selection state in the host click path). The black video panel is present in **gold too** (the Smacker intro isn't playing there either). Prior verdict text: **CLOSE (S127: description now WORD-WRAPS — the multi-line phase blurb wraps within its box, paragraph break preserved, instead of one clipped line off the right edge; `BOB_NO_WORDWRAP` reverts). S126: duplicate date GONE — covered-static erase emulation; date color pixel-exact vs gold `(183,250,255)`; tabs gold-faced from the persisted stream. Remaining: tab face size (R6.2)**. _S125: tab row spreads as gold._ Earlier history: **PARTIAL → improved (S125: tab row spreads as gold)** — the phase tabs now spread across the full row with gold's exact column layout: the DLGINIT bag persists the listbox's authored columns (`A0..A3`=180px, `C2/C3`=right-aligned) which the empty-bag host boot had lost; Convoys/Eagle Attack left-aligned, Critical Period/Blitz right-aligned to their column ends, matching gold's spacing (`BOB_NO_DLGINIT_PROPS` reverts). Remaining deviations: duplicated date heading top-left (RStatic `IDC_RSTATICDATE` 1227 — in BDG's template WS_VISIBLE with a design caption resolving to the date; on Windows it is covered by the tab-row listbox's background re-blit after the first repaint and never invalidated, while our panel redraws every control every frame — paint-model story, deferred), description unwrapped (R6.2), tab font small vs gold's large face (R6.2). |
| 17 | 57-55 | Campaign enter-name (`campaignentername`) | `BOB_AUTOCLICK=1,1,1 BOB_SHOT=520` | `native-campaign-entername-2026-07-27.png` | **CLOSE (S126: date/labels in gold's large gold face + authored colors from the persisted stream; S125: REdit hosted + gold line layout)** — the name edit is the genuine hosted `CREditCtrl` (S125 REdit host: caption/word-list machinery live, blocking-keys + `CommsPlayerName` reach it), and the line layout now matches gold structurally: "Commander Bob" adjacent (IDC_ROLE right-aligned per its persisted alignment byte), "Luftwaffe <phase>" adjacent (IDC_SIDE right / IDC_PERIOD left), date centred. Phase/date strings differ only by selected phase (state). Deviations: caret line not drawn in the capture (flash-timer state), font face (R6.2), typed-input not yet harness-verified headlessly (no key-injection harness; deferred with the caret check). |
| 18 | 58-08 | Strategic map + LW Directives dialog | `BOB_AUTOCLICK=1,1,#1000:1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=1100` (S141: `#1000:1` selects the **Eagle Attack** phase; the game then opens Directives itself — no `BOB_MAP_DIRECTIVES`) | `native-strategic-directives-eagle-2026-08-08.png` (sbs: `sbs-strategic-directives.jpg`) | **PARTIAL → CLOSE (S141: the allocation GRID renders).** S137's state-gap diagnosis was right: the grid was hidden because the campaign always started in phase 0 (Convoys, standby), never an active phase. S141 makes the campaign **phase** selectable headlessly — the phase tabs are the *columns* of one `CRListBoxCtrl` (`IDC_RLIST_CAMPAIGNS`) and `CSCampaign::OnSelectRlistCampaigns(row,column)` switches phase on the COLUMN, which our hosted-listbox click hardcoded to 0. With column resolution (`GetColFromX`) the genuine event fires with column 1 → `ChangeCamp(1)` → 12 Aug Eagle Attack. On that day **the game opens the Directives dialog by itself** (the S137 scaffold is no longer needed) and the full grid draws through S137's deep TB_MISC walk: Bomber Allocation (Morning/Mid-day/Afternoon), Reconn (Missions/Aircraft/Escort/A.M. Only), Mission Timing "Coincidental", Attached/Detached Escort rows (Behind…Below, Ceiling…Late), **Ground Attack Gruppen** (Ju87/He111/Ju88/Do17 × Airfields/Docks/RDF/Convoys/London/Factories + Size per target/Secondarys), the **Missions** column (1/1/1/0/0/0 — gold's values), Escort Gruppen (Me109/Me110, %tied/%Free/Strafe), Resting, "Rest All"; footer log "Aircraft Quota Allocated" + clock "12 August 10:45 x1" (gold: 06:30). **S142: the spinner VALUES now render and they match gold number-for-number.** `CRSpinBut` — the
8th and last unhosted R\* type, and 85 of the dialog's controls — is hosted, so the grid's numeric
content draws: Bomber Allocation **Morning 40 / Mid-day 30**, Reconn **Missions 0 / Aircraft 1**, the
Ground-Attack matrix (Airfields **1**/0/0/0, Docks 0/**1**/0/0, RDF **1**/0/0/0, Convoys/London/
Factories all 0), **Size per target "1 Gruppe" ×6**, Missions **1/1/1/0/0/0**, Escort Gruppen
Me109/Me110 **2/1, 1/0, 2/0**, %tied **0** / %Free **100** ×6, Resting **1/3/2/2** and **3/1** — every
one of these agrees with gold #18, including the red spin-arrow art. **Remaining named deviations:**
(a) *new, found by this capture* — we draw a **"Sweeps" spinner row and an "Escort 1:1" combo row that
gold does not show at all**, and the Sweeps row overprints the "Ground Attack Gruppen"/"Escort
Gruppen" section headers; that is a template-membership / runtime-visibility gap (the S123 `ShowWindow`
+ S124 template-filter class), not a spin-control gap; (b) the Escort tick-boxes draw as plain circles
vs gold's red-tick squares; (c) no title-bar `? ✓ ✕` buttons. **State corroboration (FF note 15):**
this capture was taken without `BOB_TRACE_OLE`, so the run emitted no explicit record of the phase /
dialog / art set — the verdict rests on state that happens to be *rendered into the frame* (the clock
reads "12 August 08:45 x1" and the allocation values are Eagle-Attack-specific). That is adequate
here but it is luck rather than method; **SP.9** adds an always-on capture banner so no future
verdict depends on a state nothing in the output confirms. _History:_ **PARTIAL (S137: the Directives dialog is now REACHABLE + renders).** The LW Directives dialog (`LWDirectives`/`IDD_LWDIRECTIVES`) is on the **misc** toolbar (TB_MISC), which `bob_map_paint_oob` didn't walk. S137: an open scaffold (`bob_oob_open_directives` → `MiscToolBar().OpenDirectivetoggle(NULL)`, null-safe) + extend `bob_map_paint_oob` to render TB_MISC's logged children via a full recursive tree walk (`bob_oob_paint_tree_deep`, fchild+sibling — the dense grid lives in nested sub-panels the fchild-only Bases walk misses). The dialog opens (exit 0, no crash) and renders its frame + "Rest All" button + the standby reminder ("The Luftflotte are on standby, awaiting your orders.", `IDS_PHRASE_REMIND0`). _(S143 correction: that screen is **`DIRECTIVERESULTS`** (misc-toolbar child index 5), not the `LWDirectives` allocation grid (index 6). On an active day the game opens 5, whose own code opens 6 on top — DIRRSULT.CPP:197.)_ **Remaining (→ PARTIAL, was GAP):** the dense allocation **grid** (Bomber Allocation / Reconn / Escort / Ground-Attack-Gruppen spinners) doesn't show — gold #18 is **12 Aug Eagle Attack** (active gruppen), mine is **10 July Convoys** where the game shows the standby state (grid hidden until an active phase has gruppen to allocate); same fresh-day-vs-Eagle-Attack state gap as #19. Reaching an active phase to confirm the grid then renders through the deep walk is the follow-on. `BOB_MAP_DIRECTIVES` opens it; the TB_MISC paint is inert when no misc dialog is logged (no map regression). |
| 19 | 58-19 | Strategic map (LW campaign, raids/routes) | `BOB_AUTOCLICK=1,1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=700..1200` | `native-strategic-map-2026-07-25.png` | **CLOSE** — terrain, sectors A–E/Y/Z, city labels, No.11 Group, full unit-icon layer (green RAF / blue / yellow), footer event log + date-clock ("10 July 11:54 x1"), both LW toolbar rows, right ruler. Deviations: no raid stacks/route lines in the capture, ruler band plain vs gold's wooden art, accel transport buttons not in shot. **S143 re-diagnosed the raid-stack deviation and it is NOT a time/accel problem.** The old note ("fresh Convoys day, paused-start; gold is 12 Aug Eagle Attack at x300 with live raids") implied that reaching an active day and letting the clock run would produce raids. It will not. On an active day the LW player is held in a **closed two-dialog loop** — `LWDirectives::OnCancel` opens DirectiveResults (`OpenEmptyDirectiveResults`, LWDIRECT.CPP:1104) and `DirectiveResults::OnCancel` opens Directives back (DIRRSULT.CPP:194) — so the map cannot be uncovered by dismissing (measured: the close loop oscillates 6→5→6→5 indefinitely, and suppressing the day-start popup via `MMC.directivespopup` does not break the cycle). That is faithful: **the orders flow must be COMPLETED, not escaped.** And completing it is what creates the raids — `DirectiveResults::OnOK` calls `LWDirectivesResults::MakeLWPackages(dr, true)` (DIRRSULT.CPP:207), which builds the day's packages. So gold #19's raid stacks are downstream of the accept path, not of the clock. **S146: the raids now EXIST — the deviation is substantively resolved, the VERDICT is not yet re-issued.** Driving the genuine `OnOK` works once you address the right object: the logged child is an empty placeholder panel (`rtti=RDEmptyD`) and the real dialog is its `fchild`; descending one level ran `LWDirectives::OnOK` (guard `dirresults[0].targets[0]=13178`) → `DirectiveResults::OnOK` → **`MakeLWPackages`**. The map then draws **route lines across the Channel** and raid markers, the Mission Folder lists **R001 / 36 / Dive Bomb / T.O 08:53 / ToT 09:59 / Tangmere AF**, and the footer event log reports **"Geschwader Landed [R002]", "[R005]", "Geschwader Landing [R005]"** — raids built, flown and returning, at the banner-confirmed `phase=1 date=1250035200` (12 Aug, Eagle Attack). **#19 stays at its prior verdict for now** because no *unobstructed* capture exists: a dialog covered the map in both attempts (the Mission Folder is logged on a different toolbar — dismiss now sweeps all `TB_TOTAL`; and the Directives popup re-armed when its suppression flag was omitted). A parity verdict is a claim about a comparison, and the comparison shot is one flag-composition away — **SP.11 carries exactly that one run**. |

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
4. **Remaining render classes:** ~~native-DLU font FACE pass (R6.2)~~ **☑ CLOSED S131
   (MA note 26 §2):** `bob_gdi_font` drew every face in the one art TTF (Intel.ttf), so
   data/label rows rendered in the Rowan art face instead of Arial — the pervasive "font
   face" deviation on nearly every config/campaign screen. Now a per-FACE registry routes
   the game's requested face (Intel/FC-Glamour/Fusion→ART, Arial→SANS LiberationSans,
   Times→SERIF, Courier→MONO) + the italic flag (gold's combo values are Arial Italic) to
   its own TTF, threaded through the DC's selected `CFont`. ART screens stay byte-identical
   (title menu `cmp`-verified). MA note 26 §1 (EnumFontFamilies Japanese-branch) is **N/A
   for BoB** — the game already requests the English faces; §3 (combo black-fill) already
   handled by BoB's `m_FirstSweep=TRUE` host convention; MA note 27's listbox-fill warning
   heeded (untouched). `BOB_NO_FONTFACE` reverts. ~~multi-line word-wrap for description
   statics (R6.2)~~ **☑ CLOSED S127** (`CDC::DrawText` DT_WORDBREAK; only multi-line-height boxes wrap so
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
