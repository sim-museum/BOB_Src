# Session Status — 2026-07-06

Snapshot of the current working state. Authoritative running log is `PORT.md` (newest first).

## Headline
Both Product-Owner backlog items are **addressed**:
1. **Z-fighting** — cockpit fixed (S81); **external F6 view fixed (S119)** via default 3D scene depth-sort.
2. **Full campaign** — strategic map + interaction + multi-day loop + OOB dialogs (S83–S117), and a
   **faithful day-advance (S120)**.

## Committed (through S119) — in the BoB repo
- **S113–S117 — OOB info dialogs render.** Clicking a map toolbar button opens + shows its dialog over the
  map (Bases = RAF Order of Battle via hosted RListBox/RStatic; Squadrons table). Default-on, generalised
  across dialog structures (tabbed `HTabBox` and single-leaf), ASan-clean. `BOB_NO_OOB` reverts.
- **S118–S119 — F6 external-view z-fighting FIXED.** 3D scene depth-sorting (`BOB_ZDEPTH`) is now **default**
  (`BOB_NO_ZDEPTH` reverts). The external Spitfire renders with proper camo/roundels instead of the
  washed-out painter's-order self-occlusion. Cockpit unchanged; horizon clean; ASan-clean on real GL
  (GTX 1660). PO approved ship-and-field-test (watch the spinning propeller; revert via `BOB_NO_ZDEPTH` if
  the lower blade glitches — the R3.2 regression did not reproduce across 6 rotations captured).
- Cross-port: MA notes **7 (`FROM-BOB-05g`)** and **8 (`FROM-BOB-05h`)** committed in `~/ma`.

## PENDING COMMIT — done + verified, waiting on the shell (see Impediment)
- **S120 — faithful day-advance (`BOB_DAYADV`).** Replaces the `BOB_DAYLOOP` scaffold heuristic with the
  game's own review→map return. At dusk `EndOfDay` → `LaunchFullPane(enddayreview)` sets `m_currentpage=1`;
  the scaffold now drives `RFullPanelDial::ReturnToMapAfterReview` (`StartUpMapWorld` + `LaunchMap` →
  `m_currentpage=0`) — the exact CONTINUE-button path. **Verified:** campaign cycles day after day off the
  actual dusk event (5 day-advances, currdate 1247270400→1247616000, world repopulated ~1080–1136 each day).
  Also learned `ReturnToMapAfterReview` omits `BuildTargetTable`/`StartOfDay`, so `BOB_DAYLOOP`'s extra calls
  were unnecessary.
  - Files: `SRC/MFC/FULLPSYS.CPP` (`BOB_DAYADV` hook + `BOB_TRACE_DAYEND` map-drive trace),
    `SRC/MFC/MAINTBAR.CPP` (`BOB_TRACE_DAYEND` on the enddayreview launch), `PORT.md` (S120 entry —
    finish the exit-code/ASan validation line after committing).
- **Note 9 to MA** — `~/ma/port/CROSS-PORT-FROM-BOB-2026-07-06.md` (S116/S117/S120 cross-port updates:
  single-leaf OOB dialog renders → narrows their NULL-`fchild` triage to base `MakeTopDialog`/`AddChildren`;
  the faithful day-advance / `m_currentpage` seam). On disk, uncommitted.

## Remaining (documented, non-blocking)
- Propeller field-verification under default `BOB_ZDEPTH` (PO's live-flight check).
- OOB cosmetic polish: selected-tab-only, faithful placement, tab-click.

## IMPEDIMENT (blocking commit + push)
The shell cannot `fork()` — every command (`echo`/`git`/`true`) exits 1 immediately. Cause: stray real-GL
`bob` test processes from the F6 flight + day-advance runs exhausted the process table (they ignored SIGTERM
on `timeout`; likely wedged in uninterruptible GPU I/O). `pkill -9 -f build/bob` did not restore forking.
TaskList is clear; all background tasks stopped. `pkill` can't be run from here (it needs to fork).

**To unblock:** clear the stray processes from an interactive shell, e.g.
`pkill -9 -f BOB_DRIVE_C ; pkill -9 -f "Battle Of Britain" ; pkill -9 timeout`
(all match only the test commands launched this session). If they're D-state and won't die, the environment
may need a reset. Once a shell command returns, commit S120 + note 9 and push.
