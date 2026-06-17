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
| **HUD info bar** | Altitude/speed + attitude indicator (after the unit-factor fix) | `BOB_HUD` |
| **Front-end** | Navigable menu + config screens with real hosted R\* OLE controls (combo/static/listbox), combo cycle-on-click, RLE8 backgrounds | `BOB_FRONTEND=1 BOB_OLE_DRAW=1` |

Cockpit render now closely matches `doc/reference/cockpit-windows-spitfire-1.png`.

---

## The core gap to "complete"

The above are **separate env-gated bring-up scaffolds**, not the real game flow. Completing
the port = collapsing them into the game's own end-to-end loop:

1. **One window for 2D + 3D.** Front-end runs on the bob_gdi CPU framebuffer (presents via a GL
   quad); flight runs on real GL. They were never unified into one continuous app.
2. **Real initialization.** ~85 `BOB_*` env gates force state the QM boot leaves at 0. The fix is
   the game's own `SaveData::InitPreferences()` (see below) — **mechanism proven, blocked on one bug.**
3. **Real mission loop.** `BOB_BOOT_FRONTEND` *synthesizes* a scramble; the real menu → mission →
   fly → debrief → campaign flow isn't driven end-to-end.

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

### Gating bug (Phase 2/3): non-deterministic heap corruption in the combat sim
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

## Definition of done
Game boots to its real menu on one GL window; you start and complete a mission through the game's own
flow with **no `BOB_*` env vars**, with terrain/clouds/cockpit/sound/keyboard all live — i.e. it plays
the way it did on Windows, minus music (env-blocked) and multiplayer (out of scope).
