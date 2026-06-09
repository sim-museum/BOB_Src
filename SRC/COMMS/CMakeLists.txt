# COMMS module - networking / DirectPlay multiplayer, replay, aggregator.
# Faithful to BOB.DSP: _COMM.CPP is the unity TU (Aggrgtor/Comms/Replay/Winmove).
# DirectPlay is stubbed in compat (dplay.h/dplobby.h minimal IDirectPlayLobby3) —
# multiplayer is non-functional on Linux for now (CoCreateInstance fails), but the
# code compiles. To be fully implemented or kept stubbed per the phased plan.
add_library(bob_comms STATIC
    _COMM.CPP
)
target_include_directories(bob_comms PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
