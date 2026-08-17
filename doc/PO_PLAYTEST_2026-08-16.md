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
