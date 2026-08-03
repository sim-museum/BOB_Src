# Cross-port note 28 — MA → BoB (2026-08-02, MA Sprint 71)

**Subject: the OOB-only context flag that note 27 §2 deferred — how to skip the listbox black
fill on the OOB path without erasing the front-end menu. Plus a residual that measured away.**

## 1 — The OOB-only listbox-fill skip (completes note 27 §2)

Note 27 §2 warned: `CRListBoxCtrl::OnDraw`'s opaque black fill is load-bearing for the
front-end menu, so you cannot skip it globally the way the combo fill was skipped (note 26 §3) —
doing so erases the title menu. But gold's OOB tables (our Player Log Career/Log tabs) want it
skipped so the dialog background shows through (translucency).

The fix is a one-int context flag, because the two cases go through different draw entry points:
the front-end listboxes are drawn by `ma_ole_draw_all`, the OOB ones by `ma_ole_draw_toolbar`.
Set the flag around *only* the OOB draw:

```c
// ma_olecontrol.cpp  (file scope)
extern "C" { int ma_oob_lb_draw = 0; }

// inside ma_ole_draw_toolbar's CT_LISTBOX case, around the OnDraw:
ma_oob_lb_draw = 1;
c->OnDraw(&dc, lbounds, lbounds);
ma_oob_lb_draw = 0;
```

```c
// RLISTBXC.CPP  OnDraw
extern "C" int ma_oob_lb_draw;
...
if (!ma_oob_lb_draw && (!artnum && m_FirstSweep != TRUE || !m_hWnd))
    pOffScreenDC->FillRect(rcBounds, BLACK_BRUSH);
```

`ma_ole_draw_all` never sets the flag, so the menu/prefs listboxes keep their opaque box
(front-end byte-identical), and the OOB tables composite over the panel = gold. If you carried
notes 26/27, this is the clean way to finish it.

## 2 — A residual that wasn't real (combo border colour)

We had carried "native combo border is rectangular light/white vs gold's rounded-blue" as a #2
sub-residual. Measured before touching it: `AXC_DARKEDGE == AXC_LITEDGE ==
AXC_CIRCULARCOMBOBOXCOLOR == RGB(103,132,198)` (blue), and `m_bCircularStyle` is `FALSE` and not
persisted — so both native and gold draw the *same* blue rectangular border + round button. A
matched-scale crop confirmed it; the "white" was the anti-aliased blue edge at our 800-res
capture vs gold's ~1280. Retired without a code change. Worth checking your own carried
"residuals" the same way — half of ours this epic dissolved on measurement (cf. S64).

— MA
