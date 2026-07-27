/* RListBox host: the genuine CRListBoxCtrl + dispid routing. Separate TU so its
   RLISTBOX/resource.h context doesn't collide with RCOMBO's. Also defines the
   typelib/IID symbols the (uncompiled) RListBox module would have provided. */
#include "stdafx.h"
#include "rscrlbar.h"           /* CRScrlBar (RListBoxCtl members) */
#include "RListBxC.h"           /* the genuine CRListBoxCtrl */
#include "bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;

/* typelib + dispatch/event IIDs (RListBox.odl + RListBox.cpp module, not compiled).
   extern: must have external linkage to match the control headers' extern decls. */
extern const GUID _tlid               = { 0x90b5eda5, 0x666f, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
extern const WORD _wVerMajor          = 1;
extern const WORD _wVerMinor          = 7;
extern const GUID IID_DRListBox       = { 0x90b5eda6, 0x666f, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
extern const GUID IID_DRListBoxEvents = { 0x90b5eda7, 0x666f, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

struct HostRListBox : public CRListBoxCtrl, public OleHost {
    int bagCols = 0;                 /* S125: columns came from the DLGINIT bag */
    void boot(CWnd* parent) {
        m_pBobParent = parent;       /* GetParent() -> dialog, for OnDraw/ResizeToFit */
        m_hWnd = (HWND)1;            /* sentinel: OnDraw is a no-op unless m_hWnd != 0 */
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    /* S125 (#16): the genuine DoPropExchange creates this control's columns from the
       persisted `A0..A8` widths + `C0..C8` align/icon codes (0=left 1=right >=8 icon);
       our empty-bag boot lost them, so e.g. CSCampaign's phase-tab row (4x180px,
       cols 2-3 right-aligned = the gold full-width spread) collapsed to Shrink()'d
       tight columns. Recreate them exactly as RLISTBXC.CPP:470 does. The boot-time
       default AddColumn(50) is released first (Clear() with colwidths[0]==0 drops
       all columns). Widths are applied unscaled (m_hWnd=0), as on Windows where
       DoPropExchange runs before window creation. */
    void applyDesignProps() override {
        short w[9]; int a[9];
        int n = bob_dlg_columns(dlgId, ctrlId, w, a);
        if (n <= 0) return;
        Clear();
        for (int c = 0; c < n; c++) {
            HWND save = m_hWnd; m_hWnd = 0;
            if (a[c] >= 8) AddIconColumn(w[c]); else AddColumn(w[c]);
            m_hWnd = save;
            if (a[c] == 1) SetColumnRightAligned(c, TRUE);
        }
        bagCols = n;
        if (bob_ole_trace()) fprintf(stderr, "[ole] RListBox dlg=%d id=%d: %d bag columns (w0=%d)\n",
            dlgId, ctrlId, n, (int)w[0]);
    }
    void draw(CDC* pdc, int w, int h) override {
        /* recompute column widths at the *draw* height so the control's own layout
           matches the rendered glyphs (population may have used a different one).
           S125: NOT when the columns are the authored bag layout — Windows only
           Shrink()s when the game asks (PositionRListBox); shrinking the authored
           columns is what tight-packed the phase-tab row (#16). */
        g_bobListFontH = pdc->m_bobTextH;
        if (!bagCols) Shrink();
        CRect rc(0, 0, w, h);
        OnDraw(pdc, rc, rc);
    }
    /* R4.4: map a click's local Y (relative to the control's drawn top) to a list row. */
    int rowAtY(int localY) override { return (int)(short)GetRowFromY(localY); }
    /* R4.1: some hosted listboxes (e.g. CSCampaign's IDC_RLIST_CAMPAIGNS) rely on a column
       count persisted in the OCX property bag rather than calling AddColumn -- the game just
       AddString's into column 0..N. Our host boots from an empty CPropExchange, so m_list has
       no columns and AddString(text, col) derefs a NULL position. Auto-create the missing
       columns on demand (default width 100; real widths are recomputed at draw via Shrink).
       m_hWnd is cleared around AddColumn so it skips the GetDC/global-font width scaling. */
    void ensureColumns(int col) {
        while ((int)m_list.GetCount() <= col) {
            HWND save = m_hWnd; m_hWnd = 0;
            AddColumn(100);
            m_hWnd = save;
        }
    }
    void dispatch(DISPID id, VARTYPE /*vtRet*/, void* pvRet, va_list ap) override {
        switch (id) {
        case 55: { short r = GetCount();                       if (pvRet) *(short*)pvRet = r; } break;
        case 56: { const char* t = va_arg(ap, const char*); int i = va_arg(ap, int); ensureColumns(i); AddString(t ? t : "", (short)i);
                   if (bob_ole_trace()) fprintf(stderr, "[ole]   AddString[col %d] \"%s\"  (cols=%d, rows=%d)\n", i, t?t:"", m_list.GetCount(), (int)GetCount()); } break;
        case 57: { int r = va_arg(ap, int); int c = va_arg(ap, int); DeleteString((short)r, (short)c); } break;
        case 58: Clear(); bagCols = 0; break;   /* S125: our Clear() drops the bag columns
                    (private colwidths[] stays 0) and the caller rebuilds its own —
                    resume the pre-bag Shrink()-at-draw behaviour for those */
        case 59: { long w = va_arg(ap, long); AddColumn(w); } break;
        case 60: { int i = va_arg(ap, int); long w = va_arg(ap, long); SetColumnWidth((short)i, w); } break;
        case 61: { long p = va_arg(ap, long); AddPlayerNum(p); } break;
        case 62: { int r = va_arg(ap, int); long v = DeletePlayerNum((short)r);   if (pvRet) *(long*)pvRet = v; } break;
        case 63: { long p = va_arg(ap, long); int r = va_arg(ap, int); long v = ReplacePlayerNum(p,(short)r); if (pvRet) *(long*)pvRet = v; } break;
        case 64: { const char* t = va_arg(ap, const char*); int r = va_arg(ap, int); int c = va_arg(ap, int); ReplaceString(t ? t : "", (short)r, (short)c); } break;
        case 65: { int r = va_arg(ap, int); int c = va_arg(ap, int); long v = GetString((short)r,(short)c); if (pvRet) *(long*)pvRet = v; } break;
        case 66: { int r = va_arg(ap, int); long v = GetPlayerNum((short)r);       if (pvRet) *(long*)pvRet = v; } break;
        case 67: { long y = va_arg(ap, long); short v = GetRowFromY(y);            if (pvRet) *(short*)pvRet = v; } break;
        case 68: UpdateScrollBar(); break;
        case 69: { long v = GetListHeight();                   if (pvRet) *(long*)pvRet = v; } break;
        case 70: ResizeToFit(); break;
        case 71: { long c = va_arg(ap, long); long v = GetColumnWidth(c);          if (pvRet) *(long*)pvRet = v; } break;
        case 72: { long n = va_arg(ap, long); SetNumberOfRows(n); } break;
        case 73: { long r = va_arg(ap, long); InsertRow(r); } break;
        case 74: { long r = va_arg(ap, long); DeleteRow(r); } break;
        case 75: Shrink(); break;
        case 76: { BOOL v = SelectRecentlyFired();             if (pvRet) *(BOOL*)pvRet = v; } break;
        case 77: { long w = va_arg(ap, long); AddIconColumn(w); } break;
        case 78: { long f = va_arg(ap, long); int i = va_arg(ap, int); AddIcon(f,(short)i); } break;
        case 79: { int c = va_arg(ap, int); SetHorizontalOption((short)c); } break;
        case 80: { long x = va_arg(ap, long); short v = GetColFromX(x);            if (pvRet) *(short*)pvRet = v; } break;
        case 81: { long r = va_arg(ap, long); long c = va_arg(ap, long); long v = GetRowColPlayerNum(r,c); if (pvRet) *(long*)pvRet = v; } break;
        case 82: { long i = va_arg(ap, long); int b = va_arg(ap, int); SetColumnRightAligned(i,(BOOL)b); } break;
        case 83: { long r = va_arg(ap, long); long c = va_arg(ap, long); SetRowColour(r,c); } break;
        case 84: { long f = va_arg(ap, long); int r = va_arg(ap, int); int c = va_arg(ap, int); SetIcon(f,(short)r,(short)c); } break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RListBox: unhandled method dispid %ld\n", (long)id); break;
        }
    }
    void setprop(DISPID id, va_list ap) override {
        long v = va_arg(ap, long);
        switch (id) {
        case 1:  SetIsStripey((BOOL)v); break;
        case 2:  SetStripeColor((OLE_COLOR)v); break;
        case 3:  SetSelectColor((OLE_COLOR)v); break;
        case 4:  SetLines((BOOL)v); break;
        case 5:  SetLineColor((OLE_COLOR)v); break;
        case 6:  SetDarkStripeColor((OLE_COLOR)v); break;
        case 7:  SetDarkBackColor((OLE_COLOR)v); break;
        case 8:  SetLockLeftColumn((BOOL)v); break;
        case 9:  SetLockTopRow((BOOL)v); break;
        case 10: SetLockColor((OLE_COLOR)v); break;
        case 11: SetDragAndDrop((BOOL)v); break;
        case 12: SetFontNum(v); break;
        case 13: SetBlackboard((BOOL)v); break;
        case 14: SetFontNum2(v); break;
        case 15: SetLines2((BOOL)v); break;
        case 16: SetHeaderColor((OLE_COLOR)v); break;
        case 17: SetSelectWholeRows((BOOL)v); break;
        case 18: SetFontPtr(v); break;
        case 19: SetParentPointer(v); break;
        case 20: SetHilightRow(v); break;
        case 21: SetHilightCol(v); break;
        case 22: SetBorder((BOOL)v); break;
        case 23: SetCentred((BOOL)v); break;
        case 24: SetHorzSeperation(v); break;
        case 25: SetVertSeperation(v); break;
        case 26: SetToggleResizableColumns((BOOL)v); break;
        case 27: SetScrlBarOffset((short)v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        case DISPID_BACKCOLOR_: SetBackColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RListBox: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1:  *(BOOL*)pvRet = GetIsStripey(); break;
        case 12: *(long*)pvRet = GetFontNum(); break;
        case 14: *(long*)pvRet = GetFontNum2(); break;
        case 18: *(long*)pvRet = GetFontPtr(); break;
        case 19: *(long*)pvRet = GetParentPointer(); break;
        case 20: *(long*)pvRet = GetHilightRow(); break;
        case 21: *(long*)pvRet = GetHilightCol(); break;
        case 22: *(BOOL*)pvRet = GetBorder(); break;
        case DISPID_FORECOLOR_: *(OLE_COLOR*)pvRet = GetForeColor(); break;
        case DISPID_BACKCOLOR_: *(OLE_COLOR*)pvRet = GetBackColor(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RListBox: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rlistbox(CWnd* parent) { HostRListBox* h = new HostRListBox(); h->boot(parent); return h; }
