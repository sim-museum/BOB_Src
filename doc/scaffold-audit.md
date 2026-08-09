# Scaffold audit — which capabilities are actually proven?

*Started S157 (2026-08-09), from S156's retro. Live document.*

S156 found that the map's OOB dialogs had been **render-only since S113** — openable, paintable,
never clickable — and that nothing had ever failed, because the only thing that drove them
(`bob_oob_accept_directives`) called `bob_evt_fire` **directly on the dialog**, entering below the
missing layer. Forty-plus sprints of evidence about those dialogs said nothing about whether a
click could reach one.

This document exists so that class of gap is found by audit rather than by luck.

---

## 1. The distinction that matters

Most scaffolds are harmless. Sort them by **what they substitute**:

| | substitutes | proves the real path? |
|---|---|---|
| **Shallow** | an **input** — synthesizes a coordinate/keystroke, then falls into the same dispatch a real event reaches | **Yes** — everything downstream is production code |
| **Deep** | a **call** — invokes a handler, fires an event, pokes a control | **No** — enters below one or more layers and cannot report that they are missing |

**The trap:** the deeper the scaffold, the *more impressive* the evidence it produces, because it is
driving the working part of the system directly. S144–S146 used a deep scaffold to build, fly and
land whole raids. That felt like overwhelming proof the Directives dialog worked. It was proof that
everything *below the dispatch* worked.

**The audit question** is therefore not "is there a test?" but:

> For each capability, list its drivers and classify each. **If every driver is deep, the capability
> is unproven** — however rich the evidence downstream looks.

---

## 2. The click path, layer by layer

The full chain for a mouse click, and where each driver enters:

```
  [physical mouse / X server]
        v
  (1) SDL_MOUSEBUTTONDOWN in pump_events()   <- logical->drawable scaling, g_clickPending
        v
  (2) bob_gdi_get_click()                    <- BOB_CLICKXY enters here
        v
  (3) front-end: `if (got)`  |  map: `if (haveClick)`   <- BOB_AUTOCLICK / BOB_MAP_CLICK enter here
        v
  (4) hit-test -> bob_ole_click / bob_map_click_oob / toolbars / unit select
        v
  (5) handler: bob_evt_fire(...)             <- bob_oob_accept_directives enters HERE (deep)
```

| driver | enters at | class | proves |
|---|---|---|---|
| `BOB_SDL_CLICK` / `BOB_MAP_SDLCLICK` (**S157, new**) | **before (1)** — pushes a real `SDL_MOUSEBUTTONDOWN` | shallow | everything but the physical mouse |
| `BOB_CLICKXY` | (2) | shallow | 2→5 |
| `BOB_AUTOCLICK` | (3) | shallow | 3→5 |
| `BOB_MAP_CLICK` | (3) | shallow | 3→5 |
| `bob_oob_accept_directives` (`BOB_MAP_ACCEPTDIR`) | (5) | **deep** | 5 only |

Before S157 **every** driver entered at (2) or later, so layer (1) — the SDL event handler and its
coordinate scaling — had never been executed by any test.

**And there is a structural reason for that, measured in S157.** Under `SDL_VIDEODRIVER=dummy` —
which is what the entire gate suite and every headless capture recipe use — `SDL_CreateWindow`
fails (*"OpenGL support is either not configured in SDL or not available in current SDL video
driver (dummy)"*), so no window exists, none of the present / `BeginScene` / `SwapWindow` paths run,
and **`pump_events` is never called even once**. Verified directly: a trace on the first, 100th and
10 000th `pump_events` call printed nothing at all in a full headless run.

Consequences, stated precisely:

- The headless harness **cannot** exercise layer (1). Every driver entering at (2) or later was a
  necessity, not an oversight.
- This is **not** evidence that real mouse input is broken. It is evidence that no headless test can
  say anything either way about layer (1).
- Therefore `BOB_SDL_CLICK` / `BOB_MAP_SDLCLICK` are only meaningful on a **real GL display**, where
  a window exists and the pump runs. That is the one configuration a player actually uses, and it is
  the configuration in which layer (1) has to be proven.

**Layer (1) is now proven — on real GL, S157.** Same recipe without `SDL_VIDEODRIVER=dummy`:

```
[clickpath] 0. pump_events call #0                          <- the pump runs
[clickpath] 0. pump_events call #100
[sdlclick]     queued real SDL_MOUSEBUTTONDOWN fb=(712,499) rc=1
[clickpath] 1. SDL event POLLED logical=(712,499) -> fb=(712,499)    <- layer (1)
[clickpath] 2. bob_gdi_get_click CONSUMED (712,499)                  <- layer (2)
[oobclick]     (712,499) consumed by toolbar 3 child 6 ctrl id=2600  <- layers (3)(4)(5)
```

Every layer from a genuine `SDL_MOUSEBUTTONDOWN` to the game handler, in one run, with nothing
bypassed but the physical mouse and X server. This also re-proves S156's fix through the real
path rather than through injection.

---

## 3. Finding: the message dispatcher is an allowlist of three

`CWnd::SendMessageA` (`SRC/compat/afxwin.h`) handles exactly `WM_GETFILE` (0x404),
`WM_GETGLOBALFONT` (0x403) and `WM_GETSTRING` (0x410), and **`return 0` for everything else**.
`PostMessageA` handles `WM_COMMAND` via the flight-close bridge and returns TRUE for everything else.
`SendMessageToDescendants` is `{}`. `ON_MESSAGE(message, memberFxn)` expands to **nothing**.

This is MA's §8-MA83 class ("the dispatcher answers 0 for routes it never implemented") and the same
silent-success shape as §8z (eventsink exact-type match) and §8-MA91 (empty macros).

The game sends **20 distinct `WM_*` types**; 16 land in the fallthrough and die reporting success —
*and several have real, implemented handlers on the other side*:

| dead route | sends | real handler exists? | independently-tracked symptom |
|---|---|---|---|
| `WM_RELEASELASTFILE` | 21 | `RDialog::OnReleaseLastFile` (RDIALOG.CPP:1187) | release half of `WM_GETFILE` — §8-MA84's "one open per FileNum" |
| `WM_GETARTWORK` | 15 | — | missing widget box-art |
| `WM_GETHINTBOX` | 12 | — | S147 fixed NULL derefs at these sites; the route itself is dead |
| `WM_GETXYOFFSET` | 11 | `RDialog::OnGetXYOffset` (RDIALOG.CPP:1184, :2067) | "minor combo/label Y-alignment" |
| `WM_PLAYSOUND` | 10 | — | UI click sounds |
| `WM_GETOFFSCREENDC` | 7 | — | offscreen DC / double-buffering |
| `WM_SELECTTAB` | 5 | `OnSelectTab` ×4 (LOAD, LWTASKFR, RAFTASKF, RAFTASKS) | **SP.6 / gold #16 selected-tab highlight** |
| `WM_GETX2FLAG`, `WM_GETCOMBOLISTBOX`, `WM_GETCOMBODIALOG`, `WM_CANACCEPTDROP`, `WM_DROPPEDON`, `WM_CANCELMODE`, `WM_LBUTTONDOWN/UP`, `WM_SIZE` | 1–5 each | varies | — |

**Corroborating evidence that this is real, not a grep artifact:** the port has *already* been
hand-delivering two of these routes at individual call sites rather than implementing the dispatch —
`MAINFRM.CPP:1417` calls `n->OnGetXYOffset()` directly, and `FULLPSYS.CPP:1198` delivers a swallowed
`WM_GETSTRING` with the comment *"compat has no message-map dispatch, so the posted WM_GETSTRING is
swallowed; we deliver it"*. Each was written as a local workaround; together they are the same
missing subsystem, papered over twice.

**Status: hypothesis, partially verified.** Confirmed by inspection: the routes are sent, the
dispatcher drops them, and the handlers exist. *Causation for any individual symptom above is NOT
yet claimed* — per S151's rule, a measurement licenses only the claim it measures.

**Runtime census (S157, `BOB_TRACE_MSG`, campaign-map + Directives recipe).** Four dead routes
actually fire in a single ordinary run:

| id | message | sends (static) |
|---|---|---|
| `WM_USER+1` | `WM_GETARTWORK` | 15 |
| `WM_USER+2` | `WM_GETXYOFFSET` | 11 |
| `WM_USER+5` | `WM_RELEASELASTFILE` | 21 |
| `WM_USER+9` | `WM_GETX2FLAG` | 5 |

So the dead-dispatcher finding is not theoretical: **art fetch, control positioning offsets and the
file-release half of the `WM_GETFILE` protocol are all being dropped during normal use.**

`WM_SELECTTAB` did **not** fire in this recipe (it exercises the map, not a tabbed screen), so it
remains dead-by-inspection only — listed above on the strength of its four real handlers, not on
runtime evidence. A tabbed-screen recipe is needed to promote it.

*Method note:* the static counts above came from `grep -o "WM_[A-Z_]*"`, whose character class
excludes digits and so silently truncated `WM_GETX2FLAG` to `WM_GETX`. The runtime census is what
caught it. A regex that can silently produce a *plausible wrong name* is the same hazard class as
§8k(3)/§8m(2) — a tool's own limit misread as evidence about the system.

---

## 4. Open items

- Runtime census from `BOB_TRACE_MSG` — which of the 16 dead routes actually fire.
- Verify one symptom end-to-end (`WM_SELECTTAB` → SP.6 tab highlight is the highest-value target).
- Classify the non-click capabilities: campaign day advance (`BOB_DAYLOOP` — known deep), flight
  launch (the `Launch3d` bridge — deep, compensating for the missing dispatch), keyboard input
  (`kb_push` — enters below the SDL handler, same layer-(1) gap the new SDL driver closes).

---

## 5. Capability inventory (running)

| capability | drivers | class | verdict |
|---|---|---|---|
| map OOB dialog clicks | `BOB_MAP_SDLCLICK` (real SDL, real GL), `BOB_MAP_CLICK` | shallow ×2 | **proven** (S156/S157) |
| front-end menu nav | `BOB_AUTOCLICK`, `BOB_CLICKXY` | shallow | proven from layer (2); layer (1) proven for the map path, not yet re-run for the menu path |
| map toolbar buttons | `BOB_MAP_CLICK` | shallow | proven from layer (3) |
| unit selection | `bob_map_select` via the shared dispatch | shallow | proven from layer (3) |
| campaign day advance | `BOB_DAYLOOP` | **deep** | **unproven** — the faithful `EndDayReview` day-advance has never run; known gap, already in CLAUDE.md |
| flight launch from menu | `Launch3d` bridge (FULLPSYS:1190) | **deep** | **unproven** — the bridge exists *because* the posted `WM_GETSTRING` is swallowed; it is a workaround for §3's missing dispatch |
| keyboard flight input | SDL keydown → `sdl_to_dik` → `kb_push` → DInput | shallow (real SDL keys on GL) | proven end-to-end previously |
| `BOB_AUTOFLY` / `BOB_AUTOQUIT` | `kb_push` directly | **deep** (enters below the SDL handler) | proves the DInput path, not the SDL key path |

**Next targets, in value order:** (1) implement the message dispatch so the 16 dead routes reach
their existing handlers — start with `WM_SELECTTAB` and check SP.6 / gold #16's tab highlight;
(2) the `EndDayReview` day-advance, replacing `BOB_DAYLOOP`; (3) re-run the menu-nav path through
`BOB_SDL_CLICK` on real GL to close its layer-(1) gap too.
