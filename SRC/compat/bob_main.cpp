/* BoB Linux port - process entry point.
 *
 * On Windows the entry was MFC's WinMain -> AfxWinMain -> theApp.InitInstance().
 * The CWinApp subclass (CMIGApp) and its global instance live in the MFC module;
 * full runtime bring-up (SDL2 window + OpenGL present + OpenAL) is the next phase.
 * For now this provides the ELF entry so a `bob` binary links and starts. */
#ifdef FF_LINUX

#include <iostream>
#include <cstdio>
#include <unistd.h>   /* _exit */

/* Static-init-order fix: some game globals (e.g. the Lib3D object created via
   Inst3d::commonkeymaps' TU init) construct a std:: stream in their ctor, which
   copies the global std::locale. That locale is set up by std::ios_base::Init
   (the <iostream> static). Force that Init to run FIRST (highest init_priority)
   so it precedes every default-priority game global ctor — otherwise the stream
   ctor reads an uninitialised locale and segfaults. */
static std::ios_base::Init __bob_iostream_init __attribute__((init_priority(101)));

/* AfxWinMain is the MFC framework entry; it's stubbed in the compat layer for now.
   When the runtime loop is wired up this will call into theApp.InitInstance(). */
extern int AfxWinMain(void* hInstance, void* hPrevInstance, char* lpCmdLine, int nCmdShow);

int main(int argc, char** argv)
{
	(void)argc; (void)argv;
	fprintf(stderr,
		"Rowan's Battle of Britain - Linux native port\n"
		"  All %d source modules link. Runtime bring-up (SDL2/OpenGL/OpenAL)\n"
		"  is the next phase; the game loop is not yet driven.\n", 16);
	/* AfxWinMain(0, 0, (char*)"", 1);  -- enable once the runtime loop exists */

	/* Global dtors (e.g. Mast3d::~Mast3d -> Sound::ShutDownSound) assume their
	   subsystems were brought up by InitInstance(), which hasn't run yet, so
	   running them at exit derefs uninitialised DirectSound/3D state. Until the
	   runtime loop initialises those subsystems, skip C++ static teardown and
	   let the OS reclaim the process. */
	fflush(NULL);
	_exit(0);
}

#endif /* FF_LINUX */
