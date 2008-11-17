// dllmain.cpp : Defines the entry point for the DLL application.



// To provide the automatic initialization of TBB: the DLL_PROCESS_*
// calls are done by the main thread, and we exactly want a scheduler
// for the main thread. GCC has equivalent tricks.

#include <cassert>
#include <tbb/task_scheduler_init.h>
using namespace tbb;

tbb::task_scheduler_init *tbbInit;

#ifdef WIN32

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>




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

#else /* WIN32 */


// Called when the library is loaded and before dlopen() returns
void __attribute__ ((constructor)) pcg_imageio_load(void)
{
    tbbInit = NULL;
	tbbInit = new task_scheduler_init;
	assert(tbbInit != NULL);
}

// Called when the library is unloaded and before dlclose()
// returns
void __attribute__ ((destructor)) pcg_imageio_unload(void)
{
    assert(tbbInit != NULL);
	delete tbbInit;
}


#endif /* WIN32 */

