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

## Check progress

| What | Where |
|---|---|
| Backlog (Releases R1–R7 + SP) + burndown | `scrum.md` — Release SP is the screen-parity epic; §9 burndown |
| Per-gold-shot parity verdicts | `doc/screen-parity.md` — currently 16 CLOSE / 0 PARTIAL / 3 GAP of 19 |
| Side-by-side captures | `doc/parity/` |
| Engineering evidence log (newest first) | `PORT.md` |
| Live product snapshot | `STATUS.md` |
| History | `git log --oneline` on `linux-port` |

Gold standard: `/run/media/admin/BEA6-BBCE/bob/` (19 PNGs). Oracle ruling: the
gold shots as-is = the BDG 0.99 patched build (dialogs/strings read from
`boblang.dll` PE resources since S124; `BOB_NO_PE_RSRC=1` reverts).

## Current state (2026-08-02)

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
