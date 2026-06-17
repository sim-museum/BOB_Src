/* RCombo host: the genuine CRComboCtrl + dispid routing. Separate TU so its
   RCOMBO/resource.h context doesn't collide with RLISTBOX's. */
#include "stdafx.h"
#include "rlistbox.h"           /* CRListBox wrapper (RComboC.h's dropdown forward) */
#include "RCombo.h"             /* RCombo module header (RCOMBO/resource.h: IDD_LISTBOX) */
#include "uiicons.h"            /* IconsUI (RComboC.h DrawTransparentBitmap param) */
#include "RComboC.h"            /* the genuine CRComboCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;

/* RCombo dispatch/event IIDs (RComboC ctor's InitializeIIDs; values from RCombo.odl).
   Used by-address only, so exact values are immaterial -- they just need to exist. */
extern const GUID IID_DRCombo       = { 0x737cb0ca, 0xb42b, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
extern const GUID IID_DRComboEvents = { 0x737cb0cb, 0xb42b, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

struct HostRCombo : public CRComboCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        CRect rc(0, 0, w, h);
        /* Keep the control on its direct-to-pdc path: after the first OnDraw the genuine code
           switches to an offscreen-DC route (parent->SendMessage(WM_GETOFFSCREENDC) +
           CreateCompatibleBitmap), which the GDI compat doesn't provide -> NULL deref on any
           repaint (e.g. after a combo cycles). Forcing m_FirstSweep each draw renders straight
           onto the framebuffer, matching the front-end's draw model. */
        m_FirstSweep = TRUE;
        OnDraw(pdc, rc, rc);
    }
    /* Click cycles to the next value (the genuine R* combo is a click-to-advance spinner, not a
       drop-list). Advances the real CRComboCtrl's index so OnDraw shows the new value, and the
       config's writeback pass (PreDestroyPanel reads GetIndex) persists it on tab change. */
    int onClick() override {
        int n = m_list.GetCount();
        if (n <= 1) return 0;
        long cur = GetIndex();
        long nxt = (cur + 1) % n;
        SetIndex((int)nxt);
        if (bob_ole_trace()) fprintf(stderr, "[ole]   RCombo cycle index %ld -> %ld (of %d)\n", cur, nxt, n);
        return 1;
    }
    void dispatch(DISPID id, VARTYPE /*vtRet*/, void* pvRet, va_list ap) override {
        switch (id) {
        case 9:  { const char* t = va_arg(ap, const char*); AddString(t ? t : "");
                   if (bob_ole_trace()) fprintf(stderr, "[ole]   Combo AddString \"%s\" (n=%d)\n", t?t:"", m_list.GetCount()); } break;
        case 10: { long v = GetListbox(); if (pvRet) *(long*)pvRet = v; } break;
        case 11: { long r = va_arg(ap, long); SetIndex(r); } break;
        case 12: { long v = GetIndex();   if (pvRet) *(long*)pvRet = v; } break;
        case 13: Clear(); break;
        case 14: { long i = va_arg(ap, long); DeleteString(i); } break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RCombo: unhandled method dispid %ld\n", (long)id); break;
        }
    }
    void setprop(DISPID id, va_list ap) override {
        if (id == DISPID_CAPTION_ || id == DISPID_TEXT_) {
            const char* t = va_arg(ap, const char*); InternalSetText(t ? t : ""); return;
        }
        long v = va_arg(ap, long);
        switch (id) {
        case 1: SetFontNum(v); break;
        case 2: SetListboxLength(v); break;
        case 3: SetCircularStyle((BOOL)v); break;
        case 4: SetShadowColor((OLE_COLOR)v); break;
        case 5: SetEndFileNum(v); break;
        case 6: SetFileNumMain(v); break;
        case 7: SetStyle(v); break;
        case 8: SetNewJimVar(v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        case DISPID_BACKCOLOR_: SetBackColor((OLE_COLOR)v); break;
        case DISPID_ENABLED_:   SetEnabled((BOOL)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RCombo: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1: *(long*)pvRet = GetFontNum(); break;
        case 2: *(long*)pvRet = GetListboxLength(); break;
        case 3: *(BOOL*)pvRet = GetCircularStyle(); break;
        case 7: *(long*)pvRet = GetStyle(); break;
        case DISPID_FORECOLOR_: *(OLE_COLOR*)pvRet = GetForeColor(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RCombo: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rcombo(CWnd* parent) { HostRCombo* h = new HostRCombo(); h->boot(parent); return h; }

/* WM_GETFILE handler (replicates RDialog::OnGetFile, RDIALMSG.CPP): the R* controls'
   OnDraw fetch their icons/art this way. Icons (filenum>=0x10000) -> a live IconDescUI
   (the dropdown arrow etc.); MaskIcon then draws it. File/art (0x6600-0x7200) loading is
   not wired here yet (returns NULL -> those controls skip their art for now). */
extern "C" void* bob_dlg_getfile(int filenum) {
    if (filenum >= 0x10000) {
        static IconDescUI s_axicon;
        s_axicon = IconsUI(filenum);
        return (void*)&s_axicon;
    }
    return NULL;
}

/* WM_GETGLOBALFONT handler (replicates OnGetGlobalFont): the R* controls' OnDraw
   selects this font, and (via CFont::m_height -> CDC::SelectObject) it sets the real
   text height. g_AllFonts is created at front-end init (MIG.CPP). [0] = 1x scale. */
extern CFont* g_AllFonts[14][4];   /* MAXFONTS=14 (rdialog.h) */
extern "C" void* bob_dlg_getfont(int fontnum) {
    if (fontnum < 0) fontnum = -fontnum;
    if (fontnum >= 14) fontnum = 0;
    /* the front-end panels are drawn scaled-up (template rects x resolution), so the
       2x font [3] matches the box heights; fall back to [0] when [3] wasn't created. */
    CFont* f = g_AllFonts[fontnum][3];
    if (!f) f = g_AllFonts[fontnum][0];
    return (void*)f;
}
