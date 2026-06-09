# MODEL module - aircraft flight model, engine, gear, cockpit, AI.
# Faithful to BOB.DSP: _MODE.CPP is the unity TU that #includes all 16 fragment
# files (Acmai/Acmman/Acmmodel/Acmsimpl/Aerodev/Ctrl/Curves/Elmt/Engine/Gear/
# Model/Modint/Modvec/Sky/Intcock). Compiling the unity gives every fragment its
# include context and avoids duplicate symbols. _MODEL.CPP/MODES.CPP are
# alternate (non-DSP) aggregators.
add_library(bob_model STATIC
    _MODE.CPP
)
target_include_directories(bob_model PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
