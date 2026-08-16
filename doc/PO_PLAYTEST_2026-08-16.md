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
