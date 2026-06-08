# LIB3D - the software 3D engine (original Lib3D.dll). Built as a static lib.
# FLAGSW.CPP is NOT listed: it is #included by LIB3D.CPP (inline method bodies).
# ALLOC.C / RADIX.C are .C (compiled as C++ here; the one `new` identifier was
# renamed so they are valid C++).
add_library(bob_lib3d STATIC
    ALLOC.C
    RADIX.C
    L3DGUID.CPP
    GETDXVER.CPP
    LIB3D.CPP
    ${BOB_SRC}/3D/MONOTXT.CPP
)
target_include_directories(bob_lib3d PUBLIC
    ${BOB_SRC}/compat
    ${BOB_SRC}/H
    ${CMAKE_CURRENT_SOURCE_DIR}
)
