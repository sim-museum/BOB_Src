# AI module - artificial intelligence helpers.
# MSGAI (Art_Int instance + AirStruc/ArtInt methods) and USERMSG (message AI) now
# build: each got a BOB_LINUX prelude reproducing the original PCH (dosdefs base
# types + world/ai/model/anim/3dcom headers), a ViewPoint fwd-decl before
# worldinc.h, INSTANCEAI made variadic (unused 3rd arg), ArtInt data -> protected
# (INSTANCEAI handlers derive : public ArtInt), and for-scope hoists.
add_library(bob_ai STATIC
    AAA.CPP
    GRID.CPP
    MSGAI.CPP
    USERMSG.CPP
)
target_include_directories(bob_ai PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
