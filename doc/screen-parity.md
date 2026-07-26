# Screen parity vs the Windows gold standard (Release SP)

Sprint 123 (2026-07-25). Gold standard: the PO-supplied Wine captures of the Windows
build (BDG 0.99 patched), `/run/media/admin/BEA6-BBCE/bob/` — **19 PNGs** dated
2026-06-24 (the backlog says 17; two are near-duplicate side-select shots — flagged
for the PO below). Native captures land in `doc/parity/` (1024x768 GDI framebuffer /
800x600 GL dumps, this repo).

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
| 2 | 47-38 | Quick Shots select (`quickview`/CSQuick1, Scenario page) | `BOB_STARTFLYING=click BOB_AUTOCLICK=0 BOB_SHOT=220` | `native-quickshots-2026-07-25.png` | **PARTIAL** — background montage, the 2 mission combos, description text, Back/Fly (now bottom-left, matches gold) all present; black montage frame matches gold (Wine shows it black too). Deviations: page-2/3 combos drawn empty over page 1 (page-switch hides not tracked — only runtime `ShowWindow` is, and CSQuick1 positions pages by `MoveWindow`, not tracked), Scenario/Parameters/Luftwaffe tab captions not drawn, description not word-wrapped (R6.2), label set differs (source .rc vs BDG resources). "This is disabled in the demo" ghost FIXED this sprint (ShowWindow honored). |
| 3 | 47-45 | Quick Shots — Parameters/player page (Squadron/Aircraft/Duty/Callsign, name box) | page tab click — no scripted recipe yet | — | **GAP** — the page tabs are dialog controls, not menu items; needs a `BOB_CLICKXY` recipe once the tab captions render (deviation above). |
| 4 | 47-57 | "Initialising 3D" loading screen (Dover art + red progress) | transient (drawn inside `StartFlying`→`Launch3d`) | — | **GAP (by design)** — our Launch3d bridge feeds the `Start3d` paint bits directly, so the progress screen never paints headlessly. Low value: transient screen. |
| 5 | 48-09 | In-flight cockpit (Spitfire Mk I, on runway) | `BOB_BOOT_FRONTEND=1 BOB_DUMP_FRAME=150 BOB_DUMP_PATH=<p> BOB_EXIT_AFTER_DUMP=1` on `:0` under the display lock | `native-cockpit-2026-07-25.png` | **CLOSE** — cockpit frame, gunsight, instrument panel, HUD info line ("4ft Hdg 242 Speed 0Kts" matches gold's readout), mirror, clouds. Deviations: prop rendered as a static dark blade vs gold's blur disc; our white HUD bar + Tower ATC line vs gold's red-only info line; 800x600 vs gold ~1830x1080. |
| 6 | 55-25 | PC Config — More GFX (`options3d2`) | `BOB_CONFIGSCREEN=gfx2 BOB_SHOT=70` | `native-config-moregfx-2026-07-25.png` | **PARTIAL** — rows+combos+red arrows right; label pairing fixed this sprint (dialog-scoped rect lookup). Deviations: label TEXT set differs from gold (BDG resources: "Town and forest raises"/"Routes"/"Detail Level" vs source-.rc "Trees etc"/"Contour Detail"/"TextureQuality"), some rows unlabeled, tab row tight-packed at ListX vs gold's spread. |
| 7 | 55-41 | PC Config — GFX (`options3d`) | `BOB_CONFIGSCREEN=gfx BOB_SHOT=70` | `native-config-gfx-2026-07-25.png` (before: `BEFORE-config-gfx-2026-07-25.png`) | **CLOSE** — every label now pairs with its combo (was fully scrambled — see BEFORE). Deviations: gold (BDG) adds 3D+Campaign Resolution rows and renames rows ("Gamma Level"); our "BoB Linux OpenGL backend" as Display Driver (faithful for the port); tab-row spread. |
| 8 | 55-52 | PC Config — Controls (`controloptions`/SController) | `BOB_CONFIGSCREEN=control BOB_SHOT=70` | `native-config-controls-2026-07-25.png` | **PARTIAL** — device line ("First Joystick: Logitech Extreme 3D", axes/buttons count), assignment combo grid + dead-zone/mode columns render and are interactive (R5.3). Deviations: several row labels missing/overlapped (label statics differ between source .rc and BDG dialog), Calibrate/checkbox art not drawn. |
| 9 | 55-59 | PC Config — Sound (`soundoptions`) | `BOB_CONFIGSCREEN=sound BOB_SHOT=70` | `native-config-sound-2026-07-25.png` | **PARTIAL** — driver combo ("Default") + volume rows render, 4 labels place correctly. Deviations: ~half the labels missing (not in source-.rc DLGINIT), one stray empty combo pair (top/bottom; hidden-in-gold controls), tab-row spread. |
| 10 | 56-15 | Sim Config — Flight (`flightoptions`) | `BOB_CONFIGSCREEN=flight BOB_SHOT=70` (`sim` also works) | `native-sim-flight-2026-07-25.png` | **CLOSE** — full form, labels on rows. Deviations: 2 unlabeled rows (Engine Management/Prop Pitch), last row "Spool Up" vs gold "109 Fuel Capacity" (BDG), values differ (settings state, not render). |
| 11 | 56-21 | Sim Config — Game (`gameoptions`) | `BOB_CONFIGSCREEN=game BOB_SHOT=70` | `native-sim-game-2026-07-25.png` | **PARTIAL** — form + combos right; several labels missing/overlapping (same resource-delta class). |
| 12 | 56-30 | Sim Config — Mission (`missionoptions`) | `BOB_CONFIGSCREEN=mission BOB_SHOT=70` | `native-sim-mission-2026-07-25.png` | **PARTIAL** — all 6 combos render with correct values (Medium/Medium/Historic/Historic/Off/On matches gold's row order); NO labels (the mission panel's labels aren't CRStatic-hosted or absent from source .rc — root-cause next). |
| 13 | 56-45 | Sim Config — Views (`vieweroptions`) | `BOB_CONFIGSCREEN=views BOB_SHOT=70` | `native-sim-views-2026-07-25.png` | **CLOSE** — 9 rows, labels aligned (was shifted/overlapped). Deviations: one overlapped label pair (Info Line/Camera Colour), "Auto Padlock" vs gold "Auto External" etc. (resource delta). |
| 14 | 56-53 | Campaign side-select (RAF/Luftwaffe) | `BOB_AUTOCLICK=1 BOB_SHOT=250` | `native-campaign-sideselect-2026-07-25.png` | **CLOSE** — art identical, RAF/Luftwaffe/Back hit-polygons live. Deviation: captions tiny vs gold's large gold face (the screen has no textlist coords; captions are art-adjacent). |
| 15 | 57-36 | Campaign side-select (near-duplicate of #14) | same | same | same — **flag for PO: #14/#15 duplicates** (19 vs "17" count). |
| 16 | 57-47 | Campaign phase select (`campaignselect`) | `BOB_AUTOCLICK=1,1 BOB_SHOT=380` | `native-campaign-phaseselect-2026-07-25.png` (before: `BEFORE-campaign-phaseselect-2026-07-25.png`) | **PARTIAL** — Back/Begin now bottom-left at the game's ListX/ListY (was top, overlapped); phase description + date + black montage frame (gold black too) render. Deviations: phase tabs tight-packed top-left (hosted listbox draws its own columns), duplicated date heading top-left, description unwrapped (R6.2). |
| 17 | 57-55 | Campaign enter-name (`campaignentername`) | `BOB_AUTOCLICK=1,1,1 BOB_SHOT=520` | `native-campaign-entername-2026-07-25.png` | **PARTIAL** — Back/Begin placed, montage frame + Luftwaffe/Commander texts present. Deviations: name-edit line not rendered as gold's "Commander Bob|" block; side/phase/date lines scattered (label rects + edit control not hosted). |
| 18 | 58-08 | Strategic map + LW Directives dialog | directives dialog not natively reachable yet | — | **GAP** — the OOB info-dialog subsystem renders Bases etc. (S113-S117); the LW Directives dialog is a toolbar dialog not yet wired. Map behind it: see #19. |
| 19 | 58-19 | Strategic map (LW campaign, raids/routes) | `BOB_AUTOCLICK=1,1,1,1 BOB_MAP_TIMER=8 BOB_SHOT=700..1200` | `native-strategic-map-2026-07-25.png` | **CLOSE** — terrain, sectors A–E/Y/Z, city labels, No.11 Group, full unit-icon layer (green RAF / blue / yellow), footer event log + date-clock ("10 July 11:54 x1"), both LW toolbar rows, right ruler. Deviations: no raid stacks/route lines in the capture (fresh Convoys day, paused-start; gold is 12 August Eagle Attack at x300 with live raids), ruler band plain vs gold's wooden art, accel transport buttons not in shot. |

## Systemic root causes (ranked, for SP.2/SP.3 continuation)

1. **Resource-version delta (largest class, needs PO guidance):** the gold Wine build runs
   the **BDG 0.99 patched resources** (BATTLE.DIR/exe .rsrc); the native port parses the
   **original source checkout's** `MIG.RC`/`RESOURCE.H`/DLGINIT at runtime
   (`bob_dlgtemplate.cpp`). Label texts, extra rows (3D/Campaign Resolution), tab captions
   ("BDG"), and the title item ("BDG 0.99" vs "Website") all differ at the *data* level —
   no amount of compat rendering fixes that. Faithful fix = read the installed build's
   resources (PE .rsrc DIALOG/DLGINIT parser), which also closes the packaging blocker
   ("resources from the source checkout"). ~8-13 pts, flagged as its own story.
2. **Fixed this sprint:** unscoped control-rect lookup (labels took rects from other
   dialogs' templates — scrambled forms); menu lists ignoring the game's per-resolution
   `ListX/ListY` anchor (Back/Begin/Fly rows drawn top instead of bottom); runtime
   `ShowWindow` visibility ignored (demo/disabled ghost statics drawn).
3. **Remaining render classes:** multi-line word-wrap for description statics (R6.2);
   tab-row column spread (hosted listbox column widths vs gold's PositionRListBox
   spacing); page-switching dialogs (`MoveWindow` offsets not tracked — Quick Shots
   pages overlap); native-DLU font face/size pass (R6.2); edit-control hosting
   (enter-name box).

## PO questions

- Gold folder holds **19** shots, not 17; #14/#15 are near-duplicates of the same screen.
  Confirm the canonical set (inventory above covers all 19).
- Resource-version delta (root cause 1): should parity be judged against the BDG 0.99
  resources (then: approve the PE-resource-parser story) or against the original 2000
  resources the source tree ships (then: several "deviations" above become MATCHes)?
