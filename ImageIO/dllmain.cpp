// dllmain.cpp : Defines the entry point for the DLL application.

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <cassert>

// To provide the automatic initialization of TBB: the DLL_PROCESS_*
// calls are done by the main thread, and we exactly want a scheduler
// for the main thread
// TODO How does this work in Linux??

#include <tbb/task_scheduler_init.h>
using namespace tbb;

tbb::task_scheduler_init *tbbInit;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		tbbInit = NULL;
		tbbInit = new task_scheduler_init;
		assert(tbbInit != NULL);
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		assert(tbbInit != NULL);
		delete tbbInit;
		break;
	}
	return TRUE;
}

