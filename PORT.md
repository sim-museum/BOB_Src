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

### NEXT ACTIONS (resume here)
1. Convert inline asm in the shared math/input headers (same recipe as
   vector.h/mathasm.h): `H/FASTMATH.H`, `H/POLYGON.H`, `H/MYVECTOR.H`,
   `H/HARDPASM.H`, `H/KEYTEST.H`. These unblock the most files.
2. Settle the `assert(expr,str)` macro under BOB_LINUX (define consistently in
   MYERROR.H; make sure system headers don't fight it).
3. Re-run the survey; then go module-by-module (GENERAL, AI, AIRCRAFT, MISSMAN,
   MOVECODE, COMMS, INPUT, 3D, MODEL, HARDWARE, BFIELDS, then MFC/ — 454 files),
   adding each to `SRC/CMakeLists.txt` as it compiles. Assemble the remaining
   `.asm` (GRAPHICS/GRAFPASM, 3D/LSTRASM, HARDWARE/*) to nasm as their callers
   come online. Then Phase 2 (link).

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
