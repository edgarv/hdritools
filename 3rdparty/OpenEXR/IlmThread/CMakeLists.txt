# Description of the IlmThread library

set(ILMTHREAD_HEADERS
  IlmThreadPool.h IlmThread.h
  IlmThreadSemaphore.h IlmThreadMutex.h
  )
  
set(ILMTHREAD_SOURCES
  IlmThreadPool.cpp IlmThread.cpp
  IlmThreadSemaphore.cpp IlmThreadMutex.cpp
  IlmThreadPosix.cpp IlmThreadSemaphorePosix.cpp
  IlmThreadSemaphorePosixCompat.cpp
  IlmThreadMutexPosix.cpp
  )
  
if(WIN32)
  list(APPEND ILMTHREAD_SOURCES
    IlmThreadMutexWin32.cpp
	IlmThreadSemaphoreWin32.cpp
	IlmThreadWin32.cpp
	)
endif(WIN32)
  

# Creates the special target  
ADD_OPENEXR_LIBRARY(IlmThread ILMTHREAD_HEADERS ILMTHREAD_SOURCES
  "IlmThread is a thread abstraction library for use with OpenEXR and other software packages."
  ${ILMBASE_VERSION} "2005")

# We might need to link it against pthreads. We use the variable declared in the parent CMakeLists.txt
if(HAVE_PTHREAD)
  target_link_libraries(IlmThread Iex ${CMAKE_THREAD_LIBS_INIT})
else(HAVE_PTHREAD)
  target_link_libraries(IlmThread Iex)
endif(HAVE_PTHREAD)

# Disable recurrent warnings in MSVC
MSVC_COMPILE_FLAGS(IlmThread "/wd4018 /wd4800 /wd4267")
