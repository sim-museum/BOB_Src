/* RStatic host: the genuine CRStaticCtrl + dispid routing. Separate TU for its
   own resource context. RStatic is a label: text comes from SetString (runtime,
   used by campaign/name/reminder screens) or a design-time ResourceNumber (the
   config labels — not yet available without the DLGINIT property bag). */
#include "stdafx.h"
#include "RStaticC.h"           /* the genuine CRStaticCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;

extern const GUID IID_DRStatic       = { 0xc42bac3e, 0xca3c, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
extern const GUID IID_DRStaticEvents = { 0xc42bac3f, 0xca3c, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

struct HostRStatic : public CRStaticCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    void applyDesignProps() override {
        /* S126: replay the genuine persisted property stream (stock Caption/ForeColor,
           FontNum, design String, ResourceNumber, ShadowColor) through the control's
           own DoPropExchange — the full CPropExchange surrogate. m_hWnd=0 during the
           replay: on Windows loading precedes window creation. */
        const unsigned char* p; int n; int streamed = 0;
        if (bob_dlg_propbag(dlgId, ctrlId, &p, &n)) {
            CPropExchange px;
            if (px.Attach(p, n)) {
                HWND save = m_hWnd; m_hWnd = 0;
                DoPropExchange(&px);
                m_hWnd = save;
                streamed = 1;
                if (bob_ole_trace()) fprintf(stderr,
                    "[ole] RStatic dlg=%d id=%d stream: FontNum=%ld ResNum=%ld fore=%06lx \"%s\"\n",
                    dlgId, ctrlId, (long)GetFontNum(), (long)GetResourceNumber(),
                    (unsigned long)GetForeColor(), (const char*)m_string);
            }
        }
        if (streamed && GetResourceNumber()) return;
        /* runtime caption resolves genuinely: GetParentWndInfo -> WM_GETSTRING
           (ResourceNumber) -> BDG string table, at first draw. Without a stream
           (BOB_NO_PROP_STREAM) or without a ResourceNumber, keep the S124 caption
           path (IDS-name -> string table, falling back to the design literal). */
        char cap[64];
        if (bob_dlg_caption(dlgId, ctrlId, cap, sizeof cap) && cap[0]) SetString(cap);
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        CRect rc(0, 0, w, h);
        OnDraw(pdc, rc, rc);
    }
    void dispatch(DISPID id, VARTYPE, void*, va_list) override {
        if (bob_ole_trace()) fprintf(stderr, "[ole] RStatic: unhandled method dispid %ld\n", (long)id);
    }
    void setprop(DISPID id, va_list ap) override {
        /* String/Caption are BSTR; the rest are long/BOOL/COLOR */
        if (id == 3 || id == DISPID_CAPTION_ || id == DISPID_TEXT_) {
            const char* t = va_arg(ap, const char*);
            if (id == 3) SetString(t ? t : ""); else InternalSetText(t ? t : "");
            return;
        }
        long v = va_arg(ap, long);
        switch (id) {
        case 1: /* UpdateCaption */ break;
        case 2: SetFontNum(v); break;
        case 4: SetResourceNumber(v); break;
        case 5: SetPictureFileNum(v); break;
        case 6: SetCentral((BOOL)v); break;
        case 7: SetShadowColor((OLE_COLOR)v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RStatic: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 2: *(long*)pvRet = GetFontNum(); break;
        case 4: *(long*)pvRet = GetResourceNumber(); break;
        case 6: *(BOOL*)pvRet = GetCentral(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RStatic: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rstatic(CWnd* parent) { HostRStatic* h = new HostRStatic(); h->boot(parent); return h; }
