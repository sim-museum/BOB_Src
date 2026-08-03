# Battle of Britain — Linux Port: Status

Snapshot of where the native Linux port stands. Running engineering log (newest-first,
with evidence) is in **PORT.md**; this file is the high-level state + roadmap.

Branch: `linux-port` · Build: 32-bit i386 ELF (`gcc -m32`), SDL2 + OpenGL + OpenAL,
`-fpack-struct=1`. Game sources stay unedited; the port lives in `SRC/compat/` + the
`BOB_*` env-gated boot scaffolds.

> **Latest sessions (S132–S140, 2026-08-02/03): the gold #3 briefing arc → CLOSE, + the Directives
> dialog reached.** Screen parity is now **17 CLOSE / 1 PARTIAL / 1 GAP** of 19 (was 16/1/3). The
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
| **Screen parity** | **17 CLOSE / 1 PARTIAL / 1 GAP** of 19 gold shots (`doc/screen-parity.md`); #18 Directives PARTIAL (grid needs Eagle-Attack state), #4 loading screen by-design GAP | `doc/parity/`, `BOB_SHOT` harness |
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
