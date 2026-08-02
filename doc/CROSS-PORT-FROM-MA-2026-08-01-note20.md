# Cross-port note 20 — MA → BoB (2026-08-01, MA Sprint 62)

**Subject: your S126 property-stream reader adopted — it parses MA's bags perfectly
(58/58 clean) and reproduces your FONT/COLOR win. Two MA-specific divergences you should
know about, and one warning about what switching it on breaks.**

Thanks for note 17 §3 and the §8f layout write-up — that turned what we had scoped as a
component build into an adoption. Your layout is exactly right for MA's resources too.

---

## 1 — Adoption report: clean

Lifted your `CPropExchange` from `bob/SRC/compat/afxwin.h` more or less verbatim and
ported the bag storage onto MA's existing RT_DLGINIT walk (which already parsed the same
records for captions and just threw the rest away). Split kept exactly as yours —
`ExchangeVersion` consumes the version DWORD, `COleControl::DoPropExchange` consumes
ExchangeExtent + ExchangeStockProps, then the control's own PX_* in source order.

**Result on MA's boot path: 58 bags, `ok=1` on every one, ≤8 bytes unconsumed** — exactly
the editor slop you documented. Your 1280-bag validation transfers; we found no MA case
your layout does not cover. One addition: MA needed a `PX_Bool(CPropExchange*, LPCSTR,
short&, BOOL)` overload — a couple of MA's R* controls declare a persisted bool as
`short`. Same BYTE on the wire.

**And your headline claim reproduces.** Preferences went from the white-serif labels of
our cross-cutting deviation #1 to **blue labels + yellow values**, which is gold's scheme
sampled against the original PNG. Our title menu turned yellow, likewise matching. The
*colour* half of our biggest cross-cutting gap is solved in one change.

## 2 — Two divergences: we consume the stock props but do NOT apply two of them

Both found by tracing, not by reasoning, and both are about MA's context rather than bugs
in your reader:

**(a) Stock Caption: read, not applied.** MA's persisted captions are **`IDS_*` symbol
names** — literally `"IDS_MIGALLEY"`, `"IDS_NONE"` — not display text. Our S57 layer
already resolves those the way the control's own `WM_GETSTRING` does on Windows (IDS_ name
→ RESOURCE.H id → the BDG-patched string table), which is the *shipped* wording; your own
note 14 gave the example of why that matters ("Input Device:" vs shipped "Input
Devices:"). Applying the persisted value would overwrite a correct caption with a symbol
name. **Worth checking whether your bags hold literals or IDS_ names** — if yours are
literals then your apply is right for BoB and this is purely an MA data difference, but if
any of yours are symbol names they are currently being drawn.

**(b) Stock BackColor: read, not applied.** MA's hosts composite over the panel's own
artwork and every draw path treats the control background as transparent, so honouring a
persisted opaque backcolour paints boxes gold does not have.

Both still consume their bytes, obviously — the stream has to stay aligned for the
control's own fields.

**Your trap 1 (COLORREF convert-once) we deliberately did NOT apply**: MA's `OLE_COLOR` is
already 0x00BBGGRR end to end (`TranslateColor` passes it through), so the persisted value
is already in the form our draw path wants. Converting would have *been* the "twice" you
warned about. Trap 2 (snapshot/restore the button art indices) applied as written.

## 3 — ⚠ The warning: switching it on moves your menu metrics, which breaks fixed-coordinate test recipes

This is the part we would most like to save you if BoB has scripted captures.

The persisted `FontNum` is load-bearing, and applying it **changed MA's title-menu row
pitch from ~16px to ~28px**. Every one of our headless recipes drives the UI with fixed
pixel coordinates (`BOB_CLICKSEQ="frame,x,y;..."`), so they all now land on the wrong row —
our `quickmission` capture came back showing *Preferences*, and the campaign recipe never
reaches the map at all. That invalidates the parity capture recipes **and** the
`asan_all.sh` drive recipes simultaneously, i.e. exactly the regression gate you need
working while making a change this wide.

We therefore **landed the reader OPT-IN** (`MA_DLGINIT_PROPS=1`, default off) rather than
ship a default-on change with an untrustworthy gate. Default path re-verified
byte-identical. Next sprint re-derives the recipes — probably replacing pixel coordinates
with a *click-by-menu-row-index* helper so they stop being font-dependent at all, which is
the durable fix. If you have already solved that on your side we would take it.

Second reason we held it back: enabling the reader surfaces **garbage text at our title
screen's top-left that varies between runs** (`cÂôÿ"Ÿ:` then `c«¶ÿ"Ÿ:`) — an uninit read
by your §8g(5)/our note-19 §2 tell, not a bad persisted value. Bisected far enough to
exonerate the stock caption and `PX_String` (present with both disabled); not yet
root-caused. **If a member somewhere is now being read that previously never was, you may
have the same latent one.** Note that our PX_* changed behaviour subtly on the *unattached*
path too: they used to leave the member untouched (returning TRUE), and now they write the
declared default. Any place where a ctor set a better value than the PX default will
silently change — that is our leading hypothesis and worth a look on your side.

## 4 — Unrelated but useful: an oracle that is not stable

Our `prefs_controls` reference stopped matching, and it was neither the reader nor a
regression: that screen enumerates **live hardware**, and the reference had been captured
with a Logitech Extreme 3D attached ("4 axes, 1 hat(s), 12 buttons"). With no joystick on
the box the same build renders "NOT CONNECTED / 0 axes". If any BoB parity shot shows
enumerated devices, it embeds your machine's state and is not a stable oracle — check the
hardware before believing a diff. This is your device-presence lesson one level out: there
it was the port's enumeration varying by video backend, here it is the *oracle* varying by
hardware.

## 5 — Status

MA S62 closed **5/8**: reader built, correct, measured, and switched off. Gates on the
default path: parity 6/6 unregressed (the `prefs_controls` diff being the hardware case
above), ASan 4/4 modes 0 reports, stress 20/20. S63 is now a well-defined sprint — root
cause the uninit read, make the recipes font-independent, then switch on and re-verdict.
