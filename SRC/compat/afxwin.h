/* FreeFalcon Linux Port - afxwin.h: minimal MFC compatibility layer.
 *
 * BoB uses MFC for its app/UI (CWinApp, CWnd, CDialog, CView, CFrameWnd, CDC...).
 * Neither bob's nor FreeFalcon's original afxwin.h has an implementation, so this
 * is a from-scratch minimal MFC: enough of the class hierarchy, message-map
 * macros and GDI/value types to COMPILE the MFC sources. Behaviour is stubbed
 * (windows never really open); the real UI is to be backed by SDL/GL at runtime.
 *
 * NOTE: CString is bob's OWN class (SRC/H/cstring.h), not provided here.
 */
#ifndef FF_COMPAT_AFXWIN_H
#define FF_COMPAT_AFXWIN_H
#ifdef FF_LINUX

/* bob headers gate on this (e.g. MIG.h: "#ifndef __AFXWIN_H__ #error ...") */
#define __AFXWIN_H__
#define __AFX_H__
#define _AFXWIN_H_

#include "windows.h"
#include "objbase.h"
/* CString is the game's own class (SRC/H/cstring.h). afxwin.h uses it by value
   in several signatures (CWinApp::GetProfileString, CWnd::GetWindowText, ...), so
   it must be complete here. cstring.h only defines the class under #ifndef __AFX_H__
   (it defers to real MFC otherwise) and afxwin.h defines __AFX_H__ above -- so we
   temporarily undef it across the include. windows.h (above) has already provided
   the TCHAR/BOOL/etc. types it needs. Idempotent for TUs that included it earlier. */
#ifdef __AFX_H__
#undef __AFX_H__
#define BOB_REDEFINE_AFX_H
#endif
#include "cstring.h"
#ifdef BOB_REDEFINE_AFX_H
#define __AFX_H__
#undef BOB_REDEFINE_AFX_H
#endif

/* MFC collection cursor */
#ifndef __AFX_POSITION_DEFINED
#define __AFX_POSITION_DEFINED
struct __POSITION {};
typedef __POSITION* POSITION;
#endif
struct CCreateContext;   /* used by CView/CFrameWnd create paths (opaque) */
/* forward decls (classes reference each other before their definitions) */
class CDC; class CFont; class CDocument; class CView; class CWnd; class CArchive;
class CScrollBar; class CBitmap; class CMenu; class CCommandLineInfo;
extern "C" void bob_gdi_screen_size(int*, int*);   /* live SDL window size (bob_video.cpp) */
extern "C" int  bob_gdi_text(int x, int y, const char* str, int pixelH, unsigned color); /* bob_gdi_font.cpp */
extern "C" int  bob_gdi_text_width(const char* str, int pixelH);
extern "C" void bob_gdi_line(int x0, int y0, int x1, int y1, unsigned color);
extern int g_bobListFontH;   /* live R* list font pixel height (bob_ole.cpp); shared so the control's
                                Shrink/GetTextExtent and OnDraw/ExtTextOut agree on size */
extern int g_bobDlgIDD;      /* IDD of the dialog currently being created (CDialog::Create), so each
                                hosted control knows its dialog -> (dialog,control) DLGINIT caption */
void bob_ole_host_template_statics(class CWnd* dlg, int dlgId);  /* S124: create the template's
                                non-DDX label statics (PE DIALOG data) — bob_ole.cpp */
class CDataExchange; class CPrintInfo; class CCreateContext_;
struct AFX_CMDHANDLERINFO; class CPropExchange; class CFile; class CWinApp;
struct tagHELPINFO; struct COleControlSite;

/* ============================================================
 * Message-map / runtime-class macros — all no-ops. BoB's handlers are wired by
 * these on Windows; on Linux input/events are driven by SDL, so we drop them.
 * ============================================================ */
#define DECLARE_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(theClass, baseClass)
#define END_MESSAGE_MAP()
#define DECLARE_DYNAMIC(class_name)   public: virtual class CRuntimeClass* GetRuntimeClass() const { return 0; }
#define IMPLEMENT_DYNAMIC(class_name, base_class)
/* DECLARE_DYNCREATE declares the CreateObject factory bob hand-defines/registers */
#define DECLARE_DYNCREATE(class_name) public: static CObject* CreateObject(); virtual class CRuntimeClass* GetRuntimeClass() const { return 0; }
#define IMPLEMENT_DYNCREATE(class_name, base_class)
#define IMPLEMENT_RUNTIMECLASS(class_name, base_class, wSchema, pfnNew)
#define DECLARE_RUNTIMECLASS(class_name)
#define DECLARE_SERIAL(class_name)
#define IMPLEMENT_SERIAL(class_name, base_class, quan)
#define DECLARE_OLECREATE(class_name)
#define IMPLEMENT_OLECREATE(class_name, ext, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define DECLARE_DISPATCH_MAP()
#define BEGIN_DISPATCH_MAP(theClass, baseClass)
#define END_DISPATCH_MAP()
/* Linux/GCC port (S33): real OCX-event routing (bob_eventsink.cpp) — adopted from the MiG Alley
   port's general ma_eventsink, retiring BoB's two targeted bridges (R5.3b SController combo +
   R4.4 CLoad file-row). The eventsink map becomes a per-class member MaRegEvents() (so it can take
   the addresses of the PROTECTED afx_msg handlers); a file-scope registrar auto-calls it at
   static-init, registering {dialog-CLASS, control-id, event-dispid} -> thunk. bob_evt_fire matches
   by the dialog's RUNTIME type (typeid — the many dialogs reuse the same IDC_ ids). bob_evt_call
   adapts to each handler signature via overload resolution on the member-fn-ptr type. */
#include <typeinfo>
extern "C" void bob_evt_register(const void* tinfo, int id, int dispid, void (*thunk)(void*));
extern "C" int  bob_evt_fire(void* dlg, const void* tinfo, int id, int dispid);
extern "C" { extern long bob_evtA0, bob_evtA1; extern void* bob_evtP; }
template<class C> inline void bob_evt_call(C* c, void (C::*f)())          { (c->*f)(); }
template<class C> inline void bob_evt_call(C* c, void (C::*f)(int))       { (c->*f)((int)bob_evtA0); }
template<class C> inline void bob_evt_call(C* c, void (C::*f)(long))      { (c->*f)((long)bob_evtA0); }
template<class C> inline void bob_evt_call(C* c, void (C::*f)(short))     { (c->*f)((short)bob_evtA0); }
template<class C> inline void bob_evt_call(C* c, void (C::*f)(int,int))   { (c->*f)((int)bob_evtA0,(int)bob_evtA1); }
template<class C> inline void bob_evt_call(C* c, void (C::*f)(long,long)) { (c->*f)((long)bob_evtA0,(long)bob_evtA1); }
template<class C> inline void bob_evt_call(C* c, void (C::*f)(LPCSTR))    { (c->*f)((LPCSTR)bob_evtP); }
/* BoB delta over MA: the SController combo handlers are OnTextChanged*(LPCTSTR, short) — the args
   are unused stubs (the handler reads the combo's new GetIndex), so empty text + 0 is faithful. */
template<class C> inline void bob_evt_call(C* c, void (C::*f)(LPCTSTR,short)) { (c->*f)((LPCTSTR)(bob_evtP?bob_evtP:""),(short)bob_evtA0); }
template<class C, class M> inline void bob_evt_call(C*, M) {}   /* fallback: uncovered signature compiles, doesn't fire */
#define BOB_EVT_CAT2(a,b) a##b
#define BOB_EVT_CAT(a,b) BOB_EVT_CAT2(a,b)
#define DECLARE_EVENTSINK_MAP() public: static void MaRegEvents();
/* The file-scope auto-registrar's name must be unique within the translation unit. BoB's unity
   builds #include several .cpp into one TU, so __LINE__ collides (two files' BEGIN at the same
   line) — use __COUNTER__ (TU-unique), captured ONCE via the _IMPL indirection so the 4 textual
   uses share one value rather than incrementing four times. */
#define BEGIN_EVENTSINK_MAP(theClass, baseClass) BOB_EVTSINK_IMPL(theClass, __COUNTER__)
#define BOB_EVTSINK_IMPL(theClass, ctr) \
    static struct BOB_EVT_CAT(BobEvtAuto_,ctr) { BOB_EVT_CAT(BobEvtAuto_,ctr)(); } BOB_EVT_CAT(g_bobEvtAuto_,ctr); \
    BOB_EVT_CAT(BobEvtAuto_,ctr)::BOB_EVT_CAT(BobEvtAuto_,ctr)() { theClass::MaRegEvents(); } \
    void theClass::MaRegEvents() {
#define END_EVENTSINK_MAP() }
#define DECLARE_EVENT_MAP()
#define BEGIN_EVENT_MAP(theClass, baseClass)
#define END_EVENT_MAP()
#define EVENT_CUSTOM(name, fn, vts)
#define EVENT_CUSTOM_ID(name, dispid, fn, vts)
#define EVENT_STOCK_CLICK()
#define EVENT_STOCK_DBLCLICK()
#define ON_EVENT(theClass, id, dispid, fn, vts) \
    { struct BOB_EVT_CAT(BobT_,__LINE__) { static void thunk(void* d){ bob_evt_call((theClass*)d, &theClass::fn); } }; \
      bob_evt_register(&typeid(theClass), (int)(id), (int)(dispid), &BOB_EVT_CAT(BobT_,__LINE__)::thunk); }
#define ON_EVENT_RANGE(theClass, idFirst, idLast, dispid, fn, vts)
#ifndef CN_EVENT
#define CN_EVENT  0x0800   /* control-notification: OLE control event */
#endif
#define ON_EVENT_REFLECT(theClass, dispid, fn, vts)
#define ON_PROPNOTIFY(theClass, id, dispid, fn)
#define DISP_FUNCTION(theClass, name, fn, vtret, vtargs)
#define DISP_PROPERTY(theClass, name, memb, vt)
#define DISP_PROPERTY_EX(theClass, name, getFn, setFn, vt)
#define DISP_PROPERTY_NOTIFY(theClass, name, memb, notifyFn, vt)
#define DISP_PROPERTY_PARAM(theClass, name, getFn, setFn, vt, vtparams)
#define DISP_DEFVALUE(theClass, name)
#define DISP_STOCKPROP_CAPTION()
#define DISP_STOCKPROP_TEXT()
#define DISP_STOCKPROP_BACKCOLOR()
#define DISP_STOCKPROP_FORECOLOR()
#define DISP_STOCKPROP_FONT()
#define DISP_STOCKPROP_ENABLED()
#define DISP_STOCKPROP_HWND()
#define DISP_STOCKFUNC_REFRESH()
#define DISP_STOCKFUNC_DOCLICK()
/* VTS_* are string literals (MFC packs them into a BYTE[] param-type list via
   literal concatenation), NOT NULL — using NULL breaks `BYTE p[] = VTS_x VTS_y`. */
#define VTS_NONE      ""
#define VTS_I2        "\x02"
#define VTS_I4        "\x03"
#define VTS_R4        "\x04"
#define VTS_R8        "\x05"
#define VTS_CY        "\x06"
#define VTS_DATE      "\x07"
#define VTS_BSTR      "\x08"
#define VTS_DISPATCH  "\x09"
#define VTS_SCODE     "\x0A"
#define VTS_BOOL      "\x0B"
#define VTS_VARIANT   "\x0C"
#define VTS_UNKNOWN   "\x0D"
#define VTS_UI1       "\x11"
#define VTS_COLOR     "\x03"
#define VTS_XPOS_PIXELS "\x03"
#define VTS_YPOS_PIXELS "\x03"
#define VTS_PI2       "\x42"
#define VTS_PI4       "\x43"
#define VTS_PR4       "\x44"
#define VTS_PR8       "\x45"
#define VTS_PBOOL     "\x4B"
#define VTS_PVARIANT  "\x4C"
#define VTS_PUI1      "\x51"
#define VTS_WBSTR     "\x08"
/* OLE control event firing (COleControl) — no-ops */
#define EVENT_PARAM(...)
#define FireEvent(...)        ((void)0)
/* OLE ActiveX-control factory / property-page / typelib macros.
   BEGIN_OLEFACTORY declares the nested class##Factory (COleObjectFactoryEx
   subclass) so the control's out-of-line UpdateRegistry/VerifyUserLicense/
   GetLicenseKey definitions compile. The registration path is dead on Linux
   (controls are instantiated via bob_ole hosting, not COM), but must build. */
#define BEGIN_OLEFACTORY(class_name) \
public: \
    class class_name##Factory : public COleObjectFactoryEx { \
    public: \
        BOOL UpdateRegistry(BOOL bRegister);
#define END_OLEFACTORY(class_name) };
#define DECLARE_OLECREATE_EX(class_name)
#define IMPLEMENT_OLECREATE_EX(class_name, ext, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define DECLARE_OLECTLTYPE(class_name)
#define IMPLEMENT_OLECTLTYPE(class_name, idsUserType, idBmp)
#define DECLARE_OLETYPELIB(class_name)
#define IMPLEMENT_OLETYPELIB(class_name, tlid, wVerMajor, wVerMinor)
#define DECLARE_PROPPAGEIDS(class_name)
#define BEGIN_PROPPAGEIDS(class_name, count)
#define END_PROPPAGEIDS(class_name)
#define PROPPAGEID(clsid)
#define DECLARE_OLEMISC_STATUS(status)
#define BEGIN_CONNECTION_MAP(theClass, theBase)
#define END_CONNECTION_MAP()
#define CONNECTION_IID(iid)
#define CONNECTION_PART(theClass, iid, localClass)
#define BEGIN_PROPERTY_MAP(theClass)
#define END_PROPERTY_MAP()
#define DECLARE_INTERFACE_MAP()
#define BEGIN_INTERFACE_MAP(theClass, baseClass)
#define END_INTERFACE_MAP()
#define afx_msg
#define RUNTIME_CLASS(class_name) (NULL)

/* MFC diagnostic macros — no-ops (NDEBUG-style) */
#ifndef ASSERT
#define ASSERT(f)         ((void)0)
#endif
#define ASSERT_VALID(p)   ((void)0)
#define ASSERT_KINDOF(class_name, p) ((void)0)
#define VERIFY(f)         ((void)(f))
#define TRACE             (void)
#define TRACE0(s)         ((void)0)
#define TRACE1(s,a)       ((void)0)
#define TRACE2(s,a,b)     ((void)0)
#define TRACE3(s,a,b,c)   ((void)0)
#define TRACEN(s)         ((void)0)
#define DEBUG_ONLY(f)     ((void)0)

/* ON_* message-map entries (only valid inside BEGIN/END_MESSAGE_MAP, which are
   empty, but define them so stray expansions vanish too). */
#define ON_COMMAND(id, memberFxn)
#define ON_COMMAND_RANGE(id1, id2, memberFxn)
#define ON_UPDATE_COMMAND_UI(id, memberFxn)
#define ON_CONTROL(code, id, memberFxn)
#define ON_MESSAGE(message, memberFxn)
#define ON_NOTIFY(code, id, memberFxn)
#define ON_BN_CLICKED(id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn)
#define ON_CBN_SELCHANGE(id, memberFxn)
#define ON_WM_CREATE()
#define ON_WM_DESTROY()
#define ON_WM_PAINT()
#define ON_WM_SIZE()
#define ON_WM_TIMER()
#define ON_WM_CLOSE()
#define ON_WM_KEYDOWN()
#define ON_WM_KEYUP()
#define ON_WM_CHAR()
#define ON_WM_CHARTOITEM()
#define ON_WM_CANCELMODE()
#define ON_WM_LBUTTONDOWN()
#define ON_WM_LBUTTONUP()
#define ON_WM_RBUTTONDOWN()
#define ON_WM_RBUTTONUP()
#define ON_WM_MOUSEMOVE()
#define ON_WM_ERASEBKGND()
#define ON_OLEVERB(idsVerbName, memberFxn)
#define ON_STDOLEVERB(idsVerbName, memberFxn)
#ifndef AFX_IDS_VERB_PROPERTIES
#define AFX_IDS_VERB_PROPERTIES 0xFD90
#endif
#define ON_WM_SETFOCUS()
#define ON_WM_KILLFOCUS()
#define ON_WM_ACTIVATE()
#define ON_WM_ACTIVATEAPP()
#define ON_WM_SYSCOMMAND()
#define ON_WM_INITMENUPOPUP()
#define ON_WM_HSCROLL()
#define ON_WM_VSCROLL()
#define ON_WM_MOVE()
#define ON_WM_SETCURSOR()
#define ON_WM_GETDLGCODE()   /* S125: CREditCtrl (REDIT) message map */
#define ON_WM_GETMINMAXINFO()
#define ON_WM_SHOWWINDOW()
#define ON_WM_ENABLE()
#define ON_WM_MOUSEWHEEL()
#define ON_WM_CONTEXTMENU()
#define ON_WM_DEVMODECHANGE()
#define ON_WM_INITMENU()
#define ON_WM_NCLBUTTONDOWN()
#define ON_WM_NCMOUSEMOVE()
#define ON_WM_LBUTTONDBLCLK()
#define ON_WM_RBUTTONDBLCLK()
#define ON_WM_SYSKEYDOWN()
#define ON_WM_SYSKEYUP()
#define ON_WM_CAPTURECHANGED()
#define ON_WM_MOUSEACTIVATE()
#define ON_WM_NCHITTEST()
#define ON_WM_QUERYNEWPALETTE()
#define ON_WM_PALETTECHANGED()
#define ON_WM_HELPINFO()
#define ON_REGISTERED_MESSAGE(nMessageVariable, memberFxn)
#define ON_UPDATE_COMMAND_UI_RANGE(id1, id2, memberFxn)
#define ON_LBN_SELCHANGE(id, memberFxn)
#define ON_LBN_DBLCLK(id, memberFxn)
#define ON_BN_DOUBLECLICKED(id, memberFxn)
#define ON_EN_KILLFOCUS(id, memberFxn)
#define ON_EN_SETFOCUS(id, memberFxn)
#define ON_CBN_EDITCHANGE(id, memberFxn)
#define ON_CBN_DROPDOWN(id, memberFxn)

/* ============================================================
 * Value types
 * ============================================================ */
struct CSize : public SIZE {
    CSize() { cx = cy = 0; }
    CSize(int initCX, int initCY) { cx = initCX; cy = initCY; }
    CSize(SIZE s) { cx = s.cx; cy = s.cy; }
};

struct CPoint : public POINT {
    CPoint() { x = y = 0; }
    CPoint(int initX, int initY) { x = initX; y = initY; }
    CPoint(POINT p) { x = p.x; y = p.y; }
    CPoint(SIZE s) { x = s.cx; y = s.cy; }
    CPoint(DWORD dw) { x = (short)LOWORD(dw); y = (short)HIWORD(dw); }
    void Offset(int dx, int dy) { x += dx; y += dy; }
    CPoint& operator=(SIZE s) { x = s.cx; y = s.cy; return *this; }
    CPoint& operator+=(POINT p) { x += p.x; y += p.y; return *this; }
    CPoint& operator-=(POINT p) { x -= p.x; y -= p.y; return *this; }
    CPoint& operator+=(SIZE s) { x += s.cx; y += s.cy; return *this; }
    CPoint& operator-=(SIZE s) { x -= s.cx; y -= s.cy; return *this; }
    CPoint operator+(SIZE s) const { return CPoint(x + s.cx, y + s.cy); }
    CPoint operator-(SIZE s) const { return CPoint(x - s.cx, y - s.cy); }
    CSize  operator-(POINT p) const { return CSize(x - p.x, y - p.y); }
    BOOL operator==(POINT p) const { return x == p.x && y == p.y; }
    BOOL operator!=(POINT p) const { return x != p.x || y != p.y; }
};

struct CRect : public RECT {
    CRect() { left = top = right = bottom = 0; }
    CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    CRect(RECT r) { left = r.left; top = r.top; right = r.right; bottom = r.bottom; }
    int Width()  const { return right - left; }
    int Height() const { return bottom - top; }
    CPoint TopLeft() const { return CPoint(left, top); }
    CPoint& TopLeft() { return *(CPoint*)this; }
    CPoint BottomRight() const { return CPoint(right, bottom); }
    CSize  Size()    const { return CSize(right - left, bottom - top); }
    void SetRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    void SetRectEmpty() { left = top = right = bottom = 0; }
    bool IsRectEmpty() const { return left == right || top == bottom; }
    bool PtInRect(POINT p) const { return p.x >= left && p.x < right && p.y >= top && p.y < bottom; }
    void OffsetRect(int dx, int dy) { left += dx; right += dx; top += dy; bottom += dy; }
    void InflateRect(int dx, int dy) { left -= dx; right += dx; top -= dy; bottom += dy; }
    CRect& operator+=(POINT p) { OffsetRect(p.x, p.y); return *this; }
    CRect& operator-=(POINT p) { OffsetRect(-p.x, -p.y); return *this; }
    CRect& operator+=(SIZE s) { InflateRect(s.cx, s.cy); return *this; }
    CRect& operator-=(SIZE s) { InflateRect(-s.cx, -s.cy); return *this; }
    CRect& operator=(const RECT& r) { left=r.left; top=r.top; right=r.right; bottom=r.bottom; return *this; }
    BOOL IntersectRect(LPCRECT, LPCRECT) { return FALSE; }
    BOOL UnionRect(LPCRECT, LPCRECT) { return FALSE; }
    void NormalizeRect() {}
    CPoint CenterPoint() const { return CPoint((left+right)/2, (top+bottom)/2); }
    operator LPRECT() { return this; }
    operator LPCRECT() const { return this; }
};

/* CPoint/CSize arithmetic (MFC global operators) */
inline CSize  operator-(POINT a, POINT b) { return CSize(a.x - b.x, a.y - b.y); }
inline CSize  operator+(SIZE a, POINT b)  { return CSize(a.cx + b.x, a.cy + b.y); }
inline CSize  operator+(SIZE a, SIZE b)   { return CSize(a.cx + b.cx, a.cy + b.cy); }
inline CSize  operator-(SIZE a, SIZE b)   { return CSize(a.cx - b.cx, a.cy - b.cy); }
inline CPoint operator+(POINT a, SIZE s)  { return CPoint(a.x + s.cx, a.y + s.cy); }
inline CPoint operator-(POINT a, SIZE s)  { return CPoint(a.x - s.cx, a.y - s.cy); }
inline CPoint operator+(POINT a, POINT b) { return CPoint(a.x + b.x, a.y + b.y); }
inline CRect  operator+(const RECT& r, POINT p) { return CRect(r.left+p.x, r.top+p.y, r.right+p.x, r.bottom+p.y); }
inline CRect  operator-(const RECT& r, POINT p) { return CRect(r.left-p.x, r.top-p.y, r.right-p.x, r.bottom-p.y); }
inline CRect  operator+(const RECT& r, SIZE s)  { return CRect(r.left+s.cx, r.top+s.cy, r.right+s.cx, r.bottom+s.cy); }

/* MFC standard dockbar / prompt resource IDs */
#ifndef AFX_IDW_DOCKBAR_TOP
#define AFX_IDW_DOCKBAR_TOP     0xE81B
#define AFX_IDW_DOCKBAR_BOTTOM  0xE81C
#define AFX_IDW_DOCKBAR_LEFT    0xE81D
#define AFX_IDW_DOCKBAR_RIGHT   0xE81E
#define AFX_IDW_TOOLBAR         0xE81B
#define AFX_IDW_STATUS_BAR      0xE801
#define AFX_IDP_FAILED_TO_LAUNCH_HELP 0xF010
#define AFX_IDP_COMMAND_FAILURE 0xF011
#endif

/* OLE stock-property dispids */
#ifndef DISPID_FORECOLOR
#define DISPID_FORECOLOR  (-501)
#define DISPID_BACKCOLOR  (-501)
#define DISPID_ENABLED    (-514)
#define DISPID_FONT       (-512)
#define DISPID_CAPTION    (-518)
#define DISPID_TEXT       (-517)
#define DISPID_VALUE      0
#endif

/* ============================================================
 * Object / command-target roots
 * ============================================================ */
class CObject {
public:
    CObject() {}
    virtual ~CObject() {}
    virtual void Serialize(class CArchive&) {}
    BOOL IsKindOf(const void*) const { return TRUE; }   /* RUNTIME_CLASS() is (void*)0 here */
};

class CCmdTarget : public CObject {
public:
    CCmdTarget() {}
};

/* ============================================================
 * GDI objects
 * ============================================================ */
/* R6.1: GDI blit subsystem (bob_gdi_blit.cpp) -- bitmap registry + DIB decode + ROP blit. */
extern "C" {
    void* bob_bmp_create(int w, int h);
    void  bob_bmp_free(void* h);
    void  bob_bmp_dims(void* h, int* w, int* ph);
    void* bob_dib_decode(const void* info, const void* bits);
    void  bob_blit(int dstScreen, void* dstBmp, int dvpx, int dvpy, int dx, int dy, int w, int h,
                   int srcScreen, void* srcBmp, int sx, int sy, unsigned long rop);
    void  bob_stretchblit(int dstScreen, void* dstBmp, int dvpx, int dvpy, int dx, int dy, int dwd, int dhd,
                          int srcScreen, void* srcBmp, int sx, int sy, int sws, int shs, unsigned long rop);
    void  bob_gdi_fillrect(int x, int y, int w, int h, unsigned long rgb);  /* R4.2 map backdrop */
    void  bob_map_paint_begin(void);
    void  bob_map_paint_end(void);
}

class CGdiObject : public CObject {
public:
    HGDIOBJ m_hObject;
    CGdiObject() : m_hObject(NULL) {}
    HGDIOBJ GetSafeHandle() const { return m_hObject; }
    BOOL DeleteObject() { m_hObject = NULL; return TRUE; }
    BOOL Attach(HGDIOBJ h) { m_hObject = h; return TRUE; }
    HGDIOBJ Detach() { HGDIOBJ h = m_hObject; m_hObject = NULL; return h; }
};

class CFont : public CGdiObject {
public:
    int m_height = 0;   /* pixel height (abs of CreateFont's lfHeight) so SelectObject
                           can drive the R* controls' GetTextMetrics/text at the real size */
    CFont() {}
    BOOL CreateFontIndirect(const LOGFONT* lf) {
        if (lf) { long h = ((const long*)lf)[0]; m_height = (int)(h < 0 ? -h : h); }
        return TRUE; }
    BOOL CreateFont(int h, int, int, int, int, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, LPCSTR) {
        m_height = h < 0 ? -h : h; return TRUE; }
    BOOL CreatePointFont(int, LPCSTR, CDC* = NULL);
    operator HFONT() const { return (HFONT)m_hObject; }
};

class CPen : public CGdiObject {
public:
    COLORREF m_color = 0; int m_penWidth = 1; bool m_hasColor = false;
    CPen() {}
    CPen(int, int w, COLORREF c) { m_color = c; m_penWidth = w; m_hasColor = true; }
    CPen(int, int, const void*, int = 0) {}   /* ExtCreatePen geometric form (LOGBRUSH*) */
    BOOL CreatePen(int, int w, COLORREF c) { m_color = c; m_penWidth = w; m_hasColor = true; return TRUE; }
    operator HPEN() const { return (HPEN)m_hObject; }
};

class CBrush : public CGdiObject {
public:
    CBrush() {}
    CBrush(COLORREF) {}
    BOOL CreateSolidBrush(COLORREF) { return TRUE; }
    BOOL CreateStockObject(int) { return TRUE; }
    static CBrush* FromHandle(HBRUSH) { return NULL; }
    operator HBRUSH() const { return (HBRUSH)m_hObject; }
};

class CBitmap : public CGdiObject {
public:
    /* R6.1: real pixel-backed bitmaps. m_hObject holds the bob_gdi_blit registry handle. */
    BOOL CreateCompatibleBitmap(CDC*, int w, int h) { m_hObject = (HGDIOBJ)bob_bmp_create(w, h); return m_hObject != NULL; }
    BOOL CreateBitmap(int w, int h, UINT, UINT, const void*) { m_hObject = (HGDIOBJ)bob_bmp_create(w, h); return m_hObject != NULL; }
    BOOL LoadBitmapA(LPCSTR) { return FALSE; }
    BOOL LoadBitmapA(UINT) { return FALSE; }
    BOOL DeleteObject() { if (m_hObject) bob_bmp_free((void*)m_hObject); m_hObject = NULL; return TRUE; }
    /* FromHandle wraps an HBITMAP in a CBitmap (MFC returns a temporary). LoadInstances does
       imagemapinstances[0].SelectObject(CBitmap::FromHandle(map)) -- the wrapper is read
       immediately by SelectObject, so a small rotating pool of wrappers is sufficient. */
    static CBitmap* FromHandle(HBITMAP hb) {
        static CBitmap pool[8]; static int n = 0;
        CBitmap* b = &pool[n++ & 7]; b->m_hObject = (HGDIOBJ)hb; return b;
    }
    int GetBitmap(void* p) { if (p && m_hObject) { int w, h; bob_bmp_dims((void*)m_hObject, &w, &h);
            /* BITMAP: LONG bmType, bmWidth, bmHeight, bmWidthBytes; WORD bmPlanes, bmBitsPixel; LPVOID bmBits */
            long* L = (long*)p; L[0]=0; L[1]=w; L[2]=h; L[3]=w*4; return 1; } return 0; }
    operator HBITMAP() const { return (HBITMAP)m_hObject; }
};

class CDC : public CObject {
public:
    HDC m_hDC;
    /* bob_gdi backing for driving the R* controls' real OnDraw text onto the
       front-end framebuffer. m_bobScreen gates *drawing* (only the CDC we hand to
       OnDraw paints; other stub CDCs stay no-op). Measuring is always live. */
    COLORREF m_textColor = 0;
    int  m_bobVpX = 0, m_bobVpY = 0;   /* viewport origin -> the control's screen pos */
    int  m_bobTextH = 18;              /* pixel height for bob_gdi_text/measure (== row pitch) */
    bool m_bobScreen = false;
    /* R6.1: blit support. m_bobBmp = the bitmap handle SelectObject'd into a memory DC (the
       blit source/target); m_bobMemDC = a CreateCompatibleDC offscreen DC. */
    void* m_bobBmp = NULL;
    bool  m_bobMemDC = false;
    static unsigned bobColor(COLORREF c) { /* COLORREF 0x00BBGGRR -> bob 0xRRGGBB */
        return ((unsigned)(c & 0xff) << 16) | (unsigned)(c & 0xff00) | ((unsigned)(c >> 16) & 0xff); }
    CDC() : m_hDC(NULL) {}
    HDC GetSafeHdc() const { return m_hDC; }
    operator HDC() const { return m_hDC; }
    BOOL Attach(HDC h) { m_hDC = h; return TRUE; }
    HDC Detach() { HDC h = m_hDC; m_hDC = NULL; return h; }
    CGdiObject* SelectObject(CGdiObject*) { return NULL; }
    HGDIOBJ SelectStockObject(int) { return NULL; }
    CFont* m_bobCurFont = NULL;
    CFont* SelectObject(CFont* f) {
        /* NOTE: we deliberately do NOT adopt f->m_height here. The front-end panels
           are drawn scaled-up (template DLU rects x resolution), but the game's fonts
           are sized for native DLU, so the real font is tiny in our enlarged boxes
           (it shrank both combos and the tab bar). Text stays sized to the template
           box height (m_bobTextH) for coherent scaling. Adopting f->m_height needs a
           matching DPI/scale pass on the layout -- deferred. */
        CFont* old = m_bobCurFont; m_bobCurFont = f;
        return old; }
    /* S65: do NOT cache the caller's CPen*. The map route plotting
       (CMIGView::Plot*Route) selects a STACK-local CPen then later restores the
       returned "old" pen; that old pen was a stack temporary from a prior frame, so
       caching/dereferencing it on restore was a stack-use-after-return (ASan, on the
       post-mission map redraw). Keep only the pen COLOUR by value on a small LIFO
       stack and hand back a fixed sentinel the caller passes back to restore -- on
       restore we pop the saved colour and never dereference a (possibly dead) pen. */
    bool m_penHasColor = false;
    COLORREF m_penStk[16]; bool m_penHasStk[16]; int m_penSP = 0;
    CPen*  SelectObject(CPen* p)  {
        static CPen s_restoreTok;                       /* its address is the restore marker */
        if (p == &s_restoreTok) {                       /* restore: pop the saved colour */
            if (m_penSP > 0) { --m_penSP; m_penColor = m_penStk[m_penSP]; m_penHasColor = m_penHasStk[m_penSP]; }
            return NULL;
        }
        if (m_penSP < 16) { m_penStk[m_penSP] = m_penColor; m_penHasStk[m_penSP] = m_penHasColor; ++m_penSP; }
        if (p && p->m_hasColor) { m_penColor = p->m_color; m_penHasColor = true; }
        return &s_restoreTok;                           /* non-null; caller passes back to restore */
    }
    CPen*  SelectObject(CPen& p)  { return SelectObject(&p); }
    CBrush* SelectObject(CBrush*) { return NULL; }
    COLORREF SetTextColor(COLORREF c) { COLORREF o = m_textColor; m_textColor = c; return o; }
    COLORREF SetBkColor(COLORREF c) { return c; }
    int SetBkMode(int) { return 0; }
    BOOL TextOutA(int x, int y, LPCSTR s, int n) { return ExtTextOutA(x, y, 0, NULL, s, (UINT)n, NULL); }
    BOOL ExtTextOutA(int x, int y, UINT, LPCRECT, LPCSTR s, UINT n, LPINT) {
        if (m_bobScreen && s && n) {
            char buf[512]; UINT k = n < 511 ? n : 511; memcpy(buf, s, k); buf[k] = 0;
            bob_gdi_text(m_bobVpX + x, m_bobVpY + y, buf, m_bobTextH, bobColor(m_textColor));
        }
        return TRUE;
    }
    BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, LPCSTR s, UINT n, LPINT d) { return ExtTextOutA(x,y,o,r,s,n,d); }
    /* CString-accepting overloads (template triggers CString::operator LPCTSTR) */
    template<class S> BOOL TextOut(int x, int y, const S& s) { LPCSTR p=(LPCSTR)s; return TextOutA(x,y,p,(int)strlen(p)); }
    template<class S> BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, const S& s, UINT n, LPINT d) { return ExtTextOutA(x,y,o,r,(LPCSTR)s,n,d); }
    template<class S> BOOL ExtTextOut(int x, int y, UINT o, LPCRECT r, const S& s, LPINT d) { LPCSTR p=(LPCSTR)s; return ExtTextOutA(x,y,o,r,p,(UINT)strlen(p),d); }
    template<class S> int  DrawText(const S& s, LPRECT r, UINT f) { LPCSTR p=(LPCSTR)s; return DrawText(p,(int)strlen(p),r,f); }
    COLORREF SetPixel(int, int, COLORREF c) { return c; }
    COLORREF GetPixel(int, int) const { return 0; }
    BOOL Polygon(LPPOINT, int) { return TRUE; }
    BOOL Polyline(LPPOINT, int) { return TRUE; }
    BOOL Ellipse(int, int, int, int) { return TRUE; }
    BOOL Ellipse(LPCRECT) { return TRUE; }
    BOOL RoundRect(int, int, int, int, int, int) { return TRUE; }
    int  GetClipBox(LPRECT) const { return 0; }
    BOOL PtVisible(int, int) const { return TRUE; }
    BOOL RectVisible(LPCRECT) const { return TRUE; }
    UINT GetTextAlign() const { return 0; }
    int  GetTextFace(int, LPSTR) const { return 0; }
    /* Fill a TEXTMETRIC with sane defaults so the R* controls' row/scroll maths
       (height = count*tm.tmHeight, etc.) don't run on uninitialised stack. */
    BOOL GetTextMetricsA(void* p) const {
        if (p) { TEXTMETRIC* tm = (TEXTMETRIC*)p; memset(tm, 0, sizeof(*tm));
            int H = m_bobTextH > 0 ? m_bobTextH : 16;   /* row pitch == bob_gdi_text height */
            tm->tmHeight = H; tm->tmAscent = H*13/16; tm->tmDescent = H - H*13/16;
            tm->tmAveCharWidth = H/2; tm->tmMaxCharWidth = H; }
        return TRUE;
    }
    CFont* GetCurrentFont() const { static CFont s_f; return &s_f; }
    BOOL Rectangle(int, int, int, int) { return TRUE; }
    /* line drawing for the R* controls' borders (3D bevels). Coords are control-
       relative; offset by the viewport origin and draw into the framebuffer. */
    int m_penX = 0, m_penY = 0; COLORREF m_penColor = 0;
    POINT MoveTo(int x, int y) { POINT p={m_penX,m_penY}; m_penX=x; m_penY=y; return p; }
    POINT MoveTo(POINT pt) { return MoveTo(pt.x, pt.y); }
    BOOL LineTo(int x, int y) {
        if (m_bobScreen) bob_gdi_line(m_bobVpX+m_penX, m_bobVpY+m_penY, m_bobVpX+x, m_bobVpY+y, CDC::bobColor(m_penColor));
        m_penX=x; m_penY=y; return TRUE; }
    BOOL LineTo(POINT pt) { return LineTo(pt.x, pt.y); }
    /* R6.1: real blits. A screen DC (m_bobScreen) targets the bob_gdi framebuffer at its
       viewport origin; a memory DC targets its selected bitmap (m_bobBmp). */
    CBitmap* SelectObject(CBitmap* b) {
        void* old = m_bobBmp; m_bobBmp = b ? (void*)b->m_hObject : NULL;
        return CBitmap::FromHandle((HBITMAP)old);
    }
    BOOL BitBlt(int dx, int dy, int w, int h, CDC* src, int sx, int sy, DWORD rop) {
        if (!src) { /* pattern blits (BLACKNESS/WHITENESS) have no source */
            bob_blit(m_bobScreen?1:0, m_bobBmp, m_bobVpX, m_bobVpY, dx, dy, w, h, 0, NULL, 0, 0, rop);
            return TRUE; }
        bob_blit(m_bobScreen?1:0, m_bobBmp, m_bobVpX, m_bobVpY, dx, dy, w, h,
                 src->m_bobScreen?1:0, src->m_bobBmp, sx, sy, rop);
        return TRUE;
    }
    BOOL CreateCompatibleDC(CDC*) { m_bobMemDC = true; return TRUE; }
    int FillRect(LPCRECT, CBrush*) { return 0; }
    /* R4.2: solid-fill on a screen DC -> framebuffer (the strategic-map backdrop). */
    void FillSolidRect(LPCRECT r, COLORREF c) { if (m_bobScreen && r) bob_gdi_fillrect(
            m_bobVpX+r->left, m_bobVpY+r->top, r->right-r->left, r->bottom-r->top, bobColor(c)); }
    void FillSolidRect(int x, int y, int w, int h, COLORREF c) { if (m_bobScreen) bob_gdi_fillrect(
            m_bobVpX+x, m_bobVpY+y, w, h, bobColor(c)); }
    void Draw3dRect(LPCRECT, COLORREF, COLORREF) {}
    void Draw3dRect(int, int, int, int, COLORREF, COLORREF) {}
    BOOL DrawIcon(int, int, HICON) { return TRUE; }   /* R* close/tick/help button glyphs (no-op) */
    BOOL StretchBlt(int dx, int dy, int dw, int dh, CDC* src, int sx, int sy, int sw, int sh, DWORD rop) {
        if (!src) return FALSE;
        bob_stretchblit(m_bobScreen?1:0, m_bobBmp, m_bobVpX, m_bobVpY, dx, dy, dw, dh,
                        src->m_bobScreen?1:0, src->m_bobBmp, sx, sy, sw, sh, rop);
        return TRUE;
    }
    int  GetDeviceCaps(int) const { return 0; }
    COLORREF GetNearestColor(COLORREF c) const { return c; }
    CSize GetTextExtent(LPCSTR s, int len) const {
        if (!s || len <= 0) return CSize(0, m_bobTextH);
        char buf[512]; int k = len < 511 ? len : 511; memcpy(buf, s, k); buf[k] = 0;
        return CSize(bob_gdi_text_width(buf, m_bobTextH), m_bobTextH);
    }
    template<class S> CSize GetTextExtent(const S& s) const {
        LPCSTR p = (LPCSTR)s; return CSize(bob_gdi_text_width(p ? p : "", m_bobTextH), m_bobTextH);
    }
    CSize GetOutputTextExtent(LPCSTR s, int len) const { return GetTextExtent(s, len); }
    int  DrawText(LPCSTR s, int len, LPRECT r, UINT format) {
        /* S127: real DrawText for R* STATIC labels (CRStaticCtrl::OnDraw is the only
           caller — combos/buttons/listboxes draw via ExtTextOut). Honours
           DT_WORDBREAK multi-line word-wrap (long prose was drawing one clipped line
           running off the panel edge — the phase-select/QS descriptions; MA note 17
           shared find) and Windows '&' accelerator-prefix processing ("&&"->"&", a
           lone '&' removed — BDG's "Cockpit && UI" -> "Cockpit & UI", gold #8).
           BOB_NO_WORDWRAP / BOB_NO_AMP_ESCAPE revert each. */
        if (!s || !r) return m_bobTextH;
        char raw[512]; int n = len < 0 ? (int)strlen(s) : len; if (n < 0) n = 0; if (n > 511) n = 511;
        memcpy(raw, s, n); raw[n] = 0;

        static int noAmp = -1; if (noAmp < 0) noAmp = getenv("BOB_NO_AMP_ESCAPE") ? 1 : 0;
        char buf[512];
        if (!noAmp && !(format & DT_NOPREFIX)) {
            int j = 0;
            for (int i = 0; raw[i] && j < 511; i++) {
                if (raw[i] == '&') { if (raw[i+1] == '&') { buf[j++] = '&'; i++; } /* lone '&': prefix marker, dropped */ }
                else buf[j++] = raw[i];
            }
            buf[j] = 0;
        } else { memcpy(buf, raw, (size_t)n + 1); }

        int boxw = r->right - r->left;
        int pitch = (m_bobTextH > 0 ? m_bobTextH : 16) + 2;

        static int noWrap = -1; if (noWrap < 0) noWrap = getenv("BOB_NO_WORDWRAP") ? 1 : 0;
        /* Only wrap boxes tall enough to be a genuine multi-line area (>=2 lines):
           config LABELS also pass DT_WORDBREAK but sit in single-line boxes — keep
           their one-line render (our font may be wider than gold's, so wrapping a
           label that fits on Windows would spill into the row below). Descriptions
           (the tall PhaseDescription/QS statics) are the real wrap targets. */
        bool wrap = (format & DT_WORDBREAK) && !(format & DT_SINGLELINE) && !noWrap
                    && boxw > 8 && (r->bottom - r->top) >= 2 * pitch;

        /* helper: place one line honouring DT_CENTER/DT_RIGHT within [left,right] */
        if (!wrap) {
            int tw = bob_gdi_text_width(buf, m_bobTextH);
            int x = r->left;
            if (format & DT_CENTER) x = r->left + (boxw - tw) / 2;
            else if (format & DT_RIGHT) x = r->right - tw;
            if (!(format & DT_CALCRECT) && m_bobScreen)
                bob_gdi_text(m_bobVpX + x, m_bobVpY + r->top, buf, m_bobTextH, bobColor(m_textColor));
            return m_bobTextH;
        }

        /* greedy word-wrap; honour explicit '\n'. Clip vertically to the box for a
           genuine multi-line area; a single-line box (h < 2*pitch) draws its one line. */
        int y = r->top;
        int clipBottom = (r->bottom - r->top) >= 2 * pitch ? r->bottom : 0x7fffffff;
        char line[512]; int ll = 0;
        int i = 0;
        while (buf[i] || ll) {
            char c = buf[i];
            bool forceBreak = (c == '\n') || (c == 0);
            if (c == '\n') i++;
            if (!forceBreak) {
                /* accumulate one word (up to the next space/newline/end) */
                int ws = i; while (buf[i] && buf[i] != ' ' && buf[i] != '\n') i++;
                int wl = i - ws;
                /* build candidate line = current + optional space + word */
                char cand[512]; int cl = 0;
                memcpy(cand, line, ll); cl = ll;
                if (cl && cl < 511) cand[cl++] = ' ';
                for (int k = 0; k < wl && cl < 511; k++) cand[cl++] = buf[ws + k];
                cand[cl] = 0;
                if (ll == 0 || bob_gdi_text_width(cand, m_bobTextH) <= boxw) {
                    memcpy(line, cand, cl + 1); ll = cl;
                    while (buf[i] == ' ') i++;   /* swallow trailing spaces */
                    if (buf[i]) continue;        /* more text -> keep packing */
                    forceBreak = true;           /* end of string -> flush last line */
                } else {
                    /* word doesn't fit: flush current line, retry word on a fresh line */
                    forceBreak = true;
                    i = ws;                      /* re-process this word next iteration */
                }
            }
            if (forceBreak) {
                if (y + m_bobTextH > clipBottom) break;
                line[ll] = 0;
                int tw = bob_gdi_text_width(line, m_bobTextH);
                int x = r->left;
                if (format & DT_CENTER) x = r->left + (boxw - tw) / 2;
                else if (format & DT_RIGHT) x = r->right - tw;
                if (!(format & DT_CALCRECT) && m_bobScreen && ll)
                    bob_gdi_text(m_bobVpX + x, m_bobVpY + y, line, m_bobTextH, bobColor(m_textColor));
                y += pitch; ll = 0;
                if (buf[i] == 0) break;
            }
        }
        if (format & DT_CALCRECT) r->bottom = y;
        return y - r->top;
    }
    UINT SetTextAlign(UINT) { return 0; }
    int  SetMapMode(int) { return 0; }
    int  SetROP2(int) { return 0; }
    int  SetStretchBltMode(int) { return 0; }
    int  GetStretchBltMode() const { return 0; }
    static CDC* FromHandle(HDC) { return NULL; }
    POINT SetViewportOrg(int, int) { POINT p = {0,0}; return p; }
};

inline BOOL CFont::CreatePointFont(int, LPCSTR, CDC*) { return TRUE; }

class CPaintDC : public CDC { public: CPaintDC(CWnd*) {} };
class CClientDC : public CDC { public: CClientDC(CWnd*) {} };
class CWindowDC : public CDC { public: CWindowDC(CWnd*) {} };
class CMetaFileDC : public CDC { public: CMetaFileDC() {} };

/* ============================================================
 * Window / app hierarchy (stubbed — no real windows on Linux)
 * ============================================================ */
class CListBox;
/* ---- OLE/ActiveX control hosting router (bob_ole.cpp). The R* CWnd wrappers
   forward AddString/Clear/properties here; the router owns the genuine COleControl
   instance (CRListBoxCtrl, ...) and dispatches by dispid. ---- */
extern "C" {
    BOOL bob_ole_create_control(CWnd* self, const GUID* clsid, CWnd* parent, UINT id);
    void* bob_dlg_getfile(int filenum);   /* WM_GETFILE: icon/art data (bob_ole_rcombo.cpp) */
    void* bob_dlg_getfont(int fontnum);   /* WM_GETGLOBALFONT: CFont* from g_AllFonts */
    int bob_load_string(void* h, unsigned id, char* buf, int maxlen);  /* WM_GETSTRING: BDG string table (bob_resources.cpp) */
    void bob_ole_invoke(CWnd* self, DISPID id, WORD flags, VARTYPE vtRet, void* pvRet, const BYTE* pInfo, va_list ap);
    void bob_ole_setprop(CWnd* self, DISPID id, VARTYPE vt, va_list ap);
    void bob_ole_getprop(CWnd* self, DISPID id, VARTYPE vt, void* pvRet);
    CWnd* bob_ole_find_wrapper(CWnd* dlg, int id);   /* (dialog,id) -> hosted control wrapper */
}

class CWnd : public CCmdTarget {
public:
    enum { adjustBorder = 0, adjustOutside = 1 };
    HWND m_hWnd;
    CWnd() : m_hWnd(NULL) {}
    /* COleControl host-site ptr + dialog help-id, used by CRToolBar/CDialog code */
    void* m_pCtrlSite = NULL;
    UINT  m_nIDHelp = 0;
    /* window text storage: RCombo's SetIndex does SetWindowText(item); the control's
       OnDraw shows InternalGetText() == this text. */
    CString m_bobText;
    BOOL IsWindowEnabled() const { return TRUE; }
    BOOL IsZoomed() const { return FALSE; }
    void WinHelp(DWORD, UINT = 0) {}
    /* CWnd virtual handlers the toolbar/dialog fragments forward to via Base:: */
    void OnInitMenu(CMenu*) {}
    void OnInitMenuPopup(CMenu*, UINT, BOOL) {}
    void OnSetFont(CFont*) {}
    void OnCancelMode() {}
    void OnFinalRelease() {}
    void PreSubclassWindow() {}
    BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*) { return FALSE; }
    int  OnCharToItem(UINT, CListBox*, UINT) { return -1; }
    BOOL OnAmbientProperty(void*, DISPID, void*) { return FALSE; }
    static CWnd* WindowFromPoint(CPoint) { return NULL; }
    void CalcWindowRect(LPRECT, UINT = adjustBorder) {}
    HWND GetSafeHwnd() const { return m_hWnd; }
    operator HWND() const { return m_hWnd; }
    BOOL Attach(HWND h) { m_hWnd = h; return TRUE; }
    HWND Detach() { HWND h = m_hWnd; m_hWnd = NULL; return h; }
    CWnd* GetDlgItem(int id) const { return bob_ole_find_wrapper((CWnd*)this, id); }
    /* Return a non-null sentinel so RDialog/RToolBar::DDX_Control proceed to the
       global ::DDX_Control (which creates+binds the hosted ActiveX control). */
    void  GetDlgItem(int id, HWND* ph) const { if (ph) *ph = (HWND)(intptr_t)(id ? id : 1); }
    int GetDlgItemTextA(int, LPSTR, int) { return 0; }
    void SetDlgItemTextA(int, LPCSTR) {}
    BOOL SetWindowTextA(LPCSTR s) { m_bobText = s ? s : ""; return TRUE; }
    int GetWindowTextA(LPSTR, int) { return 0; }
    template<class S> int GetWindowTextA(S& s) { (void)s; return 0; }
    /* SP.2 (S123): track runtime visibility for hosted OLE controls -- the game hides
       off-page/demo controls with ShowWindow(SW_HIDE) (SW_HIDE==0); the OLE panel draw
       skips hidden hosts. Non-hosted CWnds keep the old no-op semantics. */
    BOOL ShowWindow(int nCmdShow) {
        extern void bob_ole_show_window(CWnd*, int);
        bob_ole_show_window(this, nCmdShow);
        return TRUE;
    }
    BOOL UpdateWindow() { return TRUE; }
    BOOL DestroyWindow() { return TRUE; }
    BOOL MoveWindow(int, int, int, int, BOOL = TRUE) { return TRUE; }
    BOOL MoveWindow(LPCRECT, BOOL = TRUE) { return TRUE; }
    CWnd* GetTopWindow() const { return NULL; }
    static CWnd* GetDesktopWindow() { return NULL; }
    static CWnd* FromHandle(HWND) { return NULL; }
    CWnd* GetLastActivePopup() const { return NULL; }
    /* Front-end needs real window geometry (else it picks resolution 0 and sizes panels to
       nothing). Report the live SDL window size from the video backend (bob_gdi_screen_size). */
    void GetClientRect(LPRECT r) const { if (r) {
        int w=0,h=0; bob_gdi_screen_size(&w,&h); r->left=r->top=0; r->right=w; r->bottom=h; } }
    void GetWindowRect(LPRECT r) const { if (r) {
        int w=0,h=0; bob_gdi_screen_size(&w,&h); r->left=r->top=0; r->right=w; r->bottom=h; } }
    void ClientToScreen(LPPOINT) const {}
    void ClientToScreen(LPRECT) const {}
    void ScreenToClient(LPPOINT) const {}
    void ScreenToClient(LPRECT) const {}
    BOOL CreateControl(LPCSTR, LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
    BOOL CreateControl(REFCLSID clsid, LPCSTR, DWORD, const RECT&, CWnd* parent, UINT nID) {
        return bob_ole_create_control(this, &clsid, parent, nID); }
    BOOL CreateControl(REFCLSID clsid, LPCSTR, DWORD, const RECT&, CWnd* parent, UINT nID, CFile*, BOOL, BSTR) {
        return bob_ole_create_control(this, &clsid, parent, nID); }
    /* The ActiveX-control parent (set by the host at CreateControl) -- the control's
       OnDraw/ResizeToFit do GetParent()->SendMessage(WM_GET*)/GetClientRect. */
    CWnd* m_pBobParent = NULL;
    /* hosted-ActiveX-control accessors: route to the genuine COleControl via bob_ole. */
    void SetProperty(DISPID dispid, VARTYPE vt, ...) {
        va_list ap; va_start(ap, vt); bob_ole_setprop(this, dispid, vt, ap); va_end(ap); }
    void GetProperty(DISPID dispid, VARTYPE vt, void* pv) const {
        bob_ole_getprop((CWnd*)this, dispid, vt, pv); }
    void InvokeHelper(DISPID dispid, WORD wFlags, VARTYPE vtRet, void* pvRet, const BYTE* pbInfo, ...) {
        va_list ap; va_start(ap, pbInfo); bob_ole_invoke(this, dispid, wFlags, vtRet, pvRet, pbInfo, ap); va_end(ap); }
    /* virtual so ::DDX_Control can dispatch to the R* wrapper's Create() (which
       calls CreateControl(GetClsid(),...)); base does nothing for plain windows. */
    virtual BOOL Create(LPCTSTR, LPCTSTR, DWORD, const RECT&, CWnd*, UINT, CCreateContext* = NULL) { return FALSE; }
    CWnd* GetNextWindow(UINT = 0) const { return NULL; }
    CWnd* GetWindow(UINT) const { return NULL; }
    int   GetDlgCtrlID() const { return 0; }
    LONG  GetWindowLong(int) const { return 0; }
    LONG  SetWindowLong(int, LONG) { return 0; }
    DWORD GetStyle() const { return 0; }
    DWORD GetExStyle() const { return 0; }
    CScrollBar* GetScrollBarCtrl(int) const { return NULL; }
    void  ModifyStyle(DWORD, DWORD, UINT = 0) {}
#if BOB_LINUX
    /* Linux port: return a shared no-op CDC (not NULL) so callers that deref the
       returned DC -- e.g. IconDescUI::LoadInstances(*pdc) in InitInstance -- have
       a valid object. A real GDI backend replaces this. */
    CDC* GetDC() { static CDC s_stubDC; if(!s_stubDC.m_hDC) s_stubDC.m_hDC=(HDC)1;
        s_stubDC.m_bobTextH = g_bobListFontH;   /* so R* controls' Shrink/measure use the live list height */
        return &s_stubDC; }
#else
    CDC* GetDC() { return NULL; }
#endif
    int  ReleaseDC(CDC*) { return 1; }
    BOOL EnableWindow(BOOL = TRUE) { return TRUE; }
    CWnd* SetFocus() { return NULL; }
    int MessageBoxA(LPCSTR, LPCSTR = NULL, UINT = 0) { return 0; }
    LRESULT SendMessageA(UINT m, WPARAM w = 0, LPARAM l = 0) {
        /* The R* controls fetch icon/art via the parent's WM_GETFILE handler
           (OnGetFile). Route it to the compat impl so OnDraw's icon draw doesn't
           NULL-deref. WM_GETFILE == WM_USER+4 == 0x404. */
        if (m == 0x404) return (LRESULT)bob_dlg_getfile((int)w);   /* WM_GETFILE  = WM_USER+4 */
        if (m == 0x403) return (LRESULT)bob_dlg_getfont((int)w);   /* WM_GETGLOBALFONT = +3 */
        /* S126: WM_GETSTRING = WM_USER+16 — the genuine caption-resolution path.
           CRStaticCtrl::GetParentWndInfo sends it with the persisted ResourceNumber
           (now loaded from the property stream); every RDialog answers it with
           AfxLoadString(id, buffer, buffer[0]) (RDIALMSG.CPP OnGetString). Serve it
           from the language DLL's (BDG) string table. */
        if (m == 0x410 && l) {
            char* buf = (char*)l;
            int mx = (unsigned char)buf[0]; if (mx <= 0 || mx > 99) mx = 99;
            return (LRESULT)bob_load_string(NULL, (unsigned)w, buf, mx);
        }
        return 0;
    }
    LRESULT SendMessage(UINT m, WPARAM w = 0, LPARAM l = 0) { return SendMessageA(m, w, l); }
    BOOL PostMessageA(UINT m, WPARAM w = 0, LPARAM = 0) {
        /* R1.1b inc 4.3: View3d::CloseWindow posts WM_COMMAND(IDOK/IDCANCEL) to the flight
           dialog to end flight; compat has no message dispatch, so hand it to the close
           bridge (which only acts while flight is live). WM_COMMAND == 0x0111. */
        if (m == 0x0111) { extern void bob_capture_wm_command(unsigned int, unsigned int);
                           bob_capture_wm_command(m, (unsigned int)w); }
        return TRUE;
    }
    /* `PostMessage`/`GetWindowText`/... callers are macro-mapped to the A names */
    void ScrollWindow(int, int, LPCRECT = NULL, LPCRECT = NULL) {}
    class CMenu* GetMenu() const { return NULL; }
    BOOL SetMenu(class CMenu*) { return TRUE; }
    void SendMessageToDescendants(UINT, WPARAM = 0, LPARAM = 0, BOOL = TRUE, BOOL = TRUE) {}
    void DrawMenuBar() {}
    void DragAcceptFiles(BOOL = TRUE) {}
    void RedrawWindow(LPCRECT = NULL, HRGN = NULL, UINT = 0) {}
    LRESULT DefWindowProc(UINT, WPARAM, LPARAM) { return 0; }
    BOOL ModifyStyleEx(DWORD, DWORD, UINT = 0) { return TRUE; }
    CDC* BeginPaint(LPPAINTSTRUCT) { static CDC s_paintDC; if(!s_paintDC.m_hDC) s_paintDC.m_hDC=(HDC)1; return &s_paintDC; }
    void EndPaint(LPPAINTSTRUCT) {}
    void GetWindowPlacement(void*) const {}
    void SetWindowPlacement(const void*) {}
    int  RunModalLoop(DWORD = 0) { return 0; }
    void Invalidate(BOOL = TRUE) {}
    void InvalidateRect(LPCRECT, BOOL = TRUE) {}
    void ClientToScreenRect(LPRECT) const {}
    BOOL SetTimer(UINT, UINT, void* = NULL) { return TRUE; }
    BOOL KillTimer(UINT) { return TRUE; }
    void SetWindowPos(const CWnd*, int, int, int, int, UINT) {}
    void BringWindowToTop() {}
    BOOL IsWindowVisible() const { return FALSE; }
    void SetCapture() {}
    CWnd* GetParent() const { return m_pBobParent; }
    CWnd* GetParentFrame() const { return NULL; }
    CWnd* GetParentOwner() const { return NULL; }
    BOOL UpdateData(BOOL = TRUE) { return TRUE; }
    virtual BOOL OnInitDialog() { return TRUE; }
    virtual void DoDataExchange(class CDataExchange*) {}
    virtual LRESULT WindowProc(UINT, WPARAM, LPARAM) { return 0; }
    /* standard message handlers (derived classes call base::OnXxx) */
    afx_msg int  OnCreate(void*) { return 0; }
    afx_msg void OnDestroy() {}
    afx_msg void OnPaint() {}
    afx_msg void OnSize(UINT, int, int) {}
    afx_msg void OnTimer(UINT_PTR) {}
    afx_msg void OnClose() {}
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnLButtonDown(UINT, CPoint) {}
    afx_msg void OnLButtonUp(UINT, CPoint) {}
    afx_msg void OnLButtonDblClk(UINT, CPoint) {}
    afx_msg void OnRButtonDown(UINT, CPoint) {}
    afx_msg void OnRButtonUp(UINT, CPoint) {}
    afx_msg void OnMouseMove(UINT, CPoint) {}
    afx_msg BOOL OnMouseWheel(UINT, short, CPoint) { return FALSE; }
    afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT) { return TRUE; }
    afx_msg void OnKeyDown(UINT, UINT, UINT) {}
    afx_msg void OnKeyUp(UINT, UINT, UINT) {}
    afx_msg void OnChar(UINT, UINT, UINT) {}
    afx_msg void OnHScroll(UINT, UINT, CScrollBar*) {}
    afx_msg void OnVScroll(UINT, UINT, CScrollBar*) {}
    afx_msg void OnSetFocus(CWnd*) {}
    afx_msg void OnKillFocus(CWnd*) {}
    afx_msg void OnActivate(UINT, CWnd*, BOOL) {}
    afx_msg void OnMove(int, int) {}
    afx_msg void OnShowWindow(BOOL, UINT) {}
    afx_msg void OnEnable(BOOL) {}
    afx_msg void OnWindowPosChanging(void*) {}
    afx_msg void OnWindowPosChanged(void*) {}
    afx_msg void OnCaptureChanged(CWnd*) {}
    afx_msg LRESULT OnNotify(WPARAM, LPARAM, LRESULT*) { return 0; }
    afx_msg BOOL OnCommand(WPARAM, LPARAM) { return TRUE; }
    afx_msg void OnGetMinMaxInfo(MINMAXINFO*) {}
    afx_msg void OnDevModeChange(LPSTR) {}
    afx_msg void OnActivateApp(BOOL, DWORD) {}
    afx_msg void OnHelp() {}
    afx_msg void OnHelpFinder() {}
    afx_msg void OnHelpIndex() {}
    afx_msg void OnHelpUsing() {}
    afx_msg void OnContextHelp() {}
    afx_msg LRESULT OnHelpInfo(struct tagHELPINFO*) { return 0; }
    afx_msg LRESULT OnCommandHelp(WPARAM = 0, LPARAM = 0) { return 0; }
    afx_msg int  OnMouseActivate(CWnd*, UINT, UINT) { return 1; }
    virtual BOOL PreCreateWindow(struct tagCREATESTRUCTA&) { return TRUE; }
    virtual BOOL PreTranslateMessage(void*) { return FALSE; }
    virtual void PostNcDestroy() {}
    virtual BOOL OnCmdMsg(UINT, int, void*, AFX_CMDHANDLERINFO*) { return FALSE; }
    afx_msg void OnNcMouseMove(UINT, CPoint) {}
    afx_msg void OnNcLButtonDown(UINT, CPoint) {}
    afx_msg LRESULT OnNcHitTest(CPoint) { return 0; }
    afx_msg void OnNcPaint() {}
    afx_msg BOOL OnNcCreate(void*) { return TRUE; }
    void MapDialogRect(LPRECT) const {}
    BOOL IsFrameWnd() const { return FALSE; }
    CWnd* GetTopLevelParent() const { return NULL; }
    CWnd* GetTopLevelFrame() const { return NULL; }
    CWnd* GetTopLevelOwner() const { return NULL; }
    int   GetSystemMetrics(int) const { return 0; }
};

/* Common control wrappers (all CWnd-derived stubs) */
class CStatic : public CWnd {
public:
    BOOL Create(LPCSTR, DWORD, const RECT&, CWnd*, UINT = 0) { return TRUE; }
    void SetBitmap(HBITMAP) {}
};
class CButton : public CWnd {
public:
    BOOL Create(LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    UINT GetState() const { return 0; }
    void SetState(BOOL) {}
    int  GetCheck() const { return 0; }
    void SetCheck(int) {}
};
class CEdit : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    void SetSel(int, int, BOOL = FALSE) {}
    void GetSel(int&, int&) const {}
    int  LineLength(int = -1) const { return 0; }
};
class CListBox : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  AddStringA(LPCSTR) { return 0; }
    int  GetCurSel() const { return -1; }
    int  SetCurSel(int) { return -1; }
    int  GetCount() const { return 0; }
    void ResetContent() {}
    DWORD GetItemData(int) const { return 0; }
    int  SetItemData(int, DWORD) { return 0; }
};
class CComboBox : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  AddStringA(LPCSTR) { return 0; }
    int  GetCurSel() const { return -1; }
    int  SetCurSel(int) { return -1; }
    int  GetCount() const { return 0; }
    void ResetContent() {}
    DWORD GetItemData(int) const { return 0; }
};
class CScrollBar : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  GetScrollPos() const { return 0; }
    int  SetScrollPos(int, BOOL = TRUE) { return 0; }
    void SetScrollRange(int, int, BOOL = TRUE) {}
    void GetScrollRange(LPINT, LPINT) const {}
};
class CToolBar : public CWnd {
public:
    BOOL Create(CWnd*, DWORD = 0, UINT = 0) { return TRUE; }
};
class CMenu : public CObject {
public:
    HMENU m_hMenu;
    CMenu() : m_hMenu(NULL) {}
    HMENU GetSafeHmenu() const { return m_hMenu; }
    CMenu* GetSubMenu(int) const { return NULL; }
    UINT GetMenuItemCount() const { return 0; }
    BOOL AppendMenuA(UINT, UINT_PTR = 0, LPCSTR = NULL) { return TRUE; }
    BOOL AppendMenu(UINT f, UINT_PTR id = 0, LPCSTR s = NULL) { return AppendMenuA(f, id, s); }
    BOOL InsertMenu(UINT, UINT, UINT_PTR = 0, LPCSTR = NULL) { return TRUE; }
    BOOL ModifyMenu(UINT, UINT, UINT_PTR = 0, LPCSTR = NULL) { return TRUE; }
    BOOL DeleteMenu(UINT, UINT) { return TRUE; }
    BOOL RemoveMenu(UINT, UINT) { return TRUE; }
    BOOL CreatePopupMenu() { return TRUE; }
    BOOL CreateMenu() { return TRUE; }
    BOOL LoadMenu(UINT) { return TRUE; }
    BOOL LoadMenu(LPCSTR) { return TRUE; }
    BOOL DestroyMenu() { return TRUE; }
    void Attach(HMENU h) { m_hMenu = h; }
    HMENU Detach() { HMENU h = m_hMenu; m_hMenu = NULL; return h; }
    BOOL EnableMenuItem(UINT, UINT) { return TRUE; }
    BOOL CheckMenuItem(UINT, UINT) { return TRUE; }
    UINT GetMenuState(UINT, UINT) const { return 0; }
    int  GetMenuStringA(UINT, LPSTR, int, UINT) const { return 0; }
    BOOL SetMenuItemBitmaps(UINT, UINT, CBitmap*, CBitmap*) { return TRUE; }
    BOOL TrackPopupMenu(UINT, int, int, CWnd*, LPCRECT = NULL) { return TRUE; }
};

/* SetWindowPos z-order sentinels (MFC globals: &wndTop etc.) */
static const CWnd wndTop, wndBottom, wndTopMost, wndNoTopMost;

class CDialog : public CWnd {
public:
    CDialog() {}
    CDialog(UINT, CWnd* = NULL) {}
    CDialog(LPCSTR, CWnd* = NULL) {}
    virtual int DoModal() { return -1; }   /* IDCANCEL-ish */
    /* Defined out-of-line (needs complete CDataExchange): drives DoDataExchange so
       the R* ActiveX controls get created/bound (MFC's OnInitDialog path is no-op'd). */
    BOOL Create(UINT nID, CWnd* parent = NULL);
    virtual void OnOK() {}
    virtual void OnCancel() {}
    virtual LRESULT OnCommandHelp(WPARAM, LPARAM) { return 0; }
    void EndDialog(int) {}
    void GotoDlgCtrl(CWnd*) {}
    void NextDlgCtrl() const {}
};

class CView : public CWnd {
public:
    CDocument* m_pDocument;
    CView() : m_pDocument(NULL) {}
    virtual void OnDraw(CDC*) {}
    CDocument* GetDocument() const { return m_pDocument; }
    CScrollBar* GetScrollBarCtrl(int) const { return NULL; }
    virtual BOOL OnPreparePrinting(CPrintInfo*) { return TRUE; }
    virtual void OnBeginPrinting(CDC*, CPrintInfo*) {}
    virtual void OnEndPrinting(CDC*, CPrintInfo*) {}
    virtual void OnPrint(CDC*, CPrintInfo*) {}
    BOOL DoPreparePrinting(CPrintInfo*) { return TRUE; }
};

/* ============================================================
 * CPropExchange — S126: a real persisted-property-stream READER.
 * On Windows the dialog editor saves each hosted OCX's state (IPersistStreamInit)
 * into the DLGINIT resource; at dialog creation MFC replays it into the control's
 * DoPropExchange through a CArchivePropExchange. Our hosts used to boot from an
 * EMPTY exchange (every PX_* fell back to its default), losing all design-time
 * properties. This reader replays the genuine stream (bob_dlg_propbag) instead.
 *
 * Stream layout (reverse-engineered from boblang.dll; validated offline against
 * all 1280 R*-class RT240 bags, zero failures — see bob_dlgtemplate.cpp):
 *   [DWORD licence-wchar-count][UTF-16 licence]  (COccManager licence prefix)
 *   [DWORD version]                              (ExchangeVersion)
 *   [DWORD extentX][DWORD extentY]               (ExchangeExtent, HIMETRIC)
 *   [DWORD stockPropMask] + stock props:         (ExchangeStockProps)
 *       0x02 Caption=CString  0x08 ForeColor=DWORD
 *       0x01 BackColor=DWORD  0x40 Enabled=BYTE   (other bits: abort -> defaults)
 *   [control props in DoPropExchange source order:
 *       PX_Bool=BYTE  PX_Short=WORD  PX_Long/PX_Color=DWORD  PX_String=CString]
 * Trailing bytes are editor slop — unread, exactly as on Windows.
 * A default-constructed (unattached) CPropExchange behaves as before: every
 * PX_* loads its default. On any mid-stream error m_bOk drops and the REMAINING
 * PX_* calls load defaults (fail-safe).
 * ============================================================ */
class CPropExchange {
public:
    const unsigned char* m_pData = NULL;
    int   m_nLen = 0, m_nPos = 0;
    BOOL  m_bOk = FALSE;          /* attached and healthy */
    DWORD m_dwVersion = 0;
    BOOL IsLoading() const { return TRUE; }
    DWORD GetVersion() const { return m_dwVersion; }
    /* Attach a raw DLGINIT bag: skip the licence prefix; leave m_nPos at the
       version DWORD (the control's ExchangeVersion call consumes it). */
    BOOL Attach(const unsigned char* p, int n) {
        if (!p || n < 16) return FALSE;
        DWORD lic = (DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
        if (lic < 8 || lic > 128 || 4 + 2 * (int)lic + 4 > n) return FALSE;
        m_pData = p; m_nLen = n; m_nPos = 4 + 2 * (int)lic; m_bOk = TRUE;
        return TRUE;
    }
    BOOL Need(int k) { if (!m_bOk || m_nPos + k > m_nLen) { m_bOk = FALSE; return FALSE; } return TRUE; }
    BOOL ReadU8(BYTE& v) { if (!Need(1)) return FALSE; v = m_pData[m_nPos++]; return TRUE; }
    BOOL ReadU16(WORD& v) {
        if (!Need(2)) return FALSE;
        v = (WORD)(m_pData[m_nPos] | (m_pData[m_nPos+1] << 8)); m_nPos += 2; return TRUE;
    }
    BOOL ReadU32(DWORD& v) {
        if (!Need(4)) return FALSE;
        v = (DWORD)m_pData[m_nPos] | ((DWORD)m_pData[m_nPos+1] << 8)
          | ((DWORD)m_pData[m_nPos+2] << 16) | ((DWORD)m_pData[m_nPos+3] << 24);
        m_nPos += 4; return TRUE;
    }
    /* MFC CString archive: BYTE len; 0xFF -> WORD len; 0xFF/0xFFFF -> DWORD len.
       (The 0xFFFE unicode marker never occurs in the shipped bags -> bad.) */
    BOOL ReadStr(CString& s) {
        BYTE b; if (!ReadU8(b)) return FALSE;
        DWORD n = b;
        if (b == 0xFF) {
            WORD w; if (!ReadU16(w)) return FALSE;
            if (w == 0xFFFE) { m_bOk = FALSE; return FALSE; }
            n = w;
            if (w == 0xFFFF) { if (!ReadU32(n)) return FALSE; }
        }
        if (!Need((int)n)) return FALSE;
        char tmp[1024];
        DWORD c = n < sizeof(tmp) - 1 ? n : (DWORD)sizeof(tmp) - 1;
        if (c) memcpy(tmp, m_pData + m_nPos, c);
        tmp[c] = 0;
        s = tmp;
        m_nPos += (int)n;
        return TRUE;
    }
    BOOL ExchangeProp(LPCSTR, VARTYPE, void*, const void* = NULL) { return TRUE; }
    BOOL ExchangeVersion(DWORD&, DWORD, BOOL = TRUE) { return TRUE; }
};
/* Free-function form the R* controls call first: ExchangeVersion(pPX, MAKELONG(...)).
   With a stream attached this READS the persisted version (gates the controls'
   `GetVersion()&x` branches); unattached it records the control's own default. */
inline DWORD ExchangeVersion(CPropExchange* pPX, DWORD v, BOOL = TRUE) {
    if (!pPX) return v;
    DWORD sv;
    if (pPX->m_bOk && pPX->ReadU32(sv)) pPX->m_dwVersion = sv;
    else                                pPX->m_dwVersion = v;
    return pPX->m_dwVersion;
}

/* COleControl — MFC ActiveX control base (bob's CR* control impls derive from it) */
class COleControl : public CWnd {
public:
    BOOL m_bAutoSize;
    BOOL m_bobEnabled = TRUE;     /* stock Enabled (persisted via the property stream) */
    virtual void OnDraw(CDC*, const CRect&, const CRect&) {}
    /* S126: consume the stock-property block exactly as MFC's
       COleControl::DoPropExchange (ExchangeExtent + ExchangeStockProps) — the
       R* DoPropExchange overrides call this before their own PX_* fields. */
    virtual void DoPropExchange(CPropExchange* pPX) {
        if (!pPX || !pPX->m_bOk) return;
        DWORD cx, cy, mask;
        if (!pPX->ReadU32(cx) || !pPX->ReadU32(cy)) return;      /* extent (HIMETRIC) */
        if (!pPX->ReadU32(mask)) return;
        if (mask & ~0x4Bu) { pPX->m_bOk = FALSE; return; }       /* unknown layout -> defaults */
        if (mask & 0x02) { CString cap; if (pPX->ReadStr(cap)) m_bobText = cap; }
        if (mask & 0x08) { DWORD c; if (pPX->ReadU32(c)) SetForeColor((OLE_COLOR)c); }
        if (mask & 0x01) { DWORD c; if (pPX->ReadU32(c)) SetBackColor((OLE_COLOR)c); }
        if (mask & 0x40) { BYTE e; if (pPX->ReadU8(e)) m_bobEnabled = e ? TRUE : FALSE; }
    }
    virtual void OnResetState() {}
    virtual void OnTextChanged() {}
    /* stock text/enabled the R* controls read in OnDraw */
    CString InternalGetText() { return m_bobText; }
    void    InternalSetText(LPCTSTR s) { m_bobText = s ? s : ""; }
    BOOL    GetEnabled() { return m_bobEnabled; }
    void    SetEnabled(BOOL e) { m_bobEnabled = e; }
    virtual void OnKeyDownEvent(unsigned short, unsigned short) {}   /* CRButtonCtrl base key event (no-op) */
    virtual void OnDrawMetafile(CDC*, const CRect&) {}
    void InvalidateControl(LPCRECT = NULL) {}
    void SetModifiedFlag(BOOL = TRUE) {}
    void BoundPropertyChanged(DISPID) {}
    BOOL GetControlSize(int*, int*) { return TRUE; }
    BOOL SetControlSize(int, int) { return TRUE; }
    void SetInitialSize(int, int) {}
    COleControl* GetControlUnknown() { return this; }
    void FireEventV(DISPID, const char*, va_list) {}
    void ThrowError(SCODE, LPCSTR = NULL) {}
    void SetNotPermitted() {}
    void SetNotSupported() {}
    BOOL DoSuperclassPaint(CDC*, const CRect&) { return TRUE; }
    BSTR  GetText() { return NULL; }
    void  SetText(LPCSTR) {}
    OLE_COLOR GetForeColor() { return m_foreColor; }
    OLE_COLOR GetBackColor() { return m_backColor; }
    void SetForeColor(OLE_COLOR c) { m_foreColor = c; }
    void SetBackColor(OLE_COLOR c) { m_backColor = c; }
    CFont* SelectStockFont(CDC*) { return NULL; }
    BOOL IsModified() const { return FALSE; }
    void Refresh() {}
    /* ActiveX control ctors call InitializeIIDs(&IID_Disp,&IID_Events) */
    void InitializeIIDs(const void*, const void*) {}
    OLE_COLOR TranslateColor(OLE_COLOR c, void* = NULL) { return c; }
    /* ambient/stock helpers used by the R* controls */
    BOOL AmbientUserMode() { return TRUE; }
    OLE_COLOR AmbientForeColor() { return m_foreColor; }
    OLE_COLOR AmbientBackColor() { return m_backColor; }
private:
    OLE_COLOR m_foreColor = 0;
    OLE_COLOR m_backColor = 0;
};

/* ---- design-time property page (parse-only; never instantiated at runtime) ---- */
class COlePropertyPage : public CDialog {
public:
    COlePropertyPage(UINT = 0, UINT = 0) {}
    void SetHelpInfo(LPCSTR, LPCSTR = NULL, DWORD = 0) {}
    void SetModifiedFlag(BOOL = TRUE) {}
    void SetPropText(LPCSTR) {}
};

/* ---- DoPropExchange PX_* persistence helpers. S126: with a stream attached
   (CPropExchange::Attach, fed from the DLGINIT property bag) each PX_* READS its
   persisted value — the genuine control's DoPropExchange source order IS the
   stream order. Unattached (or after a stream error) they load defaults, the
   pre-S126 behaviour. Binary encodings per the MFC archive exchange:
   Bool=BYTE, Short=WORD, Long/Color/ULong=DWORD, String=CString archive. ---- */
inline BOOL PX_Bool (CPropExchange* px, LPCSTR, BOOL&  v, BOOL  d = FALSE) {
    BYTE b; if (px && px->m_bOk && px->ReadU8(b)) v = b ? TRUE : FALSE; else v = d; return TRUE; }
inline BOOL PX_Bool (CPropExchange* px, LPCSTR, short& v, BOOL  d = FALSE) {
    BYTE b; if (px && px->m_bOk && px->ReadU8(b)) v = (short)(b ? 1 : 0); else v = (short)d; return TRUE; }
inline BOOL PX_Short(CPropExchange* px, LPCSTR, short& v, short d = 0) {
    WORD w; if (px && px->m_bOk && px->ReadU16(w)) v = (short)w; else v = d; return TRUE; }
inline BOOL PX_Long (CPropExchange* px, LPCSTR, long&  v, long  d = 0) {
    DWORD u; if (px && px->m_bOk && px->ReadU32(u)) v = (long)(int)u; else v = d; return TRUE; }
inline BOOL PX_Color(CPropExchange* px, LPCSTR, OLE_COLOR& v, OLE_COLOR d = 0) {
    DWORD u; if (px && px->m_bOk && px->ReadU32(u)) v = (OLE_COLOR)u; else v = d; return TRUE; }
inline BOOL PX_ULong(CPropExchange* px, LPCSTR, ULONG& v, ULONG d = 0) {
    DWORD u; if (px && px->m_bOk && px->ReadU32(u)) v = (ULONG)u; else v = d; return TRUE; }
inline BOOL PX_Font (CPropExchange*, LPCSTR, void*, void* = NULL)       { return TRUE; }
inline BOOL PX_Picture(CPropExchange*, LPCSTR, void*, void* = NULL)     { return TRUE; }
inline BOOL PX_String(CPropExchange* px, LPCSTR, CString& v, LPCSTR d = "") {
    if (px && px->m_bOk && px->ReadStr(v)) return TRUE;
    v = d ? d : ""; return TRUE; }

/* ---- OLE control class factory + registration shims (DllRegisterServer path;
        dead at runtime but the control sources define these members). ---- */
class COleObjectFactoryEx {
public:
    CLSID  m_clsid;
    LPCSTR m_lpszProgID;
    COleObjectFactoryEx() : m_lpszProgID(NULL) { memset(&m_clsid, 0, sizeof(m_clsid)); }
    virtual ~COleObjectFactoryEx() {}
    virtual BOOL VerifyUserLicense() { return TRUE; }
    virtual BOOL GetLicenseKey(DWORD, BSTR FAR*) { return FALSE; }
    static BOOL UpdateRegistryAll(BOOL = TRUE) { return TRUE; }
};
typedef COleObjectFactoryEx COleObjectFactory;

#ifndef OLEMISC_RECOMPOSEONRESIZE
#define OLEMISC_RECOMPOSEONRESIZE     0x00000001L
#define OLEMISC_ACTIVATEWHENVISIBLE   0x00000100L
#define OLEMISC_INSIDEOUT             0x00000080L
#define OLEMISC_CANTLINKINSIDE        0x00000010L
#define OLEMISC_SETCLIENTSITEFIRST    0x00020000L
#endif
#ifndef afxRegApartmentThreading
#define afxRegApartmentThreading 1
#endif

inline BOOL AfxOleRegisterControlClass(HINSTANCE, const GUID&, LPCSTR, UINT, UINT,
                                       int, DWORD, const GUID&, WORD, WORD) { return TRUE; }
inline BOOL AfxOleUnregisterClass(const GUID&, LPCSTR) { return TRUE; }
inline BOOL AfxOleRegisterTypeLib(HINSTANCE, const GUID&, LPCSTR = NULL) { return TRUE; }
inline BOOL AfxOleUnregisterTypeLib(const GUID&, WORD = 0, WORD = 0) { return TRUE; }
inline BOOL AfxVerifyLicFile(HINSTANCE, LPCTSTR, LPCWSTR, ... ) { return TRUE; }
#ifndef BOB_SYSALLOCSTRING
#define BOB_SYSALLOCSTRING
inline BSTR SysAllocString(const wchar_t*) { return NULL; }
inline void SysFreeString(BSTR) {}
#endif
inline UINT RegisterClipboardFormat(LPCSTR) { static UINT n = 0xC000; return ++n; }

/* ActiveX control dispatch/event IIDs: ctors pass &IID_* to InitializeIIDs
   (a no-op in the compat). Defined in bob_ole.cpp. */
extern const GUID IID_DRListBox;
extern const GUID IID_DRListBoxEvents;

/* GDI bits the R* control OnDraw paths need. */
#ifndef ETO_CLIPPED
#define ETO_CLIPPED 0x0004
#endif
#ifndef ETO_OPAQUE
#define ETO_OPAQUE  0x0002
#endif
/* Global GetTextExtentExPoint(HDC,...) — the control passes *pCDC (operator HDC).
   Report all characters fitting; width filled from the per-glyph advances later. */
inline BOOL GetTextExtentExPoint(HDC, LPCSTR, int cchString, int, LPINT lpnFit,
                                 LPINT, LPSIZE lpSize) {
    if (lpnFit) *lpnFit = cchString;
    if (lpSize) { lpSize->cx = 0; lpSize->cy = 16; }
    return TRUE;
}

class CFrameWnd : public CWnd {
public:
    CView* m_pActiveView_compat = NULL;   /* the doc/view framework is no-op'd; track it ourselves */
    BOOL Create(LPCSTR, LPCSTR, DWORD = 0, const RECT& = CRect(), CWnd* = NULL, LPCSTR = NULL) { return TRUE; }
    CView* GetActiveView() const { return m_pActiveView_compat; }
    CDocument* GetActiveDocument() const { return NULL; }
    void RecalcLayout(BOOL = TRUE) {}
    BOOL SetActiveView(CView* v, BOOL = TRUE) { m_pActiveView_compat = v; return TRUE; }
    void ExitHelpMode() {}
};

/* CFile / CArchive / CPrintInfo (afx.h) */
class CFile : public CObject {
public:
    enum { modeRead = 0, modeWrite = 1, modeReadWrite = 2, modeCreate = 0x1000,
           shareDenyNone = 0x40, shareDenyWrite = 0x20, typeBinary = 0x4000, begin = 0, current = 1, end = 2 };
    HANDLE m_hFile;
    CFile() : m_hFile(NULL) {}
    virtual BOOL Open(LPCSTR, UINT, void* = NULL) { return FALSE; }
    virtual UINT Read(void*, UINT) { return 0; }
    virtual void Write(const void*, UINT) {}
    virtual void Close() {}
    virtual DWORD GetLength() const { return 0; }
    virtual LONG Seek(LONG, UINT) { return 0; }
};

class CArchive {
public:
    enum Mode { load = 0, store = 1 };
    BOOL IsStoring() const { return FALSE; }
    BOOL IsLoading() const { return TRUE; }
};

class CPrintInfo {
public:
    BOOL m_bContinuePrinting;
    UINT m_nCurPage;
    CRect m_rectDraw;
    CPrintInfo() : m_bContinuePrinting(TRUE), m_nCurPage(1) {}
    void SetMaxPage(UINT) {}
    void SetMinPage(UINT) {}
    UINT GetFromPage() const { return 1; }
    UINT GetToPage() const { return 1; }
};

class CDocument : public CCmdTarget {
public:
    virtual BOOL OnNewDocument() { return TRUE; }
    virtual void Serialize(CArchive&) {}
    void SetTitle(LPCSTR) {}
    LPCSTR GetTitle() const { return ""; }
    void SetPathName(LPCSTR, BOOL = TRUE) {}
    LPCSTR GetPathName() const { return ""; }
    void EnableCompoundFile(BOOL = TRUE) {}
    void SetModifiedFlag(BOOL = TRUE) {}
    BOOL IsModified() { return FALSE; }
    void UpdateAllViews(CView*, LPARAM = 0, CObject* = NULL) {}
    CView* GetNextView(POSITION&) const { return NULL; }
    POSITION GetFirstViewPosition() const { return (POSITION)0; }
};

class COleDocument : public CDocument {};

void CDocument_dummy();
/* extend CDocument with the methods bob calls (added here to keep the class above
   minimal); these are just declared inline on a derived-friendly basis */

class CDocTemplate : public CCmdTarget {
public:
    CDocTemplate(UINT, void* = NULL, void* = NULL, void* = NULL) {}
};
class CSingleDocTemplate : public CDocTemplate {
public:
    CSingleDocTemplate(UINT id, void* a = NULL, void* b = NULL, void* c = NULL) : CDocTemplate(id,a,b,c) {}
    void SetContainerInfo(UINT) {}
    void SetServerInfo(UINT, UINT = 0, UINT = 0, void* = NULL, void* = NULL) {}
};
class CMultiDocTemplate : public CDocTemplate {
public:
    CMultiDocTemplate(UINT id, void* a = NULL, void* b = NULL, void* c = NULL) : CDocTemplate(id,a,b,c) {}
};

/* MFC OLE / app-init globals */
static inline BOOL AfxOleInit() { return TRUE; }
static inline void AfxEnableControlContainer(void* = NULL) {}
static inline BOOL AfxOleGetUserCtrl() { return FALSE; }
static inline void AfxPostQuitMessage(int = 0) {}
static inline void AfxOleSetUserCtrl(BOOL) {}
static inline CWinApp* AfxGetAppHelper() { return NULL; }

/* DDX/DDV (dialog data exchange) — no-ops */
/* Bind an ActiveX control wrapper: dispatch to the wrapper's virtual Create()
   (which calls CreateControl(GetClsid(),...) -> bob_ole hosting). Defined below,
   after CDataExchange is complete (needs pDX->m_pDlgWnd as the control parent). */
inline void DDX_Control(CDataExchange* pDX, int nID, CWnd& ctrl);
static inline void DDX_Text(CDataExchange*, int, int&) {}
static inline void DDX_Check(CDataExchange*, int, int&) {}
static inline void DDX_Radio(CDataExchange*, int, int&) {}
static inline void DDX_LBIndex(CDataExchange*, int, int&) {}
static inline void DDX_CBIndex(CDataExchange*, int, int&) {}

class CWinThread : public CCmdTarget {
public:
    CWnd* m_pMainWnd;
    CWnd* m_pActiveWnd;
    MSG   m_msgCur;
    CWinThread() : m_pMainWnd(NULL), m_pActiveWnd(NULL) {}
    virtual BOOL InitInstance() { return TRUE; }
    virtual int  ExitInstance() { return 0; }
    virtual int  Run() { return 0; }
};

class CWinApp : public CWinThread {
public:
    LPCSTR m_pszAppName;
    LPCSTR m_pszHelpFilePath;
    LPCSTR m_pszProfileName;
    LPCSTR m_pszExeName;
    HINSTANCE m_hInstance;
    LPSTR  m_lpCmdLine;
    int    m_nCmdShow;
    CWinApp(LPCSTR n = NULL) : m_pszAppName(n), m_hInstance(NULL), m_lpCmdLine(NULL), m_nCmdShow(0) {}
    virtual BOOL InitInstance() { return TRUE; }
    BOOL InitApplication() { return TRUE; }
    HCURSOR LoadStandardCursor(LPCSTR) const { return NULL; }
    HCURSOR LoadCursor(LPCSTR) const { return NULL; }
    HCURSOR LoadCursor(UINT) const { return NULL; }
    HICON   LoadIcon(LPCSTR) const { return NULL; }
    HICON   LoadIcon(UINT) const { return NULL; }
    HICON   LoadStandardIcon(LPCSTR) const { return NULL; }
    int     DoMessageBox(LPCSTR, UINT, UINT) { return 0; }
    void    ParseCommandLine(CCommandLineInfo&) {}
    BOOL    ProcessShellCommand(CCommandLineInfo&) { return TRUE; }
    void    EnableShellOpen() {}
    void    LoadStdProfileSettings(UINT = 0) {}
    BOOL    OnIdle(LONG) { return FALSE; }
    void    WinHelp(DWORD, UINT = 0) {}
    void    HtmlHelp(DWORD, UINT = 0) {}
    void    SetRegistryKey(LPCSTR) {}
    void    SetRegistryKey(UINT) {}
    BOOL    PumpMessage() { return TRUE; }
    BOOL    IsIdleMessage(void*) { return TRUE; }
    void    Enable3dControls() {}
    void    Enable3dControlsStatic() {}
    void    AddDocTemplate(void*) {}
    HCURSOR DoWaitCursor(int) { return NULL; }
    void    RestoreWaitCursor() {}
    void    BeginWaitCursor() {}
    void    EndWaitCursor() {}
    UINT    GetProfileIntA(LPCSTR, LPCSTR, int n) { return n; }
    BOOL    WriteProfileIntA(LPCSTR, LPCSTR, int) { return TRUE; }
    CString GetProfileStringA(LPCSTR, LPCSTR, LPCSTR = NULL);
    BOOL    WriteProfileStringA(LPCSTR, LPCSTR, LPCSTR) { return TRUE; }
};

/* OLE control module base (CRListBoxApp : COleControlModule). Registration is a
   no-op on Linux; the module object exists only so the control DLL's theApp ctor
   runs. Defined here (after CWinApp) to satisfy the inheritance. */
class COleControlModule : public CWinApp {
public:
    virtual BOOL InitInstance() { return TRUE; }
    virtual int  ExitInstance() { return 0; }
};

/* ============================================================
 * Container templates (afxtempl) — minimal, std-backed
 * ============================================================ */
template <class TYPE, class ARG_TYPE = const TYPE&>
class CArray : public CObject {
    std::vector<TYPE> v;
public:
    int  GetSize() const { return (int)v.size(); }
    int  GetCount() const { return (int)v.size(); }
    void SetSize(int n, int = -1) { v.resize(n); }
    void RemoveAll() { v.clear(); }
    int  Add(ARG_TYPE x) { v.push_back(x); return (int)v.size() - 1; }
    TYPE& operator[](int i) { return v[i]; }
    const TYPE& operator[](int i) const { return v[i]; }
    TYPE& GetAt(int i) { return v[i]; }
    void SetAt(int i, ARG_TYPE x) { v[i] = x; }
    void RemoveAt(int i, int n = 1) { v.erase(v.begin()+i, v.begin()+i+n); }
};

/* Real doubly-linked CList with working POSITION semantics. The R* ActiveX
   controls (CRListBoxCtrl::m_list is a CList<CList<char*,char*>*,...>) drive
   GetHeadPosition/GetNext/GetAt/FindIndex/SetAt/RemoveAt/InsertAfter heavily,
   so the earlier std::list-with-stubbed-POSITION shim would NULL-deref. POSITION
   is an opaque node pointer (POSITION == __POSITION*). */
template <class TYPE, class ARG_TYPE = const TYPE&>
class CList : public CObject {
    struct Node { Node* prev; Node* next; TYPE data; };
    Node* m_head; Node* m_tail; int m_count;
public:
    CList(int /*nBlockSize*/ = 10) : m_head(0), m_tail(0), m_count(0) {}
    ~CList() { RemoveAll(); }
    int  GetCount() const { return m_count; }
    int  GetSize()  const { return m_count; }
    BOOL IsEmpty()  const { return m_count == 0; }
    void RemoveAll() { Node* n=m_head; while(n){ Node* nx=n->next; delete n; n=nx; } m_head=m_tail=0; m_count=0; }
    POSITION AddTail(ARG_TYPE x) { Node* n=new Node; n->data=x; n->next=0; n->prev=m_tail; if(m_tail)m_tail->next=n; else m_head=n; m_tail=n; m_count++; return (POSITION)n; }
    POSITION AddHead(ARG_TYPE x) { Node* n=new Node; n->data=x; n->prev=0; n->next=m_head; if(m_head)m_head->prev=n; else m_tail=n; m_head=n; m_count++; return (POSITION)n; }
    TYPE& GetHead() { return m_head->data; }
    const TYPE& GetHead() const { return m_head->data; }
    TYPE& GetTail() { return m_tail->data; }
    const TYPE& GetTail() const { return m_tail->data; }
    TYPE  RemoveHead() { Node* n=m_head; TYPE d=n->data; m_head=n->next; if(m_head)m_head->prev=0; else m_tail=0; delete n; m_count--; return d; }
    TYPE  RemoveTail() { Node* n=m_tail; TYPE d=n->data; m_tail=n->prev; if(m_tail)m_tail->next=0; else m_head=0; delete n; m_count--; return d; }
    POSITION GetHeadPosition() const { return (POSITION)m_head; }
    POSITION GetTailPosition() const { return (POSITION)m_tail; }
    TYPE& GetNext(POSITION& p) { Node* n=(Node*)p; p=(POSITION)(n->next); return n->data; }
    const TYPE& GetNext(POSITION& p) const { Node* n=(Node*)p; p=(POSITION)(n->next); return n->data; }
    TYPE& GetPrev(POSITION& p) { Node* n=(Node*)p; p=(POSITION)(n->prev); return n->data; }
    const TYPE& GetPrev(POSITION& p) const { Node* n=(Node*)p; p=(POSITION)(n->prev); return n->data; }
    TYPE& GetAt(POSITION p) { return ((Node*)p)->data; }
    const TYPE& GetAt(POSITION p) const { return ((Node*)p)->data; }
    void  SetAt(POSITION p, ARG_TYPE x) { ((Node*)p)->data = x; }
    POSITION Find(ARG_TYPE x, POSITION after=0) const { Node* n = after?((Node*)after)->next:m_head; for(;n;n=n->next) if(n->data==x) return (POSITION)n; return 0; }
    POSITION FindIndex(int idx) const { if(idx<0||idx>=m_count) return 0; Node* n=m_head; while(idx-- && n) n=n->next; return (POSITION)n; }
    void  RemoveAt(POSITION p) { Node* n=(Node*)p; if(n->prev)n->prev->next=n->next; else m_head=n->next; if(n->next)n->next->prev=n->prev; else m_tail=n->prev; delete n; m_count--; }
    POSITION InsertBefore(POSITION p, ARG_TYPE x) { Node* at=(Node*)p; Node* n=new Node; n->data=x; n->next=at; n->prev=at->prev; if(at->prev)at->prev->next=n; else m_head=n; at->prev=n; m_count++; return (POSITION)n; }
    POSITION InsertAfter(POSITION p, ARG_TYPE x) { Node* at=(Node*)p; Node* n=new Node; n->data=x; n->prev=at; n->next=at->next; if(at->next)at->next->prev=n; else m_tail=n; at->next=n; m_count++; return (POSITION)n; }
    /* S125: rvalue catch-all -- MFC's CList<T,T&> accepts temporaries
       (REDIT: wordlist.InsertAfter(pos, OneWord(...))); the ARG_TYPE=T&
       overload can't bind an rvalue under C++17. Non-template overload
       still wins for lvalues, so by-value instantiations are unaffected. */
    template <class U>
    POSITION InsertAfter(POSITION p, U&& x) { Node* at=(Node*)p; Node* n=new Node; n->data=x; n->prev=at; n->next=at->next; if(at->next)at->next->prev=n; else m_tail=n; at->next=n; m_count++; return (POSITION)n; }
};

class CCommandLineInfo {
public:
    BOOL m_bShowSplash;
    BOOL m_bRunEmbedded;
    BOOL m_bRunAutomated;
    CCommandLineInfo() : m_bShowSplash(TRUE), m_bRunEmbedded(FALSE), m_bRunAutomated(FALSE) {}
    void ParseParam(LPCSTR, BOOL, BOOL) {}
};

/* MFC OLE-control event descriptor (used in CCmdTarget::OnCmdMsg event sinks) */
class AFX_EVENT {
public:
    enum EventType { event = 0, command = 1, propRequestEdit = 2 };
    AFX_EVENT(int = event, DISPID = 0, void* = NULL, void* = NULL, void* = NULL) {}
    int      m_eventKind;
    DISPID   m_dispid;
};

class CDataExchange {
public:
    BOOL m_bSaveAndValidate;
    CWnd* m_pDlgWnd;
    CWnd* PrepareCtrl(int) { return NULL; }
    CWnd* PrepareEditCtrl(int) { return NULL; }
};

inline void DDX_Control(CDataExchange* pDX, int nID, CWnd& ctrl) {
    /* virtual-dispatch to the R* wrapper's Create() (CRListBox::Create etc.),
       which carries the control CLSID; the dialog (pDX->m_pDlgWnd) becomes the
       hosted control's parent. Plain CWnd::Create is a no-op (returns FALSE). */
    static const RECT z = {0,0,0,0};
    ctrl.Create((LPCTSTR)NULL, (LPCTSTR)NULL, 0, z,
                pDX ? pDX->m_pDlgWnd : (CWnd*)NULL, (UINT)nID);
}

inline BOOL CDialog::Create(UINT nID, CWnd* parent) {
    (void)nID;
    /* MFC binds dialog controls in OnInitDialog->UpdateData->DoDataExchange; that
       path is no-op'd on Linux, so the R* ActiveX controls are never created/bound.
       Drive DoDataExchange here so DDX_Control hosts them. Gated on BOB_FRONTEND to
       leave the default/cockpit paths untouched. */
    if (getenv("BOB_FRONTEND")) {
        m_pBobParent = parent;
        int savedIDD = g_bobDlgIDD; g_bobDlgIDD = (int)nID;   /* controls created below belong to this dialog */
        CDataExchange dx; dx.m_bSaveAndValidate = FALSE; dx.m_pDlgWnd = this;
        DoDataExchange(&dx);     /* bind/create the R* ActiveX controls (DDX_Control) */
        /* S124: on Windows the dialog manager creates EVERY template item; DDX only binds
           members. Host the template's label statics no DDX_Control bound (e.g. the
           Sim-Config Mission tab's 6 labels) from the installed build's PE resources. */
        bob_ole_host_template_statics(this, (int)nID);
        OnInitDialog();          /* run the dialog's init -> populates the controls
                                    (e.g. CSDetail fills its driver/resolution combos) */
        g_bobDlgIDD = savedIDD;
    }
    return TRUE;
}

class COleDispatchDriver {
public:
    LPDISPATCH m_lpDispatch;
    BOOL m_bAutoRelease;
    COleDispatchDriver() : m_lpDispatch(NULL), m_bAutoRelease(TRUE) {}
    COleDispatchDriver(LPDISPATCH p, BOOL autoRel = TRUE) : m_lpDispatch(p), m_bAutoRelease(autoRel) {}
    COleDispatchDriver(const COleDispatchDriver& s) : m_lpDispatch(s.m_lpDispatch), m_bAutoRelease(FALSE) {}
    void AttachDispatch(LPDISPATCH p, BOOL = TRUE) { m_lpDispatch = p; }
    LPDISPATCH DetachDispatch() { LPDISPATCH p = m_lpDispatch; m_lpDispatch = NULL; return p; }
    void ReleaseDispatch() {}
    BOOL CreateDispatch(REFCLSID, void* = NULL) { return FALSE; }
    BOOL CreateDispatch(LPCSTR, void* = NULL) { return FALSE; }
    void InvokeHelper(DISPID, WORD, VARTYPE, void*, const BYTE*, ...) {}
    void SetProperty(DISPID, VARTYPE, ...) {}
    void GetProperty(DISPID, VARTYPE, void*) const {}
};

struct AFX_CMDHANDLERINFO { CCmdTarget* pTarget; void* pmf; };

/* misc MFC/Win32 control-bar + help + dispatch bits */
#ifndef CBRS_GRIPPER
#define CBRS_TOP            0x0001
#define CBRS_BOTTOM         0x0002
#define CBRS_LEFT           0x0004
#define CBRS_RIGHT          0x0008
#define CBRS_ALIGN_ANY      0x000F
#define CBRS_ALIGN_TOP      0x0001
#define CBRS_ALIGN_BOTTOM   0x0002
#define CBRS_ALIGN_LEFT     0x0004
#define CBRS_ALIGN_RIGHT    0x0008
#define CBRS_BORDER_TOP     0x0100
#define CBRS_BORDER_ANY     0x0F00
#define CBRS_GRIPPER        0x00400000
#define CBRS_TOOLTIPS       0x00010000
#define CBRS_FLYBY          0x00020000
#define CBRS_SIZE_DYNAMIC   0x00040000
#endif
#ifndef DISPATCH_METHOD
#define DISPATCH_METHOD     0x1
#define DISPATCH_PROPERTYGET 0x2
#define DISPATCH_PROPERTYPUT 0x4
#endif
#ifndef HID_BASE_RESOURCE
#define HID_BASE_RESOURCE   0x00020000
#define HID_BASE_COMMAND    0x00010000
#endif
typedef HANDLE HTASK;
typedef struct tagHELPINFO { UINT cbSize; int iContextType; int iCtrlId; HANDLE hItemHandle; DWORD_PTR dwContextId; POINT MousePos; } HELPINFO, *LPHELPINFO;
struct AFX_MSGMAP { const AFX_MSGMAP* (*pfnGetBaseMap)(); const void* lpEntries; };

#ifndef HELP_CONTEXT
#define HELP_CONTEXT      0x0001
#define HELP_QUIT         0x0002
#define HELP_INDEX        0x0003
#define HELP_CONTENTS     0x0003
#define HELP_HELPONHELP   0x0004
#define HELP_SETINDEX     0x0005
#define HELP_KEY          0x0101
#define HELP_COMMAND      0x0102
#define HELP_FINDER       0x000B
#endif

#ifndef ID_SEPARATOR
#define ID_SEPARATOR        0
#define ID_INDICATOR_CAPS   0xE721
#define ID_INDICATOR_NUM    0xE722
#define ID_INDICATOR_SCRL   0xE723
#define ID_INDICATOR_EXT    0xE720
#define AFX_IDS_IDLEMESSAGE 0xE001
#endif

/* MFC RAII wait cursor — no-op */
class CWaitCursor { public: CWaitCursor() {} ~CWaitCursor() {} void Restore() {} };

/* Resource handle + LoadString backed by the PE resource loader (bob_resources.cpp). */
extern "C" void* bob_GetResourceHandle(void);
extern "C" void  bob_SetResourceHandle(void*);
extern "C" int   bob_load_string(void* h, unsigned id, char* buf, int maxlen);
static inline int AfxLoadString(UINT id, LPSTR buf, UINT max = 256) { return bob_load_string(bob_GetResourceHandle(), id, buf, (int)max); }
static inline HINSTANCE AfxGetResourceHandle() { return (HINSTANCE)bob_GetResourceHandle(); }
static inline void AfxSetResourceHandle(HINSTANCE h) { bob_SetResourceHandle((void*)h); }

/* The global application object (defined by IMPLEMENT'd CWinApp subclass in bob) */
extern CWinApp* AfxGetApp();
extern HINSTANCE AfxGetInstanceHandle();
extern CWnd* AfxGetMainWnd();
inline void AfxMessageBox(LPCSTR) {}

/* MFC worker-thread spawn. Single-thread bring-up: stubbed (no thread started);
   the periodic 3D draw loop is driven from the main loop instead. AFX_CDECL is
   the cdecl calling-convention tag (empty on Linux/gcc). */
#ifndef AFX_CDECL
#define AFX_CDECL
#endif
typedef UINT (AFX_CDECL *AFX_THREADPROC)(LPVOID);
/* Linux port: run the thread proc on a real (detached) pthread -- the per-view
   draw loop (View3d::drawloop) is spawned via this. See bob_threads.cpp. */
extern "C" void bob_begin_thread(unsigned int (*fn)(void*), void* arg);
inline CWinThread* AfxBeginThread(AFX_THREADPROC threadFn, LPVOID arg, int = 0, UINT = 0, DWORD = 0, void* = NULL) {
    bob_begin_thread((unsigned int(*)(void*))threadFn, arg);
    static CWinThread s_dummyThread;   /* callers only use the pointer as non-NULL */
    return &s_dummyThread;
}
inline CWinThread* AfxBeginThread(const void*, int = 0, UINT = 0, DWORD = 0, void* = NULL) { return NULL; }

/* ANSI/Unicode-neutral aliases bob calls without the A suffix */
#ifndef GetDlgItemText
#define GetDlgItemText GetDlgItemTextA
#endif

#endif /* FF_LINUX */
#endif /* FF_COMPAT_AFXWIN_H */
