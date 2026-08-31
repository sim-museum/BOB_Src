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
| ⚠️ GATES | **THE SUITE OVERWRITES THE SAVED REPLAY — CONFIRMED TWICE, back it up first** | **2026-08-30.** A full `tools/bob_gates.sh` run changed `SAVEGAME/dreplay.dat` (md5 `76bcb59b…` → `29722e93…`, same size, mtime moved). That is the SAVED replay, not the `VIDEOS/replay.dat` scratch buffer which every flight legitimately truncates — and it is irreplaceable player data. The R1 gate alone leaves it untouched (verified byte-identical); some later gate in the suite does not. <br>**Before any suite run:** `cp SAVEGAME/dreplay.dat` and `cp VIDEOS/replay.dat` somewhere safe. The pre-run copies from this session are in `/tmp/bob_replay_backup/`, and the original was restored after it was noticed. <br>**S362: reproduced a second time.** A full suite run again changed `SAVEGAME/dreplay.dat`
(`76bcb59b…` → `f441d510…`); restored from the pre-run backup and verified byte-identical. So this is
deterministic, not a one-off, and it will keep eating player data until fixed. <br>⭐ **S365 — GUARDED, and the culprits are named.** The suite now hashes every file in `SAVEGAME` before the run, attributes writes per gate, and restores at exit. Verified end to end: the player's `SAVEGAME` came out byte-identical to an INDEPENDENT backup the guard did not make. <br>**Culprits: GATE 5 (campaign) and GATE 6 (combat soak) — each writes `dreplay.dat` AND `Package.dat`.** I predicted GATE 8, the one recipe that deliberately records; that was wrong twice over — wrong gate, and wrong cardinality (it is not one gate). GATE 8 and GATE 7 are innocent. `Package.dat` is a second victim that a guard watching only the named file would have missed, which is why it hashes the whole directory. <br>Detection is per gate but rollback is ONCE at exit, deliberately: restoring between gates would change what later gates see (R11's ACMI export reads a recording an earlier gate writes), and a gate broken that way would read as a port regression. <br>The guard also removes files a run CREATES — otherwise the next run's "before" snapshot bakes the litter in as if it were player data — and names DELETED files, which the existing-files loop cannot see. <br>⭐ **VALIDATION STATE (be precise about what is and is not proven).** PROVEN: the full suite run at S367 reported `PLAYER DATA: untouched by this run` with gates 1–R11 all executing, and the player's SAVEGAME came out byte-identical to an independent backup; GATE R1 passes standalone against the scratch tree, writing 181218 bytes into the SCRATCH VIDEOS while the real `replay.dat` keeps its earlier timestamp; and an INTERRUPTED run (killed at GATE 3) left SAVEGAME **and** VIDEOS byte-identical — the case a restore-at-exit guard structurally cannot cover. NOT YET PROVEN: a single full suite that is green *and* guard-silent end to end with the S368 changes in it. Two attempts were stopped externally partway. The pieces are each verified; the combination is not, and that distinction is the point. <br>⚠️ **Still a bandage.** The clean fix is to point saves at a scratch directory, but the path comes from File_Man's `FIL_SAVEGAMEDIR` table, which lives in data (`MASTER.FIL`/`DIR.DIR`) rather than a source literal. That is the follow-up. <br>**Next step:** redirect `FIL_SAVEGAMEDIR` for gate runs; the old next step, find WHICH gate writes it — the campaign, soak and strategic gates all fly — and give it the same stash/restore the MA suite already does for the player's campaign save (`port/parity_2d.sh` PIN/`SAVEDIR` pattern). A gate that eats player data is a bug in the gate. |
| R16 | 🟢 **BOTH FIXED — and the full suite is green with GATE R16 in it (S6: `PLAYER DATA: untouched`, `RUNS: all clean`, R16 arms: control drifts to 1000 ms / fix peaks at 44 ms, resets 0/8700)**  — band (vsync, S355) and JUDDER (S5: the padlock extrapolation clock was a counter, and it drifted)** | ⭐ **S5 (2026-08-30): ROOT-CAUSED AND FIXED, MEASURED BOTH WAYS.** `ViewPoint::move_time_ms` is the timestamp `view_dt` is measured from, and `view_dt` is the extrapolation `ViewFudge` applies to the padlocked item (`World += vel*view_dt`) so it is drawn where it will be between 25 Hz world moves. `STUB3D.CPP:2304` advanced it by a **nominal 40 ms per move cycle — a counter standing in for a clock**. The port's pthread move timer runs ~2% slow (accumulated 5 ms usleep slices overshoot), so the counter drifted ~20 ms/s against wall time; `view_dt` climbed to the 1000 ms clamp in ~50 s and snapped to 0. **Measured on a padlocked sortie: max=1000 ms, mean 466, 68% of frames extrapolated >300 ms.** At 100 m/s closing the bogie is drawn up to 100 m ahead of truth then yanked back — worse with speed, worse up close: the PO's report to the letter. **Fix: stamp `timeGetTime()` at each move instead of counting.** With it: **all 8700 frames <50 ms, max 45, mean 20, zero resets.** Gate `tools/bob_r16_viewdt.sh` (in the suite as GATE R16) runs both arms on a flight that provably padlocks and asserts the control drifts, the fix does not, and the tap fired. <br>Two wrong turns kept: (1) I first read the field as "stamped once, never again" — a grep filter dropped the `+=` line for its trailing comment; the unfiltered grep and the per-frame series refuted it (drift at 1/50 wall rate, not a 1 Hz sawtooth). (2) A stamp placed BESIDE the counter fought it — 1600 negative-delta resets, extrapolation disabled 41% of frames — so it had to REPLACE it. <br>⚠️ **Opens R18:** the same 2%-slow timer advances `timeofday += FRAMETIME` per tick, so the game clock itself runs slow; the timer should use absolute deadlines (`clock_nanosleep TIMER_ABSTIME`), not accumulated slices. <br>**History (S355–S357):** **PO 2026-08-30**, verbatim: *"fix 3D screen flicker. Sometimes there is a flickering band at the top of the screen, sometimes when you get close to a padlocked bogie it feels like the frame rate is about 5 fps at a time of rapid motion. On an i7 PC with a 6GB nvidia card there should never be a low fps kind of effect."* <br>**TWO SYMPTOMS, and they may be two bugs — do not assume one cause.** (a) a flickering BAND at the TOP of the screen; (b) a judder that reads as ~5 fps specifically *near a padlocked bogie during rapid motion*. <br>**The PO's hardware point is the requirement**: an i7 with a 6 GB nvidia card should never produce a low-fps effect in this game, so if the frame rate really is dropping the cause is the port, not the machine. <br>⚠️ **FIRST ESTABLISH WHICH IT IS.** "Feels like 5 fps" has at least three causes that need different fixes and look identical: the frame rate genuinely collapsing; the frame rate holding while the CAMERA moves in large steps (a padlock view that updates at a lower rate than it draws); or frames being presented out of order/duplicated. **Measure the actual frame interval before touching anything** — BoB already has `[lifetime]`/frame counters and `BOB_SHOT2D_EVERY`, and a per-frame timestamp histogram would separate (1) from (2) immediately. <br>**The band at the top smells like a present/tear issue** (vsync, a partial blit, or the 2D overlay strip drawn at a different cadence from the 3D) — `R9`'s work on present-time offsets and `g_uiOff` is adjacent and worth reading first. <br>⭐ **S355 — `BOB_TRACE_FRAMETIME` built, and it answers the first question.** Wall-clock interval between consecutive presents, as a histogram plus the worst offenders. Over **24,600 frames** of automated flight:
`<16.7ms=13593  16-33=10984  33-50=14  50-100=0  100-200=3  >200ms=6`, worst `1234 / 986 / 699 / 426 / 416 ms`.
**99.9% of frames run at 30+ fps and only THREE land between 100-200 ms**, so a sustained ~5 fps would need thousands of frames in those buckets and there are three. **Hypothesis (1), a genuine frame-rate collapse, is REFUTED for ordinary flight** — the remaining candidates are the camera update rate and frame ordering, which is where the search goes next.
<br>⭐ **THE SWAP INTERVAL WAS NEVER SET.** `SDL_GL_SetSwapInterval` appeared nowhere in the port, so presents inherited the driver default and 55% of frames completed faster than a 60 Hz refresh — i.e. swapping mid-scanout, which tears, and **a tear near the top of the frame is a band near the top of the screen**. Now requested at context creation (vsync, falling back to late-swap then uncapped), reported in the `[vid]` line. `BOB_NO_VSYNC=1` for an A/B, kept because the judder symptom is a separate question and capping the rate must not be allowed to confuse it.
<br>✅ **S356 — "presents more than once per refresh" is REFUTED; it was my own bucket boundary.** Per-site counts show ONE path issues 17,395 of 17,401 swaps, and adding a central value settles it: **`mean=16.81 ms  median=16.65 ms => 59.5 fps`**, i.e. cleanly locked to the 60 Hz refresh. The earlier "55% under 16.7 ms" was an ordinary vsync-locked stream with jitter, split by a bucket edge placed exactly at the 60 Hz period — a histogram whose boundary sits on the mode will always look bimodal. The instrument now reports mean and median so it cannot be misread that way again.
<br>⭐ **S357 — the render/present ratio is FIXED, so this layer is cleared too.** Counting `BeginScene` and `Clear` against presents: **`5.02 scenes/present`, `6.02 clears/present`, and the ratio is identical in every 600-frame sample (3000 scenes / 600 presents, repeatedly).** A *constant* ratio is a fixed multi-pass pipeline; frames being discarded irregularly during rapid motion would make it fluctuate. **The 5.02 resembling the PO's "about 5 fps" is coincidence** — worth saying, because the number is exactly the kind that invites a false conclusion. <br>So the judder is not frame delivery (S356) and not the render-to-present relationship (S357). What remains is **above** this layer: how often the game advances its own world/camera state. <br>⚠️ **Separately worth knowing: 5 scene renders and 6 full clears per DISPLAYED frame is a lot of GPU work** for a fixed pipeline. Not a judder mechanism, but a standing performance observation.
<br>**So frame DELIVERY is clean: no collapse, no double-present, 59.5 fps.** Both remaining candidates for the PO's judder are therefore about what is IN the frames rather than how often they arrive — the camera/padlock update rate, and frame ordering.
<br>⚠️ ~~**NEW, AND NOT YET EXPLAINED: the port presents MORE THAN ONCE PER REFRESH.**~~ With vsync confirmed active (`SDL_GL_GetSwapInterval()` returns 1) the histogram *still* shows 55% of intervals under 16.7 ms. Under real vsync every interval should be ~16.7 ms, so extra swaps are being issued — there are **six** `SDL_GL_SwapWindow` call sites. Double-presenting wastes GPU work and is a plausible route to visible artefacts. **Next: count swaps per site** and find which paths both present in the same frame.
<br>⭐ **S358 (sprint 4) — the camera never passes through the port at all.** `DEV_SetTransform` is a no-op stub, and that is CORRECT: the game's Lib3D fills vertex buffers with `R3DTLVERTEX` (FVF `XYZRHW`) — **pre-transformed screen-space vertices** (`bob_video.cpp:1822`, `LIB3D.CPP:255`). The Rowan engine does its own camera transform in software and hands the port already-projected coordinates. So if the IMAGE steps while frames arrive smoothly, that stepping happens in the game's view update, above everything this port controls. <br>Last port-side lead also checked and clean: the timer the game reads. `GetTickCount`/`timeGetTime` is `CLOCK_MONOTONIC` at millisecond resolution (`compat_winbase.h:115`) — a coarse or non-monotonic clock would have produced exactly this symptom, and it is neither. <br>**FOUR SPRINTS, FOUR CANDIDATES ELIMINATED BY MEASUREMENT:** frame-rate collapse (99.9% at 30+ fps), double-presenting (one site, 59.5 fps locked), render-to-present ratio (fixed at 5.02, does not fluctuate), and the camera/timer path (the camera does not traverse the port; the clock is correct). <br>**The band is FIXED** (vsync, S355) and that half stands on its own evidence. **The judder is not disproven — it is localised**, and reproducing it needs the PO's exact state.
<br>⚠️ **The PO's exact condition is NOT yet reproduced**: their judder is *"close to a padlocked bogie … at a time of rapid motion"*, and this measurement is an automated sortie with no padlock. A clean histogram here does not clear that case. |
| R15 | ✅ **DONE — `doc/TACVIEW.md`** | **PO 2026-08-30**: *"add backlog items to ma and bob - include the instructions on how to install tacview in the ma and bob repo"*. <br>This port already EXPORTS `.acmi` for Tacview (R10 fixed the 20.48 s truncation; R11 gates the yaw convention), and the PO has been loading those files into Tacview by hand — but nothing in the repo says how to get Tacview onto this machine or where the exports land. <br>**Cross-port twin of MA's PO-87** — same instructions, two repos; write it once and note any real difference (export path, ACMI version, the copy-to-Tacview step) rather than duplicating blindly. <br>**Should cover:** obtaining Tacview on Linux (which build, and whether it runs natively or under Wine), installing it, where this port writes `.acmi`, how to load one, and the known caveats already established here — the yaw convention (S276/S277: Tacview orients from **Yaw alone**, opposite rotational sense from compass) and R10's block-wrap fix, so a reader is not re-deriving them. <br>**DELIVERED (2026-08-30): `doc/TACVIEW.md`.** Written from this machine's actual setup, and it found a REAL difference from MiG Alley rather than duplicating: this port keeps an ARCHIVE — `bob_acmi_save_as()` copies the live export to `VIDEOS/<stem>.acmi` with the same stem as the `.cam`, so a saved replay is kept, while MiG Alley has only the fixed-name `acmi_current.txt` and loses each sortie to the next. Also records that the existing `VIDEOS/*.acmi` **all predate S276** and therefore carry the old yaw convention, which is exactly why `GATE R11` reads only the live file. Carries the S276/S277 Yaw-alone caveat and R10's 20.48 s truncation. |
| PO-73 | ~~Grey elliptical blob floating in the cockpit view~~ | **CLOSED (S309) — NOT A DEFECT.** It is the game's own **threat indicator**: `COverlay::DoThreat()`, texture `MskMap16/THREAT01.X8`, gated on `Save_Data.gamedifficulty[GD_HUDINSTACTIVE]`. Position and size match the source constants to within a pixel. It reads as a blank grey ellipse because it has no contacts to plot — the player is alone in this scene. **For the PO:** it is switched off with the HUD-instruments difficulty setting. |
| — | ~~`upload_texture` may be failing silently~~ | **Closed (S306).** It no longer can: the early return counts and traces itself (`BOB_TRACE_TEXFAIL`), and a summary prints at every frame dump so zero is a measured number. |
| R14 | ✅ **DONE: keyboard-command reference → `~/Documents/260829/bob_keyboard_commands.md`** | **PO 2026-08-29.** A written list of every keyboard command the game responds to, as a Markdown file in `~/Documents/260829` (the directory does not exist yet — create it). Cross-port twin of MA's PO-86; same engine, same table-driven key handling, so do them together and note where they DIFFER rather than assuming they match. <br>**RAW MATERIAL:** `SRC/H/KEYMAPS.H` (a symlink to `KEYMAPS.H`) carries **393 `KeyName(index, NAME)` entries** — the authoritative action list. **Many are `//DEADCODE`-commented** (e.g. `KeyName(01..06, KeySrc_BN_*)`, AMM 09/12/99): those are NOT live bindings and must be excluded or marked, or the reference will list keys that do nothing. <br>⚠️ **BoB has NO `DUMP_BINDINGS` equivalent** — MA's `MA_DUMP_BINDINGS` (which dumps live `ACTION = scancode` pairs after the game's own table load) has no counterpart here (`grep` finds none). So either add one, or read the table statically — **and if read statically, say so**, because a static read cannot see a binding the running game overrides. <br>⚠️ **The PO's known keys must appear and be verified:** **`S`** (the centre-square padlock they requested in R5.x — it is a BDG feature the port does NOT yet implement, so it must be listed as NOT IMPLEMENTED rather than silently omitted or wrongly shown as working), and **ALT+X** (exit the 3D, the R8 path). <br>⚠️ **Mark PORT-ADDED keys.** `Ctrl+ESC` is a port hard-exit hatch added by the compat layer, not Rowan's. Mixing port additions with original bindings misleads on both. <br>**Done when** the `.md` lists every live action with its key(s), excludes or clearly marks DEADCODE entries, distinguishes flight bindings from front-end/UI ones, flags port-added keys, and states whether the list was read statically or dumped from a running game. | <br>⭐ **DELIVERED: `~/Documents/260829/bob_keyboard_commands.md`** — **549 live bindings**, machine-read. <br>**BUILT THE MISSING TOOL:** BoB had no `DUMP_BINDINGS`, so `BOB_DUMP_BINDINGS=<path>` is now cross-ported from MA — it reads `KeyMap3d::mappings[]` **after** `Reg3dConv` loads the game's own table, so these are the bindings IN FORCE, not KEYMAPS.H's intentions. It dumps the RAW action value (`KeyVal = index*2`) and leaves naming to an offline join, keeping the fallible half re-runnable without a rebuild. <br>⭐ **BoB's MODIFIERS ARE NOT MA's — and KEYMAPS.H contains BOTH lists.** The `//DEADCODE` set is MA's (AltGr / Right Ctrl / Right Shift); the LIVE set is BoB's own: 3=**Left Ctrl**, 4=**Left Shift**, 5=**Msg**, 6=**ShMsg**, 7=**Overlay**. **Assuming the ports match — same engine, same header name — would have mislabelled every modified binding in the document.** The item said to note where they differ rather than assume; this is where. <br>⭐ **`S` IS ALREADY BOUND — to `JOYSTICKMODETOGGLE`** (an action whose KEYMAPS.H entry is `//DEADCODE`). **This bears directly on R5.x:** the PO's centre-square padlock wants the `S` key, and `S` is not free — implementing it means deciding what happens to the existing binding, not just adding a handler. The document lists `S` as NOT the PO's requested behaviour and points at R5.x. <br>**Stated limits:** 30 live actions unbound (listed, not omitted); **91 bindings resolve to a `//DEADCODE` or unnamed action index** — kept with their raw values because they ARE live in the running table; 3D/flight bindings only. <br>⚠️ **THREE INSTRUMENT FAILURES BEFORE IT WORKED, all mine:** (1) hooked only ONE of two `commonkeymaps` sites — a boot run reached the 3D and wrote nothing; (2) used `fopen`, which this TU `#define`s to `fopen_nocase` and which cannot create files — **the exact trap S243 found in `BOB_SHOT3D` earlier the same day and wrote down, walked into again**; (3) latched "done" on ENTRY, so the empty dump taken before the table was populated permanently blocked the good one. Now latches only on success and retries after `Reg3dConv`. |
| R13 | ✅ **DONE (S340): the gate asserts `stored == (start + clicks) mod N` and passes** — diagnosed in S331-S4 (the combo STARTS at 2, so `WANT=2` assumed a start it never has) | Surfaced while proving R12's instrument. A permanently-red gate trains the reader to ignore it, so it is either fixed or its expectation is corrected. <br>**What is established:** both autoclick steps land and BOTH cycle the combo (`[frontend] click (600,474) -> combo cycled (panel 0)` ×2), and a single write-back at Continue records the combo's final index: `[setfield] combo id=1075 -> val=1`. So the clicks work; the FINAL INDEX is not what the recipe expects. <br>**Two independent runs give `val=1`. The behaviour is stable, not flaky.** <br>⚠️ **HYPOTHESIS RAISED AND REFUTED IN THE SAME SPRINT — recorded so it is not re-proposed.** I suspected the gate was not idempotent: it asserts an ABSOLUTE value (`WANT=2`) after TWO RELATIVE clicks, it writes settings that persist (`SAVEGAME/settings.cfg`), and unlike MA's `parity_2d` it has **no pin/backup/restore** (0 matches for `cp -|backup|restore|pin`). That predicts drift between runs. **Run 2 produced the identical `val=1`, so history dependence is NOT the cause here.** The missing pin remains a real latent fragility worth fixing, but it must not be reported as the explanation. <br>**Arithmetic that bounds the answer:** two cycles from start index `S` over `N` items give `(S+2) mod N = 1`. `N=3, S=2` fits; so does `N=2, S=1`. The recipe's `WANT=2` assumes `S=0` and `N>=3`. **So either the combo does not start at 0, or it has fewer items than the recipe assumes.** <br>**NEXT:** trace `id=1075`'s ITEM COUNT and INITIAL index (the `[combo] GetIndex` lines carry no id, which is why this is not already answered). Then either fix the start state or correct `WANT` — and say which, rather than tuning the constant until the gate goes green. | <br>⭐ **S331-S2 (2026-08-29) — ROOT CAUSE, from the declaration rather than by tuning.** `SVIEWER.CPP:92` declares the control `SETFIELD(ADDBIT(gamedifficulty, GD_GUNCAMERAONTRIGGER, GD_GUNCAMERAATSTART), IDC_CBO_GUNCAMERAONATSTART, **RESCOMBO(CAMERAOFF,3)**, NOLEVEL)`. So **N = 3** (off / on-trigger / at-start) and the index maps to two difficulty bits: 0 = neither, 1 = ON-TRIGGER, 2 = AT-START. <br>With `N = 3`, the observed `(S + 2) mod 3 = 1` gives **S = 2** — **the setting is ALREADY "at start" before the gate touches it.** Two cycles run 2 → 0 → 1, landing on ON-TRIGGER. **The gate's `WANT=2` silently assumes the combo starts at 0**, which it does not, and that assumption is nowhere stated in the recipe. <br>**This also explains the stability that refuted my S1 hypothesis:** the start index is the same every run, so the result is reproducibly 1 rather than drifting. The two facts fit together only if the write-back does not change what the NEXT run starts from. <br>⚠️ **`S = 2` is ARITHMETIC, not observation** — derived from `N=3` and the measured result, not read from a log. So this sprint made it observable instead of asserting it: `[combo] GetIndex` lines were **anonymous**, which is exactly why a red gate could not be diagnosed from its own output. They now carry the control id (`GetIndex id=%d -> %ld`). <br>**NEXT:** run and read the actual starting index for id 1075. If it is 2, the fix is to correct the gate's premise — and the RIGHT correction is to assert what the gate actually claims (*"the UI writes the preference"*, i.e. stored value == the combo's own final index) rather than hardcoding a different constant, which would only fit the number until the start state changes again. <br>⚠️ **S331-S3: MY ID-TAG DOES NOT WORK — `GetDlgCtrlID()` returns 0 for these controls.** The trace now reads `[combo] GetIndex id=0 -> 5`, `id=0 -> 2`, … on all 21 lines. These are OLE-hosted controls; at the point `GetIndex` runs they have no dialog child id, so the tag is uniformly 0 and the lines are **still anonymous**. The instrument built to make the start index observable **does not observe it**, and that is recorded rather than worked around. <br>**So `S = 2` remains ARITHMETIC** — derived from `N = 3` (`RESCOMBO(CAMERAOFF,3)`, `SVIEWER.CPP:92`) and the measured `(S+2) mod 3 = 1` — **and is still unconfirmed by observation.** <br>**NEXT:** get the id from a source that HAS it. `GetDlgCtrlID()` is the wrong question for an OLE host; the OLE registry knows each control's `ctrlId` (bob_ole's `OleHost::ctrlId`, already printed by `BOB_TRACE_SYSBOX` and the `[panel]` line), so the map is control-pointer → ctrlId. Tag the trace from there, or log the index from the SETFIELD side, which already knows the id and prints it. <br>⭐ **S331-S4 — CONFIRMED BY OBSERVATION, not arithmetic:** `[combo] SetIndex id=1075 <- 2   (the START state)` then `[combo] GetIndex id=1075 -> 1`. **S = 2 exactly as derived.** The gun-camera preference already reads AT-START before the gate touches it; two cycles run 2 → 0 → 1, and `WANT=2` assumes a start of 0 that is never true. <br>**Two instrument failures on the way there, both worth keeping:** (1) `GetDlgCtrlID()` returns **0** for OLE-hosted controls — the registry holds `ctrlId`, so `bob_ole_ctrlid(wrapper)` now exposes what `findHost()` already knew; (2) tracing `GetIndex` could never answer the question anyway, because it fires **once, at the SETFIELD write-back** — it only ever reports the END state. **The start index is SET and never read**, so the trace had to go on `CRCombo::SetIndex`. <br>**THE FIX IS THE GATE'S ASSERTION, NOT THE CONSTANT.** `WANT=2` could be changed to 1 and the gate would go green today and lie tomorrow, because it would still encode an unstated assumption about the start state. What the gate actually claims — its own header says so — is *"the combo's value reaches Save_Data"*. That is testable without any absolute: assert `stored == (SetIndex_start + clicks) mod N`, with `N=3` from `RESCOMBO(CAMERAOFF,3)` and both endpoints now observable in the log. **A gate that green-lights on a hardcoded number it cannot justify is worse than a red one.** <br>**Remaining:** implement that assertion (R13 has used its 4 sprints and rotates first). |
| R3.8b | **R3.8 re-examined with R12's skip report: NO dialog is blank-by-filter** | **S333 (2026-08-29).** R12's instrument was built to make R3.8 readable in one line instead of narrowing by elimination; this is its first use on the question. Campaign drive (`bob_detect_probe.sh`, whose own header warns not to read its zeros as findings), 9 dialogs traced: <br>`dlg hosted=184 DREW=156 | dead-sweep-row=11 not-in-template=17` (the LW Directives grid, dlg 1032) <br>`dlg hosted=22  DREW=12  | not-visible=6 not-in-template=4` <br>`dlg hosted=0   DREW=0` — the ONLY zero-draw dialog, and it hosts nothing, which is legitimate. <br>**Every skip is attributed to a NAMED filter, and no dialog hosts controls it then fails to draw.** So the panel drawer's filters are NOT the cause of "campaign screen shows no aircraft list" — an explanation that could otherwise have cost several more sprints of elimination. <br>⚠️ **WHAT THIS DOES NOT ESTABLISH: whether the briefing/campaign screen was among those 9.** The skip line printed a POINTER, not a dialog id, so a healthy report cannot be matched to a screen. **A clean result about unidentified dialogs is not a clean result about the one in question** — the same shape as PO-82's instrument reporting health for a path it never reached. <br>**FIXED THIS SPRINT:** the skip line now carries `dlgId=` (the host record has always had it; the neighbouring `[panel]` trace already printed it, so this line was simply the odd one out). The next run can say WHICH screens were healthy. | <br>⭐ **S333-S2 — the dlgId tag paid off IMMEDIATELY, and the answer is that the screen was never reached.** With ids, the 8 traced dialogs resolve to: `1040 IDD_SIDESELECT` 3/3, `289 IDD_SCAMPAIGNSELECT` 2/2, `1043 IDD_PHASEDESCRIPTION` 1/1 (×2), `1191 IDD_CAMPNAME` 5/5, `823` (not an `IDD_`), and `1032 IDD_LWDIRECTIVES` 156/184. <br>**That is the campaign SELECTION flow. There is no briefing or aircraft-list dialog anywhere in it** — `bob_detect_probe.sh` goes selection → directives → flight and never opens the briefing R3.8 is about. <br>⚠️ **So last sprint's "no dialog is blank-by-filter" was TRUE AND IRRELEVANT to R3.8.** It described 8 dialogs, none of which is the one in question. **Without the dlgId it read as reassurance about the wrong screens** — the third time this session an instrument has reported health for a path that never reached the target (PO-82's texture counter, PO-81's straight-line flight, this). The tag is what turned an unfalsifiable "looks fine" into a specific "not reached". <br>**POSITIVE RESULT WORTH KEEPING:** those 6 campaign-selection dialogs ARE verified to draw everything they host, and `IDD_LWDIRECTIVES`'s 28 skips are fully accounted for (11 dead-sweep-row + 17 not-in-template), which is the S150 behaviour working as designed rather than a defect. <br>**NEXT: a recipe that OPENS THE BRIEFING.** `tools/bob_convoy_campaign.sh` runs the German Convoys campaign end to end and must pass through it. Until a `[skip]` line carries the briefing's own dlgId, R3.8 has no measurement at all. <br>🔴 **CORRECTION (same sprint): I READ THE LOG TOO EARLY AND WAS WRONG. The briefing WAS reached.** A 9th `[skip]` line landed after my read: **`dlgId=1164` = `IDD_BOBFRAG`** — the frag/briefing dialog — **`hosted=22 DREW=12 | not-visible=6 not-in-template=4`**. My "there is no briefing dialog anywhere in it" was an artefact of reading a running log, not a property of the run. **The lesson is the one I keep re-learning in a new costume: a partial log is not a result, and "absent" is the easiest thing to conclude prematurely.** <br>⭐ **R3.8 NOW HAS ITS FIRST REAL MEASUREMENT: 10 of the briefing's 22 hosted controls never draw** — **45 %** — split **6 `not-visible`** (the game itself called `ShowWindow(FALSE)`; SP.2/S123 honours that) and **4 `not-in-template`** (absent from the installed BDG PE template; S124 filter). It is the largest proportional skip of any dialog in the drive. <br>**This reframes R3.8 entirely.** The earlier reading was "the list is created, hosted, visible, populated and drawn at a real rect, so the defect is row PAINTING". **The skip report says something different: a large share of that dialog's controls are dropped BEFORE any painting, by two named filters.** Whether the aircraft list is among those 10 is now the whole question. <br>**NEXT (S333-S3):** the counters say HOW MANY, not WHICH. `BOB_TRACE_SYSBOX` already dumps per-control `dlgId/ctrlId/visible/dlu` — run it with `BOB_TRACE_SKIP` on the same drive and list the 6 hidden and 4 template-absent ctrlIds for dialog 1164. If the list control is among them, R3.8 is not a painting bug at all. <br>⚠️ **Do not "fix" it by removing a filter.** Both exist for measured reasons — S123 (a control the game hides must not be drawn or clicked) and S124 (drawing controls absent from the installed template put source-only combos on the BoB layout). Removing either to make a list appear would trade one visible defect for two invisible ones.
| R12 | ✅ **DONE: silent control skips now announce themselves** (cross-port from MA S329-S2) | **Generated by MA's PO-83 fix, 2026-08-29.** There, the PO's "empty variants screen" was a control type that was created, hosted, classified and populated — and then had **no branch in the main draw dispatcher**, so it was never drawn and **nothing said so**. It took four sprints of narrowing by elimination. <br>**The transferable lesson: a control skipped SILENTLY is indistinguishable from one that does not exist.** `bob_ole_draw_panel` has several distinct skip paths — `!visible`, dead sweep row (dlg 1032), not in the PE template, no DLU rect — and any of them can blank a screen without a word. <br>**BUILT:** `BOB_TRACE_SKIP=1` reports once per dialog: controls hosted for it, how many DREW, and how many each filter dropped, with an explicit `<-- EVERY control was skipped: this screen is blank BY FILTER` when nothing survives. **It reports even when everything draws** — a report that only appears on failure cannot prove it was running, which is the mistake MA's PO-82 instrument made twice. <br>**Serves R3.8 directly** ("campaign screen shows NO aircraft list"), which has been narrowed by elimination for several sprints and would be one line to read with this. <br>✅ **PROVEN TO SPEAK, and non-trivial on its first real run.** Via `tools/bob_settings_nav.sh` (the tool built for exactly this reason — *"a zero from an instrument whose code path is never reached says nothing at all"*): <br>`[skip] dialog=0xa8da070 hosted-for-this-dialog=20 DREW=20 | not-visible=0 dead-sweep-row=0 not-in-template=0 no-DLU-rect=0` <br>`[skip] dialog=0xa8ae640 hosted-for-this-dialog=23 DREW=18 | not-visible=0 dead-sweep-row=0 **not-in-template=5** no-DLU-rect=0` <br>One dialog draws everything it hosts; the other **silently drops 5 of 23 controls** to S124's PE-template filter. Whether those 5 SHOULD be dropped is a separate question — S124 added that filter deliberately for source-only controls absent from the BDG template — but until now the drop was invisible. **That is the whole point of the item.** <br>⚠️ **Two false zeros on the way, both mine, both the same mistake in different clothing:** (1) the front-end session never reaches `bob_ole_draw_panel`, so its zero meant "path not taken"; (2) I then grepped the WRAPPER's stdout (`/tmp/r12c.log`) when the script redirects the GAME's output to `$OUT/nav.log` — the lines existed all along. **A zero from a log is only evidence if you have confirmed you are reading the log the process actually wrote.** <br>⚠️ **BoB's OLE layer does NOT share MA's structure** — a single generic `OleHost`, no `CT_*` dispatch — so MA's missing-branch bug cannot exist here in that form. That RULES OUT the missing-branch explanation for R3.8 rather than confirming it, and the cross-port is a lesson, not a transplant. |
| R11 | ✅ **DONE (automatable half): ACMI orientation gate, wired into the suite** | Gate: `tools/bob_acmi_orientation.sh` (MA twin: `port/acmi_orientation.sh`, in MA's `$ALL`). Asserts the Tacview CONVENTION — written **Yaw ≈ (360 − course)** and written **Heading ≈ course** — rather than internal consistency, which passed at 3.8° on a file that was wrong because yaw and course share the convention. <br>**PROVEN BY POSITIVE *AND* NEGATIVE CONTROL** (a gate that has only ever passed proves nothing): `bob_260829_squadron.acmi` (post-fix) **PASS** raw=141.53 negated=7.15; `bob_260829_kill.acmi` (pre-fix) **FAIL** raw=4.85 negated=144.09; `bob_260829_kill_FIXED.acmi` **PASS**. MA twin: `260828_FIXED.acmi` PASS 160.31/8.50, `260828.acmi` FAIL 8.50/160.31. <br>⭐ **EAST-WEST SEGMENTS ONLY, and this is not a refinement — it is the whole gate.** Yaw 000 is a fixed point of the negation. On the PO's real post-fix sortie the WHOLE-FILE medians were raw 8.31 vs negated 10.54, which reads as "still broken" on a file that is CORRECT; restricted to east-west the same file gives 141.53 vs 7.15. I made exactly that misreading while verifying my own fix. The gate filters `|sin(course)| >= 0.85` and needs ≥50 such segments, else exits 2 INCONCLUSIVE saying "fly an east-west leg". <br>**Always prints BOTH hypotheses side by side.** Reporting only "yaw matches course" is precisely what hid the original bug for days. <br>**Two design faults found and fixed during wiring:** (1) the suite input was `acmi_current.txt` PLUS `VIDEOS/*.acmi`, but every archived file predates the fix and is legitimately wrong — the gate would have been permanently red, which trains the reader to ignore it. Now the LIVE export only; pass paths explicitly to audit the archive on purpose. (2) exit precedence took the max, so INCONCLUSIVE (2) masked FAIL (1) — a real regression sharing a run with an undiscriminating sortie was being downgraded to "could not tell". FAIL now dominates. <br>**REMAINING (manual, PO):** confirm in Tacview that an east-west leg flies nose-first. The automated half cannot see what Tacview draws — it only pins the convention against regression. |
| R9 | 🟢 **FIXED: centring is now the DEFAULT (S369)** — the PO reported the front end painting only its top-left 1024x768 on a 1920x1080 screen and had not chosen between centre and scale. Leaving a known-visible defect switched off awaiting a preference serves nobody, so centring ships as the default and the preference stays available: `BOB_SCALE_UI=1` scales, `BOB_NO_CENTRE_UI=1` restores top-left. **Centring is the half that can be defaulted safely** — it resamples nothing (the instrument that found R9 measures the identical lit-pixel count, 786427, so the canvas is intact and merely offset) and it shifts hit-testing by ONE constant the click path already compensates for, whereas scaling changes every pixel and moves hit-tests non-uniformly. GATE R9 now asserts all FOUR placements — default(centred)/topleft/centre/scale, **PASS**, pixel-exact at (448,156)-(1472,924). The gate went red when the default moved, which is what it is for; its expectation was updated because the change was deliberate, and the top-left placement kept an arm of its own so the revert path did not become untested the moment it stopped being the default. ~~🔴 ROOT-CAUSED: at 1920x1080 the front-end paints only the top-left 1024x768 and leaves 62% of the window black** | **PO 2026-08-29**, three times: *"can't exit bob - clicking on icon just makes left monitor black"*. Previously mis-read as a crash and as an all-black window; it is neither. <br>⭐ **MEASURED, by `BOB_SHOT2D_EVERY` (S243, `glReadPixels`), the only instrument here that can see this app:** <br>`present 137 -> (1024x768)  nonblack=786223/786432` = 99.97% <br>`present 138 -> (1920x1080) nonblack=786265/2073600` = **37.9%** <br>786,265 ≈ **1024x768 = 786,432**. Non-black bounding box of frame 138: **(0,0)-(1024,768)** exactly. So when the window is 1920x1080 the front-end still draws its 1024x768 artwork into the TOP-LEFT CORNER; the other 1.29 M pixels are never painted. The PO sees a black monitor with the game in one corner and cannot find the exit. Evidence frame kept at `/tmp/r9_evidence_1920.ppm`. <br>**It is NOT the viewport:** `bob_check_present_rect` fired **zero** times across the session, so `g_scrW/g_scrH` DID equal the 1920x1080 drawable. And it is not a stretched upload — a 1024x768 `g_gdiFB` scaled onto the full-viewport quad would be entirely non-black. The GDI framebuffer IS 1920x1080; only its top-left 1024x768 receives content. **The 2D front-end does not re-lay-out for the new mode.** <br>**Trigger:** `[vid] ChangeDisplaySettings 1024x768 -> 1920x1080` — the PO's campaign-resolution choice (S177/S241 made that setting reachable and effective). On Windows the game would change the DESKTOP mode so its fixed 1024x768 art filled the screen; here it resizes the SDL window instead, and nothing centres or scales the art. <br>⚠️ **RETRACTED, twice over:** (1) I measured this window as "99.98% black" with `ffmpeg x11grab` and began diagnosing from it — `x11grab` cannot see this app's GL content (MA-S233), and the framebuffer was never uniformly black. (2) I then blamed an undefined `GL_CURRENT_COLOR` modulating the present quad; `bob_gdi_present:994` already does `glDisable(GL_BLEND)` and `glColor3f(1,1,1)`, so that was wrong too. Both were replaced by this measurement. <br>**FIX NOT YET CHOSEN — it is a real design call, so it should not be guessed:** centre/letterbox the 1024x768 canvas in the larger window, or scale it to fit. **Scaling moves every hit-test**, and `bob_check_present_rect`'s own comment records that presenting into one rect while mapping clicks against another is a known offset bug (P6/MA-S209b). Centring is the conservative option and needs the click mapping shifted by the same offset. | <br>**S277 (2026-08-29) — FIX IMPLEMENTED, MEASURED, and DEFAULT-OFF (`BOB_CENTRE_UI=1`).** Centre-vs-scale is the PO's call, so this ships opt-in: scaling would move every hit-test, and presenting into one rect while mapping clicks against another is a known bug shape here (P6/MA-S209b). Centring is the conservative half and is now available to try. <br>**Verified by the instrument that found the bug** (`BOB_SHOT2D`, `glReadPixels`): at 1920×1080, centring OFF → bbox **(0,0)-(1024,768)**, `nonblack=786427/2073600`; centring ON → bbox **(448,156)-(1472,924)**, `nonblack=786427` — the SAME lit-pixel count, so the canvas is intact, exactly centred, nothing duplicated or clipped. <br>**Two wrong implementations, both caught by measurement rather than reasoning:** (1) offsetting each blit inside `bob_gdi_setdibits` left the bbox at **(133,156)-(1472,924)** — ~19k pixels are drawn by a path that never calls that function, so per-writer offsetting can never be complete. (2) Moving the canvas without clearing left the OLD position underneath (bbox `(0,0)-(1472,924)`, **1220342** lit pixels instead of 786432) because the framebuffer persists between frames. **The working shape is to offset at PRESENT time** — map the content sub-rect of the texture into a centred window rect — which moves every drawing path at once and needs no clear. <br>**Clicks move with it:** the handler subtracts `g_uiOff` from the framebuffer coordinate, so hit-testing stays aligned with what is drawn. <br>**New test hook `BOB_FORCE_MODE=WxH`** applies a display mode at startup: the PO reaches 1920×1080 through Campaign Resolution, but a gate cannot drive that UI, and without this the larger-window case was unreachable for automated verification. <br>✅ **S342: the one-frame lag is FIXED.** The offset used to be computed AFTER the quad, so every frame was placed with the PREVIOUS frame's extent — and R9's own trigger IS a mode change, so the lag landed exactly where the feature is meant to help. All drawing paths have run by present time, so this frame's extent is already complete there; resolving placement before the quad removes the staleness, and `g_uiExtLast` survives only as the fallback for a frame that drew nothing. **Verified with the instrument that found R9** (`BOB_SHOT2D`, `glReadPixels`, `BOB_FORCE_MODE=1920x1080`): **frame 1** now measures bbox **(448,156)-(1472,924)**, a 1024x768 region exactly centred, `nonblack=786427` — the same lit-pixel count as steady state, so nothing is clipped or duplicated. <br>⭐ **S359: THE SCALE OPTION NOW EXISTS, so the choice can actually be tried.** `BOB_SCALE_UI=1`
(mutually exclusive with `BOB_CENTRE_UI`, which wins if both are set since it is the conservative one).
Uniform scale — never stretches the art — filling the window and letterboxing the remainder. Measured
at 1920×1080 with the instrument that found R9:

    [scale] UI content 1024x768 -> 1440x1080 (x1.406) at (240,0) in 1920x1080
    lit bbox = (240,0)-(1680,1080) = 1440x1080, nonblack = 1555200 (fully lit)

against centring's `(448,156)-(1472,924)`, 786,427 lit — **roughly twice the screen area used**.
<br>Building it also required fixing a real gap: the content-extent measurement was gated on
`bob_centre_ui()` alone, so scaling had nothing to scale and printed nothing at all. Both the
accumulator and its reset now run for either mode.
<br>✅ **S360: HIT-TESTING UNDER SCALE IS VERIFIED, with a control arm.** A real SDL mouse click at
**(535,333)** — the scaled screen position of the main menu's "Quick Shots" (unscaled centre (210,237),
×1.406, +240 letterbox) — selects it and the run reaches **artnum 27923**. The **control**: the same
pixel with scaling OFF selects nothing and stays on 28937. So the compensation is doing the work,
rather than a coincidentally generous hit box.
<br>⚠️ **My first attempt at this test was invalid** and is recorded so it is not repeated: `BOB_CLICKXY`
injects FRAMEBUFFER coordinates directly (`bob_video.cpp:484`) and deliberately bypasses the
compensation, so feeding it a scaled coordinate tested nothing. The real-mouse path (`:662`) is the one
carrying the transform, reached via `BOB_SDL_CLICK`. Two click paths, and only one of them is the
subject.
<br>⚠️ ~~**The click mapping is written but NOT yet exercised.**~~ Clicks are un-letterboxed and divided by
the same `g_uiScale`/`g_uiScaleOffX/Y` the quad is drawn with, in the same frame — deliberately from
the same variables, because presenting into one rect while hit-testing another is the P6/MA-S209b bug
this port already has scars from. But no click has been driven through it yet, and R9's own warning is
that scaling *moves every hit-test*. **Verify hit-testing before this is made default.**
<br>✅ **S361: both placements are now GATED** — `tools/bob_r9_layout.sh`, wired into the suite as
`GATE R9`. Asserts the LIT REGION of a real capture for three arms: **default `(0,0)-(1024,768)`** (the
R9 defect itself, kept as the baseline so the bug is documented in a runnable form), **centre
`(448,156)-(1472,924)`**, **scale `(240,0)-(1680,1080)`**. All three pixel-correct. It reads pixels via
`BOB_SHOT2D`'s `glReadPixels` — the instrument that found R9 — rather than the `[centre]`/`[scale]` log
lines, which report only what the code *intended*.
<br>**Still the PO's decision:** centre or scale to fill — but now both are runnable, both verified, and
both protected from silent regression.
| R10 | ✅ **FIXED (S275): the Tacview export stopped dead at 20.48 s of every sortie** | **PO 2026-08-29**: *"tacview file works! But very short"* — 90 KB, 511 time markers, **20.40 s** of a full dogfight. <br>**ROOT CAUSE — same defect as MA's PO-79, same engine:** `REPLAY.CPP` fed `bob_acmi_time((double)replayframecount / _hz)`, and **`replayframecount` RESETS EVERY BLOCK** (`#define FRAMESINBLOCK 512`, `REPLAY.CPP:106`, wrapped at `:556`). At the first wrap the timestamp jumped back to 0; `bob_acmi_time`'s `if (seconds <= g_acmi_lastT) return 0` guard (`bob_acmi.cpp:71`) then returned 0, and the caller's **`goto acmi_done` skipped every remaining sample for the rest of the flight**. 512 frames ÷ 25 Hz = **20.48 s** — the exact length the PO got. The recording never resumed, so the export silently held only the opening seconds of every sortie. <br>⚠️ **The obvious fix is WRONG and would have cost the S274 win.** A free-running per-call counter restores monotonicity but destroys de-duplication: S274 measured that `StoreDeltas` runs **~7.7× per `replayframecount` advance**, and duplicate suppression works precisely BECAUSE the raw counter stays put across those calls. Incrementing per call would have re-inflated a sortie to the 57 MB S274 fixed. <br>**FIX:** `bob_acmi_frame_monotonic(raw)` keeps the raw counter as the de-duplication key and only lifts it past each wrap (`base += lastRaw+1` when `raw < lastRaw`), reset per recording in `bob_acmi_begin`. Verified by simulation before shipping: 3 blocks × 512 frames × 8 duplicate calls = 12,288 samples → **monotonic, and 1,536 distinct timestamps, not 12,288**, with the first wrap lifting to 512 rather than 0. `BOB_ACMI_BLOCKCLOCK=1` reverts for an A/B. <br>**Also fixed (S275b):** the PO's first export ended **mid-record** (a partial line, no terminator) because the process held unflushed stdio. `bob_acmi_time` now flushes every 25 markers, bounding loss to ~1 s of track. <br>⚠️ **Still open:** `bob_acmi_save_as()` publishes `VIDEOS/<stem>.acmi` with a truncation guard, but the PO's flight left `VIDEOS/Bob.cam` with **no `.acmi` beside it** — that call is not reached on the interactive route, so the export must currently be copied out of `acmi_current.txt` by hand. |
| R8 | ✅ **FIXED (S241): ALT+X out of the 3D killed bob with a bogus "desktop resolution is TOO LOW" fatal** | **PO 2026-08-29**: *"tried to save bob dogfight replay, but crashed on ALT X from 3D"* (`~/Videos/260829_bob_dogfight.mp4`). The session log gives the exact sequence: `[startfly] flight close (id=1) -> OnOK + OnFlyingClosed` → screen repainted → `*** FATAL: Sorry, your current desktop resolution is TOO LOW to continue! This simulation requires at least 1024x768` — on a **1920×1080** desktop. Leaving the 3D calls `SaveDataLoad::ChangeMode()` to restore the 2D UI resolution, and that is the path that failed. <br>**ROOT CAUSE (the uninit-read-fed-by-a-Win32-stub class again):** `bob_enum_display_mode` (`compat/bob_video.cpp:1009`) treated `idx` as a plain index: `if (n <= 0 \|\| (int)idx >= n) return 0;`. `ENUM_CURRENT_SETTINGS` is **`(DWORD)-1`**, so `(int)idx == -1` **slips past that bounds check** (-1 >= n is false), and the descending-order reversal then asked SDL for mode `n - 1 - (-1)` == **n**, one past the end. Every `EnumDisplaySettings(ENUM_CURRENT_SETTINGS)` therefore failed. <br>**Why that became a fatal:** the header stub (`bob_dx_extra.h:84`) leaves the caller's `DEVMODE` **untouched** on failure, and `TWODPREF.CPP:401` **ignores the return value** — so `devmode.dmPelsWidth` was uninitialised **stack garbage**. `TwoDPref.cpp:402` zeroes it only when it falls outside `[512,8200]`; garbage landing in **512..1023** is kept, and `:450-452` then trips the fatal at `:454`. **This is why the crash was intermittent, not constant** — it depended on what was on the stack. <br>**FIX:** answer `ENUM_CURRENT_SETTINGS` / `ENUM_REGISTRY_SETTINGS` from `SDL_GetCurrentDisplayMode` (falling back to `SDL_GetDesktopDisplayMode`), which is what Windows returns, and reject negative indices explicitly. <br>⚠️ **Not yet proven end to end** — the code path needs a flight plus ALT+X to exercise, so it is fixed-by-construction and awaiting the PO's next 3D exit. Two other `ENUM_CURRENT_SETTINGS` callers benefit: `TWODPREF.CPP:267` and the mode-list builders. |
| R1 | Recording chain proven in two halves | ⭐ **S347: NAVIGATION UNBLOCKED — the route into the 3D is `BOB_AUTOCLICK="0,1,2"`, and BOTH HALVES NOW RUN IN ONE PROCESS.** Enumerated with `BOB_DUMP_MENU`. 27917 is reached from **Quick Shots**, not from Sim Config: `28937 main -> 0 "Quick Shots" -> 27923 -> 1 "Fly" -> 27917`, whose menu is three items — **`0 "Back"  1 "Sim Config"  2 "Fly"`**. So the recorded obstacle was wrong twice over: index 0 is **Back**, not the strategic map, and there is a **Fly at index 2**. `BOB_AUTOCLICK="6,3,#1075,6,0,1,2"` (Sim Config, Views, cycle, Continue, Quick Shots, Fly, Fly) produces `[setfield] combo id=1075 -> val=0` **and** `Launch3d done; InThe3D=1` in a single run — the UI half and the flight half together, which is what R1 asks for. <br>⚠️ **S346 overreached**: it reported "the blocker does not reproduce" from the Sim Config path alone. The correct scope is that it does not reproduce *on that path*; 27917 is real and reachable on the Quick Shots path. <br>⭐ **S348: RECORDING VERIFIED — the chain runs end to end in ONE process.** `BOB_AUTOCLICK="0,1,2"` reaches the 3D (`Launch3d done; InThe3D=1`) and the flight WRITES: `VIDEOS/replay.dat` went 946087 -> **173058** bytes, i.e. truncated (`ResetFileFlag`) and rewritten as a fresh recording, growing ~1.2 KB/s while a flight is live. So navigate -> fly -> record all compose in a single run, which is what R1 asked for. <br>⚠️ **`VIDEOS/replay.dat` IS A SCRATCH BUFFER and every flight truncates it** — that is the game's own behaviour, not a port defect, but it means any automated flight destroys whatever was there. The SAVED replay `SAVEGAME/dreplay.dat` is a different file and was verified byte-identical before and after. Back up `VIDEOS/replay.dat` before any recording run. <br>⚠️ **S346's "the blocker does not reproduce" was too broad, and S347's "wrong on both counts" was also too strong.** `FULLPSYS.CPP:1524` (S316) had already enumerated 27917 and recorded that on THAT screen index 0 is Back, whose `onselect` ran LaunchMap into the S315 segfault — which is where R1's "strategic map" note came from. My `0,1,2` is a PER-SCREEN sequence (main menu -> Quick Shots, 27923 -> Fly, 27917 -> Fly), not three clicks on 27917. Both are right about different things. <br>~~S314: pre-flight delay knob added (`BOB_STARTFLYING_DELAY`, default 30 = unchanged); still blocked, on NAVIGATION not timing.~~ The UI half passes inside the combined process (`[setfield] combo id=1075 -> val=2`). The delay does move the pre-flight, but `BOB_AUTOCLICK` advances **one step per screen PAINT** while the pre-flight fires on a **tick** count, so the clicks always run after it regardless — and "Continue" out of Sim Config therefore returns to **artnum 27917**, not the main menu 28937. Index 0 there is the strategic map, which segfaults. **Next: enumerate 27917's menu.** `BOB_DUMP_HITTARGETS` does NOT work here (no output — its dump sits behind a condition this path never reaches), so that needs a different instrument. |
| — | Strategic map segfaults from the post-Sim-Config screen | Found by S313 taking a wrong menu index: `CMapDlg::InvalidateAnotherItem` → `Persons2::ConvertPtrUID` (`Persons2.cpp:251`), signal 11, fault_addr=0x4800. Different subsystem from the S310–S312 OLE work. Not investigated. |
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


## S313 — R1: the gate exists, one half moved, and the blocker is named

`tools/bob_r1_continuous.sh` runs the session STATUS has wanted for many sprints: one process, Sim
Config → Gun Camera → Fly → `replay.dat`, with **no arming hook of any kind**. `BOB_GUNCAM=1` arms
the recorder directly and `BOB_GUNCAM=pref` sets the preference just before the game reads it — both
skip the UI, which is exactly what R1 is about.

**What moved:** assertion 1 passes. `[setfield] combo id=1075 -> val=2` now happens *in the same
process that then attempts the flight*. That half had only ever been measured in a run of its own,
and the join between halves was explicitly recorded as an inference. Half of that inference is gone.

**Where it stops, and why it is not a product defect:** the two halves use boot modes that do not
compose. `bob_replay_record` uses `BOB_BOOT_FRONTEND=1`, which stands up a Quick Mission directly
and never passes through a menu — so it *cannot* visit Sim Config. The alternative,
`BOB_FRONTEND` + `BOB_STARTFLYING=click`, says plainly what it wants:

```
[startfly] navigate to Fly by clicks (use BOB_AUTOCLICK=0,1,2)
```

It expects the click list to be *only* the navigation to Fly, and it runs its pre-flight on a
hard-coded timer (`if (++sf_t < 30) return;`). Prepending five clicks for the preference changes
which screen Continue returns to — **artnum 27917 with the harness, 28937 without** — and menu
indices are per-screen.

### The wrong-index run found a crash

Appending `0` for Quick Shots opened the **strategic map** instead, which segfaulted:

```
CRASH: signal 11  fault_addr=0x4800
  Persons2::ConvertPtrUID (Persons2.cpp:251)
  CMapDlg::InvalidateAnotherItem (MapDlg.cpp:1187)
  CMIGApp::OnIdle
```

Logged as its own line. It is in a different subsystem from the S310–S312 OLE dispatch work, but I
have not run an older binary to prove that, so it is recorded as *found*, not as *pre-existing*.

**Two things driving the same menus do not compose by concatenating their click lists** — and the
failure mode is not an error but a click landing somewhere plausible.


## S314 (bob) — R1: the blocker is navigation, not timing

Added `BOB_STARTFLYING_DELAY=<ticks>` (default 30, so every existing recipe is unchanged) to push
`BOB_STARTFLYING=click`'s pre-flight past the Sim Config trip. **It works and it does not help**,
which is worth writing down because the hypothesis was reasonable and wrong:

`BOB_AUTOCLICK` advances **one step per screen paint**; the pre-flight fires on a **tick count**.
Those are different clocks. Delaying the pre-flight to tick 300 moves it later in wall time but the
clicks still land after it, because the screens that pace them are painted after init either way.
So the ordering — pre-flight, then clicks — is invariant under this knob.

The consequence is unchanged: "Continue" out of Sim Config returns to **artnum 27917** rather than
the main menu **28937**, menu indices are per-screen, and index 0 on 27917 opens the strategic map.

**The strategic-map segfault reproduced a second time**, same frames:

```
Persons2::ConvertPtrUID (Persons2.cpp:251)
CMapDlg::InvalidateAnotherItem (MapDlg.cpp:1187)
CMIGApp::OnIdle
```

Two independent runs, so it is reproducible rather than incidental — worth its own fix regardless
of R1.

⚠️ `BOB_DUMP_HITTARGETS=1` produces **no output** on this path. Its dump is inside a block this
configuration never reaches, so "no hit targets printed" says nothing about the screen. Anyone
enumerating 27917 needs a different instrument — do not read the silence as "the screen has no
menu", which is exactly the null-as-fact trap this port keeps re-learning.
