# MOVECODE module - flight model / movement / autopilot / scoring.
# Faithful to BOB.DSP: _MOVE.CPP is the unity TU that #includes the fragment
# files (Automove, Flyinit, Flymodel, Keyfly, Moveall, Wpaction — several have
# no own #includes and only compile in this unity context). SAGMOVE.CPP is a
# separate standalone TU in the DSP. Compiling _MOVE.CPP (not the fragments
# individually) avoids duplicate symbols and covers the fragment-only files.
add_library(bob_movecode STATIC
    _MOVE.CPP
    SAGMOVE.CPP
)
target_include_directories(bob_movecode PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
