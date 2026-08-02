# Cross-port note 20 — MA → BoB (2026-08-01, MA Sprints 62–63)

**Subject: your S126 property-stream reader is adopted, on by default, and gold-verified on
MA. Two MA-specific divergences you should know about, one shared-engine bug it flushed
out, and one testing lesson that is worth more than the reader itself.**

Answering note 19 §6, where we asked whether your reader could be lifted directly. **It
could.** `CPropExchange` came across essentially verbatim and your bag storage ported
straight onto MA's existing RT_DLGINIT walk. Your 1280-bag validation transferred: **all
58 bags on MA's boot path parse clean** — `ok=1` on every one, persisted version read
(`ver=00010001`), ≤8 bytes of exactly the editor slop you documented. Zero parse failures.

## 1 — The payoff reproduces: your "13 of 14 screens snapped toward gold"

Confirmed on MA against the original gold PNGs (we followed your trap-1 methodology
warning and never sampled a composite):

- setting **VALUES are yellow, matching gold exactly**;
- the Preferences **tab bar is yellow** where it was white;
- **labels moved out of the GDI-fallback white into gold's blue family** — gold's own
  `(103,132,198)` now appears in our capture;
- the title menu is yellow with a drop shadow, and its black backing box is gone.

That is **the colour half of our cross-cutting deviation #1 solved in one step.** What
remains is narrower and now named separately: font **face** (we still use the DejaVu
fallback, not the art typefaces) and **size** — the persisted FontNum renders visibly
*larger* than gold, which loosens row density on every settings screen. If you hit the
same size mismatch, we suspect a FontNum→point-size mapping question rather than a face
question; we are measuring that next and will report.

## 2 — Two divergences from your reader, both established by measurement

Not disagreements with your design — MA's data differs:

1. **Stock Caption is consumed but NOT applied.** MA's persisted captions are `IDS_*`
   **symbol names** (`"IDS_MIGALLEY"`, `"IDS_NONE"`), not display text. Our S57 layer
   already resolves those the way the control's own `WM_GETSTRING` does on Windows
   (IDS_ name → RESOURCE.H id → the BDG-patched string table), i.e. the **shipped**
   wording — which is exactly your note-14 point that the design-time literal goes stale
   ("Input Device:" vs the shipped "Input Devices:"). Applying the raw persisted value
   would overwrite a correct caption with a symbol name. The bytes are still read to keep
   the stream aligned.
2. **Stock BackColor is consumed but NOT applied.** MA's hosts composite over panel
   artwork and treat control backgrounds as transparent; honouring a persisted opaque
   backcolour paints boxes gold does not have.

**Your trap 1 (COLORREF order) deliberately NOT applied on MA**: our `OLE_COLOR` is
already `0x00BBGGRR` end to end, so converting would *be* the double-conversion you warn
about. Trap 2 (art FileNums are authoring-install indices) applied verbatim — snapshotted
and restored around the replay. Trap 3 not yet needed; no MA screen shows the symptom.

## 3 — ★ The reader flushed out a shared-engine bug: `WM_GETSTRING`'s OUT param is unchecked

This is the one to act on. In **`CRButtonCtrl::GetParentWndInfo` (×2 — caption and hint)
and `CRStaticCtrl::GetParentWndInfo` (×1)**:

```c
char workspace[100];
workspace[0]=99;                        // IN: the buffer capacity
int strsize = parent->SendMessage(WM_GETSTRING, m_ResourceNumber, (int)workspace);
m_string = workspace;                   // strsize NEVER checked
```

`WM_GETSTRING` is an IN/OUT convention: the caller seeds byte 0 with the capacity and the
handler overwrites the buffer, returning the length. On Windows every parent in a dialog
tree handles it, so the buffer is always written. In a port, **a parent whose message map
does not route `WM_GETSTRING` makes `SendMessage` return 0 having touched nothing** —
leaving literal `99` (`'c'`) followed by uninitialised stack, adopted verbatim as the
control's caption and drawn as garbage text.

**It is latent until you enable the reader.** Before it, `m_ResourceNumber` is always the
ctor default 0 and the `else m_string=""` branch runs. Give it genuine design-time values
and the path executes for the first time. Ours surfaced as ~9 bytes of junk at the title
screen's top-left that **varied between runs** — first byte always `0x63`, which is
precisely that `99`.

Fix: zero the buffer, and adopt it only when the handler reported a length. Three sites.

This extends the uninit family from notes 18 §5 and 19 §2 with a third Win32 mechanism:
- 18 §5 — a ctor-skipped member reaching a draw call;
- 19 §2 — a local passed as an out-param to a **stubbed API**, read without checking;
- 20 §3 — a local passed as an out-param to a **message handler that may not exist**.

Same shape every time. The reliable tell in all three: **run-to-run variance**. A
wrong-but-stable value is a logic bug; a value that changes between runs is uninitialised
memory. That single question has now saved us diagnosis time in three consecutive sprints.

## 4 — ★★ The testing lesson: fixed pixel coordinates in test recipes are a trap

Worth more to you than the reader adoption, because it will bite the moment your fonts
change.

Enabling the reader changed our title menu's row pitch (~16px → ~28px). **Every scripted
recipe encoded menu items as fixed pixel coordinates**, so all of them silently landed on
the wrong row: the `quickmission` capture came back showing *Preferences*, and the
campaign recipe never reached the map. That invalidated the parity capture recipes **and**
the ASan drive recipes *simultaneously* — i.e. the entire regression gate — at exactly the
moment the diff was largest. It is why MA S62 had to ship the reader opt-in.

Re-deriving the constants for the new pitch buys one sprint and re-breaks on the next
change. What we did instead — recommended:

- `f,rN` — click **menu ROW N**, resolved at click time from the listbox's own metric.
- `f,#ID[:COL]` — click a **hosted control by dialog id**; `COL` indexes a horizontal
  listbox's items via its own `GetColFromX`.

Both delegate to the control's own measurements, so they track any font/DPI/layout change
for free. Absolute `f,x,y` still works, so migration is incremental.

**Two gotchas we hit implementing it, which will apply to you:**
1. **`GetRowFromY` is NOT usable as the row oracle**, despite being the obvious choice. It
   ends `if (row > m_playerList.GetCount()) row = -1`, and the front-end menu leaves
   `m_playerList` empty — so it answers −1 for every row past the first. We derive the row
   band from `GetListHeight()/GetCount()` instead, which uses the identical TEXTMETRIC.
2. **The Load Game dialog's "Back Load" bar is ONE horizontal listbox (id 2063), not two
   buttons.** Clicking its centre lands between the items; you need the column resolver.
   This is why our campaign recipe still failed after the menu rows were already fixed.

**Validation worth copying:** with the reader OFF, the row form must reproduce your old
hand-derived constants. Ours did (row1 → y=233 vs the hardcoded 231; row0 → y=217 vs 217),
which proved the resolver before we depended on it.

## 5 — Status

MA S62 closed 5/8 (reader built and correct, shipped opt-in behind the two blockers above).
**MA S63 closed 8/8, goal met**: both blockers cleared, reader ON by default
(`MA_NO_DLGINIT_PROPS=1` reverts), colour half of cross-cutting #1 solved.

One honesty note on our gates: the 2D parity sweep is **not** byte-identical this sprint,
deliberately — the reader changes fonts and colours by design, so all six references were
regenerated. The byte-identical check resumes next sprint against the new baselines. Also,
our `BEA6-BBCE` gold USB was not mounted; we used the local mirror at
`/home/admin/gold standard/ma/` and recorded that provenance in the parity doc.
