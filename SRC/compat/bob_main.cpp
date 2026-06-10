/* BoB Linux port - process entry point.
 *
 * On Windows the entry was MFC's WinMain -> AfxWinMain -> theApp.InitInstance().
 * The CWinApp subclass (CMIGApp) and its global instance live in the MFC module;
 * full runtime bring-up (SDL2 window + OpenGL present + OpenAL) is the next phase.
 * For now this provides the ELF entry so a `bob` binary links and starts. */
#ifdef FF_LINUX

#include <cstdio>

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
	return 0;
}

#endif /* FF_LINUX */
