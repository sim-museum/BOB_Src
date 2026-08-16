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

/* S131 (MA note 26 §2): a per-FACE registry. The game CreateFonts each g_AllFonts[] with a
   real face name (Intel/FC-Glamour/Fusion -> ART; Arial -> SANS; Times -> SERIF; Courier ->
   MONO -- classified in CFont::bobFaceKind, afxwin.h). Until now bob_gdi drew every face in
   the one art TTF (Intel.ttf), so data/label rows rendered in the Rowan art face instead of
   Arial -- the pervasive "font face" parity deviation. Now each KIND resolves to its own TTF.
   Face 0 (ART) keeps the exact old load order so the ART screens (title menu, headers,
   listbox) stay byte-identical. The current face is set by CDC text draws via
   bob_gdi_set_face(); the front-end menu path leaves it 0 (ART). BOB_NO_FONTFACE forces every
   request back to ART (the pre-S131 behaviour) for A/B + safety. */
/* face code = kind + (italic ? 4 : 0); kind 0=ART 1=SANS 2=SERIF 3=MONO. 8 slots. */
enum { FACE_ART = 0, FACE_SANS = 1, FACE_SERIF = 2, FACE_MONO = 3, FACE_KINDS = 4, FACE_N = 8 };
struct Face { unsigned char* buf; stbtt_fontinfo info; int state; };   /* state: 0 unloaded, 1 ok, -1 fail */
static Face g_faces[FACE_N];
static int  g_curFace = 0;

extern "C" void bob_gdi_set_face(int code) {
	static int noFace = -1;
	if (noFace < 0) noFace = getenv("BOB_NO_FONTFACE") ? 1 : 0;
	g_curFace = (noFace || code < 0 || code >= FACE_N) ? FACE_ART : code;
}

static int try_into(Face* fa, const char* p)   /* fopen + read + InitFont into fa */
{
	if (!p) return 0;
	FILE* f = fopen(p, "rb"); if (!f) return 0;
	fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
	if (n <= 0) { fclose(f); return 0; }
	unsigned char* buf = (unsigned char*)malloc(n);
	size_t got = fread(buf, 1, n, f); fclose(f);
	if ((long)got != n || !stbtt_InitFont(&fa->info, buf, stbtt_GetFontOffsetForIndex(buf, 0))) {
		free(buf); return 0;
	}
	fa->buf = buf; fprintf(stderr, "[gdifont] face loaded %s\n", p); return 1;
}
static int load_face(int code)
{
	if (code < 0 || code >= FACE_N) code = FACE_ART;
	Face* fa = &g_faces[code];
	if (fa->state) return fa->state > 0;
	int kind = code % FACE_KINDS;      /* 0 ART 1 SANS 2 SERIF 3 MONO */
	int ital = code >= FACE_KINDS;     /* italic variant? */
	char path[1200];
	const char* drive = getenv("BOB_DRIVE_C");
	if (kind == FACE_ART) {
		/* ART regular is unchanged from the pre-S131 load_font() (override; the game's own
		   frontend fonts, some non-parseable -> skip; then a system serif). The Rowan art
		   face has no italic variant -> ART italic reuses ART regular. */
		if (ital) { if (load_face(FACE_ART)) { *fa = g_faces[FACE_ART]; return 1; } fa->state = -1; return 0; }
		const char* gameFonts[] = { "Intel.ttf", "g101016_.ttf", "FUSION_B.TTF", NULL };
		if (try_into(fa, getenv("BOB_FONT"))) { fa->state = 1; return 1; }
		for (int i = 0; drive && gameFonts[i]; i++) {
			snprintf(path, sizeof(path), "%s/windows/Fonts/%s", drive, gameFonts[i]);
			if (try_into(fa, path)) { fa->state = 1; return 1; }
		}
		if (try_into(fa, "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf")) { fa->state = 1; return 1; }
	} else {
		/* SANS/SERIF/MONO: metric-compatible Liberation faces (Arial/Times/Courier), regular
		   or italic per `ital`, DejaVu fallback. Gold's English data/label rows are Arial
		   (values italic); LiberationSans matches its metrics. */
		const char* cand[3] = { NULL, NULL, NULL };
		if (kind == FACE_SANS) {
			cand[0] = ital ? "/usr/share/fonts/truetype/liberation/LiberationSans-Italic.ttf"
			               : "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
			cand[1] = ital ? "/usr/share/fonts/truetype/dejavu/DejaVuSans-Oblique.ttf"
			               : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
		} else if (kind == FACE_SERIF) {
			cand[0] = ital ? "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf"
			               : "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf";
			cand[1] = ital ? "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Italic.ttf"
			               : "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf";
		} else { /* MONO */
			cand[0] = ital ? "/usr/share/fonts/truetype/liberation/LiberationMono-Italic.ttf"
			               : "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf";
			cand[1] = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
		}
		for (int i = 0; i < 3; i++) if (try_into(fa, cand[i])) { fa->state = 1; return 1; }
		/* last resort: the regular variant of the same kind, then ART, so text still renders */
		if (ital && load_face(kind)) { *fa = g_faces[kind]; return 1; }
		if (load_face(FACE_ART)) { *fa = g_faces[FACE_ART]; return 1; }
	}
	fprintf(stderr, "[gdifont] FAILED to load face code %d\n", code); fa->state = -1; return 0;
}
/* resolve the font to render/measure with -- the current face, ART fallback. */
static stbtt_fontinfo* cur_font(void)
{
	int k = g_curFace;
	if (!load_face(k)) { if (!load_face(FACE_ART)) return NULL; k = FACE_ART; }
	return &g_faces[k].info;
}

/* Draw `str` at (x,y) top-left, glyph pixel height `pixelH`, colour 0x00RRGGBB.
   Antialiased, alpha-blended into the BGRA framebuffer. Returns the advance width. */
/* S164 (cross-port from MA S135/S141): a DRAWN-TEXT diagnostic.
 *
 * BOB_TRACE_TEXT=<substring>  trace every draw whose text contains <substring>, uncapped.
 * BOB_TRACE_TEXT=1            trace the first 24 draws (a general look).
 * BOB_TRACE_GARBAGE=1         report only text containing bytes outside printable ASCII.
 *
 * The last one is the useful one, and it is why this exists. MA found 53 sites of
 * `sprintf("%s", <CString>)` -- an idiom that works by accident on MSVC (CString is one pointer
 * and the ABI copies it) and passes the object's ADDRESS under GCC, so %s prints the raw bytes of
 * a pointer. It never crashes and never logs; it just draws a few bytes of rubbish where a word
 * should be, which reads as "the label is missing" from across the room. A detector that watches
 * what actually reaches the rasteriser finds the whole class at once, including any site a grep
 * would miss.
 *
 * Filter, don't cap: a fixed print budget is spent by whatever draws first (the menu), so the
 * screen under investigation never gets a line. Booked six times in MA before it stuck. */
static void bob_text_trace(const char* str)
{
	const char* want = getenv("BOB_TRACE_TEXT");
	if (want) {
		if (want[0] == '1' && !want[1]) {
			static int n = 0;
			if (n++ < 24) fprintf(stderr, "[text] \"%s\"\n", str);
		} else if (strstr(str, want)) {
			fprintf(stderr, "[text] \"%s\"\n", str);
		}
	}
	if (getenv("BOB_TRACE_GARBAGE")) {
		for (const unsigned char* p = (const unsigned char*)str; *p; ++p) {
			if (*p < 0x20 || *p >= 0x7f) {
				fprintf(stderr, "[garbage] drawn text has non-printable bytes:");
				for (const unsigned char* q = (const unsigned char*)str; *q && q < (const unsigned char*)str + 32; ++q)
					fprintf(stderr, " %02x", *q);
				fprintf(stderr, "  as-text \"%s\"\n", str);
				fflush(stderr);
				break;
			}
		}
	}
}

extern "C" int bob_gdi_text(int x, int y, const char* str, int pixelH, unsigned color)
{
	stbtt_fontinfo* fnt = cur_font();
	if (!fnt || !str || pixelH <= 0) return 0;
	bob_text_trace(str);
	int fbw, fbh; unsigned* fb = bob_gdi_dc_bits(&fbw, &fbh);
	if (!fb) return 0;
	float scale = stbtt_ScaleForPixelHeight(fnt, (float)pixelH);
	int ascent; stbtt_GetFontVMetrics(fnt, &ascent, NULL, NULL);
	int baseline = y + (int)(ascent * scale + 0.5f);
	unsigned cr = (color >> 16) & 0xff, cg = (color >> 8) & 0xff, cb = color & 0xff;
	float penx = (float)x;
	for (const unsigned char* p = (const unsigned char*)str; *p; p++) {
		int x0, y0, x1, y1;
		stbtt_GetCodepointBitmapBox(fnt, *p, scale, scale, &x0, &y0, &x1, &y1);
		int gw = x1 - x0, gh = y1 - y0;
		if (gw > 0 && gh > 0) {
			unsigned char* glyph = (unsigned char*)malloc((size_t)gw * gh);
			stbtt_MakeCodepointBitmap(fnt, glyph, gw, gh, gw, scale, scale, *p);
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
		int aw; stbtt_GetCodepointHMetrics(fnt, *p, &aw, NULL);
		penx += aw * scale;
		if (p[1]) penx += stbtt_GetCodepointKernAdvance(fnt, p[0], p[1]) * scale;
	}
	return (int)penx - x;
}

/* Draw a 1px line (x0,y0)->(x1,y1) into the GDI framebuffer (R* control borders).
   color is 0xRRGGBB. Bresenham; clipped to the framebuffer. */
extern "C" void bob_gdi_line(int x0, int y0, int x1, int y1, unsigned color)
{
	int fbw, fbh; unsigned* fb = bob_gdi_dc_bits(&fbw, &fbh);
	if (!fb) return;
	unsigned px = 0xFF000000u | (color & 0xFFFFFF);
	int dx = x1 - x0, dy = y1 - y0;
	int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
	int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
	int err = (adx > ady ? adx : -ady) / 2, e2;
	for (;;) {
		if (x0 >= 0 && x0 < fbw && y0 >= 0 && y0 < fbh) fb[(size_t)y0 * fbw + x0] = px;
		if (x0 == x1 && y0 == y1) break;
		e2 = err;
		if (e2 > -adx) { err -= ady; x0 += sx; }
		if (e2 <  ady) { err += adx; y0 += sy; }
	}
}

/* Measure the advance width of `str` at `pixelH` without drawing (for centring/click rects). */
extern "C" int bob_gdi_text_width(const char* str, int pixelH)
{
	stbtt_fontinfo* fnt = cur_font();
	if (!fnt || !str || pixelH <= 0) return 0;
	float scale = stbtt_ScaleForPixelHeight(fnt, (float)pixelH);
	float penx = 0;
	for (const unsigned char* p = (const unsigned char*)str; *p; p++) {
		int aw; stbtt_GetCodepointHMetrics(fnt, *p, &aw, NULL);
		penx += aw * scale;
		if (p[1]) penx += stbtt_GetCodepointKernAdvance(fnt, p[0], p[1]) * scale;
	}
	return (int)penx;
}
