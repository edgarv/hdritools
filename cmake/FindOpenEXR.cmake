# - Find OpenEXR the library
# Find the native OpenEXR includes and libraries
# This module defines
#  OpenEXR_INCLUDE_DIR, where to find OpenEXR/ImfRgbaFile.h, etc.
#  OpenEXR_LIBRARIES, libraries to link against to use OpenEXR.
#  OpenEXR_FOUND, If false, do not try to use OpenEXR.



# TODO: For Linux the easiest option should be to use FindPkgConfig !!
# TODO: This is just a super hack! Implement this properly!

find_path(OpenEXR_INCLUDE_DIR ImfRgbaFile.h
  PATHS /usr/include
  PATHS /usr/include/OpenEXR
  )
mark_as_advanced(OpenEXR_INCLUDE_DIR)

# Finds the five openexr libraries libraries
include(FindReleaseAndDebug)
set(SEARCH_PATH /usr/lib64)

FIND_RELEASE_AND_DEBUG(HALF Half Halfd ${SEARCH_PATH})
FIND_RELEASE_AND_DEBUG(IEX Iex Iexd ${SEARCH_PATH})
FIND_RELEASE_AND_DEBUG(IMATH Imath Imathd ${SEARCH_PATH})
FIND_RELEASE_AND_DEBUG(ILMTHREAD IlmThread IlmThreadd ${SEARCH_PATH})
FIND_RELEASE_AND_DEBUG(ILMIMF IlmImf IlmImfd ${SEARCH_PATH})
 

# Sets the result
set(OpenEXR_FOUND FALSE)
if(OpenEXR_INCLUDE_DIR AND ILMIMF_LIBRARY AND
   HALF_LIBRARY AND IEX_LIBRARY AND IMATH_LIBRARY AND ILMTHREAD_LIBRARY)
  set(OpenEXR_FOUND TRUE)
endif(OpenEXR_INCLUDE_DIR AND ILMIMF_LIBRARY AND
   HALF_LIBRARY AND IEX_LIBRARY AND IMATH_LIBRARY AND ILMTHREAD_LIBRARY)
   
if(OpenEXR_FOUND)
  set(OpenEXR_FOUND ${OpenEXR_FOUND} PARENT_SCOPE)
  set(OpenEXR_LIBRARIES ${HALF_LIBRARY} ${IEX_LIBRARY}
    ${IMATH_LIBRARY} ${ILMTHREAD_LIBRARY} ${ILMIMF_LIBRARY} )
  if(NOT OpenEXR_FIND_QUIETLY)
    message(STATUS "Found OpenEXR.")
  endif(NOT OpenEXR_FIND_QUIETLY)
else(OpenEXR_FOUND)
  set(OpenEXR_PREFIX_PATH "${OpenEXR_PREFIX_PATH}-NOTFOUND" CACHE PATH 
    "Unknown OpenEXR base location." FORCE)
  if(OpenEXR_FIND_REQUIRED)
    message(FATAL_ERROR "OpenEXR not found!")
  endif(OpenEXR_FIND_REQUIRED)
endif(OpenEXR_FOUND)
   
