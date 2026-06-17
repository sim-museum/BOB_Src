# HARDWARE module - sound (DirectSound/OpenAL), music (MIDI/DirectMusic),
# radio chatter, soundfonts, message engine.
# Faithful to BOB.DSP: _HARD.CPP is the unity TU (digdrvr/sample/music/Sound/
# Rchatter/Sfonts/messengn). The legacy pre-DirectDraw VGA files (HARD320*,
# Hardpub, Hardwin) are commented out of the unity and are not in the game build.
# MIDI/DirectMusic playback is stubbed in compat (mmsystem.h) for now.
add_library(bob_hardware STATIC
    _HARD.CPP
)
target_include_directories(bob_hardware PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
# Sample::LoadBuffer does `*((PCMWAVEFORMAT*)&tmpwf) = wavformat` where tmpwf is a
# WAVEFORMATEX (18 bytes, pack(1)) but PCMWAVEFORMAT is 20 bytes -> a benign 2-byte
# stack overflow, harmless on Windows (no stack protector) but tripping -fstack-protector
# here (__stack_chk_fail abort once sound plays). Game code stays pristine; disable the
# protector for this TU to match the original behaviour.
target_compile_options(bob_hardware PRIVATE -fno-stack-protector)
