# FILES module - file system / archive loading / CD access.
# FILEI.CPP and _FILE.CPP are unity aggregators (excluded).
add_library(bob_files STATIC
    DOSFILE.CPP
    LOADLIST.CPP
    DISKIO.CPP
    LOADLIB.CPP
    WINFILE.CPP
    FILEMAN.CPP
)
target_include_directories(bob_files PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
