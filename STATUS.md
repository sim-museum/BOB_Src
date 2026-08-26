# Battle of Britain — Linux port status

Last updated: 2026-08-26 (sprint 303)

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
| PO-73 | Grey elliptical blob floating in the cockpit view | Reproduced and captured. Leading suspect: a draw whose texture reaches draw time with `glTex=0` (never uploaded → renders untextured → grey). Make suspect and evidence come from the SAME frame, then confirm. |
| — | `upload_texture` may be failing silently | Any texture arriving at draw time with `glTex=0` got past `if (t->texDirty \|\| !t->glTex) upload_texture(t)`. A defect in its own right, whether or not it is PO-73. |
| R1 | Recording chain proven in two halves | The UI→preference and preference→recording links were measured in SEPARATE runs (committing the setting writes no file, so it cannot be staged). One continuous UI→flight→recording session would close it. |
| — | Combat is never reached in any recipe | Known since S206: the flight consumes the wall clock the raid needs. The ACM tree remains untested code. |

## Instruments

`BOB_DUMP_FRAME=N` (**3D frame grab — use this, it predates and works**) · `BOB_TRACE_PRESENT` ·
`BOB_TRACE_BLOB` + `BOB_BLOB_HILITE` + `BOB_BLOB_TEX=<glTex>` (find/paint a suspect draw) ·
`BOB_TRACE_PROJ` · `BOB_DUMP_HITTARGETS` (dump menu + control rects; never guess pixels) ·
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
