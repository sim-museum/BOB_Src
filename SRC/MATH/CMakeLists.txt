# MATH module - fixed-point/vector math + the matrix transform asm.
# NOTE: MATHU.CPP is a corrupted unity-include stub (literal "SOURCE=.." paths)
# and is excluded; MATH.CPP and MATRIX.CPP are compiled as separate TUs.
add_library(bob_math STATIC
    MATH.CPP
    MATRIX.CPP
    matrasm.nasm
)
target_include_directories(bob_math PUBLIC
    ${BOB_SRC}/compat
    ${BOB_SRC}/H
)
