# Rowan's Battle of Britain - Linux native port (32-bit / i386)
# See PORT.md for architecture notes and the phased plan.
cmake_minimum_required(VERSION 3.16)
# Must be set before the ASM_NASM language is enabled by project().
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
project(BattleOfBritain C CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- 32-bit i386 target (faithful to the original; keeps ASM, /Zp1, ptr sizes) ---
# Non-PIE: the original was non-PIC, and the many register-level inline-asm
# wrappers clobber ebx (the PIC GOT register) -> build/link as -no-pie.
# Scope to C/CXX so these don't leak into the nasm command line.
add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-m32> $<$<COMPILE_LANGUAGE:C,CXX>:-fno-pie>)
add_link_options(-m32 -no-pie)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

# --- AddressSanitizer diagnostic build (BOB_ASAN=ON) ---------------------
# Off by default: the normal build is unaffected. Configure a SEPARATE tree
# (e.g. build-asan/) with -DBOB_ASAN=ON to instrument heap accesses for the
# Sprint-1 R1.2 heap-corruption hunt (InitPreferences combat path). Keeps the
# faithful Release layout (-O3 + -fpack-struct) so the layout-sensitive bug
# still reproduces; adds frame pointers for readable backtraces.
option(BOB_ASAN "Build with AddressSanitizer (heap-corruption diagnostics)" OFF)
if(BOB_ASAN)
    # -fsanitize-recover=address: with ASAN_OPTIONS=halt_on_error=0 this makes
    # ASan REPORT each invalid access and CONTINUE, so one run surfaces every
    # error in order (the benign boot-path over-reads AND the later combat
    # write-corruption) instead of stopping at the first.
    # -g: line numbers for the inlined destructor/delete[] sites (R1.3d double-free hunt).
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize=address>
                        $<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize-recover=address>
                        $<$<COMPILE_LANGUAGE:C,CXX>:-g>
                        $<$<COMPILE_LANGUAGE:C,CXX>:-fno-omit-frame-pointer>)
    add_link_options(-fsanitize=address)
endif()

set(BOB_SRC ${CMAKE_SOURCE_DIR}/SRC)

# Platform / API-shim defines. /Zp1 in the original -> pack everything to 1.
add_compile_definitions(
    BOB_LINUX _LINUX
    FF_LINUX            # the reused ff/ compat headers gate everything on FF_LINUX
    stricmp=strcasecmp strnicmp=strncasecmp strcmpi=strcasecmp strcmpni=strncasecmp
)

# Legacy 1990s Win32 C++: relax strictness so the bulk compiles unmodified.
# All C/CXX-only so nasm never sees them.
add_compile_options(
    $<$<COMPILE_LANGUAGE:C,CXX>:-fpermissive>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-strict-aliasing>
    # The codebase uses the MSVC-era idiom of passing *(T*)NULL as a "no arg"
    # sentinel and testing `if (&param)` inside the callee (e.g. View3d::MakePassive's
    # `const CRect& pos = *(CRect*)NULL` + `if (&pos) coords=pos;`). GCC's optimiser
    # assumes a reference/this can never be null and deletes such guards under -O3,
    # turning the sentinel into a NULL read. Keep the guards honest.
    $<$<COMPILE_LANGUAGE:C,CXX>:-fno-delete-null-pointer-checks>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fcommon>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fpack-struct=1>  # matches MSVC /Zp1 (file formats depend on it)
    $<$<COMPILE_LANGUAGE:C,CXX>:-w>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-write-strings>
)

# Compat layer (windows.h, ddraw.h, dsound.h, ... ) visible to every target.
include_directories(${BOB_SRC}/compat ${BOB_SRC}/H ${BOB_SRC}/h)

find_package(Threads REQUIRED)

add_subdirectory(SRC)

# --- the `bob` executable -------------------------------------------------
# A game built around global ctors (the MFC theApp object, static dispatch
# tables, the BFieldGlobalTable, ...) -> link every module --whole-archive so
# nothing is dropped for lack of a main()-reachable reference.
# --allow-multiple-definition: -fcommon tentative defs appear in several TUs.
set(BOB_MODULE_LIBS
    bob_compat bob_math bob_lib3d bob_aircraft bob_general bob_files
    bob_input bob_missman bob_3d bob_ai bob_movecode bob_model
    bob_hardware bob_comms bob_mfc bob_bfields bob_rlistbox)
add_executable(bob ${BOB_SRC}/compat/bob_main.cpp)
target_link_libraries(bob
    -Wl,--whole-archive ${BOB_MODULE_LIBS} -Wl,--no-whole-archive
    -Wl,--allow-multiple-definition
    -L/usr/lib/i386-linux-gnu -lSDL2 -lGL -lopenal
    Threads::Threads m)
