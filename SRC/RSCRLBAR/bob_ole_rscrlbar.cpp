/* bob_ole_rscrlbar.cpp -- host for the CRScrlBarCtrl OCX (FUNC-SWEEP-BOB LBSCROLL-1, 2026-10-01).
   Cross-port of MA's ma_olescroll.cpp (MA S140).

   WHY. RScrlBar (CLSID 0x505aee46) was the R* type BoB never hosted: the UI crawler's logs show
   "[ole] UNHOSTED control clsid.Data1=505aee46 -- wrapper is a no-op" on 141 front-end paths. Every
   list box that outgrows its box asks CRListBoxCtrl::UpdateScrollBar for a bar, and BoB answered with
   nothing -- so with 21 saved campaigns the Load Campaign list painted its rows below the box, over
   the RAF / Luftwaffe / Back / Load buttons, and rows 14+ could never be reached.

   The bar is NOT a template control: UpdateScrollBar creates it at runtime as a child of the LIST BOX
   and places it with Move(). So the dialog walk never sees it (parentDlg is the list box) -- the list
   box host draws and clicks its own bars (bob_ole_rlistbox.cpp).

   Dispids follow RSCRLBRC.CPP's dispatch map: 1 MinValue, 2 MaxValue, 3 StepSize, 4 HorzAlign,
   5 parentPointer, 6 PageSize, 7 ScrollPos, 8 UseMessagesInsteadOfEvents, 9 FileNumOffset, 10 Move.
   The click drives the control's OWN OnLButtonDown/OnLButtonUp (arrow / page / thumb arithmetic),
   exactly as MA's host does; the host supplies only the geometry (GetClientRect). */
#include "stdafx.h"
#include "RScrlBrC.h"           /* the genuine CRScrlBarCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

struct HostRScrlBar : public CRScrlBarCtrl, public OleHost {
    long bx = 0, by = 0, bw = 0, bh = 0;      /* Move() rect, in the LIST BOX's client coordinates */
    void boot(CWnd* /*parent*/) {
        m_hWnd = (HWND)1;                     /* OnDraw / OnLButtonDown take the real path */
        OnResetState();
    }
    void GetClientRect(LPRECT r) const override {
        if (!r) return;
        r->left = r->top = 0;
        r->right = bw > 0 ? bw : 16; r->bottom = bh > 0 ? bh : 16;
    }
    void draw(CDC* pdc, int w, int h) override {
        CRect rc(0, 0, w, h);
        OnDraw(pdc, rc, rc);
    }
    int clickAt(int lx, int ly) {             /* returns the new scroll position */
        long before = GetScrollPos();
        OnLButtonDown(0, CPoint(lx, ly));
        OnLButtonUp(0, CPoint(lx, ly));
        if (getenv("BOB_TRACE_SCROLL"))
            fprintf(stderr, "[scroll] click bar id=%d local(%d,%d) pos %ld -> %ld (min %ld max %ld page %ld)\n",
                    ctrlId, lx, ly, before, (long)GetScrollPos(), (long)GetMinValue(),
                    (long)GetMaxValue(), (long)GetPageSize());
        return (int)GetScrollPos();
    }
    void dispatch(DISPID id, VARTYPE, void* /*pvRet*/, va_list ap) override {
        if (id == 10) {                       /* Move(left, top, right, bottom) */
            long l = va_arg(ap, long), t = va_arg(ap, long), r = va_arg(ap, long), b = va_arg(ap, long);
            bx = l; by = t; bw = r - l; bh = b - t;
            Move(l, t, r, b);
            if (getenv("BOB_TRACE_SCROLL"))
                fprintf(stderr, "[scroll] Move bar id=%d -> (%ld,%ld)-(%ld,%ld)\n", ctrlId, l, t, r, b);
        }
    }
    void setprop(DISPID id, va_list ap) override {
        switch (id) {
        case 1: SetMinValue(va_arg(ap, long)); return;
        case 2: SetMaxValue(va_arg(ap, long)); return;
        case 3: SetStepSize(va_arg(ap, long)); return;
        case 4: SetHorzAlign(va_arg(ap, int)); return;
        case 5: SetParentPointer(va_arg(ap, long)); return;
        case 6: SetPageSize(va_arg(ap, long)); return;
        case 7: SetScrollPos(va_arg(ap, long)); return;
        case 8: SetUseMessagesInsteadOfEvents(va_arg(ap, int)); return;
        case 9: SetFileNumOffset(va_arg(ap, long)); return;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1: *(long*)pvRet = GetMinValue(); return;
        case 2: *(long*)pvRet = GetMaxValue(); return;
        case 3: *(long*)pvRet = GetStepSize(); return;
        case 4: *(BOOL*)pvRet = GetHorzAlign(); return;
        case 5: *(long*)pvRet = GetParentPointer(); return;
        case 6: *(long*)pvRet = GetPageSize(); return;
        case 7: *(long*)pvRet = GetScrollPos(); return;
        case 8: *(BOOL*)pvRet = GetUseMessagesInsteadOfEvents(); return;
        case 9: *(long*)pvRet = GetFileNumOffset(); return;
        }
    }
};

OleHost* bob_make_rscrlbar(CWnd* parent) { HostRScrlBar* h = new HostRScrlBar(); h->boot(parent); return h; }

/* For the list box host: the bar's Move() rect and visibility, its drawing and its click. */
extern "C" int bob_scrlbar_rect(OleHost* h, int* x, int* y, int* w, int* hh) {
    HostRScrlBar* s = dynamic_cast<HostRScrlBar*>(h);
    if (!s || !s->visible || s->bw <= 0 || s->bh <= 0) return 0;
    *x = (int)s->bx; *y = (int)s->by; *w = (int)s->bw; *hh = (int)s->bh;
    return 1;
}
extern "C" void bob_scrlbar_set_parent(OleHost* h, CWnd* dlg) {   /* OnDraw asks the dialog for its art */
    HostRScrlBar* s = dynamic_cast<HostRScrlBar*>(h);
    if (s) s->m_pBobParent = dlg;
}
extern "C" int bob_scrlbar_click(OleHost* h, int lx, int ly) {
    HostRScrlBar* s = dynamic_cast<HostRScrlBar*>(h);
    return s ? s->clickAt(lx, ly) : 0;
}
