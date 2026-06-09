/* FreeFalcon Linux Port - mmsystem.h compatibility */
#ifndef FF_COMPAT_MMSYSTEM_H
#define FF_COMPAT_MMSYSTEM_H
#ifdef FF_LINUX

#include "compat_types.h"
#include "compat_winbase.h"  /* timeGetTime / timeBeginPeriod / timeEndPeriod */

typedef UINT MMRESULT;
#define MMSYSERR_NOERROR  0
#define MMSYSERR_ERROR    1
#define TIMERR_NOERROR    0

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
#pragma pack(push, 1)
typedef struct tWAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;
#pragma pack(pop)
#endif

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

#ifndef _PCMWAVEFORMAT_
#define _PCMWAVEFORMAT_
#pragma pack(push, 1)
typedef struct waveformat_tag {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
} WAVEFORMAT, *PWAVEFORMAT, *LPWAVEFORMAT;

typedef struct pcmwaveformat_tag {
    WAVEFORMAT wf;
    WORD       wBitsPerSample;
} PCMWAVEFORMAT, *PPCMWAVEFORMAT, *LPPCMWAVEFORMAT;
#pragma pack(pop)
#endif /* _PCMWAVEFORMAT_ */

typedef struct timecaps_tag {
    UINT wPeriodMin;
    UINT wPeriodMax;
} TIMECAPS, *PTIMECAPS, *LPTIMECAPS;

static inline MMRESULT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) {
    (void)cbtc;
    if (ptc) { ptc->wPeriodMin = 1; ptc->wPeriodMax = 1000000; }
    return MMSYSERR_NOERROR;
}

/* PlaySound stubs */
#define SND_SYNC      0x0000
#define SND_ASYNC     0x0001
#define SND_NODEFAULT 0x0002
#define SND_LOOP      0x0008
#define SND_PURGE     0x0040
#define SND_FILENAME  0x00020000
static inline BOOL PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound) {
    (void)pszSound; (void)hmod; (void)fdwSound;
    return TRUE;
}
#define PlaySound PlaySoundA
static inline BOOL sndPlaySoundA(LPCSTR pszSound, UINT fuSound) { (void)pszSound; (void)fuSound; return TRUE; }
#define sndPlaySound sndPlaySoundA

/* Joystick API (mmsystem.h) */
#ifndef JOY_RETURNX
#define JOY_RETURNX        0x00000001
#define JOY_RETURNY        0x00000002
#define JOY_RETURNZ        0x00000004
#define JOY_RETURNR        0x00000008
#define JOY_RETURNU        0x00000010
#define JOY_RETURNV        0x00000020
#define JOY_RETURNPOV      0x00000040
#define JOY_RETURNBUTTONS  0x00000080
#define JOY_RETURNALL      (JOY_RETURNX|JOY_RETURNY|JOY_RETURNZ|JOY_RETURNR|JOY_RETURNU|JOY_RETURNV|JOY_RETURNPOV|JOY_RETURNBUTTONS)
#define JOYERR_NOERROR     0
#define JOYERR_PARMS       165
#define JOYERR_NOCANDO     166
#define JOYERR_UNPLUGGED   167
#endif

typedef struct joyinfo_tag {
    UINT wXpos;
    UINT wYpos;
    UINT wZpos;
    UINT wButtons;
} JOYINFO, *PJOYINFO, *LPJOYINFO;

typedef struct joyinfoex_tag {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwXpos;
    DWORD dwYpos;
    DWORD dwZpos;
    DWORD dwRpos;
    DWORD dwUpos;
    DWORD dwVpos;
    DWORD dwButtons;
    DWORD dwButtonNumber;
    DWORD dwPOV;
    DWORD dwReserved1;
    DWORD dwReserved2;
} JOYINFOEX, *PJOYINFOEX, *LPJOYINFOEX;

static inline MMRESULT joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji) { (void)uJoyID; (void)pji; return JOYERR_UNPLUGGED; }
static inline MMRESULT joyGetPos(UINT uJoyID, LPJOYINFO pji)     { (void)uJoyID; (void)pji; return JOYERR_UNPLUGGED; }
static inline UINT     joyGetNumDevs(void) { return 0; }

#endif /* FF_LINUX */
#endif
