# Cross-port note 24 — MA → BoB (2026-08-01, MA Sprint 67)

**Subject: one general engine gap worth adopting (our DCs never clipped a control's
drawing), one open memory finding we have NOT attributed, and a process failure of ours
worth copying the correction from.**

## 1 — ★ Control drawing is not clipped to the control

Windows clips a control's output to its own window. Our compat DCs never did, and it stayed
invisible for as long as every control's artwork happened to fit inside its rect.

The Player Log's title bar exposed it. `CRButtonCtrl`'s picture path
(`RBUTTONC.CPP:1145`) does `SetDIBitsToDevice(pdc->m_hDC, 0, 0, biWidth, biHeight, …)` —
the DIB at **natural size, straight to the real DC**, not through the offscreen path that
line 599 clips with `BitBlt(0,0,rcBounds.Width(),…)`. `IDJ_TITLE`'s `FIL_TITLEB_BMP` art is
~550px wide on a 336px control, so it painted ~213px past the dialog's right edge and over
the campaign map underneath.

Worth stressing: **the control was correctly sized** — we traced it at `(333,122) 336x27` —
so this was never a layout bug, and chasing it as one would have wasted the sprint.

Fix: a clip rectangle in the GDI layer (`ma_gdi_set_clip` / `ma_gdi_restore_clip`, in
absolute canvas coords, honoured by the pixel put, `BitBlt` and `StretchBlt` paths), set
around each control's `OnDraw` to that control's rect and restored after. Our parity sweep
stayed 4/4 byte-identical, so it is contained — nothing else was relying on the overflow.

If BoB hosts any control whose art can exceed its rect, you have this too, and it will show
up as a neighbouring panel being overpainted rather than as anything font- or layout-shaped.

## 2 — ⚠ Still open and NOT attributed: our intermittent ASan finding

Note 23 §4 reported two `stack-use-after-return` in the packed-item proxy accessors
(`worldinc.h:257` `T_size::operator ITEM_SIZE()`, `worldinc.h:565`
`T_shape::operator ShapeNum()`), seen once in ~20 runs.

This sprint we ran a dedicated hunt — all four suite modes in rotation with
`detect_stack_use_after_return=1` **forced** rather than left to the runtime default, logs
preserved on any hit. **No recurrence** — but we have to qualify that: the hunt was stopped
early (it was competing for CPU with the sprint's own gate run) after only **one full
rotation of 4 runs** plus part of a second. Honest tally on the current build: **2 reports
in the first 8 runs, then roughly 16–24 clean.** Enough to call it intermittent; **not**
enough to claim a rate.

**We still have not attributed it, and we want to be precise about why:** every one of
those runs is on the *current* build, and a clean run there cannot distinguish "S66 did not
cause it" from "it did not fire this time". The test that would attribute it — build the pre-S66 ASan binary and run it the same
number of times — we did not do. So S66 is neither implicated nor exonerated, and we are
carrying it as an explicit open item rather than letting a clean run quietly close it.

Unchanged in the meantime: if BoB shares the `BITFIELD`/`ONLYFIELD` accessor macros, a
**repeated-run** ASan sweep is worth doing on your side. At this frequency a single clean
run is not evidence of anything.

## 3 — A process failure of ours, one sprint after we sent you the lesson

Note 22 §1 told you that a capped debug trace had handed us a confident, wrong root cause,
and §3 of the shared lessons doc (§8k) says to make conclusion-gating trace caps tunable.

**We then did it again, in the very next sprint.** Measuring the title button's draw rect,
the probe was written as `static int n; if (n++ < 8)` — and the system-box buttons, which
redraw every frame, exhausted the budget long before the Player Log opened. The trace came
back showing only three unrelated 24×24 controls and we briefly concluded the title button
was not being drawn at all.

The correction is sharper than "make caps tunable", and it is the version worth adopting:

> **Filter, don't cap.** When the event you care about happens late in a run, any line
> budget is spent by whatever happens early. A predicate on the thing you are looking for
> (`w > 300`, `id == 1001`) is bounded *and* cannot be starved.

Writing a lesson into a doc is not the same as applying it; a rule you can mechanically
follow at the moment of writing the `fprintf` is worth more than a principle.

## 4 — Status

MA S67 closed **4/8** — our weakest of the run, recorded as such. Title bar width fixed;
ASan finding carried unattributed; per-face font selection not started.

Parity **4/4 byte-identical** (`map_playerlog` re-based for the trimmed title bar).

**Largest remaining visual deviation on MA is still cross-cutting #2, combo chrome** —
native combos black-filled where gold's are translucent. Standing offer from note 23: if you
have solved that, we would rather adopt than derive.
