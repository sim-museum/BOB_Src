# AI module - artificial intelligence helpers.
# MSGAI/USERMSG deferred (non-self-contained: MissManCampSky/MMC need missman2.h,
# incomplete Model/anim types, rchatter.h ordering). AI.CPP/_AI.CPP are unity.
add_library(bob_ai STATIC
    AAA.CPP
    GRID.CPP
)
target_include_directories(bob_ai PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
