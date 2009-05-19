# Test for checking for openexrdll
# Under windows, if OpenEXR was compiled as a DLL,  this test will fail because of linking errors.
#include(CheckCXXSourceCompiles)
#set(CMAKE_REQUIRED_INCLUDES "${OpenEXR_INCLUDE_DIR}/OpenEXR")
#set(CMAKE_REQUIRED_LIBRARIES ${OpenEXR_LIBRARIES})
#CHECK_CXX_SOURCE_COMPILES("#include <half.h>\nint main(){ half h1=1.0f;float f1=h1-1.0f;return 0;}\n" OpenEXR_WIN_STATIC)


# The included FindMatlab.cmake script as of Cmake 2.6.2 only works for Matlab 7.0 (at least on Windows)

# Sets a default for MATLABROOT if it is not there. It looks for Matlab 7.x in the registry
if(NOT MATLABROOT)
  if (WIN32)
    foreach(m_minor RANGE 20 0 -1)
	  # get_filename_component doesn't handle the Wow6432Node registry issues, but find_* do
	  find_path(MATLAB_BIN_DIR "matlab.exe" 
	    PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MathWorks\\MATLAB\\7.${m_minor};MATLABROOT]" 
		PATH_SUFFIXES "bin" 
		DOC "Matlab binaries directory for Windows")
      if(MATLAB_BIN_DIR)
	    # The root directory is just the parent of this path
		get_filename_component(MATLABROOT "${MATLAB_BIN_DIR}/../" ABSOLUTE CACHE)
        message(STATUS "Found Matlab 7.${m_minor} at ${MATLABROOT}")
	    break()
      endif(MATLAB_BIN_DIR)
    endforeach()
	mark_as_advanced(MATLAB_BIN_DIR MATLABROOT)
  else(WIN32)
    # Standard Unix directories
    set(MATLABROOT
      /usr/local/matlab-7sp1/
      /opt/matlab-7sp1/
      $ENV{HOME}/matlab-7sp1/
      $ENV{HOME}/redhat-matlab/
      /opt/matlab/
      /usr/local/matlab/
      $ENV{MATLABROOT}
    )
  endif (WIN32)
endif(NOT MATLABROOT)

# Directory of the includes
find_path(MATLAB_INCLUDE_DIR "mex.h"
  PATHS ${MATLABROOT}
  PATH_SUFFIXES extern/include
  DOC "Matlab mex include file"
  )
mark_as_advanced(MATLAB_INCLUDE_DIR)

# Set the appropriate suffixes according to the platform
if(MSVC)
  if(CMAKE_CL_64)
    set(MATLABLIB extern/lib/win64/microsoft)
    set(MEX_EXT "mexw64")
  else(CMAKE_CL_64)
    set(MATLABLIB extern/lib/win32/microsoft)
    set(MEX_EXT "mexw32")
  endif(CMAKE_CL_64)

  # Linker settings: it just avoids dll_export stuff
  set(MEX_LDFLAGS /export:mexFunction)
  # This variable is only used in gcc, but setting it here simplifies the checking
  set(MEX_MAPFILE "unused for MSVC")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Regular x86
    set(MATLAB_ARCH glnx86)
    set(MEX_EXT "mexglx")
  else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # AMD64 aka EMT64 aka Intel64
    set(MATLAB_ARCH glnxa64)
    set(MEX_EXT "mexa64")
  endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

  set(MATLABLIB bin/${MATLAB_ARCH})
  find_file(MEX_MAPFILE mexFunction.map
    PATHS ${MATLABROOT}
    PATH_SUFFIXES extern/lib/${MATLAB_ARCH}
    DOC "gcc version script for mex files"
  )

  # Whole set of linker flags
  set(MEX_LDFLAGS "-pthread -Wl,--version-script,${MEX_MAPFILE} -Wl,--no-undefined")
else(MSVC)
  message(FATAL_ERROR "TODO: Add support for the ${CMAKE_SYSTEM_NAME} platform!")
endif(MSVC)

# Find the basic mex libraries
find_library(MATLAB_MX_LIBRARY  NAMES mx libmx 
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
)
find_library(MATLAB_MEX_LIBRARY NAMES mex libmex
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
)
find_library(MATLAB_MAT_LIBRARY NAMES mat libmat
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
)
mark_as_advanced(MATLAB_MX_LIBRARY MATLAB_MEX_LIBRARY MATLAB_MAT_LIBRARY MEX_MAPFILE)

# Fail if the required stuff wasn't found
set(MATLAB_FIND_REQUIRED TRUE)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATLAB DEFAULT_MSG 
  MATLAB_INCLUDE_DIR
  MATLAB_MX_LIBRARY
  MATLAB_MEX_LIBRARY
  MATLAB_MAT_LIBRARY
  MEX_MAPFILE
)

set(MATLAB_LIBRARIES
  ${MATLAB_MX_LIBRARY}
  ${MATLAB_MEX_LIBRARY}
  ${MATLAB_MAT_LIBRARY}
)

# Super macro to create mex projects. All the arguments to the macro are the source files.
# Note that rpath is always enabled for mex files (this is what Matlab does)
macro(add_mex name sources)
  add_library(${name} SHARED ${sources})
  set_target_properties(${name} PROPERTIES
    SUFFIX ".${MEX_EXT}"
    PREFIX ""
    COMPILE_DEFINITIONS "MATLAB_MEX_FILE"
    LINK_FLAGS "${MEX_LDFLAGS}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
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
