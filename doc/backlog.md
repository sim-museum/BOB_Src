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
| R1 | As a player, I can record a flight in BoB and replay it. | 13 | Fly, exit, open the replay and watch the flight back; the recording round-trips (`header + frames == EOF` exactly, the arithmetic MA's `replay_record.sh` asserts). | 🔨 **NEW — do this FIRST.** BoB has the same `Replay` class as MA (`SRC/COMMS/REPLAY.CPP`, same `ReplayRead`/`BackupSmokeInfo`/`LoadBlockHeader` shape), and **S241 already fixed the 32-bit `ReplayRead` overflow here pre-emptively**, plus a second copy of the unchecked-size pattern in `BackupCloudInfo` that MA does not have. Unknown: whether BoB's record/playback path is reached at all in this port. **First step is a reach check, not a fix** — establish whether a flight records anything before assuming what is broken. |
| R2 | Saving a replay also writes a Tacview `.acmi`. | 8 | `.acmi` appears beside the `.cam`; the `.cam` is **byte-identical** to what the same save produced before. | 🔨 **NEW — blocked on R1.** Nothing should be built on a replay path that does not work; MA learned this the expensive way (EPIC L's L1 was blocked on PO-68 for exactly this reason). **`SRC/compat/ma_acmi.cpp` should port nearly unchanged** — it deliberately takes plain C types and knows nothing about the game's structures, so only the tee's field names differ. |
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
