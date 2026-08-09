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
    /* S160: report a fire that found NO handler, and say what IS registered for that id. A miss is
       the symptom of both known traps and they need opposite fixes, so the trace must distinguish
       them rather than leave it to argument:
         - §8z    : handler registered on a BASE class; exact type match here can never reach it.
         - §8-BoB155: we fired under the PANEL wrapper's type (RDEmptyD/...) while the handler is
                      registered on the contained dialog (CSQuick1/...).
       Printing the fired type next to the registered types tells you which in one line.
       Deduped per (id,dispid,type) — BOB_TRACE_OLE is per-control-per-frame and once wrote 70 MB. */
    if (!fired && getenv("BOB_TRACE_EVT")) {
        static std::vector<long> seen;
        long key = ((long)id << 20) ^ ((long)dispid << 8) ^ (long)(size_t)dt;
        bool dup = false;
        for (size_t k = 0; k < seen.size(); k++) if (seen[k] == key) { dup = true; break; }
        if (!dup && seen.size() < 200) {
            seen.push_back(key);
            fprintf(stderr, "[evt_miss] id=%d dispid=%d fired-as=%s -- NO HANDLER",
                    id, dispid, dt ? dt->name() : "?");
            int shown = 0;
            for (size_t i = 0; i < v.size(); i++)
                if (v[i].id == id && v[i].dispid == dispid && v[i].ti && shown < 4) {
                    fprintf(stderr, "%s registered-on=%s", shown ? "," : "; ", v[i].ti->name());
                    shown++;
                }
            if (!shown) fprintf(stderr, "; nothing registered for this id/dispid");
            fprintf(stderr, "\n");
        }
    }
    return fired;
}
