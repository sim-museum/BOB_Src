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
| Per-gold-shot parity verdicts | `doc/screen-parity.md` — currently 16 CLOSE / 0 PARTIAL / 3 GAP of 19 |
| Side-by-side captures | `doc/parity/` |
| Engineering evidence log (newest first) | `PORT.md` |
| Live product snapshot | `STATUS.md` |
| History | `git log --oneline` on `linux-port` |

Gold standard: `/run/media/admin/BEA6-BBCE/bob/` (19 PNGs). Oracle ruling: the
gold shots as-is = the BDG 0.99 patched build (dialogs/strings read from
`boblang.dll` PE resources since S124; `BOB_NO_PE_RSRC=1` reverts).

## Current state (2026-08-02)

- S126 closed (`6ab411d`): property-stream reader (`9105e25`) capture-proven —
  14-recipe sweep 14/14, 13 screens moved toward gold, #16 duplicate-date fixed
  (covered-static settled-state erase) → #16 CLOSE.
- **S127 closed: label-render fidelity.** `CDC::DrawText` now implements real
  DT_WORDBREAK word-wrap (phase-select #16 + QS #2 descriptions wrap in their
  boxes; ≥2-line-box guard keeps config labels single-line — no regression) and
  '&' accelerator escape ("Cockpit && UI"→"Cockpit & UI", #8). `BOB_NO_WORDWRAP`
  / `BOB_NO_AMP_ESCAPE` revert. → **#16 PARTIAL→CLOSE, #8 CLOSE, #2 improved**
  (parity 16 CLOSE / 1 PARTIAL / 3 GAP of 19).
- **S128 closed: hosted the `CRRadioCtrl`** (6th R\* control type) → the
  Quick-Shots page-tab row (Scenario/Parameters/Luftwaffe/RAF, `IDC_RRADIO`)
  now renders each caption + its selection-tick icon. **#2 PARTIAL→CLOSE**
  (parity **16 CLOSE / 0 PARTIAL / 3 GAP** of 19). #3 (Parameters page)
  prerequisite met; remaining half = tab-click page-switch + `MoveWindow`.
- GL gates PASS (S127 + S128): safe default `./bob` exit 0; flight frame-150
  95.2% non-black on `:0`; dummy==GL `cmp` byte-identical on mainmenu,
  phaseselect (S127) and quickshots (S128).
- Cross-port: MA note 17's `DrawText DT_WORDBREAK` shared find implemented
  BoB-side (S127) — outbound BoB note = shared-doc §8o; new-R\*-control-type
  hosting checklist + MaskIcon temp-bind = §8p (S128). MA copy synced
  byte-identical. Inbound MA notes 18–25 (§8g–8n) logged.
