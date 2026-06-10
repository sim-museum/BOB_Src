# AI module - artificial intelligence helpers.
# _AI.CPP is the unity TU that #includes the fragments (Aaa/Msgai/Spotted/usermsg/
# grid) — it covers Spotted.cpp (ArtInt spotting AI) which has no standalone build.
# The fragments' earlier fixes: MSGAI/USERMSG got a BOB_LINUX prelude reproducing
# the PCH; INSTANCEAI made variadic; ArtInt data -> protected; for-scope hoists.
add_library(bob_ai STATIC
    _AI.CPP
)
target_include_directories(bob_ai PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
