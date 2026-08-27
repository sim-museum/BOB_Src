# Battle of Britain — Linux port status

Last updated: 2026-08-27 (sprint 306)

## What works

- **3D flight** from the front end and from the campaign; the sim runs, records, and exports.
- **Replay recording** (EPIC R). Driven by the game's own `GD_GUNCAMERAATSTART` preference, not a
  test hook — the shipping path. A campaign sortie yields a 182 KB `replay.dat` and 511 ACMI markers.
- **The Preferences UI writes that preference**: `[setfield] combo id=1075 -> val=2`, and `val & 2`
  is the bit that arms recording. Gate: `tools/bob_settings_nav.sh`.
- **Tacview `.acmi` export**: real Lon/Lat, aircraft type names (Spitfire / Hurricane / Bf-109 /
  Bf-110 / Ju-87 / Ju-88 / He-111 / Do-17 / Defiant), player at full rate, AI decimated 1-in-5,
  unplaced aircraft filtered out. Speed verified against the PO's own HUD (176.9 kt vs 177 Kts).
- **Headless navigation** of the front end and the Sim Config screens by measured hit rects.

## Open, with the next concrete step

| id | what | next step |
|---|---|---|
| PO-73 | Grey elliptical blob floating in the cockpit view | **The never-uploaded-texture hypothesis is REFUTED (S306)** — 0 upload bails in the very frame that contains the blob, under a working positive control. Remaining routes to an untextured draw: the garbage-dimension guard setting `t=0` (`BOB_TRACE_GARBAGE`), a draw with `t==NULL`, or one with `hasTex=0`. Take those next; the blob is not a texture that failed to upload. |
| — | ~~`upload_texture` may be failing silently~~ | **Closed (S306).** It no longer can: the early return counts and traces itself (`BOB_TRACE_TEXFAIL`), and a summary prints at every frame dump so zero is a measured number. |
| R1 | Recording chain proven in two halves | The UI→preference and preference→recording links were measured in SEPARATE runs (committing the setting writes no file, so it cannot be staged). One continuous UI→flight→recording session would close it. |
| — | Combat is never reached in any recipe | Known since S206: the flight consumes the wall clock the raid needs. The ACM tree remains untested code. |

## Instruments

`BOB_DUMP_FRAME=N` (**3D frame grab — use this, it predates and works**) · `BOB_TRACE_PRESENT` ·
`BOB_TRACE_BLOB` + `BOB_BLOB_HILITE` + `BOB_BLOB_TEX=<glTex>` (find/paint a suspect draw) ·
`BOB_TRACE_PROJ` · `BOB_DUMP_HITTARGETS` (dump menu + control rects; never guess pixels) ·
`BOB_TRACE_TEXFAIL` (+ `BOB_TEXFAIL_EVERY=N` positive control) ·
`BOB_TRACE_SETFIELD` (settings write-back) · `BOB_TRACE_COMBO` · `BOB_TRACE_GARBAGE` ·
`BOB_AUTOCLICK` (menu indices and `#id` control clicks) · `BOB_GUNCAM=1|pref`.

Navigation, measured: main menu `0`=Quick Shots `1`=Campaigns `4`=Replay `5`=PC Config
`6`=Sim Config; Sim Config tabs `0`=Flight `1`=Game `3`=Views `6`=Continue. Gun Camera is on
**Views** (id 1075), not Game.

## Cautions for whoever picks this up

- **`CRCombo::GetIndex` used to return an uninitialised `long`** when no OLE host answered the
  dispatch — and `SETFIELD` writes its bits straight into `Save_Data.gamedifficulty`, so an
  unanswered dispatch silently set or cleared real preferences. Now sentinel-detected and defaulted
  to 0. **The 0 is damage control, not a fix** — it is a guess, kept only because deterministic
  beats random.
- **A silent probe is a fact about the probe.** Three wrong conclusions this session came from
  nulls produced by instruments that were unreachable, not switched on, or comparing model-space
  coordinates against screen pixels. Give every new detector a positive control before trusting it.
- **`is2D` is never false on the draw path** — all geometry arrives pre-transformed. Projection
  code for "3D draws" is dead.
- **Case-colliding twins**: MFC compiles UPPERCASE, COMMS/3D mixed-case. Some are symlinks
  (`Replay.cpp -> REPLAY.CPP`) and some are not. Check before editing.


## S306 — PO-73: the texture-upload hypothesis is dead, and the probe that suggested it was skewed

### The probe was reading the wrong instant

`[blobtex]` logged `t->glTex` at bob_video.cpp:~2298. `upload_texture(t)` did not run until the
bind, ~130 lines further down. So **on the first draw of any texture the probe saw `glTex=0` — for
healthy textures too**, because they had simply not been uploaded *yet*. The suspect and the frame
came from different instants, which is precisely what STATUS.md recorded as the next step. The
upload now happens before the probe reads, so `glTex==0` there means *never uploaded*, and the
bind-time call is a no-op guard.

That skew is the likely origin of the hypothesis itself: a probe that reports `glTex=0` for
perfectly good textures will keep nominating "the texture never uploaded".

### The early return that could not be seen

```c
if (!s || !s->bits || s->w<=0 || s->h<=0) return;   /* before glGenTextures — glTex stays 0 */
```

Callers then `glBindTexture(GL_TEXTURE_2D, 0)` with texturing enabled. Object 0 has no image, so
it is **incomplete**, and GL specifies that sampling an incomplete texture behaves as if texturing
were disabled — the fragment keeps its raw vertex colour and the draw comes out flat and
untextured. The mechanism was real and the reasoning behind the hypothesis was sound.

### It just does not happen here

| run | uploads bailed | blob in frame |
|---|---|---|
| baseline | **0** | yes — grey ellipse, ~110×25 px near (74, 70) at 800×600 |
| `BOB_TEXFAIL_EVERY=4` | 166 802 | frame differs from baseline by **32.0%** of pixels |

**The null is controlled.** `BOB_TEXFAIL_EVERY=N` forces surfaces down the bail path, and it took
two attempts to make it worth anything:

- v1 keyed on a **call counter**. It bailed one call — but the caller retries every draw while
  `glTex` is still 0, so the next draw uploaded it and the two arms came out **byte-identical**.
  A control that changes nothing proves nothing, and this one would have been easy to read as
  "forcing the defect has no visual effect".
- v2 keys on **surface identity**, so the chosen surfaces stay broken, as the real defect would.
  32% of pixels move, `[texfail]` fires, and `[blobtex]` prints `glTex=0 <-- NEVER UPLOADED`.

With the instrument demonstrably able to fire, the baseline's zero is a measurement: **no texture
reaches draw time unuploaded in the frame that contains the blob.** The hypothesis is eliminated,
not merely unsupported.

The frame-dump summary line prints the count unconditionally, so a future run reports zero as a
number rather than as the absence of a log line.
