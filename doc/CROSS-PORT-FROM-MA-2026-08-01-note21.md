# Cross-port note 21 — MA → BoB (2026-08-01, MA Sprint 64)

**Subject: two engine bugs the property reader exposed once it was on — one of them means
BoB may be silently losing artwork right now — plus a measurement error of ours worth
copying the correction from.**

Follow-up to note 20. The reader has been running default-on for a sprint; this is what
using it in anger turned up.

## 1 — ★ `GetFileNum(name)` may be a stub in your port too, and it costs you artwork silently

In MA this was `int GetFileNum(const char*) { return 0; }` with a comment saying "until the
FileMan name index is wired". It is the filename→FileNum resolver that the R* controls'
string-file setters (`SetNormalFileNumString`, `SetPressedFileNumString`) call.

That matters more than it looks, because it is **the sound half of your trap 2**. You told
us the persisted numeric `Normal/PressedFileNum` are authoring-install indices and must be
discarded — which we do. The persisted **NAME** is then the only remaining source of a
control's design-time art. With `GetFileNum` stubbed to 0, that source resolves to nothing:
**every control whose art is named rather than numbered silently loses its artwork**, with
no error anywhere.

Fix is small if you already parse the graphics table: MA resolves against the
`F_GRAFIX.G` `FIL_* = 0xNNNN` equates that its dialog-template layer already loads for the
template artmap. Verified live — `FIL_ICON_BASES`, `FIL_ICON_B_AIRFIELD_ON`, … all resolve.

**Grep `GetFileNum` in your compat layer.** If it returns 0, you have this.

## 2 — ⚠ …but do NOT apply the resolved names to every button. We measured the regression.

Having made `GetFileNum` real, the obvious next step is: after the property replay, take
`m_NormalFileNumString`, resolve it, and set the art. Note that `PX_String` writes those
members **directly** and never runs the dispatch setter, so nothing converts them unless
you do it explicitly.

We did, and the parity sweep immediately caught it: **the invisible system-box buttons
("Quit"/"Size") materialised in the top-left 72×52 of every front-end screen.** That is
precisely the failure mode your note-16 caveat described and that MA's S58 responded to by
narrowing the design-bag **caption** application to tickbox-class buttons only. The art
path needs the same class narrowing, and we do not yet have a defensible criterion, so we
shipped the code **disabled** behind a flag rather than guess one.

So: implement `GetFileNum` (§1 — that is a pure fix), but treat *applying* persisted art
names as a separate, narrowed change. If you already have a class criterion for trap 2 on
your side, that is the thing we would most like back.

## 3 — `CString(LPCWSTR)` declared but never defined — a link-only gap

Reading any OCX getter's BSTR back (`CString s = c->GetNormalFileNumString();`) failed to
**link**, not to compile, so the gap was invisible until something actually did it.

Worth knowing the convention it hides: in these ports `AllocSysString()` returns a BSTR
that is really a malloc'd **narrow** string — the OCX property path never adopted UTF-16.
So `CString(LPCWSTR)` must treat its pointer as narrow bytes. We have now written that at
the definition instead of leaving it as folklore. If your `CString` has the same
declared-not-defined ctor, you have the same latent trap.

## 4 — A measurement error of ours, and the rule we adopted after it

Our S63 recorded a residual: "native renders LARGER than gold". **It does not.** Measured:
gold's label glyph band is **10 px** and native's **11 px**; gold's row pitch **52 px**,
native's **51 px**. The same absolute font size.

The error: we compared a **1280×1003** gold screenshot against an **800×600** native
capture and read the resulting 1.64× density difference as a font defect. Our own parity
doc already warned "layout is resolution-relative — judge layout/art/content, not pixel
dimensions". The warning was in the header and we did not apply it.

The rule we adopted, which we would suggest for your parity doc too: **the gold set was
captured at a different resolution from the native captures, and the game selects its panel
ART SET by resolution — so capturing at gold's resolution is a different art path, not a
flag. Until that exists, no verdict may rest on relative size, spacing or density.** Only
layout order, art, content and colour.

This cost nothing to correct but would have cost a whole sprint hunting a font-scaling bug
that isn't there. If any of your 14 verdicts lean on apparent size or density, they are
worth re-checking against absolute glyph metrics.

## 5 — Status, and one thing we would like

MA S64 closed **6/8**. Gates green: parity **5/5 byte-identical** (the byte-identical check
resumed this sprint after S63's deliberate rebase), ASan 4/4 modes 0 reports, stress 20/20.

**Still open on our side, third sprint running: the Player Log's title bar.** It is now
precisely located rather than mysterious — `ma_px_replay` **never fires for id 1001**, so
IDD 276's bag for `IDJ_TITLE` is not reaching our bag map, even though that DLGINIT stream
demonstrably begins with `e9 03` (= 1001). It is a bag storage/keying question in the
RT_DLGINIT walk, not an art or draw question.

**If your RT240 walk has any special case around the FIRST record in a bag stream, or
around controls that carry no `FIL_` art name, we would like to hear about it** — that is
where we are looking next.
