/* RTabs host: the genuine CRTabsCtrl (the tab strip of every HTabBox dialog) + dispid routing.
   TABHEAD-1 (2026-09-25). The PO: "several report pages are missing headings" -- RAF Resources
   (10/11/12/13 Group), Review (Strength/Aircraft/Assets/Claims/Enemy), Asset List, ... all show
   their pages with no tab row, and always the LAST page. RTabs was the one R* control type this
   port never hosted ([ole] UNHOSTED control clsid.Data1=4a1e1986), so:
     * RDialog::AddChildren's `GetDlgItem(IDJ_TABCTRL)` got a no-op wrapper: GetFirstTab() always
       answered 0, so AttachTabToTabControl treated EVERY page as the first one and never hid the
       others; and AddTab/SelectTab went nowhere -- no strip was drawn and none could be clicked;
     * the pages are prepended to the sibling list (RDIALOG.CPP:737 `dial->sibling=fchild`), so
       the paint walk's `fchild` is the page added LAST -- 13 Group, Enemy -- which is what the
       PO saw instead of the first tab.

   Dispids (from the wrapper, SRC/MFC/RTABS.CPP, and RTABSCTL.H):
     1 FirstTab | 2 HorzAlign | 3 FontNum | 4 SetShadow | 5 AddTab(BSTR,I4) | 6 Clear
     7 CalculateHeight(I4)->I4 | 8 CalcWidestWord()->I4 | 9 SelectTab(I4)->I4 ; event 1 SelectTab(I4)

   Two things the genuine control needs that this port does not provide, supplied host-side:
   (1) Its tab art is the OCX's OWN bitmap resources (IDB_TABUP/IDB_TABDOWN, RTABS.RC), fetched
       with CBitmap::LoadBitmap -- a stub here. They are read from the installed RTabs.ocx's PE
       resources instead (the same loader boblang.dll goes through) and pre-selected into the
       control's m_TabUpDC/m_TabDownDC, with m_bInit cleared so OnDraw does not redo it.
   (2) OnDraw composes into an OFFSCREEN DC and blits it back; this port's GDI draws text only on
       a screen DC, so the offscreen text would be lost. draw() runs the same horizontal row
       layout as OnDraw (copied, not reinterpreted) straight onto the screen DC and calls the
       genuine DrawRow for each row, so the tab rects the click handler reads are the genuine
       ones. The parent width both use comes from GetParent()->GetClientRect(), which this port
       answers with the whole screen -- so GetParent() is a small proxy that reports the tab
       strip's own drawn width and forwards everything else to the real dialog. */
#include "stdafx.h"
#include "RTABSCTL.H"
#include "resource.h"            /* RTABS/resource.h: IDB_TABUP / IDB_TABDOWN */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <typeinfo>

extern int g_bobListFontH;
extern "C" void* bob_LoadLibrary(const char* path);
extern "C" const void* bob_res_get(void* h, unsigned type, unsigned id, unsigned* outSize);
extern "C" int bob_evt_fire(void* dlg, const void* tinfo, int id, int dispid);
extern "C" { extern long bob_evtA0, bob_evtA1; extern void* bob_evtP; }

/* GetParent() proxy: the genuine layout asks the PARENT for its client width. */
struct BobTabParentProxy : public CWnd {
    int w = 0, h = 0;
    void GetClientRect(LPRECT r) const override {
        if (!r) return;
        r->left = r->top = 0; r->right = w; r->bottom = h;
    }
};

static void* bob_rtabs_module()
{
    static void* mod = (void*)-1;
    if (mod == (void*)-1) {
        mod = bob_LoadLibrary("RTabs.ocx");
        if (!mod) mod = bob_LoadLibrary("\\Program Files\\Rowan Software\\Battle Of Britain\\RTabs.ocx");
        fprintf(stderr, "[rtabs] RTabs.ocx resources %s\n", mod ? "loaded" : "NOT FOUND -- tabs draw text only");
    }
    return mod;
}

/* RT_BITMAP (2) resource -> bob bitmap handle. The resource is a packed DIB: BITMAPINFOHEADER,
   colour table, bits. */
static void* bob_rtabs_bitmap(unsigned id)
{
    void* mod = bob_rtabs_module();
    if (!mod) return NULL;
    unsigned sz = 0;
    const unsigned char* p = (const unsigned char*)bob_res_get(mod, 2, id, &sz);
    if (!p || sz < 40) return NULL;
    unsigned biSize = p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24);
    unsigned bpp = p[14] | (p[15] << 8);
    unsigned comp = p[16] | (p[17] << 8) | (p[18] << 16) | ((unsigned)p[19] << 24);
    unsigned used = p[32] | (p[33] << 8) | (p[34] << 16) | ((unsigned)p[35] << 24);
    unsigned ncol = used ? used : (bpp <= 8 ? (1u << bpp) : 0);
    if (comp == 3) ncol = 3;                 /* BI_BITFIELDS masks */
    if (biSize + ncol * 4 >= sz) return NULL;
    return bob_dib_decode(p, p + biSize + ncol * 4);
}

struct HostRTabs : public CRTabsCtrl, public OleHost {
    BobTabParentProxy proxy;
    CWnd* realParent = NULL;
    void boot(CWnd* parent) {
        realParent = parent;
        m_pBobParent = &proxy;               /* see (2) above */
        m_hWnd = (HWND)1;                    /* the control's SendMessage/font paths test m_hWnd */
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
        /* (1): the OCX's own tab art, selected into the control's DCs exactly as OnDraw would */
        void* up = bob_rtabs_bitmap(IDB_TABUP);
        void* dn = bob_rtabs_bitmap(IDB_TABDOWN);
        m_TabUp.m_hObject = (HGDIOBJ)up;
        m_TabDown.m_hObject = (HGDIOBJ)dn;
        m_TabUpDC.CreateCompatibleDC(NULL);   m_TabUpDC.SelectObject(&m_TabUp);
        m_TabDownDC.CreateCompatibleDC(NULL); m_TabDownDC.SelectObject(&m_TabDown);
        m_TabUpWidth = m_TabUpHeight = m_TabDownWidth = m_TabDownHeight = 0;
        if (up) bob_bmp_dims(up, &m_TabUpWidth, &m_TabUpHeight);
        if (dn) bob_bmp_dims(dn, &m_TabDownWidth, &m_TabDownHeight);
        m_bInit = FALSE;
        if (bob_ole_trace()) fprintf(stderr, "[rtabs] art up=%dx%d down=%dx%d\n",
            m_TabUpWidth, m_TabUpHeight, m_TabDownWidth, m_TabDownHeight);
    }
    void applyDesignProps() override {
        const unsigned char* bp; int bn;
        if (!bob_dlg_propbag(dlgId, ctrlId, &bp, &bn)) return;
        CPropExchange px;
        if (!px.Attach(bp, bn)) return;
        HWND save = m_hWnd; m_hWnd = 0;
        DoPropExchange(&px);
        m_hWnd = save;
    }
    /* the strip is laid out by RDialog::OnSize (tabcontrol->MoveWindow(0,0,w,CalculateHeight(w))),
       not by its template rect (8,12,51,23 in IDDS_EMPTYPAGE), so its geometry is live */
    int liveGeometry() override { return 1; }
    int isTabStrip() override { return 1; }
    int tabStripHeight(int w) override { proxy.w = w; return m_textList.GetCount() ? CalculateHeight(w) : 0; }
    void GetClientRect(LPRECT r) const override {
        if (!r) return;
        r->left = r->top = 0; r->right = sw > 0 ? sw : 16; r->bottom = sh > 0 ? sh : 30;
    }
    int  tabCount() { return m_textList.GetCount(); }
    int  selected() { return m_iCurrentSelection; }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        proxy.w = w; proxy.h = h;
        int count = m_textList.GetCount();
        if (count == 0) return;
        if (!m_bHorzAlign) {                 /* VTabBox strips: not reached by any RAF report page */
            static int said = 0;
            if (!said++) fprintf(stderr, "[rtabs] vertical tab strip (dlg=%d) not drawn\n", dlgId);
            return;
        }
        m_rectList.RemoveAll();
        m_tabList.RemoveAll();
        pdc->SetTextAlign(TA_LEFT | TA_TOP);
        pdc->SetBkMode(TRANSPARENT);
        pdc->SetTextColor(TranslateColor(GetForeColor()));
        CFont* pOldFont = pdc->SelectObject((CFont*)realParent->SendMessage(WM_GETGLOBALFONT,
                                             m_FontNum < 0 ? -m_FontNum : m_FontNum, NULL));
        CRect rect;
        this->GetParent()->GetClientRect(rect);
        /* ---- CRTabsCtrl::OnDraw's horizontal branch (RTABSCTL.CPP), onto pdc ---- */
        POSITION position = m_textList.GetHeadPosition();
        char* text;
        int width, start = 0, finish;
        float seperation, lastseperation = 0;
        int total = 0, laststart = 0, lastfinish = 0;
        BOOL lastrow = FALSE, notreallylastrow = FALSE;
        int row = 0;
        for (int x = 0; x < count; x++) {
            if (x == m_iCurrentSelection) lastrow = TRUE;
            text = m_textList.GetNext(position);
            width = pdc->GetTextExtent(text).cx + 15;
            total += width;
            if (total > rect.Width() || x == count - 1) {
                if (x > start && total > rect.Width()) {
                    if (x == m_iCurrentSelection) notreallylastrow = TRUE;
                    x--;
                    if (position == NULL) position = m_textList.GetTailPosition();
                    else m_textList.GetPrev(position);
                    total -= width;
                }
                finish = x;
                seperation = (float)(rect.Width() - total + (15 * (finish - start + 1))) / (float)(finish - start + 1);
                if (lastrow == FALSE || notreallylastrow == TRUE) {
                    DrawRow(seperation, start, finish, pdc, row, rect.Width(), FALSE);
                    notreallylastrow = FALSE;
                } else {
                    laststart = start; lastfinish = finish; lastseperation = seperation;
                    lastrow = FALSE;
                    row--;
                }
                total = 0;
                start = x + 1;
                row++;
            }
        }
        DrawRow(lastseperation, laststart, lastfinish, pdc, row, rect.Width(), TRUE);
        pdc->SelectObject(pOldFont);
    }
    /* A click selects the tab under it through the genuine OnLButtonDown (it walks the m_rectList
       DrawRow just built and calls SelectTab -> ShowWindow on the pages), then delivers the
       SelectTab event the genuine FireSelectTab cannot (FireEvent is a no-op macro here) to the
       container's ON_EVENT (RDEmptyP::OnSelectTabTabctrl). */
    int onClickXY(int localX, int localY) override {
        int before = m_iCurrentSelection;
        OnLButtonDown(0, CPoint(localX, localY));
        if (bob_ole_trace() || getenv("BOB_TRACE_TABS"))
            fprintf(stderr, "[rtabs] click dlg=%d local=(%d,%d) tab %d -> %d of %d\n",
                    dlgId, localX, localY, before, m_iCurrentSelection, m_textList.GetCount());
        if (m_iCurrentSelection != before && realParent) {
            long body = 0;
            POSITION p = m_windowList.FindIndex(m_iCurrentSelection);
            if (p) body = (long)m_windowList.GetAt(p);
            bob_evtA0 = body; bob_evtA1 = 0; bob_evtP = 0;
            bob_evt_fire((void*)realParent, &typeid(*realParent), ctrlId, 1 /*SelectTab*/);
        }
        return 1;                            /* a click on the strip is always consumed */
    }
    void dispatch(DISPID id, VARTYPE, void* pvRet, va_list ap) override {
        switch (id) {
        case 5: { const char* t = va_arg(ap, const char*); long wnd = va_arg(ap, long);
                  AddTab(t ? t : "", wnd);
                  if (bob_ole_trace() || getenv("BOB_TRACE_TABS"))
                      fprintf(stderr, "[rtabs] dlg=%d AddTab '%s' page=%p (%d tabs)\n",
                              dlgId, t ? t : "", (void*)wnd, m_textList.GetCount()); } break;
        case 6: Clear(); break;
        case 7: { long tw = va_arg(ap, long); long r = CalculateHeight(tw); if (pvRet) *(long*)pvRet = r; } break;
        case 8: { long r = CalcWidestWord(); if (pvRet) *(long*)pvRet = r; } break;
        case 9: { long t = va_arg(ap, long); long r = SelectTab(t); if (pvRet) *(long*)pvRet = r; } break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RTabs: unhandled method dispid %ld\n", (long)id); break;
        }
    }
    void setprop(DISPID id, va_list ap) override {
        long v = va_arg(ap, long);
        switch (id) {
        case 1: SetFirstTab(v); break;
        case 2: SetHorzAlign((BOOL)v); break;
        case 3: SetFontNum(v); break;
        case 4: SetSetShadow((short)v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        case DISPID_BACKCOLOR_: SetBackColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RTabs: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1: *(long*)pvRet = GetFirstTab(); break;
        case 2: *(BOOL*)pvRet = GetHorzAlign(); break;
        case 3: *(long*)pvRet = GetFontNum(); break;
        case 4: *(short*)pvRet = GetSetShadow(); break;
        case DISPID_FORECOLOR_: *(OLE_COLOR*)pvRet = GetForeColor(); break;
        case DISPID_BACKCOLOR_: *(OLE_COLOR*)pvRet = GetBackColor(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RTabs: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rtabs(CWnd* parent) { HostRTabs* h = new HostRTabs(); h->boot(parent); return h; }
