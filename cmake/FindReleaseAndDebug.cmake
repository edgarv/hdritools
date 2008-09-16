# Small macro to find both relase and/or debug versions of the libraries
# It will define the variable {LIBPREFIX}_LIBRARY with the proper debug and/or
# release libraries. Be aware that mixing runtimes can be dangerous!
macro(FIND_RELEASE_AND_DEBUG LIBPREFIX 
      LIBNAME_RELEASE LIBNAME_DEBUG SEARCH_PATHS)

  find_library(${LIBPREFIX}_OPTIMIZED_LIBRARY 
    NAMES ${LIBNAME_RELEASE}
    PATHS ${SEARCH_PATHS}
    PATH_SUFFIXES lib
  )
  find_library(${LIBPREFIX}_DEBUG_LIBRARY 
    NAMES ${LIBNAME_DEBUG}
    PATHS ${SEARCH_PATHS}	
    PATH_SUFFIXES lib
  )
  
  # Sets up the found version: either release and or debug
  if(${LIBPREFIX}_OPTIMIZED_LIBRARY AND ${LIBPREFIX}_DEBUG_LIBRARY)
  # Use both debug and release versions if supported, otherwise default to release
    if (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
      set(${LIBPREFIX}_LIBRARY 
        optimized "${${LIBPREFIX}_OPTIMIZED_LIBRARY}"
        debug "${${LIBPREFIX}_DEBUG_LIBRARY}"
      )
    else(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
	  set(${LIBPREFIX}_LIBRARY "${${LIBPREFIX}_OPTIMIZED_LIBRARY}")
	endif(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
  elseif(${LIBPREFIX}_OPTIMIZED_LIBRARY)
    set(${LIBPREFIX}_LIBRARY "${${LIBPREFIX}_OPTIMIZED_LIBRARY}")
  elseif(${LIBPREFIX}_OPTIMIZED_LIBRARY)
    set(${LIBPREFIX}_LIBRARY "${${LIBPREFIX}_DEBUG_LIBRARY}")
  endif(${LIBPREFIX}_OPTIMIZED_LIBRARY AND ${LIBPREFIX}_DEBUG_LIBRARY)
  
  if(${LIBPREFIX}_LIBRARY)
    set(${LIBPREFIX}_LIBRARY ${${LIBPREFIX}_LIBRARY} 
      CACHE FILEPATH "The ${LIBPREFIX} library"
	  )
  endif(${LIBPREFIX}_LIBRARY)

  # We don't want to pollute the gui with non-user friendly entries
  mark_as_advanced(${LIBPREFIX}_LIBRARY
    ${LIBPREFIX}_OPTIMIZED_LIBRARY ${LIBPREFIX}_DEBUG_LIBRARY)
  
endmacro(FIND_RELEASE_AND_DEBUG)