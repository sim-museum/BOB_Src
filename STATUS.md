# Battle of Britain — Linux port status

Last updated: 2026-08-27 (sprint 309)

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
| PO-73 | ~~Grey elliptical blob floating in the cockpit view~~ | **CLOSED (S309) — NOT A DEFECT.** It is the game's own **threat indicator**: `COverlay::DoThreat()`, texture `MskMap16/THREAT01.X8`, gated on `Save_Data.gamedifficulty[GD_HUDINSTACTIVE]`. Position and size match the source constants to within a pixel. It reads as a blank grey ellipse because it has no contacts to plot — the player is alone in this scene. **For the PO:** it is switched off with the HUD-instruments difficulty setting. |
| — | ~~`upload_texture` may be failing silently~~ | **Closed (S306).** It no longer can: the early return counts and traces itself (`BOB_TRACE_TEXFAIL`), and a summary prints at every frame dump so zero is a measured number. |
| R1 | Recording chain proven in two halves | The UI→preference and preference→recording links were measured in SEPARATE runs (committing the setting writes no file, so it cannot be staged). One continuous UI→flight→recording session would close it. |
| — | Combat is never reached in any recipe | Known since S206: the flight consumes the wall clock the raid needs. The ACM tree remains untested code. |

## Instruments

`BOB_DUMP_FRAME=N` (**3D frame grab — use this, it predates and works**) · `BOB_TRACE_PRESENT` ·
`BOB_TRACE_BLOB` + `BOB_BLOB_HILITE` + `BOB_BLOB_TEX=<glTex>` (find/paint a suspect draw) ·
`BOB_TRACE_PROJ` · `BOB_DUMP_HITTARGETS` (dump menu + control rects; never guess pixels) ·
`BOB_TRACE_TEXFAIL` (+ `BOB_TEXFAIL_EVERY=N` positive control) ·
`BOB_COMBO_FORCE_UNANSWERED=1` (forces GetIndex's guard) · `BOB_OLE_FORCE_NOHOST=1` +
`BOB_TRACE_OLE_UNANSWERED=1` (forces and reports unanswered dispatches at the ROUTER — a different
claim from the previous hook, see S311) · `BOB_BLOB_SKIP=<set>` (omit
draws by texture — **use this, not `BOB_BLOB_TEX` hilite**) · `BOB_GLTEX_ONLY=<n>` (dump one
texture + its alpha) · `tools/bob_blob_bisect.sh` ·
`BOB_TRACE_SETFIELD` (settings write-back) · `BOB_TRACE_COMBO` · `BOB_TRACE_GARBAGE` ·
`BOB_AUTOCLICK` (menu indices and `#id` control clicks) · `BOB_GUNCAM=1|pref`.

Navigation, measured: main menu `0`=Quick Shots `1`=Campaigns `4`=Replay `5`=PC Config
`6`=Sim Config; Sim Config tabs `0`=Flight `1`=Game `3`=Views `6`=Continue. Gun Camera is on
**Views** (id 1075), not Game.

## Cautions for whoever picks this up

- **`CRCombo::GetIndex` used to return an uninitialised `long`** when no OLE host answered the
  dispatch — and `SETFIELD` writes its bits straight into `Save_Data.gamedifficulty`, so an
  unanswered dispatch silently set or cleared real preferences. Sentinel-detected since S289 and
  defaulted to 0, which was recorded here as "damage control, not a fix".
  **S310 stopped writing the guess at all.** `GetIndex` now also raises
  `g_bob_combo_unanswered`, and the write-back skips `whatbit` when it is set: an unanswered read
  means *no new value*, so the stored preference is the best information available.
  `BOB_NO_COMBO_GUARD=1` restores the old always-write behaviour.
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


## S307 — PO-73 named: it is the aircraft shadow

`glTex=43`: 128×128, `Amask=0xf000`, a **dark ellipse at a flat alpha of 102/255**, transparent
everywhere else. 1943 opaque-ish texels against an on-screen ellipse of 1781 px on a 125×125 quad —
about 1:1, so the sprite is drawn at native size. Dark, flat, elliptical, ~40% blended: a shadow
blob. Skipping that single draw removes the ellipse and leaves the sky and clouds untouched.

So PO-73 is not a texturing defect. The sprite is doing exactly what it was authored to do; it is
in the wrong **place**. `ThreeDee::do_object_shad` (`SRC/3D/3DCODE.CPP:2656`) only reads
`tempitemptr->World` and offsets it by the viewer, so whatever should pin the shadow to the terrain
happens upstream of it. That is the next thread.

### Why the previous instrument could not have found this

`BOB_BLOB_HILITE` paints a suspect draw by `glDisable(GL_TEXTURE_2D)` + a flat colour. **That
discards the texture's alpha.** A transparent cloud billboard therefore paints as an *opaque*
magenta rectangle, so every alpha draw over the region scores 100% whether or not it is the
culprit. Measured: hiliting `{11}` alone filled the whole test box. S303b read this as "containment
alone hilited the whole sky" and treated it as a targeting problem, but it is not — **the probe
changes the thing it is measuring**, and no amount of narrowing the region fixes that.

`BOB_BLOB_SKIP` omits the draw instead. Every other draw renders normally, so the question becomes
"did the ellipse disappear", which alpha cannot confound.

### The bisect that lied, and what it cost

`tools/bob_blob_bisect.sh` first scored "is the box magenta". Every round said yes, so it discarded
nothing on evidence and simply walked to the last candidate, reporting `glTex=116` — **a
confident, specific, wrong answer produced entirely by the search's own default.** A bisect with no
positive signal does not fail loudly; it returns whichever element it happened to end on.

The second scorer counted dark pixels. Skipping the cloud layer *raised* the count from 1781 to
9575 (the sky behind is darker), so "fewer dark pixels" was confounded too.

What worked was scoring the **shape**: the largest connected dark component. The ellipse is
`113×21, aspect 5.4, fill 0.96`; a cloud-removal flood is `171×56, fill 1.27` — a filled rectangle,
plainly not an ellipse. And rather than trust a bisect on a metric that had already misled twice,
all 30 candidates were skipped **individually**: exactly one, `glTex=43`, collapsed the component
from 1781 px to 9. Thirty runs took under two minutes, which is the other lesson — the bisect was
optimising a cost that was never the problem.


## S308 — the shadow reading was wrong; the texture identification was not

S307 called `glTex=43` "the aircraft shadow sprite". **Withdrawn.** Two independent checks:

- `ThreeDee::Add_Shadow` returns immediately for the player's aircraft. Traced:
  `detail=1 dopiloted=0 isPlayer=1` on every call, and the gate is
  `if (dopiloted || (ac != Manual_Pilot.ControlledAC2))`.
- `ThreeDee::do_object_shad` — the **only** route by which a `SHADOW_OBJECT` enters the draw list
  (`3DCODE.CPP:1853` is its sole live caller) — is entered **0 times** in a 300-frame run.

No shadow is drawn in this scene, so the ellipse cannot be one.

**What was actually wrong with S307 was the kind of evidence, not the care taken over it.** The
draw was identified by *measurement*: skip `glTex=43` and the ellipse vanishes while the sky and
clouds do not. That still stands. The name "aircraft shadow" came from *looking at the texture* —
a dark flat ellipse at 40% alpha certainly resembles a shadow blob — and an interpretation of art
is not a measurement of a code path. The two sat in the same sentence and the second inherited the
first's confidence.

The check that broke it was cheap and available at the time: ask whether any shadow is drawn at
all. Worth remembering that "what does this look like" and "what code puts it there" are separate
questions, and only the second one names a defect.

Cross-check that the game's own shadow art is not this texture either: `MASKMAP/shadow.pcx` is
128×128 solid black with `shadow_trans.pcx` as its mask, and that mask is a **mottled, cloud-shaped
blob** — not the clean flat ellipse in `glTex=43`.

### Where the draw comes from

`BOB_BLOB_BT=<glTex>` (new) backtraces the draw:

```
draw_fvf -> DEV_DrawPrimitiveVB -> Lib3D::RenderTPolyList -> Lib3D::EndScene
         -> ThreeDee::render3d -> ThreeDee::render -> View3d::drawloop
```

It is flushed from the **deferred transparent-polygon list** at end of scene, which is why the
stack names no game object: by then the queuer is long gone. Naming it needs an instrument at
**insertion** time, or a texture-id/filename recorded at surface creation.

A screen-locked-vs-world-locked test was attempted and is **inconclusive**, recorded so it is not
re-run blind: with the aircraft near-stationary the whole frame is pixel-identical across frames
150/300/450, and with `BOB_AUTOFLY=view40` panning the view the detector's window floods with
terrain (largest dark component becomes the full 171×56 window). The discriminator needs a
detector that tracks the ellipse across a moving background, not a fixed window.


## S309 — PO-73 closed: the blob is the threat indicator, working correctly

`COverlay::DoThreat()` (`SRC/3D/OVERLAY.CPP:7756`) — *"display threat indicator in the top left
corner of the screen"* — draws a squashed disc that plots nearby aircraft as sticks, from
`MskMap16/THREAT01.X8`. Its constants, at a 640 reference with `SCX = w/640`, `SCY = h/640`:

```
TOP_LEFT_X = 10, TOP_LEFT_Y = 10, RADIUS = 50, Y_RADIUS = 10
centre = (sRADIUS + sTOP_LEFT_X, sRADIUS + sTOP_LEFT_Y)      // from baseStickX/baseStickY
```

| at 800×600 | intended | measured |
|---|---|---|
| centre | (75.0, 71.9) | (75.5, 71) |
| width | 2·sRADIUS = 125 | quad 125 |
| height | 2·sY_RADIUS = 18.75 | 21 |

Nothing is misplaced, mis-scaled, or mis-textured. It looks like a featureless grey ellipse because
it has **no contacts to draw on it** — the same run shows only the player's aircraft in the world.
It is gated on `Save_Data.gamedifficulty[GD_HUDINSTACTIVE]`, so the PO can switch it off with the
HUD-instruments difficulty setting.

### How it was finally named

The draw is flushed from the deferred transparent list, so nothing at bind time knows what it is.
The chain that closed the gap, each link a small instrument:

1. `BOB_BLOB_SKIP=43` — skipping that draw removes the ellipse and nothing else (S307).
2. `g_lib3d_uniqueTextID`, carried from `RenderTPolyList` to the GL backend. Useful for its *type*
   bits, but its index is only a texture-manager slot (`LIB3D.CPP:6595` hands out sequential `j`),
   so it names nothing in the game's art. Worth recording: this looked like the answer and was not.
3. `g_lib3d_map0` — the material's `MAPDESC*`. That pointer *is* identity: `BOB_TRACE_IMAGEMAP`
   logs every imagemap as it is created with its `(dir, file)` and `FileNum`, so the pointer seen
   at draw time resolves to `ImageMapNumber=0x020d, FileNum=39693`, whose load opens
   `MskMap16/THREAT01.X8`. The pairing rule (each trace immediately precedes its `fopen`) was
   checked across **all 241** loads before being relied on, not assumed from one example.
4. `THREAT01NO` appears exactly once in the source, in `DoThreat`.

### Two stale-global bugs in the instrument, both caught by disagreement with what was known

- `uniqueTextID` was first written only inside `if (!SameTransMat(...))`, so any poly reusing a
  material kept its predecessor's id. It reported **one id for five different `glTex` values** —
  visibly impossible, which is the only reason it was caught.
- The global was never invalidated, so draws arriving by other paths reported the last T-poly's id
  instead of "unknown". Now reset after every draw: five of the seven textures over the blob
  correctly report `NULL`, and only the two that really pass through `RenderTPolyList` carry an id.

A global written on one path and read on all of them will always answer, and a plausible wrong
answer is worse than no answer here — this is the same failure as the S307 identification, in a
different medium.


## S310 — the combo fallback stopped being a guess

S289 caught `CRCombo::GetIndex` returning stack garbage when no OLE host answered, and defaulted it
to 0. This file has said ever since that the 0 is *damage control, not a fix*, and it was right: the
one caller that matters is `SETFIELD`'s write-back, which takes the returned value and writes its
**bits into `Save_Data.gamedifficulty`**. A deterministic wrong answer is still a wrong answer, and
the preference it corrupts includes `GD_GUNCAMERAATSTART` — the one deciding whether a sortie is
recorded at all.

The fix is not a better default. There is no correct value to invent, so the write-back now **skips
the write** when the dispatch went unanswered, and says so.

### The guard had to be made observable before it could be believed

In a healthy run `bob_settings_nav` reports **"combo dispatches returning stack garbage: 0"** — the
condition never occurs, so the guard's behaviour is unobservable and indistinguishable from no guard
at all. `BOB_COMBO_FORCE_UNANSWERED=1` forces it:

| arm | unanswered | gun-camera preference (want 2) |
|---|---|---|
| healthy, default | 0 | **2** — gate PASSes |
| forced fault, guard on | 20 | **none** — the write is skipped |
| forced fault, guard off (`BOB_NO_COMBO_GUARD=1`) | — | **0** — a fabricated value stored |

The third row is the point. Under the identical fault the old code writes a **0** into
`GD_GUNCAMERAATSTART`, silently turning the gun camera off; the new code leaves the setting alone.
That harm had been described in this file for twenty sprints and never demonstrated — it took one
env var to turn the description into a measurement.

Both forced arms make the gate go **red**, which is correct: preferences that fail to store are a
failure, and the guard converts a silent corruption into a loud one.


## S311 — the same defect at 30 more call sites, fixed once at the router

S310 fixed `CRCombo::GetIndex`. The obvious next question is whether it was alone. It was not:
auditing every value-returning `InvokeHelper` in the port — does the local `result` get initialised
before the call? — gives **31 sites, and 30 of them are unguarded**:

| file | count | examples |
|---|---|---|
| `RLISTBOX.CPP` (both twins) | 22 | `GetCount`, `GetRowFromY`, `GetColFromX`, `GetListHeight`, `GetString` |
| `RSPINBUT.CPP` | 4 | the price/value option setters |
| `RTABS.CPP` | 3 | `CalculateHeight`, `CalcWidestWord`, `SelectTab` |
| `RCOMBO1.CPP` / `RCOMBO.CPP` | 2 | `GetListbox` |

`GetRowFromY` and `GetColFromX` are what the port's own click resolution uses to turn a pixel into a
menu row or a tab column. S289 found one of these the hard way, after it silently corrupted a
preference; the rest were the same defect waiting for the same conditions.

**Patching 31 sites means inventing 31 default values.** The root is one line —
`if (h) h->dispatch(...)` leaves `pvRet` untouched when a control has no host — so the fix is there:
zero the return buffer, sized by the caller's `VARTYPE`, and raise a flag. Zero is not claimed to be
right; there is no right answer. It is *deterministic*, and the flag lets callers that can do better
do better — the settings write-back declines to store anything at all.

⚠️ Sized by `VARTYPE` deliberately: these wrappers pass a `short` for `VT_I2` and a 4-byte
`long`/`BOOL` for `VT_I4`/`VT_BOOL`. Writing four bytes into a `short` would corrupt whatever sits
next to it on the stack — trading a read hazard for a write one.

### The change would have silently disabled S310

S310 detected "nobody answered" with a sentinel: set `result = -0x5EED`, call, see if it survived.
The router now writes 0 into that same slot, so the sentinel would never survive, `GetIndex` would
report *answered* every time, and the guard added one sprint earlier would quietly stop working —
with every gate still green, because the healthy path never exercises it. `GetIndex` now reads the
router's flag instead. **A sentinel only works while nothing else writes that slot**, and the thing
that broke it was a fix for the same underlying defect.

Re-verified after the rewire, all three arms unchanged: healthy → 0 unanswered and the gun-camera
preference stored 2 (PASS); forced fault with the guard → write skipped; forced fault without it →
a fabricated 0 stored.

### Two hooks, two different claims

- `BOB_COMBO_FORCE_UNANSWERED=1` forces the flag **inside `CRCombo::GetIndex`** and never reaches
  the router.
- `BOB_OLE_FORCE_NOHOST=1` makes every host lookup **in the router** miss.

They test different code. A front-end boot yields **0** unanswered dispatches, and that zero only
means something because the injector yields **28** — measured, not assumed. Using the first hook to
"prove" the second is how a control quietly stops controlling anything.


## S312 — the same fix for `getprop`, and an honest note that it is defensive

`bob_ole_getprop` has the identical shape to `bob_ole_invoke` (`if (h) h->getprop(...)`, caller's
buffer untouched otherwise), and the R* controls' property getters declare `result` uninitialised at
**206** sites: VT_I4 154, VT_BOOL 45, VT_I2 6, VT_CY 1. Same treatment — zero by VARTYPE, raise the
same flag.

**Not 219, which is what my first audit said.** The other 13 are `VT_BSTR` and pass a **`CString`**,
not a raw BSTR: `CString result; GetProperty(DISPID_CAPTION, VT_BSTR, &result);`. That is a
default-constructed object — already valid and empty, so never a hazard — and zeroing four bytes of
one would null its internal pointer and corrupt it. The script counted them because it looked for an
`=` and a default constructor is an initialiser it could not see. The router leaves `VT_BSTR` alone.

### ⚠️ This one is defensive, and says so

The invoke fix had a demonstrated victim: S289's `GetIndex` silently corrupted `GD_GUNCAMERAATSTART`,
and the three-arm A/B reproduces it on demand. This one has no such demonstration. With
`BOB_OLE_FORCE_NOHOST=1` — every host lookup forced to miss — the unanswered branch **never fired**,
on a title boot or a settings-screen boot. So the count was moved ahead of the `if`, to separate
"the branch never ran" from "the function is never called":

**`bob_ole_getprop` is called 0 times** on the front-end and Sim Config paths.

So the 206 sites are unreachable on everything currently exercised. The fix costs nothing and closes
the hole if those paths ever wake up, but it is **not** evidence of a bug that was happening — and
the difference matters, because a defensive change described as a fix inflates what the port is
known to have solved.
