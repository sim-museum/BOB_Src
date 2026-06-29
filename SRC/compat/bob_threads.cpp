/* BoB Linux port - threads + multimedia timer backend (Phase 2c).
 *
 * The compat layer already has a real pthread-backed handle/event system
 * (compat_winbase.h: CreateThread/CreateEvent/SetEvent/WaitForSingleObject).
 * Two MFC/MM entry points were still stubbed and gate the render loop:
 *   - AfxBeginThread  -> the per-view draw thread (View3d::drawloop) never ran.
 *   - timeSetEvent    -> the periodic "move cycle" (Mast3d::StaticTimeProc ->
 *                        TimeProc) never fired.
 * Both are implemented here over pthreads. The Win32 event handshake the draw
 * thread/move-cycle use (doneframe etc.) is already real in compat_winbase.h.
 */
#ifdef FF_LINUX

#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <execinfo.h>

/* ---- S46 diagnostic: trace what drives a view's drawing state to D_CLOSE ---
 * The per-view draw thread (View3d::drawloop) returns permanently when
 * drawing==D_CLOSE; if nothing re-spawns it the window freezes ("hang"). This
 * env-gated (BOB_TRACE_DRAW) helper prints a backtrace at each D_CLOSE setter
 * site (STUB3D.CPP) plus draw-thread start/exit below, so a native repro names
 * the exact culprit without gdb perturbing the race. Default-off, no-op unless
 * BOB_TRACE_DRAW is set. */
extern "C" void bob_trace_draw(const char* where)
{
	if (!getenv("BOB_TRACE_DRAW")) return;
	fprintf(stderr, "[draw] %s (tid=%lu)\n", where, (unsigned long)pthread_self());
	void* bt[32];
	int n = backtrace(bt, 32);
	backtrace_symbols_fd(bt, n, 2 /*stderr*/);
	fflush(stderr);
}

/* ---- AfxBeginThread: run an MFC AFX_THREADPROC on a detached pthread ----- */
typedef unsigned int (*bob_threadproc)(void*);

struct ThreadArg { bob_threadproc proc; void* arg; };
static void* thread_trampoline(void* p) {
	ThreadArg a = *(ThreadArg*)p; free(p);
	bob_trace_draw("thread_trampoline: ENTER (draw/worker thread start)");
	if (a.proc) a.proc(a.arg);
	bob_trace_draw("thread_trampoline: EXIT (thread returned -- e.g. drawloop saw D_CLOSE)");
	/* S46: AfxBeginThread's only caller is View3d::drawloop (the flight draw
	 * thread), so reaching here means flight ended (View3d::drawloop saw
	 * D_CLOSE -- e.g. the pilot pressed F12/KEY_CONFIGMENU or Alt+X/EXITKEY).
	 * In the BOB_BOOT_FRONTEND boot-to-flight scaffold there is NO front-end
	 * menu to return to, so the window would otherwise freeze on the last
	 * frame (CMIGApp::Run keeps pumping with nothing to draw). Exit cleanly
	 * instead -- mirrors the SDL_QUIT path (bob_video.cpp). NOT done for the
	 * real front-end (BOB_FRONTEND), where closing flight returns to the menu
	 * and a new view/draw thread is created for the next mission. */
	if (getenv("BOB_BOOT_FRONTEND")) {
		fprintf(stderr, "[draw] flight draw thread ended in boot-to-flight scaffold "
		                "(no menu to return to) -> clean exit\n");
		fflush(stderr);
		_exit(0);
	}
	return NULL;
}
extern "C" void bob_begin_thread(bob_threadproc proc, void* arg)
{
	if (!proc) return;
	ThreadArg* a = (ThreadArg*)malloc(sizeof(ThreadArg));
	a->proc = proc; a->arg = arg;
	pthread_t t;
	if (pthread_create(&t, NULL, thread_trampoline, a) == 0)
		pthread_detach(t);          /* draw loops run until their view closes */
	else
		free(a);
}

/* ---- timeSetEvent: a periodic (or one-shot) multimedia timer ------------- */
typedef void (*bob_timecb)(unsigned, unsigned, unsigned long, unsigned long, unsigned long);

struct Timer {
	bob_timecb    cb;
	unsigned long user;
	unsigned      delayMs;
	int           periodic;
	volatile int  kill;
	pthread_t     th;
	int           inUse;
};
#define BOB_MAX_TIMERS 32
static Timer g_timers[BOB_MAX_TIMERS];
static pthread_mutex_t g_timerLock = PTHREAD_MUTEX_INITIALIZER;

static void* timer_thread(void* p)
{
	Timer* t = (Timer*)p;
	do {
		/* sleep in small slices so timeKillEvent is responsive */
		unsigned remaining = t->delayMs ? t->delayMs : 1;
		while (remaining && !t->kill) {
			unsigned slice = remaining > 5 ? 5 : remaining;
			usleep(slice * 1000);
			remaining -= slice;
		}
		if (t->kill) break;
		if (t->cb) t->cb((unsigned)(t - g_timers) + 1, 0, t->user, 0, 0);
	} while (t->periodic && !t->kill);
	pthread_mutex_lock(&g_timerLock);
	t->inUse = 0;
	pthread_mutex_unlock(&g_timerLock);
	return NULL;
}

/* fdwTimer: TIME_ONESHOT=0, TIME_PERIODIC=1 (bit 0) */
extern "C" unsigned int bob_time_set_event(unsigned delayMs, unsigned /*res*/,
		bob_timecb cb, unsigned long user, unsigned fdwTimer)
{
	if (getenv("BOB_NO_TIMER")) return 1;   /* diagnostic: no real timer thread */
	pthread_mutex_lock(&g_timerLock);
	int slot = -1;
	for (int i = 0; i < BOB_MAX_TIMERS; i++) if (!g_timers[i].inUse) { slot = i; break; }
	if (slot < 0) { pthread_mutex_unlock(&g_timerLock); return 0; }
	Timer* t = &g_timers[slot];
	memset(t, 0, sizeof(*t));
	t->cb = cb; t->user = user; t->delayMs = delayMs;
	t->periodic = (fdwTimer & 1) ? 1 : 0;
	t->inUse = 1; t->kill = 0;
	if (pthread_create(&t->th, NULL, timer_thread, t) != 0) { t->inUse = 0; pthread_mutex_unlock(&g_timerLock); return 0; }
	pthread_detach(t->th);
	pthread_mutex_unlock(&g_timerLock);
	return (unsigned)slot + 1;          /* timer id (non-zero) */
}

extern "C" unsigned int bob_time_kill_event(unsigned id)
{
	if (id == 0 || id > BOB_MAX_TIMERS) return 0;
	Timer* t = &g_timers[id - 1];
	t->kill = 1;                        /* the timer thread frees its slot */
	return 0;                           /* MMSYSERR_NOERROR */
}

#endif /* FF_LINUX */
