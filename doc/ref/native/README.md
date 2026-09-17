
## RE-SEEDED 2026-09-16 — the font-metric flag was turned on by PO decision

`BOB_FONT_EM` is now **default-on** (`BOB_NO_FONT_EM=1` reverts), so **all eight references here
were re-taken on this date.**

Measured against the 2026-09-15 gold before re-seeding: the side-select caption goes
**24 px → 28 px** against the gold's **28**, ink 1824 vs 1825, width 157 vs 156, left edge 520 vs
522. The flag multiplies glyph size by `hhea/em`, which is **1.171** for this port's
`g101016_.ttf`; checked on `config-gfx`'s `3D Resolution` label, **h 12 → 14, ratio 1.167** against
the predicted 1.171.

⚠️ `mainmenu` was **byte-identical before and after** — the title menu draws through
`bob_draw_menu`'s own pixel height (`resW*36/1000`) and never reaches the em/cell conversion. That
is a useful negative control: the flag moves the R\* control text and nothing else.

⚠️ Regression oracles, not gold ones — see the header of `tools/bob_parity.sh`.
