# Battle of Britain — Run & Check Progress

## Run the game

```bash
cd "/home/admin/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 /home/admin/bob/build/bob
```

That is the real front-end (menus, config screens, campaign strategic map, fly).
Alternative: `BOB_BOOT_FRONTEND=1 /home/admin/bob/build/bob` jumps straight into
a Quick-Mission cockpit. Requires a healthy GL display session.

Rebuild: `cd /home/admin/bob/build && ninja bob`.

## Check progress

| What | Where |
|---|---|
| Backlog (Releases R1–R7 + SP) + burndown | `scrum.md` — Release SP is the screen-parity epic; §9 burndown |
| Per-gold-shot parity verdicts | `doc/screen-parity.md` — currently 14 CLOSE / 2 PARTIAL / 3 GAP of 19 |
| Side-by-side captures | `doc/parity/` |
| Engineering evidence log (newest first) | `PORT.md` |
| Live product snapshot | `STATUS.md` |
| History | `git log --oneline` on `linux-port` |

Gold standard: `/run/media/admin/BEA6-BBCE/bob/` (19 PNGs). Oracle ruling: the
gold shots as-is = the BDG 0.99 patched build (dialogs/strings read from
`boblang.dll` PE resources since S124; `BOB_NO_PE_RSRC=1` reverts).

## Current state (2026-07-27)

- S124 closed (`6f82c00`): PE resource layer, all 8 config tabs CLOSE vs gold.
- S125 closed (`533aba9`): enter-name CLOSE, phase-select column spread fixed.
- S126 closed (`6ab411d`): property-stream reader (`9105e25`) capture-proven —
  14-recipe sweep 14/14, 13 screens moved toward gold, #16 duplicate-date fixed
  (covered-static settled-state erase; `BOB_NO_COVER_ERASE` reverts) → #16 CLOSE.
- GL gates RESTORED (GLX wedge healed machine-wide): default-run exit-0 and the
  flight frame-150 gate both pass. New standing acceptance bar: SDL-dummy
  `BOB_SHOT` capture must be byte-identical (`cmp`) to the real-GL capture.
- Cross-port: MA note 17 inbound (`3bf5cc8`) — template-visibility routing
  checklist (parent-rect clipping + `!WS_VISIBLE` initial state), DrawText
  `DT_WORDBREAK`, device-presence determinism. Not yet processed by a BoB sprint.
