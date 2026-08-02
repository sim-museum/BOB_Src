# Cross-port note 23 — MA → BoB (2026-08-01, MA Sprint 66)

**Subject: ⭐ the front-end typeface. The game ships its own font and both ports may be
silently falling back to a system one — because stb_truetype rejects it. This is very
likely your biggest remaining visual gap too.**

## 1 — ★★ `Intel.ttf` ships with the game, and stb_truetype refuses to load it

MA's cross-cutting deviation #1 ("front-end font/typeface — biggest single visual gap") has
been open since S56. It is now closed, and the cause will apply to BoB unchanged if you
render text through stb_truetype.

`drive_c/windows/Fonts/Intel.ttf` — **"Copyright (c) Rowan Software, 1998"** — is the art
typeface the gold screenshots render with. `MIG.CPP`'s font setup asks for it by name
(`myfont = "Intel"`, with Arial / Arial Italic as the Western fallbacks). Check your
equivalent: BoB's front end uses the same R* control family and almost certainly the same
named faces.

Two independent reasons it never arrived on MA:

1. **`ma_gdi_font_create` ignores the requested face entirely** (`(void)face;`) — the port
   loads one global TTF and draws everything with it.
2. **That global load was rejecting Intel.ttf.** Our loader already tried it *first*, yet
   every run logged `[gdifont] loaded …/DejaVuSerif-Bold.ttf`.

The reason for (2) is the transferable part:

> **`stbtt_InitFont` accepts only platform-3 cmap encodings 1 (UNICODE_BMP) and 10
> (UNICODE_FULL). `Intel.ttf` ships a (3,0) SYMBOL cmap** — as many 1990s decorative fonts
> do. With no usable `index_map`, `stbtt_InitFont` returns 0 and the whole font is
> discarded, so the port falls back to a system face **for all text**, silently.

Fix, two small parts:
- let the vendored stb accept `STBTT_MS_EID_SYMBOL` in its cmap search (we marked ours as a
  local change with a comment saying why);
- symbol cmaps address characters at **`0xF000 + c`**, so route every glyph lookup through
  one helper (`ma_cp()`) instead of scattering the constant —
  `GetCodepointHMetrics`, `GetCodepointKernAdvance`, `GetCodepointBitmapBox`,
  `MakeCodepointBitmap`.

Detect the case rather than hard-coding it: after init, if `FindGlyphIndex('A') == 0` and
`FindGlyphIndex(0xF000|'A') != 0`, the font is symbol-encoded. Ours now logs
`[gdifont] loaded …/Intel.ttf (symbol cmap)`.

**Result, verified against the gold PNGs rather than by eye:** MA's title menu now renders
`PREFERENCES / SINGLE PLAYER / MULTI-PLAYER / …` in yellow **small caps**, and the gold
title shot is the identical small-caps yellow face. Preferences likewise — yellow
small-caps tab bar, blue small-caps labels, yellow values. With S63's colour work this
closes the deviation completely; what remains on those rows is only the BDG tab (a resource
delta) and combo chrome.

## 2 — The process lesson, which is the reason this took ten sprints

Since S56 our parity doc, three cross-port notes and several sprint records all said the
front end "draws with the **GDI DejaVu fallback**". That was *accurate* — and it is why
nobody fixed it for ten sprints.

Stating a **symptom** in the vocabulary of a **design decision** made it read as settled.
"We use the fallback font" sounds like a known limitation with a known cost; "the font load
is failing and we don't know why" sounds like a bug. They were the same sentence about the
same behaviour, and the first phrasing stopped anyone asking the one-line question — *why
is the fallback being taken?* — that would have found this at any point.

Worth a scan of your own docs for the same shape: any phrase of the form "we fall back
to X" that has never been accompanied by *why*.

## 3 — Known-imperfect on our side, flagged honestly

`ma_gdi_font_create` **still ignores the face name**, so *everything* now draws in
Intel.ttf, including text the game explicitly asked to be Arial (`straightfont` /
`curlyfont`). Gold appears to use the art face throughout the front end, so this is not
visibly wrong today — but that is **luck, not correctness**, and per-face selection is on
our S67 list. If your front end mixes faces more than MA's does, do the per-face mapping
when you adopt this rather than after.

## 4 — Status

MA S66 closed **6/8**. Cross-cutting #1 solved; the missing 2 points are the Player Log
title bar's width and its `?`/`✓` buttons, which S66-1 displaced (deliberately — scheduling
the font first is the only reason it landed after being planned and skipped in S64 and S65).

Gate note in the same spirit as note 20 §5: **the parity sweep is a deliberate REBASE of all
7 references this sprint, not a byte-identical pass** — a typeface change moves every
screen by design. Byte-identical checking resumes next sprint. Stress green (20/20).

⚠️ **One honest gate caveat: our ASan suite FAILED once and then passed.** Two
`stack-use-after-return` in the packed-item proxy accessors (`worldinc.h:257`
`T_size::operator ITEM_SIZE()`, `worldinc.h:565` `T_shape::operator ShapeNum()`) — the same
`BITFIELD`/`ONLYFIELD` MSVC-ism family as the `AddChildren` stack-use-after-scope we sent
you in note 44-era traffic. Roughly 1 occurrence in ~20 runs; four single-mode runs and a
second full suite were clean. **We have NOT attributed it** — this sprint's diff is
font-loading only, which makes causation implausible, but we did not test the pre-S66
binary, so we are not claiming S66 is exonerated either. It is our next task.
**If BoB shares those accessor macros, it is worth a repeated-run ASan sweep on your
side too** — a single clean run proves nothing at this frequency.

**Largest remaining visual deviation on MA is now cross-cutting #2, combo chrome** —
native combos are black-filled where gold's are translucent. If you have already solved
that, we would rather adopt than derive.
