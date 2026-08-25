# Battle of Britain — Run & Check Progress

## Run the game

```bash
cd "/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 /home/admin/bob/build/bob
```

That is the real front-end (menus, config screens, campaign strategic map, fly).
Alternative: `BOB_BOOT_FRONTEND=1 /home/admin/bob/build/bob` jumps straight into
a Quick-Mission cockpit. Requires a healthy GL display session.

Rebuild: `cd /home/admin/bob/build && ninja bob`.

Run the **German Convoys campaign** end to end (S195 — the gold video's own route,
`~/gold standard/bob/bob_convoy_campaign.mp4`):

```bash
gl-lock tools/bob_convoy_campaign.sh
```

Nine assertions from side-select to `InThe3D=1`. Its negative control is documented in the script:
drop `BOB_MAP_ACCEPTDIR` and the last four go to zero, because an unaccepted set of Luftwaffe
orders means the campaign genuinely has nothing to fly.

Run the per-sprint DoD gates (one lock acquisition, self-certifying):

```bash
gl-lock tools/bob_gates.sh <outdir> [baseline-dir]
```

It runs the 14-recipe headless sweep, the safe-default check, dummy-vs-real-GL `cmp`, the flight
frame, GATE 5 (the German Convoys campaign end to end), GATE 6 (the combat soak) and **GATE 7 (the
strategic soak, S206)**, and an A/B against `baseline-dir` — then **hashes `build/bob` before and
after and exits 2 if the binary changed under it**. Do not rebuild while a gate is queued or
running: treat "queued" as "running" (S148 lost a sweep to a mid-run rebuild, and the mixed-binary
result looked exactly like a regression).

**GATE 7 — `tools/bob_strategic_soak.sh`** is the one with a negative control. Plain, it asserts
that the German raid flies its route and the RAF is tasked to intercept it; `STARVE=1` re-runs the
old flight-based recipe, on which the same binary measures zero, and the gate must go **red**. That
difference *is* the S206 finding: the raid was never stuck, the runs were too short. Do not add an
aircraft-level combat assertion to it — a map-only run has no player aircraft, so those movecodes
cannot appear there whatever the code does.

## Check progress

| What | Where |
|---|---|
| Backlog (Releases R1–R7 + SP) + burndown | `scrum.md` — Release SP is the screen-parity epic; §9 burndown |
| Per-gold-shot parity verdicts | `doc/screen-parity.md` — **18 CLOSE / 0 PARTIAL / 1 GAP** of 19; only #4 (transient loading screen) is a GAP, **by design**. See the **Capture-vs-gold STATE audit** section there: #16/#17/#19's verdicts are earned as of S149, four rows remain unverified (SP.20) |
| Side-by-side captures | `doc/parity/` |
| Engineering evidence log (newest first) | `PORT.md` |
| Live product snapshot | `STATUS.md` |
| History | `git log --oneline` on `linux-port` |

Gold standard: `/run/media/admin/BEA6-BBCE/bob/` (19 PNGs). Oracle ruling: the
gold shots as-is = the BDG 0.99 patched build (dialogs/strings read from
`boblang.dll` PE resources since S124; `BOB_NO_PE_RSRC=1` reverts).

## Current state (2026-08-08)

- **S149 closed: the gate suite certifies its own validity, and a deliberate audit
  found a THIRD parity verdict resting on a capture-vs-gold state mismatch.**
  Gates now run from versioned `tools/bob_gates.sh` (see above) which hashes the
  binary before and after. The audit: gold #17 is **Eagle Attack, 12–23 Aug**, but
  the recipe `1,1,1` never selected a phase, so every capture was Convoys — the same
  root as #16 (S141) and #19 (S148). All three now carry the `#1000:1` phase token
  and match gold on state. **The lesson recorded in `doc/screen-parity.md`: a state
  difference the recipe CAN fix is a defect in the test; one it cannot is a finding
  about the port** — the doc had been merging the two, which is how three verdicts
  looked earned when they were not. Four "combo values (settings state)" rows
  (#6/#9/#10/#11) are the same shape but settings-resident: **not verified** (SP.20).
- **S148 closed: captures aim at a STATE, not a moment.** `BOB_SHOT_WHEN=clear`
  fires on the first map paint with no logged child on any toolbar, AND-able with
  `BOB_SHOT_DATE` / `BOB_SHOT_TIME_LT`. Seven counter-based attempts at gold #19's
  frame missed; the first state-based one hit. Gold #19's verdict is now made on a
  like-for-like 12-Aug Eagle-Attack frame, and its raid-stack difference is measured
  as **timing** (the map is covered precisely while raids fly).
- **S146 closed: the LW orders flow completes** — `LWDirectives::OnOK` →
  `DirectiveResults::OnOK` → `MakeLWPackages`. The day's raids are built, fly and
  land (routes, Mission Folder R001/Tangmere AF, "Geschwader Landed"). Reaching it
  needed descending from the logged child (an `RDEmptyD` **placeholder panel**) to
  its `fchild`, the real dialog.
- **S142 closed: hosted `CRSpinButCtrl` — the 8th and LAST R\* control type; the
  Directives grid's numbers render and match gold #18 value-for-value.** The dialog
  is mostly made of this type (85 DDX-bound spinners), driven via `CRSpinButExtra`'s
  `Clear()->MakeNumList()->SetIndex()` (= dispids 7 / 5×N / setprop 2). Two compat
  gaps closed: `CWnd::ReleaseCapture` was absent from `afxwin.h`, and the control's
  **static** `m_bDrawing` flag (cleared only inside `DrawBitmap`) would have latched
  on any black-fill draw and silently stopped the other 84 spinners — cleared
  host-side. **Repro:** `BOB_AUTOCLICK=1,1,#1000:1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=900`.
  New deviation booked as SP.8: a "Sweeps" row + "Escort 1:1" row we draw and gold
  doesn't. **Careful with `BOB_TRACE_OLE` on this screen** — it is per-control-per-frame
  and writes ~70 MB over a full run, slow enough to miss the capture tick.
- **S141 closed: the campaign PHASE is selectable — gold #18's Directives
  allocation grid renders; #18 PARTIAL → CLOSE (parity 18 CLOSE / 0 PARTIAL /
  1 GAP, the GAP being #4 by design).** BoB models a tab row as the *columns*
  of one `CRListBoxCtrl`; `CSCampaign::OnSelectRlistCampaigns(row, column)`
  picks the campaign phase from the **column**, which our hosted-listbox click
  hardcoded to 0 — so every campaign the port ran started in phase 0 (Convoys,
  10 July, a standby day with nothing to allocate). Now resolved through the
  genuine control's own `GetColFromX` (`colAtX` beside `rowAtY`;
  `BOB_NO_LIST_COL` reverts). `BOB_AUTOCLICK` gained a `#ID[:COL]` step that
  resolves its click point from the control's own drawn rect + column metrics
  (adopted from MA S62/S63 — never fixed pixels in a drive recipe).
  **Repro:** `BOB_AUTOCLICK=1,1,#1000:1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=1100`
  (the game opens Directives itself on an active day — `BOB_MAP_DIRECTIVES` is
  no longer needed). Carried: the ~50 numeric spinner boxes need `CRSpinBut`
  hosted (8th R\* type, the only unhosted one); #19's raid-stack deviation
  still stands — no headless way to dismiss a game-opened OOB dialog
  (`OpenDirectivetoggle` stacks a second one).
- **S140 closed: hosted `CREdtBtCtrl` (7th R\* control type) — the "Bob" briefing
  name box renders; gold #3 PARTIAL → CLOSE.** BoBFrag's pilot slots
  `IDC_PILOT_0..14` are `CREdtBt` (edit-button), DDX-bound; `OnInitDialog`'s
  `SetCaption(playerslotname)` sets the player's "Bob". Hosted the genuine
  `CREdtBtCtrl` (new `SRC/REDTBT/bob_ole_redtbt.cpp` + OCX `REDTBTC.CPP` +
  `CLSID_REdtBt`) mirroring the REdit host (caption is stock →
  `InternalSetText`; `captiontext` refreshed from `InternalGetText()` in
  `draw()`). Two OCX compile-compat fixes (BOB_LINUX): `IconsUI` forward-decl
  `:int`→`:unsigned int` + `MaskIcon(CPoint&)` temp-bind (§8p). Gates: safe
  default exit 0; mainmenu dummy==GL byte-identical; flight 94.9% non-black.
  **Parity 17 CLOSE / 1 PARTIAL / 1 GAP** — with S135 roster + S136
  Return-to-Player + S139 Fly footer, every gold-#3 element renders.
- **S139 closed: footer-listbox clip fix — clipped last footer/tab columns now
  render.** The footer/tab `CRListBoxCtrl` lays columns at its own internal widths
  but `ExtTextOut`-clips each to the passed `rcBounds`; `bob_draw_menu` passed a
  tight `total` (re-measured text widths) that clipped the last column off
  (bobfrag "Fly", QS Scenario "Fly", config tab edges). Widened the listbox clip
  to the remaining screen width (positions internal + hit-rects from `wids[]` →
  nothing moves; `BOB_NO_FOOTER_CLIP` reverts). bobfrag footer = Back/Sim
  Config/Fly (gold #3); QS footer = Back/Fly (gold #2). A/B verified each screen's
  diff is a benign clipped-edge reveal (mainmenu byte-identical). Gates: safe
  default exit 0; flight 94.9% non-black. #3 nearer CLOSE (only the "Bob" name box
  `CREdtBt` remains).
- **S137 closed: LW Directives dialog (gold #18) now REACHABLE + renders — #18
  GAP → PARTIAL.** The Directives dialog (`LWDirectives`) is on the **misc**
  toolbar (TB_MISC), which `bob_map_paint_oob` never walked. Added
  `bob_oob_open_directives` (`MiscToolBar().OpenDirectivetoggle(NULL)`, null-safe)
  + `BOB_MAP_DIRECTIVES` trigger + extended `bob_map_paint_oob` to render TB_MISC
  logged children via a full recursive walk (`bob_oob_paint_tree_deep`). The dialog
  opens (exit 0) + renders its frame + "Rest All" + standby reminder. Bases OOB
  (TB_MAIN, unchanged) still renders; TB_MISC paint inert when no misc dialog
  logged (no map regression); safe default exit 0. **Honest:** the dense
  allocation grid doesn't show — gold #18 is 12 Aug Eagle Attack (active gruppen);
  mine is 10 July Convoys standby state (grid hidden until active), same state gap
  as #19. Parity now **16 CLOSE / 2 PARTIAL / 1 GAP** (only #4 loading-screen GAP
  remains, by design). Evidence `doc/parity/native-strategic-directives-2026-08-02.png`.
- **S136 closed: template-driven BUTTON hosting — gold #3's "Return to Player"
  renders.** `IDC_RETURNTOPLAYER` (2146) is a template-only button no DDX binds →
  never created (our creation is DDX-driven). Extended the S124 template-driven
  static hosting to non-DDX **buttons** (`bob_dlg_enum_buttons` +
  `bob_make_rbutton`; `BOB_NO_TEMPLATE_BUTTONS` reverts) → the briefing's "Return to
  Player" now draws top-left with its caption = gold #3's key element. Config +
  QS-Scenario + mainmenu + phase-select all **byte-identical** on/off (surgical);
  safe default exit 0; flight 94.9% non-black. **#3 stays PARTIAL** (improved) —
  remaining: "Bob" name box (`CREdtBt` pilot slot, unhosted) + Fly footer item
  (gated); button art is the tickbox icon vs gold's bezel.
- **S135 closed: the mission BRIEFING (gold #3, `IDD_BOBFRAG`) RENDERS.** New
  `BOB_BOBFRAG` scaffold (FULLPSYS.CPP, BOB_LINUX, default-off) is the reliable
  headless reach: QS click-mode pre-flight seeds `quickdef` →
  `LaunchScreen(&quickmission)` inits CSQuick1 → `LaunchScreen(&bobfrag)`, stopping
  at the briefing (no flight) so `BOB_SHOT` captures it. Renders closely matching
  gold #3: crashed-109 + pink-cloud + two-He111 background, the roster listbox
  (`CRListBoxCtrl` id=1481: **Unit / Aircraft / Duty / Callsign → 54 Squadron /
  Spitfire / Patrol / Trumpet**), Back/Sim Config footer, exit 0. **#3 GAP →
  PARTIAL** (parity 16 CLOSE / 1 PARTIAL / 2 GAP). Deviations: "Return to Player"
  (RButton, not front-end-hosted); name edit (`CREditCtrl` 1923) created-not-drawn;
  Fly footer item gated. Gates: safe default exit 0; mainmenu dummy==GL
  byte-identical; flight 94.9% non-black. Evidence
  `doc/parity/native-quickshots-bobfrag-2026-08-02.png`.
- **S134 spike: gold #3 re-mapped to `IDD_BOBFRAG` (the mission briefing).**
  Following S133's "flight-line click → editor" remaining-step, `grep
  IDC_RETURNTOPLAYER` showed gold #3's "Return to Player" is in **`IDD_BOBFRAG`**
  (`class BoBFrag`, BOBFRAG.CPP) — the mission **briefing** (pilot roster +
  Squadron/Aircraft/Duty/Callsign + formation callsign buttons + Back/Sim
  Config/Fly footer), not a QS `CSQuickLine`/`QuickParameters` sub-editor. Reached
  via the mission-fly / campaign-intercept flow (`{IDS_FLY,&bobfrag,...}`;
  `BOB_CAMPAIGN_FLY`) — the QS Scenario "Fly" (`FragFly2`) goes straight to flight
  (hangs under the SDL-dummy capture driver, no GL). No code shipped. #3 stays
  GAP, now mapped to its true screen; next sprint = reach BoBFrag headlessly +
  render its roster (likely reuses the S133 nested draw + RButton callsigns).
- **S133 closed: the QS order-of-battle flight-lines RENDER.** S132 fixed the
  crash so the RAF/Luftwaffe tabs load; the `CSQuickLine` content stayed blank
  because the per-panel draw only reaches controls whose `parentDlg` is that
  panel, and the OOB is a `DialList` of `CSQuickLine` rows each with its own
  `parentDlg`. Probe (`BOB_TRACE_OOBTREE`) showed the game's layout is unusable
  headlessly (nested nodes: viewsize height 0, full-screen GetWindowRect —
  MoveWindow/OnSize stubs). Fix: `bob_fp_draw_nested`/`bob_nested_walk`
  (FULLPSYS.CPP, BOB_LINUX, default-on, `BOB_NO_QS_NESTED` reverts) walk the
  panel's child `RDialog` tree and draw each nested dialog's hosted controls,
  **synthesizing** the vertical row stacking (`BOB_QS_ROWSTEP`) + reusing the
  template-rect column layout. RAF tab now shows the flight row (piloted-flag icon
  + Patrol/Altitude/Skill → Spitfire IA/1/Veteran). **MA note 28 (OOB listbox
  black-fill) verified N/A** — the map Bases dialog already composites correctly.
  Gates: config-gfx2 + QS-Scenario `cmp` byte-identical on/off (no regression);
  mainmenu dummy==GL byte-identical; flight 94.9% non-black; safe default exit 0.
  **Honest:** single-flight rows validated; multi-row stacking synthesized (not
  yet captured with >1 flight); renders the OOB **list** — gold #3 proper (the
  per-flight *editor* via a flight-line click) stays GAP with that click the only
  step left. Evidence `doc/parity/native-quickshots-oob-raf-nested-2026-08-02.png`.
  Cross-port §8s.
- **S132 closed: the S130 QS order-of-battle crash is FIXED.** The RAF/Luftwaffe
  tabs (clickable since S129) ran `QuickMissionBlue/Red`, whose variadic `DialList`
  copy-constructs a `DialBox` from `*(DialBox*)NULL` for inactive flight slots →
  SIGSEGV. Fixed with a null-reference-safe `DialBox` copy ctor (RDIALOG.H,
  BOB_LINUX) — inactive slot → empty leaf DialBox; `AddChildren` renders it as an
  empty `RDEmptyP`. Two layers (ctor deref + uninit `diallist` behind copy-elision →
  copy `diallist` explicitly). RAF-tab exit 0; 13/13 sweep; mainmenu/controls/phase
  `cmp` byte-identical to S131 (core change transparent); flight + OOB dummy==GL +
  safe default pass. **Honest:** crash fixed + screen unblocked, but the `CSQuickLine`
  flight-line CONTENT (gold #3 fields) doesn't paint yet (nested-dialog-render gap).
  #3 stays GAP, crash blocker gone. Cross-port §8q addendum.
- **INBOUND MA note 28 (MA S71) — `doc/CROSS-PORT-FROM-MA-2026-08-02-note28.md`,
  scoped for a future OOB-polish sprint.** Completes note 27 §2: skip the
  `CRListBoxCtrl` black fill on the OOB draw path ONLY (a one-int context flag set
  around the OOB `OnDraw`), so the front-end menu keeps its opaque box (byte-identical)
  while OOB tables composite over the panel = gold. Check whether BoB's OOB dialogs
  (Bases/Squadrons, S113-S117) show opaque-black listboxes first. Also §2: measure
  carried "residuals" before fixing — half of MA's dissolved on measurement.
- S126 closed (`6ab411d`): property-stream reader (`9105e25`) capture-proven —
  14-recipe sweep 14/14, 13 screens moved toward gold, #16 duplicate-date fixed
  (covered-static settled-state erase) → #16 CLOSE.
- **S127 closed: label-render fidelity.** `CDC::DrawText` now implements real
  DT_WORDBREAK word-wrap (phase-select #16 + QS #2 descriptions wrap in their
  boxes; ≥2-line-box guard keeps config labels single-line — no regression) and
  '&' accelerator escape ("Cockpit && UI"→"Cockpit & UI", #8). `BOB_NO_WORDWRAP`
  / `BOB_NO_AMP_ESCAPE` revert. → **#16 PARTIAL→CLOSE, #8 CLOSE, #2 improved**
  (parity 16 CLOSE / 1 PARTIAL / 3 GAP of 19).
- **S128 closed: hosted the `CRRadioCtrl`** (6th R\* control type) → the
  Quick-Shots page-tab row (Scenario/Parameters/Luftwaffe/RAF, `IDC_RRADIO`)
  now renders each caption + its selection-tick icon. **#2 PARTIAL→CLOSE**
  (parity **16 CLOSE / 0 PARTIAL / 3 GAP** of 19).
- **S129 closed: QS tab navigation works** — a click on a tab fires the genuine
  `Selected` event (`OleHost::onButtonClick` → `bob_ole_click` → S33 eventsink →
  `CSQuick1::OnSelectedRradio` → `LaunchDial`), switching the panel page. Clicking
  Parameters renders the mission-params page; Scenario switches back (bidirectional).
  **Gold #3 mapping corrected:** `16-47-45` is the player-flight editor
  (Squadron/Aircraft/Duty/Callsign, `CSQuickLine`), not the Parameters tab — so #3
  stays GAP with its true path (a flight-line click) now known. Parity unchanged.
  **All `bob` runs now go through `gl-lock`** (incl. headless captures) per the
  shared-display protocol.
- **S130 spike: gold #3 (QS order-of-battle) root-caused.** The RAF/Luftwaffe tabs
  (clickable since S129) reach `QuickMissionBlue`/`Red` — a never-run screen that
  SIGSEGVs. Root cause: the variadic `DialList`'s per-slot ternary
  `(count>k)?DialBox(temp):*(DialBox*)NULL` copy-constructs a DialBox from null for
  inactive flight slots (benign-on-MSVC/faults-on-GCC UB; `fullpane.cpp:215`). Fix is
  game-code (name the DialBox locals so the ternary yields a reference) — deferred;
  banked with cross-port note §8q. #3 stays GAP, now exactly root-caused. Tabs 0/1
  (Scenario/Parameters) render fine; 2/3 reach the crash.
- GL gates PASS (S127 + S128): safe default `./bob` exit 0; flight frame-150
  95.2% non-black on `:0`; dummy==GL `cmp` byte-identical on mainmenu,
  phaseselect (S127) and quickshots (S128).
- Cross-port: MA note 17's `DrawText DT_WORDBREAK` shared find implemented
  BoB-side (S127) — outbound BoB note = shared-doc §8o; new-R\*-control-type
  hosting checklist + MaskIcon temp-bind = §8p (S128); null-DialBox-copy trap =
  §8q (S130). MA copy synced byte-identical. Inbound MA notes 18–25 (§8g–8n) logged.
- **S131 closed: per-face font registry (MA note 26 §2) — the pervasive "font FACE"
  deviation is FIXED.** `bob_gdi_font` drew every face in the one art TTF (Intel.ttf);
  now an 8-slot registry (4 kinds × regular/italic) routes the game's requested face +
  italic to its own TTF: Intel→ART, Arial→LiberationSans, Times→LiberationSerif,
  Courier→LiberationMono. Threaded through the DC's selected `CFont`. Config/campaign
  data/labels now render Arial (values italic) = gold. ART screens `cmp` byte-identical;
  controls dummy==GL byte-identical; 14/14 sweep; flight + safe default pass.
  `BOB_NO_FONTFACE` reverts. **MA note 26 §1 (Japanese-branch) N/A for BoB** — a
  `BOB_TRACE_FONT` dump showed the game already requests Arial/Courier/Intel; §3
  (combo fill) already handled by `m_FirstSweep=TRUE`. **MA note 27 (S70) processed:**
  its listbox-black-fill-is-load-bearing warning heeded (untouched). Shared-doc §8r
  (BoB's adoption + the "not-Japanese/§1-N/A" diagnostic + the italic extension).
