# Gold provenance — what each gold IS, and how it was made

Written 2026-09-17 (GOLDPROV-BOB-1). **A gold without a recorded recipe is a number nobody can
re-derive.** MiG Alley lost three sprints on 2026-09-17 to golds whose scale was never written down;
this file exists so BoB does not repeat it.

## The two corpora are NOT the same kind of artefact

| corpus | what it is | scale |
|---|---|---|
| `260915_bob_*.mp4`, `bob_convoy_campaign.mp4` | **FULLSCREEN screen recordings, 1920×1080 @ 60 fps** | measured: 99.9 % of pixels dynamic across 14 sampled frames, bbox `x[9..1910] y[5..1074]`, every margin live content — no window border, no letterbox |
| `Screenshot from 2026-06-24 *.png` (×6) | desktop stills | window size **not recorded**; treat as MiG Alley's stills are treated — unusable for absolute pixel scale until re-made |

## `doc/reference/260915_gold_sideselect_raf.png` — 1024×768

⛔ **Derivation UNKNOWN.** Tested by reproduction and it is **not** a resized frame of either campaign
video: 66 frames sampled (RAF @12 s ×14, RAF @2 s ×22, German @4 s ×30), each resized to 1024×768 and
compared — best `mean|diff|` **45.9–48.0 with a FLAT distribution**, which is the signature of no
match rather than a near miss.

⭐ **Most likely a direct screenshot of the game window at its native 1024×768**, taken the same day
as the videos. Fits the exact size, the 1-px caption agreement GOLDVID-BOB-3 S5/S6 measured
(resampling would not be that clean), and MiG Alley S11's finding that a PO screenshot is the only
capture method that works on this machine. **Inference, not established.**

**It is still usable** — the caption agreement is too specific to be chance — but cite it as
*"provenance unknown, matches our native size"*, not as a known-scale oracle.

## Why BoB needs no 1080 reference arm (and cannot have one)

MiG Alley has `port/ref/native1080/` and it matches its 1080 gold video frames. **BoB cannot:**

```
BOB_FORCE_RES=1920x1080  ->  [vid] front-end layout pinned to 1920x1080
capture is 1920x1080, but content bbox = x[0..1023] y[0..767]  -> exactly 1024x768, top-left
                                          37.9 % of the frame lit, the rest black
```

The flag pins the **layout**; the front-end art set is 1024×768 and does not scale. **This is the
measured form of the PO's own report — "setting campaign resolution at 1920x1080 has no effect".**

**Consequence:** BoB's native output is 1024×768, its committed gold extract is 1024×768, and that
1:1 comparison is both correct and the only one available. Do not build a 1080 arm here; compare at
1024×768.

⚠️ Scope: this measures the **front end**. Whether a larger art set exists for the 3-D path is not
established here.

## Rule for new golds

Record, beside the file: **source** (video + timestamp, or "screenshot"), **capture method**, and
**resolution**. If it cannot be reconstructed, say so — as this file does above.
