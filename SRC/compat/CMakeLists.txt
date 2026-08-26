# Compat implementation objects (the headers live here too, on the global -I path).
#  - cstring_impl.cpp : MFC-compatible CString (impl lived in mfc42.lib on Windows)
#  - bob_stubs.cpp    : case-insensitive file I/O + _findfirst family + _itoa,
#                       DirectX creation entry stubs, and the GUIDs/globals the
#                       game references by name.
#  - bob_video.cpp    : DirectDraw7/Direct3D7 -> SDL2 + OpenGL backend.
#  - bob_resources.cpp: minimal Win32 PE resource loader (boblang.dll strings).
add_library(bob_compat STATIC
    cstring_impl.cpp
    bob_stubs.cpp
    bob_video.cpp
    bob_acmi.cpp
    bob_msgmap.cpp
    bob_resources.cpp
    bob_threads.cpp
    bob_gdi_font.cpp
    bob_gdi_blit.cpp      # R6.1: GDI blit subsystem (CBitmap/BitBlt/CreateDIBitmap)
    openal_dsound.cpp     # DirectSound -> OpenAL audio backend
    bob_dlgtemplate.cpp   # runtime .rc dialog-template parser (control positions)
    bob_music.cpp         # DirectMusic (game music path) -> FluidSynth
    bob_eventsink.cpp     # S33: general OCX-event routing (ON_EVENT -> dialog handler via RTTI)
)
target_include_directories(bob_compat PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
# the dialog-template parser reads the .rc/RESOURCE.H from the source tree at runtime
set_source_files_properties(bob_dlgtemplate.cpp PROPERTIES
    COMPILE_DEFINITIONS "BOB_SRC_DIR=\"${BOB_SRC}\"")
# SDL2 + system OpenGL for the video backend (no sdl2.pc on this box -> link by name).
target_include_directories(bob_compat PUBLIC /usr/include/SDL2)
