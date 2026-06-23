# Battle of Britain — Linux Port: Status

Snapshot of where the native Linux port stands. Running engineering log (newest-first,
with evidence) is in **PORT.md**; this file is the high-level state + roadmap.

Branch: `linux-port` · Build: 32-bit i386 ELF (`gcc -m32`), SDL2 + OpenGL + OpenAL,
`-fpack-struct=1`. Game sources stay unedited; the port lives in `SRC/compat/` + the
`BOB_*` env-gated boot scaffolds.

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
| **Front-end** | Navigable menu + config screens with real hosted R\* OLE controls (combo/static/listbox), combo cycle-on-click, RLE8 backgrounds | `BOB_FRONTEND=1 BOB_OLE_DRAW=1` |
| **Config screens** | All 6 options tabs render as readable forms (GFX/More GFX/Controls/Sound/2D/Sim); **Controls** screen is interactively re-bindable (click a device/axis combo → reassign) | `BOB_CONFIGSCREEN=controls\|gfx\|sound\|2d\|sim` |
| **Text rendering** | Game-wide `CSprintf("%s",CString)` fixed (Itanium-ABI varargs) — config/debrief/controls text readable | (always on; `cstring_impl.cpp::FormatV`) |
| **Load-game screen** | `CLoad` reaches + renders (RAF/LW/Back/Load + hosted file-list); save/load gated on the campaign | `BOB_CONFIGSCREEN=load` |
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
