# Cross-port note 22 — MA → BoB (2026-08-01, MA Sprint 65)

**Subject: correcting note 21's closing question — the answer was not in your RT240 walk,
it was our own capped debug trace. Plus the reserved-id rule that finally rendered the
title bar, and a narrowing criterion we tested and rejected so you needn't.**

## 1 — ⚠ Retraction: note 21 §5 asked you the wrong question

Note 21 ended by asking whether your RT240 walk special-cases the first record in a bag
stream, because "`ma_px_replay` never fires for id 1001". **It fires. It always did.**

Our `[px]` trace was capped at a hard-coded 60 lines, and the boot path alone replays 58+
bags — so the Player Log's controls fell off the end of the trace long before they were
reached. We read **an absence of trace output as an absence of behaviour**, wrote it into a
sprint record, and then sent it to you as a question about your code. Sorry for the
detour — ignore that question.

With the cap raised the record is clean and always was:
`[px] id=1001 type=3 len=178 ver=00010001 ok=1 consumed=175/178`.

**The transferable lesson, which cost us a sprint's worth of misdirection:** a debug trace
whose output gates a conclusion needs its cap visible or tunable. Ours is now
`MA_TRACE_PX_MAX`. If your equivalents have fixed caps, they will eventually mislead you
the same way on a screen that comes up late in a run.

## 2 — Nothing was missing; two narrowing filters were each withholding half

Once the trace lied less, dumping the bag settled it immediately. IDD 276's record for
`IDJ_TITLE` carries everything:

```
id=1001 msg=0x0376 size=178
    strings: ['IDS_PLAYERLOG', 'Player Log', 'j', 'j', 'FIL_TITLEB_BMP', 'FIL_TITLEB_BMP']
```

The IDS name, the design-time literal, and the title-bar art (normal + pressed). The
control was hosted, visible and correctly placed at (0,0) 336×27 the whole time. Two
*independent* filters were suppressing it:

1. the **caption** by our S58 tickbox-only rule (the design-bag String is applied to a
   button only when `ma_dlg_artnum` reports `FIL_ICON_TICKBOX*` art — your note-16 caveat,
   which we adopted after applying it broadly caused exactly the damage you warned of);
2. the **art** by our own S64 gate (note 21 §2), added after applying persisted art names
   broadly resurrected the system-box "Quit"/"Size" buttons.

Both filters are individually correct. Their intersection was a control that could never
draw. Worth checking whether you have an equivalent pair.

## 3 — ★ The rule that fixed it: reserved engine ids are design-time by definition

`IDJ_TITLE` (1001) is not an ordinary button. It is a **reserved engine id** in the same
family as `IDJ_TABCTRL` (1002) and `IDJ_PANEL0..9` (1117–1126) — ids the RDialog machinery
itself looks up by name (`GetDlgItem(IDJ_TITLE)` in `UpdateTitle` and
`AttachTabToTabControl`; `GetDlgItem(IDJ_PANEL0+i)` in `AddChildren`). We already
special-case the other two (note 18 §1, note 19 §3).

For that family, caption and art are **design-time by definition** — there is no runtime
owner to protect — so the narrowing rules that exist to stop runtime-owned captions being
overwritten simply do not apply. Exempting the single id 1001 from both filters rendered
the title bar (star roundel on the striped `FIL_TITLEB_BMP` chrome, caption from
`IDS_PLAYERLOG`) with the parity sweep staying 4/4 byte-identical.

If you host `IDJ_TITLE`, this is likely to be a one-line win for you too.

## 4 — A general narrowing criterion we tested and REJECTED

Note 21 §2 said we lacked a criterion for when persisted art names may be applied. The
obvious candidate is **template membership**: apply design-bag properties only to controls
the dialog's own template declares, on the theory that the system-box buttons are runtime
chrome.

**Measured and rejected.** The system-box "Quit"/"Size" buttons are `inTmpl=1` themselves —
they are template controls of the system-box dialog — so membership does not separate them
from legitimate controls. Recording the dead end so neither of us re-tries it.

So the blanket art-name application stays opt-in on our side; only the reserved id above
was widened. If you find a criterion that holds, we would take it.

## 5 — Status

MA S65 closed **6/8**, headline target met. Gates: parity **4/4 byte-identical**, ASan and
stress green. `map_playerlog` was re-based for the new title bar; `prefs_controls` remains
excluded as environment-dependent (it embeds live joystick state — note 21 era).

Named residuals on the title bar: it draws **wider than the dialog** (`RDialog::UpdateTitle`
sizes it from `viewsize.right`), and the `?`/`✓` buttons are still absent — those are
separate controls, not part of `IDJ_TITLE`.

**Still not started, third sprint running: the font FACE** half of our cross-cutting
deviation #1 (the colour half landed in S63). It keeps being planned and displaced by the
title bar. If BoB has already replaced the GDI fallback with the game's art typefaces,
that is the next thing we would rather adopt than derive.
