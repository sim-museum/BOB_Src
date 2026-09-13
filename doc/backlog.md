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

### MP-5 (cont. 3) — host FLIES; client joins and is registered, but does not follow into 3D

Two-instance loopback run with the `quickdef` fix in place:

| | host | client |
|---|---|---|
| joined / registered | hosting, 2 players | `Open(JOIN)`, **`host assigned us pid 4`** |
| `quickdef` seeded | ✅ `-> currmissnum=35337` | — (never reaches the seed) |
| aircraft built | ✅ | — |
| **`InThe3D=1`** | ✅ **yes** | ❌ no |
| FATAL | 0 | 0 |

**The peer handshake works.** Host logs `client joined from 127.0.0.1:45850 -> assigned pid 4`, the
client logs `host assigned us pid 4`, and traffic crosses both ways (`Send pid 3 -> 4 (ok)`,
`received 524 data bytes from pid 3`). The 8 earlier `no peer yet -- nothing transmitted` sends are
all at lines 75-76, **before** the join at line 180 — the host was alone in its Ready Room. Not a
defect, and worth pinning so it is not chased.

**What is missing:** the client received only **6** packets and never set `FlyNowFlag`, so
`CReadyRoom::OnTimer` never called `UINetworkSelectFly()`.

⚠️ **Prime suspect, and it is the defect the PO already asked to have fixed:** the client's log is
dominated by `EnumSessions: probing 127.0.0.1:47624` right to the end — **the Select-Session screen's
2365 timer is still polling from a screen closed minutes earlier**, because `WM_DESTROY` is never
dispatched and `CSelectSession::OnDestroy` (which calls `KillTimer`) never runs. That is continuous
UDP chatter at the host during the exact window in which the Fly handshake has to complete.

**Next: fix the `WM_DESTROY` dispatch.** It was deferred twice as "too big to bundle"; it is now on
the critical path for the PO's stated goal, which changes the trade.

**Harness note for whoever runs this next:** the client's session-row `BOB_SDL_CLICK` ticks must fall
**before** its `BOB_AUTOCLICK` Select step (row at 280-320, Select at 360). Moving them to 600 made
the client select nothing and stall on the session list -- it looks exactly like a discovery failure
and is not one. Also: `[menu] screen=0x...` pointers differ between processes, so match screens by
`artnum` (Ready Room = 27918), never by address.

### MP-5 (cont. 4) — EnumSessions was EATING the game's packets (fixed); WM_DESTROY dispatch REVERTED

**Two findings, one fix, one regression of my own caught by its own A/B.**

## ⭐ Fixed: `EnumSessions` destroyed every non-OFFER packet on the shared socket

`BobDPlay4::EnumSessions` sends a probe and then runs its own `recvfrom` loop on the **same `fd`**
the game uses, for up to `waitms`. Anything that was not `MSG_OFFER` was read off the socket and
**silently dropped**. And the Select-Session screen's 2365 timer keeps calling `EnumSessions` long
after that screen closes, so a client sitting in the Ready Room ran that loop **245 times** —
swallowing the host's traffic during exactly the window the Fly handshake needs. Measured before the
fix: the client received **6** packets in a whole session and never set `FlyNowFlag`.

Non-OFFER packets now go into the same queue `pump()` fills, and the effect is visible:

    [dplay] EnumSessions: rescued 190 data bytes from pid 3 (would have been dropped)
    [dplay] EnumSessions: rescued  31 data bytes from pid 3 (would have been dropped)

Those two are exactly the host's `Send 190 bytes pid 3 -> 2` and `Send 31 bytes pid 3 -> 4`, which
before this change were destroyed by the enumeration loop.

## ⛔ Reverted: dispatching WM_DESTROY BREAKS HOSTING

`ON_WM_DESTROY()` expands to nothing here, so all 25 `OnDestroy` overrides are dead code — the same
state `OnTimer` was in before R24. I made `CWnd::OnDestroy` virtual and called it from
`DestroyWindow()`. **It breaks the host outright**, and the A/B says so unambiguously:

| | `Open(CREATE)` | host bound to UDP |
|---|---|---|
| `BOB_NO_WM_DESTROY=1` | **1** | 1 |
| dispatch enabled | **0** | 0 |

with the game's own *"Could not create session or player"* modal. So at least one of those 25
handlers tears down state the comms setup needs, or runs where Windows would not have delivered
WM_DESTROY. **Now opt-in (`BOB_WM_DESTROY=1`)**, because waking 25 never-executed handlers is not a
change to make on the critical path of the PO's goal. It also did NOT fix the stale timer: the
Select-Session dialog is never destroyed at all, so `DestroyWindow` — and therefore any dispatch
hung off it — is never reached for that screen. Killing that timer needs the front-end's
screen-switch path, not `WM_DESTROY`.

## State

| | host | client |
|---|---|---|
| hosts / joins | ✅ `Open(CREATE)` | ✅ `assigned us pid` |
| Ready Room | ✅ | ✅ (artnum 27918) |
| **enters 3D** | ✅ **`InThe3D=1`** | ❌ still no |
| FATAL | 0 | 0 |

**Next:** the client reaches the Ready Room and receives host traffic, but never sets `FlyNowFlag`.
With the packets no longer being eaten, the question is now narrow: does the host's `UISendFlyNow()`
actually transmit to the client's pid, and does `CReadyRoom::OnTimer` see it? Trace both ends of that
one message.

**Harness fixes worth keeping:** wait ~12 s after launching before polling for a process (a
`! pgrep` guard fires instantly otherwise and reports a failure that has not happened); and never
print a success line outside the loop's success branch — an `until` that exits on timeout printed
"host hosting" for a host that never hosted.

### MP-5 (cont. 5) — joiners now reach the host's broadcast group; the host's GAME layer still does not know them

## ⭐ Fixed: a joining client was never added to the group the host broadcasts to

`UISendFlyNow()` sends `PID_FLYNOW` via **`SendMessageToGroup`**. Measured on a working join:

    [dplay] CreateGroup "(unnamed)" -> gid 2
    [dplay] AddPlayerToGroup player 3 -> group 2      <- the HOST's own player, and nothing else

The game only ever calls `AddPlayerToGroup` for its own player, so the group had one member and the
Fly-Now broadcast reached nobody. Real DirectPlay tells the host about a remote player with a
`DPSYS_CREATEPLAYERORGROUP` system message and the game adds it; this shim never delivered one.

The shim now records pids it hands to joiners and puts them in every group — both directions of the
ordering (join-then-CreateGroup and CreateGroup-then-join). Confirmed: `auto-added joining pid 4 to
group 2 (2 members)`.

## What is now established, and what is left

**Traffic flows both ways.** Host: 6 packets `received ... from pid 4`. Client: 6 sends `pid 4 -> ...
(ok)`. The transport, the join, the pid assignment and the group are all working.

⛔ **But the host's GAME layer never registers the client as a player.** Its only `CreatePlayer`
calls are its own (pid 1, pid 3). So `CountPlayers()` sees one player, `H2H_Player[]` has no entry
for the joiner, and `UINetworkSelectFly()`'s host branch has nobody to send Fly-Now to or collect a
go-response from — it flies alone, which is exactly what the logs show (host `InThe3D=1`, client
still in the Ready Room).

⚠️ **And the obvious route is a dead end, checked rather than assumed:** the game's
`DPSYS_CREATEPLAYERORGROUP` case in `ProcessSystemMessage` (`COMMS.CPP:1064`) casts the message and
`break`s — **it does nothing**. So synthesising that system message in the shim would not create the
player either. Worth recording so the next attempt does not spend a sprint on it.

**Next: find what actually creates an `H2H_Player` entry for a REMOTE player on the host** — the
client is already sending packets the host receives (44, 190, 12 bytes), so the question is which of
those the host is meant to act on, and why `ProcessPlayerMessage` is not turning one into a player.

## Running state

| | host | client |
|---|---|---|
| hosts / joins / pid | ✅ | ✅ |
| in the host's broadcast group | ✅ | ✅ (new) |
| packets crossing | ✅ 6 | ✅ 6 |
| known to the host's game layer | ✅ | ❌ |
| **enters 3D** | ✅ | ❌ |

### MP-5 (cont. 6) — the host RECEIVES the joiner's packets but never DISPATCHES them

The client's join is sent by `DPlay::AttemptToJoin()`:

```c
pack.PacketID = PID_PASSWORD;
ULong from = myDPlayID, to = DPID_ALLPLAYERS;
res = lpDP4->Send(from, to, 0, &pack, sizeof(PASSWORDPACK));
```

and the host's handler `Process_PM_Password` → `CheckPassword` → `GetNextAvailableSlot` is what
allocates the `H2H_Player` slot and replies with `newslot`/`groupID`/`aggID`. **That is the step that
would make the host's game layer aware of the client**, and it is the missing link.

**Traced both, and the answer is unambiguous:** with `BOB_TRACE_DPLAY=1` the host prints
`[mp] Process_PM_Password ...` and `[mp] CheckPassword ...` if either runs. Across the runs:

* host **receives** the client's packets — `received 44 data bytes from pid 4` (and 47, 12, 190 in
  other runs);
* host prints **neither** trace. `Process_PM_Password` is never entered.

So the packets arrive at the shim and are queued, and the game layer never turns them into a join.
The gap is between the shim's queue and `ProcessPlayerMessage`'s `case PID_PASSWORD` (`COMMS.CPP:1297`).

**Next, and it is one question:** does the host's Ready Room ever drain the receive queue? The client
side clearly does (it processes the host's replies), so the asymmetry is on the host. Candidates, in
order: whether `ReceiveNextMessage*` is called on the host's ready-room tick at all; whether the
packet's `to` field (`DPID_ALLPLAYERS`) is routed to the host's own pid by the shim's `Receive`; and
whether the size test that precedes the dispatch rejects it.

Instruments left in place, all `BOB_TRACE_DPLAY`-gated: `Process_PM_Password` entry (with `from` and
`Host`), and `CheckPassword` (name, player, allocated slot, pwordOK).

**Running state unchanged otherwise:** host hosts, seeds `quickdef`, builds aircraft, reaches
`InThe3D=1`; client joins, gets a pid, is now in the host's broadcast group, exchanges packets,
reaches the Ready Room — and is still invisible to the host's game layer.

### MP-5 (cont. 7) — ⭐ the client NEVER SENDS ITS JOIN PACKET, and the branch that would send it is never reached

Traced every packet the host receives, by id:

    [mp] ProcessPlayerMessage: PacketID=16 size=47 from=4 (PID_PASSWORD=9)
    [mp] ProcessPlayerMessage: PacketID=21 size=12 from=4 (PID_PASSWORD=9)

**`PID_PASSWORD` is 9, and the host never sees it.** That packet is what
`DPlay::AttemptToJoin()` sends, and it is the ONLY thing that makes the host run
`CheckPassword` → `GetNextAvailableSlot` → allocate an `H2H_Player` slot for the joiner. Without it
the host's game layer never knows the client exists, `CountPlayers()` returns 1, and
`UINetworkSelectFly()`'s host branch has nobody to send Fly-Now to — it flies alone, exactly as
observed.

**Also ruled out on the way, each by measurement rather than reasoning:**

* the `aggID` filter in `UIUpdateMainSheet` — `packet from=4 aggID=1 -> dispatch`, nothing dropped;
* the shim's `Receive` — it does not filter on `to`, so a `DPID_ALLPLAYERS` packet is delivered;
* the host's ready-room pump — `UIUpdateMainSheet` does drain and dispatch, and its traces prove it.

### Where the join is supposed to happen

`RFullPanelDial::CreatePlayer` (`FULLPANE.CPP:1440`) branches:

```c
if (_DPlay.UIPlayerType == DPlay::PLAYER_HOST)  { /* host: no join */ }
else                                            { res = _DPlay.AttemptToJoin(); ... }
```

Traced both sides. **The host prints `UIPlayerType=1 (HOST) -> HOST path`. The client prints
NOTHING — it never reaches `RFullPanelDial::CreatePlayer` at all.** So this is not a
mis-set `UIPlayerType` (`PLAYER_HOST=1`, `PLAYER_GUEST=2`, field initialises to 0); the client's
navigation simply never runs the function that would join.

**Next, and it is the last link:** find what invokes `RFullPanelDial::CreatePlayer` on the
host's path and why the client's Continue does not reach it. The client does reach the locker room
and does get a shim-level pid (`host assigned us pid 4`), so the gap is between the locker room's
Continue and this function.

Instruments kept, all `BOB_TRACE_DPLAY`-gated: packet ids at `ProcessPlayerMessage`, the
`aggID` decision in `UIUpdateMainSheet`, `Process_PM_Password`/`CheckPassword` entry, and the
host/join branch in `RFullPanelDial::CreatePlayer`.

### MP-5 (cont. 8) — the client sits on the LOCKER ROOM; my harness cannot click its Continue

Corrected picture of where the client actually stops. Its last screen is the **locker room**:

    [menu] screen=... artnum=27920 items=3 HORIZ
    [menu]   0 "Back"      centre=(63,747)
    [menu]   1 "Continue"  centre=(156,747)      <- this is what calls CreatePlayer
    [menu]   2 "Load Game" centre=(227,747)

and `FPLAYOUT.CPP:574` maps that item to `&RFullPanelDial::CreatePlayer`, whose else-branch is the
`AttemptToJoin()` that sends `PID_PASSWORD`. **So the join is one click away and the click is not
landing.**

**Two drivers tried, both fail on this screen specifically:**

* `BOB_AUTOCLICK="2,2,1,1"` — the 4th step never fires. Only three menu clicks appear (MP, Join,
  Select) plus the three session-row SDL clicks.
* `BOB_SDL_CLICK` at ticks 620/760/900 on (156,747) — **no `[sdlclick]` line at all** for those
  ticks, though the earlier row clicks at 280-320 fired normally. The tick counter increments once
  per pump call, so a later tick cannot simply be skipped: the pump appears to stop advancing after
  Select, which is when `JoinCommsGame` → `JoinComms()` runs and blocks on its own comms timeout.

**This is a HARNESS limit, not a proven game defect, and the distinction matters:** everything I have
said about "the client never sends PID_PASSWORD" is true of runs in which *Continue was never
clicked*. It does not establish that the join fails when a human clicks it.

⭐ **Cheapest way to settle it: the PO clicks Continue on a real client.** One click answers whether
`AttemptToJoin` fires, whether the host allocates a slot, and whether Fly then works — and the
instruments are already in and default-off:

    BOB_TRACE_DPLAY=1   ->  [mp] CreatePlayer: UIPlayerType=... -> join path
                            [mp] Process_PM_Password from=... Host=1     (on the HOST)
                            [mp] CheckPassword name=... -> slot=N        (on the HOST)

If those three appear, the join works and the remaining question is only Fly. If `CreatePlayer`
prints the join path but the host never prints `Process_PM_Password`, the defect is real and is in
`AttemptToJoin`'s send.

### MP-5 (cont. 9) — ⭐⭐ THE JOIN WORKS END TO END. Both sides now enter 3D; the client stalls on a sync packet

## The harness limit is gone — `BOB_SDL_CLICK_MS`

`BOB_SDL_CLICK` fires on a PUMP-CALL counter, and that counter stops advancing when the game blocks
inside its own comms timeouts — so after the client clicked Select, `JoinComms()` blocked and every
later tick-scheduled click was never reached. Measured: row clicks at ticks 280-320 fired, ticks 620+
produced no injection line at all. **`BOB_SDL_CLICK_MS="ms,x,y[;...]"` schedules on elapsed
milliseconds instead**, so a stalled pump DELAYS a click rather than losing it. It fires on the first
pump call after its deadline:

    [sdlclickms] pushed SDL_MOUSEBUTTONDOWN (156,747) at 20233ms (due 20000ms) rc=1

## ⭐⭐ With Continue actually clicked, the whole join works

    client:  [mp] CreatePlayer: UIPlayerType=2 (HOST=1 GUEST=2) -> join path
    host:    [mp] ProcessPlayerMessage: PacketID=9 size=44 from=4      <- PID_PASSWORD
    host:    [mp] Process_PM_Password from=4 Host=1
    host:    [mp] CheckPassword name="Bob" player=4 -> slot=1 pwordOK=1

**The host allocates slot 1 to the client, by name.** ⛔ **This RETRACTS the earlier conclusion that
"the client never sends PID_PASSWORD"** — that was true only of runs in which Continue was never
clicked, exactly as flagged when it was recorded. The join was never broken; my driver could not
reach the button.

## Where it stands now

| | host | client |
|---|---|---|
| hosts / joins | ✅ | ✅ |
| **host allocates a player slot for the client** | ✅ **slot 1, "Bob"** | — |
| Ready Room | ✅ | ✅ (artnum 27918) |
| **enters the 3D transition** | ✅ `InThe3D=1` | ✅ reaches artnum 0 |
| completes 3D entry | ✅ | ⛔ **`FATAL: Timed out (SIP)`** |

**The client's failure is now a SYNC, not a missing join.** `WINMOVE.CPP:1499` — inside
`SendPacketToAggregator`, the client sits in *"Receive Random List"* waiting for the host and quits
after `CommsTimeoutLength` (20 s).

**Also found, and it is the likely next fix:** the client's `quickdef` seed **never runs** (`seeded
from scenario` = 0 on the client). The seed lives in `CommsSelectFly`, the menu action behind the Fly
item — but a client never clicks Fly: `CReadyRoom::OnTimer` sees `FlyNowFlag` and calls
`UINetworkSelectFly()` then `OnSelectRlistbox(1,1)` itself. So the client enters 3D with an empty
quickdef, exactly the state that made the HOST fatal before the seed was added.

**Next, in order:** (1) seed `quickdef` on the client's auto-launch path too, not just the menu
action; (2) then trace the random-list send on the host against the receive on the client.

### MP-5 (cont. 10) — the quickdef seed now covers BOTH roles; the SIP timeout is NOT caused by it

Moved the seed out of `RFullPanelDial::CommsSelectFly` (the menu action behind Fly, which only the
HOST runs) into `DPlay::UINetworkSelectFly` — the one function both roles must call before flying.
The body lives in `FULLPANE.CPP` behind `bob_mp_seed_quickdef()` because `CSQuick1` is an MFC-layer
type the comms TU does not include; calling through a hook leaves the include graph as it was.

**It works on the client now:**

    [mp] seed_quickdef: scenario 0 -> currmissnum=35337 altmissnum=35340 target=13178 (Host=0)

⛔ **And the client still dies with `FATAL: Timed out (SIP)`.** So the empty quickdef was NOT the
cause of that timeout — a hypothesis worth killing, since it was the obvious one. The seed is still
correct and stays: a client entering 3-D with no mission data would have failed later anyway.

**⚠️ A near-miss worth recording.** The move was done as two edits; the first (removing the seed from
`CommsSelectFly`) succeeded and the second failed its anchor assertion, leaving the tree with the
HOST fix deleted and nothing in its place. Caught immediately by grepping both files for the marker
string, but a build in that window would have looked like a fresh regression of a fix that was
working an hour earlier. **When moving code between files, verify BOTH ends before building** — the
assertion protects the destination, not the source you already cut.

**MP-5 sprint count: ~10, past the PO's 6-sprint cap. Rotating off.**

State on handover: host hosts, seeds quickdef, builds aircraft, allocates a player slot for the
client (`CheckPassword name="Bob" player=4 -> slot=1`), and reaches `InThe3D=1`. Client joins,
reaches the Ready Room, seeds quickdef, enters the 3-D transition, and times out in
`SendPacketToAggregator`'s *"Receive Random List"* wait (`WINMOVE.CPP:1499`, 20 s).
**Next: trace the random-list SEND on the host against that RECEIVE.**


### MP-5 (cont. 11, Opus 5, 2026-09-12) — `DPRECEIVE_TOPLAYER` was ignored by the shim; and the harness lives in the tree now

**Two things, one of them a real port defect.**

**1. The receive filter the game depends on was not implemented.** `DPlay::ReceiveNextMessageToMe`
(`COMMS.CPP:2155`) sets `To = myDPlayID` and calls `lpDP4->Receive(&from, &To, flags, ...)` with
`DPRECEIVE_TOPLAYER`, whose whole point is "give me only messages addressed to THIS player" — the
function's own comment says so: *"receive message to mydplayid in case I am aggregator. Dont want to
receive packets sent to aggregator here!!!!"*. The shim's `Receive` took `DWORD` unnamed, ignored it,
treated `lpidTo` as output only, and returned whatever sat at the queue head. Every caller therefore
drained every other caller's traffic: whichever loop polled first consumed the packet. A wait loop
looking for one specific message can lose it to an unrelated pump — and the client's 20 s *"Receive
Random List"* wait (`WINMOVE.CPP:1499`) is exactly such a loop.

Implemented where DirectPlay would: deliver the first queued message addressed **to that player**, to
a **group the player belongs to** (real DirectPlay expands a group send to its members, and the shim
already tracks membership for the FlyNow broadcast), or to **0** (the game's own request address).
Anything else stays queued for the caller it belongs to. `DPRECEIVE_ALL`/flags 0 keep the old
take-the-head behaviour; `BOB_NO_RECV_FILTER=1` is the negative control.

**2. `tools/bob_mp_two_instance.sh`** — the two-instance recipe existed only in shell history and had
to be reconstructed from this file every time. It now runs host + client, asserts both reach 3D and
that the client clears the random-list wait, and takes the timings as env.

**What the runs then showed, in order (this is the harness converging, not the game changing):**

| run | client got to | why it stopped |
|---|---|---|
| 1 | Join screen, host's session listed | row click at 20 s, before the list |
| 2 | session selected, packet sent to host | Continue never clicked |
| 3 | **Ready Room (artnum 27918)** | — |
| 4 | (running) | host's Fly was rescheduled |

⭐ **Run 3's finding is about the ORDER, not the join:** the host's four autoclick steps fire as each
screen appears, so it flew within seconds — while the client was still working through
Join/Select/Continue. The FlyNow broadcast the client's Ready Room waits for had already gone.
`HOST_FLY_MS` now schedules the host's Fly (menu item 1 at (121,747) on artnum 27918) in wall-clock
milliseconds, after the client is in its own Ready Room. **Whether the SIP timeout survives that
ordering is the open question** — runs 1-3 never reached the wait at all, so they cannot answer it,
and neither does the receive filter on its own.

### MP-5 cont. 15 (2026-09-12, run 10) — ROOT CAUSE: the host's `Implemented` is wiped between the Fly decision and the 3-D launch

Run 10 carried the three-gate trace at the head of `SendInitPacket()`. Both instances answered, and
they answered differently:

    host.log   [mp] SendInitPacket: Implemented=0 Joining=0 Host=1
    client.log [mp] SendInitPacket: Implemented=1 Joining=0 Host=0

That is the entire bug. `SendInitPacket()` wraps its whole body in `if (_DPlay.Implemented)`, so the
HOST — the only instance that can transmit the random list — falls straight through and sends
nothing, while the CLIENT (Implemented=1, Host=0, Joining=0) enters the "Receive Random List" wait
and dies 20 s later on `SayAndQuit("Timed out (SIP)")`. Neither instance is misbehaving in the
`Host`/`Joining` gates; only `Implemented` is wrong, and only on the host.

**Where it is lost.** `Implemented` is set TRUE in exactly one place, `DPlay::InitialFlagReset()`
(`COMMS.CPP:3368`), whose own comment says it exists to "reset all flags etc before launching 3d".
It is cleared in `DPlay::UIUpdateMainSheet()` (`COMMS.CPP:513`), unconditionally, on the third line
of the function: `// make sure status is not 3D!  Implemented=FALSE;`. The host's log puts one of
those calls between the two:

    [frontend] click (121,747) -> menu item 1
    [mp] seed_quickdef: ... (Host=1)          <- inside UINetworkSelectFly -> InitialFlagReset, Implemented=TRUE
    [dplay] Send 190/12/588/143 bytes pid 3 -> 2
    [frontend] painted screen artnum=0 + dials + menu + presented      <- one more front-end tick
    [mp] UIUpdateMainSheet: packet from=4 len=588 -> dispatch          <- Implemented=FALSE
    [frontend] (bridge) StartFlying -> Launch3d(wasrunning=0)
    [mp] SendInitPacket: Implemented=0 Joining=0 Host=1                <- sends nothing

The client's tail has no such paint tick between its last `UIUpdateMainSheet` (log line 982) and
`SendInitPacket` (993), which is why its flag survives.

**Why the author's own guard does not save the host.** `UIUpdateMainSheet` reads

    if (LeaveCommsFlag) return;
    Implemented=FALSE;              <- cleared FIRST
    if (FlyNowFlag) return;         <- the "we are ready to go, wait till selectfly" guard

so the clear happens before the guard in any case; but on the HOST the guard is moot, because the
host never sets `FlyNowFlag` at all. `FlyNowFlag=TRUE` is set only in `Process_PM_FlyNow()`
(`COMMS.CPP:3585`) — the CLIENT's reaction to receiving `PID_FLYNOW`. The host initiates the flight
from the menu (`FULLPANE.CPP:1664`, `if (!_DPlay.UINetworkSelectFly())`) with the flag false
throughout, which is confirmed in the log: the host's final `UIUpdateMainSheet` printed a `dispatch`
line, and the receive loop that prints it sits *below* the `if (FlyNowFlag) return;`.

**Ready for the next BoB rotation (MP-5 is at the PO's 4-sprint cap, so it is recorded, not
applied).** Two candidate fixes, in preference order:

1. Make the clear conditional on not having committed to the launch. `InitialFlagReset()` also sets
   `GameRunning=TRUE`; gating the clear (`if (!GameRunning) Implemented=FALSE;`) keeps the "status is
   not 3D" intent for every ordinary UI tick and stops the one tick that matters. Check first what
   else writes `GameRunning`, and whether it is false on a normal front-end tick.
2. Set `Implemented=TRUE` in the bridge at `Launch3d` entry, immediately before `SendInitPacket()`
   runs. Smaller blast radius, but it papers over the clear rather than naming it.

Either way the test is the same and is already automated: `tools/bob_mp_two_instance.sh` must show
`Send 114 bytes` in the host log (currently zero occurrences in a full 420 s run) and no
`FATAL: Timed out (SIP)` in the client's.

**One more defect noticed in passing, not yet acted on.** In the `Joining` branch of
`SendInitPacket()` the received list is walked with `UWord* ptr; ptr=(UWord*)Buffer; ptr+=sizeof(ULong);`
— a `UWord*` advanced by 4 elements, i.e. 8 bytes, to "skip PID" (4 bytes). The host's send has no
PID word in front of `RndPacket` at all (`SendMessageToGroup((char*)&RndPacket, 57*sizeof(UWord))`),
so once the host does transmit, expect this branch to read the list misaligned. The non-joining
"Receive Random List" path — the one our client actually takes — should be checked for the same.


### R3 follow-up (S440, 2026-09-12) — the cap that truncated the ACMI silently

R3 closed at S439 carrying one named limit, and this closes it. The all-aircraft walk in
`REPLAY.CPP` ran `while (_ac && _id < 256)` while S438 had measured the live `ACList` at **exactly
148**. A raid 73 % larger would therefore have dropped aircraft from the exported track **with
nothing said anywhere** — and that is the real defect, not the number: a truncated ACMI opens in
Tacview and looks complete, so the missing aeroplanes read as an export bug (or as the AI not having
flown) rather than as truncation.

- **Cap raised to 1024**, overridable with `BOB_ACMI_MAXOBJ` (floored at 16). 1024 is not arbitrary:
  it is 6.9x the measured list AND the ceiling that keeps the synthetic-id band `0x4000 + _id`
  inside the `0x4400` bound the S437 identity table indexes with.
- **Truncation now reports**, once per run, on stderr, and deliberately **not** under
  `BOB_TRACE_ACMI` — a report that only appears when you already suspected the problem does not fix
  a silent failure. It prints what was written, the cap in force, the override's name, and how many
  aircraft were still on the list behind the cursor.
- The cap itself is kept. It is what stops a corrupt or cyclic `ACList` spinning in this loop
  forever, which is a real hazard on a head-inserted list.

Built clean; `bob` carries the string. **Not yet observed firing** — doing so needs a sortie with
more than 1024 aircraft, which no mission here generates, so this is a hardening plus an instrument,
honestly labelled: the 256-object truncation could have happened silently and now cannot.

⚠️ **Process note, recorded because it nearly cost real work.** While making this edit I truncated
`SRC/COMMS/REPLAY.CPP` to 0 bytes: the script did `open(path,'w',encoding='latin-1').write(text)`
where the text contained a non-latin-1 character, and `open(...,'w')` truncates the file *before*
`write()` raises. The build then failed in `Winmove.cpp` with `VELSHIFT was not declared` — a
misleading symptom in a different file, because `REPLAY.CPP:110` is where `#define VELSHIFT 6`
lives. Recovered fully with `git checkout -- SRC/COMMS/REPLAY.CPP`: HEAD carried every R3 sprint
(S274b/S437/S438) and the repo's modified-file count went 16 -> 15, which is the proof that the file
had no uncommitted work in it and nothing was lost. **Rule: encode the whole string (or otherwise
prove it writes) BEFORE opening the target for writing, and check the resulting file length.**


### MP-5 cont. 16 (2026-09-12) — ✅ **FIXED AND PROVEN**: the client reaches 3-D, no SIP timeout

cont.15 named the cause; this applies the one-line gate and measures the result on the same harness
that has failed for five sprints (`tools/bob_mp_two_instance.sh`, `SECS=420`, logs in
`~/Documents/260912/logs/bob_mp2k/`).

**The change** (`COMMS.CPP`, `UIUpdateMainSheet`): `Implemented=FALSE;` — unconditional, third line
of the function — is now

    if (oldclear || !GameRunning) Implemented=FALSE;

**`GameRunning` is the right gate, and that was checked rather than assumed.** `UINetworkSelectFly`
sets every active player's status to `CPS_3D` (`COMMS.CPP:815`) *before* it calls
`InitialFlagReset()` (`COMMS.CPP:891`), and the `if (stop) GameRunning=FALSE;` test twenty lines
below the clear only fires when **no** player is `CPS_3D`. So `GameRunning` is TRUE across exactly
the pre-launch tick that was destroying the flag, and FALSE on an ordinary front-end tick — where
`Implemented` is still cleared, so "make sure status is not 3D" keeps its meaning everywhere it ever
mattered. `BOB_MP5_OLDCLEAR=1` restores the old behaviour as a control arm.

**Measured, before → after:**

| | before (run 10) | after (run 11) |
|---|---|---|
| host `SendInitPacket` gates | `Implemented=0 Joining=0 Host=1` | **`Implemented=1 Joining=0 Host=1`** |
| host `Send 114 bytes` | **0 occurrences** in a full run | **`[dplay] Send 114 bytes pid 3 -> 2 (ok)`** |
| client receives the list | never | **`[dplay] received 114 data bytes from pid 3`** |
| client outcome | `FATAL: Timed out (SIP)` after 20 s | **no FATAL, no timeout** |
| instances reaching 3-D | host only | **host AND client** (`InThe3D=1` in both logs) |

The new guard also announces itself under `BOB_TRACE_DPLAY`
(`[mp] UIUpdateMainSheet: KEPT Implemented=1 (GameRunning)`), which is how the fix was confirmed to
be the thing that changed rather than run-to-run luck.

**Carried forward, not hidden.** The misaligned PID skip noted in cont.15 is still there: the
joining branch of `SendInitPacket` does `ptr += sizeof(ULong)` on a `UWord*` (8 bytes, to skip a
4-byte PID) while the host's send carries no PID word at all. Our client takes the *non*-joining
"Receive Random List" path, which is why it is now fine; a real `Joining` client would still read the
list misaligned. That is the next MP-5 item.
