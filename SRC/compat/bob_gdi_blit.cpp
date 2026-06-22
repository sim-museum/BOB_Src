/* R6.1 -- GDI blit subsystem (BitBlt / StretchBlt / CBitmap / CreateDIBitmap).
 *
 * The front-end icon/box-art system and the strategic map both render through GDI:
 *   - IconDescUI::LoadInstances decodes an icon-sheet .bmp with CreateDIBitmap and
 *     SelectObject's it into a memory DC (imagemapinstances[]);
 *   - MaskIcon / SolidIcon then BitBlt rectangles from that memory DC onto the target
 *     DC with raster ops (SRCAND mask + SRCPAINT colour, or SRCCOPY);
 *   - the strategic map (CMIGView::UpdateBitmaps) BitBlt/StretchBlt's 256x256 map tiles.
 * The compat had all of this stubbed (CreateDIBitmap->NULL, CDC::BitBlt->no-op), so no
 * icon or map tile ever reached the framebuffer.
 *
 * This file provides the real pixel plumbing: a bitmap registry (handle -> ARGB pixels),
 * a DIB decoder (8/24/32-bit, top- or bottom-up), and a raster-op blit between bitmaps
 * and/or the bob_gdi front-end framebuffer. CDC/CBitmap (afxwin.h) + CreateDIBitmap
 * (compat_wingdi.h) are thin inline wrappers over these extern "C" entry points.
 *
 * Pixel format throughout: 0x00RRGGBB (matches the bob_gdi framebuffer, which presents
 * (p>>16,p>>8,p) as R,G,B). ROPs operate bitwise on the packed RGB, exactly like Win32. */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" unsigned* bob_gdi_dc_bits(int* w, int* h);   /* the front-end framebuffer */

namespace {
struct BobBmp { int w, h; uint32_t* pix; };
void* g_lastDib = nullptr;   /* most-recently decoded bitmap (for the BOB_BLIT_TEST self-test) */
int   g_mapPaintActive = 0;  /* R4.2: while a strategic-map paint runs, the C-GDI StretchDIBits/
                                FillSolidRect target the front-end framebuffer (map coords are
                                full-window/screen coords). Off otherwise so stray GDI is a no-op. */

/* The standard Win32 BITMAPINFOHEADER prefix we need (matches compat_wingdi.h's struct
   layout: DWORD biSize, LONG biWidth/biHeight, WORD biPlanes/biBitCount, DWORD biCompression...). */
#pragma pack(push,1)
struct DibHdr { uint32_t biSize; int32_t biWidth; int32_t biHeight; uint16_t biPlanes;
                uint16_t biBitCount; uint32_t biCompression; uint32_t biSizeImage;
                int32_t biXPPM, biYPPM; uint32_t biClrUsed, biClrImportant; };
#pragma pack(pop)
}

extern "C" {

/* --- bitmap registry ------------------------------------------------------ */
void* bob_bmp_create(int w, int h) {
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return nullptr;
    BobBmp* b = (BobBmp*)calloc(1, sizeof(BobBmp));
    if (!b) return nullptr;
    b->w = w; b->h = h;
    b->pix = (uint32_t*)calloc((size_t)w * h, 4);
    if (!b->pix) { free(b); return nullptr; }
    return b;
}

void bob_bmp_free(void* h) { if (!h) return; BobBmp* b = (BobBmp*)h; free(b->pix); free(b); }

uint32_t* bob_bmp_pixels(void* h, int* w, int* ph) {
    if (!h) return nullptr; BobBmp* b = (BobBmp*)h;
    if (w) *w = b->w; if (ph) *ph = b->h; return b->pix;
}

void bob_bmp_dims(void* h, int* w, int* ph) {
    BobBmp* b = (BobBmp*)h; if (w) *w = b ? b->w : 0; if (ph) *ph = b ? b->h : 0;
}

/* Decode a packed DIB (BITMAPINFOHEADER + palette + bits) into a registered bitmap.
   `info` points at the BITMAPINFOHEADER; the RGBQUAD palette follows it (for <=8 bpp);
   `bits` is the pixel data. Handles 8-bit palettised, 24-bit BGR, 32-bit BGRX, with
   bottom-up (biHeight>0) or top-down (biHeight<0) row order. Uncompressed only. */
void* bob_dib_decode(const void* info, const void* bits) {
    if (!info || !bits) return nullptr;
    const DibHdr* h = (const DibHdr*)info;
    int w = h->biWidth, hh = h->biHeight;
    bool topDown = hh < 0; if (hh < 0) hh = -hh;
    if (w <= 0 || hh <= 0 || w > 8192 || hh > 8192) return nullptr;
    if (h->biCompression != 0) return nullptr;     /* BI_RGB only */
    int bpp = h->biBitCount;
    BobBmp* b = (BobBmp*)bob_bmp_create(w, hh);
    if (!b) return nullptr;

    const uint8_t* pal = (const uint8_t*)info + (h->biSize ? h->biSize : 40);  /* RGBQUAD[] */
    const uint8_t* src = (const uint8_t*)bits;
    int srcRowBytes = ((w * bpp + 31) / 32) * 4;    /* DWORD-aligned rows */

    for (int row = 0; row < hh; row++) {
        int srow = topDown ? row : (hh - 1 - row);  /* bottom-up -> flip to top-down */
        const uint8_t* sr = src + (size_t)srow * srcRowBytes;
        uint32_t* dr = b->pix + (size_t)row * w;
        for (int x = 0; x < w; x++) {
            uint32_t rgb;
            if (bpp == 8) {
                const uint8_t* q = pal + (size_t)sr[x] * 4;   /* RGBQUAD: B,G,R,reserved */
                rgb = ((uint32_t)q[2] << 16) | ((uint32_t)q[1] << 8) | q[0];
            } else if (bpp == 24) {
                const uint8_t* q = sr + (size_t)x * 3;        /* BGR */
                rgb = ((uint32_t)q[2] << 16) | ((uint32_t)q[1] << 8) | q[0];
            } else if (bpp == 32) {
                const uint8_t* q = sr + (size_t)x * 4;        /* BGRX */
                rgb = ((uint32_t)q[2] << 16) | ((uint32_t)q[1] << 8) | q[0];
            } else if (bpp == 4) {
                uint8_t idx = (x & 1) ? (sr[x >> 1] & 0x0f) : (sr[x >> 1] >> 4);
                const uint8_t* q = pal + (size_t)idx * 4;
                rgb = ((uint32_t)q[2] << 16) | ((uint32_t)q[1] << 8) | q[0];
            } else if (bpp == 1) {
                uint8_t idx = (sr[x >> 3] >> (7 - (x & 7))) & 1;
                const uint8_t* q = pal + (size_t)idx * 4;
                rgb = ((uint32_t)q[2] << 16) | ((uint32_t)q[1] << 8) | q[0];
            } else { rgb = 0; }
            dr[x] = rgb;
        }
    }
    g_lastDib = b;
    if (getenv("BOB_TRACE_BLIT")) { static int n=0; if(n++<12)
        fprintf(stderr, "[blit] dib decode %dx%d bpp%d %s -> bmp=%p\n", w, hh, bpp,
                topDown?"top-down":"bottom-up", (void*)b); }
    if (getenv("BOB_DUMP_BLIT")) {   /* dump the decoded bitmap to verify the DIB decode */
        static int dn=0; char path[64]; snprintf(path,sizeof(path),"/tmp/blit_dib_%d.ppm",dn++);
        FILE* fp=fopen(path,"wb");
        if (fp) { fprintf(fp,"P6\n%d %d\n255\n",w,hh);
            for (int i=0;i<w*hh;i++){ uint32_t p=b->pix[i]; unsigned char rgb[3]={(unsigned char)(p>>16),(unsigned char)(p>>8),(unsigned char)p}; fwrite(rgb,1,3,fp); }
            fclose(fp); fprintf(stderr,"[blit] dumped decoded DIB -> %s\n",path); }
    }
    return b;
}

/* Raster-op blit. Destination is the bob_gdi framebuffer (dstScreen, offset by the
   target DC's viewport dvpx/dvpy) or a registered bitmap (dstBmp). Source likewise.
   rop: SRCCOPY/SRCAND/SRCPAINT/SRCINVERT/BLACKNESS/WHITENESS (bitwise on packed RGB). */
void bob_blit(int dstScreen, void* dstBmp, int dvpx, int dvpy,
              int dx, int dy, int w, int h,
              int srcScreen, void* srcBmp, int sx, int sy, unsigned long rop) {
    if (w <= 0 || h <= 0) return;
    uint32_t* dpix; int dw, dh;
    if (dstScreen) { int fw, fh; dpix = (uint32_t*)bob_gdi_dc_bits(&fw, &fh); dw = fw; dh = fh; dx += dvpx; dy += dvpy; }
    else { BobBmp* b = (BobBmp*)dstBmp; if (!b) return; dpix = b->pix; dw = b->w; dh = b->h; }
    if (!dpix) return;

    uint32_t* spix = nullptr; int sw = 0, sh = 0;
    if (srcScreen) { int fw, fh; spix = (uint32_t*)bob_gdi_dc_bits(&fw, &fh); sw = fw; sh = fh; }
    else if (srcBmp) { BobBmp* b = (BobBmp*)srcBmp; spix = b->pix; sw = b->w; sh = b->h; }

    for (int row = 0; row < h; row++) {
        int ty = dy + row; if (ty < 0 || ty >= dh) continue;
        int syy = sy + row;
        for (int col = 0; col < w; col++) {
            int tx = dx + col; if (tx < 0 || tx >= dw) continue;
            uint32_t s = 0;
            if (spix) { int sxx = sx + col;
                if (sxx >= 0 && sxx < sw && syy >= 0 && syy < sh) s = spix[(size_t)syy * sw + sxx]; }
            uint32_t* d = &dpix[(size_t)ty * dw + tx];
            switch (rop) {
                case 0x00CC0020: *d = s; break;                 /* SRCCOPY  */
                case 0x008800C6: *d &= s; break;                /* SRCAND   */
                case 0x00EE0086: *d |= s; break;                /* SRCPAINT */
                case 0x00660046: *d ^= s; break;                /* SRCINVERT*/
                case 0x00000042: *d = 0; break;                 /* BLACKNESS*/
                case 0x00FF0062: *d = 0xFFFFFF; break;          /* WHITENESS*/
                default: *d = s; break;
            }
        }
    }
}

/* Stretch blit (nearest-neighbour). Used by the strategic-map tile draw. */
void bob_stretchblit(int dstScreen, void* dstBmp, int dvpx, int dvpy,
                     int dx, int dy, int dwd, int dhd,
                     int srcScreen, void* srcBmp, int sx, int sy, int sws, int shs, unsigned long rop) {
    if (dwd <= 0 || dhd <= 0 || sws <= 0 || shs <= 0) return;
    uint32_t* dpix; int dw, dh;
    if (dstScreen) { int fw, fh; dpix = (uint32_t*)bob_gdi_dc_bits(&fw, &fh); dw = fw; dh = fh; dx += dvpx; dy += dvpy; }
    else { BobBmp* b = (BobBmp*)dstBmp; if (!b) return; dpix = b->pix; dw = b->w; dh = b->h; }
    if (!dpix) return;
    uint32_t* spix = nullptr; int sw = 0, sh = 0;
    if (srcScreen) { int fw, fh; spix = (uint32_t*)bob_gdi_dc_bits(&fw, &fh); sw = fw; sh = fh; }
    else if (srcBmp) { BobBmp* b = (BobBmp*)srcBmp; spix = b->pix; sw = b->w; sh = b->h; }
    if (!spix) return;
    for (int row = 0; row < dhd; row++) {
        int ty = dy + row; if (ty < 0 || ty >= dh) continue;
        int syy = sy + (int)((int64_t)row * shs / dhd);
        for (int col = 0; col < dwd; col++) {
            int tx = dx + col; if (tx < 0 || tx >= dw) continue;
            int sxx = sx + (int)((int64_t)col * sws / dwd);
            uint32_t s = (sxx >= 0 && sxx < sw && syy >= 0 && syy < sh) ? spix[(size_t)syy * sw + sxx] : 0;
            uint32_t* d = &dpix[(size_t)ty * dw + tx];
            if (rop == 0x008800C6) *d &= s; else if (rop == 0x00EE0086) *d |= s; else *d = s;
        }
    }
}

/* --- R4.2 strategic-map C-GDI plumbing -------------------------------------
 * The map (CMIGView::UpdateBitmaps) draws with the C GDI API on an HDC: StretchDIBits for
 * each 256x256 terrain tile and FillSolidRect for the backdrop. HDC carries no surface in
 * compat, but map coords are full-window screen coords, so while a map paint is active we
 * route these straight to the bob_gdi framebuffer. */
void bob_map_paint_begin(void) { g_mapPaintActive = 1; }
void bob_map_paint_end(void)   { g_mapPaintActive = 0; }

/* StretchDIBits: decode the source DIB (info+bits) and stretch-blit a sub-rect to the
   framebuffer. Mirrors the Win32 signature's geometry (dest x/y/w/h, src x/y/w/h). */
int bob_stretchdibits(int xd, int yd, int wd, int hd, int xs, int ys, int ws, int hs,
                      const void* bits, const void* info, unsigned long rop) {
    if (!g_mapPaintActive || !bits || !info) return 0;
    void* tmp = bob_dib_decode(info, bits);
    if (!tmp) return 0;
    /* src sub-rect (xs,ys,ws,hs) of the decoded tile -> dest (xd,yd,wd,hd) on the framebuffer */
    bob_stretchblit(/*dstScreen*/1, NULL, 0, 0, xd, yd, wd, hd,
                    /*srcScreen*/0, tmp, xs, ys, ws, hs, rop);
    bob_bmp_free(tmp);
    return hd;
}

/* FillSolidRect on the framebuffer (map backdrop). rgb is 0x00RRGGBB. */
void bob_gdi_fillrect(int x, int y, int w, int h, unsigned long rgb) {
    if (!g_mapPaintActive) return;
    int fw, fh; uint32_t* fb = (uint32_t*)bob_gdi_dc_bits(&fw, &fh); if (!fb) return;
    for (int row = 0; row < h; row++) { int ty = y + row; if (ty < 0 || ty >= fh) continue;
        for (int col = 0; col < w; col++) { int tx = x + col; if (tx < 0 || tx >= fw) continue;
            fb[(size_t)ty * fw + tx] = (uint32_t)rgb; } }
}

/* R6.1 self-test: SRCCOPY-blit the most-recently decoded bitmap (the icon sheet) onto the
   front-end framebuffer top-left, exercising the full screen-target blit path with real
   decoded pixels. Gated BOB_BLIT_TEST; called from bob_gdi_present just before the swap. */
void bob_blit_selftest(void) {
    if (!g_lastDib) return;
    bob_blit(/*dstScreen*/1, NULL, 0, 0, /*dx,dy*/8, 8, /*w,h*/256, 192,
             /*srcScreen*/0, g_lastDib, /*sx,sy*/0, 0, 0x00CC0020 /*SRCCOPY*/);
}

}  /* extern "C" */
