# R25 — the struck "S" on the Controls screen: analysis, and the instrument to use next

**Written 2026-09-01 with no shell available** (`/tmp` quota exhausted, every Bash command
returns exit 1). Nothing here is measured this sprint; it is a reading of the code plus the
measurements already banked in STATUS. It exists so the next sprint starts from a named
hypothesis and the right instrument, instead of repeating pixel bisection.

## What is already established (measured, S422)

* The difference is **81 px, one glyph**: the "S" of "Small" at x 552–577, y 488–501, drawn with a
  stray vertical stroke in most runs and cleanly in others. Both variants spell "Small".
* **The committed reference holds the struck version.** So the screen is usually wrong and
  occasionally right — a rendering defect, not gate noise.
* Ruled out by measurement: the MFC timer (same rate with `BOB_TIMERS` unset); screen order
  (byte-identical 4/4 after the real suite prefix, 16 consecutive clean runs in a reused tree);
  seeded settings state (five fresh trees, identical `settings.cfg`, one still differed);
  an uninitialised **glyph** buffer (`malloc`→`calloc` changed nothing — same 1-in-6).

## Why "intermittent with identical inputs" is the whole clue

Same binary, same `settings.cfg`, same screen, same recipe — and the output differs about one run
in six. Nothing in the *inputs* varies, so the variable is **process state**: heap contents, or a
read of memory that was never written. That is this port's documented bug class (uninitialised
reads fed by Win32 stubs), and it is why the glyph-buffer theory was attractive. It was the right
*class* and the wrong *object*.

## The hypothesis worth testing

The stray stroke sits hard against the combo's **left border**, and the value text starts there.
Two candidate objects, both upstream of the rasteriser:

1. **An uninitialised member of the hosted `CRComboCtrl`.** `HostRCombo::applyDesignProps`
   (`SRC/RCOMBO/bob_ole_rcombo.cpp`) replays a persisted property stream and **returns early if the
   dialog has no property bag** — leaving whatever `OnResetState()` and the constructor left behind.
   `m_FontNum`, `m_Style` and `m_ListboxLength` come from that stream. A style or inset member read
   uninitialised would select a marginally different draw path per run.
2. **`m_FirstSweep` forced TRUE on every draw** (`HostRCombo::draw`, deliberate — the genuine code
   otherwise switches to an offscreen DC the GDI compat cannot provide). If the genuine `OnDraw`
   paints a frame or the value *twice* on a first sweep, every repaint takes that path. On its own
   this is deterministic and cannot explain intermittency — but combined with (1) it would decide
   *where* the second paint lands.

## The instrument to use — not more pixel bisection

Pixel diffing has now cost two sprints and eliminated four theories without naming the object.
The defect is an uninitialised read; **use a tool that finds uninitialised reads directly**:

* Run the Controls-screen capture under **valgrind memcheck** (or an MSan/ASan build) and look for
  a conditional jump or a move depending on uninitialised memory inside the combo's `OnDraw`.
  One run either names the member or rules the class out.
* Because the flake needs several runs to show, run the capture in a loop under the tool and keep
  the first report — do not require the *pixels* to differ, since memcheck flags the read whether
  or not the garbage happened to change the output that run.

Do **not** re-seed the reference to the clean variant first. The clean render is the rare one; a
reference re-seeded to it would go red five times in six, and re-seeding an unexplained difference
is the failure `bob_parity.sh` exists to prevent.

## Cleanup owed first (caused by R25's own bisection)

`tools/bob_use_scratch.sh` builds a scratch game tree per invocation and **never removes it**, and
`bob_scratch_gamedir.sh` copies `VIDEOS` wholesale when only `DIR.DIR` and `Bob.cam` are needed.
Running it in loops exhausted the `/tmp` quota and broke every shell command. Two fixes owed:

1. a cleanup trap (or a single reused tree keyed by the caller) so gates stop accumulating copies;
2. copy from `VIDEOS` only what R1 actually needs.

This matters beyond tidiness: S378 records that when `/tmp` last hit its quota, a gate's backup came
out empty and the restore wrote a 0-byte file over the player's campaign save.
