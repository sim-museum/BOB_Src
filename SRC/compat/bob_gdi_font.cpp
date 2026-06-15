/* GDI text rendering via stb_truetype (header-only, public domain).
   Rasterizes the game's own TTF (windows/Fonts/FUSION_B.TTF) -- or a system fallback --
   into the GDI BGRA framebuffer (bob_gdi_dc_bits, in bob_video.cpp). Backs the front-end
   menu text and CDC::ExtTextOut. No external lib dependency (stb is all-inline). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 8)          /* keep stb's structs native-ABI despite the global -fpack-struct=1 */
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"
#pragma pack(pop)

extern "C" unsigned* bob_gdi_dc_bits(int*, int*);

static unsigned char* g_fontBuf = NULL;
static stbtt_fontinfo  g_font;
static int g_fontState = 0;    /* 0 = unloaded, 1 = ok, -1 = failed */

static int try_one(const char* p)   /* fopen + read + InitFont; keeps g_fontBuf on success */
{
	if (!p) return 0;
	FILE* f = fopen(p, "rb"); if (!f) return 0;
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	if (n <= 0) { fclose(f); return 0; }
	unsigned char* buf = (unsigned char*)malloc(n);
	size_t got = fread(buf, 1, n, f); fclose(f);
	if ((long)got != n || !stbtt_InitFont(&g_font, buf, stbtt_GetFontOffsetForIndex(buf, 0))) {
		free(buf); return 0;
	}
	g_fontBuf = buf; fprintf(stderr, "[gdifont] loaded %s\n", p); return 1;
}
static int load_font(void)
{
	if (g_fontState) return g_fontState > 0;
	char path[1200];
	const char* drive = getenv("BOB_DRIVE_C");
	/* Try, in order: override; the game's frontend fonts (some are non-standard TTFs stb
	   can't parse -> skip to the next); then a system serif close to the wine menu face. */
	const char* gameFonts[] = { "Intel.ttf", "g101016_.ttf", "FUSION_B.TTF", NULL };
	if (try_one(getenv("BOB_FONT"))) { g_fontState = 1; return 1; }
	for (int i = 0; drive && gameFonts[i]; i++) {
		snprintf(path, sizeof(path), "%s/windows/Fonts/%s", drive, gameFonts[i]);
		if (try_one(path)) { g_fontState = 1; return 1; }
	}
	if (try_one("/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf")) { g_fontState = 1; return 1; }
	fprintf(stderr, "[gdifont] FAILED to load any font\n"); g_fontState = -1; return 0;
}

/* Draw `str` at (x,y) top-left, glyph pixel height `pixelH`, colour 0x00RRGGBB.
   Antialiased, alpha-blended into the BGRA framebuffer. Returns the advance width. */
extern "C" int bob_gdi_text(int x, int y, const char* str, int pixelH, unsigned color)
{
	if (!load_font() || !str || pixelH <= 0) return 0;
	int fbw, fbh; unsigned* fb = bob_gdi_dc_bits(&fbw, &fbh);
	if (!fb) return 0;
	float scale = stbtt_ScaleForPixelHeight(&g_font, (float)pixelH);
	int ascent; stbtt_GetFontVMetrics(&g_font, &ascent, NULL, NULL);
	int baseline = y + (int)(ascent * scale + 0.5f);
	unsigned cr = (color >> 16) & 0xff, cg = (color >> 8) & 0xff, cb = color & 0xff;
	float penx = (float)x;
	for (const unsigned char* p = (const unsigned char*)str; *p; p++) {
		int x0, y0, x1, y1;
		stbtt_GetCodepointBitmapBox(&g_font, *p, scale, scale, &x0, &y0, &x1, &y1);
		int gw = x1 - x0, gh = y1 - y0;
		if (gw > 0 && gh > 0) {
			unsigned char* glyph = (unsigned char*)malloc((size_t)gw * gh);
			stbtt_MakeCodepointBitmap(&g_font, glyph, gw, gh, gw, scale, scale, *p);
			int ox = (int)penx + x0, oy = baseline + y0;
			for (int gy = 0; gy < gh; gy++) for (int gx = 0; gx < gw; gx++) {
				int a = glyph[gy * gw + gx]; if (!a) continue;
				int px = ox + gx, py = oy + gy;
				if (px < 0 || px >= fbw || py < 0 || py >= fbh) continue;
				unsigned* d = &fb[(size_t)py * fbw + px];
				unsigned db = (*d) & 0xff, dg = ((*d) >> 8) & 0xff, dr = ((*d) >> 16) & 0xff;
				int r = (cr * a + dr * (255 - a)) / 255;
				int g = (cg * a + dg * (255 - a)) / 255;
				int b = (cb * a + db * (255 - a)) / 255;
				*d = 0xFF000000u | ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b;
			}
			free(glyph);
		}
		int aw; stbtt_GetCodepointHMetrics(&g_font, *p, &aw, NULL);
		penx += aw * scale;
		if (p[1]) penx += stbtt_GetCodepointKernAdvance(&g_font, p[0], p[1]) * scale;
	}
	return (int)penx - x;
}

/* Measure the advance width of `str` at `pixelH` without drawing (for centring/click rects). */
extern "C" int bob_gdi_text_width(const char* str, int pixelH)
{
	if (!load_font() || !str || pixelH <= 0) return 0;
	float scale = stbtt_ScaleForPixelHeight(&g_font, (float)pixelH);
	float penx = 0;
	for (const unsigned char* p = (const unsigned char*)str; *p; p++) {
		int aw; stbtt_GetCodepointHMetrics(&g_font, *p, &aw, NULL);
		penx += aw * scale;
		if (p[1]) penx += stbtt_GetCodepointKernAdvance(&g_font, p[0], p[1]) * scale;
	}
	return (int)penx;
}
