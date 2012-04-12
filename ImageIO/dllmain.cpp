/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

// dllmain.cpp : Defines the entry point for the DLL application.



#include <cassert>
#include <tbb/task_scheduler_init.h>
using namespace tbb;

#if !defined(IMAGEIO_TBB_DEFAULT_INIT)

// To provide the automatic initialization of TBB: the DLL_PROCESS_*
// calls are done by the main thread, and we exactly want a scheduler
// for the main thread. GCC has equivalent tricks.
tbb::task_scheduler_init *tbbInit = NULL;

#endif // !IMAGEIO_TBB_DEFAULT_INIT


#if defined(_WIN32)

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
#if !defined(IMAGEIO_TBB_DEFAULT_INIT)
        tbbInit = NULL;
        tbbInit = new task_scheduler_init;
        assert(tbbInit != NULL);
#endif
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
#if !defined(IMAGEIO_TBB_DEFAULT_INIT)
        assert(tbbInit != NULL);
        delete tbbInit;
#endif
        break;
    }
    return TRUE;
}

#else /* _WIN32 */


// Called when the library is loaded and before dlopen() returns
void __attribute__ ((constructor)) pcg_imageio_load(void)
{
#if !defined(IMAGEIO_TBB_DEFAULT_INIT)
    tbbInit = NULL;
    tbbInit = new task_scheduler_init;
    assert(tbbInit != NULL);
#endif
}

// Called when the library is unloaded and before dlclose()
// returns
void __attribute__ ((destructor)) pcg_imageio_unload(void)
{
#if !defined(IMAGEIO_TBB_DEFAULT_INIT)
    assert(tbbInit != NULL);
    delete tbbInit;
#endif
}


#endif /* _WIN32 */
