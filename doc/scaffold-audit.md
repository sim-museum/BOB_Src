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

---

## 6. S158 groundwork: the signature problem is already solved by the game

Implementing the dispatch looked like it needed template machinery, because the handlers are *not*
MFC-shaped: `long RDialog::OnGetXYOffset()` and `void CLoad::OnSelectTab()` take no arguments and
differ in return type.

They don't need it. `SRC/H/GLOBDEFS.H` already defines an adapter family:

```c
#define MSG2_0(memberFxn)  LRESULT MSG2_##memberFxn(int,int)     {return (int)memberFxn();}
#define MSG2_1(memberFxn)  LRESULT MSG2_##memberFxn(int a,int)   {return (int)memberFxn(a);}
#define MSG2_2(memberFxn)  LRESULT MSG2_##memberFxn(int a,int b) {return (int)memberFxn(a,b);}
#define MSG2_0v(memberFxn) LRESULT MSG2_##memberFxn(int,int)     {memberFxn();return 0;}
#define MSG2_1v(memberFxn) LRESULT MSG2_##memberFxn(int a,int)   {memberFxn(a);return 0;}
```

and the headers use them alongside the declarations (`RDIALMSG.H`: `afx_msg long OnGetXYOffset();`
then `MSG2_0(OnGetXYOffset);`). Every wrapped handler therefore already has a **uniform
`LRESULT (Class::*)(int,int)`** entry point — this is the original authors' own answer to the same
signature-variance problem.

So `ON_MESSAGE(message, memberFxn)` can expand to a registration of `&ThisClass::MSG2_##memberFxn`,
keyed on `typeid(ThisClass)`, and `SendMessageA` can look up `(typeid(*this), msg)` — the same shape
as the existing OCX eventsink. **Check before relying on it:** confirm each target handler actually
has a `MSG2_*` line (coverage is per-header and may be incomplete), and keep the dispatch behind an
env flag (default off) so the registration is inert until measured — the blast radius is every
`BEGIN_MESSAGE_MAP` in the game.

**Watch for the §8z trap:** key the lookup so a handler registered on a BASE class is still found;
the eventsink's exact-`type_info` match is precisely the bug that made every base-registered event
dead for the port's whole life.

---

## 7. S158: the dead dispatcher was hiding dead *code*, not just dead routes

Giving `SendMessageA` a real dispatch produced a **link** error before it ever ran:

```
undefined reference to `operator+(CString const&, char)'
  RLISTBXC.CPP:1700  CRListBoxCtrl::OnMouseMove
  RCOMBOC.CPP:1134   CRComboCtrl::OnMouseMove
```

The call site:

```c
phintbox = (CDialog*)GetParent()->SendMessage(WM_GETHINTBOX, NULL, NULL);
if (phintbox) { CString realhint = CString(' ') + str + ' '; ... }
```

compat's `SendMessage` was an inline allowlist of three that returned a literal `0` for
`WM_GETHINTBOX`. The optimizer could therefore prove `phintbox` NULL, delete the guarded block
entirely, and never emit the `operator+` call. Four single-character `operator+` overloads
(`CString+TCHAR`, `TCHAR+CString`, and their `char` twins — `typedef char TCHAR`, so two
signatures) had been **declared but never implemented for the port's entire life**, and nothing
ever failed, because no reachable code called them.

**The general shape, worth carrying to any port:** a stub that returns a *compile-time constant*
does not merely disable a feature at runtime — it lets the optimizer delete every branch that
depends on it, which suppresses the link errors that would otherwise reveal the rest of the missing
implementation. **The gap hides its own symptoms.** Restoring one route surfaced two more.

Corollary for estimating: "the dispatcher is missing" is not a bounded fix. Every route restored
makes previously-unreachable code reachable, and that code may reference more unimplemented compat.
Expect the work to grow as it succeeds — and treat each new link error as *evidence the fix is
working*, not as a setback.

### 7a. The stub existed in TWO layers, and fixing one registered nothing

`ON_MESSAGE` was stubbed in `compat/afxwin.h` **and again** in `SRC/H/GLOBDEFS.H`, which does
`#undef ON_MESSAGE` first and re-defines it empty under `#if defined(BOB_LINUX)` — with a comment
from an earlier session of this port: *"Message maps are stubbed (no-op) in the Linux port."*
GLOBDEFS.H wins. So the first implementation compiled cleanly, defined 146 `bob_mm_register()`
functions, and **registered nothing at all**.

It would have run, dispatched nothing, changed no pixel, and looked exactly like "the routes are
dead for some other reason". That is the same silent-success shape S158 exists to remove.

**What caught it was a static check, before spending a run:**

```
$ objdump -d --demangle bob | grep -c 'call.*bob_msgmap_chain'   # 296  <- BEGIN_MESSAGE_MAP worked
$ objdump -d --demangle bob | grep -c 'call.*bob_msgmap_add'     # 0    <- ON_MESSAGE did not
```

Two counters that should both be non-zero, one of which wasn't. **When you implement a registration
mechanism, verify the registrations exist before you test the behaviour** — `nm`/`objdump` answers
it in seconds, and a behavioural test that comes back negative cannot tell you *which* half failed.
(After fixing GLOBDEFS.H: `bob_msgmap_add` 86 call sites.)

Note also *why* symbol-counting was needed rather than looking for the runner objects: the
per-class registrar is an empty struct whose constructor GCC inlines into the TU's
`_GLOBAL__sub_I` thunk, so `nm | grep bob_mm_runinst_` finds **zero** even when registration works
perfectly. Absence of the symbol was not absence of the call — checking the wrong symbol would have
produced a confident wrong conclusion.

### 7b. Restoring a route keeps surfacing what the stub hid

Each step of enabling the dispatch exposed another gap that had been invisible:

| enabling step | what surfaced | why it was hidden |
|---|---|---|
| real `SendMessage` dispatch | `operator+(CString const&, char)` ×2 undefined | call sites were behind `if (ptr)` where the stub returned a constant 0, so the optimizer deleted them |
| real `ON_MESSAGE` | `WM_COMMANDHELP` not declared (3 sites) | the empty macro never evaluated its `message` argument, so the constant never had to exist |

An empty macro doesn't just discard the registration — **it also stops its arguments from ever
having to be valid**. Expect a wave of small missing definitions when you un-stub one, and read each
as confirmation the mechanism is now live.

### 7c. Static proof the mechanism targets the right routes

Registered message ids, read straight out of the binary (the immediates pushed to
`bob_msgmap_add`) — no display, no run needed:

```
$ objdump -d --demangle bob | grep -B8 'call.*bob_msgmap_add' \
    | grep -oE '\$0x(4[0-9a-f]{2}|365)' | sort | uniq -c | sort -rn
```

| id | message | regs | id | message | regs |
|---|---|---|---|---|---|
| `0x401` | `WM_GETARTWORK` | 6 | `0x40a` | `WM_GETOFFSCREENDC` | 5 |
| `0x402` | `WM_GETXYOFFSET` | 7 | `0x40b` | `WM_GETHINTBOX` | 5 |
| `0x403` | `WM_GETGLOBALFONT` | 6 | `0x40c` | `WM_PLAYSOUND` | 6 |
| `0x404` | `WM_GETFILE` | 6 | `0x40d` | `WM_GETCOMBOLISTBOX` | 6 |
| `0x405` | `WM_RELEASELASTFILE` | 5 | `0x40e` | `WM_GETCOMBODIALOG` | 5 |
| `0x406` | **`WM_SELECTTAB`** | **9** | `0x40f` | `WM_ACTIVEXSCROLL` | 5 |
| `0x407` | `WM_CANACCEPTDROP` | 1 | `0x410` | `WM_GETSTRING` | 6 |
| `0x408` | `WM_DROPPEDON` | 1 | `0x365` | `WM_COMMANDHELP` | 3 |
| `0x409` | `WM_GETX2FLAG` | 5 | | | |

**All 16 `WM_USER` routes the game sends are covered**, plus `WM_COMMANDHELP`. `WM_SELECTTAB` has
the most registrations of any route, consistent with its four `OnSelectTab` handlers plus the
`ON_MESSAGE_CLASS` row in FULLPSYS.CPP — it is the SP.6 / gold #16 suspect and is now wired.

Still unproven at this point, and not claimed: that any of these *fire* at runtime, that behaviour
changes, or that the tab highlight is fixed. Registration is necessary, not sufficient.

### 7d. Known risk before the runtime census: the declared map bases are not the real bases

`BEGIN_MESSAGE_MAP(theClass, baseClass)` is the only base information available at registration
time, and in this codebase it is frequently **not** the real C++ base:

| class | map declares | actual C++ base | own ON_MESSAGE rows |
|---|---|---|---|
| `LWDirectives` | `CDialog` | `RowanDialog` | **0** |
| `RDialog` | `CDialog` | — | full set |
| `CRToolBar` | `CDialog` | `CDialog` | full set |

So the chain for a derived dialog walks `LWDirectives -> CDialog` and **never reaches `RDialog`'s
handlers**. The four classes `RDIALMSG.H` names ("rdialog, rmdldlg, rtoolbar, AND cmigview should
all respond to all these messages") each declare the full `ON_MESSAGE` list themselves — which is
exactly why the static census shows 5–9 registrations per route rather than one.

**Prediction, to be checked against the run rather than assumed:** messages sent to those four
classes dispatch at depth 0; messages sent to a derived dialog with no rows of its own still miss.
If misses dominate, the fix is to stop relying on the declared base and instead record, per
registered class, a probe `bool(*)(void*)` doing `dynamic_cast<T*>(...)` — emitted from the same
macro where `T` is known — and fall back to scanning probes on a miss. That is inheritance-correct
regardless of what the map declares. **Not implemented speculatively; the census decides.**

### 7e. Runtime census — the prediction in §7d was right, and the chain walk works

First census (`BOB_MSG_DISPATCH=1 BOB_TRACE_MSG=1`, campaign-map recipe). The dispatch is live:

```
[msg] DISPATCHED 0x401 (WM_GETARTWORK)      to RFullPanelDial (depth 0) -> 27922
[msg] DISPATCHED 0x402 (WM_GETXYOFFSET)     to CRToolBar      (depth 1) -> 65280
[msg] DISPATCHED 0x405 (WM_RELEASELASTFILE) to CRToolBar      (depth 1) -> 0
```

| | count |
|---|---|
| `WM_RELEASELASTFILE` dispatched | 19 |
| `WM_GETXYOFFSET` dispatched | 18 |
| `WM_GETARTWORK` dispatched | 3 |
| receivers | `CRToolBar` (depth 1) ×37, `RFullPanelDial` (depth 0) ×3 |
| still unhandled (unique ids) | `0x401`, `0x402`, `0x405`, `0x409`, `0x40a` |

Two things this establishes:

1. **The base-class walk works and is needed** — `depth 1` means a handler registered on a base was
   found for a derived object. That is the §8z failure mode (exact `type_info` match, no walk)
   avoided *and demonstrated*, not merely asserted. `WM_GETARTWORK` returning **27922** matches the
   `fe_art=27922` in the shot-state banner: a real art FileNum, not a placeholder.
2. **§7d's prediction held.** The *same* message ids appear in both lists — dispatched for
   `CRToolBar`/`RFullPanelDial`, still missed for other receivers. Those are the derived dialogs
   whose maps declare `CDialog` while really deriving from `RowanDialog`, with no rows of their own.

So the fallback designed in §7d was implemented **after** the measurement justified it, not before:
each registered class also records a `dynamic_cast<T*>` probe, and a lookup that exhausts the
declared chain scans probes for a class that both handles the message and matches the object. It is
inheritance-correct whatever the map declares, and costs nothing on the hit path (miss-only).
148 probes registered, 86 handler rows.
