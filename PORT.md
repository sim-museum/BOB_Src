# Rowan's Battle of Britain — Linux Native Port

Goal: build the game to run **natively on Ubuntu 26.04** (no Wine), from the
original Windows source in `SRC/`. Game data: a working Wine install at
`/home/m/sgl/TUE/BattleOfBritain` — specifically
`.../WP/drive_c/Program Files/Rowan Software/Battle Of Britain/` (contains
`bob.exe`, `Lib3D.dll`, `BoB.pdb`, and all assets). Reference port:
`/home/m/ff` (a completed FreeFalcon Ubuntu 26.04 port — same era/tech;
its `CLAUDE.md` is the playbook and `src/compat/` is reusable).

## Architecture of the original (reconnaissance, 2026-06-08)

- **`bob.exe`** — PE32 i386 GUI, ~2.9 MB. Built by `SRC/MFC/BOB.DSP` (VC6),
  **1090 source entries**, `/machine:I386`, `/Zp1` (1-byte struct packing — the
  binary file formats depend on this), `/G6`, `_AFXDLL` (MFC as shared DLL).
  Links: `ddraw dplay dinput dsound dxguid winmm htmlhelp quartz strmbase
  vfw32 lib3d`.
- **`Lib3D.dll`** — software 3D engine, built by `SRC/LIB3D/LIB3D.DSP`.
- **~615K LOC** C/C++ across `SRC/` (660 .H, 464 .CPP + more). Main exe pulls
  mostly from `SRC/MFC/` (454 local .cpp) plus `3D MODEL BFIELDS MISSMAN
  MOVECODE HARDWARE AIRCRAFT FILES COMMS MYCMDS GENERAL AI MATH INPUT GRAPHICS`,
  headers in `SRC/H/`.
- **Standalone tools** (separate `main()`/`.dsp`, NOT part of the game exe,
  port last/never): `MEDITOR`, `PLACENAM`, `ITEMGRID`, `BFIELDS/BATNODE`.
- **Hand x86 ASM, ~9.8K lines, 8 files** — all **32-bit flat** MASM
  (`.386` / `USE32`; the "16 bit" in comments = 16-bit *color*, not code):
  `GRAPHICS/GRAFPASM.ASM` (6755), `MEDITOR/TPAINTWL.ASM` (editor-only),
  `3D/LSTRASM.ASM`, `MATH/MATRASM.ASM`, `HARDWARE/{PRO,PROLOG,HARDPASM}.ASM`,
  `FILES/CDROM.ASM`. Plus 8 files with inline `__asm`.
- **Rendering**: software rasteriser (the ASM blitters + `HARDWARE/HARD320*`),
  final framebuffer presented via **DirectDraw** (only ~11 files touch DDraw;
  **zero Direct3D**). This is good news: keep the software renderer, replace
  only the present path.
- **MFC**: `_AFXDLL` globally, but only ~12 files `#include <afx*>` directly
  (rest via `stdafx.h` PCH). Dialogs/CWinApp usage is the porting risk to size.

## Strategic decisions

1. **Build 32-bit native (i386 ELF on x86-64 Ubuntu via multilib).** This is the
   faithful, far-more-tractable path vs. the 64-bit route `ff` took:
   - The `.386/USE32` ASM assembles directly (no rewrite to C).
   - `/Zp1` packing, pointer sizes, and all binary file-format assumptions stay
     valid → **avoids the entire class of 32/64-bit pointer-truncation bugs**
     that dominated the `ff` port logs.
   - i386 multiarch is already enabled on this box.
2. **Reuse `/home/m/ff/src/compat`** as the Windows compat seed (windows.h,
   ddraw.h, dsound.h, dinput.h, io.h, etc.), adapted to 32-bit and extended for
   bob's MFC/DirectPlay/VFW surface. Seeded into `SRC/compat/`.
3. **SDL2 (window/input/timing) + OpenGL (present the software framebuffer as a
   texture) + OpenAL (DirectSound shim)**, mirroring `ff`.
4. **MASM ASM** → assemble with a MASM-compatible assembler (`jwasm`/`uasm`)
   targeting `elf32`; fallback = translate to NASM. Editor-only ASM is skipped.
5. **Case-insensitive file IO shim** (`open_nocase`, like `ff`) — game data is
   mixed-case; Linux is case-sensitive.
6. Stub first, implement later: DirectPlay (multiplayer), DirectShow/VFW (intro
   videos), HTML Help → stubs to reach a running single-player build.

## Toolchain (NEEDS USER — no passwordless sudo here)

i386 multiarch is enabled; apt is reachable. Please run:

```bash
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib libc6-dev-i386 \
    nasm cmake ninja-build \
    libsdl2-dev:i386 libgl1-mesa-dev:i386 libglu1-mesa-dev:i386 \
    libglew-dev:i386 libopenal-dev:i386
# MASM-compatible assembler for the .asm files (try in order):
sudo apt-get install -y jwasm || sudo apt-get install -y uasm || echo "fallback: translate ASM to nasm"
```

(If the `:i386` dev packages conflict, the fallback is to extract `.deb`s into a
local `extern/` like `ff` does — see `ff/CLAUDE.md` "extern/usr".)

## Phased plan

- **Phase 0 — Foundation** *(in progress)*: recon ✓, `linux-port` branch ✓,
  compat seed ✓, this doc ✓, top-level CMake skeleton, toolchain install (user).
- **Phase 1 — Compat layer + leaf builds**: 32-bit CMake per module; assemble the
  ASM; get `MATH`, `LIB3D` compiling against the compat headers; grow the shim
  until each module's TU compiles.
- **Phase 2 — Link**: resolve/stub every undefined Win32/MFC/DirectX symbol;
  produce a linking (if not-yet-running) `bob` ELF.
- **Phase 3 — Runtime bringup**: SDL2 window + GL present of the software
  framebuffer; case-insensitive IO; reach the main menu.
- **Phase 4 — Subsystems**: input (SDL), sound (OpenAL), then in-sim rendering
  and gameplay, iterating like `ff` did (instant action → campaign).

## Status log

- **2026-06-08 (1)**: Reconnaissance complete; decisions above locked in. Created
  `linux-port` branch, `SRC/compat/` (seeded 24 headers from `ff`), `build/`,
  this doc. Blocked on toolchain install (handed to user).
- **2026-06-08 (2)**: Toolchain installed & verified — `gcc/g++ -m32` work;
  **SDL2 + OpenGL + OpenAL link in 32-bit** (smoke test passed). GLEW dropped
  (we only need a basic GL textured quad); jwasm/uasm unavailable -> ASM is
  translated to **NASM/elf32**.
- **2026-06-08 (3)**: **ASM porting approach proven end-to-end** (the riskiest
  unknown). `MATH/MATRASM.ASM` (MASM) hand-translated to `MATH/matrasm.nasm`
  (NASM/elf32) — assembles, exports `XASMTransform`/`XASMDoBigXProd`. A C test
  using the **GCC inline-asm wrapper** (`call X...` with `"a"/"d"/"b"/"c"` reg
  constraints, non-PIE) calls `XASMDoBigXProd` and returns **bit-exact correct**
  results. Locked in `-fno-pie -no-pie` (CMakeLists). This is the reusable recipe
  for all 8 `.asm` files + the ~30 inline-`_asm` sites.

- **2026-06-08 (4)**: **MATH module fully ported & building via CMake/Ninja** ->
  `libbob_math.a` (MATH.CPP + MATRIX.CPP + matrasm.nasm.o, 32-bit). Established
  the full repeatable methodology (see recipes below). Foundational headers now
  Linux-clean and used tree-wide: `mathasm.h`, `vector.h`, `modvec.h`,
  `mymath.h`, `dosdefs.h`, `worldinc.h`, `uidvals.g`.
  - **DOSDEFS.H**: added a `__GNUC__` branch that `#define`s `__MSVC__` (reuse
    the MSVC non-asm code paths), `BOB_LINUX`, `__forceinline inline`, and uses
    the system `FILE`/`<cstdio>` instead of the Windows `_iobuf`.
  - **Case-insensitive includes**: `fix_include_case.py` (953 symlinks); a
    one-shot pass rewrote `\`->`/` in `#include`s across 261 files.
  - **DOS Ctrl-Z (0x1A) EOF bytes** stripped from 41 source/header files.
  - **CMake**: `FF_LINUX` defined globally (the ff compat headers gate on it);
    nasm object-format set to elf32 *before* project(); C/CXX compile options
    wrapped in `$<COMPILE_LANGUAGE:C,CXX>` so they don't leak into nasm.
  - Two genuine MSVC-isms fixed: `mymath.h` redeclared `pow/exp` w/o noexcept
    (-> `<cmath>` under Linux); a cross-class friend named a private
    `mobileitem::MoveList` (made public). A corrupt `uidvals.g` (spurious
    `#endif`, mangled `UI_56`) was repaired.

### Build (current)
```bash
cd /home/m/bob/build && cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release && ninja
```
Per-file probe (fast iteration):
```bash
g++ -m32 -fno-pie -fpermissive -fno-strict-aliasing -fcommon -fpack-struct=1 -w \
  -DNDEBUG -DBOB_LINUX -D_LINUX -DFF_LINUX -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp \
  -Dstrcmpi=strcasecmp -ISRC/compat -ISRC/H -c SRC/<dir>/<FILE>.CPP -o /tmp/x.o
```

- **2026-06-08 (5)**: Tree-wide cross-cutting fixes + remaining-work survey.
  - **Opaque enum forward-decls**: MSVC allows `enum X;` (no underlying type);
    GCC needs `enum X : int;`. A script converted 22 such enums *and* added
    `: int` to their full definitions (else "underlying type mismatch", e.g.
    `Angles`, `PhraseTables`). Faithful — all values fit in int.
  - **Unity-build aggregators excluded** (8 files that `#include "SOURCE=.."`
    the individual .cpp's): `MATHU MISSP MOVEP 3D/3D FILEI GENEC AI/AI MODES`.
    We compile the individual TUs, so these are never built.
  - **Survey** (8 core dirs, unity excluded): ~23 pass / ~78 fail. The failures
    are dominated by **inline asm still to convert in shared headers**:
    `hardpasm.h`, `fastmath.h`, `polygon.h`, `myvector.h`, `keytest.h`, plus
    per-file `_asm`. Also: bob's 2-arg `assert(expr,str)` macro (MYERROR.H) vs
    the system 1-arg assert — needs a consistent BOB_LINUX policy; a few more
    opaque enums used pre-declaration (`KeyVal3D`, `MAXqueuesize`).

- **2026-06-08 (6)**: Converted the shared inline-asm headers that blocked the
  most files (recipe in "Inline-asm conversion recipe"):
  - `FASTMATH.H` (fpSqrt/FloatToInt -> `__builtin_sqrt`/`lrint`),
    `POLYGON.H` (`ASM_Call` -> GCC indirect register-ABI call),
    `MYVECTOR.H` (was a byte-identical dup of vector.h -> now `#include "vector.h"`),
    `HARDPASM.H` (VGA I/O-port + DPMI/DOS-int routines -> Linux stubs;
    `ASM_Blat`->memcpy, `ASM_Splat`->dword/byte fill),
    `KEYTEST.H` (`const int` + comment-suffixed opaque enum).
  - Other cross-cutting: `const NAME=val;` implicit-int (FileMan.h) -> `const int`;
    `__assume`/`_assume` -> no-op in DOSDEFS.H GNU block; comprehensive opaque-enum
    normalization (forward-decls + definitions get `: int`).
  - **Survey now 25/101 compile** (8 core dirs). The asm cascade is gone; the
    remaining ~76 fail on per-module C++ issues, not shared headers.
  - REMAINING per-module roots to chase next: bob's 2-arg `assert(expr,str)` macro
    vs system 1-arg (MYERROR.H — decide a BOB_LINUX policy); class-not-forward-
    declared (`ArtInt` in ai.h); incomplete-type uses (`MESSAGE_STRUC`); an
    `enum ImageMapNumber`/`PlaneTypeSelect` "underlying type mismatch" that only
    appears in full-include context (imagemap.h compiles standalone) — bisect the
    include chain (persons2.h/landscap.h/transite.h) to find the stray
    `enum X;` (likely macro-generated) that lacks `: int`.

- **2026-06-08 (7)**: **Solved the enum underlying-type problem** (a deep C++
  conformance issue that drove a 1500-error cascade) and settled several more
  cross-cutting roots. MATH still builds.
  - **Root cause**: MSVC allows opaque `enum X;` and auto-widens enum values;
    GCC (1) rejects opaque forward decls without a fixed underlying type, and
    (2) forbids mixing fixed/unfixed declarations of the same unscoped enum.
  - **Resolution (consistent across decl + def)**: every opaque-forward-declared
    enum is `: int` on BOTH its `enum X;` forward decl AND its definition —
    including the macro-generated ones: `MINMAX(name,min,max)` (DOSDEFS.H) now
    emits `enum name : int {...}` under BOB_LINUX. `: int` handles the negative
    MINMAX ranges (e.g. -32768) too. The dozen velocity range-types whose max
    sentinel was `2147483648` (INT_MAX+1, and unrepresentable in MakeField's
    `int` template param anyway) were clamped to `2147483647`.
  - **Do NOT** put `: int`/`: unsigned int` on *anonymous* enums or on enum
    definitions that have no opaque forward decl — GCC auto-sizes those
    correctly (incl. `0xffffffff` flag enums); forcing a type breaks them.
  - Also fixed: `assert(expr,str)` -> variadic no-op under BOB_LINUX (MYERROR.H);
    `INT3`/`NOP` -> no-ops (DOSDEFS.H); `__assume`/`_assume` no-ops; `class
    ArtInt;` forward decl (ai.h).
  - **Survey holds at ~25/101**: the cascade is gone, but the surveyed dirs sit
    HIGH in the dependency tree, so each remaining file fails on the FIRST of
    several independent issues (missing class forward-decls, the `INSTANCEAI`
    macro, `ItemBase`->`ItemPtr` C-style downcasts GCC rejects, incomplete-type
    `MESSAGE_STRUC` uses). These are per-file/per-module, not cross-cutting.
  - **Recommended pivot for next session**: stop surveying high-tree dirs; build
    bottom-up via CMake instead — pick the next *leaf* modules (LIB3D, then
    GENERAL/FILES) whose whole file set compiles, add each to SRC/CMakeLists.txt,
    and only climb the tree as dependencies come online. The per-file C++ fixes
    (forward decls, downcasts, INSTANCEAI) are then localized per module.

- **2026-06-08 (8)**: **LIB3D module — 5 of 6 files compile.**
  - Compiling: `L3DGUID.CPP`, `GETDXVER.CPP`, `3D/MONOTXT.CPP`, and `ALLOC.C` /
    `RADIX.C` (these are C — they use `new`/`old` as identifiers — so CMake
    builds them with `LANGUAGE C`). `FLAGSW.CPP` is excluded (it is `#include`d
    by LIB3D.CPP as inline `Lib3D::` method bodies). `SRC/LIB3D/CMakeLists.txt`
    written (not yet added to SRC/CMakeLists.txt — waiting on LIB3D.CPP).
  - Compat/header additions (help the whole tree): `interface`->`struct`
    (compat_types.h, for the DirectX interface headers); `__int64`/`__uint64`
    in the DOSDEFS.H GNU block; `<fstream.h>`/`<iostream.h>` shims; guarded out
    the unused `<dmusici.h>` (DirectMusic) include in GETDXVER (its probe is
    commented out anyway).
  - **LIB3D.CPP (the 18k-line software rasteriser): 196 -> 135 errors.** All
    inline asm converted (the hard part): cpu_id (stubbed -> generic non-MMX
    path), SetToTopBit, FloatToInt, SineCosine x2, fpSqrt, MagicRotate,
    MaskAndRot (color-channel rotates -> portable C), and the rdtsc TIMER macros
    (no-ops). Remaining 135, by category:
    * ~56 **MSVC for-loop-scope leaks** — a function declares `for(int i=...)`
      then reuses `i` in later `for(i=...)` loops; GCC scopes `i` to the first
      loop. Fix = hoist `int i;` (and sometimes `int j;`) to function scope in
      ~13 functions. NOTE: a regex hoist is UNSAFE here (for-loops used as
      unbraced if/while bodies, and double-declaration) — verified twice that it
      regresses; do these **surgically**, per function.
    * ~10 const-correctness (`const SVertex*&` bound to `SVertex*`).
    * DirectDraw compat gaps: `DDERR_NODRIVERSUPPORT`, `ChangeDisplaySettings`,
      `IDirectDrawGammaControl` (incomplete), gamma-control members.
    * 3 fstream-by-value (deleted copy ctor), 3 `RNDCOLOUR`->`DNDCOLOR` ambiguous
      conversion, `va_start`/`va_end` (need `<cstdarg>`).

- **2026-06-08 (9)**: **LIB3D module BUILDS** -> `libbob_lib3d.a`, wired into
  SRC/CMakeLists.txt next to MATH. `ninja` now produces both static libs.
  LIB3D.CPP (18k-line rasteriser): **196 -> 0 errors**. Finished what (8) started:
  ~60 for-loop-scope leaks hoisted surgically; ClipSetCols const-correctness
  (decl+def); R3DCOLOUR ambiguity (drop value `operator ULong()`, keep ref one)
  + reinterpret-cast at SetColLighting sites; getNextToken fstream-by-ref; `0i64`
  -> `0LL`. New **`compat/bob_dx_extra.h`** supplies the missing Win32/DDraw/D3D
  symbols (D3DERR_* codes, DEVMODE, DDGAMMARAMP, IDirectDrawGammaControl, the
  DLL_/DM_/CDS_ constants, ChangeDisplaySettings/EnumDisplaySettings/StretchDIBits
  /_i64toa stubs, IIDs) and is pulled in from windows.h. Adding `#include
  <windows.h>` to LIB3D.CPP (it only had ddraw/d3d/objbase) cleared ~59 at once.
  `ALLOC.C` `new` identifier renamed; the .C files build as C++.

  **Two modules now build: bob_math + bob_lib3d.** General-purpose recipes that
  recur tree-wide and are now proven: inline-asm conversion, for-scope hoisting
  (surgical only), MSVC opaque enums, `interface`/`__int64`/`0i64`/`i64toa`
  -isms, and the bob_dx_extra compat header for DirectX symbols.

- **2026-06-08 (10)**: **Phase 2 underway — AIRCRAFT is the 3rd module to build**
  (`libbob_aircraft.a`). Cross-cutting fixes this batch unblocked it entirely and
  lifted AI/MODEL/3D: MSVC calling-convention keywords (`__cdecl` etc.) in the
  DOSDEFS GNU block; member-function-TYPE typedefs `typedef R(Class::Name)(args)`
  -> `typedef R(Name)(args)` (VIEWSEL.H/COLLIDED.H); `PlaneTypeSelect` def given
  `: int` (FLYINIT.H, comment-suffixed); `=NULL` pure-specifier -> `=0`; compat
  shims `winerror.h`/`ole2.h`; `Vfw.h` case-alias.
  - **De-duplicated real per-dir status** (earlier counts were inflated by the
    case-alias symlinks; survey now skips `-L`): AIRCRAFT 8/8 ✓, MISSMAN 8/17,
    3D 7/30, MODEL 6/21, FILES 4/7, GENERAL 2/4, AI 1/7, MOVECODE 1/11,
    HARDWARE/BFIELDS low. MFC/ (454 files, the game core) not yet surveyed.
  - **Identified next roots** (Phase 2 grind):
    * **~13 files: `_asm` blocks** in game .cpp (per-file conversion; 28 game
      .cpp files contain inline asm total).
    * **bit-field-overlay template conflict** — `replay.h`/`persons2.h` use
      `LASTFIELD`/`MidField`/`Overview` macros (BITCOUNT.H) to build bit-packed
      unions; GCC reports `Overview ... conflicts with a previous declaration`.
      Deep template issue MSVC tolerated; affects ~13 files. Handle carefully.
    * **`Wrapper.h`** missing — actively included by the legacy VGA HARDWARE
      files (HARD320/HARDVBE*, the pre-DirectDraw software renderers). Likely
      stub it, or these may be excludable deadcode for the DirectDraw build.
    * **`Vfw.h`** (Video for Windows) — alias added; `vfw.h` compat may need
      more for the movie/intro path. Token-pasting `##` macro issues (Landscap).

- **2026-06-08 (11)**: **FILES is the 5th module to build** (libbob_files.a:
  DOSFILE/LOADLIST/DISKIO/LOADLIB/WINFILE/FILEMAN). Five module libs now build:
  math, lib3d, aircraft, general, files.
  - FILEMAN: `FileMan : public fileman` on Linux (codebase calls
    `FILEMAN.publicMethod()` externally, only valid with an accessible base;
    MSVC was lax); `eip()`/`esp()` -> `__builtin_return_address`/
    `__builtin_frame_address`; `_set_new_handler` guarded; implicit-int consts.
  - More cross-cutting roots cleared: `_WINBASE_` marker (compat winbase headers;
    bob gates Win32 structs on it), `stub3d.h` atomic xchg (18 files),
    `ole2ver.h` shim, FASTMATH for-scope, SHAPES `fileblock` fwd-decl, `__cdecl`,
    member-func-type typedefs, etc.
  - **KNOWN HARD BLOCKER (deferred): the bit-field overlay** (BITCOUNT.H
    FIRSTFIELD/BITFIELD/LASTFIELD via replay.h/persons2.h, ~13 files). Each
    FIRSTFIELD needs an `Overview` typedef (= storage size) visible to its
    BITFIELD/LASTFIELD. Original puts it in the anonymous union (GCC: "Overview
    conflicts"); hoisting to the struct (current BOB_LINUX) fixes single-
    FIRSTFIELD structs but multi-FIRSTFIELD ones (e.g. _asprim_values) hit
    "member typedef redeclaration". `-fms-extensions` doesn't help. The clean fix
    is to rework the macros to thread the storage type through each field macro
    (or generate a unique-per-union typedef) WITHOUT editing every BITFIELD call
    site in the headers - needs a focused, careful pass. This gates a chunk of
    AI/MODEL/MISSMAN.

- **2026-06-08 (12)**: **Bit-field overlay blocker SOLVED** (commit ba832b3).
  The only multi-FIRSTFIELD struct is replay.h's `_asprim_values` (two UByte
  unions). Added a **NEXTFIELD** macro: a 2nd+ FIRSTFIELD-style union that, on
  Linux, reuses the struct-scope `Overview` typedef the first FIRSTFIELD hoisted
  (avoids the member-typedef redeclaration); on MSVC it's just FIRSTFIELD.
  WORLDINC.H's 7 FIRSTFIELDs are each in their own single-FIRSTFIELD struct, so
  the hoist alone covers them. Clears bitcount.h for the replay/persons2 cluster.

- **2026-06-09 (13)**: **Two more modules build — INPUT (6th) and 3D (8th, 4/5)
  — plus a partial MISSMAN (7th, 7/10). Eight module libs total.** Also a major
  **survey-methodology fix**: many BoB .cpp are *fragments* (zero `#include`,
  pulled into a unity aggregator like MFC/_MFC.cpp); the survey now excludes both
  unity aggregators AND zero-include fragments, so per-module counts reflect real
  standalone TUs.
  - **INPUT** (ANALOGUE + KEYLIST; KEYSTUB is a _MFC fragment). Compat additions
    that recur tree-wide: `IN`/`OUT`/`OPTIONAL` SAL macros + `FIELD_OFFSET`
    (compat_types.h); `DECLARE_INTERFACE`/`_` COM macros (objbase.h); joystick
    API JOYINFO(EX)/JOY_*/joyGetPos* (mmsystem.h ×2); DInput A-aliases
    (IDirectInputDevice2A/7A/8A as #defines so `struct X;` fwd-decls still work),
    DIDOI_* flags, DIEB_NOTRIGGER (dinput.h).
  - **Cross-cutting roots (high leverage, no regressions):**
    * **string ambiguity** — the iostream.h/fstream.h shims did `using namespace
      std`, dragging in std::string which collided with BoB's own
      `typedef char* string` (dosdefs.h) → every `string&` param ambiguous.
      Replaced with selective `using std::<stream-name>` (VC6's <iostream.h>
      exposed stream names globally but NOT std::string). Faithful + fixes it
      tree-wide.
    * **old-iostream BSTREAM.H** — BOB_LINUX branches: openmode `+`->`|`
      (operator+ on ios::openmode decays to int, no matching open()); emulate the
      MSVC `ios::noreplace` extension (fail-if-exists) via an existence check.
    * **PROF.H** — all 4 inline-_asm 64-bit timer routines -> portable C
      (ht:lt add/sub, /1000 quotient+remainder, rdtsc via __builtin_ia32_rdtsc).
    * legacy un-prefixed keywords `pascal`/`_pascal`/`cdecl` -> no-ops (DOSDEFS).
    * `DAM(...)` variadic trampoline (MSVC fills omitted macro args empty).
  - **MISSMAN** 7/10 (DEBRIEF/INTRMISS/NODEBOB/NODEKILL/ONEMISS/PEACMISS/
    SO51MISS). NODEBOB also needed 2 for-scope hoists + `SUBCALL` macro
    `assert(this)`->`assert(this);` (the -DNDEBUG assert is `((void)0)`, an
    expression, not the {}-statement form MSVC used). NODEBOB.H now
    `#include "package.h"` (struct Profile::PackageStatus/BetterRule).
  - **3D** 4/5: LANDSCAP done (InterpLight `->##p1` paste, fpSqrt/fpTan/fpSinCos
    asm, for-scope ×6, abs(unsigned) cast, `Shape.newco`->`Shape::newco` static).

- **2026-06-09 (14)**: **Four more modules build — AI (9th), MOVECODE (10th),
  MODEL (11th), HARDWARE (12th). Twelve module libs total.** Two big methodology
  shifts this batch:
  - **Filter to BOB.DSP game files (303 TUs).** Many .cpp in the tree are
    editor/tool/deadcode NOT in the game exe (CEDITOR, MEGLOBAL, HARD320*,
    PERSONS5, vcl/devstudio-path files). The survey now skips anything whose
    basename isn't in BOB.DSP — stops wasting effort on non-game files.
  - **Compile the `_XXX.CPP` unity per module, not individual TUs.** For modules
    whose DSP build uses a unity aggregator (`_MODE`/`_MOVE`/`_HARD`/`_COMM`/
    `_BFIE`...), THAT unity is the faithful, link-complete build unit: it pulls
    the fragment files (which have no own #includes), gives every fragment the
    include context the others established, and avoids duplicate symbols.
    Dramatic effect — MODEL's 16 fragments compiled as a unity reduced to **2
    errors** total; HARDWARE 52->0, MOVECODE clean, once the standalone roots
    were cleared. (NOTE: the earlier modules MATH..AI/MISSMAN/3D were wired from
    standalone TUs and are therefore link-INCOMPLETE — they miss fragment-only
    files; revisit by switching them to their unities for the Phase-2 link.)
  - **Cross-cutting roots (no regression to the 12 libs):**
    * **NODEBOB.H now #includes uniqueid.h** — cleared the `UniqueID has not been
      declared` cascade from package.h/nodebob.h/bfnumber.h **tree-wide (111->1)**.
    * **assert/nassert under BOB_LINUX expand to NOTHING** (was `((void)0)`) —
      handles bob's no-semicolon `assert(x) if(...)` and unbraced
      `if(c) assert(x); else` idioms that `((void)0)` breaks.
    * **compat LONG = `long`** (Win32 ABI; was int32_t) — fixes the
      int32_t-vs-long conflicting-typedef vs cstring.h.
    * MIDI output API stubbed (mmsystem.h); DirectSound ANSI aliases +
      IDirectSound_* C-macros (dsound.h); CLSCTX_INPROC/IStream/LPSTREAM
      (objbase.h); __RPC_FAR/FAR/NEAR (compat_types.h); raddef.h-before-radio.g
      pattern for USE_PHRASE_* aircraft enums.
  - Recurring per-file idioms now well-understood: **static-member-via-type** the
    MSVC `.`-on-a-typename laxity — `Shape.newco`/`TimerCode.FRAMETIME`/
    `mobileitem.ACList`/`LandScape._blockWidth` -> `::`; **for-scope hoists**;
    **FPU asm** -> `__builtin_sqrt/atan2/sin/cos`.

- **2026-06-09 (15)**: **COMMS is the 13th module** (via `_COMM.CPP` unity).
  DirectPlay stubbed (minimal IDirectPlayLobby3 in compat/dplobby.h — the empty
  compat stub had been shadowing the real SDK header). **compat DWORD/ULONG are
  now `unsigned long`** (Win32 ABI, matches bob's ULong) — fixes ULong&/DWORD
  bind mismatches, no regression. DEFINE_GUID declaration-form made variadic
  (1-arg `DEFINE_GUID(BOB_GUID)` MSVC empty-fill). BAD_RV implicit-int extern;
  implicit-int statics; n for-scope hoists. **Thirteen module libs build.**
  Remaining non-MFC: BFIELDS (bfrefs.g corruption), GRAPHICS (asm-only),
  MYCMDS (empty). Then the MFC game core.

### Deferred / known work
- **BFIELDS**: blocked by a **corruption in generated header SRC/H/bfrefs.g**
  (~line 209: a GR_Pack_TakeTime table's declaration + first entries are missing,
  leaving a dangling `eTime_W2G1},`). Present in the original imported source.
  GLOBREFS pair/pair04 macro fixes are already committed; module needs bfrefs.g
  regenerated/reconstructed before it can build.
- **COMMS** (_COMM.CPP, ~41 errs): DirectPlay-heavy (IDirectPlayLobby3 incomplete,
  ULong&/DWORD bind, FILE_ATTRIBUTE_*, BOB_GUID). Multiplayer — deferred to stubs
  per the phased plan.
- **MFC game core (177 .cpp in SRC/MFC) — THE major remaining work.** Scoped this
  session: `compat/afxwin.h` (and afx.h/afxext/afxcmn/...) are **empty stubs** —
  no CWnd/CDC/CFont/CDialog/CWinApp/CView/message-maps. **FreeFalcon's afxwin.h is
  ALSO empty (FF didn't use MFC), so there is no MFC layer to borrow** — this is a
  from-scratch MFC compat buildout. Groundwork done: stubbed the remaining afx
  umbrella headers (afxole/afxodlgs/afxauto/afxpriv/afxmt/afxdisp/afxtempl) and
  added case-alias symlinks for the 17 `.cpp` fragments `_MFC.CPP` #includes
  (MIGView.cpp->MIGVIEW.CPP, etc.). `_MFC.CPP` now reaches `resource.h` + the
  `#error include 'stdafx.h'` PCH guard (MIG.h gates on `#ifndef __AFXWIN_H__`) —
  past those lies the real flood of undefined MFC classes. Add `-ISRC/MFC` to the
  MFC target include path (resource.h lives there).
  - **Exact MFC surface to implement** (from a `: public CXxx` survey): base
    classes **CWnd (15 derived), CDialog (6), CWinApp, CView, CFrameWnd,
    COleDocument, COleDispatchDriver, CCommandLineInfo** + roots **CObject,
    CCmdTarget, CWinThread**; GDI **CDC, CFont, CGdiObject, CPen, CBrush,
    CBitmap**; value types **CRect, CPoint, CSize** (RECT/POINT already in
    compat_types.h); **CDataExchange**; no-op message-map macros
    (DECLARE_MESSAGE_MAP, BEGIN/END_MESSAGE_MAP, ON_*, DECLARE_DYNCREATE,
    IMPLEMENT_*). afxwin.h must `#define __AFXWIN_H__`. **CString is bob's own
    (SRC/H/cstring.h), NOT MFC.** Also need a **streams.h** (DirectShow) stub.
    Back the layer with SDL/GL at runtime later; covers 3D/OVERLAY too.
- **MISSMAN** PACKAGES/SAVEGAME/UIMSG, **AI** MSGAI/USERMSG: rchatter.h not
  self-contained, incomplete MissMan/CString, SECSPERMIN/Directives::RAF. (Most
  of these likely resolve when built via the module unity rather than standalone.)
- **ACMSIMPL GentleBankData**: used in 3 sites, defined nowhere in the tree —
  extern-declared for now; **Phase-2 link TODO**.

### NEXT ACTIONS (resume here)
1. **Switch the standalone-wired modules to their `_XXX.CPP` unities** for
   link-completeness (AIRCRAFT/MISSMAN/3D/AI/INPUT — MATH/LIB3D/GENERAL/FILES are
   small/likely complete). This both fixes missing fragment code AND tends to
   reduce errors (shared context).
2. **MFC game core (177 .cpp)** — the big remaining chunk. Build a from-scratch
   minimal MFC compat layer in `compat/afxwin.h` (CObject/CCmdTarget/CWnd/CDialog/
   CDC/CFont/CWinApp/CView/CDocument/CFrameWnd + no-op message-map macros), since
   neither bob's nor ff's afxwin.h has it. Then `_MFC.CPP` + the standalone MFC
   TUs. (afx stubs + `_MFC.CPP` cpp case-symlinks already in place.)
3. Reconstruct/​regenerate **bfrefs.g** to unblock BFIELDS.
4. Then **Phase 2 (link)**: assemble remaining `.asm` (GRAPHICS/GRAFPASM,
   3D/LSTRASM, HARDWARE/*) to nasm; resolve undefined symbols (GentleBankData,
   DirectPlay/DirectShow stubs); produce the `bob` ELF.

- **2026-06-09 (16)**: **MFC compat FOUNDATION built** (`compat/afxwin.h`,
  `compat/streams.h`). afxwin.h is now a real minimal MFC: `__AFXWIN_H__` define;
  no-op message-map/runtime-class macros (DECLARE/BEGIN/END_MESSAGE_MAP, ON_*,
  DECLARE_DYNCREATE, IMPLEMENT_*, afx_msg, RUNTIME_CLASS); value types CRect/
  CPoint/CSize (on RECT/POINT/SIZE); the class hierarchy CObject -> CCmdTarget ->
  {CWnd -> CDialog/CView/CFrameWnd, CDocument -> COleDocument, CWinThread ->
  CWinApp}; GDI CGdiObject -> CDC/CFont/CPen/CBrush/CBitmap; plus CCommandLineInfo,
  CDataExchange, COleDispatchDriver, AfxGetApp/AfxGetMainWnd. streams.h is a
  minimal DirectShow stub (IGraphBuilder/IMediaControl/IVideoWindow/IMediaEventEx/
  IBasicAudio + CLSIDs) so FULLPSYS.CPP (intro movies) compiles; video deferred.
  - **Result: `_MFC.CPP` no longer has ANY missing-MFC-base-class errors** — the
    foundation resolves. It now shows ~2655 BoB-specific errors (down from "no MFC
    at all"), so MFC is now normal per-root grinding like every other module:
    top roots = map-tile macros `MAP_<N>` (~900, a macro not expanding),
    FIL_MAP* enums, BoB's own UI classes incomplete (RDialog/CRButton/CSystemBox —
    ordering), OLE types (OLE_COLOR, VT_*). Compile the MFC module with
    **`-ISRC/MFC`** (resource.h). 13 module libs still build (afxwin/streams are
    not included by them).
  - **The ~900 `MAP_<N>` errors all cascade from `FIL_MAP_TABLE`** (frmap2.h),
    which is **defined nowhere in the tree** ("did you mean FIL_MAP_xARMY" — the
    sibling map file-enums exist but not this one) — another genuinely-missing
    symbol like GentleBankData/BOB_GUID, likely from an uncommitted/generated
    files.g. Resolve before the MAP table will compile. The MFC grind also has
    tractable cross-cutting roots to clear first: OLE_COLOR (typedef = DWORD) and
    VT_* VARIANT constants (add to a compat oleauto/wtypes), then the per-file
    ordering of bob's own RDialog/CRButton/CSystemBox UI classes.

- **2026-06-09 (17)**: **MFC module grind — `_MFC.CPP` 2655 -> 723 errors** on the
  built MFC foundation. Compile the MFC module with **`-ISRC/MFC`**.
  - **afxwin.h fleshed out**: common control wrappers (CStatic/CButton/CEdit/
    CListBox/CComboBox/CScrollBar/CToolBar), container templates CArray/CList
    (std-backed; CList POSITION iteration stubbed empty — UI lists not driven
    yet), diagnostic macros (ASSERT/VERIFY/TRACE*), OLE event/dispatch map macros
    (BEGIN_EVENTSINK_MAP/ON_EVENT/DISP_*/VTS_*), CRect operators, and many
    CDC/CWnd/CWinApp/COleDispatchDriver methods. POSITION/CCreateContext types.
  - **compat_types.h**: OLE base types (OLE_COLOR/VARENUM VT_*/BSTR/VARTYPE/
    LPDISPATCH/DATE/...). **compat_wingdi.h**: PS_*/TRANSPARENT/MM_TEXT/R2_*/
    GetDeviceCaps indices/DT_* flags.
  - **The file-enum map cascade fix** (~1289 errors): `_MFC.CPP` now defines
    F_BATTLE and force-includes dosdefs.h + files.g at the top, locking FileNum
    with F_COMMON.G(FIL_MAP_TABLE)+F_GRAFIX.G+F_SOUNDS.G before any fragment's
    own files.g (a fragment defined F_COMMON first, excluding the map enums).
  - **PACKAGE.H now #includes uniqueid.h** (self-contained, like nodebob.h) —
    cleared the UniqueID cluster. `_MFC.CPP` early-includes uniqueid/cstring/
    rdialog/rbutton so the dialog/map fragments see bob's own UI base classes
    (their headers don't self-include them).
  - **_MFC.CPP driven 723 -> 301** (cumulative 2655 -> 301). Cleared this pass:
    globdefs.h ON_MESSAGE map-builders no-op'd; cstring.h included BEFORE stdafx
    (so CString is complete when __AFX_H__ flips bob's "MFC present" branches);
    afxwin.h grew controls (CButton/CEdit/CListBox/CComboBox/CScrollBar/CMenu/
    CToolBar), CArray/CList(+POSITION), CFile/CArchive/CPrintInfo, CRect/CPoint/
    CSize arithmetic, OLE-ctl factory/proppage/connection macros, CDC CString-
    template text methods, AFX_CMDHANDLERINFO/HELP_*/AfxLoadString; the file-enum
    map-cascade fix; PACKAGE.H self-contains uniqueid.h; _MFC early-includes the
    bob UI base headers (rdialog/rbutton/rlistbox/rmdldlg/maintbar/titlebar/
    sysbox/hintbox) + case-alias symlinks for them.
  - **_MFC.CPP driven 301 -> ~193** (cumulative 2655 -> ~193, >92%). Added: full
    CWnd message-handler virtuals (OnLButtonDown/OnMouseMove/OnPaint/...), all the
    MFC ON_WM_*/ON_*N map-entry macros as no-ops, CMenu/CFile/CArchive/CPrintInfo/
    CPropExchange, CDC GDI methods (Polygon/Ellipse/ExtTextOut CString overloads),
    CRect/CPoint arithmetic, NMHDR/MINMAXINFO/HELPINFO/HTASK, SIZE_*/TPM_*/QS_*/
    CBRS_*/DISPATCH_*/HID_* consts, AfxLoadString/HELP_*; cstring.h moved before
    stdafx (CString complete when __AFX_H__ flips bob's branches); globdefs.h
    ON_MESSAGE no-op'd; many bob UI headers early-included.
  - **STRUCTURAL BOUNDARY reached (~193 left).** The remaining tail is dominated
    by bob's ActiveX-control wrapper classes (CRCombo/CRStatic/CRSpinBut/CRTabs/
    CRComboExtra/Rtestsh1/CSQuick1/...) which are FORWARD-DECLARED in headers but
    DEFINED in their own .cpp control projects (SRC/rcombo, SRC/rstatic, ...), so
    they're incomplete where the unity uses them as value members. Options:
    (a) move those class defs into headers, (b) compile each control project and
    include its def, or (c) accept them as separate TUs. Plus the other ~150
    standalone MFC .cpp beyond `_MFC.CPP`. Recipe for the non-control tail is
    mechanical: run _MFC, add the next method/macro/const to afxwin.h or
    early-include the bob header. **13 module libs keep building — afxwin/streams
    aren't included by them, so all MFC work is regression-safe.**

### Inline-asm conversion recipe (validated)
At each `_asm`/`__asm`/`#pragma aux` site add a `#if defined(BOB_LINUX)` branch
*before* the `__MSVC__`/`__WATCOMC__` one (BOB_LINUX is checked first):
- **register-arg routine** kept in nasm -> GCC wrapper:
  `__asm__ volatile("call X..." : "=a"(r) : "a"(),"d"(),"b"(),"c"() : "esi","edi","cc","memory")`
- **FPU/bit/math** routine -> portable C with `__builtin_*` (sqrt/sin/cos/lrint/
  fabs/clz/ctz); fixed-point 64-bit ops via `long long`/`int64`. Keep the
  original under `#elif`/`#else` so the Windows build is untouched.

### ASM porting recipe (validated)
1. Translate each MASM `.ASM` -> `<name>.nasm`: `SEGMENT`->`section`, `_X<name>`
   public label -> `X<name>` (no leading underscore on Linux ELF), `@@local`->
   `.local`, `dword ptr ds:[sym]`->`[sym]`, hex `1Fh`->`0x1F`, scratch `dd ?`->
   `.bss resd`. Add `section .note.GNU-stack noalloc noexec ...`.
2. In `H/DOSDEFS.H`, stop defining `__MSVC__` under `__GNUC__` (define a
   `BOB_LINUX` path instead) so GCC skips the MSVC `_asm{}` blocks.
3. At each `#ifdef __MSVC__` inline-asm site add `#elif defined(BOB_LINUX)` with
   a GCC `__asm__ volatile("call X...": "=a"(r): "a"(),"d"(),"b"(),"c"() :
   "esi","edi","cc","memory")` wrapper (omit ebx/ebp from clobbers when the
   routine preserves them; pass `&ref` for reference args).
