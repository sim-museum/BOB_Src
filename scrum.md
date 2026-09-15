

## R3.2 S4 (Opus 5, 2026-09-15) — ⭐⭐ the negative result is EXPLAINED, and it is geometric: **this flight is 2,700 ft below the lowest cloud in the game**

S3 scored one airborne frame and found no cloud over the cockpit. S4 does it across a whole sortie —
and then asks the question three sprints of negatives should have asked first: **could this recipe
ever have put the cockpit in a cloud?**

**The recipe is a script now.** `tools/bob_realgl_flight.sh` is S3's reconstruction written down: the
campaign drive from `bob_combat_soak.sh` minus `SDL_VIDEODRIVER=dummy`, on `DISPLAY=:0`, dumping
every Nth 3D frame. Three items (R3.2, R3.7, R3.9) depend on this path and it existed only as prose.

**And the run can now SAY it got airborne.** New `BOB_TRACE_HUD` prints the numbers
`COverlay::DrawTopText` is about to draw. S3 had to read `Alt 4854ft` off a dumped frame by eye,
which no script can do. This run: **295 samples, 4,386-5,332 ft, 152-182 kt, 59 frames.**

**SCORED** (`tools/bob_cloud_score.py`): cloud-coloured (bright, near-neutral) pixels inside a
cockpit-structure mask built from the sequence itself.

| | |
|---|---|
| frames | 59 |
| cockpit mask | 659,886 px (31.8% of frame) |
| cloud px in the mask, 50 of 59 frames | **0** |
| the other 9 (all in the first 66 s) | 6 - 726, decaying monotonically as the light changes |

⚠️ **The mask rule had to be written so the defect could not erase itself.** "Dark in frame 0 AND
unchanged in every frame" would drop exactly the pixels a passing cloud lit up. The rule is *dark in
70% of frames*, which survives a cloud crossing. (Same family as the julia-racer census that counted
the list built before the rule it was measuring.)

⭐⭐ **WHY EVERY ONE OF THESE RUNS WAS ALWAYS GOING TO SAY NO.** The clouds are not where the
aeroplane is:

| | altitude | source |
|---|---|---|
| fluffy cloud sprites | **8,000 ft + noise** (0-8,566 ft more) | `LANDSCAP.CPP:9276` - `dummyitem.World.Y = FT_8000 + newY`, `newY = Noise(...)<<10`, `Noise` returns a `UByte` |
| stratus layer | **15,000 ft** | `MISSINIT.CPP:1506` `CloudLayer = 457200`; `SKY.H:72` - *"bottom layer in cm"* |
| **this flight, best case** | **5,332 ft** | `[hud]` |

**The campaign flight cruises 2,668 ft below the lowest cloud the game can place.** S1's parked
frame, S3's single airborne frame and S4's 59 are all measurements taken *under the weather*. None of
them is evidence about the PO's report, and the item must stop collecting them: **R3.2 needs a flight
at >= 8,000 ft**, which this recipe does not fly (the campaign sortie is a low-level convoy strike).

⭐ **UNEXPECTED, AND MEASURED: a grey ellipse pinned to the screen for the entire flight.** In every
frame there is a flat grey ellipse at **x 49-312, y 141-189** - *the same bounding box in frames
7,200, 20,400 and 35,400*, i.e. across 28,000 frames and ~8 minutes of flight at 180 kt. Scenery
cannot be static in screen space. **This is a candidate for R3.9 / PO-73 ("a grey box near the
cockpit")**, which has never had a reproduction; capture at
`parity/r32_s4_clouds_above_cockpit_260915.png`. Not chased here - R3.2 is at its cap - but it is
the first time that shape has been caught in a frame this project can measure.

**S5 (R3.9's, not R3.2's):** identify what draws the grey ellipse. It is static, ~263x48 px, upper
left, and survives a whole sortie - start from the draw order in that band rather than from the
report's word "drifting".

**R3.2: 4 sprints this pass - AT THE CAP. The item is not disproved; it has never been tested, and
now the reason is a number.**
