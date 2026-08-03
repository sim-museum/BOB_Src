# Cross-port note 27 — MA → BoB (2026-08-02, MA Sprint 70)

**Subject: the OOB (out-of-briefing) dialog draw path can silently lack a control-type case —
ours had no listbox, so tabular content in a tabbed OOB dialog never drew. Plus: the listbox
opaque-fill is load-bearing for the front-end menu, unlike the combo.**

## 1 — A missing `case` in the OOB draw loop = a whole control invisible

Our OOB dialogs (the campaign map's Player Log / Squads / etc.) render through a dedicated walk
(`ma_map_paint_oob` → per node `ma_ole_draw_toolbar`), *separate* from the front-end's
`ma_ole_draw_all`. `ma_ole_draw_toolbar` dispatched on control type — STATIC/EDIT/EDTBT/TABS/
BUTTON/COMBO — but had **no `CT_LISTBOX` case**. So the Player Log Career tab's stats table (an
RListBox populated in `CCareer::OnInitDialog`) existed, was positioned, was populated, and was
simply never drawn. The Name edit box on the same tab rendered, which is what made it look like
a data/population problem rather than a missing draw case.

If your OOB/briefing dialogs go through a different draw path than your front-end controls
(ours do), audit that path's type switch against the full control set — a control that renders
fine on the front end can be missing from the OOB switch. The fix was one case mirroring the
front-end listbox draw (drive `CRListBoxCtrl::OnDraw` at the control's toolbar-offset rect via
the DC viewport origin).

## 2 — ★ The listbox opaque-fill is LOAD-BEARING for the front-end menu (unlike the combo)

Note 26 §3: skipping `CRComboCtrl`'s black `FillRect` on the no-artwork path made combos
translucent = gold, with no front-end cost. `CRListBoxCtrl::OnDraw` has the *identical* pattern
(`if (!artnum && …) FillRect(BLACK_BRUSH)`), and gold's Player Log tables likewise show the
photo through a translucent box — so the obvious move is to skip it there too. **Don't, at
least not globally:** the front-end MENU and the prefs value rows are the same `CRListBoxCtrl`,
and they *rely* on that opaque box — skipping it globally **erased the title menu** (1.6% of the
title screen, caught by the byte-identical sweep). The combo and the listbox look like the same
fix but aren't: the combo is only ever a value box over composited art; the listbox is also the
primary menu surface. A correct listbox fix needs to skip the fill on the OOB path only (a
context flag), which we deferred. Flagging so you don't repeat the regression if you carry
note 26 §3 across to your listbox.

— MA
