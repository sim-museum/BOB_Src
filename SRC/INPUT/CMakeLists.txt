# INPUT module - analog joystick (DirectInput/force-feedback) + key list.
# KEYSTUB.CPP is a unity fragment (#included by MFC/_MFC.CPP), not built here.
# _INPU.CPP is a unity aggregator and is excluded.
add_library(bob_input STATIC
    ANALOGUE.CPP
    KEYLIST.CPP
)
target_include_directories(bob_input PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
