/* FreeFalcon Linux Port - afxole.h (MFC OLE/ActiveX support shim) */
#ifndef FF_STUB_afxole_h
#define FF_STUB_afxole_h

/* OLE drag-and-drop used by the R* controls' OnLButtonDown (football-player
   list drag). No drag-drop on Linux yet -> CacheGlobalData stores nothing and
   DoDragDrop reports "no effect". */
#ifndef BOB_OLE_DND
#define BOB_OLE_DND
typedef DWORD DROPEFFECT;
#ifndef DROPEFFECT_NONE
#define DROPEFFECT_NONE  0
#define DROPEFFECT_COPY  1
#define DROPEFFECT_MOVE  2
#define DROPEFFECT_LINK  4
#endif
class COleDataSource {
public:
    void CacheGlobalData(unsigned int, HGLOBAL, void* = NULL) {}
    void CacheData(unsigned int, void*, void* = NULL) {}
    DROPEFFECT DoDragDrop(DWORD = (DROPEFFECT_COPY|DROPEFFECT_MOVE),
                          LPCRECT = NULL, void* = NULL) { return DROPEFFECT_NONE; }
    void Empty() {}
};
class COleDataObject {
public:
    BOOL  IsDataAvailable(unsigned int) { return FALSE; }
    HGLOBAL GetGlobalData(unsigned int) { return NULL; }
    BOOL  AttachClipboard() { return FALSE; }
};
class COleDropTarget {
public:
    BOOL Register(CWnd*) { return TRUE; }
    void Revoke() {}
};
#endif /* BOB_OLE_DND */

#endif
