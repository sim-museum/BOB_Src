# Battle of Britain — Linux Port: Status

Snapshot of where the native Linux port stands. Running engineering log (newest-first,
with evidence) is in **PORT.md**; this file is the high-level state + roadmap.

Branch: `linux-port` · Build: 32-bit i386 ELF (`gcc -m32`), SDL2 + OpenGL + OpenAL,
`-fpack-struct=1`. Game sources stay unedited; the port lives in `SRC/compat/` + the
`BOB_*` env-gated boot scaffolds.

> ## Latest (S180–S191, 2026-08-16/17): the Fly "hang" was a rendering handoff that was never written
>
> One continuous night of Product-Owner-driven work. The PO reported an apparent hang **five times**
> (*"clicked FLY, dead parrot"*, *"it's now definitely an ex-parrot"*). Root cause was ours, and it
> was half of a change I had made the sprint before.
>
> | # | What | Result |
> |---|---|---|
> | **S180** | `LaunchFullPane` set the game's page state but never handed the screen to the front-end painter — so `bob_frontend_tick` painted **neither** the map nor the page | window froze on its last frame; clock + toolbars correctly gated off with it. **A/B on one frozen binary: 0 suppression events with the fix, 49 with `BOB_NO_PAGE_HANDOFF=1`** |
> | S180b | `MASTER.FIL`'s `FIL_x*` entries carry a marker `x` at a shifting position that nothing substituted | `buxtton1→button1` and 5 more, **6 for 6** against the shipped art; retry-on-miss only |
> | S181 | `RMdlDlg` blitted its whole ~780×585 art **sheet** from the origin (Windows clips it to the dialog's own DC) | template size now kept by the `.rc` parser; modal clips to it. **GATE 1c had been pinned to the buggy layout** — coordinates re-derived from `IDDS_MODAL_DIALOG`, geometry now asserted |
> | S182 | `ChangeDisplaySettings` was `{ return 0; }` — reports `DISP_CHANGE_SUCCESSFUL`, discards the mode | the PO's **1920×1080** was found (S177) and thrown away; routed to the real SDL resize |
> | S183 | `CSystemBox` is not in `toolbars[]`, so no walk drew or clicked it — and it is the **only** route to `OnBye → ExitCampaign` | the campaign had no exit at all; added to paint **and** click walks in one edit. Gold confirms it is the red X at upper right |
> | S184 | Same missing clip as S181 on the OOB tree walks | `InterceptOffered` (the dialog the PO clicks FLY on) painted past `x=1024`; now clipped to its template |
> | S185 | Text sized from each control's **box** (`h−4`), not from the font the game selected | a 14px font rendered at 36px. **Shrink-only** adoption (the one contrary case is a 48px ART font in a 26px box). Also **un-truncated** captions the controls' own shrink logic had been cutting |
> | S186 | OOB dialogs centred on a magic `340`px | centred on the real template width; first attempt measured the **wrapper** (the panel-wrapper rule, 5th time in that file) |
> | S187 | `BOB_MAP_CLICK` fired after 6 map paints, so it could only click dialogs that exist immediately | settle count is now a third field |
> | S187b | `SDL_CreateWindow` failure logged ~181× per map paint under dummy SDL | **345,600 lines → 35** in the same run |
> | **S188** | The click walk mirrored the paint walk's **set** but not its **order** | a click could pass **through** the dialog covering it. A/B: old order → `consumed by toolbar 1 ctrl id=21` (dialog beneath); top-down → `SWALLOWED` by the one on top |
> | S189/S190 | *"fast forward did nothing, then went to 300x"* | **not a defect.** `>>` hands the clock to automatic control: 1× while any dialog is open, 300× idle, 20–40× with raids inbound. Traced inside `MoveAllSAGs`: `reqacceltype=5` with a dialog open, `=2` (300×) without |
> | S191 | Census instead of a fourth per-kind special case | the classifier knew **5 of 9** R* types. `BOB_TRACE_CENSUS` over 20+ dialogs found **exactly two gaps** in the whole exercised surface |
>
> **The scaffold was passing *because* of the bug.** `BOB_CAMPAIGN_FLY` cleared the map-active flag
> **by hand** before its own `LaunchFullPane`, so it reached the briefing happily for ~40 sprints
> while every real click path froze. That compensating line is deleted; the harness now exercises
> the production handoff. A scaffold that sets engine state immediately before calling a public
> entry point is documenting a missing line, not preparing a fixture.
>
> **Verified end to end on the final binary** (real GL): map → intercept authorised → briefing →
> **Fly → `InThe3D=1`**, with **0** `[pagegate]` events. Gates green on every commit: 14/14 recipes,
> GATE 1c PASS, GATE 3 dummy==GL byte-identical, GATE 4 98.7% non-black, GATE 4b `blackTex=0`,
> binary unchanged across each run.
>
> **Two findings I would rather record than gloss:**
> - I predicted the S188 click-through at the Fly button and **both arms agreed** — I had aimed
>   outside the actual overlap. A null result from a probe aimed at the wrong pixel is not evidence
>   of absence; printing the real drawn bounds gave the discriminating target.
> - My own `[accel] band` sampler reported a constant for a whole run and I nearly read that as
>   "the band never moves". It sampled once per map paint while the value changes far faster.
> - The backlog claimed listboxes/edits/spin buttons were "presumably still missing". **Wrong** —
>   all hosted. The one type that genuinely is not is **RTabs** (the OOB tab bar).
>
> **Open after this session:** BOB-PO-18 (delete `g_bob_map_active`; MA's dispatch is the reference
> design and cannot express the S180 bug), BOB-PO-19 (OOB dialogs overlap — no layout engine),
> BOB-PO-21 (host RTabs; MA S60 is the reference, but `CRTabsCtrl` draws via `CBitmap` + memory-DC
> `BitBlt`, not the DIB path every current host uses).

> ## Previously (S164–S169, 2026-08-16): the message map is ON by default, and a cross-port census
>
> Six sprints applying (and checking) what the sister MiG Alley port learned, plus closing the
> blocker S158/S159 stopped on.
>
> | # | What | Result |
> |---|---|---|
> | S164 | `sprintf("%s", <CString>)` prints pointer bytes under GCC | **126 sites** corrected; ABI proved in isolation; `BOB_TRACE_GARBAGE` detector added |
> | S165 | `CDialog::DoModal` was a `-1` stub, so `RMessageBox` answered every confirmation itself | real modal loop; **the quit dialog now appears and is answered** (gate 1c: Save 0, Yes 1, Cancel 2) |
> | S166 | `BOB_DLG_TEARDOWN`, gated "until measured" | measured (37–42 hosts, stable); **default deliberately NOT flipped** — the gates never exercise the leak |
> | S167 | S159's file-block fatal | the compat held a `WM_GETFILE` block open and ignored all **23** `WM_RELEASELASTFILE` sends; fixed, control-armed |
> | S168 | second blocker: `WM_GETARTWORK` sent controls down the offscreen path | pinned to 0 (MA's documented answer); **A/B 11/14 → 14/14 byte-identical → message dispatch DEFAULT ON** |
> | S169 | census of five MA findings against this tree | two already solved here; one (mip-mapping) **wrong in MA**, corrected there |
> | S170 | unhosted-control census + ASan over the changed lifetimes | census **clean** (all 8 types hosted); ASan: 0 heap errors on three paths |
> | S171 | gold comparison: the map ruler's labels ran off the screen | `SetTextAlign` was a no-op; fixed — and it **corrects S169's own census entry** |
>
> **Open:** `TERRAIN-1` (dogfight tiles scrambled — still not reproduced; a gold-vs-ours cockpit
> comparison came back clean, so it needs the PO's mission/altitude detail), `HAT-2` (needs a
> physical hat press), the offscreen-composite path (`BOB_ARTWORK_DISPATCH=1` still shows it
> unimplemented), and `BOB_DLG_TEARDOWN` (measured in S166, default still off — needs a re-open
> loop recipe, not another gate run).
>
> **Method note from this session:** the Wine gold captures in `gold standard/bob/` are a stronger
> oracle than our own captures, and they found S171 after a grep-based census had cleared the same
> API. MA learned the same lesson the hard way — four of its five committed parity references had a
> defect baked into them.

> ## Previously (S156–S163, 2026-08-09/10): a play session found four real bugs the gates could not reach
>
> The Product Owner played the shipped build under `gdb` and reported four defects in ~20 minutes.
> All four are now fixed, root-caused and gated. **Every one lived in an interaction path no
> headless recipe exercised** — the direct payoff of S157's scaffold audit.
>
> | # | Symptom (as reported) | Root cause | Sprint |
> |---|---|---|---|
> | QS-1 | "clicking on fly or back has no effect" | `bob_ole_draw_listbox` never recorded its drawn rect, so the menu **re-derived** click rects from text widths; they drifted off the paint. "Fly" painted x=132..153, rect x=83..108 — **zero overlap** | S160 |
> | QS-2 | "select dogfight … lower drop-down still gives training" | the combo click branch fired `TextChanged` **without setting `bob_evtA0`**, so the handler's `index` parameter carried a stale value from the last radio/listbox click. Wrong family applied → launched Training/Takeoff | S161 |
> | QS-3 | "after ALT-X … no back button to return" | the 3D view sets 800×600 and **nothing restored the mode**; the front end kept laying out at 1024×768 and drew its menu row at y=710 — below the buffer. Not missing, unreachable | S162 |
> | HAT | "hat switch … has no effect" | POV was written to the **immediate** device state only; the **buffered** `GetDeviceData` path the game reads had no POV loop — hat computed, then discarded | S163 |
>
> **The pattern, stated once:** all four are *one half of a pair implemented, the other silently
> missing*. Alongside them S158 found `CWnd::SendMessageA` serving **3 of the 20** `WM_*` routes the
> game sends (the rest `return 0`), and `ON_MESSAGE`/`BEGIN_MESSAGE_MAP` expanding to **nothing** —
> so every `ON_MESSAGE` row in the game was decorative. None of these ever failed loudly.
>
> **What made them invisible** (S157, `doc/scaffold-audit.md`): sort scaffolds by what they
> substitute. A **shallow** one substitutes an *input* and still runs the real dispatch — harmless.
> A **deep** one substitutes a *call*, entering below the layer under test, and can never report
> that layer missing. S156 found the map's OOB dialogs had been **render-only since S113**: the only
> thing that ever drove them called `bob_evt_fire` directly. The trap is that the deeper the
> scaffold, the *more impressive* the evidence it produces — S144–S146 used one to build, fly and
> land whole raids through a dialog that could not be clicked.
>
> **Also landed:** S156 map OOB dialogs take real clicks; S157 `BOB_SDL_CLICK`/`BOB_MAP_SDLCLICK`
> push a **real `SDL_MOUSEBUTTONDOWN`**, proving the whole chain on real GL for the first time
> (headless can *never* test it — under `SDL_VIDEODRIVER=dummy` `SDL_CreateWindow` fails, so
> `pump_events` is never called at all); S158 the message map is real (all 16 dead routes register,
> base-class walk verified at `depth 1`) but stays **default-off**, blocked on a file-block fatal.
>
> **Open:** `TERRAIN-1` (dogfight tiles scrambled — not yet reproduced; my headless dogfight frame
> looks coherent, so it needs the PO's mission/altitude detail), `HAT-2` (delivery restored and
> verified; *actuation* needs a physical hat press), `S159` (message dispatch default-on blocked on
> the `bob_fp_repaint` file-block collision).
>
> **Method notes worth keeping** (the expensive lessons, all in `doc/scaffold-audit.md`):
> a stub returning a **compile-time constant** lets the optimizer delete dependent branches, which
> suppresses the link errors that would reveal the rest of the missing implementation — restoring one
> route surfaced two more. A **bisect over artifacts collected at different times** tests time as
> much as code; only a same-conditions A/B isolates the variable. And **when a comparison moves,
> establish reproducibility before explaining it** — a one-run diff and a real regression look
> identical.

> **Latest (S142, 2026-08-08): the R\* ActiveX control set is COMPLETE — all 8 types hosted.**
> `CRSpinBut` was the last unhosted one, and the LW Directives dialog is mostly made of it (**85**
> DDX-bound spinners), which is why S141 could render that screen's labels and headers while every
> allocation number stayed blank. Hosted via the §8p recipe (`SRC/RSPINBUT/bob_ole_rspinbut.cpp`,
> `CLSID_RSpinBut`), and the grid's numbers now match gold #18 **value for value** — Morning 40 /
> Mid-day 30, Reconn 0/1, the Ground-Attack matrix, Missions 1/1/1/0/0/0, Escort 2/1·1/0·2/0,
> %Free 100, Resting 1/3/2/2 and 3/1 — with the spin-arrow art drawn. Two compat gaps closed en
> route: `CWnd::ReleaseCapture` was absent (no hosted control had ever called it), and the control's
> **static** `m_bDrawing` reentrancy flag — cleared only inside `DrawBitmap` — would have latched on
> any black-fill draw and silently killed the other 84 spinners for the rest of the process
> (neutralised host-side, no game-code edit). One new deviation booked (**SP.8**): we draw a "Sweeps"
> row and an "Escort 1:1" row that gold doesn't show.
>
> **Prior (S141, 2026-08-08): the campaign PHASE is selectable — gold #18's Directives
> allocation grid renders.** Screen parity is now **18 CLOSE / 0 PARTIAL / 1 GAP** of 19, and the
> single remaining GAP (#4, the transient "Initialising 3D" loading screen) is **by design**.
> BoB models a tab row as the **columns of one `CRListBoxCtrl`**, and
> `CSCampaign::OnSelectRlistCampaigns(row, column)` picks the campaign phase from the **column** —
> which our hosted-listbox click hardcoded to `0` while resolving the row faithfully. So every
> campaign the port had ever run started in phase 0 (Convoys, 10 July, a standby day), and that,
> not any renderer, is why S137's Directives dialog came up empty. Resolving the column through the
> genuine control's own `GetColFromX` (`BOB_NO_LIST_COL` reverts) makes the phase selectable: the
> select screen reads **12th August – 23rd August** (Eagle Attack), and on that day the game opens
> Directives itself with the full allocation grid drawing through S137's deep TB_MISC walk.
> `BOB_AUTOCLICK` also gained a metrics-resolved `#ID[:COL]` step (adopted from MA S62/S63 — drive
> recipes must never encode fixed pixels). Carried: the ~50 numeric spinner boxes need `CRSpinBut`,
> the 8th R\* type and the only unhosted one; and gold #19's raid-stack deviation still stands
> because there is no headless way to *dismiss* a game-opened OOB dialog.
>
> **Prior (S132–S140, 2026-08-02/03): the gold #3 briefing arc → CLOSE, + the Directives
> dialog reached.** Screen parity was then **17 CLOSE / 1 PARTIAL / 1 GAP** of 19 (from 16/1/3). The
> Quick-Shots → mission-briefing thread was carried from crash to full parity:
> - **S132** — fixed the QS order-of-battle SIGSEGV (a variadic `DialList` copy-constructs a
>   `DialBox` from `*(DialBox*)NULL`; null-reference-safe copy ctor, RDIALOG.H).
> - **S133** — the QS order-of-battle flight-lines render (nested `DialList` draw walk; the game's
>   layout is dead headlessly, so the row stacking is synthesized).
> - **S134** (spike) — re-mapped gold #3 to `IDD_BOBFRAG` (the mission *briefing*), not a QS editor.
> - **S135** — the BoBFrag briefing renders (new `BOB_BOBFRAG` reach scaffold): roster listbox
>   (Unit/Aircraft/Duty/Callsign → 54 Squadron/Spitfire/Patrol/Trumpet) + background. #3 GAP→PARTIAL.
> - **S136** — template-driven *button* hosting → the "Return to Player" button (a template-only,
>   non-DDX RButton) renders.
> - **S137** — the LW **Directives** dialog (gold #18) is now reachable + framed: it's on the misc
>   toolbar (TB_MISC), which `bob_map_paint_oob` didn't walk; added the open scaffold + a recursive
>   TB_MISC paint. #18 GAP→PARTIAL (the dense allocation grid needs an active/Eagle-Attack phase).
> - **S139** — footer-listbox clip fix: the last footer/tab column was clipped by a tight width; the
>   "Fly" footer item now renders on the briefing (gold #3) and the QS Scenario page (gold #2).
> - **S140** — hosted `CREdtBtCtrl` (the 7th R\* control type) → the "Bob" pilot **name box**
>   renders. **With the roster + Return-to-Player + Fly footer, every gold-#3 element now renders →
>   #3 PARTIAL → CLOSE.**
>
> Only #18 (Directives grid, needs the Eagle-Attack campaign state — same gap as #19's raid-day
> map) and #4 (the by-design transient "Initialising 3D" loading screen) are non-CLOSE. All gated,
> `gl-lock`-run, dummy==GL byte-identical + safe-default + flight-94.9%-non-black verified per sprint.
>
> Prior (S131, 2026-08-02): **Per-face font registry** — the pervasive "font FACE"
> deviation is fixed. `bob_gdi_font` drew every font in the one Rowan art TTF, so data/label rows
> rendered in the art face instead of Arial. Now an 8-slot registry (ART=Intel, SANS=LiberationSans,
> SERIF=LiberationSerif, MONO=LiberationMono, each ×regular/italic) routes the game's requested
> face + `bItalic` flag to its own TTF, threaded through the DC's selected `CFont`. Config/campaign
> data/labels now render Arial with italic values = gold's scheme; the ART screens (title menu,
> headers) are `cmp`-verified byte-identical. Adopted from MA note 26 §2 (§1 Japanese-branch N/A —
> BoB already requests English faces; §3 combo-fill already handled). `BOB_NO_FONTFACE` reverts.
>
> Prior (S129, 2026-08-02): **Quick-Shots tab navigation** — building on S128's
> hosted `CRRadioCtrl`, a click on a page tab now fires the genuine `Selected` event
> (`OleHost::onButtonClick` → `bob_ole_click` → the S33 eventsink → `CSQuick1::OnSelectedRradio`
> → `LaunchDial`), switching the panel page. Clicking Parameters renders the mission-parameters
> form (Target Area / T.D. / Weather / Time / Name); Scenario switches back — bidirectional,
> verified. Gold #3 (`16-47-45`) turned out to be the *player-flight editor* (Squadron/Aircraft/
> Duty/Callsign, `CSQuickLine`), a different screen than the Parameters tab, so #3 stays GAP with
> its true path identified; parity unchanged **16 CLOSE / 0 PARTIAL / 3 GAP**. dummy==GL `cmp`
> byte-identical. (All `bob` runs now go through `gl-lock`, incl. headless captures.)
>
> Prior (S128, 2026-08-02): **Hosted the `CRRadioCtrl`** — the 6th (and last
> front-end) R\* control type. The Quick-Shots page-tab row (Scenario / Parameters / Luftwaffe /
> RAF, `IDC_RRADIO`), previously blank because that control had no host, now renders each caption
> with its selection-tick / radio icon. New host `SRC/RRADIO/bob_ole_rradio.cpp` + factory CLSID
> + build integration, mirroring the REdit/RButton pattern. #2 PARTIAL→CLOSE.
>
> Prior (S127, 2026-08-02): **Label-render fidelity** — compat `CDC::DrawText` now
> implements real DT_WORDBREAK word-wrap (the phase-select and Quick-Shots descriptions wrap
> inside their boxes, paragraph breaks preserved, instead of one clipped line off the panel edge)
> and Windows '&' accelerator escape ("Cockpit && UI" → "Cockpit & UI"). A ≥2-line-box guard keeps
> single-line config labels untouched (no regression). Parity was **15 CLOSE / 1 PARTIAL / 3 GAP**
> of 19 (#8/#16 deviations retired, #2 improved). `BOB_NO_WORDWRAP` / `BOB_NO_AMP_ESCAPE`
> revert. Implements MA note 17's shared `DrawText DT_WORDBREAK` find (outbound BoB note = §8o).
>
> Prior (S124, 2026-07-26): **BDG-oracle PE resources** — the port now reads the
> installed build's PE `.rsrc` DIALOG/DLGINIT (`English/TEXT/boblang.dll` = BDG 0.99, the parity
> oracle per SM ruling) instead of the source checkout's `.rc`: BDG rects/rows, faithful
> IDS→string-table captions, template-driven hosting of non-DDX label statics, and a
> template-membership draw filter. **All 8 config tabs are now CLOSE vs the gold shots** (More GFX
> label-for-label; Mission tab fully labeled — was label-less). `BOB_NO_PE_RSRC` reverts.
> Also closes the packaging resource-root blocker for dialog data.
>
> Prior (S123, 2026-07-25): **Release SP opened** — all 19 gold shots inventoried with scripted
> repros + native captures ([`doc/screen-parity.md`](doc/screen-parity.md), captures in
> `doc/parity/`); three systemic parity fixes (dialog-scoped label rects; menu lists at the game's
> `ListX/ListY`; runtime `ShowWindow`). Capture harness `BOB_SHOT`/`BOB_SHOT_PATH`.
>
> Prior: [`doc/STATUS-2026-07-01.md`](doc/STATUS-2026-07-01.md) (S72→S82) — all 7 historic
> quick missions (23–29) now fly; **double-exposure aircraft** and **cockpit cloud z-fighting** both FIXED;
> quick-mission fleet ASan-clean; **full-campaign epic** scoped + Phase-1 started.

---

## What works (verified)

| Subsystem | State | How to see it |
|-----------|-------|---------------|
| **Build / link** | All 16 game modules + MFC compile and link to a 7.8 MB ELF; default `./bob` exits 0 | `cd build && ninja bob` |
| **3D flight** | Cockpit + daylit terrain on real GL | `BOB_BOOT_FRONTEND=1` |
| **Landscape (RTT)** | Green ground via FBO render-to-texture; **default-on** (A/B 51%→99% non-black) | default; `BOB_NO_FBO_RTT` reverts |
| **Textures** | FULL_RES 256×256 land RT (4× detail) | default; `BOB_TEXQ`/`BOB_FILTER` |
| **Clouds** | Fluffy clouds — sky matches the Windows reference | default; `BOB_NOCLOUDS` reverts |
| **Audio** | DirectSound→OpenAL: looping engine + one-shot effects play (10 sources) | default; `BOB_NOSOUND`, `BOB_TRACE_SND` |
| **Keyboard flight input** | SDL→DIK→DirectInput→`OnKeyInput`→flight commands (event-driven) | proven via `BOB_AUTOFLY=throttle` |
| **Joystick flight input** | DirectInput→SDL_Joystick (enumerate/caps/objects/buffered `GetDeviceData`); default axis map (aileron/elevator/rudder/throttle). **PO fly-test passed.** | connect a stick; `BOB_TRACE_JOY` |
| **In-flight mouse** | DirectInput→SDL relative motion → the in-3D UI cursor (`AU_UI_X/Y`) | default; `BOB_MOUSEFLY`, `BOB_NOMOUSE` |
| **HUD info bar** | Altitude/speed + attitude indicator (after the unit-factor fix) | `BOB_HUD` |
| **Front-end** | Navigable menu + config screens with real hosted R\* OLE controls — **7 types**: listbox/combo/static/button/edit/radio/**edit-button** (`CREdtBt`, S140), combo cycle-on-click, RLE8 backgrounds | `BOB_FRONTEND=1 BOB_OLE_DRAW=1` |
| **Mission briefing** | The BoBFrag briefing (gold #3) renders CLOSE: roster listbox, "Return to Player", the "Bob" pilot name box, Back/Sim Config/Fly footer | `BOB_BOBFRAG=1 BOB_SHOT=120` |
| **Screen parity** | **18 CLOSE / 0 PARTIAL / 1 GAP** of 19 gold shots (`doc/screen-parity.md`); S141 closed #18 (Directives grid — the Eagle-Attack state is now reachable), leaving only #4, the transient loading screen, as a **by-design** GAP | `doc/parity/`, `BOB_SHOT` harness |
| **Config screens** | All 6 options tabs render as readable forms (GFX/More GFX/Controls/Sound/2D/Sim); **Controls** screen is interactively re-bindable (click a device/axis combo → reassign) | `BOB_CONFIGSCREEN=controls\|gfx\|sound\|2d\|sim` |
| **OCX event routing** | **General eventsink (S33):** the game's own `BEGIN_EVENTSINK_MAP`/`ON_EVENT` maps drive control events via RTTI dispatch — combo TextChanged + listbox Select fire the genuine handlers (replaces the two targeted R5.3b/R4.4 bridges) | `BOB_TRACE_OLE` |
| **Text rendering** | Game-wide `CSprintf("%s",CString)` fixed (Itanium-ABI varargs) — config/debrief/controls text readable | (always on; `cstring_impl.cpp::FormatV`) |
| **Load-game screen** | `CLoad` reaches + renders (RAF/LW/Back/Load + hosted file-list); save/load gated on the campaign | `BOB_CONFIGSCREEN=load` |
| **Strategic map** | Campaign map renders terrain (SE England/Channel/France, sector labels) **+ the unit-icon layer** (RAF squadron/airfield/fighter + LW raid markers) — matches the Wine gold ref. Toolbars + scroll/zoom/click still to do. | enter the campaign map (e.g. load a save); `BOB_TRACE_ICONS` |
| **Campaign sim (post-load)** | A loaded campaign's strategic-day sim **advances without crashing** (clock + raids progress) — the post-load garbage-UID crash family is retired (`ConvertPtrUID` honors its bounds contract). Stale loaded refs resolve to NULL (minor target-reacquire fidelity gap, banked); post-*mission* sim path still WIP. | `BOB_POSTLOAD_FF` + `BOB_MAP_TIMER` |
| **Menu↔flight (one process)** | **R1.1b complete:** menu→flight via the game's own `StartFlying→Rtestsh1→Launch3d` (forced + real menu clicks) AND flight→menu (F12→`CloseWindow`→`OnFlyingClosed`→rendered front-end screen); clean 3D-device teardown (DD7 refcount fix). Full menu→fly→menu, one window. | `BOB_FRONTEND=1 BOB_OLE_DRAW=1 BOB_STARTFLYING=1` (`=click` + `BOB_AUTOCLICK=0,1,2`; `BOB_AUTOQUIT=<s>` to exit) |

Cockpit render now closely matches `doc/reference/cockpit-windows-spitfire-1.png`.

---

## ★ The core gap to "complete" — CLOSED (Definition of Done met, 2026-06-17)

The three gaps below are all **closed**. Bare **`./bob`** launched from the game's install dir now
**boots to the real Battle of Britain title screen with no `BOB_*` env vars** (derives the data path
from cwd), and a mouse+keyboard playthrough flies a full Quick Mission and returns to the menu —
menu → mission → fly → debrief → next, the game's own flow, one GL window. (Minus music — env-blocked
32-bit synth — and multiplayer — out of scope.) See PORT.md (newest entries, Sprints 4–7).

1. **One window for 2D + 3D — DONE (R1.1b).** The menu↔flight control-flow merge: the game's own
   `StartFlying→Rtestsh1→Launch3d` (entry) + F12/Alt+X→`OnFlyingClosed` (return), one process/window.
   Fixed the 3D-device teardown (DD7 refcount COM bug) so flight cycles cleanly.
2. **Real initialization — DONE (R1.4).** `SaveData::InitPreferences()` is the default init (the four
   combat-corruption bugs that blocked it are fixed); device init now also runs at the front-end boot.
3. **Real mission loop — DONE (R2.x).** The real menu Fly drives `LoadSetPiece`; Alt+X→debrief; the
   debrief→next chain runs (campaign continuity); stress + variety validated, 0 crashes.

---

## Roadmap

- **Phase 1 — Unify the window** (front-end on the same real-GL window as flight). High value, medium risk.
- **Phase 2 — Real init via `InitPreferences()`.** *Mechanism proven; blocked on the heap-corruption bug below.*
- **Phase 3 — Real mission loop** (menu "Fly" → `LoadSetPiece` → mission-end → debrief). Highest value/risk;
  expect a grind of latent uninitialized-state bugs (same pattern as the fixes already landed).
- **Phase 4 — joystick (`DI_EnumDevices`, needs hardware), intro Smacker video, save/load round-trip.**
- **Phase 5 — polish:** front-end blit subsystem (combo box-art/arrows), DPI/fonts; deferred render
  issues (mirror horizon UVs, trilinear mipmaps, terrain detail combiner).

---

## Blockers / next concrete task

### Gating bug (Phase 2/3): heap corruption in the combat sim — **RESOLVED (2026-06-17), InitPreferences is now the default init**
**Update (Scrum Sprints 2–3, see PORT.md):** the "non-deterministic corruption" was four distinct bugs,
all now fixed + ASan-verified: the `SetPilotedAcAnim` new[]/delete mismatch (R1.3a), the `Reg3dConv` OOB
write (R1.3b), the trilinear loader-SEGV (pinned bilinear, R1.3c), and the transient **double-free** in
`RemoveDeadListFromWorld` — `TransientItem::operator delete` re-ran the destructor → anim buffer freed
twice; fixed to `::operator delete(obj)` (R1.3d). **`SaveData::InitPreferences()` is now the DEFAULT init**
(R1.4): the env-gated `Save_Data` hacks are retired; the game initialises itself the way FULLPANE does.
Verified (R1.5): faithful flight (sky/terrain/cockpit, OpenAL engine loop + effects, HUD) with no feature
env vars, 90s clean. **Release 1 shipped.** The historical description below is retained for context.

### (historical) non-deterministic heap corruption in the combat sim
- `SaveData::InitPreferences()` (SAVEGAME.CPP:2279) is the real factory-defaults + `settings.cfg` +
  device-init function the QM boot skips (it bypasses FULLPANE). Wiring it in set all real defaults
  (volumes/`textureQuality=3`/HUD/units) and sound played — **the mechanism is correct.**
- But the resulting full-combat sim corrupts the heap: **non-deterministic** crashes — sometimes
  SIGABRT (`double free` in `TransObj::RemoveDeadListFromWorld`), sometimes SIGSEGV with *zero*
  transient deletes. = a heap/struct overrun, prime suspect a `-fpack-struct=1` layout overrun (same
  class as the audio `PCMWAVEFORMAT`-into-`WAVEFORMATEX` overrun already fixed).
- **Next:** get a memory tool onto the `InitPreferences` combat path — valgrind memcheck (slow; the
  GL flight resists it, may need a software-GL/`SDL_VIDEODRIVER=dummy` fallback), or build the suspect
  combat/transient TUs for ASan — to catch the first invalid write, fix it, then re-land
  `InitPreferences` as the default init and retire the per-feature `BOB_*` Save_Data forces.

### Hard blocker (environment): MIDI music
No 32-bit fluidsynth, no system soft-synth, proprietary `.DIR` music archive. Needs an **environment
change** (install 32-bit fluidsynth or run a soft-synth to target via 32-bit ALSA seq), then implement
`midiOut*` → that synth. Sound effects + engine work; only music is blocked.

### Out of scope
DirectPlay multiplayer (large, low priority).

---

## Definition of done — ✅ MET (2026-06-17)
Game boots to its real menu on one GL window; you start and complete a mission through the game's own
flow with **no `BOB_*` env vars**, with terrain/clouds/cockpit/sound/keyboard all live — i.e. it plays
the way it did on Windows, minus music (env-blocked) and multiplayer (out of scope). **Achieved:** bare
`./bob` from the install dir boots the real title screen (data path derived from cwd) and a mouse+keyboard
playthrough flies a full Quick Mission → debrief → menu. Run: `cd "<drive_c>/Program Files/Rowan
Software/Battle Of Britain" && /path/to/bob`. (`BOB_NO_RUN` = link-only safe default; `BOB_DRIVE_C`
overrides the data path.) Beyond DoD: R3 polish/peripherals (joystick, save/load, Smacker, render fidelity).
