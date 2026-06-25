/* bob_eventsink.cpp — OCX control event routing (FireClicked/Selected/TextChanged -> dialog handler).
 *
 * Adopted from the MiG Alley port's ma_eventsink (S33) to retire BoB's two targeted OCX bridges
 * (R5.3b SController combo-rebind + R4.4 CLoad file-row-click).
 *
 * Real MFC connects a control's events to the hosting dialog's ON_EVENT handlers via the eventsink
 * map + IConnectionPoint. On Linux the maps were no-ops, so a combo's TextChanged / a listbox's
 * Select went nowhere (hence the per-screen bridges). We rebuild it generally: the redefined
 * ON_EVENT macros (afxwin.h) register, for each (dialog-CLASS, control-id, event-dispid), a thunk
 * that casts the dialog and calls the (protected) handler. At fire time bob_evt_fire matches by
 * control-id + dispid + the dialog's RUNTIME type (typeid) — RTTI disambiguates the many dialogs
 * that reuse the same IDC_ ids. Trace with BOB_TRACE_OLE. */

#include <vector>
#include <typeinfo>
#include <stdio.h>
#include <stdlib.h>

/* event args set by the firing control before bob_evt_fire (read by the bob_evt_call thunks in
   afxwin.h for non-VTS_NONE handlers, e.g. OnSelectRlistboxfile(long,long) via A0/A1). */
extern "C" { long bob_evtA0 = 0, bob_evtA1 = 0; void* bob_evtP = 0; }

struct EvtEntry { const std::type_info* ti; int id; int dispid; void (*thunk)(void*); };
static std::vector<EvtEntry>& evtmap() { static std::vector<EvtEntry> v; return v; }

extern "C" void bob_evt_register(const void* tinfo, int id, int dispid, void (*thunk)(void*)) {
    EvtEntry e; e.ti = (const std::type_info*)tinfo; e.id = id; e.dispid = dispid; e.thunk = thunk;
    evtmap().push_back(e);
    if (getenv("BOB_TRACE_OLE")) { static int n=0; if(++n<=3||(n%100)==0) fprintf(stderr,"[evt_register] #%d id=%d dispid=%d type=%s\n", n, id, dispid, e.ti?e.ti->name():"?"); }
}

/* dlg = the dialog instance; tinfo = &typeid(*dlg) (passed by the caller, which has the concrete
   pointer). Call every handler whose class matches the dialog's runtime type. Returns #fired. */
extern "C" int bob_evt_fire(void* dlg, const void* tinfo, int id, int dispid) {
    const std::type_info* dt = (const std::type_info*)tinfo;
    std::vector<EvtEntry>& v = evtmap();
    int fired = 0;
    for (size_t i = 0; i < v.size(); i++) {
        if (v[i].id == id && v[i].dispid == dispid && v[i].ti && dt && *v[i].ti == *dt) {
            if (getenv("BOB_TRACE_OLE")) fprintf(stderr,"[evt_fire] id=%d dispid=%d type=%s -> HANDLER CALLED\n", id, dispid, dt->name());
            v[i].thunk(dlg); fired = 1;
        }
    }
    return fired;
}
