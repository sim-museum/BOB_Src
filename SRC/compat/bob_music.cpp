/* bob_music.cpp — BoB Linux port: the game's MIDI music path on FluidSynth.
 *
 * WHAT THE GAME ACTUALLY USES.  Unlike the sister MiG Alley port (which drives music
 * through the Miles `AIL_*` sequence API), Battle of Britain's music layer is
 * **DirectMusic**: SRC/HARDWARE/MUSIC.CPP's `Music` class does
 *
 *     CoCreateInstance(CLSID_DirectMusicPerformance) -> IDirectMusicPerformance
 *     gpPerf->Init(&gpDMusic, <IDirectSound>, NULL)
 *     CoCreateInstance(CLSID_DirectMusicLoader)      -> IDirectMusicLoader
 *     gpDMusic->EnumPort/GetDefaultPort/CreatePort   -> IDirectMusicPort (+Activate,
 *                                                       AddPort, AssignPChannelBlock)
 *     gpLoader->GetObject(DMUS_OBJ_MEMORY blob)      -> IDirectMusicSegment
 *     gpPerf->PlaySegment / Stop / IsPlaying / SetGlobalParam(GUID_PerfMasterVolume)
 *
 * and the music payload is handed to the loader **in memory** — SOUND.CPP's `LoadTune`
 * pulls the numbered file out of the MUSIC directory (DIR.DIR index, FILEMAN.CPP) and
 * passes (ptr,size) straight to `Music::Play`.  So the natural place to back the game
 * with a soft-synth is right here: implement those five COM interfaces over FluidSynth.
 * The game code stays untouched.
 *
 * WHAT IS ADOPTED FROM MiG ALLEY: the hard part — the in-memory XMI -> SMF converter
 * (`xmi_to_smf`, from ~/ma/SRC/compat/ma_music.cpp, FluidSynth cannot read XMI) and the
 * SoundFont fallback chain.  The COM plumbing above is BoB-specific and new.
 *
 * DEGRADATION: if FluidSynth won't initialise, if no SoundFont resolves, or if
 * BOB_NOMUSIC is set, `bob_com_create_instance` fails the CLSID_DirectMusicPerformance
 * request exactly as the old stub CoCreateInstance did -> `Music::Init` returns false ->
 * MusicAllowed stays false -> the game runs silent.  Bare `./bob` is unaffected.
 *
 * Env (all default-off / default-safe, per the port's conventions):
 *   BOB_NOMUSIC        disable the music path entirely (revert to the old stub)
 *   BOB_SOUNDFONT      override the SoundFont (else the system GM chain below)
 *   BOB_TRACE_MUSIC    `[music]` trace lines
 *   BOB_FLUID_DRIVER   force a FluidSynth audio driver (pulseaudio/alsa/sdl2/file/...)
 *   BOB_FLUID_FILE     with BOB_FLUID_DRIVER=file, the .wav to render into
 *   BOB_MUSIC_PPQN     XMI->SMF tick base (default 60, as MiG Alley)
 */
#ifdef FF_LINUX

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include "Dmusici.h"

#include <fluidsynth.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>   /* usleep (selftest) */

/* The two CLSIDs the game CoCreateInstance()s are declared (not defined) by the SDK
   header; nothing else in the tree defines them, so define them here with their real
   DirectMusic values (SRC/H/DMUSICI.H:997,1013).  We do NOT rely on the *values* of the
   other DirectMusic GUIDs: bob_stubs.cpp defines several of them (CLSID_DirectMusicSegment,
   IID_IDirectMusicSegment, GUID_StandardMIDIFile, GUID_PerfMasterVolume, ...) as all-zero,
   so GUID equality is meaningless for them — the code below keys off call sites instead. */
extern const GUID CLSID_DirectMusicPerformance;
extern const GUID CLSID_DirectMusicLoader;
const GUID CLSID_DirectMusicPerformance =
	{0xd2ac2881,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
const GUID CLSID_DirectMusicLoader =
	{0xd2ac2892,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
/* ...and the two IIDs the same call sites pass (DMUSICI.H:1098,1104). Now that
   CoCreateInstance actually *reads* its GUID arguments (it used to ignore them, so the
   compiler elided every reference), these have to exist. */
extern const GUID IID_IDirectMusicPerformance;
extern const GUID IID_IDirectMusicLoader;
const GUID IID_IDirectMusicPerformance =
	{0x07d43d03,0x6523,0x11d2,{0x87,0x1d,0x00,0x60,0x08,0x93,0xb1,0xbd}};
const GUID IID_IDirectMusicLoader =
	{0x2ffaaca2,0x5dca,0x11d2,{0xaf,0xa6,0x00,0xaa,0x00,0x24,0xd8,0xb6}};

/* our single synthetic port GUID (distinct + non-zero — the same keystone as the
   DirectInput device GUIDs in bob_stubs.cpp: an all-zero GUID compares equal to
   everything, and Save_Data.MusicDevice round-trips this value) */
static const GUID BOB_FLUID_PORT_GUID =
	{0xb0bf1d01,0x0001,0x4000,{0xb0,0x0b,0xf1,0x11,0xd5,0x79,0x00,0x01}};

static int mtrace(void) { static int t = -1; if (t < 0) t = getenv("BOB_TRACE_MUSIC") ? 1 : 0; return t; }
#define MTRACE(...) do { if (mtrace()) fprintf(stderr, __VA_ARGS__); } while (0)

/* ---- FluidSynth global state ------------------------------------------------ */
static fluid_settings_t*     g_set   = 0;
static fluid_synth_t*        g_syn   = 0;
static fluid_audio_driver_t* g_drv   = 0;
static fluid_player_t*       g_play  = 0;
static int                   g_sfont = -1;
static int                   g_ready = 0;
static float                 g_gain  = 0.4f;   /* base gain, scaled by the game's volume */

/* ---- XMIDI -> SMF conversion (adopted verbatim in spirit from ~/ma/SRC/compat/
   ma_music.cpp; FluidSynth plays Standard MIDI files, not XMI) ----------------- */
typedef uint32_t U32;
struct Buf { unsigned char* d; size_t n, cap; };
static void bput(Buf* b, unsigned char v){ if(b->n>=b->cap){ b->cap=b->cap?b->cap*2:1024; b->d=(unsigned char*)realloc(b->d,b->cap);} b->d[b->n++]=v; }
static void bput_be32(Buf* b, U32 v){ bput(b,v>>24); bput(b,v>>16); bput(b,v>>8); bput(b,v); }
static void bput_be16(Buf* b, U32 v){ bput(b,v>>8); bput(b,v); }
static void bput_vlq(Buf* b, U32 v){ unsigned char s[5]; int i=0; s[i++]=v&0x7F; v>>=7; while(v){ s[i++]=0x80|(v&0x7F); v>>=7; } while(i) bput(b,s[--i]); }

struct Ev { U32 t; int order; unsigned char* bytes; U32 len; };

static U32 rd_be32(const unsigned char* p){ return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]; }

/* Find the first EVNT chunk, recursing into IFF containers (FORM/CAT/LIST). XMI nests
   the event stream as FORM XDIR / CAT XMID / FORM XMID / EVNT, so a flat scan misses it. */
static const unsigned char* find_evnt(const unsigned char* p, const unsigned char* end, U32* len) {
	while (p + 8 <= end) {
		U32 sz = rd_be32(p+4);
		const unsigned char* body = p + 8;
		if (body + sz > end) sz = (U32)(end - body);          /* clamp to bounds */
		if (!memcmp(p, "EVNT", 4)) { *len = sz; return body; }
		if (!memcmp(p, "FORM", 4) || !memcmp(p, "CAT ", 4) || !memcmp(p, "LIST", 4)) {
			const unsigned char* r = find_evnt(body + 4, body + sz, len);
			if (r) return r;
		}
		p = body + sz + (sz & 1);
	}
	return 0;
}

static unsigned char* xmi_to_smf(const unsigned char* xmi, U32 xmilen, U32* outlen) {
	const unsigned char* end = xmi + xmilen;
	U32 elen = 0;
	const unsigned char* ev = find_evnt(xmi, end, &elen);
	if (!ev) return 0;
	const unsigned char* p = ev;
	const unsigned char* eend = ev + elen;

	int ppqn = getenv("BOB_MUSIC_PPQN") ? atoi(getenv("BOB_MUSIC_PPQN")) : 60;
	if (ppqn <= 0) ppqn = 60;

	Ev* evs = 0; size_t nev = 0, cap = 0;
	int order = 0;
	U32 abst = 0;
	#define ADD(_t, ...) do { unsigned char _b[] = {__VA_ARGS__}; \
		if(nev>=cap){cap=cap?cap*2:256; evs=(Ev*)realloc(evs,cap*sizeof(Ev));} \
		evs[nev].t=(_t); evs[nev].order=order++; evs[nev].len=sizeof(_b); \
		evs[nev].bytes=(unsigned char*)malloc(sizeof(_b)); memcpy(evs[nev].bytes,_b,sizeof(_b)); nev++; } while(0)
	#define ADDBLOB(_t, _ptr, _len) do { \
		if(nev>=cap){cap=cap?cap*2:256; evs=(Ev*)realloc(evs,cap*sizeof(Ev));} \
		evs[nev].t=(_t); evs[nev].order=order++; evs[nev].len=(_len); \
		evs[nev].bytes=(unsigned char*)malloc(_len); memcpy(evs[nev].bytes,(_ptr),(_len)); nev++; } while(0)

	while (p < eend) {
		/* XMIDI delay: consecutive bytes < 0x80 accumulate */
		while (p < eend && *p < 0x80) { abst += *p; p++; }
		if (p >= eend) break;
		unsigned char st = *p++;
		int hi = st & 0xF0;
		if (st == 0xFF) {                          /* meta */
			if (p >= eend) break;
			unsigned char mt = *p++;
			U32 ln = 0; while (p < eend) { unsigned char c=*p++; ln=(ln<<7)|(c&0x7F); if(!(c&0x80)) break; }
			Buf mb = {0,0,0};
			bput(&mb,0xFF); bput(&mb,mt); bput_vlq(&mb, ln);
			for (U32 i=0;i<ln && p<eend;i++) bput(&mb,*p++);
			ADDBLOB(abst, mb.d, mb.n); free(mb.d);
			if (mt == 0x2F) break;                 /* end of track */
		} else if (st == 0xF0 || st == 0xF7) {     /* sysex */
			U32 ln = 0; while (p < eend) { unsigned char c=*p++; ln=(ln<<7)|(c&0x7F); if(!(c&0x80)) break; }
			Buf sb = {0,0,0}; bput(&sb, st); bput_vlq(&sb, ln);
			for (U32 i=0;i<ln && p<eend;i++) bput(&sb,*p++);
			ADDBLOB(abst, sb.d, sb.n); free(sb.d);
		} else if (hi == 0x90) {                   /* note on (XMIDI: + duration) */
			if (p+1 >= eend) break;
			unsigned char note=*p++, vel=*p++;
			ADD(abst, st, note, vel);
			U32 dur=0; while (p < eend) { unsigned char c=*p++; dur=(dur<<7)|(c&0x7F); if(!(c&0x80)) break; }
			ADD(abst+dur, (unsigned char)(0x80|(st&0x0F)), note, 0x40);
		} else if (hi == 0x80 || hi == 0xA0 || hi == 0xB0 || hi == 0xE0) {
			if (p+1 >= eend) break;
			unsigned char d1=*p++, d2=*p++;
			ADD(abst, st, d1, d2);
		} else if (hi == 0xC0 || hi == 0xD0) {
			if (p >= eend) break;
			unsigned char d1=*p++;
			ADD(abst, st, d1);
		} else {
			break;                                 /* unknown -> stop */
		}
	}
	if (!nev) { free(evs); return 0; }

	/* stable sort by (tick, order) */
	for (size_t i=1;i<nev;i++){ Ev key=evs[i]; long j=(long)i-1;
		while (j>=0 && (evs[j].t>key.t || (evs[j].t==key.t && evs[j].order>key.order))) { evs[j+1]=evs[j]; j--; }
		evs[j+1]=key; }

	Buf trk = {0,0,0};
	U32 prev = 0;
	for (size_t i=0;i<nev;i++) {
		bput_vlq(&trk, evs[i].t - prev); prev = evs[i].t;
		for (U32 k=0;k<evs[i].len;k++) bput(&trk, evs[i].bytes[k]);
		free(evs[i].bytes);
	}
	free(evs);
	bput_vlq(&trk, 0); bput(&trk,0xFF); bput(&trk,0x2F); bput(&trk,0x00);

	Buf out = {0,0,0};
	const char* MThd="MThd"; for(int i=0;i<4;i++) bput(&out,MThd[i]);
	bput_be32(&out, 6); bput_be16(&out, 0); bput_be16(&out, 1); bput_be16(&out, (U32)ppqn);
	const char* MTrk="MTrk"; for(int i=0;i<4;i++) bput(&out,MTrk[i]);
	bput_be32(&out, (U32)trk.n);
	for (size_t i=0;i<trk.n;i++) bput(&out, trk.d[i]);
	free(trk.d);
	*outlen = (U32)out.n;
	return out.d;
}

/* Turn a raw music blob into an SMF image we can hand to a fluid_player.
   Accepts XMI (IFF "FORM") and plain Standard MIDI ("MThd") — DirectMusic accepted both,
   and the DIR.DIR index in this install names .xmi. Returns malloc'd buffer or NULL. */
static unsigned char* blob_to_smf(const void* data, int len, U32* outlen) {
	const unsigned char* b = (const unsigned char*)data;
	if (!b || len < 12) return 0;
	if (!memcmp(b, "MThd", 4)) {
		unsigned char* c = (unsigned char*)malloc((size_t)len);
		if (!c) return 0;
		memcpy(c, b, (size_t)len); *outlen = (U32)len;
		return c;
	}
	if (!memcmp(b, "FORM", 4)) return xmi_to_smf(b, (U32)len, outlen);
	return 0;
}

/* ---- FluidSynth bring-up ---------------------------------------------------- */
static int music_init(void) {
	if (g_ready) return 1;
	if (getenv("BOB_NOMUSIC") || getenv("BOB_NOSOUND")) return 0;
	static int failed = 0;
	if (failed) return 0;                       /* don't retry every CoCreateInstance */

	g_set = new_fluid_settings();
	if (!g_set) { failed = 1; return 0; }
	fluid_settings_setnum(g_set, "synth.gain", g_gain);
	const char* fdrv = getenv("BOB_FLUID_DRIVER");
	const char* ffile = getenv("BOB_FLUID_FILE");
	if (ffile && *ffile) fluid_settings_setstr(g_set, "audio.file.name", ffile);

	g_syn = new_fluid_synth(g_set);
	if (!g_syn) { MTRACE("[music] new_fluid_synth failed -> no music\n"); goto fail; }

	/* SoundFont chain (adopted from ~/ma/SRC/compat/ma_music.cpp:196-202): explicit
	   override, then the system General-MIDI banks. MiG Alley's last resort was the
	   game's own MUSIC/fieldsnr.sf2 (a single custom preset, not a GM bank); BoB's
	   install ships no .sf2 at all, so a system GM bank is the only option — but keep
	   the game-relative probe in case an install has one. */
	{
		const char* sfo = getenv("BOB_SOUNDFONT");
		if (sfo && *sfo) g_sfont = fluid_synth_sfload(g_syn, sfo, 1);
		static const char* cands[] = {
			"/usr/share/sounds/sf2/default-GM.sf2",
			"/usr/share/sounds/sf2/FluidR3_GM.sf2",
			"/usr/share/sounds/sf2/TimGM6mb.sf2",
			"/usr/share/soundfonts/default.sf2",
			"MUSIC/fieldsnr.sf2",
			0 };
		for (int i = 0; cands[i] && g_sfont < 0; i++)
			g_sfont = fluid_synth_sfload(g_syn, cands[i], 1);
	}
	if (g_sfont < 0) {
		MTRACE("[music] no SoundFont found -> no music (set BOB_SOUNDFONT)\n");
		goto fail;
	}

	/* Audio driver. FluidSynth renders through its own driver, alongside (not through)
	   the OpenAL SFX path in openal_dsound.cpp. Try the explicit override, then the
	   backends this box is likely to have; every one failing = silent degradation. */
	{
		const char* cands[5]; int n = 0;
		if (fdrv && *fdrv) cands[n++] = fdrv;
		else { cands[n++] = "pulseaudio"; cands[n++] = "alsa"; cands[n++] = "sdl2"; }
		cands[n] = 0;
		for (int i = 0; i < n && !g_drv; i++) {
			fluid_settings_setstr(g_set, "audio.driver", cands[i]);
			g_drv = new_fluid_audio_driver(g_set, g_syn);
			if (g_drv) MTRACE("[music] audio.driver=%s\n", cands[i]);
		}
	}
	if (!g_drv) { MTRACE("[music] no usable FluidSynth audio driver -> no music\n"); goto fail; }

	g_ready = 1;
	MTRACE("[music] FluidSynth ready (soundfont id=%d)\n", g_sfont);
	return 1;

fail:
	if (g_drv) { delete_fluid_audio_driver(g_drv); g_drv = 0; }
	if (g_syn) { delete_fluid_synth(g_syn); g_syn = 0; }
	if (g_set) { delete_fluid_settings(g_set); g_set = 0; }
	g_sfont = -1; failed = 1;
	return 0;
}

static void music_stop(void) {
	if (g_play) { fluid_player_stop(g_play); delete_fluid_player(g_play); g_play = 0; }
	if (g_syn) fluid_synth_system_reset(g_syn);
}

/* ============================================================================
 * The DirectMusic COM objects.
 *
 * NOTE ON HRESULTs: MUSIC.CPP treats several failures as FATAL
 * (`_Error.SayAndQuit("GetObject failed")`, "download on segment failed",
 * "playsegment failed", "Could not create/activate/add port"). So every method the
 * game checks returns S_OK even when there is nothing to do — a music blob we can't
 * parse yields a *silent* segment rather than a quit. Only Performance::Init (and the
 * CoCreateInstance for the performance) fail, and the game handles those gracefully.
 * ==========================================================================*/

struct BobSegment : public IDirectMusicSegment {
	long ref;
	unsigned char* smf;
	U32 smflen;
	BobSegment() : ref(1), smf(0), smflen(0) {}
	~BobSegment() { free(smf); }

	STDMETHODIMP QueryInterface(REFIID, LPVOID FAR* ppv) {
		/* the only QI the game does is ->IID_IDirectMusicObject (to hand the object to
		   IDirectMusicLoader::ReleaseObject); the stub GUIDs are all-zero so we can't
		   discriminate — return ourselves, ReleaseObject just drops the ref. */
		if (!ppv) return E_POINTER;
		*ppv = (void*)this; ref++; return S_OK;
	}
	STDMETHODIMP_(ULONG) AddRef(void)  { return (ULONG)++ref; }
	STDMETHODIMP_(ULONG) Release(void) { if (--ref <= 0) { delete this; return 0; } return (ULONG)ref; }

	STDMETHODIMP GetLength(MUSIC_TIME* p)            { if (p) *p = 0; return S_OK; }
	STDMETHODIMP SetLength(MUSIC_TIME)               { return S_OK; }
	STDMETHODIMP GetRepeats(DWORD* p)                { if (p) *p = 0; return S_OK; }
	STDMETHODIMP SetRepeats(DWORD)                   { return S_OK; }
	STDMETHODIMP GetDefaultResolution(DWORD* p)      { if (p) *p = 0; return S_OK; }
	STDMETHODIMP SetDefaultResolution(DWORD)         { return S_OK; }
	STDMETHODIMP GetTrack(REFGUID, DWORD, DWORD, IDirectMusicTrack** pp) { if (pp) *pp = 0; return E_NOTIMPL; }
	STDMETHODIMP GetTrackGroup(IDirectMusicTrack*, DWORD* p) { if (p) *p = 0; return E_NOTIMPL; }
	STDMETHODIMP InsertTrack(IDirectMusicTrack*, DWORD)      { return E_NOTIMPL; }
	STDMETHODIMP RemoveTrack(IDirectMusicTrack*)             { return E_NOTIMPL; }
	STDMETHODIMP InitPlay(IDirectMusicSegmentState** pp, IDirectMusicPerformance*, DWORD) { if (pp) *pp = 0; return S_OK; }
	STDMETHODIMP GetGraph(IDirectMusicGraph** pp)    { if (pp) *pp = 0; return E_NOTIMPL; }
	STDMETHODIMP SetGraph(IDirectMusicGraph*)        { return S_OK; }
	STDMETHODIMP AddNotificationType(REFGUID)        { return S_OK; }
	STDMETHODIMP RemoveNotificationType(REFGUID)     { return S_OK; }
	STDMETHODIMP GetParam(REFGUID, DWORD, DWORD, MUSIC_TIME, MUSIC_TIME*, void*) { return S_OK; }
	STDMETHODIMP SetParam(REFGUID, DWORD, DWORD, MUSIC_TIME, void*)              { return S_OK; }
	STDMETHODIMP Clone(MUSIC_TIME, MUSIC_TIME, IDirectMusicSegment** pp)         { if (pp) *pp = 0; return E_NOTIMPL; }
	STDMETHODIMP SetStartPoint(MUSIC_TIME)           { return S_OK; }
	STDMETHODIMP GetStartPoint(MUSIC_TIME* p)        { if (p) *p = 0; return S_OK; }
	STDMETHODIMP SetLoopPoints(MUSIC_TIME, MUSIC_TIME) { return S_OK; }
	STDMETHODIMP GetLoopPoints(MUSIC_TIME* a, MUSIC_TIME* b) { if (a) *a = 0; if (b) *b = 0; return S_OK; }
	STDMETHODIMP SetPChannelsUsed(DWORD, DWORD*)     { return S_OK; }
};

struct BobPort : public IDirectMusicPort {
	long ref;
	BobPort() : ref(1) {}
	STDMETHODIMP QueryInterface(REFIID, LPVOID FAR* ppv) { if (!ppv) return E_POINTER; *ppv = (void*)this; ref++; return S_OK; }
	STDMETHODIMP_(ULONG) AddRef(void)  { return (ULONG)++ref; }
	STDMETHODIMP_(ULONG) Release(void) { if (--ref <= 0) { delete this; return 0; } return (ULONG)ref; }

	STDMETHODIMP PlayBuffer(LPDIRECTMUSICBUFFER)              { return S_OK; }
	STDMETHODIMP SetReadNotificationHandle(HANDLE)            { return S_OK; }
	STDMETHODIMP Read(LPDIRECTMUSICBUFFER)                    { return S_FALSE; }
	STDMETHODIMP DownloadInstrument(IDirectMusicInstrument*, IDirectMusicDownloadedInstrument** pp,
	                                DMUS_NOTERANGE*, DWORD)   { if (pp) *pp = 0; return S_OK; }
	STDMETHODIMP UnloadInstrument(IDirectMusicDownloadedInstrument*) { return S_OK; }
	STDMETHODIMP GetLatencyClock(IReferenceClock** pp)        { if (pp) *pp = 0; return E_NOTIMPL; }
	STDMETHODIMP GetRunningStats(LPDMUS_SYNTHSTATS)           { return E_NOTIMPL; }
	STDMETHODIMP Compact(void)                                { return S_OK; }
	STDMETHODIMP GetCaps(LPDMUS_PORTCAPS pc);
	STDMETHODIMP DeviceIoControl(DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPOVERLAPPED) { return E_NOTIMPL; }
	STDMETHODIMP SetNumChannelGroups(DWORD)                   { return S_OK; }
	STDMETHODIMP GetNumChannelGroups(LPDWORD p)               { if (p) *p = 1; return S_OK; }
	STDMETHODIMP Activate(BOOL)                               { return S_OK; }
	STDMETHODIMP SetChannelPriority(DWORD, DWORD, DWORD)      { return S_OK; }
	STDMETHODIMP GetChannelPriority(DWORD, DWORD, LPDWORD p)  { if (p) *p = 0; return S_OK; }
	STDMETHODIMP SetDirectSound(LPDIRECTSOUND, LPDIRECTSOUNDBUFFER) { return S_OK; }
	STDMETHODIMP GetFormat(LPWAVEFORMATEX, LPDWORD, LPDWORD)  { return E_NOTIMPL; }
};

static void fill_caps(LPDMUS_PORTCAPS pc) {
	if (!pc) return;
	DWORD sz = pc->dwSize;
	memset(pc, 0, sizeof(DMUS_PORTCAPS));
	pc->dwSize = sz ? sz : sizeof(DMUS_PORTCAPS);
	pc->guidPort = BOB_FLUID_PORT_GUID;
	pc->dwClass  = DMUS_PC_OUTPUTCLASS;
	/* Advertise a DLS-capable software synth: MUSIC.CPP's Play() takes the DLS branch,
	   which is the one that calls SetParam(GUID_StandardMIDIFile) once and then plays.
	   (Without DMUS_PC_DLS it re-runs AttemptFileOpen, loading the blob twice.) */
	pc->dwFlags  = DMUS_PC_DLS | DMUS_PC_SOFTWARESYNTH | DMUS_PC_DIRECTSOUND;
	pc->dwMaxChannelGroups = 1;
	pc->dwMaxVoices        = 256;
	pc->dwMaxAudioChannels = 2;
	const char* name = "FluidSynth (Linux port)";
	int i = 0; for (; name[i] && i < DMUS_MAX_DESCRIPTION - 1; i++) pc->wszDescription[i] = (WCHAR)name[i];
	pc->wszDescription[i] = 0;
}
STDMETHODIMP BobPort::GetCaps(LPDMUS_PORTCAPS pc) { fill_caps(pc); return S_OK; }

struct BobDMusic : public IDirectMusic {
	long ref;
	BobDMusic() : ref(1) {}
	STDMETHODIMP QueryInterface(REFIID, LPVOID FAR* ppv) { if (!ppv) return E_POINTER; *ppv = (void*)this; ref++; return S_OK; }
	STDMETHODIMP_(ULONG) AddRef(void)  { return (ULONG)++ref; }
	STDMETHODIMP_(ULONG) Release(void) { if (--ref <= 0) { delete this; return 0; } return (ULONG)ref; }

	STDMETHODIMP EnumPort(DWORD dwIndex, LPDMUS_PORTCAPS pc) {
		if (dwIndex != 0) return S_FALSE;      /* exactly one port */
		fill_caps(pc); return S_OK;
	}
	STDMETHODIMP CreateMusicBuffer(LPDMUS_BUFFERDESC, LPDIRECTMUSICBUFFER* pp, LPUNKNOWN) { if (pp) *pp = 0; return E_NOTIMPL; }
	STDMETHODIMP CreatePort(REFCLSID, LPDMUS_PORTPARAMS, LPDIRECTMUSICPORT* pp, LPUNKNOWN) {
		if (!pp) return E_POINTER;
		*pp = new BobPort();
		MTRACE("[music] CreatePort -> %p\n", (void*)*pp);
		return S_OK;
	}
	STDMETHODIMP EnumMasterClock(DWORD, LPDMUS_CLOCKINFO) { return S_FALSE; }
	STDMETHODIMP GetMasterClock(LPGUID, IReferenceClock** pp) { if (pp) *pp = 0; return E_NOTIMPL; }
	STDMETHODIMP SetMasterClock(REFGUID)  { return S_OK; }
	STDMETHODIMP Activate(BOOL)           { return S_OK; }
	STDMETHODIMP GetDefaultPort(LPGUID pg) { if (!pg) return E_POINTER; *pg = BOB_FLUID_PORT_GUID; return S_OK; }
	STDMETHODIMP SetDirectSound(LPDIRECTSOUND, HWND) { return S_OK; }
};

struct BobLoader : public IDirectMusicLoader {
	long ref;
	BobLoader() : ref(1) {}
	STDMETHODIMP QueryInterface(REFIID, LPVOID FAR* ppv) { if (!ppv) return E_POINTER; *ppv = (void*)this; ref++; return S_OK; }
	STDMETHODIMP_(ULONG) AddRef(void)  { return (ULONG)++ref; }
	STDMETHODIMP_(ULONG) Release(void) { if (--ref <= 0) { delete this; return 0; } return (ULONG)ref; }

	STDMETHODIMP GetObject(LPDMUS_OBJECTDESC pDesc, REFIID, LPVOID FAR* ppv) {
		if (!ppv) return E_POINTER;
		BobSegment* seg = new BobSegment();
		if (pDesc && (pDesc->dwValidData & DMUS_OBJ_MEMORY) && pDesc->pbMemData && pDesc->llMemLength > 0) {
			U32 n = 0;
			unsigned char* smf = blob_to_smf(pDesc->pbMemData, (int)pDesc->llMemLength, &n);
			if (smf) { seg->smf = smf; seg->smflen = n;
				MTRACE("[music] GetObject: %ld B blob -> %u B SMF\n", (long)pDesc->llMemLength, n); }
			else MTRACE("[music] GetObject: %ld B blob not XMI/SMF -> silent segment\n", (long)pDesc->llMemLength);
		} else {
			MTRACE("[music] GetObject: no memory blob -> silent segment\n");
		}
		*ppv = (void*)seg;
		return S_OK;      /* never fail: the game SayAndQuit()s on a failed GetObject */
	}
	STDMETHODIMP SetObject(LPDMUS_OBJECTDESC)                   { return S_OK; }
	STDMETHODIMP SetSearchDirectory(REFGUID, WCHAR*, BOOL)      { return S_OK; }
	STDMETHODIMP ScanDirectory(REFGUID, WCHAR*, WCHAR*)         { return S_OK; }
	STDMETHODIMP CacheObject(IDirectMusicObject*)               { return S_OK; }
	STDMETHODIMP ReleaseObject(IDirectMusicObject* pObj)        { if (pObj) pObj->Release(); return S_OK; }
	STDMETHODIMP ClearCache(REFGUID)                            { return S_OK; }
	STDMETHODIMP EnableCache(REFGUID, BOOL)                     { return S_OK; }
	STDMETHODIMP EnumObject(REFGUID, DWORD, LPDMUS_OBJECTDESC)  { return S_FALSE; }
};

struct BobPerformance : public IDirectMusicPerformance {
	long ref;
	BobSegment* playing;    /* borrowed (not owned) — the game keeps its own ref */
	BobPerformance() : ref(1), playing(0) {}

	STDMETHODIMP QueryInterface(REFIID, LPVOID FAR* ppv) { if (!ppv) return E_POINTER; *ppv = (void*)this; ref++; return S_OK; }
	STDMETHODIMP_(ULONG) AddRef(void)  { return (ULONG)++ref; }
	STDMETHODIMP_(ULONG) Release(void) { if (--ref <= 0) { delete this; return 0; } return (ULONG)ref; }

	STDMETHODIMP Init(IDirectMusic** ppDM, LPDIRECTSOUND, HWND) {
		if (!music_init()) return E_FAIL;      /* -> Music::Init returns false -> silent */
		if (ppDM) *ppDM = new BobDMusic();
		MTRACE("[music] performance Init OK\n");
		return S_OK;
	}
	STDMETHODIMP PlaySegment(IDirectMusicSegment* pSeg, DWORD, __int64, IDirectMusicSegmentState** pp) {
		if (pp) *pp = 0;
		BobSegment* s = (BobSegment*)pSeg;
		music_stop();
		playing = s;
		if (!g_ready || !s || !s->smf) { MTRACE("[music] PlaySegment: nothing to play\n"); return S_OK; }
		g_play = new_fluid_player(g_syn);
		if (!g_play) return S_OK;
		if (fluid_player_add_mem(g_play, s->smf, s->smflen) != FLUID_OK) {
			MTRACE("[music] fluid_player_add_mem failed\n");
			delete_fluid_player(g_play); g_play = 0; return S_OK;
		}
		fluid_player_play(g_play);
		MTRACE("[music] PlaySegment: playing %u B SMF\n", s->smflen);
		return S_OK;
	}
	STDMETHODIMP Stop(IDirectMusicSegment*, IDirectMusicSegmentState*, MUSIC_TIME, DWORD) {
		MTRACE("[music] Stop\n"); music_stop(); playing = 0; return S_OK;
	}
	STDMETHODIMP IsPlaying(IDirectMusicSegment*, IDirectMusicSegmentState*) {
		if (g_play && fluid_player_get_status(g_play) == FLUID_PLAYER_PLAYING) return S_OK;
		return S_FALSE;
	}
	STDMETHODIMP GetSegmentState(IDirectMusicSegmentState** pp, MUSIC_TIME) { if (pp) *pp = 0; return S_FALSE; }
	STDMETHODIMP SetPrepareTime(DWORD)      { return S_OK; }
	STDMETHODIMP GetPrepareTime(DWORD* p)   { if (p) *p = 0; return S_OK; }
	STDMETHODIMP SetBumperLength(DWORD)     { return S_OK; }
	STDMETHODIMP GetBumperLength(DWORD* p)  { if (p) *p = 0; return S_OK; }
	STDMETHODIMP SendPMsg(DMUS_PMSG*)       { return S_OK; }
	STDMETHODIMP MusicToReferenceTime(MUSIC_TIME, REFERENCE_TIME* p) { if (p) *p = 0; return S_OK; }
	STDMETHODIMP ReferenceToMusicTime(REFERENCE_TIME, MUSIC_TIME* p) { if (p) *p = 0; return S_OK; }
	STDMETHODIMP GetTime(REFERENCE_TIME* a, MUSIC_TIME* b) { if (a) *a = 0; if (b) *b = 0; return S_OK; }
	STDMETHODIMP AllocPMsg(ULONG cb, DMUS_PMSG** pp) { if (!pp) return E_POINTER; *pp = (DMUS_PMSG*)calloc(1, cb); return *pp ? S_OK : E_OUTOFMEMORY; }
	STDMETHODIMP FreePMsg(DMUS_PMSG* p)     { free(p); return S_OK; }
	STDMETHODIMP GetGraph(IDirectMusicGraph** pp) { if (pp) *pp = 0; return E_NOTIMPL; }
	STDMETHODIMP SetGraph(IDirectMusicGraph*)     { return S_OK; }
	STDMETHODIMP SetNotificationHandle(HANDLE, REFERENCE_TIME) { return S_OK; }
	STDMETHODIMP GetNotificationPMsg(DMUS_NOTIFICATION_PMSG** pp) { if (pp) *pp = 0; return S_FALSE; }
	STDMETHODIMP AddNotificationType(REFGUID)     { return S_OK; }
	STDMETHODIMP RemoveNotificationType(REFGUID)  { return S_OK; }
	STDMETHODIMP AddPort(IDirectMusicPort*)       { return S_OK; }
	STDMETHODIMP RemovePort(IDirectMusicPort*)    { return S_OK; }
	STDMETHODIMP AssignPChannelBlock(DWORD, IDirectMusicPort*, DWORD) { return S_OK; }
	STDMETHODIMP AssignPChannel(DWORD, IDirectMusicPort*, DWORD, DWORD) { return S_OK; }
	STDMETHODIMP PChannelInfo(DWORD, IDirectMusicPort** pp, DWORD* g, DWORD* m) { if (pp) *pp = 0; if (g) *g = 0; if (m) *m = 0; return S_OK; }
	STDMETHODIMP DownloadInstrument(IDirectMusicInstrument*, DWORD, IDirectMusicDownloadedInstrument** pp,
	                                DMUS_NOTERANGE*, DWORD, IDirectMusicPort** ppp, DWORD*, DWORD*) {
		if (pp) *pp = 0; if (ppp) *ppp = 0; return S_OK;
	}
	STDMETHODIMP Invalidate(MUSIC_TIME, DWORD)    { return S_OK; }
	STDMETHODIMP GetParam(REFGUID, DWORD, DWORD, MUSIC_TIME, MUSIC_TIME*, void*) { return S_OK; }
	STDMETHODIMP SetParam(REFGUID, DWORD, DWORD, MUSIC_TIME, void*)              { return S_OK; }
	STDMETHODIMP GetGlobalParam(REFGUID, void*, DWORD) { return S_OK; }
	STDMETHODIMP SetGlobalParam(REFGUID, void* pParam, DWORD dwSize) {
		/* The game's only global param is GUID_PerfMasterVolume (MUSIC.CPP:SetVolume),
		   a LONG in hundredths of a dB (DigitalDriver::GetDecibels). The stub GUIDs are
		   all-zero so we can't key off rguidType — a 4-byte param IS the volume. */
		if (pParam && dwSize == sizeof(long) && g_ready) {
			long cb = *(long*)pParam;                 /* attenuation, <= 0 */
			if (cb > 0) cb = 0;
			if (cb < -6000) cb = -6000;               /* -60 dB ~ silence */
			double gain = g_gain * pow(10.0, cb / 2000.0);
			fluid_synth_set_gain(g_syn, (float)gain);
			MTRACE("[music] master volume %ld (0.01dB) -> gain %.3f\n", cb, gain);
		}
		return S_OK;
	}
	STDMETHODIMP GetLatencyTime(REFERENCE_TIME* p) { if (p) *p = 0; return S_OK; }
	STDMETHODIMP GetQueueTime(REFERENCE_TIME* p)   { if (p) *p = 0; return S_OK; }
	STDMETHODIMP AdjustTime(REFERENCE_TIME)        { return S_OK; }
	STDMETHODIMP CloseDown(void) { MTRACE("[music] CloseDown\n"); music_stop(); return S_OK; }
	STDMETHODIMP GetResolvedTime(REFERENCE_TIME, REFERENCE_TIME* p, DWORD) { if (p) *p = 0; return S_OK; }
	STDMETHODIMP MIDIToMusic(BYTE, DMUS_CHORD_KEY*, BYTE, BYTE, WORD* p) { if (p) *p = 0; return S_OK; }
	STDMETHODIMP MusicToMIDI(WORD, DMUS_CHORD_KEY*, BYTE, BYTE, BYTE* p) { if (p) *p = 0; return S_OK; }
	STDMETHODIMP TimeToRhythm(MUSIC_TIME, DMUS_TIMESIGNATURE*, WORD*, BYTE*, BYTE*, short*) { return S_OK; }
	STDMETHODIMP RhythmToTime(WORD, BYTE, BYTE, short, DMUS_TIMESIGNATURE*, MUSIC_TIME* p) { if (p) *p = 0; return S_OK; }
};

/* ---- the CoCreateInstance hook (called from compat/objbase.h) ---------------- */
extern "C" HRESULT bob_com_create_instance(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	(void)riid;
	if (!ppv) return E_NOINTERFACE;
	MTRACE("[music] CoCreateInstance clsid=%08x-%04x-%04x\n",
		(unsigned)rclsid.Data1, (unsigned)rclsid.Data2, (unsigned)rclsid.Data3);
	if (memcmp(&rclsid, &CLSID_DirectMusicPerformance, sizeof(GUID)) == 0) {
		if (!music_init()) { MTRACE("[music] no synth -> DirectMusic unavailable\n"); return E_NOINTERFACE; }
		*ppv = (void*)(IDirectMusicPerformance*)new BobPerformance();
		return S_OK;
	}
	if (memcmp(&rclsid, &CLSID_DirectMusicLoader, sizeof(GUID)) == 0) {
		if (!g_ready) return E_NOINTERFACE;
		*ppv = (void*)(IDirectMusicLoader*)new BobLoader();
		return S_OK;
	}
	/* R6.1: CLSID_DirectPlay -- the single entry point multiplayer was missing. The engine's own
	   DPlay::CreateDPlayInterface() asks for it here and, getting E_NOINTERFACE, reports
	   "not connected". Served by SRC/compat/bob_dplay.cpp. Value-compared rather than including
	   DPLAY.H (which drags the whole DirectX header in for one GUID):
	     CLSID_DirectPlay = d1eb6d20-8923-11d0-9d97-00a0c90a43cb   (DPLAY.H:46)
	   BOB_NO_DPLAY=1 restores the old E_NOINTERFACE, which is the negative control for the R6.1
	   gate: with it set the lobby must still degrade gracefully, exactly as before. */
	{
		static const GUID kDPlay = { 0xd1eb6d20, 0x8923, 0x11d0,
		                             { 0x9d,0x97,0x00,0xa0,0xc9,0x0a,0x43,0xcb } };
		if (memcmp(&rclsid, &kDPlay, sizeof(GUID)) == 0) {
			if (getenv("BOB_NO_DPLAY")) return E_NOINTERFACE;
			extern HRESULT bob_dplay_create(void** ppv);
			return bob_dplay_create((void**)ppv);
		}
	}
	return E_NOINTERFACE;
}

/* ---- BOB_MUSIC_SELFTEST=<file.xmi|file.mid> --------------------------------
   Drives the *same* COM chain MUSIC.CPP drives (CoCreateInstance -> Init -> ports ->
   loader GetObject(memory) -> PlaySegment -> IsPlaying), with a file read from disk
   standing in for the numbered-file blob LoadTune() would supply. Diagnostic only,
   default-off; called from bob_main.cpp. */
extern "C" void bob_music_selftest(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f) { fprintf(stderr, "[music] selftest: cannot open %s\n", path); return; }
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	unsigned char* buf = (unsigned char*)malloc((size_t)(n > 0 ? n : 1));
	if (!buf || n <= 0 || fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return; }
	fclose(f);

	IDirectMusicPerformance* perf = 0;
	if (bob_com_create_instance(CLSID_DirectMusicPerformance, CLSID_DirectMusicPerformance, (LPVOID*)&perf) != S_OK) {
		fprintf(stderr, "[music] selftest: no DirectMusic performance (no synth/soundfont/driver)\n");
		free(buf); return;
	}
	IDirectMusic* dm = 0;
	if (perf->Init(&dm, 0, 0) != S_OK || !dm) { fprintf(stderr, "[music] selftest: Init failed\n"); free(buf); return; }

	IDirectMusicLoader* loader = 0;
	bob_com_create_instance(CLSID_DirectMusicLoader, CLSID_DirectMusicLoader, (LPVOID*)&loader);
	if (!loader) { fprintf(stderr, "[music] selftest: no loader\n"); free(buf); return; }

	GUID def; dm->GetDefaultPort(&def);
	DMUS_PORTPARAMS pp; memset(&pp, 0, sizeof(pp)); pp.dwSize = sizeof(pp);
	IDirectMusicPort* port = 0;
	dm->CreatePort(def, &pp, &port, 0);
	if (port) { port->Activate(TRUE); perf->AddPort(port); perf->AssignPChannelBlock(0, port, 1); }

	DMUS_OBJECTDESC od; memset(&od, 0, sizeof(od));
	od.dwSize = sizeof(od);
	od.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_MEMORY;
	od.pbMemData = (LPBYTE)buf;
	od.llMemLength = n;
	IDirectMusicSegment* seg = 0;
	loader->GetObject(&od, CLSID_DirectMusicSegment, (LPVOID*)&seg);
	if (!seg) { fprintf(stderr, "[music] selftest: no segment\n"); free(buf); return; }

	long vol = 0;                                  /* 0 = full (0.00 dB attenuation) */
	perf->SetGlobalParam(CLSID_DirectMusicPerformance, &vol, sizeof(vol));
	perf->PlaySegment(seg, 0, 0, 0);
	fprintf(stderr, "[music] selftest: %s (%ld B) -> SMF %u B, playing\n",
		path, n, ((BobSegment*)seg)->smflen);

	int secs = getenv("BOB_MUSIC_SELFTEST_SECS") ? atoi(getenv("BOB_MUSIC_SELFTEST_SECS")) : 3;
	for (int i = 0; i < secs * 2; i++) {
		HRESULT st = perf->IsPlaying(seg, 0);
		fprintf(stderr, "[music] selftest t=%.1fs IsPlaying=%s\n", i * 0.5, st == S_OK ? "S_OK" : "S_FALSE");
		if (st != S_OK && i > 1) break;
		usleep(500 * 1000);
	}
	perf->Stop(0, 0, 0, 0);
	seg->Release(); loader->Release(); if (port) port->Release(); dm->Release(); perf->Release();
	free(buf);
	fprintf(stderr, "[music] selftest: done\n");
}

#endif /* FF_LINUX */
