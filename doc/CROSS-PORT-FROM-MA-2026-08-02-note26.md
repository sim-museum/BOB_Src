# Cross-port note 26 — MA → BoB (2026-08-02, MA Sprint 69)

**Subject: ★ the port was silently running as a JAPANESE system, so the front end never once
requested Arial — plus a per-face font registry, and the combo box was the one control still
filling itself opaque black.**

## 1 — ★★ `EnumFontFamilies` always "found" the font, forcing the Japanese branch

`MIG.CPP::InitInstance` (the same code in both ports) picks its font set by probing for a
Japanese face and branching:

```c
char* curlyfont    = "ＭＳ 明朝";     // MS Mincho  (high-byte / Shift-JIS)
char* straightfont = "ＭＳ ゴシック"; // MS Gothic
int   gotfont = false;
char* myfont = "Intel";
EnumFontFamilies(dc, curlyfont, EnumFontFamProc, &gotfont);  // is a Japanese face installed?
if (gotfont) { myfont = curlyfont; /* JAPANESE build */ }
else { straightfont = "Arial"; curlyfont = "Arial Italic";
       EnumFontFamilies(dc, "Intel", …);  /* ENGLISH build: myfont=Intel, data text=Arial */ … }
```

Our compat `EnumFontFamiliesA` was:

```c
static inline int EnumFontFamiliesA(HDC, LPCSTR, void* proc, LPARAM p) {
    if (proc) ((FONTENUMPROC)proc)(0,0,0,p);   // <-- ALWAYS calls the proc
    return 0;
}
```

and `EnumFontFamProc` unconditionally sets `gotfont=true`. So the very first probe (for MS
Mincho) "succeeded" → **the game took the Japanese branch and asked for MS Mincho for every
font**. MS Mincho isn't shipped, so each request fell through our face fallback to the art
face (Intel.ttf). Net effect: the port drew *everything* in the Rowan art face and **never
requested Arial at all** — which happens to match gold on the Intel-only screens (title menu,
listbox) and silently mismatches on every data/label row, where gold's English box used Arial.

**Fix (one line of intent):** report a family present only for a **pure-ASCII** name — we ship
no CJK faces, so the CJK probe must fail:

```c
static inline int EnumFontFamiliesA(HDC, LPCSTR family, void* proc, LPARAM p) {
    if (proc && family && family[0]) {
        int latin = 1; for (const unsigned char* q=(const unsigned char*)family; *q; ++q)
            if (*q >= 0x80) { latin = 0; break; }
        if (latin) ((FONTENUMPROC)proc)(0,0,0,p);
    }
    return 0;
}
```

Now the English branch runs; runtime faces are `Intel` (headers/title/listbox) + `Arial`
(data/labels) exactly as on the gold Windows box. **Check yours:** `grep -n EnumFontFamilies
<compat>` and `MA_TRACE_FONT`-style trace the faces `ma_gdi_font_create` actually receives —
if they are mojibake CJK, you have the same trap. Same family as note 25 (icons) / note 23
(Intel.ttf): a stub that *succeeds* hides the gap.

## 2 — Per-face font registry (honour the `face` arg)

Our `ma_gdi_font_create` ignored `face` and drew every font in one global TTF. Replaced with a
tiny cached registry keyed by face KIND: ART = Intel.ttf (the Rowan art face; load order
preserved so ART screens stay byte-identical), SANS = LiberationSans (metric-compatible with
Arial) / DejaVuSans, SERIF = Liberation/DejaVu Serif. `face_kind()` classifies the requested
name (Intel/Header→ART, Arial/Free/*Sans→SANS, Times/MS Serif→SERIF, **unknown→ART** so
nothing regresses). `MaFont` carries its resolved face; text/metric/extent route through the
DC font's face; the S66 `(3,0)` symbol-cmap offset is now per-face (`ma_cp_f(t,·)`).
Gold-verified: Preferences and Quick Mission go from all-Intel to blue-sans labels +
yellow-sans values = gold's scheme.

## 3 — The combo box was the one control still filling itself opaque black

`CRComboCtrl::OnDraw` (`RCOMBOC.CPP`) fills its value box with `BLACK_BRUSH` when
`WM_GETARTWORK` returns 0. We deliberately return 0 (the panel's OnPaint already composited
its background; hosted controls draw transparently over it) — so this fill painted an **opaque
black box where gold shows a transparent one** (the panel/photo visible through a thin
bordered outline). Every other hosted control already drew transparently; the combo was the
lone hold-out. Fix: skip the black `FillRect` on the Linux path — the border pens and the
transparent `FIL_COMBO_BUTTON` still draw the chrome. Combos are now translucent = gold. If
your combos read as black rectangles against a translucent gold reference, this is why.

— MA
