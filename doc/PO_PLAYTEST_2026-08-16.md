# PO play-test, 2026-08-16 (BoB)

Reported after S164–S172. Numbered for the backlog; evidence images are the PO's.

| # | Report | Status / first finding |
|---|---|---|
| BOB-PO-1 | **Turkey-shoot debrief font too large** — the Combat Report's values ("None/None/None") render at roughly double the size of the labels beside them. | **Lead, not yet fixed.** `bob_dlg_getfont` (RCOMBO/bob_ole_rcombo.cpp) *always* returns the **2× face** `g_AllFonts[fontnum][3]`, on the stated assumption that "the front-end panels are drawn scaled-up … so the 2x font matches the box heights". Where that assumption does not hold — a panel drawn 1:1 — every control that asks for a font gets one twice the intended size, and the mixed appearance follows from some controls falling back to `[0]` when `[3]` was never created. The fix is to pick the face from the panel's ACTUAL scale (`RFullPanelDial::resolutions[m_currentres]` vs the window width), not unconditionally. Needs a way to reach the debrief headlessly before it can be verified. |
| BOB-PO-2 | **Turkey-shoot terrain tiles scrambled** — ground made of rotated/offset patches, one black; tiles at wrong angles with water planes floating. | This is the long-standing **TERRAIN-1**, previously "not reproduced" — and it now has a repro context: the **turkey-shoot** scenario, external/padlock view, low altitude. That is the detail the earlier attempt lacked. |
| BOB-PO-3 | **1920×1080 window is not centred** (MA centres it). | ✅ **Fixed, S172.** `ensure_window`'s resize branch was a bare `SDL_SetWindowSize`, which keeps the old top-left so a larger mode spills off-screen. Now re-centres, and drops the border + pins to (0,0) when the mode fills the desktop. Ported from MA S155 — which also carried a second half, see below. |
| BOB-PO-4 | **Two mouse icons on the M view.** | Not yet investigated. |
| BOB-PO-5 | **A bogie shows in the upper-left view but is never visible.** | Not yet investigated. |
| BOB-PO-6 | **Numpad-Enter does not show the instrument panel** — gives a super-zoomed cockpit view. | Not yet investigated. A view/zoom keybind. |
| BOB-PO-7 | **Campaign map vs the gold standard.** | Partly done in S171 (the right-edge ruler's labels were running off the screen; `SetTextAlign` was a no-op). The rest of the comparison is open. |
| BOB-PO-8 | **Missing initial Luftwaffe dialog.** | Not yet investigated. |

## S172 also carried MA's other half

MA S155 found that `ensure_window` is reached from `SetCooperativeLevel`/`SetDisplayMode` during
flight setup, which there ran on the **flight thread**: SDL's X11 backend needs those calls on the
thread that created the window, and off-thread it wedges — MA hung at 100% CPU with the window
never returning, and cost a play session before it was found. BoB has the same call shape, so it
now gets the same guard *before* it bites: off-thread callers record the size and the main thread
applies it from the pump (`BOB_WINDOW_ANYTHREAD=1` reverts).

## BOB-PO-2 (terrain tiles): reproduction attempts, all negative

The images show, unmistakably: ground built from large rectangular patches, each carrying a
different piece of aerial photography, **rotated and offset relative to each other**, with gaps
(one black) and — in the higher shot — a water plane floating at a different height from the green
tiles around it. That is per-tile transform state going wrong, not a texture or a filter problem.

What was tried, on real GL, and what each produced:

| recipe | result |
|---|---|
| `BOB_BOOT_FRONTEND=1 BOB_AUTOFLY=view40` (F6 external) | external view works; aircraft **never leaves the runway** (Alt 4ft, 0 Kts) — flat airfield terrain, no corruption |
| `BOB_AUTOFLY=toext` (throttle + nose-up trim + F6) | same: Alt 4ft. The injected throttle/trim keys do not achieve takeoff in this build |
| `BOB_STARTFLYING=click BOB_AUTOCLICK=10` | never reaches Fly ("navigate to Fly by clicks") |
| `BOB_STARTFLYING=click BOB_AUTOCLICK=0,1,2` | flies, but a **takeoff** scenario — on the runway again |
| `BOB_STARTFLYING=click BOB_AUTOCLICK=1,0,2` | never reaches flight (timeout) |

> **RETRACTED (S173e).** The conclusion below — "the turkey shoot is unreachable headlessly" — was
> wrong, and wrong in the most avoidable way: every recipe in the table above drives the **click**
> path (`BOB_AUTOCLICK`/`BOB_STARTFLYING=click`), and the forced-flight path has read
> **`BOB_QM_INDEX`** since the boot scaffold was written. `BOB_BOOT_FRONTEND=1 BOB_QM_INDEX=11`
> boots straight into it: `[boot] QM: idx=11 title=2237` (`IDS_MISTYPE_DOGFIGHTING`) and a frame-150
> dump reads **`Alt 9524ft  Speed 301 Kts  Power 90`** — airborne at the scenario's `FT_10000`, not
> on a runway. Five recipes were tried and all five varied the same axis. *When a series of attempts
> all fail the same way, the next thing to vary is the mechanism, not the parameters.*
>
> Index note: counting the table structurally gave 10 and the game answered `title=2236`
> (`IDS_MISTYPE_FAMILIARISATION`) — off by one, because of the `#ifdef BOB_DEMO_VER` block at the
> head of `quickmissions[]`. The trace settled it; the prediction was stated first so it could be
> wrong out loud.

So every frame the CLICK recipes can capture is of an aircraft **on the ground at an airfield**, where
the terrain is flat and correct. The turkey shoot is `//COMBAT : turkey` in `SRC/BFIELDS/QMISS.CPP:370`
— 1 Spitfire vs 1 Me109, `FT_10000`, over Ramsgate — and it **starts airborne**, which is exactly
the state the corruption appears in and the one no recipe here reaches.

**What would settle it cheaply**, in order:
1. Whether the corruption also appears in the **cockpit** view or only the external/padlock view.
   (It is drawn by the same terrain path either way, so "external only" would point at the
   external-view scene setup that S118/S119 already had to fix once.)
2. Whether the tile layout **differs between two runs of the same scenario**. Run-to-run variance
   is this port's signature for an uninitialised read fed by a stub (see the shared notes), and it
   would collapse the search immediately.
3. The approximate altitude when it starts — the higher shot shows smooth distant ground with a
   few sharp tiles at the wrong angle near the bottom, which looks like a LOD/tile-selection
   boundary.

The right fix for the harness, separately, is a quick-shot recipe that can select a **named**
scenario rather than a row index, so an airborne dogfight is reachable headlessly at all.

---

# PO play-test, round 2 (same day, after S172)

> "in turning fight tiles seem better, but stil update too slowly / floating square / debrief font
> still too large / on campaign screen clicking red icon at bottom again does not dismiss dialog,
> not mission dialog appears (for German side) / **time is stopped at 0x**"

| # | Report | Status |
|---|---|---|
| BOB-PO-9  | **Campaign clock stopped at x0.** | ✅ **Fixed, S173.** Root cause below — and it also accounts for BOB-PO-10 and BOB-PO-8. |
| BOB-PO-10 | **No mission dialog for the German side.** | ✅ **Follows from BOB-PO-9.** With the clock running, the LW Directives dialog (dlg 1032) appears on its own: `dialogs=6 … dlg=1032:156/184` in the click arm vs `dialogs=5` and no 1032 in the control arm. Not a separate defect. |
| BOB-PO-8  | **Missing initial Luftwaffe dialog** (round 1). | ✅ Same cause. A campaign whose clock never advances fires no day/period events. |
| BOB-PO-11 | **Red icon at the bottom does not dismiss the dialog.** | Open. Distinct from the above — the accel row is now hit-testable, so this is a different control's click route. |
| BOB-PO-2  | **Terrain tiles "update too slowly"** (was: scrambled). | Refined by the PO: with S172 in, the tiles are *better* but refresh late. Re-read as a tile/LOD **update rate** problem rather than corrupt per-tile transforms. |
| BOB-PO-12 | **Floating square.** | Open, not investigated. *Correcting my own first note here:* I initially flagged S173's icon-numbering fix as a candidate cause. That is unlikely — the PO lists this between two in-flight observations (tile update rate, then the debrief), so it is a **3D** artifact, and the icon sheet feeds the 2D map/UI, which the 3D view does not draw through. Recorded so the next session does not spend the lead. |
| BOB-PO-1  | **Debrief font still too large.** | Still open; S173 confirms the diagnosis from a second, independent screen (the campaign clock's LCD text overflows its plate for exactly this reason) and adds the game's own rule as the target: `RFullPanelDial::OnGetGlobalFont` returns `g_AllFonts[fontnum][m_currentres]`, and `resolutions[]={800,800,1024}` has **no index 3** — the face this port pins to is one the shipped game never selects. |

## BOB-PO-9 root cause — the campaign clock could not be started

`RFullPanelDial::LaunchMap` starts the strategic map **paused on purpose** (S94), because the real
game freezes until the player presses an accel button:

```c
MMC.curracceltype = MMC.ACCEL_PAUSED;      /* "the accel buttons (Play/FF) run it" */
```

Those buttons were **not reachable**, so "waits for the player" became "waits forever":

1. **A hand-written id list.** `bob_map_paint_toolbars` drew three named ids (PAUSE/PLAY/
   FASTFORWARD) at a hand-placed row. `IDDT_TITLEBAR` is a complete 209×56 DLU design with
   **four** buttons — `IDC_CONTROL` at DLU x26 was in no list and so was never drawn at all.
2. **A scaffold standing in for the panel.** `bob_map_paint_titlebar` filled a parchment rectangle
   and `TextOut`'d a clock string it composed with the same format `TitleBar::Redraw()` uses. It
   looked like a clock, so nothing looked missing — but it could never grow the buttons, and the
   real controls had no drawn rect, which is what `bob_ole_click` hit-tests. Unclickable.
3. **A mis-numbered icon table.** `bob_icon_pagenum` parsed `iconnum.g` with
   `if (strncmp(p,"ICON_",5)) continue;` **before** the index counter — but 32 of that file's 94
   enumerators are `B_ICON_*` map markers, which are real enum members consuming real values. Every
   icon past index 34 was therefore numbered short by up to 32: `ICON_PAUSE` (79) resolved to page
   47, so the three buttons that *were* drawn showed round gold map tokens.

Fixed by drawing the panel from its own template (`bob_ole_draw_panel`, all 8 controls, geometry
from the resource so no control can be omitted by oversight), counting every enumerator, and adding
the missing half of the art-blit pair — the panel path set the text viewport but never the DIB
origin/clip, so art-bearing controls drew captions in the right place and bitmaps somewhere else.

**Evidence** (identical recipes, one click apart — the control arm is the point):

| arm | tick | campaign clock |
|---|---|---|
| no click | 1100 | `time=23400` — unchanged from tick 850 |
| Play clicked at tick 900 | 1100 | `time=23620`, panel reads **x1**, Play lit, Pause released |

## Method note

Three defects, one chain, and each layer was individually plausible enough to stop at. The thing
that kept it moving was refusing to accept a *rendered* clock as evidence of a *running* one:
the scaffold drew the right string from the right variables, so every screenshot showed a clock,
and only `x0` — a value, not a picture — said it was stopped. **Measure the state, not the picture.**

## BOB-PO-1 (debrief font) — attempted in S173b, REVERTED, and why

The diagnosis in the round-1 table is right about the mechanism and wrong about the fix.

Implemented: `bob_dlg_getfont` deferring to `RFullPanelDial::OnGetGlobalFont`, i.e. the game's own
rule `g_AllFonts[fontnum][m_currentres]` instead of the hardcoded 2× face `[3]`. Then measured
before shipping, and backed it out. Two reasons, in order of weight:

1. **The screen I could measure says the current face is right.** Cropping our `config-gfx` and
   gold's Display-Driver page to the *same rect at the same scale*, our label text and gold's are
   near-identical in size. The change would have selected `[2]` (5/3×) and made the whole front-end
   ~17% smaller than gold — a regression on a screen the PO has already accepted as usable, traded
   for an improvement I could not observe.
2. **The screen that motivated it is unreachable headlessly.** The turkey-shoot debrief has no
   capture recipe (see the five negative attempts above), so "did this fix the debrief?" had no
   answer available. Shipping an unverifiable change that provably degrades a verified one is the
   wrong trade.

**What the evidence actually points at.** The faces are `[0]=1× [1]=4/3 [2]=5/3 [3]=2×`, and
`MIG.CPP` creates `[1]`,`[2]`,`[3]` **only under `FI_4VER`**. A font without that flag therefore has
*only* `[0]`. So on one screen, controls using an `FI_4VER` font get `[3]`=2× and their neighbours
using a plain font fall back to `[0]`=1× — which is exactly the PO's report, "values at roughly
double the size of the labels beside them". The defect is the **2× cliff between neighbours**, not
a global scale error, and no choice of a single global face fixes it.

Also worth recording: the campaign clock's text was oversized for the same *visible* reason and had
a completely different cause — its box was being drawn at the config-panel's 1.5× DLU scale instead
of the toolbar's 1:1, and the text height is derived from the box height. Fixing the geometry fixed
the text with no font change at all. **Two screens with the same symptom did not share a cause**,
which is the strongest argument against treating "font too large" as one bug.

**Next step (needs a harness, not a guess):** a headless route to the debrief, then dump which
`fontnum` each control on it asks for and which faces exist for those numbers. That turns this from
a scale argument into a table.

## BOB-PO-11 (red icon does not dismiss the dialog) — mechanism found, S173c

The footer event log is the third instance of the S173 pattern in the same band. `IDDT_TELETYPE`
is a real 317×56 DLU panel — three two-column log lines (`IDC_ITEM1..3` + `IDC_LINE1..3`) between
two end caps — and all eight controls were hosted and **none drawn** (`dlg=1065:0/8`).
`bob_map_paint_teletype` hand-rendered the log instead, on the stated belief that "those controls'
OnDraw doesn't run headlessly" — true when written, false since the OCX hosting landed. It even
re-walked `Node_Data.intel` with the same `MessageTitleToText`/`GetTargName` calls
`TeleType::Refresh` uses, so the port carried two implementations of one thing and drew the copy.

**Why this is very likely the PO's report.** The log lines' handler is a **toggle**:

```c
void TeleType::OnClickedLine1() {
    Refresh();
    if (makertn)
        if (!LoggedChild(0)) LogChild(0, makertn(messages[3]));  /* open  */
        else                 CloseLoggedChild(0);                /* close */
}
```

Click a line, its dialog opens; click **again**, it closes. The PO's wording was "clicking red icon
at bottom **again** does not dismiss dialog". With those six controls never drawn they had no
recorded screen rect, and `bob_ole_click` hit-tests exactly those rects — so the *first* click could
open a dialog by some other route while the *second* had nothing to land on. Drawing the panel
gives all six their rects.

Stated as a lead, not a fix: the PO says "red icon", and the log lines are not red icons, so this
may instead be one of the red Luftwaffe cards on the right of the same band. What is now true
either way is that the log lines are drawn and clickable, which they were not before. The
open→close→open assertion is the test to run against `dialogs=N`.

**Appearance, checked against gold rather than assumed.** Gold's log is a **light grey** plate with
dark two-column text. The scaffold drew a black-brown inset with amber text and blue place names —
wrong on every count. The real controls take `FIL_TELEBACK`, which the S173 art dump showed is a
uniform light-grey 500×500 plate: the same art the clock's LCD uses, and the colour gold shows.

**Empty is not broken.** At tick 850 with the campaign paused, the panel draws three blank lines —
and so did the scaffold: `[teletype] intel.latest=0 wrap=0 shown=0`. There are simply no intel
messages until the clock runs, which is BOB-PO-9's territory. Recorded because a blank panel is
exactly the kind of thing that gets "fixed" twice.

**Verified with the clock running** (Play clicked, tick 1100): the real controls render the log in
gold's layout — light-grey plate, place name in the left column, message in the right, dark text:

```
Creil          Aircraft Quota Allocated
Colombert      Aircraft Quota Allocated
```

against gold's `…werp / Aircraft Quota Allocated`. Two lines rather than three because only two
messages exist at that point. `dlg=1065:8/8` in the same run (was `0/8`).

## BOB-PO-4 (two mouse icons on the M view) — one theory checked and eliminated

The obvious port explanation is the classic one: the game hides the OS cursor because it draws its
own, the port's hide is a no-op, and you see both. Half of that is true —
`compat_winuser.h`'s `ShowCursor` maintains Windows' display counter exactly (so
`while (ShowCursor(FALSE) >= 0);` terminates) and **never tells SDL**, so it cannot hide anything.
An inert stub of precisely the shape this port keeps getting caught by.

**But it is not this defect:** grepping the whole game tree for `ShowCursor(` finds *no* caller —
the only hits are an unrelated `m_bShowCursor` member in `REDIT/REDITCTL.CPP`, all but one of them
dead-coded. The game never asks for the cursor to be hidden by that route, so a working
`ShowCursor` would change nothing here.

Left as-is deliberately: wiring `SDL_ShowCursor` into a function nothing calls would be an
unverifiable change that looks like progress. Noted in case a later screen does call it.

Next place to look for the real cause: whatever draws the in-flight/map cursor sprite (the
`AU_UI_X`/`AU_UI_Y` UI cursor in `ANALOGUE.CPP`) — two *sprites*, rather than one sprite plus the OS
pointer. Needs the M view, which is not currently reachable headlessly.

## BOB-PO-6 (numpad-Enter does not show the instrument panel) — checked against the SHIPPED keymap

Not a broken key-delivery path, and probably not a port defect at all.

- **Delivery is fine.** `sdl_to_dik` maps `SDL_SCANCODE_KP_ENTER -> 0x9C` (`DIK_NUMPADENTER`), so the
  key reaches DirectInput. `KEYMAPS.H:656` defines `Raw_J_enter = DIK_NUMPADENTER`.
- **Nothing is bound to it.** Its only `KeyMap` entry in the source is **dead-coded**:
  `//Dead KeyMap(DROPBOMB, J_enter, norm)`.
- **The installed game agrees.** `KEYBOARD/keys.xml` — the shipped keymap, a better oracle than the
  source drop — contains **no `J_enter` mapping at all**, while other keypad keys are present
  (`J_delete`, `J_insert`, `J_lockscr`, `J_move*`, `J_pageup/down`, `J_sysreq`). Plain `enter` is
  `PADLOCKTOG`.
- **The panel toggles are elsewhere:** `INFOPANEL` = `i`, `HUDTOGGLE` = `h`.

So numpad-Enter does nothing in this build by design. Worth asking the PO to confirm on `i`
before any binding is invented — adding one would be changing the game's controls, not porting them.

One part still unexplained and worth a look: the PO reports numpad-Enter *does* something ("super
zoomed in view of cockpit"). An unbound key should be inert, so either another key is being
delivered for that scancode or a view/zoom action has a fallthrough. That, unlike the binding, would
be a genuine port bug.

## BOB-PO-11 — ROOT CAUSE: one open dialog disabled every click on the map

The S173c lead (undrawn log lines) was real but **not sufficient**: adding `TB_REPORT` to the click
walk changed nothing, and a three-arm test said so — one click, two clicks and no click all produced
an identical `dialogs=6`. The prediction that the fourth toolbar would restore the toggle was wrong,
and the test is what said so rather than a second guess.

The actual cause is one line in the compat layer:

```c
void GetWindowRect(LPRECT r) const { if (r) {
    int w=0,h=0; bob_gdi_screen_size(&w,&h); r->left=r->top=0; r->right=w; r->bottom=h; } }
```

`CWnd::GetWindowRect` answers with **the whole screen, for every window**. S156's rule — *a click
that lands on an open OOB dialog but hits no control is swallowed, so a dialog's background does not
select the map behind it* — implements that test as

```c
CRect r; d->GetWindowRect(r);
if (cx >= r.left && cx < r.right && cy >= r.top && cy < r.bottom) return 1;   /* swallow */
```

which reads `cx >= 0 && cx < 1024 && cy >= 0 && cy < 768`: **true for every pixel on the screen.**
So from the moment *any* OOB dialog is open, every map click is discarded before reaching the accel
row, the two icon rows, the event log, or unit selection. The whole map goes dead, and the only way
back is a route that does not go through this dispatch.

That is the PO's report exactly — *"clicking red icon at bottom again does not dismiss dialog"* — and
it is not specific to the red icon or to dismissing: **nothing** is clickable once a dialog is open.
It also explains why the log-line arms did nothing: the campaign opens the LW Directives dialog
(`dlg=1032`) by itself once the clock starts, so by tick 1000 every click was already being eaten.

The Play click in S173 worked only because it happened at tick 900, before that dialog appeared —
a sobering detail: the earlier fix verified green on a recipe that got in just ahead of this bug.

**Fix (S173d).** The swallow region comes from what the dialog *drew* — the union of its subtree's
paint-recorded control rects (`bob_ole_drawn_bounds`, unioned over the same nodes
`bob_oob_click_tree` walks), so the swallow region and the hit-test region are built from one
traversal of one set of rects and cannot drift from what the player sees. If a dialog drew nothing
hit-testable, the click is **not** swallowed: letting it through is the recoverable failure, eating
it is not.

**Known limitation of that fix, stated rather than discovered later.** The union is over the
*controls* a dialog drew, not over its background art, so a dialog whose art extends past its
outermost control has a margin where a click still falls through to the map instead of being
swallowed. That is a narrower miss than the old behaviour by a wide margin — a few pixels of border
versus the entire screen — and it errs in the recoverable direction. Tightening it properly means
recording the art's drawn extent at `DoPaint` time alongside the controls', which is the same
paint-records-its-own-geometry idiom and worth doing when a symptom actually points at it.

Also: the swallow's own trace was gated behind `BOB_TRACE_OLE`, which is unusable for click
questions (per-control-per-frame; it once wrote 70 MB and starved a run). So the single most
consequential thing this dispatch does — *discarding* a click — was the one thing it never reported.
It is unconditional now, and the toolbar dispatch gained a `[tbclick]` line, because "did the click
land?" and "did the handler decline?" are opposite bugs that a dialog count cannot tell apart.

### BOB-PO-11 — verified

Three arms, identical coordinates, with the swallow fixed:

| arm | clicks reaching the log | result |
|---|---|---|
| one click  | `[tbclick] (200,657) consumed by event log (TB_2) ctrl id=1291` | `dialogs=7`, new `dlg=1087` — **opened** |
| two clicks | consumed twice, id=1291 | `dialogs=6`, `dlg=1087` gone — **closed** |

`OnClickedLine1`'s open→close toggle end to end, and `1291` is `IDC_LINE1` — the trace names the
control rather than leaving it inferred, which is why it was worth adding.

The first arm also exposed the next omission immediately: `dlg=1087:**0/15**`. The dialog opened and
drew nothing, because `bob_map_paint_oob` walked `TB_MAIN` and `TB_MISC` while the game logs children
on **three** toolbars. The paint walk and the click walk have to agree about the *set* as well as the
geometry — the same omission this sprint had already fixed on the click side an hour earlier, which
is a fair sign the rule deserves to be checked as a pair every time either side gains a member.

With `TB_REPORT` added to the paint walk too, `dlg=1087` goes **0/15 -> 14/15** and the dialog it
opens is the **Messages** panel: the filter tickboxes (Flight / Engagements / Spotting / Target /
Management, drawn with the red ticks the S173 icon-numbering fix restored) over a Time / Location /
Message table with the clicked entry highlighted —

```
06:30  Creil        Aircraft Quota Allocated     <- the clicked line, in red
06:30  Colombert    Aircraft Quota Allocated
```

which is what clicking an event-log line is supposed to open. Feature restored end to end: the line
draws, takes the click, opens its dialog, the dialog paints, and a second click closes it.

## BOB-PO-2 — REPRODUCED headlessly (S173e), and the recipe that does it

```
cd "<game dir>"
gl-lock env DISPLAY=:0 BOB_BOOT_FRONTEND=1 BOB_QM_INDEX=11 BOB_AUTOFLY=view40 \
  BOB_DUMP_FRAME=220 BOB_DUMP_PATH=/tmp/turkey_ext.ppm BOB_EXIT_AFTER_DUMP=1 ./build/bob
```

`BOB_QM_INDEX=11` is the turkey shoot (`[boot] QM: idx=11 title=2237` = `IDS_MISTYPE_DOGFIGHTING`);
`BOB_AUTOFLY=view40` presses F6 for the external view once flight is live.

**Cockpit arm** (`doc/img_turkey_cockpit_repro.png`, frame 150): `Alt 9524ft Speed 301Kts Power 90`
— airborne at the scenario's `FT_10000`, gunsight and instruments correct, sky and horizon clean.

**External arm** (`doc/img_turkey_tiles_repro.png`, frame 220): `Alt 9539ft Speed 300Kts`. The
aircraft and everything **above** the horizon render correctly. **Below** the horizon the ground is
black-to-dark-grey **flat rectangular blocks** at differing shades with visible tile seams, plus one
lighter grey rectangle low-right — the PO's "patches, one black … tiles at wrong angles".

So the defect is now reproducible on demand, in one command, without a play session. What it is not
yet: diagnosed. The next cheap discriminators, in order —

1. **Is it altitude or scenario?** Run a *familiarisation* airborne entry with the same external
   view. If its ground is equally black, this is not turkey-specific and the search collapses to
   terrain streaming at altitude.
2. **Is it run-to-run stable?** Two identical runs, hash the frames. Variance is this port's
   signature for an uninitialised read fed by a stub — it would collapse the search immediately.
3. **Does the cockpit view show it too?** The cockpit arm above never points at the ground; a
   nose-down attitude or a look-down view separates "the terrain is wrong" from "the external-view
   scene setup is wrong" (the latter has already needed fixing once, S118/S119).

### BOB-PO-2 — two of the three discriminators run

**1. Altitude, not scenario.** Familiarisation free flight (`BOB_QM_INDEX=7`, same external view) at
**`Alt 1052ft`** renders the near ground *beautifully* — green fields, hedgerows, roads, a ploughed
brown field — with a **black band along the horizon** carrying the same dark rectangular blocks.
The turkey shoot at **`Alt 9539ft`** shows that same blackness filling the whole ground.

So it is **not turkey-specific**, and the unifying statement is: **distant terrain renders black**;
altitude only decides how much of the screen counts as distant. At 1000ft most of the view is near
terrain (correct) and only the horizon strip is black; at 9500ft essentially everything visible is
distant, so the entire ground is black. That reframes the report from "tiles at wrong angles" to
"the far terrain is not being shaded/textured", which is a different and much narrower search.

**2. Run-to-run: the ground pattern varies, its darkness does not.** Two identical turkey runs,
frame 220, differ by:

| region | pixels differing >8 | of |
|---|---|---|
| sky    | 1,040   | 216,000 (0.5%) |
| ground | 24,087  | 240,000 (10%)  |

with ground mean brightness **32 vs 31** — i.e. both runs are equally black and the *pattern* is what
moves. Non-determinism concentrated in exactly the failing region is this port's signature for an
uninitialised read fed by a stub.

**Caveat, stated because it would be easy to skip:** terrain **streaming** is time-dependent, so a
frame-220 difference is also consistent with "different tiles had finished loading". That benign
explanation has not been excluded. The discriminator is a **later frame** (e.g. 400–600, after
streaming settles): if the ground still differs run-to-run there, streaming lag is out and an
uninitialised read is in.

**3.** Cockpit-vs-external is still unrun.

**Adjacent prior art, not yet linked to this.** `PORT.md` records a game-side
`LandScape::InfiniteStrip` (LANDSCAP.CPP:7427) horizon-UV defect — garbage `v` texcoords derived from
`cloud_height_*` / sky-layer math — but that was diagnosed in the **mirror** path
(`RenderMirrorLandscape`), which is dormant by default. Whether the main view's distant strip shares
that derivation is a question, not a conclusion; it is the first place to look precisely because the
geometry involved is the same horizon backdrop.

### BOB-PO-2 — discriminator 2 settled: the distant terrain is NON-DETERMINISTIC geometry

Frame 500, streaming settled, two identical runs:

| region | pixels differing >8 | of |
|---|---|---|
| sky    | 1,428  | 216,000 (0.7%)  |
| ground | 49,650 | 240,000 (**20.7%**) |

The variance did not settle — it **doubled** (10% at frame 220). Before reading anything into that,
the obvious confound: by frame 500 the two aircraft could simply be somewhere different, which would
change the ground for an entirely innocent reason. The HUD answers it —

```
run 1:  Alt 9648ft  Hdg 0  Speed 296Kts  Power 90
run 2:  Alt 9649ft  Hdg 0  Speed 296Kts  Power 90
```

**One foot** of altitude, same heading, same speed. A 1 ft camera shift at 9,600 ft cannot move 20%
of ground pixels, and the sky — same camera, same frame — moves 0.7%. So the ground content genuinely
differs with the camera held still. Streaming lag is out.

**What differs is large flat QUADS, not tile textures** (`doc/img_distant_terrain_variance.png`, run 1
above, run 2 below): run 1 carries a wide black band across the lower ground and a light grey
parallelogram at the right; run 2 has neither, and a dark parallelogram elsewhere. Big polygons
appearing, moving and vanishing between runs. Also present in both: cloud-like grey material
rendering **below** the horizon.

That changes the leading hypothesis. "Tiles at wrong angles" reads like texture/LOD; what the
evidence shows is **geometry with unstable coordinates** — and this port already has a diagnosed
instance of exactly that, in exactly this geometry. `PORT.md`: `LandScape::InfiniteStrip`
(LANDSCAP.CPP:7427) builds the horizon strip from `cloud_height_*` values derived from
`MissManCampSky().Layer[0].AltBase/AltTop` and double-precision sky math, and in the mirror path that
derivation was measured producing garbage texcoords (`v ~ -2.4e24`). The corroboration here is the
cloud material below the horizon: the same sky-layer data feeding the same backdrop.

**Still a hypothesis, not a conclusion.** The mirror finding was about *texcoords* in a *dormant*
path; this is about *positions* in the main view. The test that would settle it is to dump the
horizon strip's vertex coordinates over two runs and see whether they differ — the same
"measure the state, not the picture" move that settled the clock. That is the next step, and it is
now cheap, because the scenario is reachable in one command.

### BOB-PO-2 — the InfiniteStrip hypothesis is REFUTED (S173f)

Measured rather than argued. `BOB_TRACE_HORIZON=<call index>` dumps the horizon strip's whole input
state once per run; two runs, frame 500, call 450:

```
run 1  viewer_y=308854 fvy=0.168883421  layer0 AltBase=0 AltTop=0  fogCol=90b8e8 skyBase=77a9e2  pitch=1164
run 2  viewer_y=308860 fvy=0.168886702  layer0 AltBase=0 AltTop=0  fogCol=90b8e8 skyBase=77a9e2  pitch=1163
```

Three conclusions, in order of how much they change the search:

1. **`fogCol` is `90b8e8` — light blue, not black.** `InfiniteStrip` does
   `g_lpLib3d->Wipe(fogCol.all)`, so the frame is cleared to light blue. The black ground is
   therefore **not** the background showing through where terrain failed to draw: something is
   actively drawing black over the lower screen. That inverts the natural reading of the symptom.
2. **The horizon inputs are stable.** Same `fogCol`, same `skyBase`, same layer data across runs.
   The only differences are sim-timing noise — `viewer_y` differs by 6 units of 308,857 (0.002%),
   pitch by one, `sunProportion` in the sixth decimal. Nothing there can move 20% of ground pixels.
   **So `InfiniteStrip` is not the source of the non-determinism**, and the prior-art link flagged
   last session is refuted for the main view. Good: it was the cheapest hypothesis to kill, and
   killing it removes the whole horizon/sky-math branch from the search.
3. **`layer0 AltBase=0 AltTop=0`** — the cloud layer's altitudes are both zero, so
   `cloud_height_top`/`cloud_height_bottom` are `0.000000000`. The band-selection branch
   (`layerAltitudeBottom <= 0 || viewer_y < layerAltitudeBottom`) then *always* takes "view below
   cloud layer", so it does not crash — but a sky layer with zero extent is almost certainly not
   what the mission data means, and it is the same shape as this port's other silent-default bugs.
   Logged as its own question, not merged into BOB-PO-2.

**Where this points next:** the landscape tile draw itself, not the backdrop. Since the wipe is light
blue and the lower screen is black, the far terrain is being drawn *with black* (unlit / untextured /
wrong material) rather than skipped. The next measurement is the same shape as this one — dump what
the land draw is handed (per-tile colour/texture/light state) for two runs and compare — rather than
another look at the picture.

### BOB-PO-2 — CHARACTERISED (S173g): tiles converge, far too slowly

Measured the black-patch coverage of the ground region across frames, turkey shoot, external view:

| frame | near-black ground | ground mean RGB |
|---|---|---|
| 220 | 35.6% / 35.8% (two runs) | (24, 27, 28) |
| 300 | 17.8% | (34, 39, 40) |
| 500 | 12.9% / 1.9% (two runs) | (59, 65, 68) / (62, 68, 72) |
| 800 | **0.0%** | (73, 82, 89) |

**It is not corruption. It is convergence.** Tiles start black and fill in monotonically; by frame
800 — roughly 13 seconds at 60 fps — the ground is fully textured and no near-black pixels remain.
Everything earlier in this investigation that looked like a defect in the *content* was really the
same curve sampled at different points:

- "the ground is black at altitude" — frame 220, 35% black, early on the curve;
- "the pattern is non-deterministic, 20% of ground pixels differ" — frames 500 vs 500 at **12.9% vs
  1.9%**, i.e. two runs at different points on the *same* curve, not two different pictures;
- "large flat black quads move between runs" — individual tiles crossing from black to textured at
  different moments.

So the run-to-run difference is variance in the **rate**, not in the result. That is a much less
alarming defect than "uninitialised read", and I had it pointed the wrong way for two rounds: the
frame-220 variance measurement was real but I read non-determinism into a curve I had only sampled
twice, at one frame. **Sampling a time-varying process at one instant and calling the difference
non-determinism is a mistake worth naming** — the fix was to sample the axis, not to argue about
the two points.

**Both render targets are healthy**, which rules out the composition path: the sea RTT is a proper
dark blue-green water surface and the land RTT (familiarisation, inland) is fully detailed farmland
with fields, hedgerows and tracks (means 50 and 46). Whatever is black on screen is a tile that has
not yet been given its texture, not a tile given a broken one.

**And it matches the PO exactly.** Round 1: "one black patch". Round 2, after S172: *"tiles seem
better, but still update too slowly"*. The second report is the accurate one, and it is now a
measurement rather than an impression.

**Next: find what paces tile creation.** The suspects are `SRC/3D/TILEMAKE.CPP` (the tile
compositor, which calls `UploadTexture`) and whatever budgets how many tiles it may build per frame.
The question to answer first is whether the pacing is the game's own design (a deliberate
per-frame budget, tuned for 1999 hardware and now mis-scaled) or something the port added — because
those have opposite fixes.

### BOB-PO-2 — four hypotheses eliminated; the terrain itself is drawn black (S173h)

Each of these was killed by a measurement, not by argument:

| hypothesis | killed by |
|---|---|
| `InfiniteStrip` horizon geometry (the prior-art link) | its inputs are stable across runs and its colours are all light blue |
| The screen wipe showing through where terrain failed | `Wipe(fogCol)` and `fogCol=90b8e8` — light blue, not black |
| Tile-cache capacity stall | `stillWaiting=0` from call ~120 onward; nothing is queued for building |
| Broken render-target composition | sea RTT is a proper water surface, land RTT is fully detailed farmland |
| Cloud-layer altitude (`layer0 AltBase=0 AltTop=0`) | **correct**: the turkey shoot is `weather=0`, and `Cloud::SetCloud` calls `NullCloud()` when cover is 0. Retracted as a lead. |

What is left is forced. The frame is cleared to light blue; every backdrop band is light blue
(`grHorizCol=a2c3ec`, `midCol=99beea`, `fogCol=90b8e8`); and a large region is nevertheless **pure
black with straight-edged, polygon-shaped internal structure** (native-resolution crop: a hard
horizontal boundary with parallelogram quads inside it). Nothing can produce that except geometry
drawn *over* the backdrop. **The terrain polygons are being drawn black**, and they brighten to
correct over ~13 seconds.

Since tile *creation* is idle throughout (`stillWaiting=0`, `freeTiles` all zero — a full,
steady-state cache), the thing converging is not which tiles exist. The two candidates left are what
the terrain polygons are *shaded* with (vertex lighting arriving dark and ramping) and what they are
*textured* with (a landscape texture handle not yet bound, sampling black). Both are directly
measurable in the same style as everything above — dump the per-tile light/texture state across
frames — and that is the next step.

**Method note.** Five rounds of this investigation each produced a confident-looking wrong answer:
black terrain, then non-deterministic geometry, then InfiniteStrip, then a capacity stall, then the
cloud layer. What moved it forward every time was measuring the state that would *distinguish* the
candidates rather than looking harder at the picture — and being willing to write down that the
previous round's answer was wrong.

### BOB-PO-2 — two more eliminations, including one seductive coincidence

**Untextured tiles: measured, and they are not the cause.** `BOB_TRACE_TILES` now also counts cache
entries whose `textHandle == HTEXTNULL` (such a tile is skipped by `Render2Surface` and can only draw
untextured):

```
call   0: noTexture=261/289      call 120: noTexture=33/289
call  60: noTexture=144/289      call 180+: noTexture=33/289   (plateau, permanent)
```

Tiles get textured quickly up to call ~120, then **33 of 289 stay untextured forever**.

`MAX_LAND_TEXTURES = 256` and the tile grid is `_wholeAreaSizeMIN` 17 × 17 = **289**. And
**289 − 256 = 33**. That is a very persuasive arithmetic match for "the texture pool is too small by
exactly the number of black tiles", and `Lib3D::AllocateLandscapeTexture`'s exhaustion path even
returns `NULL_LAND_TEXT_REF` behind an `INT3` that this port compiles out (`-DNDEBUG`) and that is a
no-op on Linux anyway — a doubly silent failure of precisely this port's signature kind.

**It is a coincidence.** Counting the failures directly (`texAllocFail` per band, incremented on the
exhaustion path) gives **0 for the whole run, every band**. The allocator never fails. Those 33
tiles are never *requested* a texture — `stillWaiting=0` — so they are idle cache slots, not denied
ones. Recorded at length because the arithmetic was good enough to act on, and the counter is what
stopped it: **a number that fits is not a mechanism that fired.**

**Eliminated so far:** InfiniteStrip geometry; the screen wipe; tile-creation capacity; RTT
composition; cloud-layer altitude; texture-pool exhaustion; untextured tiles.

**What is left:** the terrain polygons are drawn over a correct light-blue backdrop, with textures
successfully allocated and composited, and they are still black — so the remaining variable is what
they are *shaded* with. `Render2Surface` runs under `SetObjectLighting(useLightFlag)` and the strip
sets `LF_VERTEX`; terrain vertex lighting arriving near-zero and ramping over ~13 s fits every
measurement taken. That is the next thing to dump, in the same style: the light state handed to the
terrain draw, sampled across frames.

### BOB-PO-2 — a confound in my own measurement, checked; and lighting eliminated

**Lighting does not ramp.** Sampling the strip's colours every 100 calls for a whole run gives
`grHorizCol=a2c3ec midCol=99beea fogCol=90b8e8` — *identical at every sample*. So the "brightening"
is not the lighting model converging. That was the last hypothesis I had named, and it is dead too.

**A confound in my own numbers, found and corrected.** The earlier convergence table measured a
*fixed screen band* (rows 55–93%). But pitch changes markedly during the run (1492 at call 200 →
1164 at call 450) and the horizon moves with it, so that band contained different content at
different times — "black fell to 0%" could have meant "sky moved into my window". Re-measured with
the horizon **detected per frame** (steepest brightness drop) and only the region below it:

| frame | horizon row | below-horizon mean RGB | near-black |
|---|---|---|---|
| 220 | 313 | (26, 29, 30) | 34.2% |
| 300 | 324 | (34, 39, 40) | 17.6% |
| 500 | 348 | (49, 52, 54) | 14.2% |
| 800 | 374 | (49, 55, 56) | **0.0%** |

The horizon does move (313 → 374), so the confound was real; correcting for it leaves the
conclusion standing. Pure-black patches genuinely go to zero, and the residual is dark sea
(mean ~50), not black.

**But "resolved" and "left the view" are still not separated.** Nothing about the tiles changes
after call ~120 — the build queue is idle, no texture allocation ever fails, the untextured count is
pinned at 33, and the lighting is constant. So it is entirely possible the black patches are
specific *places* that render black, and that flying away from them is what removes them, rather
than anything getting fixed. That would also explain the run-to-run differences: slightly different
flight paths put different tiles in view.

**The discriminator** is a camera that does not move: hold position (or dump the same world region
from two attitudes) and see whether a given patch stays black. That separates "black tiles get
repaired" from "black tiles go off-screen", and those have completely different fixes. It is the
next thing to run, and it is cheap.

### BOB-PO-2 — CORRECTION: it is the NEAR terrain that is black, not the distant terrain

I had this backwards, in several commits, stated confidently. Splitting the below-horizon region
into a band just under the horizon (far terrain) and a band well below it (nearer terrain):

| frame | far band (just below horizon) | near band (well below) |
|---|---|---|
| 220 | mean (57,61,62) — black **0.1%** | mean (14,15,15) — black **61.2%** |
| 300 | mean (84,87,89) — black **0.0%** | mean (12,15,15) — black **49.0%** |
| 500 | mean (75,79,81) — black **0.0%** | mean (31,34,34) — black **36.8%** |
| 800 | mean (74,78,80) — black **0.0%** | mean (30,36,36) — black **0.0%** |

**The distant terrain is consistently fine** (0% black at every frame). The black is in the nearer
band. Every statement of the form "distant terrain renders black" in the entries above is wrong and
should be read as its opposite.

That also explains the familiarisation arm, which I had filed as agreeing with the old story: at
1052 ft the near ground was correct farmland and the *horizon* carried a black band. Under the old
reading those two arms agreed; under the corrected one they disagree, and the thing that actually
distinguishes them is what is underneath the aircraft — **the turkey shoot is over the Channel
(sea), the familiarisation is inland (land)**. The sea RTT dump is consistent with this: it is a
real water surface but a dark one (mean 50).

**Honest state of this investigation.** Seven hypotheses have been eliminated by measurement
(InfiniteStrip, the wipe, tile-creation capacity, RTT composition, cloud altitude, texture-pool
exhaustion, untextured tiles, lighting), one arithmetic coincidence was caught by a counter, one
confound was caught in my own method, and the headline geographic claim turned out to be inverted.
What is solidly established:

- the scenario is reproducible in one command, airborne, on demand;
- the backdrop, the wipe, the lighting and the render targets are all correct;
- terrain polygons near the aircraft draw black, and the effect is strongest early;
- it correlates with being over water rather than over land.

**Next, and deliberately narrow:** does a *sea* tile render black where a *land* tile does not?
Fly the familiarisation scenario out over the coast, or the turkey shoot inland, and measure the
near band in each. That is one variable, it is the one the two arms actually differ in, and it does
not depend on any of the eliminated machinery.

### BOB-PO-2 — the limit of screen-band statistics, and where to pick this up

Applying the same banded measurement to the land arm inverts it again:

| arm | far band (just below horizon) | near band (well below) |
|---|---|---|
| turkey, over sea, 9,500 ft | mean (57–84) black **0.0–0.1%** | mean (12–31) black **36–61%** |
| familiarisation, inland, 1,052 ft | mean (9,9,4) black **82.6%** | mean (39,39,10) black **2.9%** |

The two arms are **exact opposites**, so neither "the distant terrain is black" nor "the near
terrain is black" survives as a description. And the colours show why the question is malformed:
turkey's "far band" reads (74,78,80), a grey-blue haze, which is not ground at all — at 9,500 ft the
rows just under the horizon are atmosphere, while at 1,052 ft they are land. **The bands do not
reliably correspond to the same thing in the two arms**, so comparing them was never going to
settle anything.

That is the honest stopping point for this method. Screen-region statistics were the right tool to
establish *that* something is wrong, to kill eight mechanisms, and to catch two errors of my own —
but they cannot identify *which surfaces* are black, because a screen band is not a surface.

**Pick this up with a different kind of measurement, not another band.** The question is "which
geometry draws black", and it should be asked of the draw path directly: tag the terrain draw with
what it is about to render (tile index, its texture handle, its world position, land-vs-sea) and
dump that for the polygons covering a known-black screen region. The port already has the hook
shape for this — `draw_fvf` per-quad tracing was used for the mirror-horizon work — so this is
plumbing, not research.

**What is banked and reliable:**

- a one-command headless repro of an airborne dogfight (`BOB_QM_INDEX=11 BOB_AUTOFLY=view40`),
  which the notes previously recorded as impossible;
- three env-gated diagnostics that default off — `BOB_TRACE_HORIZON` (strip inputs + band colours,
  periodic), `BOB_TRACE_TILES` (queue depth, per-rez free slots, untextured count, allocation
  failures), and the existing `BOB_DUMP_RTT`;
- eight eliminated mechanisms, each with the measurement that killed it;
- the fact that the backdrop, wipe, lighting, render targets and texture allocation are all
  **correct**, which is worth as much as a positive finding: it means the defect is in what the
  terrain draw does with them.

### BOB-PO-2 — ROOT CAUSE CANDIDATE (S173l): the low-resolution land textures are black

Asking the draw path directly, as the previous entry said to. Three measurements, each forced by
the one before:

1. **The frame is cleared light blue.** `[clear] col=0x0090b8e8 flags=1` — that is `fogCol`. So
   uncovered pixels cannot read black.
2. **Vertex colours are fine.** `BOB_TRACE_COL` per texture-width bucket: mean diffuse 100–173
   across every bucket (`tex256w` 4258 quads at (173,173,174), `tex64w` 411 at (101,100,100), …).
   Nothing is drawn with a black vertex colour.
3. Since colour = texture × diffuse, the **texture** must be black — so classify the bound texture
   per quad (`BOB_TRACE_TEXBLACK`, sampling each surface's bits once and caching by surface):

```
quads: blackTex=12582  darkTex=108  normalTex=287304  noTex=6   (126 distinct surfaces)
  blackTex dims 128x128 quads=2869
  blackTex dims  64x64  quads=9713
```

**The black textures are 64×64 and 128×128 — the LOW-RESOLUTION landscape tiles.** The 256×256 land
texture is not among them, which matches the RTT dumps: the 256 I captured was a proper water
surface in the turkey shoot and proper farmland in the familiarisation arm.

**And `BOB_DUMP_RTT` dumps exactly one surface.** The FBO render-to-texture path — recorded in
CLAUDE.md as the change that took ground from 51% to 99% non-black — appears to cover only one
texture size. `Render2Surface` renders each tile at `7-rez`, so the 64 and 128 tiles are rendered
into targets that are not FBO-backed, their output goes nowhere, and they stay black. Quads drawn
with them are black regardless of correct geometry, correct vertex colour and a correct clear.

This also retro-explains the whole earlier muddle. Which parts of the screen use low-rez tiles
depends on view distance, altitude and attitude — so the black region moved when the aircraft
pitched, differed between the sea and land arms, differed between two runs at slightly different
positions, and shrank as the aircraft climbed and more of the view fell to the 256 tiles. Every one
of those observations was real; none of them was the cause.

**Next (a fix, not a measurement):** confirm that only one land-texture size gets an FBO, then give
the other sizes the same RTT treatment. The A/B is already defined — `BOB_NO_FBO_RTT` reverts the
existing path, and `blackTex` counts give a numeric pass/fail rather than an impression.

### BOB-PO-2 — the mechanism, confirmed (S173m)

`BOB_TRACE_RTT` over a turkey-shoot run:

```
[rtt] created FBO=1 tex=44 256x256 complete=1      <- exactly ONE FBO, and it is 256x256
[rtt] ACCEPTED render-target texture 256x256   x1
[rtt] ACCEPTED render-target texture 128x128   x2  <- accepted as RTT, no FBO ever created
[rtt] Lock readback 256x256                    x20 <- readback only ever at 256x256
[rtt] SetRenderTarget -> RTT                   x40
[rtt] SetRenderTarget -> MAIN (unbind fbo)     x40
```

Forty render-target binds and **one** FBO. The engine composites each tile into a single scratch
render target and reads it back to upload into that tile's own texture (`Render2Surface` ends with
`EndScene` + `UploadTexture`); the readback path is servicing **only 256×256**. So tiles whose own
texture is 64×64 or 128×128 receive nothing, and that is exactly the set measured as black:

```
blackTex dims 128x128 quads=2869
blackTex dims  64x64  quads=9713      (256x256 is NOT in the black set)
```

Two independent measurements — which textures are black, and which sizes the RTT path services —
name the same set. That is the mechanism.

**Why the symptom looked so unstable.** Which screen regions use 64/128 tiles depends on view
distance, so the black moved with pitch and altitude, differed between the sea and land arms,
differed between two runs a few feet apart, and receded as the aircraft climbed and more of the view
fell to 256-tiles. All of that was downstream of one thing: the low-resolution tiles never get their
texture.

**The fix is real work, not a one-liner**, and should be sized as its own story: give every land
texture size the same render-to-texture treatment the 256 gets (or make the readback/upload path
size-general). The A/B is already defined and numeric — `BOB_NO_FBO_RTT` reverts the existing path,
`BOB_TRACE_TEXBLACK` gives `blackTex` counts that must go to zero, and the frame-800 near-band
near-black percentage is the picture-level check.
