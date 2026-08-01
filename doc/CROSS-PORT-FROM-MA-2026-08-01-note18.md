# Cross-port note 18 — MA → BoB (2026-08-01, MA Sprint 60)

**Subject: two engine-level gaps in the shared RDialog/OCX machinery. Both are almost
certainly live in BoB too, both are invisible until you host a tabbed OOB dialog, and one
of them silently zeroes the geometry of every dialog tree.**

Context: MA S60 went after the campaign-map **Player Log** (our parity gold shot #15) —
a `MakeTopDialog(Place(x,y), DialList(DialBox(CPlyr_log), HTabBox(IdList(...), …)))` tree,
i.e. the same `RDIALOG.CPP` machinery both ports share. Three of the four named deviations
turned out to be two engine gaps, not four screen bugs.

---

## 1. Template-declared OCX controls with no `DDX_Control` are never created

Your S124 §8f lesson (which we adopted in S57) was **"the Windows dialog manager creates
EVERY template item; DDX-driven creation silently misses some"** — and we both implemented
it *for RStatic only* (`ma_host_template_statics` on our side).

It is not a static-specific problem. Concretely, in MA:

| Template | id | Coclass | Owns |
|---|---|---|---|
| `IDD_PLAYERLOG` (276) | 1001 `IDJ_TITLE` | **RButton** `78918646` | the dialog title bar |
| `IDD_EMPTYPAGE` (130) | 1002 `IDJ_TABCTRL` | **RTabs** `4a1e1986` | the tab bar |

`RDEmptyP::DoDataExchange` is empty — it binds nothing — so IDJ_TABCTRL never existed.
`RDialog::AddChildren(…, childtype, titles)` opens with
`CRTabs* t = (CRTabs*)GetDlgItem(IDJ_TABCTRL); t->SetHorzAlign(…)` and
`AttachTabToTabControl` ends with `t->AddTab(text, this)` — with no control, both take the
`TRACE0("No tab control exists")` early-out. **Result: no tab bar, no title bar, and the
tab pages never attached.** Note IDJ_TITLE is an RButton, a coclass we had hosted since
Phase 4 — it was missing purely because nothing instantiated it from the template.

**What we did:** turned the static-only hoster into a kind table
(`ma_host_template_controls`), with a per-kind `needsLabel` rule — a tab bar legitimately
starts empty, a caption-less static does not. Kind taxonomy moved into one shared header
so the parser and its consumers cannot drift.

**Worth checking in BoB:** any dialog whose template declares a control the dialog class
does not bind. `IDJ_TITLE` is the high-value one — it is in dozens of templates.

**Also found, not yet fixed (ours):** **RScrlBar `505aee46` is created 16× on our
campaign-map path and is completely unhosted** — it falls through `ma_ole_create`'s CLSID
chain into `CT_OTHER` and silently does nothing. If BoB routes by the same chain, check
it; a scrollbar that no-ops is easy to mistake for "the list just doesn't scroll".

## 2. ★ No RDialog in a dialog tree ever learns its own size

This is the one to check first — it is systemic and it silently degrades geometry rather
than crashing.

`RDialog`'s ctor zeroes `homesize`/`viewsize`, and the single line that would refresh them
from the client rect is **commented out in the shipped source** (`RDIALOG.CPP:147`, in
`DoDataExchange`). On Windows that is harmless: MFC created a real window sized from the
RT_DIALOG template, so `GetClientRect` answers correctly. In a port whose
`CDialog::Create` never sets a size, **every RDialog in a tree answers 0×0**, and the tree
builders derive everything from exactly that call:

- `MakeParentDialog`: `dial->GetClientRect(rect)` → the "if dialsize is meaningless"
  branch runs on a 0×0 rect, so the tree never gets placed where `Place()` asked.
- `AddChildren`: sizes each child from `dial->homesize.Width()` → 0.
- `RDialog::OnSize`: hands `IDJ_TABCTRL` a `MoveWindow` whose width is
  `viewsize.Width()` == 0 → our draw loop then skipped the tab bar on its `w <= 0` guard.

**Fix:** export the template's own cx/cy from the dialog-template parser (we were already
parsing it for the S59 clip test and just never surfaced it) and seed
`m_maW/m_maH`/`homesize`/`viewsize` from it, filling only values still at their zero
default so any explicit `MoveWindow` still wins. Measured on the Player Log: tab host
0×0 → 420×258, CPlyr_log → 336×396, tab pages → 420×228.

### ⚠ Scope it to the tree builders, not to `CDialog::Create`

We tried the obvious thing first — seed the size in `CDialog::Create` — and it **broke the
front end**: canvas 644 → 600 with Load-panel art bleeding into the campaign map. `Create`
is shared with the full-screen panels, which establish their size by other means. Re-scoped
to the three `Create` call sites inside the tree builders, which are the only consumers of
a dialog's own size. If you apply this, apply it there, and keep the comment explaining
why — it reads like something worth "cleaning up" into `Create` later.

## 3. Two smaller ones

- **OOB paint walk.** Our S53 OOB renderer descended `fchild` to the *first art-bearing*
  node and rendered only that. Any node with `artnum == 0` was skipped — and the Player
  Log has two load-bearing ones (CPlyr_log owns the title bar, RDEmptyP owns the tab bar).
  Fixed by recursing over siblings+children and rendering every **visible** node; the
  engine already hides the non-selected tab pages itself (`AddChildren` does
  `ShowWindow(SW_SHOW)` on the first tab, `SW_HIDE` on the rest), so honouring
  `m_maVisible` reproduces "first tab only" by the engine's own mechanism instead of by
  stopping the walk early. Cheap and strictly more correct.
- **An OCX's bitmaps live in the OCX, not in the .exe.** `CRTabsCtrl::OnDraw` does
  `m_TabUp.LoadBitmap(IDB_TABUP)`; inside the control `AfxGetInstanceHandle()` is
  *RTabs.ocx*. Our `CBitmap::LoadBitmap` no-op could never have served it and neither can
  a Mig.exe/BoB.exe resource lookup. The `.ocx` files ship in the install dir, so we load
  the OCX as an extra PE module through the existing resource layer and hand the control
  ready-made memory DCs, then clear its `m_bInit` so `OnDraw` skips its own load. Got us
  the real 297×31 tab art. Same trick should work for any R* control art you are missing.

## 4. S59 uninit-PX discipline: one more instance, and a caveat

`CRTabsCtrl`'s ctor **does** init its only `DoPropExchange`-persisted member (`m_FontNum`)
— we checked rather than assuming, per your note-16 lesson. But its four `HICON` members
are never assigned at all: every `LoadImage` in the ctor is commented out, so
`DrawIconEx` is handed uninitialised heap on Windows too and draws nothing. We NULL them
under the port define so the no-op is deterministic instead of environment-dependent.
Generalised lesson for the audit: **the uninit hazard is not limited to PX-persisted
members** — any ctor-skipped member that reaches a draw call is the same bug class.

## 5. Status / honesty

MA S60 closed at **5 of 8 pts**. The mechanisms above are landed, gate-clean (ASan 4/4
modes 0 reports; stress 20/20; all four 2D parity captures byte-identical to their
committed references) and demonstrably working — all three tabs register with the correct
gold captions and the real art loads. **But the tab bar is still not composited at the
right screen offset, so our parity #15 stays PARTIAL.** Prime suspect for S61:
`RDialog::OnGetXYOffset` only accumulates an offset when
`newparent->parent->artnum == artnum`, and every node in this tree has `artnum == 0`
except the tab pages. If you have already solved offsetting for nested art-less dialogs in
BoB, that is the one thing we would most like back.
