# Battle of Britain — Linux port backlog

Items are tracked here rather than scattered through `PORT.md` entries, which is where they lived
until 2026-08-25. `PORT.md` remains the dated engineering log; this is the ordered work list.

---

## R — Replay, and Tacview export *(PO-added 2026-08-25)*

> **PO:** *"get replay working, add tacview export as well as .cam export to bob"*

MiG Alley reached this first, and the two ports share the engine, so **most of the cost is already
paid** — but the order matters and R1 must come before R2.

| # | Story | Pts | Acceptance | Status |
|---|-------|-----|------------|--------|
| R1 | As a player, I can record a flight in BoB and replay it. | 13 | **RECORDING: ✅ DONE and gated (S260/S262). READER: ✅ parses its own recording (S266).** Remaining: the UI path (choose a replay and play it) and confirming the 3D view actually moves. | Fly, exit, open the replay and watch the flight back; the recording round-trips (`header + frames == EOF` exactly, the arithmetic MA's `replay_record.sh` asserts). | 🔨 **NEW — do this FIRST.** BoB has the same `Replay` class as MA (`SRC/COMMS/REPLAY.CPP`, same `ReplayRead`/`BackupSmokeInfo`/`LoadBlockHeader` shape), and **S241 already fixed the 32-bit `ReplayRead` overflow here pre-emptively**, plus a second copy of the unchecked-size pattern in `BackupCloudInfo` that MA does not have. Unknown: whether BoB's record/playback path is reached at all in this port. **S258 did the reach check, and the answer is: THE RECORDER IS NOT BROKEN, IT IS NEVER ARMED.**

**Reach established at two levels before concluding anything.** (a) `BOB_BOOT_FRONTEND` reaches `[boot] View3d interactive; draw thread running` with a 1037-item world; (b) the *real* campaign path reaches `[frontend] (bridge) Launch3d done; InThe3D=1`. **Both fly. Neither produces a single `[reclog]` line or any `.dat`/`.cam` file.**

**The machinery is intact and identical to MA's** — `Replay::OpenRecordLog` exists, `Winmove.cpp` calls it behind `_Replay.StartRecordFlag`, and `Transite.cpp` sets that flag at the same three relative places MA does.

⭐ **All three arming sites carry the SAME gate, and all three sit in WEAPON-FIRE paths:**
```c
if (Save_Data.gamedifficulty[GD_GUNCAMERAONTRIGGER] && !_DPlay.Implemented && !_Replay.Record)
```
So BoB records **only when the gun-camera-on-trigger preference is on AND the player fires**. Automated flights never shoot, so nothing arms. This matches MiG Alley's manual exactly — *"the gun camera can either be off all the time, or on when the trigger is pressed, or on all the time"* — which means **the "on all the time" mode is not one of these three sites** and is the thing to find next.

**NEXT STEP (testable, not speculative):** set `GD_GUNCAMERAONTRIGGER`, fly, fire, and check for `[reclog]`. If it arms, R1 is a *preference-plumbing* story, not a repair. Two twin traps already avoided here: the compiled files are **`Winmove.cpp`** and **`Transite.cpp`** (mixed case), not their SHOUTING twins. |
| R2 | Saving a replay also writes a Tacview `.acmi`. | 8 | ✅ **DONE (S268/S272).** 148 distinct objects on a campaign flight, coloured by side (288k Blue / 273k Red samples). Units cross-validated: **IAS vs position-derived speed = 0.9656**, anchored by the PO's own cockpit HUD (`Speed 177Kts, Alt 5123ft`). Remaining polish: the file is **57 MB** for one sortie at 25 Hz x 148 objects — worth a sampling option. | `.acmi` appears beside the `.cam`; the `.cam` is **byte-identical** to what the same save produced before. | 🔨 **NEW — blocked on R1.** Nothing should be built on a replay path that does not work; MA learned this the expensive way (EPIC L's L1 was blocked on PO-68 for exactly this reason). **`SRC/compat/ma_acmi.cpp` should port nearly unchanged** — it deliberately takes plain C types and knows nothing about the game's structures, so only the tee's field names differ. |
| R3 | Every aircraft exports, not just the player. | 5 | AI aircraft appear as distinct objects. | 🔨 **NEW.** MA walks `*AirStruc::ACList` stepping `*ac->nextmobile` — **the same link the replay reader uses** (MA S226 cost four sprints to learn that; do not re-derive it). |

⭐ **Carry MA's two hard-won constraints into R2 unchanged:**
1. **Tee from the SIM, do not convert the `.cam`.** A `REPLAYPACKET` is packed deltas against a
   reconstructed world; converting one means re-implementing the playback integrator and inheriting
   every alignment bug (MA S211/L0).
2. **Additive only, so the existing replay path is its own control.** "Did I break the recording?"
   must be answerable by `cmp` and arithmetic, not judgement.

---

## P — Carried from the engineering log

| # | Item | Status |
|---|------|--------|
| P5 | BDG-oracle audit — which parity fixes were made against the BDG binary rather than Rowan's source | 🔨 **Partly done (S228/S230).** First two candidates came back **clean**; S228's supporting examples were retracted. The *risk* is real but currently unillustrated. |
| P6 | Present rect vs click rect divergence (`bob_check_present_rect`) | 🔨 **UNDEMONSTRATED.** S232's detector sat in the wrong function until S256 (the 2D front end presents through `bob_gdi_present`, not `present_surface`), and the positive control could not create the condition — neither port's window is `SDL_WINDOW_RESIZABLE`. **Next step: drive a real mode change, not a WM resize.** |
| P7 | `WM_*` route coverage (`doc/wm-route-audit.md`) | ⚠️ **S250's headline RETRACTED by S254** — I audited the dispatcher by reading a file that does not enumerate what it dispatches. The dispatcher is **default-on since S168** and working (80 dispatches vs 0 with `BOB_NO_MSG_DISPATCH=1`). What stands: the inventory of 20 sent messages and which have handlers. What is unknown: per-route runtime coverage. |
| P8 | `LPTSTR` eventsink thunk | ✅ **S252** — closed while **latent** (BoB hosts no edit controls, so nothing fires it today). Cross-ported from MA S251, where the same gap destroyed save files. |


---

## V — Visual defects *(PO-added 2026-08-25)*

| # | Story | Pts | Acceptance | Status |
|---|-------|-----|------------|--------|
| V1 | No stray geometry floats in the cockpit view. | 5 | Flying a mission, no unattached grey quad drifts across the canopy. | 🔨 **NEW (PO-73).** PO: *"a floating box drifted by"*, with a capture showing a **grey rectangle in the sky**, upper-centre, unattached to any aircraft or scenery, while the HUD read `Alt 5123ft Hdg 133 Speed 177Kts`. Not investigated. First question is the cheap one and it is NOT "which shape is it": **is the box drawn in the 3D scene or composited over it?** A scene object has depth and parallax with head movement; an overlay does not. That one observation splits the candidate space in half before any code is read. |

## S282 — aircraft types named in both exports (CLOSED)

Tacview draws the right airframe now: BoB emits Spitfire / Hurricane / Bf-109 / Bf-110 / Ju-87 /
Ju-88 / He-111 / Do-17 / Defiant, MA emits MiG-15 / F-86 / F-80 / F-82 / F-84 / F-51 / Yak-9 / F4U /
Meteor / B-29 / B-26 / C-54 / C-47.

`classtype->phrasename` is an offset from a base in the generated radio-phrase enum, so the names
come from the game's own header rather than a guessed table. THE BASE DIFFERS PER GAME -- BoB uses
the singular PHRASE_ONE_AIRCRAFT (0x2700), MA the plural PHRASE_NAMED_ACS (0x1400) -- which is why
each was fixed from call sites in its own tree instead of sharing one constant.

This also retired an open item the slow way had stalled on. S279/S281 named two MA types by counting
aircraft and sides; that reasoning could not separate the rest, and 5123/5130 sat unnamed. The header
CONFIRMED both hand-derived calls (5121=PHRASE_MIGS, 5122=PHRASE_F86S) and supplied the remainder:
5123=F-80, 5130=B-29. Worth remembering which move actually paid: the counting argument was sound but
capped, and the thing that broke the cap was looking for where the game keeps the answer itself.

Verified: BoB campaign run, 511 markers, four types, ZERO unrecognised ids (BOB_TRACE_ACMI_TYPE=1 is
the instrument that would have said otherwise). MA 40-aircraft run, four types, no AC-<id> left.
Coverage limit: only 4 of BoB's 11 and 4 of MA's 13 rows have been seen in a live run; the unseen
rows are transcribed from the header and unexercised.

## S289 — the gun-camera combo: write path verified, UI reach NOT achieved

**R1's remaining question was "does the Preferences combo write GD_GUNCAMERAATSTART?" Partly answered.**

**Verified by reading the path end to end:** `SETFIELD` (SG2COMBO.H) does `val = rcombo->GetIndex()`
and `ADDBIT` maps bit 1 -> GD_GUNCAMERAATSTART, so combo item 2 arms the recorder. The port's combo
host DOES answer that dispatch -- `HostRCombo::dispatch` case 12 returns a real index -- and combos
are bound to a host by `bob_ole_create_control`. So for a hosted combo the write path is sound.

**Defect found and hardened along the way.** `CRCombo::GetIndex` returned an UNINITIALISED long:

```c
long result;                                   /* never initialised */
InvokeHelper(0xc, DISPATCH_METHOD, VT_I4, &result, NULL);
return result;
```

`bob_ole_invoke` is `if (h) h->dispatch(...)` -- with no registered host it never writes pvRet, and
the caller gets stack garbage. This is not a cosmetic read: SETFIELD writes val's BITS straight into
Save_Data.gamedifficulty, so an unanswered dispatch silently sets or clears real preferences,
GD_GUNCAMERAATSTART among them. Now sentinel-detected, reported under BOB_TRACE_COMBO=1, and
defaulted to 0. ⚠️ THE 0 IS DAMAGE CONTROL, NOT A FIX -- it is a guess ("first item"), chosen
because deterministic beats random, and it is traced rather than silent for exactly that reason.

**NOT ESTABLISHED: that a user can set this from the UI.** The instrument recorded ZERO GetIndex
calls, because no recipe here opens the Preferences dialog -- grep of tools/ finds none. A zero from
an instrument whose code path was never reached says nothing, which is the S258/S204 lesson and the
reason this is written as an open gap rather than a pass. NEXT: a recipe that reaches the settings
screen; until then R1's UI claim is untested, and only the preference MECHANISM is proven (S287).

## S291 — the settings screen is reachable headlessly; R1's UI claim now has evidence

**S289 could not answer "does the Preferences combo write GD_GUNCAMERAATSTART?" because nothing here
had ever opened that dialog** -- the instrument logged zero calls, and a zero from an unreached code
path is not a result. `tools/bob_settings_nav.sh` is the missing recipe. PASSES.

Navigation was MEASURED, not guessed off a screenshot (BOB_DUMP_HITTARGETS=1 dumps every hit rect):
main menu 6=Sim Config; tab bar 3=Views; the Gun Camera combo is id 1075. Note it lives on VIEWS,
not Game -- I looked on Game first because "difficulty setting" was the wrong mental model.

**Result: `[setfield] combo id=1075 -> val=2`.** ADDBIT maps bit 1 -> GD_GUNCAMERAATSTART, so the UI
does store the preference. Watched at the WRITE-BACK rather than read off the screen: the combo
rendering "On" would only prove what was painted.

**R1 recording chain, both links measured:**
  1. UI combo -> val=2 -> GD_GUNCAMERAATSTART set  (S291, this gate)
  2. GD_GUNCAMERAATSTART -> 182137-byte recording + 511 ACMI markers  (S287)

⚠️ **The two halves were measured in SEPARATE runs** and joined at a variable both read. That join
is an inference, however short -- no single continuous session has gone UI -> flight -> recording,
because committing the setting did not write any file on disk, so it cannot be staged across runs.
Said plainly rather than rounded up to "end-to-end verified".

**Side result:** the S289 uninit hazard (`CRCombo::GetIndex` returning stack garbage when no OLE host
answers) did NOT fire on the real settings screen -- 12 dispatches, 0 unanswered. The hardening
stays, and the gate now asserts that count is zero so a regression would be caught.

## S293 — PO-73 (grey box) is UNREACHABLE: no recipe here renders a 3D frame

**Hypothesis tested and rejected:** the renderer drops a texture whose dimensions look garbage, and
untextured geometry draws grey -- a good fit for "floating grey box". `BOB_TRACE_GARBAGE=1` over a
flight that DID reach interactive 3D reported **zero** skips. Reach checked first, so the null means
something this time.

**Then the instrument problem.** `BOB_SHOT` lives in FULLPSYS.CPP and captures the 2D front-end
canvas; a mid-flight shot silently writes no file. So nothing here could photograph a 3D frame --
the S233 blind spot again (x11grab cannot see a GL window either). Added `bob_shot3d_maybe()`:
glReadPixels of the default framebuffer immediately before the swap, at ALL SIX swap sites.
`BOB_SHOT3D=N` grabs one frame, `BOB_SHOT3D_EVERY=M` grabs a sequence -- a box that DRIFTS needs a
sequence, which is what makes this report hard.

**It captured nothing, and THAT is the finding.** At every 10th frame, over a run that logged
"View3d interactive; draw thread running" and recorded a replay: zero grabs. `SDL_GL_SwapWindow`
appears in exactly one file tree-wide, all six sites are instrumented, and the helper never ran.

⭐ **So the headless flight recipes NEVER PRESENT A FRAME.** They simulate, record, and export
correctly -- S287's 182137-byte recording and 511 ACMI markers are real -- while drawing nothing to
screen. Every "flight" gate here has been flying blind, which is precisely why a visual defect the
PO can see in one glance has no instrument that can reach it.

**NEXT (blocks PO-73 and every future 3D visual report):** a flight recipe that actually renders.
Note `gl-gates-need-an-unlocked-session` -- a locked desktop already makes real-GL runs hang at the
title, so the presenting path is known to be environment-sensitive; this is likely the same family.
Until then PO-73 cannot be reproduced here, and asking the PO to re-photograph it is the cheaper
path than guessing.

## S295 — PO-73 REPRODUCED, and S293's conclusion RETRACTED

**⚠️ RETRACTION FIRST. S293 concluded "the headless flight recipes never present a frame" and wrote
it up as the sprint's finding. IT IS FALSE.** `BOB_TRACE_PRESENT=1` on the same recipe:
**15,480 presents**, via the 3d-fb path, centre pixel rgb=(133,165,204) -- sky blue. The flight
renders perfectly. What actually happened is that the instrument I built in S293 never ran, and I
read "my new probe is silent" as "the engine does not present", which is the same mistake this log
already records three times: trusting a null from an instrument whose own reach was never checked.
A silent probe is a fact about the probe until proven otherwise.

**Two compounding errors, both cheap to have avoided:**
1. `BOB_DUMP_FRAME=N` ALREADY dumps a 3D frame (present_dbg, bob_video.cpp:796). S293 built a
   duplicate without grepping for prior art first.
2. That duplicate never executes despite sitting at the same swap sites as the working dumper, and
   there is no case-colliding twin to blame (the build compiles the edited file). Still unexplained;
   it is now a curiosity, not a blocker, because the pre-existing dumper does the job.

**PO-73 REPRODUCED.** `BOB_DUMP_FRAME` captures a Spitfire cockpit with a **grey elliptical blob in
the upper-left sky** -- the PO's "floating box that drifted by".

⭐ **IT IS SCREEN-LOCKED, NOT WORLD-LOCKED.** Two captures at different times, heading 240 -> 228
(12 deg of yaw, ~160 px of expected horizontal shift at this FOV), and the blob sits at the SAME
screen coordinates (~x18..118, y50..73) in both. World geometry cannot do that. So it is a 2D or
screen-space element with a broken transform, NOT a mis-drawn aircraft, cloud or shadow -- which
rules out most of the obvious suspects in one measurement.

**NEXT:** identify the screen-space draw. Candidates: a spotting/label sprite, a flak or smoke
billboard, or an overlay quad. The 2D draw path is `bob_gdi_present`/overlay, and the blob's fixed
rect makes it findable by logging small screen-space quads in that region.

## S297 — PO-73: the blob's draw path is identified; its identity is NOT yet proven

**Probe added** (`BOB_TRACE_BLOB=1`): logs draws whose screen bbox lands in the blob's known
rectangle, with a backtrace. Four hits, all `is2D=1`, all textured.

**Caller path (addr2line):**
`ThreeDee::render3d -> Lib3D::FlushAsBackground -> Lib3D::EndScene -> Lib3D::RenderTPolyList ->
DEV_DrawPrimitiveVB -> draw_fvf` -- the BACKGROUND TRANSPARENT polygon list, i.e. the billboard/
sprite path, not aircraft or terrain geometry.

**What the blob looks like, enlarged:** a smooth grey ellipse with a soft edge and NO texture
detail, seen near edge-on.

**Hypothesis raised and PARTLY KNOCKED DOWN:** an aircraft shadow disc drawn at the wrong altitude
would look exactly like this, and would also explain the screen-locked observation -- a shadow
attached to the PLAYER's own aircraft holds its screen position while the player yaws, so
"screen-locked" does NOT after all require a 2D element. But BoB's `DrawShadowTriangle` is DEAD CODE
(TILEMAKE.CPP, commented out JON 18Oct00), so shadows are not drawn that way here.

⚠️ **NOT PROVEN: that any of the four logged quads IS the blob.** They are textured (hasTex=1, live
glTex) while the blob shows no texture detail, and I have not tied a specific draw to the pixels.
Recording this as an open thread rather than promoting a plausible caller to a conclusion -- S295's
retraction in this same file came from exactly that move.

**NEXT, and it is decisive and cheap:** hilite the candidates. The renderer already has
`BOB_GARBAGE_HILITE` painting suspect geometry magenta; do the same for blob-region quads and
capture a frame. If the ellipse turns magenta, identity is settled in one run. If it does not, the
blob is drawn somewhere the probe never looked and the search restarts with that ruled out.

## S299 — PO-73: the S297 caller is ELIMINATED (by experiment, not argument)

**Result: the candidates were painted magenta and the blob stayed GREY. Zero magenta pixels in its
region.** So the ellipse is NOT drawn by anything the S297 probe catches, and
`FlushAsBackground -> RenderTPolyList` -- the plausible caller S297 named -- is ruled out.

⭐ **This is why S297 stopped short of calling it identified.** The backtrace was real, the caller
was plausible, and it was wrong. One run settled what more reading would not have.

**WHY THE PROBE WAS BLIND, which matters more than the negative:** every hit it logged was
`is2D=1`, and its bbox test compares vertex positions against a SCREEN rectangle. For 3D draws
(`is2D=0`) those positions are in world/clip space, so no 3D draw could ever match the test
regardless of where it lands on screen. The probe could only ever have found a 2D culprit -- it was
incapable of finding a 3D one, which is now the leading possibility.

**Also tried and USELESS as a discriminator:** `BOB_NOTEX=1` to test whether the blob is textured.
It renders the WHOLE scene white -- blob and sky both (255,255,255) -- so it separates nothing here.
Recorded so the next reader does not spend a run on it.

**NEXT:** a probe that can see 3D draws -- transform the vertices by the active matrices before the
screen-rect test, or hilite by primitive shape/size independent of space. The blob is a smooth
soft-edged ellipse, which in a 3D path suggests a billboard or an impostor/LOD sprite for a distant
object. Note the shadow hypothesis is NOT dead: BoB's DrawShadowTriangle is dead code, but that only
rules out THAT shadow implementation, not a shadow drawn some other way.

## S301 — the S299 test was INVALID (right answer, void evidence); probe now projects 3D

**⚠️ CORRECTION TO S299.** That sprint reported "candidates painted magenta, blob stayed grey,
therefore innocent". **The hilite never fired.** The probe's outer condition was
`getenv("BOB_TRACE_BLOB") && count>=3`, and the S299 run set only `BOB_BLOB_HILITE` -- so the block
never executed, `garbageHi` was never set, and NOTHING was painted. "Zero magenta pixels" measured a
switch I had failed to turn on, not the geometry. The conclusion was right by luck.

**Now tested properly** (both vars set): magenta DOES appear -- 91 px in the sampled window, proving
the hilite path works -- and **the blob is STILL GREY**. So the 2D candidates really are innocent,
and `FlushAsBackground -> RenderTPolyList` really is eliminated. Same conclusion, real evidence.

**Probe upgraded:** for `is2D=0` draws it now projects each vertex through the live
MODELVIEW/PROJECTION matrices and viewport before the screen-rect test, instead of comparing raw
model-space coordinates against screen pixels (the S297 defect).

⚠️ **AND ITS NULL IS NOT YET TRUSTWORTHY: zero 3D hits.** The projection has NO positive control --
nothing confirms it produces sane screen coordinates for a draw known to be on screen. A probe
reporting "no 3D draw is there" while possibly computing garbage is the exact failure this log has
recorded four times. **Do not conclude the blob is non-3D.**

**NEXT, in this order:** (1) validate the projection -- log the projected bbox of ANY 3D draw and
check it lands in plausible screen ranges; only then (2) trust or act on the zero-3D-hits result.

## S303 — PO-73: the projection was moot, and a strong suspect emerged

**Positive control first, as S301 required -- and it changed the picture.** `BOB_TRACE_PROJ=1`
logged **zero** 3D draws, because `is2D` is NEVER false on this path: every primitive reaching
draw_fvf is already transformed. So (a) S301's projection code is dead, (b) its "zero 3D hits" null
is fully explained and was never evidence about the blob, and (c) the culprit MUST be a 2D draw the
region test was failing to match. The positive control earned its keep: it retired a whole line of
enquiry that would otherwise have looked open.

**Containment instead of size** (an alpha billboard's quad is far larger than its opaque part):
the blob's centre is covered by MANY draws -- the cloud billboards blanket that whole region, so
"the centre turned magenta" proved only that something big covers it, not that it IS the blob.

**Distinct textures drawn over the blob's centre -- ten, and one stands out:**

| glTex | size | quad |
|---|---|---|
| 11, 27, 45, 49, 50, 51, 53, 54, 56 | 256x256 / 512x512 | cloud-sized billboards |
| **0** | **128x128** | **125x125** |

`glTex=0` means the texture has dimensions and bpp but NO GL texture object -- never uploaded, or
the upload failed. Binding 0 draws UNTEXTURED, which in this renderer is exactly the flat grey the
blob shows. That is a strong suspect on mechanism, not just on correlation.

⚠️ **NOT CONFIRMED.** Hiliting only glTex=0 left the blob GREY at the sampled frame (centre
rgb=(127,139,153)). Either it is not the culprit, or that draw is intermittent and absent from
frame 8000 -- the texture list was accumulated over a whole run while the hilite test samples ONE
frame. Those are different claims and this run cannot separate them.

**NEXT:** make the test per-frame -- on the frame that is dumped, log every draw covering the blob
centre with its glTex, so suspect and evidence come from the SAME frame. Also worth checking why any
texture reaches draw time with glTex=0 given the `if (t->texDirty || !t->glTex) upload_texture(t)`
guard immediately above: an upload that silently fails would be a defect in its own right.

## S306–S309 — PO-73 CLOSED: the blob is the game's threat indicator, working correctly

This thread ended at S303 with a next step and a hypothesis, both of which were carried out. Both
answers were no. Recording the closure here because this file is the running PO-73 log, and a log
that stops mid-investigation reads as an open invitation to redo it — which is exactly what the
julia-racer port lost three sprints to this week when a retraction was filed under one track and
the rotation returned by another.

**S303's "why does any texture reach draw time with glTex=0" — refuted, under a control.** The
probe read `t->glTex` about 130 lines *before* `upload_texture` ran at bind time, so the first draw
of every texture reported `glTex=0`, healthy ones included. Suspect and evidence came from different
instants — the very defect S303 asked to fix. With the upload hoisted above the probe, the count of
uploads that bail before creating a texture is **0** in the frame that contains the blob.
That zero is a measurement, not a silence: `BOB_TEXFAIL_EVERY=N` forces the bail path and moves
32% of the frame's pixels.

**The hilite could never have answered this, and S299's negative result was uninterpretable for the
same reason.** `BOB_BLOB_HILITE` does `glDisable(GL_TEXTURE_2D)` plus a flat colour, which discards
the texture's **alpha** — a transparent cloud billboard paints as an *opaque* magenta rectangle. Any
bisect built on it follows the largest transparent quad. `BOB_BLOB_SKIP` omits the draw instead,
leaving every other draw untouched.

**Identified by skip-isolation:** omitting `glTex=43` removes the ellipse and nothing else. Naming
it needed identity to survive the deferred transparent list: `g_lib3d_map0` carries the material's
`MAPDESC*` to the GL layer, and `BOB_TRACE_IMAGEMAP` logs every imagemap at creation with its
`(dir,file)` and `FileNum` — resolving the blob to `ImageMapNumber=0x020d` → **`MskMap16/THREAT01.X8`**.
That pairing rule was checked across all 241 loads before being relied on.

**`THREAT01NO` appears exactly once in the source**, in `COverlay::DoThreat` (`SRC/3D/OVERLAY.CPP:7756`)
— *"display threat indicator in the top left corner of the screen"*. Its constants predict centre
(75.0, 71.9), 125 wide, 18.75 tall; the measured ellipse is centred (75.5, 71) on a 125x125 quad,
21 tall. **Nothing is misplaced, mis-scaled or mis-textured.** It looks featureless because it has
no contacts to plot — the player is alone in that scene. It is gated on
`Save_Data.gamedifficulty[GD_HUDINSTACTIVE]`, so the PO can switch it off with the HUD-instruments
difficulty setting.

**The S307 misstep, kept because it is the useful part.** S307 named `glTex=43` "the aircraft shadow
sprite" from the *texture's appearance* — a dark flat ellipse at 40% alpha does look like a shadow
blob. `do_object_shad`, the only route a `SHADOW_OBJECT` takes into the draw list, is entered **0
times**; no shadow is drawn in this scene at all. The draw had been identified by measurement and
the *name* by inspection, and the two sat in one sentence where the second inherited the first's
confidence. "What does this look like" and "what code puts it there" are separate questions, and
only the second one names a defect.
