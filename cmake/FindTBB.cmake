# - Find TBB - (Intel Thread Building Blocks) library
# Find the native TBB includes and libraries
# This module defines
#  TBB_INCLUDE_DIR, where to find tbb/task.h, etc.
#  TBB_LIBRARIES, libraries to link against to use TBB.
#  TBB_FOUND, If false, do not try to use TBB.
#
# The script search at the default directories and
# at the base location pointed by TBB_PREFIX_PATH


# TODO: It should also set this:
# TBB_VERSION_MAJOR
# TBB_VERSION_MINOR
# TBB_VERSION_PATCH

find_path(TBB_INCLUDE_DIR tbb/task.h
  PATHS ${TBB_PREFIX_PATH}/include
        /usr/include
  )
mark_as_advanced(TBB_INCLUDE_DIR)
  
# Set the actual locations for MSVC
if(MSVC)
  if(CMAKE_CL_64)
    set(TBB_PLATFORM em64t)
  else(CMAKE_CL_64)
    set(TBB_PLATFORM ia32)
  endif(CMAKE_CL_64)
  
  if(MSVC71)
    set(TBB_COMPILER "vc7.1")
  elseif(MSVC80)
    set(TBB_COMPILER "vc8")
  elseif(MSVC90)
    set(TBB_COMPILER "vc9")
  else(MSVC90)
    message(SEND_ERROR "Unsupported/Unknown MSVC version.")
  endif(MSVC71)
  
  set(TBB_LIB_SUFFIX ${TBB_PLATFORM}/${TBB_COMPILER})
  
endif(MSVC)

# On linux the paths are slightly different
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  # To match the TBB makefile, find the output of uname -m (cmake provides uname -p)
  execute_process(COMMAND uname -m
    OUTPUT_VARIABLE UNAME_M
	ERROR_QUIET
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	
  if(${UNAME_M} STREQUAL "i686")
    set(TBB_PLATFORM ia32)
  elseif(${UNAME_M} STREQUAL "ia64")
    set(TBB_PLATFORM itanium)
  elseif(${UNAME_M} STREQUAL "x86_64")
    set(TBB_PLATFORM em64t)
  elseif(${UNAME_M} STREQUAL "sparc64")
	set(TBB_PLATFORM sparc)
  else(${UNAME_M} STREQUAL "i686")
    message(FATAL_ERROR "Unknown machine type: ${UNAME_M}")
  endif(${UNAME_M} STREQUAL "i686")
	
  # This works only with gcc so far
  execute_process(COMMAND pwd)
  execute_process(COMMAND ${CMAKE_HOME_DIRECTORY}/cmake/tbb_runtime.sh
    OUTPUT_VARIABLE TBB_RUNTIME
	ERROR_QUIET
	OUTPUT_STRIP_TRAILING_WHITESPACE)
	
  set(TBB_LIB_SUFFIX ${TBB_PLATFORM}/${TBB_RUNTIME})

elseif(APPLE)
  # Fixed path with the commercial-aligned binary release
  set(TBB_LIB_SUFFIX "ia32/cc4.0.1_os10.4.9")

endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

include(FindReleaseAndDebug)

# Tries to find the required libraries
FIND_RELEASE_AND_DEBUG(TBB tbb tbb_debug
  "${TBB_PREFIX_PATH}/${TBB_LIB_SUFFIX}" )
FIND_RELEASE_AND_DEBUG(TBBMALLOC tbbmalloc tbbmalloc_debug
  "${TBB_PREFIX_PATH}/${TBB_LIB_SUFFIX}")

# Set the results
if(TBB_INCLUDE_DIR AND TBB_LIBRARY AND TBBMALLOC_LIBRARY)
  set(TBB_FOUND TRUE)
else(TBB_INCLUDE_DIR AND TBB_LIBRARY AND TBBMALLOC_LIBRARY)
  set(TBB_FOUND FALSE)
endif(TBB_INCLUDE_DIR AND TBB_LIBRARY AND TBBMALLOC_LIBRARY)
  
if(TBB_FOUND)
  set(TBB_FOUND ${TBB_FOUND} PARENT_SCOPE)
  set(TBB_LIBRARIES ${TBB_LIBRARY} ${TBBMALLOC_LIBRARY} )
  if(NOT TBB_FIND_QUIETLY)
    message(STATUS "Found TBB.")
  endif(NOT TBB_FIND_QUIETLY)
else(TBB_FOUND)
  set(TBB_PREFIX_PATH "${TBB_PREFIX_PATH}-NOTFOUND" CACHE PATH "Unknown TBB base location." FORCE)
  if(TBB_FIND_REQUIRED)
    message(FATAL_ERROR "TBB not found!")
  endif(TBB_FIND_REQUIRED)
endif(TBB_FOUND)
