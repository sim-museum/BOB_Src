# Cross-port note 25 — MA → BoB (2026-08-02, MA Sprint 68)

**Subject: ★ icons were a no-op in our compat and nobody noticed for the whole port — every
`DrawIcon` call silently drew nothing. Plus where the R* control icons actually live.**

## 1 — ★★ `DrawIcon` / `LoadIcon` were stubs, so NO icon in the port ever rendered

Our `CDC::DrawIcon` was `{ return TRUE; }` with the comment *"icons not yet rasterised"*,
and `LoadIconA` returned `NULL`. Both had been that way since the compat layer was written.
The consequence is broader than any one screen: **anything the engine draws as an icon was
invisible, everywhere, with no error and no trace.**

Worth checking yours (`grep -n "DrawIcon\|LoadIconA" <compat>`) — a stub that returns
success is exactly the kind of gap that never surfaces as a bug report, only as "that
screen doesn't look quite like the reference".

The case that exposed it for us: the Player Log title bar's `?` / `✓` buttons. The route
there is worth repeating because it is engine logic, not guesswork — `RDialog`'s eventsink
says

```
ON_EVENT(RDialog, IDJ_TITLE, 2 /* Cancel */, OnCancel, VTS_NONE)
ON_EVENT(RDialog, IDJ_TITLE, 3 /* OK */,     OnOK,     VTS_NONE)
```

i.e. **the title control itself raises Cancel/OK**, so it must draw its own buttons.
`CRButtonCtrl` does (`RBUTTONC.CPP:521-536`), gated on its persisted `CloseButton` /
`TickButton` flags — and our Player Log's title bag really does carry `close=0 tick=1`. The
✓ was always meant to be there; there was simply nothing behind `DrawIcon`.

## 2 — Where the R* control icons live: `Rbutton.ocx`, not the .exe

Same shape as note 23's `Intel.ttf` and S60's RTabs art: inside a control,
`AfxGetInstanceHandle()` is **that control's own module**.

We scanned every shipped PE. `Mig.exe` carries only `RT_GROUP_ICON` 128/129. **`Rbutton.ocx`
carries 828–832** — exactly `IDI_BYEUP` / `IDI_TICKUP` / `IDI_TICKDOWN` / `IDI_HELPUP`.

A small warning from our own process: an early `ls *.ocx | head` truncated `Rbutton.ocx`
out of the listing (note the lowercase 'b') and briefly convinced us RButton simply was not
installed — which would have made the whole story "the resources don't ship, nothing to do".
**Don't let a truncated listing become a negative result.**

## 3 — Implementation notes, if you adopt

`RT_GROUP_ICON` is a directory (`GRPICONDIR` + `GRPICONDIRENTRY[]`) whose entries name
`RT_ICON` resources **by id** — you have to follow the indirection; the group is not the
image.

The `RT_ICON` payload needs its own decoder, not your DIB path:
- `biHeight` is **double** the real height — the XOR (colour) bitmap, then the 1bpp AND
  (mask) bitmap;
- both are bottom-up;
- a mask bit of **1 means transparent**.

We handle 1/4/8/24/32 bpp, cache by id, and blit alpha-keyed through the same
viewport-origin and clip path as everything else (the clip from note 24 §1 matters here —
a 32×32 icon drawn near a control's right edge would otherwise spill).

`LoadIconA` ignores `hInstance` on purpose and searches Mig.exe → Rbutton.ocx →
RTickBox.ocx, for the same reason as the font and tab art: the handle we would get is
meaningless in this port, but the *id* is not.

**Result:** `?` and `✓` render red on the title bar as gold shows them, and our parity sweep
stayed **5/5 byte-identical** — the change is confined to screens that actually call
`DrawIcon`.

## 4 — Status, and the debt we are still carrying

MA S68: `?`/`✓` done. **Per-face font selection is now on its third carry** — our
`ma_gdi_font_create` still ignores the requested face, so everything draws in the art face
(matching gold by luck, as note 23 §3 flagged).

The intermittent `stack-use-after-return` from note 23 §4 is **still being attributed**.
S67 failed to do it properly (we only tested the current build, which cannot distinguish
"didn't cause it" from "didn't fire"). S68 built the pre-S66 binary via a git worktree and
ran an alternating S65↔HEAD harness with `detect_stack_use_after_return=1` forced; the
sample achieved is in our sprint record. We will report a conclusion when we have one
rather than when a run happens to come back clean.
