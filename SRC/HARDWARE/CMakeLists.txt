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
# S48: the Sample::LoadBuffer 20-byte stack over-write (PCMWAVEFORMAT->WAVEFORMATEX
# copy) is now fixed at source (SAMPLE.CPP, #if BOB_LINUX), so -fstack-protector is
# safe again and re-enabled (default). Was: -fno-stack-protector to mask the overflow.
