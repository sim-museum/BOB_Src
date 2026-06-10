/* BoB Linux port - DirectDraw7 / Direct3D7 -> SDL2 + OpenGL backend.
 *
 * PHASE 1: window + GL context + the device/surface enumeration and creation
 * skeleton, so Lib3D::Initialise and Lib3D::SetDriverAndMode complete and a
 * real window appears. The COM interfaces declared in compat/ddraw.h +
 * compat/d3d.h are C-style { lpVtbl, ...state... }; here we make concrete
 * GL-backed objects by pointing lpVtbl at our own function tables. The game
 * (SRC/LIB3D/LIB3D.CPP) calls them via p->Method() unchanged -- NO game edits.
 *
 * Rendering methods (Clear/DrawPrimitiveVB/SetRenderState/...) are safe no-ops
 * in this phase; Phase 2+ fills them with real GL. See PORT.md.
 */
#ifdef FF_LINUX

/* SDL/GL system headers use the native ABI -- keep -fpack-struct=1 away from
 * them (see the struct-stat hazard in bob_stubs.cpp). */
#pragma pack(push,8)
#include <SDL2/SDL.h>
#include <GL/gl.h>
#pragma pack(pop)

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "compat_types.h"
#include "ddraw.h"
#include "d3d.h"
#include "dinput.h"

extern const GUID IID_IDirect3D7;
extern const GUID IID_IDirect3DTnLHalDevice;
extern const GUID IID_IDirect3DHALDevice;

/* ============================ SDL window/context ========================== */
static SDL_Window*  g_win = NULL;
static SDL_GLContext g_ctx = NULL;
static int g_scrW = 1024, g_scrH = 768;     /* current display-mode size */
static int g_traceVid = 0;

#define VLOG(...) do{ if(g_traceVid) fprintf(stderr,"[vid] " __VA_ARGS__); }while(0)

static void ensure_window(int w, int h)
{
	if (w > 0 && h > 0) { g_scrW = w; g_scrH = h; }
	if (g_win) {
		SDL_SetWindowSize(g_win, g_scrW, g_scrH);
		return;
	}
	g_traceVid = getenv("BOB_TRACE_VID") ? 1 : 0;
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "[vid] SDL_Init failed: %s\n", SDL_GetError());
		return;
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	g_win = SDL_CreateWindow("Rowan's Battle of Britain (Linux native port)",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		g_scrW, g_scrH, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!g_win) { fprintf(stderr, "[vid] SDL_CreateWindow failed: %s\n", SDL_GetError()); return; }
	g_ctx = SDL_GL_CreateContext(g_win);
	if (!g_ctx) { fprintf(stderr, "[vid] SDL_GL_CreateContext failed: %s\n", SDL_GetError()); return; }
	SDL_GL_MakeCurrent(g_win, g_ctx);
	fprintf(stderr, "[vid] SDL2 window %dx%d + GL context: %s | %s\n",
		g_scrW, g_scrH, (const char*)glGetString(GL_RENDERER), (const char*)glGetString(GL_VERSION));
	/* clear once so the window isn't garbage while the rest of init runs */
	glViewport(0, 0, g_scrW, g_scrH);
	glClearColor(0.05f, 0.05f, 0.10f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(g_win);
}

/* Pump the SDL event queue so the window stays responsive / closeable. */
static void pump_events(void)
{
	if (!g_win) return;
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT ||
		    (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
			fprintf(stderr, "[vid] window closed -> exit\n");
			SDL_Quit();
			_exit(0);
		}
	}
}

/* ============================ object structs ============================== */
struct GLSurface7 {
	IDirectDrawSurface7Vtbl* lpVtbl;
	DDSURFACEDESC2 desc;            /* width/height/caps/pixelformat */
	int   w, h, bpp;
	void* bits;                     /* system-memory backing for Lock/Unlock */
	size_t bytes;
	GLSurface7* back;               /* attached back buffer (complex primary) */
	GLSurface7* zbuf;               /* attached z-buffer */
	int   isPrimary;
};
struct GLClipper  { IDirectDrawClipperVtbl* lpVtbl; HWND hwnd; };
struct GLPalette  { IDirectDrawPaletteVtbl* lpVtbl; PALETTEENTRY ent[256]; };
struct GLVB7      { IDirect3DVertexBuffer7Vtbl* lpVtbl; D3DVERTEXBUFFERDESC d; void* data; };
struct GLDevice7  { IDirect3DDevice7Vtbl* lpVtbl; };
struct GLD3D7     { IDirect3D7Vtbl* lpVtbl; };
struct GLDD7      { IDirectDraw7Vtbl* lpVtbl; HWND hwnd; DWORD coopFlags; };

/* one shared instance for the singleton sub-objects */
static IDirectDrawSurface7Vtbl  g_surfVtbl;
static IDirectDrawClipperVtbl   g_clipVtbl;
static IDirectDrawPaletteVtbl   g_palVtbl;
static IDirect3DVertexBuffer7Vtbl g_vbVtbl;
static IDirect3DDevice7Vtbl     g_devVtbl;
static IDirect3D7Vtbl           g_d3dVtbl;
static IDirectDraw7Vtbl         g_ddVtbl;
static GLDevice7 g_theDevice;
static GLD3D7    g_theD3D;

static ULONG generic_release(void*) { return 0; }   /* leak-on-release for skeleton objs */
static ULONG generic_addref(void*)  { return 1; }

/* ============================ surface methods ============================= */
static size_t surf_bytes(int w, int h, int bpp) { return (size_t)w * h * ((bpp+7)/8 ? (bpp+7)/8 : 4); }

static HRESULT SURF_GetSurfaceDesc(IDirectDrawSurface7* This, LPDDSURFACEDESC2 d) {
	GLSurface7* s = (GLSurface7*)This;
	if (d) { *d = s->desc; d->dwWidth = s->w; d->dwHeight = s->h; d->lPitch = s->w * ((s->bpp+7)/8); }
	return DD_OK;
}
static HRESULT SURF_Lock(IDirectDrawSurface7* This, LPRECT, LPDDSURFACEDESC2 d, DWORD, HANDLE) {
	GLSurface7* s = (GLSurface7*)This;
	if (!s->bits) { s->bytes = surf_bytes(s->w, s->h, s->bpp); s->bits = calloc(1, s->bytes ? s->bytes : 1); }
	if (d) { *d = s->desc; d->dwWidth=s->w; d->dwHeight=s->h; d->lPitch = s->w*((s->bpp+7)/8); d->lpSurface = s->bits; }
	return DD_OK;
}
static HRESULT SURF_Unlock(IDirectDrawSurface7*, LPRECT) { return DD_OK; }
static HRESULT SURF_GetAttachedSurface(IDirectDrawSurface7* This, LPDDSCAPS2 caps, IDirectDrawSurface7** out) {
	GLSurface7* s = (GLSurface7*)This;
	if (!out) return DDERR_INVALIDPARAMS;
	if (caps && (caps->dwCaps & DDSCAPS_ZBUFFER)) { *out = (IDirectDrawSurface7*)s->zbuf; return s->zbuf?DD_OK:DDERR_NOTFOUND; }
	*out = (IDirectDrawSurface7*)s->back;          /* default: back buffer */
	return s->back ? DD_OK : DDERR_NOTFOUND;
}
static HRESULT SURF_AddAttachedSurface(IDirectDrawSurface7* This, IDirectDrawSurface7* a) {
	GLSurface7* s = (GLSurface7*)This; GLSurface7* as = (GLSurface7*)a;
	if (as && (as->desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)) s->zbuf = as; else s->back = as;
	return DD_OK;
}
static HRESULT SURF_GetPixelFormat(IDirectDrawSurface7* This, LPDDPIXELFORMAT pf) {
	GLSurface7* s = (GLSurface7*)This; if (pf) *pf = s->desc.ddpfPixelFormat; return DD_OK;
}
static HRESULT SURF_Blt(IDirectDrawSurface7*, LPRECT, IDirectDrawSurface7*, LPRECT, DWORD, LPDDBLTFX) { pump_events(); return DD_OK; }
static HRESULT SURF_BltFast(IDirectDrawSurface7*, DWORD, DWORD, IDirectDrawSurface7*, LPRECT, DWORD) { return DD_OK; }
static HRESULT SURF_Flip(IDirectDrawSurface7*, IDirectDrawSurface7*, DWORD) { if (g_win) SDL_GL_SwapWindow(g_win); pump_events(); return DD_OK; }
static HRESULT SURF_SetPalette(IDirectDrawSurface7*, LPDIRECTDRAWPALETTE) { return DD_OK; }
static HRESULT SURF_SetClipper(IDirectDrawSurface7*, LPDIRECTDRAWCLIPPER) { return DD_OK; }
static HRESULT SURF_IsLost(IDirectDrawSurface7*) { return DD_OK; }
static HRESULT SURF_Restore(IDirectDrawSurface7*) { return DD_OK; }
static HRESULT SURF_GetDC(IDirectDrawSurface7*, HDC* p) { if (p) *p = (HDC)0; return DD_OK; }
static HRESULT SURF_ReleaseDC(IDirectDrawSurface7*, HDC) { return DD_OK; }
static HRESULT SURF_PageLock(IDirectDrawSurface7*, DWORD) { return DD_OK; }
static HRESULT SURF_PageUnlock(IDirectDrawSurface7*, DWORD) { return DD_OK; }
static HRESULT SURF_QueryInterface(IDirectDrawSurface7* This, REFIID, void** ppv) { if (ppv) *ppv = This; return DD_OK; }
static ULONG   SURF_Release(IDirectDrawSurface7* This) { GLSurface7* s=(GLSurface7*)This; if(s->bits) free(s->bits); free(s); return 0; }
static HRESULT SURF_GetCaps(IDirectDrawSurface7* This, LPDDSCAPS2 c) { GLSurface7* s=(GLSurface7*)This; if(c)*c=s->desc.ddsCaps; return DD_OK; }

static GLSurface7* make_surface(const DDSURFACEDESC2* in, int defW, int defH)
{
	GLSurface7* s = (GLSurface7*)calloc(1, sizeof(GLSurface7));
	s->lpVtbl = &g_surfVtbl;
	if (in) s->desc = *in;
	s->desc.dwSize = sizeof(DDSURFACEDESC2);
	s->w = (in && (in->dwFlags & DDSD_WIDTH)  && in->dwWidth)  ? (int)in->dwWidth  : defW;
	s->h = (in && (in->dwFlags & DDSD_HEIGHT) && in->dwHeight) ? (int)in->dwHeight : defH;
	s->bpp = (in && (in->dwFlags & DDSD_PIXELFORMAT) && in->ddpfPixelFormat.dwRGBBitCount)
	          ? (int)in->ddpfPixelFormat.dwRGBBitCount : 16;
	VLOG("CreateSurface %dx%d bpp%d caps=%08x\n", s->w, s->h, s->bpp,
	     in?(unsigned)in->ddsCaps.dwCaps:0);
	return s;
}

/* ============================ IDirectDraw7 methods ======================= */
static HRESULT DD_CreateSurface(IDirectDraw7*, LPDDSURFACEDESC2 d, IDirectDrawSurface7** out, IUnknown*) {
	if (!out) return DDERR_INVALIDPARAMS;
	GLSurface7* s = make_surface(d, g_scrW, g_scrH);
	/* complex flip chain -> also make the back buffer and attach it */
	if (d && (d->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
		s->isPrimary = 1;
		if (d->dwFlags & DDSD_BACKBUFFERCOUNT && d->dwBackBufferCount > 0) {
			DDSURFACEDESC2 bd; memset(&bd,0,sizeof(bd)); bd.dwSize=sizeof(bd);
			bd.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT;
			bd.dwWidth=g_scrW; bd.dwHeight=g_scrH; bd.ddsCaps.dwCaps=DDSCAPS_BACKBUFFER;
			bd.ddpfPixelFormat=d->ddpfPixelFormat;
			s->back = make_surface(&bd, g_scrW, g_scrH);
		}
	}
	*out = (IDirectDrawSurface7*)s;
	return DD_OK;
}
static HRESULT DD_SetCooperativeLevel(IDirectDraw7* This, HWND h, DWORD f) {
	GLDD7* dd=(GLDD7*)This; dd->hwnd=h; dd->coopFlags=f; ensure_window(g_scrW, g_scrH); return DD_OK;
}
static HRESULT DD_SetDisplayMode(IDirectDraw7*, DWORD w, DWORD h, DWORD, DWORD, DWORD) { ensure_window((int)w,(int)h); return DD_OK; }
static HRESULT DD_RestoreDisplayMode(IDirectDraw7*) { return DD_OK; }
static HRESULT DD_GetCaps(IDirectDraw7*, LPDDCAPS a, LPDDCAPS b) {
	if (a) { memset(a,0,sizeof(DDCAPS)); a->dwSize=sizeof(DDCAPS); a->dwVidMemTotal=256u*1024*1024; a->dwVidMemFree=256u*1024*1024; }
	if (b) { memset(b,0,sizeof(DDCAPS)); b->dwSize=sizeof(DDCAPS); }
	return DD_OK;
}
static HRESULT DD_GetAvailableVidMem(IDirectDraw7*, LPDDSCAPS2, LPDWORD tot, LPDWORD freeM) {
	if (tot) *tot = 256u*1024*1024; if (freeM) *freeM = 256u*1024*1024; return DD_OK;
}
static HRESULT DD_GetDeviceIdentifier(IDirectDraw7*, LPDDDEVICEIDENTIFIER2 id, DWORD) {
	if (id) { memset(id,0,sizeof(*id)); strncpy(id->szDescription,"BoB Linux OpenGL backend",sizeof(id->szDescription)-1); }
	return DD_OK;
}
static HRESULT DD_GetDisplayMode(IDirectDraw7*, LPDDSURFACEDESC2 d) {
	if (d) { memset(d,0,sizeof(*d)); d->dwSize=sizeof(*d); d->dwFlags=DDSD_WIDTH|DDSD_HEIGHT; d->dwWidth=g_scrW; d->dwHeight=g_scrH; }
	return DD_OK;
}
static HRESULT DD_CreateClipper(IDirectDraw7*, DWORD, LPDIRECTDRAWCLIPPER* out, IUnknown*) {
	GLClipper* c=(GLClipper*)calloc(1,sizeof(GLClipper)); c->lpVtbl=&g_clipVtbl; if(out)*out=(IDirectDrawClipper*)c; return DD_OK;
}
static HRESULT DD_CreatePalette(IDirectDraw7*, DWORD, LPPALETTEENTRY src, LPDIRECTDRAWPALETTE* out, IUnknown*) {
	GLPalette* p=(GLPalette*)calloc(1,sizeof(GLPalette)); p->lpVtbl=&g_palVtbl;
	if (src) memcpy(p->ent, src, sizeof(p->ent));
	if (out) *out=(IDirectDrawPalette*)p; return DD_OK;
}
static HRESULT DD_QueryInterface(IDirectDraw7*, REFIID riid, void** ppv) {
	if (!ppv) return DDERR_INVALIDPARAMS;
	if (riid == IID_IDirect3D7) { *ppv = (void*)&g_theD3D; return DD_OK; }
	*ppv = NULL; return E_NOINTERFACE;
}
static ULONG DD_Release(IDirectDraw7* This) { free((void*)This); return 0; }
/* Report a couple of display modes so EnumerateDriverModes builds a list. */
static HRESULT DD_EnumDisplayModes(IDirectDraw7*, DWORD, LPDDSURFACEDESC2, LPVOID ctx, LPDDENUMMODESCALLBACK2 cb) {
	if (!cb) return DD_OK;
	static const int modes[][2] = {{1024,768},{1280,1024},{800,600},{1280,720},{1920,1080}};
	static const int bpps[] = {16, 32};
	for (unsigned m=0; m<sizeof(modes)/sizeof(modes[0]); ++m)
	for (unsigned b=0; b<2; ++b) {
		DDSURFACEDESC2 d; memset(&d,0,sizeof(d)); d.dwSize=sizeof(d);
		d.dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_PIXELFORMAT;
		d.dwWidth=modes[m][0]; d.dwHeight=modes[m][1];
		d.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
		d.ddpfPixelFormat.dwFlags=DDPF_RGB;
		d.ddpfPixelFormat.dwRGBBitCount=bpps[b];
		if (bpps[b]==16){ d.ddpfPixelFormat.dwRBitMask=0xF800; d.ddpfPixelFormat.dwGBitMask=0x07E0; d.ddpfPixelFormat.dwBBitMask=0x001F; }
		else            { d.ddpfPixelFormat.dwRBitMask=0xFF0000; d.ddpfPixelFormat.dwGBitMask=0x00FF00; d.ddpfPixelFormat.dwBBitMask=0x0000FF; }
		if (cb(&d, ctx) == 0 /*DDENUMRET_CANCEL*/) return DD_OK;
	}
	return DD_OK;
}

/* ============================ IDirect3D7 methods ========================= */
typedef HRESULT (*D3DEnumDevCB)(LPSTR, LPSTR, LPD3DDEVICEDESC7, LPVOID);
typedef HRESULT (*D3DEnumZCB)(LPDDPIXELFORMAT, LPVOID);

static void fill_devdesc(D3DDEVICEDESC7* dd, const GUID* guid) {
	memset(dd, 0, sizeof(*dd));
	dd->deviceGUID = *guid;
	dd->dwDevCaps = 0xFFFFFFFF;
	dd->dwDeviceRenderBitDepth = DDBD_16 | DDBD_32;
	dd->dwDeviceZBufferBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
	dd->dwMinTextureWidth = 1; dd->dwMinTextureHeight = 1;
	dd->dwMaxTextureWidth = 4096; dd->dwMaxTextureHeight = 4096;
	dd->dwMaxTextureRepeat = 4096; dd->dwMaxTextureAspectRatio = 4096;
	dd->dwMaxAnisotropy = 16; dd->dwMaxActiveLights = 8;
	dd->dwTextureOpCaps = 0xFFFFFFFF; dd->dwFVFCaps = 0xFFFFFFFF;
	dd->dwVertexProcessingCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwSize = sizeof(dd->dpcLineCaps);
	dd->dpcLineCaps.dwTextureCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwTextureFilterCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwTextureBlendCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwTextureAddressCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwShadeCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwRasterCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwZCmpCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwSrcBlendCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwDestBlendCaps = 0xFFFFFFFF;
	dd->dpcLineCaps.dwAlphaCmpCaps = 0xFFFFFFFF;
	dd->dpcTriCaps = dd->dpcLineCaps;
}
static HRESULT D3D_EnumDevices(IDirect3D7*, void* cbv, LPVOID arg) {
	D3DEnumDevCB cb = (D3DEnumDevCB)cbv; if (!cb) return DD_OK;
	D3DDEVICEDESC7 dd;
	/* HAL first, then TnL-HAL (preferred -> the game cancels on it) */
	fill_devdesc(&dd, &IID_IDirect3DHALDevice);
	if (cb((LPSTR)"HAL", (LPSTR)"OpenGL HAL", &dd, arg) == 0) return DD_OK;
	fill_devdesc(&dd, &IID_IDirect3DTnLHalDevice);
	cb((LPSTR)"TnLHAL", (LPSTR)"OpenGL TnL HAL", &dd, arg);
	return DD_OK;
}
static HRESULT D3D_EnumZBufferFormats(IDirect3D7*, REFCLSID, void* cbv, LPVOID ctx) {
	D3DEnumZCB cb = (D3DEnumZCB)cbv; if (!cb) return DD_OK;
	DDPIXELFORMAT pf;
	memset(&pf,0,sizeof(pf)); pf.dwSize=sizeof(pf); pf.dwFlags=DDPF_ZBUFFER;
	pf.dwZBufferBitDepth=16; pf.dwZBitMask=0x0000FFFF; if (cb(&pf,ctx)==0) return DD_OK;
	memset(&pf,0,sizeof(pf)); pf.dwSize=sizeof(pf); pf.dwFlags=DDPF_ZBUFFER;
	pf.dwZBufferBitDepth=32; pf.dwZBitMask=0xFFFFFFFF; cb(&pf,ctx);
	return DD_OK;
}
static HRESULT D3D_CreateDevice(IDirect3D7*, REFCLSID, LPDIRECTDRAWSURFACE7, LPDIRECT3DDEVICE7* out) {
	if (out) *out = (IDirect3DDevice7*)&g_theDevice; return DD_OK;
}
static HRESULT D3D_CreateVertexBuffer(IDirect3D7*, LPD3DVERTEXBUFFERDESC d, LPDIRECT3DVERTEXBUFFER7* out, DWORD) {
	GLVB7* vb=(GLVB7*)calloc(1,sizeof(GLVB7)); vb->lpVtbl=&g_vbVtbl;
	if (d) { vb->d=*d; size_t n = (size_t)(d->dwNumVertices?d->dwNumVertices:1) * 64; vb->data=calloc(1,n); }
	if (out) *out=(IDirect3DVertexBuffer7*)vb; return DD_OK;
}
static HRESULT D3D_EvictManagedTextures(IDirect3D7*) { return DD_OK; }
static HRESULT D3D_QueryInterface(IDirect3D7* This, REFIID, void** ppv) { if (ppv) *ppv=This; return DD_OK; }

/* ============================ vertex-buffer methods ====================== */
static HRESULT VB_Lock(IDirect3DVertexBuffer7* This, DWORD, LPVOID* pp, LPDWORD ps) {
	GLVB7* vb=(GLVB7*)This; if(pp)*pp=vb->data; if(ps)*ps = vb->d.dwNumVertices*64; return DD_OK;
}
static HRESULT VB_Unlock(IDirect3DVertexBuffer7*) { return DD_OK; }
static HRESULT VB_ProcessVertices(IDirect3DVertexBuffer7*, DWORD, DWORD, DWORD, IDirect3DVertexBuffer7*, DWORD, IDirect3DDevice7*, DWORD) { return DD_OK; }
static ULONG   VB_Release(IDirect3DVertexBuffer7* This) { GLVB7* vb=(GLVB7*)This; if(vb->data)free(vb->data); free(vb); return 0; }

/* ============================ device methods (Phase 1: no-ops) =========== */
static HRESULT DEV_ok(IDirect3DDevice7*) { return D3D_OK; }
static HRESULT DEV_BeginScene(IDirect3DDevice7*) { pump_events(); return D3D_OK; }
static HRESULT DEV_EndScene(IDirect3DDevice7*) { return D3D_OK; }
static HRESULT DEV_Clear(IDirect3DDevice7*, DWORD, LPD3DRECT, DWORD, D3DCOLOR, D3DVALUE, DWORD) { return D3D_OK; }
static HRESULT DEV_SetViewport(IDirect3DDevice7*, LPD3DVIEWPORT7) { return D3D_OK; }
static HRESULT DEV_GetViewport(IDirect3DDevice7*, LPD3DVIEWPORT7 v) {
	if (v) { v->dwX=0; v->dwY=0; v->dwWidth=g_scrW; v->dwHeight=g_scrH; v->dvMinZ=0; v->dvMaxZ=1; } return D3D_OK;
}
static HRESULT DEV_SetRenderTarget(IDirect3DDevice7*, LPDIRECTDRAWSURFACE7, DWORD) { return D3D_OK; }
static HRESULT DEV_SetRenderState(IDirect3DDevice7*, D3DRENDERSTATETYPE, DWORD) { return D3D_OK; }
static HRESULT DEV_GetRenderState(IDirect3DDevice7*, D3DRENDERSTATETYPE, LPDWORD v) { if(v)*v=0; return D3D_OK; }
static HRESULT DEV_SetTextureStageState(IDirect3DDevice7*, DWORD, D3DTEXTURESTAGESTATETYPE, DWORD) { return D3D_OK; }
static HRESULT DEV_GetTextureStageState(IDirect3DDevice7*, DWORD, D3DTEXTURESTAGESTATETYPE, LPDWORD v) { if(v)*v=0; return D3D_OK; }
static HRESULT DEV_SetTransform(IDirect3DDevice7*, D3DTRANSFORMSTATETYPE, LPD3DMATRIX) { return D3D_OK; }
static HRESULT DEV_SetTexture(IDirect3DDevice7*, DWORD, LPDIRECTDRAWSURFACE7) { return D3D_OK; }
static HRESULT DEV_SetMaterial(IDirect3DDevice7*, LPD3DMATERIAL7) { return D3D_OK; }
static HRESULT DEV_SetLight(IDirect3DDevice7*, DWORD, LPD3DLIGHT7) { return D3D_OK; }
static HRESULT DEV_LightEnable(IDirect3DDevice7*, DWORD, BOOL) { return D3D_OK; }
static HRESULT DEV_DrawPrimitiveVB(IDirect3DDevice7*, D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, DWORD) { return D3D_OK; }
static HRESULT DEV_DrawIndexedPrimitiveVB(IDirect3DDevice7*, D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER7, DWORD, DWORD, LPWORD, DWORD, DWORD) { return D3D_OK; }
static HRESULT DEV_CreateStateBlock(IDirect3DDevice7*, DWORD, LPDWORD h) { if(h)*h=1; return D3D_OK; }
static HRESULT DEV_ApplyStateBlock(IDirect3DDevice7*, DWORD) { return D3D_OK; }
static HRESULT DEV_EnumTextureFormats(IDirect3DDevice7*, void* cbv, LPVOID arg) {
	/* report a handful of formats the game understands (RGB565/1555/4444 + 8bpp) */
	typedef HRESULT (*TFCB)(LPDDPIXELFORMAT, LPVOID); TFCB cb=(TFCB)cbv; if(!cb) return DD_OK;
	DDPIXELFORMAT pf;
	struct { DWORD bits, r,g,b,a; } fmts[] = {
		{16,0xF800,0x07E0,0x001F,0}, {16,0x7C00,0x03E0,0x001F,0x8000}, {16,0x0F00,0x00F0,0x000F,0xF000},
	};
	for (unsigned i=0;i<sizeof(fmts)/sizeof(fmts[0]);++i){
		memset(&pf,0,sizeof(pf)); pf.dwSize=sizeof(pf); pf.dwFlags=DDPF_RGB|(fmts[i].a?DDPF_ALPHAPIXELS:0);
		pf.dwRGBBitCount=fmts[i].bits; pf.dwRBitMask=fmts[i].r; pf.dwGBitMask=fmts[i].g; pf.dwBBitMask=fmts[i].b; pf.dwRGBAlphaBitMask=fmts[i].a;
		if (cb(&pf,arg)==0) return DD_OK;
	}
	return DD_OK;
}
static HRESULT DEV_GetCaps(IDirect3DDevice7*, LPD3DDEVICEDESC7 d) { if(d) fill_devdesc(d,&IID_IDirect3DTnLHalDevice); return D3D_OK; }
static HRESULT DEV_GetDirect3D(IDirect3DDevice7*, LPDIRECT3D7* p) { if(p)*p=(IDirect3D7*)&g_theD3D; return D3D_OK; }

/* ============================ clipper / palette ========================== */
static HRESULT CLIP_SetHWnd(IDirectDrawClipper* This, DWORD, HWND h) { ((GLClipper*)This)->hwnd=h; return DD_OK; }
static HRESULT PAL_SetEntries(IDirectDrawPalette* This, DWORD, DWORD start, DWORD n, LPPALETTEENTRY e) {
	GLPalette* p=(GLPalette*)This; if(e && start+n<=256) memcpy(&p->ent[start], e, n*sizeof(PALETTEENTRY)); return DD_OK;
}
static HRESULT PAL_GetEntries(IDirectDrawPalette* This, DWORD, DWORD start, DWORD n, LPPALETTEENTRY e) {
	GLPalette* p=(GLPalette*)This; if(e && start+n<=256) memcpy(e, &p->ent[start], n*sizeof(PALETTEENTRY)); return DD_OK;
}

/* ============================ vtbl wiring ================================ */
static void init_vtbls_once(void)
{
	static int done = 0; if (done) return; done = 1;

	g_surfVtbl.AddRef=(ULONG(*)(IDirectDrawSurface7*))generic_addref;
	g_surfVtbl.Release=SURF_Release;
	g_surfVtbl.QueryInterface=SURF_QueryInterface;
	g_surfVtbl.GetSurfaceDesc=SURF_GetSurfaceDesc;
	g_surfVtbl.Lock=SURF_Lock; g_surfVtbl.Unlock=SURF_Unlock;
	g_surfVtbl.GetAttachedSurface=SURF_GetAttachedSurface;
	g_surfVtbl.AddAttachedSurface=SURF_AddAttachedSurface;
	g_surfVtbl.GetPixelFormat=SURF_GetPixelFormat;
	g_surfVtbl.Blt=SURF_Blt; g_surfVtbl.BltFast=SURF_BltFast; g_surfVtbl.Flip=SURF_Flip;
	g_surfVtbl.SetPalette=SURF_SetPalette; g_surfVtbl.SetClipper=SURF_SetClipper;
	g_surfVtbl.IsLost=SURF_IsLost; g_surfVtbl.Restore=SURF_Restore;
	g_surfVtbl.GetDC=SURF_GetDC; g_surfVtbl.ReleaseDC=SURF_ReleaseDC;
	g_surfVtbl.PageLock=SURF_PageLock; g_surfVtbl.PageUnlock=SURF_PageUnlock;
	g_surfVtbl.GetCaps=SURF_GetCaps;

	g_clipVtbl.AddRef=(ULONG(*)(IDirectDrawClipper*))generic_addref;
	g_clipVtbl.Release=(ULONG(*)(IDirectDrawClipper*))generic_release;
	g_clipVtbl.SetHWnd=CLIP_SetHWnd;

	g_palVtbl.AddRef=(ULONG(*)(IDirectDrawPalette*))generic_addref;
	g_palVtbl.Release=(ULONG(*)(IDirectDrawPalette*))generic_release;
	g_palVtbl.SetEntries=PAL_SetEntries; g_palVtbl.GetEntries=PAL_GetEntries;

	g_vbVtbl.AddRef=(ULONG(*)(IDirect3DVertexBuffer7*))generic_addref;
	g_vbVtbl.Release=VB_Release;
	g_vbVtbl.Lock=VB_Lock; g_vbVtbl.Unlock=VB_Unlock; g_vbVtbl.ProcessVertices=VB_ProcessVertices;

	g_devVtbl.AddRef=(ULONG(*)(IDirect3DDevice7*))generic_addref;
	g_devVtbl.Release=(ULONG(*)(IDirect3DDevice7*))generic_release;
	g_devVtbl.BeginScene=DEV_BeginScene; g_devVtbl.EndScene=DEV_EndScene; g_devVtbl.Clear=DEV_Clear;
	g_devVtbl.SetViewport=DEV_SetViewport; g_devVtbl.GetViewport=DEV_GetViewport;
	g_devVtbl.SetRenderTarget=DEV_SetRenderTarget;
	g_devVtbl.SetRenderState=DEV_SetRenderState; g_devVtbl.GetRenderState=DEV_GetRenderState;
	g_devVtbl.SetTextureStageState=DEV_SetTextureStageState; g_devVtbl.GetTextureStageState=DEV_GetTextureStageState;
	g_devVtbl.SetTransform=DEV_SetTransform; g_devVtbl.SetTexture=DEV_SetTexture;
	g_devVtbl.SetMaterial=DEV_SetMaterial; g_devVtbl.SetLight=DEV_SetLight; g_devVtbl.LightEnable=DEV_LightEnable;
	g_devVtbl.DrawPrimitiveVB=DEV_DrawPrimitiveVB; g_devVtbl.DrawIndexedPrimitiveVB=DEV_DrawIndexedPrimitiveVB;
	g_devVtbl.CreateStateBlock=DEV_CreateStateBlock; g_devVtbl.ApplyStateBlock=DEV_ApplyStateBlock;
	g_devVtbl.EnumTextureFormats=DEV_EnumTextureFormats; g_devVtbl.GetCaps=DEV_GetCaps;
	g_devVtbl.GetDirect3D=DEV_GetDirect3D;
	g_theDevice.lpVtbl=&g_devVtbl;

	g_d3dVtbl.AddRef=(ULONG(*)(IDirect3D7*))generic_addref;
	g_d3dVtbl.Release=(ULONG(*)(IDirect3D7*))generic_release;
	g_d3dVtbl.QueryInterface=D3D_QueryInterface;
	g_d3dVtbl.EnumDevices=D3D_EnumDevices; g_d3dVtbl.CreateDevice=D3D_CreateDevice;
	g_d3dVtbl.CreateVertexBuffer=D3D_CreateVertexBuffer;
	g_d3dVtbl.EnumZBufferFormats=D3D_EnumZBufferFormats;
	g_d3dVtbl.EvictManagedTextures=D3D_EvictManagedTextures;
	g_theD3D.lpVtbl=&g_d3dVtbl;

	g_ddVtbl.AddRef=(ULONG(*)(IDirectDraw7*))generic_addref;
	g_ddVtbl.Release=DD_Release;
	g_ddVtbl.QueryInterface=DD_QueryInterface;
	g_ddVtbl.CreateSurface=DD_CreateSurface;
	g_ddVtbl.SetCooperativeLevel=DD_SetCooperativeLevel;
	g_ddVtbl.SetDisplayMode=DD_SetDisplayMode; g_ddVtbl.RestoreDisplayMode=DD_RestoreDisplayMode;
	g_ddVtbl.GetCaps=DD_GetCaps; g_ddVtbl.GetAvailableVidMem=DD_GetAvailableVidMem;
	g_ddVtbl.GetDeviceIdentifier=DD_GetDeviceIdentifier; g_ddVtbl.GetDisplayMode=DD_GetDisplayMode;
	g_ddVtbl.EnumDisplayModes=DD_EnumDisplayModes;
	g_ddVtbl.CreateClipper=DD_CreateClipper; g_ddVtbl.CreatePalette=DD_CreatePalette;
}

/* ============================ creation entry points ====================== */
extern "C" HRESULT DirectDrawCreateEx(GUID*, LPVOID* lplpDD, REFIID, IUnknown*)
{
	init_vtbls_once();
	GLDD7* dd = (GLDD7*)calloc(1, sizeof(GLDD7));
	dd->lpVtbl = &g_ddVtbl;
	if (lplpDD) *lplpDD = dd;
	VLOG("DirectDrawCreateEx -> %p\n", (void*)dd);
	return DD_OK;
}

extern "C" HRESULT DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA cb, LPVOID ctx, DWORD)
{
	/* report a single primary display driver (NULL GUID = primary) */
	if (cb) cb(NULL, (LPSTR)"Primary Display Driver", (LPSTR)"display", ctx, NULL);
	return DD_OK;
}

/* ====================================================================== *
 * DirectInput (PHASE 1: non-fatal stub -- reports no input).             *
 * Real keyboard/mouse/joystick via SDL events is a later phase; for now  *
 * the game must be able to create the input devices so it proceeds to    *
 * SetDriverAndMode (which brings the window up) and into the game loop.  *
 * ====================================================================== */
static IDirectInputVtbl       g_diVtbl;
static IDirectInputDeviceVtbl g_didevVtbl;
static IDirectInputDeviceA    g_diKeyboard, g_diMouse, g_diJoystick, g_diGeneric;

static HRESULT DIDEV_GetDeviceState(IDirectInputDeviceA*, DWORD cb, LPVOID buf) { if (buf && cb) memset(buf,0,cb); return 0; }
static HRESULT DIDEV_GetDeviceData(IDirectInputDeviceA*, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD inout, DWORD) { if (inout) *inout = 0; return 0; }
static HRESULT DIDEV_ok(IDirectInputDeviceA*) { return 0; }
static HRESULT DIDEV_SetProperty(IDirectInputDeviceA*, REFGUID, LPCDIPROPHEADER) { return 0; }
static HRESULT DIDEV_GetProperty(IDirectInputDeviceA*, REFGUID, LPDIPROPHEADER) { return 0; }
static HRESULT DIDEV_SetDataFormat(IDirectInputDeviceA*, LPCDIDATAFORMAT) { return 0; }
static HRESULT DIDEV_SetCoop(IDirectInputDeviceA*, HWND, DWORD) { return 0; }
static HRESULT DIDEV_EnumObjects(IDirectInputDeviceA*, LPDIENUMDEVICEOBJECTSCALLBACKA, LPVOID, DWORD) { return 0; }
static HRESULT DIDEV_GetCaps(IDirectInputDeviceA*, LPDIDEVCAPS c) { if (c) { DWORD sz=c->dwSize; memset(c,0,sz?sz:sizeof(*c)); c->dwSize=sz?sz:sizeof(*c); } return 0; }
static ULONG   DIDEV_addref(IDirectInputDeviceA*) { return 1; }
static ULONG   DIDEV_release(IDirectInputDeviceA*) { return 0; }

static HRESULT DI_CreateDevice(IDirectInputA*, REFGUID rguid, LPDIRECTINPUTDEVICE* out, IUnknown*) {
	if (!out) return E_FAIL;
	/* hand back a shared dummy device (any of them is fine -- all no-op) */
	if      (rguid == GUID_SysKeyboard) *out = &g_diKeyboard;
	else                                *out = &g_diGeneric;
	return 0;
}
static HRESULT DI_EnumDevices(IDirectInputA*, DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD) { return 0; }
static ULONG   DI_addref(IDirectInputA*) { return 1; }
static ULONG   DI_release(IDirectInputA*) { return 0; }

static IDirectInputA g_theDI;

static void init_dinput_once(void) {
	static int done=0; if (done) return; done=1;
	g_didevVtbl.AddRef=DIDEV_addref; g_didevVtbl.Release=DIDEV_release;
	g_didevVtbl.GetDeviceState=DIDEV_GetDeviceState; g_didevVtbl.GetDeviceData=DIDEV_GetDeviceData;
	g_didevVtbl.Acquire=DIDEV_ok; g_didevVtbl.Unacquire=DIDEV_ok; g_didevVtbl.Poll=DIDEV_ok;
	g_didevVtbl.SetProperty=DIDEV_SetProperty; g_didevVtbl.GetProperty=DIDEV_GetProperty;
	g_didevVtbl.SetDataFormat=DIDEV_SetDataFormat; g_didevVtbl.SetCooperativeLevel=DIDEV_SetCoop;
	g_didevVtbl.EnumObjects=DIDEV_EnumObjects; g_didevVtbl.GetCapabilities=DIDEV_GetCaps;
	g_diKeyboard.lpVtbl=g_diMouse.lpVtbl=g_diJoystick.lpVtbl=g_diGeneric.lpVtbl=&g_didevVtbl;

	g_diVtbl.AddRef=DI_addref; g_diVtbl.Release=DI_release;
	g_diVtbl.CreateDevice=DI_CreateDevice; g_diVtbl.EnumDevices=DI_EnumDevices;
	g_theDI.lpVtbl=&g_diVtbl;
}

extern "C" HRESULT DirectInputCreateA(HINSTANCE, DWORD, LPDIRECTINPUT* ppDI, IUnknown*)
{
	init_dinput_once();
	if (ppDI) *ppDI = &g_theDI;
	return 0;
}

#endif /* FF_LINUX */
