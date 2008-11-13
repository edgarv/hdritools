# Test for checking for openexrdll
# Under windows, if OpenEXR was compiled as a DLL,  this test will fail because of linking errors.
#include(CheckCXXSourceCompiles)
#set(CMAKE_REQUIRED_INCLUDES "${OpenEXR_INCLUDE_DIR}/OpenEXR")
#set(CMAKE_REQUIRED_LIBRARIES ${OpenEXR_LIBRARIES})
#CHECK_CXX_SOURCE_COMPILES("#include <half.h>\nint main(){ half h1=1.0f;float f1=h1-1.0f;return 0;}\n" OpenEXR_WIN_STATIC)


# The included FindMatlab.cmake script as of Cmake 2.6.2 only works for Matlab 7.0 (at least on Windows)

# Sets a default for MATLABROOT if it is not there. It looks for Matlab 7.x in the registry
if(NOT MATLABROOT)
  foreach(m_minor RANGE 9 0 -1)
    get_filename_component( THE_PATH 
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MathWorks\\MATLAB\\7.${m_minor};MATLABROOT]" ABSOLUTE)
    if(EXISTS ${THE_PATH})
      message(STATUS "Found Matlab 7.${m_minor} at ${THE_PATH}")
	  SET(MATLABROOT ${THE_PATH} CACHE PATH "Matlab 7.x instalation directory.")
	  break()
    endif(EXISTS ${THE_PATH})
  endforeach()
endif(NOT MATLABROOT)

# Directory of the includes
find_path(MATLAB_INCLUDE_DIR "mex.h"
  "${MATLABROOT}/extern/include"
  )
mark_as_advanced(MATLAB_INCLUDE_DIR)
  
# Find the linker libraries
if(MSVC)
  if(CMAKE_CL_64)
    set(MATLABLIB "${MATLABROOT}/extern/lib/win64/microsoft")
	set(MEX_EXT "mexw64")
  else(CMAKE_CL_64)
    set(MATLABLIB "${MATLABROOT}/extern/lib/win32/microsoft")
	set(MEX_EXT "mexw32")
  endif(CMAKE_CL_64)
else(MSVC)
  message(FATAL_ERROR "This script only supports MSVC for now")
endif(MSVC)

find_library(MATLAB_MX_LIBRARY  "libmx"  ${MATLABLIB} )
find_library(MATLAB_MEX_LIBRARY "libmex" ${MATLABLIB} )
find_library(MATLAB_MAT_LIBRARY "libmat" ${MATLABLIB} )
mark_as_advanced(MATLAB_MX_LIBRARY MATLAB_MEX_LIBRARY MATLAB_MAT_LIBRARY)

set(MATLAB_LIBRARIES
  ${MATLAB_MX_LIBRARY}
  ${MATLAB_MEX_LIBRARY}
  ${MATLAB_ENG_LIBRARY}
)

# Super macro to create mex projects. All the arguments to the macro are the source files
macro(add_mex name sources)
  add_library(${name} SHARED ${sources})
  set_target_properties(${name} PROPERTIES
    SUFFIX ".${MEX_EXT}"
	COMPILE_DEFINITIONS "MATLAB_MEX_FILE"
	LINK_FLAGS "/export:mexFunction"
    )
  target_link_libraries(${name} ${MATLAB_LIBRARIES})
  include_directories(${MATLAB_INCLUDE_DIR})
endmacro(add_mex)

# Creates the projects
add_mex(exrread  exrread.cpp)
add_mex(exrwrite exrwrite.cpp)


# Links them against OpenEXR
target_link_libraries(exrread ${OpenEXR_LIBRARIES})
target_link_libraries(exrwrite ${OpenEXR_LIBRARIES})
include_directories(${OpenEXR_INCLUDE_DIR})

# And installs the mex binary and the .m file
install(TARGETS exrread exrwrite
  RUNTIME DESTINATION "matlab"
  LIBRARY DESTINATION "matlab"
  )
install(FILES exrread.m exrwrite.m
  DESTINATION "matlab"
  )
