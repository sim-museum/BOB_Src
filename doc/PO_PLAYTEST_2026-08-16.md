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
