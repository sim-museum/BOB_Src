# GENERAL module - allocator, static data, win-error helpers.
# GENEC.CPP and _GENE.CPP are unity aggregators (#include the individual .cpp's).
add_library(bob_general STATIC
    MALLOC.CPP
    STATIC.CPP
    WINERROR.CPP
)
target_include_directories(bob_general PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
