# Compat implementation objects (the headers live here too, on the global -I path).
#  - cstring_impl.cpp : MFC-compatible CString (impl lived in mfc42.lib on Windows)
#  - bob_stubs.cpp    : case-insensitive file I/O + _findfirst family + _itoa,
#                       DirectX creation entry stubs, and the GUIDs/globals the
#                       game references by name.
add_library(bob_compat STATIC
    cstring_impl.cpp
    bob_stubs.cpp
)
target_include_directories(bob_compat PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
