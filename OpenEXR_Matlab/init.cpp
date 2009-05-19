/** Initialization file, will be called upon loading the mex module */

/* First part: includes */
#if defined( WIN32 )
  #define WINDOWS_LEAN_AND_MEAN
  #include <windows.h>
#elif USE_SYSCONF
  #include <unistd.h>
#endif

#include <ImfThreading.h>

/* Second part: function to get the number of CPUs */
int get_num_cpus()
{
#if defined( WIN32 )
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	const int n = (int)info.dwNumberOfProcessors;
	return n > 0 ? n : 1;
#elif USE_SYSCONF
	const int n = sysconf(_SC_NPROCESSORS_ONLN);
	return n > 0 ? n : 1;
#else
	// Nothing better
	return 1;
#endif

}

/* Third part: Common function call to do the initialization */
inline void setImfThreads()
{
	const int n = get_num_cpus();
	Imf::setGlobalThreadCount(n);
}


/* Last part: actual shared library initialization */
#ifdef WIN32

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		setImfThreads();
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


#else

// Called when the library is loaded and before dlopen() returns
void __attribute__ ((constructor)) mex_openexr_load(void)
{
    setImfThreads();
}

#endif
