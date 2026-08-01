# Cross-port note 19 — MA → BoB (2026-08-01, MA Sprint 61)

**Subject: four defects that all presented as "the dialog draws in the wrong place".
Three are in code we share; the fourth is a trap in how this port stubs Win32. If BoB
renders any `MakeTopDialog`/`AddChildren` tree, expect all four.**

Follow-up to note 18. That note fixed *what gets created* and *how big it is*; this one is
*where it lands*. Together they took MA's Player Log (gold shot #15) from a bare
pilot-photo blit at the top-left to a centred, tabbed dialog with a working tab bar.

---

## 1. ★ `ClientToScreen` / `ScreenToClient` are no-ops — and `OnGetXYOffset` is built on them

`RDialog::OnGetXYOffset` (`RDIALOG.CPP`) computes a dialog's screen origin like this:

```c
GetClientRect(rect); windowrect = rect; ClientToScreen(windowrect);
while (newparent->parent) {
    if (newparent->parent->artnum == artnum) {
        newparent->parent->GetClientRect(parentrect);
        newparent->parent->ClientToScreen(parentrect);
        offsetx = parentrect.left - windowrect.left;   // <-- 0 - 0
        ...
```

In MA's compat layer both `CWnd::ClientToScreen` and `ScreenToClient` are **empty
functions** — never needed while only full-screen panels rendered. With them inert both
rects stay client-relative and every subtraction is `0 - 0`, so **every dialog reports
offset ~0 and the whole tree composites at the top-left.** Check
`grep -n 'ClientToScreen' <your compat afxwin.h>` first; if they are stubs, this is live
in BoB too and it will look like a layout bug in whatever dialog you render first.

**We did NOT fix it by giving `ClientToScreen` real screen semantics.** It is called from
panel code that currently depends on the identity behaviour, and note 18's warning about
`CDialog::Create` was the same shape of mistake. Instead we replaced `OnGetXYOffset`'s
body under the port define with the accumulated origin up the RDialog `parent` chain
(tree children are MoveWindow'd with parent-relative rects; the top node carries the
absolute placement).

**If you do that, drop the title-bar nudge too.** The stock body ends with "if there's a
title on the main parent add an extra offset". In the accumulated form the title is
already accounted for (see §3 — each child is placed into a placeholder whose rect starts
below the title bar), so it double-counts. And because it is gated on
`top->fchild->artnum == artnum` it applies to the art-less tab host but NOT to the
art-bearing tab page, shifting them out of step by exactly the title height — we measured
tab host y=176 vs page y=179, so only the top 3px of the tab bar survived, painted over by
the page's own art. That one is nasty because it looks like a z-order bug.

## 2. ★★ Unchecked `RegQueryValueEx` + uninitialised locals → a different dialog origin every run

The highest-value find, and the one most likely to be silently wrong in BoB right now.

Startup reads the Windows metrics:

```c
DWORD type;  unsigned char buff[10];  DWORD size = 9;
RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\desktop\\WindowMetrics", 0, ..., &k);
RegQueryValueEx(k, "BorderWidth", NULL, &type, buff, &size);
if (type == REG_SZ) RDialog::borderwidth = atoi((char*)buff)/15;
else                RDialog::borderwidth = *(int*)buff/15;
RDialog::borderwidth = 2 - RDialog::borderwidth;
```

**The return code is never checked.** On Windows that is fine — those keys always exist.
In the port `RegQueryValueEx` is a stub returning `ERROR_FILE_NOT_FOUND` that writes
neither `type` nor `buff`, so `if (type == REG_SZ)` tests uninitialised stack and
`*(int*)buff` reads uninitialised stack. `borderwidth` then feeds `MakeParentDialog`'s
sizing (`dialsize.left + rect.right - 1 + borderwidth*2`), so **every top-level dialog
tree gets a garbage origin — a different one on each run.**

That run-to-run variance is the diagnostic. We measured `m_ma=(978990,978859)` on one run
and `(979004,978793)` on the next; a wrong-but-stable number would have sent us hunting a
logic bug for hours. `actscrw`/`actscrh` are read the same way in the same block.

Fix is one line each: zero `type` and `buff` before the queries (or check the return).
Then a failed query is deterministic and `borderwidth` lands on a sane `2 - 0`.

**Generalise the audit:** this is note 18 §5's lesson one level further out. Not just
ctor-skipped members — **any local passed as an out-parameter to a stubbed Win32 API and
then read without checking the return** is the same bug class. Worth a sweep for
`RegQueryValueEx`, `GetVersionEx`, `SystemParametersInfo`, `GetDeviceCaps` and friends.

## 3. `IDJ_PANEL0..9` placeholders: children stack BELOW the parent instead of inside it

`RDialog::AddChildren` decides where each child dialog goes:

```c
int uid = IDJ_PANEL0 + i;              // 1117..1126
CWnd* cntrl = GetDlgItem(uid);
if (cntrl) { cntrl->GetWindowRect(&posn); ScreenToClient(&posn); cntrl->ShowWindow(SW_HIDE); }
else       { posn.top = usedy; posn.right = dial->homesize.Width(); ... }   // stack below
```

The `IDJ_PANEL*` entries are plain **native** template controls (not OCXes) that draw
nothing and exist only to mark where a child goes. Note 18's template hosting creates OCX
kinds, so these were still missing, `GetDlgItem` returned NULL, and every child dialog got
**stacked below its parent**. On MA's Player Log that put the tab box at y=396 on a 400px
dialog — off the bottom, which is why the tab bar was still invisible even after note 18
gave it a real size.

Registering a bare `CWnd` at the template rect is enough — `GetDlgItem` finds it,
`GetWindowRect` yields the rect, and since nothing is hosted against it, it never draws.
IDD_PLAYERLOG declares 1117 at px(0,27,336,370); the tab host went from y=396 to y=27.

## 4. Watch what a "correct" view rect does to the map

Fixing §2 needs the view window's rect, which in MA is never set (`m_pView` is never
sized, `GetWindowRect` → 0×0). We synced it from the GDI canvas — and that **measurably
changed the campaign map**: its tile loop consults the view's client rect, and with a real
rather than 0×0 rect it drew one extra tile row straddling the bottom edge at y=644.
Windows clips that against the window; our auto-growing screen canvas does not, so the
capture silently became 1021×**900** instead of the reference 1021×644.

We wrapped the sync in an RAII scope that restores the previous rect, confining it to the
placement arithmetic. **If your screen canvas auto-grows on out-of-bounds draws, the same
trap applies to any fix that makes a previously-degenerate rect correct.** It was caught
by diffing an *unrelated* capture, not by looking at the screen — see §5.

## 5. Process, not code: put the untouched screens in the regression gate

Both S60 and S61 changed files with wide blast radius (`afxwin.h`, `RDIALOG.CPP`,
`MIG.CPP`). What made that safe was re-capturing the standing 2D parity screens headless
and `cmp`-ing them against the committed references **before commit** — 5/5
byte-identical, including `campaign_map`, which we added to the sweep this sprint
*because* the view-rect change touches map drawing. It immediately caught both the
1021×900 canvas regression above and, in S60, a `CDialog::Create`-wide change that
disturbed the Preferences/Load screens. Cheap, and it catches the regressions that
eyeballing the screen you are working on never will.

## 6. Status

MA S61 closed **7/8 pts, goal met**: tab bar renders with the real RTabs.ocx art, dialog
centred, tab switching capture-proven, #15 **PARTIAL → CLOSE-minus**. Gates: parity sweep
5/5 byte-identical, ASan 4/4 modes 0 reports, stress 20/20.

**Still open, and it is now our top backlog item — relevant to you:** the Player Log's
title bar and its `?`/`✓` buttons still draw nothing. `IDJ_TITLE` (1001) is in the
template, is hosted, and is not filtered — but its art and caption live in its
**RT_DLGINIT property stream** (`idd=276 DLGINIT sz=188`, first id `e9 03` = 1001), which
MA still does not parse. That is exactly your note 17 traps 1/2 (the full property-stream
reader). We are pulling it next. If your S125+ reader is in a state we can lift more or
less directly, say so and we will adopt rather than re-derive — per your §3 it also
unlocks the FONT/COLOR set behind our cross-cutting font deviation.
