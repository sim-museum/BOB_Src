/* RButton host: the genuine CRButtonCtrl + dispid routing. Separate TU for its own
   RBUTTON/resource.h context. The strategic-map toolbars (CMainToolbar/CMiscToolbar/
   TitleBar) host their buttons as this OCX; its OnDraw blits the per-state art FileNum
   (NormalFileNum/PressedFileNum) via WM_GETFILE -> SetDIBitsToDevice. Mirrors the RCombo/
   RStatic hosts (bob_ole_host.h interface). S88 (campaign Phase 2 button rows). */
#include "stdafx.h"
#include "RButton.h"            /* RButton module header (RBUTTON/resource.h) */
#include "RButtonC.h"           /* the genuine CRButtonCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;
extern "C" int bob_icon_pagenum(const char* name);   /* ICON_* name -> ICON_PAGE value (GETFILE.CPP) */

/* RButton dispatch/event IIDs (values from RButton.odl; used by-address only). */
extern const GUID IID_DRButton       = { 0x78918644, 0xa917, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
extern const GUID IID_DRButtonEvents = { 0x78918645, 0xa917, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

struct HostRButton : public CRButtonCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    void applyDesignProps() override {
        /* the button face art: NormalFileNumString ("FIL_...") from the DLGINIT bag ->
           SetNormalFileNumString resolves it to m_NormalFileNum via GetFileNum. */
        /* S90: the strategic-map toolbars default MOST buttons' NormalFileNumString to a shared
           "FIL_ICON_BASES" in the .rc; the shipped game differentiates each by function at runtime,
           but that assignment isn't in this source drop's toolbar code. Reconstruct it -- map each
           toolbar control id to its matching iconnum.g sheet icon (1:1 by function), so the row
           shows the correct distinct faces (bases/weather/squadrons/... per the wine reference). */
        static const struct { int id; const char* icon; } kBtnIcon[] = {
            {1807,"ICON_WEATHER"},{1827,"ICON_BASES"},{1829,"ICON_SQUADRONS"},{1832,"ICON_PILOTS"},
            {1835,"ICON_ASSETS"},{1837,"ICON_REVIEW"},{1841,"ICON_MISSIONS"},{1844,"ICON_AIRCRAFT"},
            {1848,"ICON_HOSTILES"},{1001,"ICON_THUMB"},{1003,"ICON_SAVE"},{1004,"ICON_ZOOM"},
            {1005,"ICON_ZOOM"},{1006,"ICON_MAPFILTERS"},{1007,"ICON_DIRECTIVES"},{1055,"ICON_REPLAY"},
            /* TitleBar accel/time controls (S94) */
            {1838,"ICON_PAUSE"},{1842,"ICON_PLAY"},{1845,"ICON_FFORWARD"},
        };
        int forced = 0;
        for (unsigned k = 0; k < sizeof kBtnIcon/sizeof kBtnIcon[0]; k++)
            if (kBtnIcon[k].id == ctrlId) { int pv = bob_icon_pagenum(kBtnIcon[k].icon);
                if (pv) { SetNormalFileNum(pv); forced = 1; } break; }
        char art[48]; int got = bob_dlg_artname(dlgId, ctrlId, art, sizeof art);
        if (!forced && got && art[0]) {
            /* FIL_ICON_* faces are sprite-sheet icons -> resolve to their ICON_PAGE value (the
               map-icon/IconDescUI renderer draws it), not the renamed/absent per-file art. */
            const char* icn = strncmp(art, "FIL_", 4) == 0 ? art + 4 : art;
            int pv = (strncmp(icn, "ICON_", 5) == 0) ? bob_icon_pagenum(icn) : 0;
            if (pv) SetNormalFileNum(pv);
            else    SetNormalFileNumString(art);   /* standalone-BMP faces (teleback, thumbnail, ...) */
        }
        if (bob_ole_trace()) fprintf(stderr, "[ole] RButton dlg=%d id=%d art=%s -> NormalFileNum=0x%lx\n",
            dlgId, ctrlId, got?art:"(none)", (long)GetNormalFileNum());
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        CRect rc(0, 0, w, h);
        /* Force the direct-to-pdc sweep each paint: the genuine transparent path switches to
           an offscreen DC via parent->SendMessage(WM_GETOFFSCREENDC) (not provided by the GDI
           compat) -> NULL deref. m_FirstSweep=TRUE keeps art rendering straight onto pdc, as the
           RCombo host does. */
        m_FirstSweep = TRUE;
        m_bDrawing = FALSE;
        OnDraw(pdc, rc, rc);
    }
    /* S92: a click on this button signals bob_ole_click to fire its Clicked event (dispid 1) on the
       owning toolbar -> the genuine ON_EVENT handler (OnClickedBases/Missionfolder/...). */
    int onClick() override { return 1; }
    void dispatch(DISPID id, VARTYPE, void*, va_list) override {
        if (bob_ole_trace()) fprintf(stderr, "[ole] RButton: unhandled method dispid %ld\n", (long)id);
    }
    void setprop(DISPID id, va_list ap) override {
        /* String/Caption/*String props are BSTR (const char*); the rest long/BOOL/COLOR. */
        switch (id) {
        case 0x8:  { const char* t = va_arg(ap, const char*); SetString(t ? t : ""); return; }
        case 0xc:  { const char* t = va_arg(ap, const char*); SetNormalFileNumString(t ? t : ""); return; }
        case 0xd:  { const char* t = va_arg(ap, const char*); SetPressedFileNumString(t ? t : ""); return; }
        case 0x15: { const char* t = va_arg(ap, const char*); SetHintString(t ? t : ""); return; }
        case DISPID_CAPTION_: { const char* t = va_arg(ap, const char*); InternalSetText(t ? t : ""); return; }
        default: break;
        }
        long v = va_arg(ap, long);
        switch (id) {
        case 0x1:  m_updateCaption = (BOOL)v; break;   /* DISP_PROPERTY_NOTIFY member (no setter) */
        case 0x2:  SetMovesParent((BOOL)v); break;
        case 0x3:  SetFontNum(v); break;
        case 0x4:  SetCloseButton((BOOL)v); break;
        case 0x5:  SetTickButton((BOOL)v); break;
        case 0x6:  SetShowShadow((BOOL)v); break;
        case 0x7:  SetShadowColor((unsigned long)v); break;
        case 0x9:  SetResourceNumber(v); break;
        case 0xa:  SetNormalFileNum(v); break;
        case 0xb:  SetPressedFileNum(v); break;
        case 0xe:  SetPressed((BOOL)v); break;
        case 0xf:  SetDisabled((BOOL)v); break;
        case 0x12: SetTransparency((BOOL)v); break;
        case 0x13: SetTransparentBitMap(v); break;
        case 0x14: SetHelpButton((BOOL)v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        case DISPID_BACKCOLOR_: SetBackColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RButton: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 0x3:  *(long*)pvRet = GetFontNum(); break;
        case 0x9:  *(long*)pvRet = GetResourceNumber(); break;
        case 0xa:  *(long*)pvRet = GetNormalFileNum(); break;
        case 0xb:  *(long*)pvRet = GetPressedFileNum(); break;
        case 0xe:  *(BOOL*)pvRet = GetPressed(); break;
        case 0xf:  *(BOOL*)pvRet = GetDisabled(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RButton: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rbutton(CWnd* parent) { HostRButton* h = new HostRButton(); h->boot(parent); return h; }
