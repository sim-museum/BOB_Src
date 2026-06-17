/* ==========================================================================
 *  openal_dsound.cpp — DirectSound -> OpenAL backend.
 *
 *  The game's DigitalDriver (SRC/HARDWARE/DIGDRVR.CPP) creates a DirectSound
 *  object, a primary buffer + a 3D listener, then a pool of Sample objects;
 *  each Sample CreateSoundBuffer()s a static secondary buffer, Lock()s the
 *  whole thing, memcpy's the WAV PCM in, Unlock()s, then Play()s it (looping
 *  for engines, one-shot for effects), modulating SetFrequency() for RPM and
 *  SetVolume()/SetPan() or the 3D SetPosition()/SetVelocity() for spatialisation.
 *
 *  We back each secondary buffer with one AL buffer + one AL source. The
 *  primary buffer + 3D listener map to the single global AL listener.
 *
 *  COM C-vtbl pattern (mirrors bob_video.cpp): each concrete struct starts with
 *  a `*Vtbl* lpVtbl` and points it at a static vtable of stdcall-less thunks.
 *  No STL (this TU is built with -fpack-struct=1; keep libstdc++ types out).
 * ======================================================================== */
#include "stdafx.h"
#include "dsound.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static int al_trace() { static int t=-1; if(t<0) t=getenv("BOB_TRACE_SND")?1:0; return t; }
#define SLOG(...) do{ if(al_trace()) fprintf(stderr,"[snd] " __VA_ARGS__); }while(0)

/* ----- one-time OpenAL device/context ------------------------------------ */
static ALCdevice*  g_alDev = 0;
static ALCcontext* g_alCtx = 0;
static int al_init() {
	if (g_alCtx) return 1;
	if (getenv("BOB_NOSOUND")) return 0;
	g_alDev = alcOpenDevice(NULL);
	if (!g_alDev) { SLOG("alcOpenDevice failed -> silent\n"); return 0; }
	g_alCtx = alcCreateContext(g_alDev, NULL);
	if (!g_alCtx) { alcCloseDevice(g_alDev); g_alDev=0; SLOG("alcCreateContext failed\n"); return 0; }
	alcMakeContextCurrent(g_alCtx);
	/* listener at origin, facing -Z, up +Y (DirectSound default) */
	ALfloat ori[6] = {0,0,-1, 0,1,0};
	alListener3f(AL_POSITION,0,0,0); alListenerfv(AL_ORIENTATION,ori);
	SLOG("OpenAL up: device=%s\n", alcGetString(g_alDev, ALC_DEVICE_SPECIFIER));
	return 1;
}

static ALenum al_format(int ch, int bits) {
	if (ch >= 2) return bits==8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
	return bits==8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
}

/* ----- concrete objects --------------------------------------------------- */
struct OpenAL3DBuffer;
struct OpenALSoundBuffer {
	IDirectSoundBufferVtbl* lpVtbl;
	int      isPrimary;
	ALuint   buf, src;          /* secondary: an AL buffer + source */
	void*    pcm;               /* Lock staging (the whole buffer) */
	int      bytes;             /* dwBufferBytes */
	int      channels, bits, freq, baseFreq;
	int      relative;          /* 2D (SetPan) source: relative + no rolloff */
	int      ref;
	OpenAL3DBuffer* d3d;        /* lazily-created 3D wrapper */
};
struct OpenAL3DBuffer {
	IDirectSound3DBufferVtbl* lpVtbl;
	OpenALSoundBuffer* owner;
	int ref;
};
struct OpenAL3DListener {
	IDirectSound3DListenerVtbl* lpVtbl;
	int ref;
};
struct OpenALDirectSound {
	IDirectSoundVtbl* lpVtbl;
	int ref;
};

static IDirectSoundVtbl          g_dsVtbl;
static IDirectSoundBufferVtbl    g_dsbVtbl;
static IDirectSound3DBufferVtbl  g_ds3dbVtbl;
static IDirectSound3DListenerVtbl g_ds3dlVtbl;
static void build_vtbls();

/* ============================ IDirectSoundBuffer ========================== */
static ULONG STDMETHODCALLTYPE DSB_AddRef(IDirectSoundBuffer* This){ return ++((OpenALSoundBuffer*)This)->ref; }
static ULONG STDMETHODCALLTYPE DSB_Release(IDirectSoundBuffer* This){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(--b->ref>0) return b->ref;
	if(!b->isPrimary){ if(b->src){alSourceStop(b->src);alDeleteSources(1,&b->src);} if(b->buf)alDeleteBuffers(1,&b->buf); free(b->pcm); }
	if(b->d3d) free(b->d3d); free(b); return 0;
}
static HRESULT STDMETHODCALLTYPE DSB_QueryInterface(IDirectSoundBuffer* This, REFIID, void** ppv){
	/* the game QIs a secondary buffer for IID_IDirectSound3DBuffer (only when 3D),
	   and the primary buffer for IID_IDirectSound3DListener. Branch on type. */
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(!ppv) return E_FAIL;
	if (b->isPrimary) {
		OpenAL3DListener* l=(OpenAL3DListener*)calloc(1,sizeof(OpenAL3DListener));
		l->lpVtbl=&g_ds3dlVtbl; l->ref=1; *ppv=l; SLOG("QI primary -> 3DListener\n"); return DS_OK;
	}
	if (!b->d3d){ b->d3d=(OpenAL3DBuffer*)calloc(1,sizeof(OpenAL3DBuffer)); b->d3d->lpVtbl=&g_ds3dbVtbl; b->d3d->owner=b; }
	b->d3d->ref++; *ppv=b->d3d; SLOG("QI secondary -> 3DBuffer\n"); return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_Lock(IDirectSoundBuffer* This, DWORD off, DWORD bytes,
		LPVOID* p1, LPDWORD b1, LPVOID* p2, LPDWORD b2, DWORD){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This;
	if (!b->pcm && b->bytes>0) b->pcm=calloc(1,b->bytes);
	DWORD n = bytes ? bytes : (DWORD)b->bytes;
	if ((int)(off+n) > b->bytes) n = b->bytes>(int)off ? b->bytes-off : 0;
	if (p1) *p1 = (char*)b->pcm + off; if (b1) *b1 = n;
	if (p2) *p2 = NULL;               if (b2) *b2 = 0;   /* game writes the whole buffer contiguously */
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_Unlock(IDirectSoundBuffer* This, LPVOID, DWORD, LPVOID, DWORD){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This;
	if (b->isPrimary || !b->pcm || b->bytes<=0) return DS_OK;
	alBufferData(b->buf, al_format(b->channels,b->bits), b->pcm, b->bytes, b->freq);
	alSourcei(b->src, AL_BUFFER, b->buf);
	SLOG("Unlock -> buffered %d bytes %dch/%dbit @%dHz\n", b->bytes,b->channels,b->bits,b->freq);
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_Play(IDirectSoundBuffer* This, DWORD, DWORD, DWORD flags){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(b->isPrimary||!b->src) return DS_OK;
	alSourcei(b->src, AL_LOOPING, (flags & DSBPLAY_LOOPING) ? AL_TRUE : AL_FALSE);
	alSourcePlay(b->src);
	SLOG("Play src=%u loop=%d\n", b->src, (flags&DSBPLAY_LOOPING)?1:0);
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_Stop(IDirectSoundBuffer* This){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(!b->isPrimary&&b->src) alSourceStop(b->src); return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_SetVolume(IDirectSoundBuffer* This, LONG dB){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(b->isPrimary||!b->src) return DS_OK;
	float gain = dB<=DSBVOLUME_MIN ? 0.f : powf(10.f, dB/2000.f);   /* hundredths-dB -> linear */
	alSourcef(b->src, AL_GAIN, gain); return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_SetPan(IDirectSoundBuffer* This, LONG pan){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(b->isPrimary||!b->src) return DS_OK;
	if (!b->relative){ alSourcei(b->src,AL_SOURCE_RELATIVE,AL_TRUE); alSourcef(b->src,AL_ROLLOFF_FACTOR,0.f); b->relative=1; }
	alSource3f(b->src, AL_POSITION, pan/10000.f, 0.f, -0.25f);   /* x left/right, slightly in front */
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_SetFrequency(IDirectSoundBuffer* This, DWORD freq){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(b->isPrimary||!b->src||b->baseFreq<=0) return DS_OK;
	alSourcef(b->src, AL_PITCH, (float)freq/(float)b->baseFreq); return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_GetStatus(IDirectSoundBuffer* This, LPDWORD st){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(!st) return DS_OK; *st=0;
	if(!b->isPrimary&&b->src){ ALint s=0,l=0; alGetSourcei(b->src,AL_SOURCE_STATE,&s); alGetSourcei(b->src,AL_LOOPING,&l);
		if(s==AL_PLAYING){ *st|=DSBSTATUS_PLAYING; if(l) *st|=DSBSTATUS_LOOPING; } }
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_GetCurrentPosition(IDirectSoundBuffer* This, LPDWORD play, LPDWORD write){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; ALint off=0;
	if(!b->isPrimary&&b->src) alGetSourcei(b->src,AL_BYTE_OFFSET,&off);
	if(play) *play=(DWORD)off; if(write) *write=(DWORD)off; return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DSB_SetCurrentPosition(IDirectSoundBuffer* This, DWORD pos){
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)This; if(!b->isPrimary&&b->src) alSourcei(b->src,AL_BYTE_OFFSET,(ALint)pos); return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_SetFormat(IDirectSoundBuffer*, LPCWAVEFORMATEX){ return DS_OK; }   /* primary: no-op */
static HRESULT STDMETHODCALLTYPE DSB_Restore(IDirectSoundBuffer*){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_GetCaps(IDirectSoundBuffer*, LPDSBCAPS){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_GetFormat(IDirectSoundBuffer*, LPWAVEFORMATEX, DWORD, LPDWORD){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_GetVolume(IDirectSoundBuffer*, LPLONG v){ if(v)*v=0; return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_GetPan(IDirectSoundBuffer*, LPLONG v){ if(v)*v=0; return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_GetFrequency(IDirectSoundBuffer* This, LPDWORD v){ if(v)*v=((OpenALSoundBuffer*)This)->baseFreq; return DS_OK; }
static HRESULT STDMETHODCALLTYPE DSB_Initialize(IDirectSoundBuffer*, LPDIRECTSOUND, LPCDSBUFFERDESC){ return DS_OK; }

/* ============================ IDirectSound3DBuffer ======================== */
static ULONG STDMETHODCALLTYPE D3DB_AddRef(IDirectSound3DBuffer* This){ return ++((OpenAL3DBuffer*)This)->ref; }
static ULONG STDMETHODCALLTYPE D3DB_Release(IDirectSound3DBuffer* This){ OpenAL3DBuffer* d=(OpenAL3DBuffer*)This; return --d->ref>0?d->ref:0; }
static HRESULT STDMETHODCALLTYPE D3DB_QI(IDirectSound3DBuffer*, REFIID, void** p){ if(p)*p=0; return E_FAIL; }
static ALuint d3db_src(IDirectSound3DBuffer* This){ OpenAL3DBuffer* d=(OpenAL3DBuffer*)This; return d->owner?d->owner->src:0; }
static HRESULT STDMETHODCALLTYPE D3DB_SetPosition(IDirectSound3DBuffer* This, D3DVALUE x,D3DVALUE y,D3DVALUE z,DWORD){
	ALuint s=d3db_src(This); if(s){ OpenAL3DBuffer* d=(OpenAL3DBuffer*)This; if(d->owner) d->owner->relative=0;
		alSourcei(s,AL_SOURCE_RELATIVE,AL_FALSE); alSource3f(s,AL_POSITION,x,y,z); } return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_SetVelocity(IDirectSound3DBuffer* This, D3DVALUE x,D3DVALUE y,D3DVALUE z,DWORD){
	ALuint s=d3db_src(This); if(s) alSource3f(s,AL_VELOCITY,x,y,z); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_SetMinDistance(IDirectSound3DBuffer* This, D3DVALUE d,DWORD){
	ALuint s=d3db_src(This); if(s) alSourcef(s,AL_REFERENCE_DISTANCE,d); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_SetMaxDistance(IDirectSound3DBuffer* This, D3DVALUE d,DWORD){
	ALuint s=d3db_src(This); if(s) alSourcef(s,AL_MAX_DISTANCE,d); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_SetMode(IDirectSound3DBuffer* This, DWORD mode,DWORD){
	ALuint s=d3db_src(This); if(s) alSourcei(s,AL_SOURCE_RELATIVE, mode==1/*HEADRELATIVE*/?AL_TRUE:AL_FALSE); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_ok(IDirectSound3DBuffer*){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_okv(IDirectSound3DBuffer*, D3DVECTOR* v){ if(v){v->x=v->y=v->z=0;} return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_okd(IDirectSound3DBuffer*, LPDWORD v){ if(v)*v=0; return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_okf(IDirectSound3DBuffer*, D3DVALUE* v){ if(v)*v=0; return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_okl(IDirectSound3DBuffer*, LPLONG v){ if(v)*v=0; return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_okall(IDirectSound3DBuffer*, LPDS3DBUFFER){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DB_okca(IDirectSound3DBuffer*, LPDWORD a, LPDWORD b){ if(a)*a=0; if(b)*b=0; return DS_OK; }

/* ============================ IDirectSound3DListener ====================== */
static ULONG STDMETHODCALLTYPE D3DL_AddRef(IDirectSound3DListener* This){ return ++((OpenAL3DListener*)This)->ref; }
static ULONG STDMETHODCALLTYPE D3DL_Release(IDirectSound3DListener* This){ OpenAL3DListener* l=(OpenAL3DListener*)This; int r=--l->ref; if(r<=0){free(l);return 0;} return r; }
static HRESULT STDMETHODCALLTYPE D3DL_QI(IDirectSound3DListener*, REFIID, void** p){ if(p)*p=0; return E_FAIL; }
static HRESULT STDMETHODCALLTYPE D3DL_SetPosition(IDirectSound3DListener*, D3DVALUE x,D3DVALUE y,D3DVALUE z,DWORD){ alListener3f(AL_POSITION,x,y,z); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_SetVelocity(IDirectSound3DListener*, D3DVALUE x,D3DVALUE y,D3DVALUE z,DWORD){ alListener3f(AL_VELOCITY,x,y,z); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_SetOrientation(IDirectSound3DListener*, D3DVALUE fx,D3DVALUE fy,D3DVALUE fz,D3DVALUE tx,D3DVALUE ty,D3DVALUE tz,DWORD){
	ALfloat o[6]={fx,fy,fz,tx,ty,tz}; alListenerfv(AL_ORIENTATION,o); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_SetDistanceFactor(IDirectSound3DListener*, D3DVALUE,DWORD){ return DS_OK; }   /* unit scale: leave AL default */
static HRESULT STDMETHODCALLTYPE D3DL_SetRolloffFactor(IDirectSound3DListener*, D3DVALUE, DWORD){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_SetDopplerFactor(IDirectSound3DListener*, D3DVALUE f,DWORD){ alDopplerFactor(f); return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_Commit(IDirectSound3DListener*){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_okf(IDirectSound3DListener*, D3DVALUE* v){ if(v)*v=0; return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_okv(IDirectSound3DListener*, D3DVECTOR* v){ if(v){v->x=v->y=v->z=0;} return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_okvv(IDirectSound3DListener*, D3DVECTOR* a, D3DVECTOR* b){ if(a){a->x=a->y=a->z=0;} if(b){b->x=b->y=b->z=0;} return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_okall(IDirectSound3DListener*, LPDS3DLISTENER){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE D3DL_okallc(IDirectSound3DListener*, LPCDS3DLISTENER, DWORD){ return DS_OK; }

/* ============================ IDirectSound ================================ */
static ULONG STDMETHODCALLTYPE DS_AddRef(IDirectSound* This){ return ++((OpenALDirectSound*)This)->ref; }
static ULONG STDMETHODCALLTYPE DS_Release(IDirectSound* This){ OpenALDirectSound* d=(OpenALDirectSound*)This; int r=--d->ref; if(r<=0){free(d);return 0;} return r; }
static HRESULT STDMETHODCALLTYPE DS_QueryInterface(IDirectSound* This, REFIID, void** p){ if(p)*p=This; return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_CreateSoundBuffer(IDirectSound*, LPCDSBUFFERDESC d, LPDIRECTSOUNDBUFFER* out, IUnknown*){
	if(!out||!d) return E_FAIL;
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)calloc(1,sizeof(OpenALSoundBuffer));
	b->lpVtbl=&g_dsbVtbl; b->ref=1;
	if (d->dwFlags & DSBCAPS_PRIMARYBUFFER) { b->isPrimary=1; *out=(IDirectSoundBuffer*)b; SLOG("CreateSoundBuffer PRIMARY\n"); return DS_OK; }
	b->bytes=(int)d->dwBufferBytes;
	WAVEFORMATEX* w=d->lpwfxFormat;
	b->channels = w?w->nChannels:1; b->bits = w?w->wBitsPerSample:16;
	b->freq = w?(int)w->nSamplesPerSec:22050; b->baseFreq=b->freq;
	if (al_init()){ alGenBuffers(1,&b->buf); alGenSources(1,&b->src); alSourcef(b->src,AL_PITCH,1.f); alSourcef(b->src,AL_GAIN,1.f); }
	*out=(IDirectSoundBuffer*)b;
	SLOG("CreateSoundBuffer %d bytes %dch/%dbit @%dHz flags=0x%lx\n", b->bytes,b->channels,b->bits,b->freq,(unsigned long)d->dwFlags);
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DS_GetCaps(IDirectSound*, LPDSCAPS c){
	if(c){ DWORD sz=c->dwSize; memset(c,0,sz); c->dwSize=sz;
		c->dwFlags=DSCAPS_PRIMARYSTEREO|DSCAPS_PRIMARY16BIT|DSCAPS_SECONDARYSTEREO|DSCAPS_SECONDARY16BIT|DSCAPS_CONTINUOUSRATE; }
	return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DS_DuplicateSoundBuffer(IDirectSound*, LPDIRECTSOUNDBUFFER orig, LPDIRECTSOUNDBUFFER* dup){
	/* share the AL buffer, new source */
	OpenALSoundBuffer* s=(OpenALSoundBuffer*)orig; if(!s||!dup) return E_FAIL;
	OpenALSoundBuffer* b=(OpenALSoundBuffer*)calloc(1,sizeof(OpenALSoundBuffer));
	*b=*s; b->ref=1; b->d3d=0; b->pcm=0;
	if(al_init()){ alGenSources(1,&b->src); alSourcei(b->src,AL_BUFFER,s->buf); } b->buf=s->buf;
	*dup=(IDirectSoundBuffer*)b; return DS_OK;
}
static HRESULT STDMETHODCALLTYPE DS_SetCooperativeLevel(IDirectSound*, HWND, DWORD){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_Compact(IDirectSound*){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_GetSpeakerConfig(IDirectSound*, LPDWORD c){ if(c)*c=0; return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_SetSpeakerConfig(IDirectSound*, DWORD){ return DS_OK; }
static HRESULT STDMETHODCALLTYPE DS_Initialize(IDirectSound*, const GUID*){ return DS_OK; }

static void build_vtbls(){
	static int done=0; if(done) return; done=1;
	g_dsVtbl.QueryInterface=DS_QueryInterface; g_dsVtbl.AddRef=DS_AddRef; g_dsVtbl.Release=DS_Release;
	g_dsVtbl.CreateSoundBuffer=DS_CreateSoundBuffer; g_dsVtbl.GetCaps=DS_GetCaps;
	g_dsVtbl.DuplicateSoundBuffer=DS_DuplicateSoundBuffer; g_dsVtbl.SetCooperativeLevel=DS_SetCooperativeLevel;
	g_dsVtbl.Compact=DS_Compact; g_dsVtbl.GetSpeakerConfig=DS_GetSpeakerConfig; g_dsVtbl.SetSpeakerConfig=DS_SetSpeakerConfig;
	g_dsVtbl.Initialize=DS_Initialize;

	g_dsbVtbl.QueryInterface=DSB_QueryInterface; g_dsbVtbl.AddRef=DSB_AddRef; g_dsbVtbl.Release=DSB_Release;
	g_dsbVtbl.GetCaps=DSB_GetCaps; g_dsbVtbl.GetCurrentPosition=DSB_GetCurrentPosition; g_dsbVtbl.GetFormat=DSB_GetFormat;
	g_dsbVtbl.GetVolume=DSB_GetVolume; g_dsbVtbl.GetPan=DSB_GetPan; g_dsbVtbl.GetFrequency=DSB_GetFrequency;
	g_dsbVtbl.GetStatus=DSB_GetStatus; g_dsbVtbl.Initialize=DSB_Initialize; g_dsbVtbl.Lock=DSB_Lock;
	g_dsbVtbl.Play=DSB_Play; g_dsbVtbl.SetCurrentPosition=DSB_SetCurrentPosition; g_dsbVtbl.SetFormat=DSB_SetFormat;
	g_dsbVtbl.SetVolume=DSB_SetVolume; g_dsbVtbl.SetPan=DSB_SetPan; g_dsbVtbl.SetFrequency=DSB_SetFrequency;
	g_dsbVtbl.Stop=DSB_Stop; g_dsbVtbl.Unlock=DSB_Unlock; g_dsbVtbl.Restore=DSB_Restore;

	g_ds3dbVtbl.QueryInterface=D3DB_QI; g_ds3dbVtbl.AddRef=D3DB_AddRef; g_ds3dbVtbl.Release=D3DB_Release;
	g_ds3dbVtbl.GetAllParameters=D3DB_okall; g_ds3dbVtbl.GetConeAngles=D3DB_okca; g_ds3dbVtbl.GetConeOrientation=D3DB_okv;
	g_ds3dbVtbl.GetConeOutsideVolume=D3DB_okl; g_ds3dbVtbl.GetMaxDistance=D3DB_okf; g_ds3dbVtbl.GetMinDistance=D3DB_okf;
	g_ds3dbVtbl.GetMode=D3DB_okd; g_ds3dbVtbl.GetPosition=D3DB_okv; g_ds3dbVtbl.GetVelocity=D3DB_okv;
	g_ds3dbVtbl.SetAllParameters=(HRESULT(STDMETHODCALLTYPE*)(IDirectSound3DBuffer*,LPCDS3DBUFFER,DWORD))D3DB_ok;
	g_ds3dbVtbl.SetConeAngles=(HRESULT(STDMETHODCALLTYPE*)(IDirectSound3DBuffer*,DWORD,DWORD,DWORD))D3DB_ok;
	g_ds3dbVtbl.SetConeOrientation=(HRESULT(STDMETHODCALLTYPE*)(IDirectSound3DBuffer*,D3DVALUE,D3DVALUE,D3DVALUE,DWORD))D3DB_ok;
	g_ds3dbVtbl.SetConeOutsideVolume=(HRESULT(STDMETHODCALLTYPE*)(IDirectSound3DBuffer*,LONG,DWORD))D3DB_ok;
	g_ds3dbVtbl.SetMaxDistance=D3DB_SetMaxDistance; g_ds3dbVtbl.SetMinDistance=D3DB_SetMinDistance;
	g_ds3dbVtbl.SetMode=D3DB_SetMode; g_ds3dbVtbl.SetPosition=D3DB_SetPosition; g_ds3dbVtbl.SetVelocity=D3DB_SetVelocity;

	g_ds3dlVtbl.QueryInterface=D3DL_QI; g_ds3dlVtbl.AddRef=D3DL_AddRef; g_ds3dlVtbl.Release=D3DL_Release;
	g_ds3dlVtbl.GetAllParameters=D3DL_okall; g_ds3dlVtbl.GetDistanceFactor=D3DL_okf; g_ds3dlVtbl.GetDopplerFactor=D3DL_okf;
	g_ds3dlVtbl.GetOrientation=D3DL_okvv; g_ds3dlVtbl.GetPosition=D3DL_okv; g_ds3dlVtbl.GetRolloffFactor=D3DL_okf;
	g_ds3dlVtbl.GetVelocity=D3DL_okv; g_ds3dlVtbl.SetAllParameters=D3DL_okallc; g_ds3dlVtbl.SetDistanceFactor=D3DL_SetDistanceFactor;
	g_ds3dlVtbl.SetDopplerFactor=D3DL_SetDopplerFactor; g_ds3dlVtbl.SetOrientation=D3DL_SetOrientation;
	g_ds3dlVtbl.SetPosition=D3DL_SetPosition; g_ds3dlVtbl.SetRolloffFactor=D3DL_SetRolloffFactor;
	g_ds3dlVtbl.SetVelocity=D3DL_SetVelocity; g_ds3dlVtbl.CommitDeferredSettings=D3DL_Commit;
}

/* ----- entry point: replaces the E_FAIL stub in bob_stubs.cpp ------------- */
extern "C" HRESULT DirectSoundCreate(GUID* /*guid*/, LPDIRECTSOUND* ppDS, IUnknown*) {
	if (!ppDS) return E_FAIL;
	if (!al_init()) { *ppDS=NULL; return E_FAIL; }   /* BOB_NOSOUND / no device -> game runs silent */
	build_vtbls();
	OpenALDirectSound* d=(OpenALDirectSound*)calloc(1,sizeof(OpenALDirectSound));
	d->lpVtbl=&g_dsVtbl; d->ref=1; *ppDS=(IDirectSound*)d;
	SLOG("DirectSoundCreate -> OpenAL\n");
	return DS_OK;
}
