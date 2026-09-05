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
| R3 | Every aircraft exports, not just the player. | 5 | AI aircraft appear as distinct objects. | ✅ **DONE (S436–S439).** Measured on the exported file, not in-process: **39 AI aircraft + 1 player** as distinct objects, **0 ids whose `Name` changes and 0 whose `Color` changes across 6,535 time markers**, ids are game uids (`136c`…) not 1..N. Identity fixed to `uniqueID.count` in S437. Known limit: the walk still caps at `_id < 256` and the list runs to 148, so a raid 73 % larger truncates silently. Original note: MA walks `*AirStruc::ACList` stepping `*ac->nextmobile` — **the same link the replay reader uses** (MA S226 cost four sprints to learn that; do not re-derive it). |

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

---

## ⭐ PO PRIORITY RULING (2026-09-05)

PO, verbatim: *"backlog priority, highest first: ma EPIC M, bob R3, ff GMRADAR-8 and PIT-1,
julia PERF-1, AI car rear-tyre rods, also ma and julia multiplayer"*

**BoB's only named item is R3 — every aircraft exports, not just the player — at rank 2 overall.**
Everything else on this board (R1's remaining UI path, R2's 57 MB sampling polish, P5/P6/P7, UI-2,
MP-3) sits below the ruling's six ranks and keeps its existing relative order.

**R3 is NOT blocked.** It was written when R2 was open; R2 closed at S268/S272, so the tee exists and
R3 is additive on top of it. MA's `L3` is the same story and is done — walk `*AirStruc::ACList`
stepping `*ac->nextmobile`, **the same link the replay reader uses** (MA S226 spent four sprints
learning that; do not re-derive it).

BoB's Fable 5.1 parkings are untouched by this ruling: headless flight ("aircraft will not roll")
and R3.2 (depth sort / cockpit) stay parked, and R3.2's propeller and cloud/canopy cases with them.

## ⭐ PO CADENCE RULE CHANGE (2026-09-05) — **4 sprints per item, not 8 or 12**

PO, verbatim: *"continue scrum, highest backlog items first, then other backlog items, no more than
4 sprints on any one backlog item"*.

**This supersedes the old 8-sprint (BoB/Julia) and 12-sprint (MA/FF) limits.** From now on an item
gets **at most 4 sprints in a pass**, then the loop moves to the next item.

**My reading, stated so it can be corrected in one word:** 4 sprints is a **rotation cap, not a
death sentence** — the item stays open and is eligible again on a later pass through the backlog. It
is not the old rule's "mark it for Fable 5.1 and never run it again". Items already parked for
Fable 5.1 stay parked; the new cap does not retroactively re-park anything, and it does not re-park
MA's MP-2, which the PO un-parked by naming it at rank 6.

Order within a pass: the PO's 2026-09-05 priority ruling first (MA EPIC M → BoB R3 → FF GMRADAR-8
and PIT-1 → Julia AI-CARGFX → Julia PERF-1 → MA and Julia multiplayer), then everything else.

---

## S436 (2026-09-05) — R3 is **IMPLEMENTED**; its acceptance fails on one thing, and MA has the same bug

Rank 2 of the PO's priority ruling. **The first question was whether R3 is still open at all, and
mostly it is not.** The board row says *"🔨 NEW"*; the code says otherwise.

### What is already built and evidenced

`SRC/COMMS/REPLAY.CPP:466` — `AirStrucPtr _ac = *AirStruc::ACList; while (_ac && _id < 256)` — the
all-aircraft walk R3 asks for, stepping `*_ac->nextmobile`, **already shipped with R2** (S268–S285).
With it: the dead/unplaced filters (S285, using the game's own `Status.deadtime` predicate), side
colouring, player identification via `Manual_Pilot.ControlledAC2` (S274b), AI decimation to 5 Hz
(S274), and the S282 type names. Evidence on record: **148 distinct objects, 288k Blue / 273k Red
samples, 511 markers, four aircraft types, zero unrecognised ids.**

⚠️ **Checked first, because MA was bitten by it this same day:** `SRC/COMMS/Replay.cpp` here is an
intact **symlink** to `REPLAY.CPP`, and `_COMM.CPP` compiles it. BoB's pair is NOT split — unlike
MA's, where the two are separate regular files 781 lines apart. Same filename, opposite situation;
the check is cheap and is now the first move on any replay work in either tree.

So the board row is stale in the way this file already warned about at MP-4: *"a resolution buried
under an unchanged header is invisible to exactly the sweep that is supposed to find it."*

### 🔴 But the acceptance criterion is "AI aircraft appear as **distinct objects**", and that fails over time

**The object id is POSITIONAL.** `unsigned long _id = 0;` is reset every frame and `_id++` counts
position in `ACList`. The code already reasons about this once — *"the `_id++` stays OUTSIDE this
skip: object ids must keep referring to the same aircraft across frames"* — which handles a
**skipped** aircraft but not a **changed list**. And the list changes:

| site | operation |
|---|---|
| `MOVECODE/MOVEALL.CPP:1442–1443` — `nextmobile=ACList; ACList=*this;` | **head insertion** — a new aircraft is pushed on the FRONT |
| `BFIELDS/PERSONS3.CPP:3464` — `AirStruc::ACList=*Me->nextmobile;` | **head removal** |

Every spawn or despawn therefore shifts every subsequent aircraft's index by one, and from that
frame on **object N is a different aeroplane**. In Tacview that reads as tracks swapping identity
mid-file — a Spitfire's trace continuing as a Heinkel's. The objects are distinct at any one instant
and not consistently identified across the sortie, which is the half of "distinct objects" a debrief
tool actually needs.

**The engine already provides the stable identity.** `ac->uniqueID.count` is what the replay stream
itself writes for cross-references (`Replay.cpp:1737`, `:1772`, `:4080`, `:4087`…). Using it as the
ACMI object id is a smaller change than the walk that is already there.

**Secondary, same line:** `_id < 256` silently truncates the walk. A campaign that instantiates more
than 256 aircraft exports the first 256 and says nothing.

### ⭐ CROSS-PORT: MA's L3 is marked ✅ DONE and has the IDENTICAL defect

`~/ma/SRC/COMMS/Replay.cpp:558–601` is the same code — `unsigned long _id = 0; while (_ac && _id <
256) { _id++; … ma_acmi_object_ias(_id, …) }` — and MA's list is head-inserted by
`AirStruc::AddToList()` (`MOVEALL.CPP:891–892`) and head-removed at `PERSONS3.CPP:3139`. Same engine,
same bug, and MA's L3 row claims acceptance. **L3 should be reopened**; whichever port fixes it first
should hand the other the `uniqueID.count` change rather than both re-deriving it.

### Not fixed here, and why

The fix is small but it changes the **on-disk meaning of every object id**, so it needs its own
before/after: a recorded sortie exported both ways, with the id→aircraft mapping counted per frame.
That is a run, not a read, and it belongs in the sprint that lands the change. **State on handover:
mechanism proven from the code, instrument named (count ids whose backing pointer changes between
frames), and the identity to use already chosen.**

### S437 (2026-09-05) — R3: the fix is landed and PROVEN DISTINCT; ⚠️ **S436's "acceptance fails" claim is DOWNGRADED — the defect did not reproduce**

**Landed** (`SRC/COMMS/REPLAY.CPP`, additive, `BOB_ACMI_POSID=1` reverts): the ACMI object id is now
`_ac->uniqueID.count` — the engine's own `UNIQUE_ID` (a 14-bit field, `WORLDINC.H:245`), which the
replay stream already writes for every cross-reference — instead of the aircraft's position in
`ACList`. A zero uid (`UID_Null`) exports under a synthetic id at `0x4000 + _id`, a band real uids
cannot reach, rather than being dropped. Edited `REPLAY.CPP`, the real file; `Replay.cpp` is still a
symlink to it (checked after the write).

## ⚠️ The measurement, and what it did to my own claim

S436 asserted R3's acceptance *fails* because positional ids swap when `ACList` mutates. The
mechanism in the code is real — head insertion at `MOVEALL.CPP:1442-1443`, head removal at
`PERSONS3.CPP:3464`. **It did not fire.**

Instrument: remember which `AirStrucPtr` each id referred to last frame, count the ids whose backing
pointer changed. A/B under `tools/bob_convoy_campaign.sh` (the campaign recipe that produced R2's
148 objects), both arms reaching comparable depth:

| arm | walked | checks | **swaps** |
|---|---|---|---|
| control `BOB_ACMI_POSID=1` (positional) | 1,034,501 | 1,033,353 | **0** |
| fix (uniqueID) | 1,046,501 | 1,046,353 | **0** |

**A million opportunities in the control arm and zero swaps.** The aircraft set is established before
the tee starts and does not change during the sortie, so the positional scheme is never exercised in
the way S436 predicted. **The acceptance-failure claim is withdrawn and re-filed as a LATENT
hazard:** the code shape is genuinely fragile, and nothing observed is broken by it.

## What the run DOES prove, and it is worth having

**`uniqueID.count` is distinct across all ~148 aircraft and stable across 400+ frames.** Zero swaps
in the fix arm is not a null result here: a duplicate uid would have made two aircraft share a slot
and shown up as a swap on the following frame. So the identity adopted is genuinely unique, which is
the property R3 actually needs — proven, rather than assumed from the fact that the engine calls it
unique.

## Two honest caveats

1. **The first attempt measured nothing and looked like a pass.** The instrument reported on
   `_rep % 20000`, which fired only at iteration 0, so the first A/B printed `0 swaps / 0 checks` in
   both arms — a zero from an instrument that had never had a second frame to compare against. It was
   caught by reading `checks`, not `swaps`. `BOB_ACMI_IDEVERY` now sets the cadence (default 50), and
   the line reports `walked` and the frame number so a silent arm cannot be mistaken for a clean one.
   `instrument-bookkeeping-lies`, again, and it nearly banked a false pass.
2. **Both campaign runs were cut short by MY harness** (a 10-minute cap on the pair), at frames 333
   and 414 — not by the game. The counts above are from partial sorties. A full run could still
   produce a spawn; that is the only way this becomes reproducible, and it is the next test if anyone
   wants to close the latent hazard rather than leave it fixed-and-unproven.

**On-disk change to note:** object ids in exported `.acmi` files are now game uids, not 1..N. Files
recorded before and after this sprint are not id-comparable.

**R3: the walk, filters, colours, types and now the identity are all in. Sprint 2 of 4.**

### S438 (2026-09-05) — R3: the list is **exactly 148 and never changes**; the 256 cap has 1.7x headroom

Both remaining R3 threads answered by one direct measurement, replacing numbers I had *derived* and
got wrong.

**Instrument:** `_id == 1` marks the first aircraft of a frame, so the previous frame's count is
final at that instant; min/max reported in the existing trace line.

    [acmi] idscheme=uniqueID identity: 0 swaps / 965853 checks / 966001 walked (frame 383)
           listlen min=148 max=148 cap=256

* **`ACList` holds exactly 148 aircraft and does not change size across 383 frames.** That is
  independent corroboration of S437's zero swaps: the list is not mutated during a sortie, so the
  positional-id hazard cannot fire here. Two different instruments, same conclusion.
* **148 matches R2's recorded "148 distinct objects" exactly**, which is a useful cross-check that
  the walk and the export agree on what is in the world.
* **The `_id < 256` cap has 1.7x headroom, not the comfortable margin it looks like.** A campaign
  raid 73 % larger than this one truncates silently. Left as a known limit rather than raised: the
  right fix is to drop the cap and bound the walk on the list itself, and that wants a run with a
  bigger raid to be worth anything.

⚠️ **Correcting my own arithmetic from earlier this sprint.** I first derived list length from the
sampled trace and got "min 125.0, median 166.7, max 166.7", and read the spread as *"the list CHANGES
SIZE"*. Both figures were artifacts of reports landing every 500 iterations rather than on frame
boundaries. The direct measurement says **148, constant**. Deriving a quantity from an instrument
sampled on a different axis produced a confident wrong answer, and the fix was to measure the
quantity itself.

**R3 state: walk ✅, filters ✅, colours ✅, types ✅, identity ✅ (uniqueID, proven distinct),
list-size hazard ✅ measured-absent, `_id<256` cap ⚠️ known limit. Sprint 3 of 4.**

### S439 (2026-09-05) — R3 **CLOSED**, on evidence from the artifact rather than from the process

The three prior sprints measured the exporter from *inside* the game. This one reads the file a
player would actually open — `acmi_current.txt`, written live in the game directory (7.5 MB from the
S438 campaign run, under the new `uniqueID` id scheme).

| measure | result |
|---|---|
| distinct object ids | **40** |
| of which `Pilot=Player` | **1** (`136c`) — so **39 AI aircraft** |
| object samples | 55,504 |
| time markers | 6,535 |
| **ids whose `Name` changes** | **0** |
| **ids whose `Color` changes** | **0** |
| id form | game uids (`136c`, `136d`, `1370`…), **not** 1..N |
| samples spanned per object | min 47,622 / max 55,480 of 55,504 |

**That is R3's acceptance criterion met and measured.** "AI aircraft appear as distinct objects"
needs two things — distinct, and *the same* object throughout — and both are now properties of the
delivered file rather than of a counter I wrote. Zero name changes and zero colour changes across
6,535 markers is the artifact-level restatement of S437's zero swaps, arrived at by a different
route, which is why it is worth having both.

**Caveat on coverage, stated rather than glossed:** every object in this run is a **Ju-87** — the
Luftwaffe Convoys campaign launches a single Stuka squadron, so this file exercises one type. Four
types were evidenced separately at S282. This run proves *identity*, not type coverage.

**Also worth recording:** 40 objects exported from a 148-aircraft list. The gap is the S285
dead/unplaced filters doing their job — 108 aircraft tracked by the campaign but not instantiated in
the local 3D area, which is exactly what those filters exist to keep out of the file.

**R3 closes at 4 of 4 sprints.** Remaining known limit, carried not hidden: the `_id < 256` cap.

---

## 🔴 PO CRASH 2026-09-05 — client SIGSEGV joining a LAN game. **FIXED (guard), root cause is systemic**

**PO's report:** `BOB_DPLAY_HOST=192.168.254.14 …/BattleOfBritain-x86_64.AppImage`, into Multi-Player,
clicked around, went back (`menu item 0` → `painted screen artnum=28937`), then
`CRASH: signal 11 fault_addr=(nil)`, `eax=ebx=ecx=0`.

### Symbolised against the exact binary they ran (260905 image, mounted)

    0x820a434  DPlay::UIGetSessionListUpdate()   Comms.cpp:288
    0x82c673f  CSelectSession::OnTimer(unsigned)  session.cpp:235
    0x832cd07  bob_timers_tick                    bob_ole.cpp:1301
    0x80835c2  bob_msg_wait                       bob_video.cpp:216

`Comms.cpp:288` is `DPSessionDesc2.guidApplication = *lpAppGuid;` — **a NULL dereference**, which is
exactly `fault_addr=(nil)`.

### Mechanism, established from the code

1. Opening the Select-Session screen arms timer **2365** (`session.cpp:119`) that polls
   `UIGetSessionListUpdate()` every tick.
2. Leaving the screen runs **`DPlay::ExitDirectPlay()`** (`COMMS.CPP:154`), which sets
   **`lpAppGuid = NULL`** *and* `lpDP4 = NULL`.
3. **The timer is still armed**, so the next tick dereferences NULL.

⭐ **Why it survived review: the function already guards `lpDP4` four lines below the crash.** The
code *looks* defended. The guard existed for one pointer and not the other.

### Fixed now — three guards, additive

`UIGetSessionListUpdate()`, `UIAssignServices()` and `UINewPlayer()` all dereferenced `lpAppGuid`
(and two of them `lpDP4`) unchecked. Each now returns early if DirectPlay is not initialised, and
**says so on stderr** rather than failing silently — a screen that lists nothing must not look like a
screen that is working.

**Verified no regression on the happy path:** the join recipe still reports
`[sessions] UIGetSessionListUpdate -> 1 session(s), res=DP_OK` repeatedly, and the guard does not
fire. ⚠️ **The crash itself was NOT reproduced here** — this run initialises DirectPlay and never
tears it down mid-screen. The fix is justified by the code path and the PO's backtrace, not by a
local repro, and that distinction is recorded rather than glossed.

### ⭐⭐ ROOT CAUSE, and it is much bigger than this crash: `WM_DESTROY` IS NEVER DISPATCHED

`CSelectSession` **does** kill its timer — in `OnDestroy()` (`session.cpp:280`), wired with
`ON_WM_DESTROY()` at `:108`. It never ran. **`WM_DESTROY` appears nowhere in the port's compat
layer** — nothing sends or dispatches it.

**So no dialog's `OnDestroy` has ever run in this port: 25 files define one, and `SRC/MFC` holds 27
`KillTimer` calls.** Every timer armed by a dialog outlives it, and every other cleanup in an
`OnDestroy` is dead code. This crash is one instance of that.

**Not fixed here, deliberately.** Dispatching `WM_DESTROY` touches 25 dialogs at once and is not a
change to land while the PO is mid-session; the guard makes this crash impossible either way. Filed
as its own item — and it belongs with **P7** (`WM_*` route coverage) and MA's **N3**, which exist for
exactly this class. **This is the first CONFIRMED, crashing instance of that class, which raises its
priority from an audit to a defect.**

---

## 🔴 MP-5 (2026-09-05): multiplayer cannot reach 3D — **"No player A/C set up on entering 3d!"**

**Goal set by the PO:** a client BoB session attached to a host, in a real 3D multiplayer session.
**Result: the transport is fine and the FLIGHT is the blocker.** Diagnosed end to end on this box by
running a HOST and a CLIENT as two instances over loopback, each in its own scratch game tree.

### What now works, fully automated (no human clicks)

* **Host:** `BOB_AUTOCLICK="2,1,1"` → Multi-Player → Create Game → Continue. Reaches its Ready Room,
  `host bound to UDP 47624`, `Open(CREATE) session "BoB"`.
* **Client:** `BOB_AUTOCLICK="2,2,1,1"` + `BOB_SDL_CLICK` on the session row → Multi-Player → Join →
  select → Continue. Reaches the Ready Room, `Open(JOIN)`, `CreatePlayer`.
* **Discovery and join work on loopback AND across the PO's two real PCs** (192.168.254.57 →
  192.168.254.14, `EnumSessions: found "BoB"`).
* The client **does not need to click Fly**: `CReadyRoom::OnTimer` polls `_DPlay.FlyNowFlag` and
  calls `UINetworkSelectFly()` itself (`READY.CPP:363`).

### ⛔ The blocker, with the whole chain measured

The host clicks Fly, `StartFlying -> Launch3d`, and then dies:

    *** FATAL: SRC/BFIELDS/Persons3.cpp3384

`PERSONS3.CPP:3384` is `_Error.EmitSysErr("No player A/C set up on entering 3d!")`, guarded by
`if (!Manual_Pilot.ControlledAC2)`. With `BOB_TRACE_PLAYERSQ=1`:

    [psq] ExpandPilotedFlights: pilotedaircraft=(nil) uniqueID.count=-1 SagBANDEND=4864 gate=0
    [psq] NO player (pt=-1 squad 65535 nat 0); reassigning to NONE AC pt=-1
    [psq] group ... lines: 0            <- THE AIRCRAFT-GROUP BUILD LOOP NEVER RAN

So, in order:

1. **No aircraft groups are built at all** for the session — zero `[psq] group` lines, where a
   single-player mission emits one per group.
2. `pilotedaircraft` therefore stays NULL, so the gate
   `if (pilotedaircraft && pilotedaircraft->uniqueID.count < SagBANDEND)` (`:3960`) is **0** and no
   piloted flight is expanded.
3. The S72/S74 recovery — walk `ACList` for any flyable aircraft and re-associate the player — finds
   **NONE**, because the world has no aircraft to offer.
4. `ControlledAC2` is NULL on entering 3D → FATAL.

**The recovery path is not at fault and must not be "fixed": it correctly reports that there is
nothing to recover to.** The defect is upstream, at step 1.

### ⭐ Why there are no aircraft: the player squadron is never chosen

`pilotedaircraft` is only assigned where a built group's squadron matches `Pack_PlayerSquad`
(`PERSONS3.CPP:858-875`). Nothing ran, and the trace shows `squad 65535` (0xffff = unset) and
`nat 0`. The session-setup screen carries **"Game Type"** and **"Select Side"**, and the automated
run clicked Continue straight past them — as, in effect, does the PO, because **"Select Side" is the
known-blank panel** already logged in this port (UI-2 addendum, 2026-09-04, 4 sprints, two
hypotheses eliminated) and its sibling defect in MiG Alley is **MP-2**, whose radios are erased by
`DestroyPanel`.

**So the blank Select-Side panel is not cosmetic — it is what stops multiplayer flying.** That
raises its priority from a rendering complaint to the blocker on the PO's stated goal, and it ties
the BoB and MA multiplayer items to one cause.

### Also fixed this session (separate, real, and mine to clean up)

**A closed client leaked its player on the host.** `bob_video.cpp`'s `SDL_QUIT` handler ended the
process with `_exit(0)` — no destructors, no DirectPlay teardown — so every client whose window was
closed stayed registered on the host forever. The host's Fly then calls `GetAllGoResponses()`, which
spins for `CommsTimeoutLength` (**20 s** on TCP/IP) waiting for answers from those ghosts **without
pumping messages** — which is precisely the "not responding / force quit" dialog the PO hit on their
host. The ghosts were mine: four automated test joins, each ended by closing the window.
`bob_comms_shutdown()` now destroys the player and closes the session before exit.

### Next

1. **Make Select Side work** (BoB UI-2 addendum / MA MP-2 — one cause, two ports). Until a side is
   chosen there is no player squadron and no aircraft.
2. Then re-run the two-instance loopback harness above; it is fully scripted and needs no human.
3. Consider bounding `GetAllGoResponses` with a message pump, so a slow or absent peer degrades to a
   refusal rather than a 20-second frozen window.

### MP-5 (cont.) — ⛔ **"Select Side" is NOT the bug. No BATTLEFIELD is loaded at all.**

The PO asked me to fix Select Side, on my own suggestion that it was the blocker. **Measured, and my
suggestion was wrong.**

**Select Side is deliberately empty in Death Match.** `LOCKER.CPP:266`:

```c
if (_DPlay.GameType != DPlay::DEATHMATCH)
{
    pradio = GETDLGITEM(IDC_RRADIO_SELECTSIDE);
    pradio->AddButton(RESSTRING(RED));
    pradio->AddButton(RESSTRING(UN));
}
```

The client branch adds the RED/UN buttons **only when the game type is not Death Match**, and the
session was Death Match. A radio with no buttons draws nothing — which is why `id=2129` never
appears in any host/draw log while its sibling `id=2128` (Game Type, the same `CRRadio` class in the
same `CLockerRoom` dialog) draws fine. **Two identical controls, one populated and one not, by
design.** Chasing it would have been a wasted sprint, and the port's own UI-2 addendum had already
warned that the multiplayer side-selection screen was unverified.

### The real cause, measured

New instrument `BOB_TRACE_BFIELD=1` counts battlefield parses and the `T_airgrp` records inside
them, so **"the battlefield had no aircraft" can be told from "no battlefield was parsed"** — two
bugs that look identical from the FATAL. On a host that navigates Create Game → Continue → Fly:

    [bfield] processbfieldtoplevel call ... : 0
    [bfield] T_airgrp # ...                 : 0
    *** FATAL: Persons3.cpp3384

**Zero battlefield parses.** Aircraft exist only as `T_airgrp` records inside a battlefield
(`PERSONS2.CPP:635` → `toplevel_airgrp` → `make_airgrp`), and the multiplayer path enters 3D without
ever loading one. So:

* it is not that the player was assigned the wrong aircraft,
* it is not that Select Side was blank,
* **the world is empty because no mission data was loaded.**

The single-player route reaches a battlefield through `NodeData::CheckTargetLoaded` →
`TargetToBf(target)` → `Persons4::LoadSubPiece` — i.e. a *target* selects the battlefield file. The
comms/Death-Match path chooses a **scenario** (`_DPlay.GameIndex`, `currquickmiss`, the "1) Implode"
combo) and nothing observed maps that to a battlefield load.

**Next, and it is now one specific question:** what is supposed to turn `_DPlay.GameIndex` into a
loaded battlefield for a comms game, and does that code run at all in this port? `BOB_TRACE_BFIELD=1`
answers it the moment the call is found — a single-player Quick Mission run with the same trace gives
the working control to compare against.

### MP-5 (cont. 2) — the missing piece is **battlefield FileNum 35337**, and my "no battlefield" claim was VOID

⛔ **Retracting the previous entry's headline.** It said "zero battlefield parses" for multiplayer.
**That measurement was worthless: the instrument was in `SRC/BFIELDS/PERSONS2.CPP`, which is NOT IN
THE BUILD.** The compiled file is `SRC/BFIELDS/Persons2.cpp` — a *separate regular file* (55,660 vs
56,323 bytes, different md5), pulled in through `_BFIE.CPP`'s unity include. The dead copy's trace
never executed, so it printed 0 for multiplayer **and** for a known-good single-player flight. A zero
from code that does not run, read as a finding — `stale-duplicate-sources`, walked into after I had
correctly checked for exactly this on three other files the same day.

*(It also cost a truncated source: the re-patch hit a `UnicodeEncodeError` **after** `open(...,'w')`
had emptied the file. Restored from git, verified byte-identical by md5. The write now encodes first
and only opens the file once the bytes exist.)*

### With the trace in live code, the real comparison

| | single-player (flies) | multiplayer (FATAL) |
|---|---|---|
| battlefield parses | **28** | **15** |
| `T_airgrp` records | **1** | **0** |
| `[psq]` aircraft groups | 1 | 0 |
| piloted aircraft | 1 | 0 |

**Multiplayer does load battlefields — it loads a SUBSET.** 27 distinct FileNums in single-player, 15
in multiplayer. Files loaded ONLY by single-player:

    30721 30722 30769 30771 30772 30773 30847 30848 30861 30862 30863 30864 31498 35337

⭐ **And the aircraft live in exactly one of them: FileNum 35337.** Correlating each `T_airgrp` with
the `LoadSubPiece` that preceded it:

    [bfield] LoadSubPiece file=35337 slot=26
    [bfield] processbfieldtoplevel call #28
    [bfield] T_airgrp #1

Single-player loads 35337 **last**, and that parse is the one that yields the only air group in the
mission. Multiplayer never loads it. Note both arms load 35333 (single-player twice, multiplayer
once), so the 353xx band is the mission data and multiplayer is getting only part of it.

### So the chain, end to end and all measured

1. Multiplayer never loads battlefield **35337**.
2. No `T_airgrp` record is parsed → `make_airgrp` never runs → **0 aircraft groups**.
3. `pilotedaircraft` stays NULL → the expansion gate at `PERSONS3.CPP:3960` is 0.
4. The S72/S74 recovery walks `ACList` and finds no flyable aircraft — correctly, there are none.
5. `ControlledAC2` is NULL entering 3D → **FATAL "No player A/C set up on entering 3d!"**

### Next, and it is one question

Single-player reaches 35337 through `NodeData::CheckTargetLoaded` → `TargetToBf(target)`. **Trace
that call in both arms**: if multiplayer never calls it, the comms path never establishes a target
for its scenario (`_DPlay.GameIndex`, the "1) Implode" combo), and that is the fix site. The
instrument to add is one line, and the control run already exists.
