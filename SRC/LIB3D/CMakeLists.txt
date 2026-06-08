# LIB3D - the software 3D engine (original Lib3D.dll). Built as a static lib.
# FLAGSW.CPP is NOT listed: it is #included by LIB3D.CPP (inline method bodies).
# ALLOC.C / RADIX.C are C (they use `new`/`old` as identifiers) -> LANGUAGE C.
add_library(bob_lib3d STATIC
    ALLOC.C
    RADIX.C
    L3DGUID.CPP
    GETDXVER.CPP
    LIB3D.CPP
    ${BOB_SRC}/3D/MONOTXT.CPP
)
set_source_files_properties(ALLOC.C RADIX.C PROPERTIES LANGUAGE C)
target_include_directories(bob_lib3d PUBLIC
    ${BOB_SRC}/compat
    ${BOB_SRC}/H
    ${CMAKE_CURRENT_SOURCE_DIR}
)
