/* tools/clip_probe.cpp -- R21 (S402): does bob's text clip actually clip?
 *
 * WHY THIS EXISTS. MA's twin probe (port/clip_probe.cpp) found that MA's ETO_CLIPPED wiring was
 * INERT: MA has two pixel writers, and the one the clip tested was only the bitmap-font fallback,
 * while every real string went through an unclipped TTF path. I had read that code twice and
 * concluded the opposite both times. BoB's clip (S395) and its ETO_CLIPPED wiring (S398) were
 * written the same way -- by reading -- and have never been measured. So measure them.
 *
 * No display and no SDL: bob_gdi_dc_bits is EXTERNAL to bob_gdi_font.cpp, so this stubs it with a
 * plain buffer and links the SHIPPED font object.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const int W = 400, H = 60;
static unsigned* g_fb;

extern "C" {
    unsigned* bob_gdi_dc_bits(int* w, int* h) { if (w) *w = W; if (h) *h = H; return g_fb; }
    int  bob_gdi_text(int x, int y, const char* s, int pixelH, unsigned color);
    void bob_gdi_text_clip(int x0, int y0, int x1, int y1);
    void bob_gdi_get_text_clip(int* x0, int* y0, int* x1, int* y1);
}

static void count(int cut, long* left, long* right) {
    *left = *right = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (g_fb[y * W + x] & 0x00FFFFFFu) { if (x < cut) (*left)++; else (*right)++; }
}

int main(void) {
    int fails = 0;
    g_fb = (unsigned*)calloc((size_t)W * H, 4);
    const char* msg = "CLIPPING TEST STRING WIDE ENOUGH TO CROSS THE CUT";
    const int CUT = 150;

    /* 1. unclipped -- the control. If the text does not cross the cut, every later arm is vacuous. */
    memset(g_fb, 0, (size_t)W * H * 4);
    bob_gdi_text(4, 10, msg, 20, 0xFFFFFF);
    long l0, r0; count(CUT, &l0, &r0);
    printf("  unclipped:            left=%ld right=%ld\n", l0, r0);
    if (!(l0 > 0 && r0 > 0)) { printf("  FAIL: text did not cross the cut -- test is vacuous\n"); fails++; }

    /* 2. clipped to the left of the cut */
    memset(g_fb, 0, (size_t)W * H * 4);
    bob_gdi_text_clip(0, 0, CUT, H);
    bob_gdi_text(4, 10, msg, 20, 0xFFFFFF);
    bob_gdi_text_clip(0, 0, 0, 0);
    long l1, r1; count(CUT, &l1, &r1);
    printf("  clipped to x<%d:      left=%ld right=%ld\n", CUT, l1, r1);
    if (r1 != 0) { printf("  FAIL: text drew OUTSIDE the clip rect\n"); fails++; }
    if (l1 == 0) { printf("  FAIL: text vanished INSIDE the clip rect\n"); fails++; }

    /* 3. an EMPTY rect must mean NO clip, not "clip everything" */
    memset(g_fb, 0, (size_t)W * H * 4);
    bob_gdi_text_clip(0, 0, 0, 0);
    bob_gdi_text(4, 10, msg, 20, 0xFFFFFF);
    long l2, r2; count(CUT, &l2, &r2);
    printf("  empty rect (0,0,0,0): left=%ld right=%ld\n", l2, r2);
    if (l2 + r2 == 0) { printf("  FAIL: an empty rect blanked the text\n"); fails++; }

    printf(fails == 0 ? "\nPASS: the text clip clips, an empty rect does not\n" : "\nFAIL (%d)\n", fails);
    free(g_fb);
    return fails == 0 ? 0 : 1;
}
