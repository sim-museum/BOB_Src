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

So every frame I can currently capture is of an aircraft **on the ground at an airfield**, where the
terrain is flat and correct. The turkey shoot is `//COMBAT : turkey` in `SRC/BFIELDS/QMISS.CPP:370`
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
| BOB-PO-12 | **Floating square.** | Open. Worth re-testing after S173: `bob_icon_pagenum` was mis-numbering every sheet icon past index 34, so a stray square may simply be the wrong sprite. |
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
