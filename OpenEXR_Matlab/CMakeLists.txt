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

    # In Linux and Mac the matlab binary found on the path should be
    # symlink of to the actual binary location
    if (UNIX)
      find_program(MATLAB_BIN matlab)
      find_program(READLINK_BIN readlink /bin /usr/bin /usr/local/bin /sbin)
      mark_as_advanced(MATLAB_BIN READLINK_BIN)
      if(MATLAB_BIN AND READLINK_BIN)
        execute_process(COMMAND "${READLINK_BIN}" "${MATLAB_BIN}"
          RESULT_VARIABLE out_ret
          OUTPUT_VARIABLE out_tmp
        )
        if(NOT out_ret)
          # At this point ${out_ret} should have the shape ${MATLABROOT}/bin/matlab,
          # add that root to the beginning of the search path
          get_filename_component(out_tmp "${out_tmp}" PATH)
          get_filename_component(out_tmp "${out_tmp}/../" ABSOLUTE)
          list(INSERT MATLABROOT 0 "${out_tmp}")
        endif()
      endif()
    endif() 

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
  # This variables are only used in gcc, but setting them here simplifies the checking
  set(MEX_MAPFILE "unused for MSVC")
  set(MEX_VERSION_SRC "unused for MSVC")
  
  # Avoid secure warnings with exr headers
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1 -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1)

elseif(UNIX)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Regular x86
    if(NOT APPLE)
      set(MATLAB_ARCH glnx86)
      set(MEX_EXT "mexglx")
    else(NOT APPLE)
      set(MATLAB_ARCH maci)
      set(MEX_EXT "mexmaci")
    endif(NOT APPLE)
  else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # AMD64 aka EMT64 aka Intel64
    if(NOT APPLE)
      set(MATLAB_ARCH glnxa64)
      set(MEX_EXT "mexa64")
    else(NOT APPLE)
      set(MATLAB_ARCH maci64)
      set(MEX_EXT "mexmaci64")
    endif(NOT APPLE)
  endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

  set(MATLABLIB bin/${MATLAB_ARCH})
  find_file(MEX_MAPFILE mexFunction.map
    PATHS ${MATLABROOT}
    PATH_SUFFIXES extern/lib/${MATLAB_ARCH}
    DOC "gcc version script for mex files"
  )

  # Also include the version information file
  find_path(MEX_VERSION_SRC "mexversion.c"
    PATHS ${MATLABROOT}
    PATH_SUFFIXES extern/src
    DOC "Matlab mex version source file"
  )
  mark_as_advanced(MEX_VERSION_SRC)

  # Whole set of linker flags
  if(NOT APPLE)
    set(MEX_LDFLAGS "-pthread -Wl,--version-script,${MEX_MAPFILE} -Wl,--no-undefined")
  else(NOT APPLE)
   set(MEX_LDFLAGS "-Wl,-flat_namespace -undefined suppress")
   set(MEX_LDFLAGS "${MEX_LDFLAGS} -Wl,-exported_symbols_list,${MEX_MAPFILE}")
   add_definitions(-DMX_COMPAT_32)
  endif(NOT APPLE)
else(MSVC)
  message(FATAL_ERROR "TODO: Add support for the ${CMAKE_SYSTEM_NAME} platform!")
endif(MSVC)

# The mac uses a different type of library target
if (NOT APPLE)
  set(MEX_TARGET_TYPE "SHARED")
else()
  set(MEX_TARGET_TYPE "MODULE")
endif()

# Find the basic mex libraries
find_library(MATLAB_MX_LIBRARY  NAMES mx libmx 
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
  NO_DEFAULT_PATH
)
find_library(MATLAB_MEX_LIBRARY NAMES mex libmex
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
  NO_DEFAULT_PATH
)
find_library(MATLAB_MAT_LIBRARY NAMES mat libmat
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
  NO_DEFAULT_PATH
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
  MEX_VERSION_SRC
)

set(MATLAB_LIBRARIES
  ${MATLAB_MX_LIBRARY}
  ${MATLAB_MEX_LIBRARY}
  ${MATLAB_MAT_LIBRARY}
)

# Super function to create mex projects. All the arguments to the macro are the source files.
# Note that rpath is always enabled for mex files (this is what Matlab does)
function(add_mex name)
  set(sources ${ARGV})
  list(REMOVE_AT sources 0)
  add_library(${name} ${MEX_TARGET_TYPE} ${sources})
  set_target_properties(${name} PROPERTIES
    SUFFIX ".${MEX_EXT}"
    PREFIX ""
    COMPILE_DEFINITIONS "MATLAB_MEX_FILE"
    LINK_FLAGS "${MEX_LDFLAGS}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
  )
  target_link_libraries(${name} ${MATLAB_LIBRARIES})
  include_directories(${MATLAB_INCLUDE_DIR})
endfunction(add_mex)

# For automatic multithreading support, find out a way to get the number of CPUs
# For non-msvc platforms try with the POSIX sysconf
if(NOT MSVC)
  include(CheckCSourceRuns)
  CHECK_C_SOURCE_RUNS(
    "#include <unistd.h>\n int main() { int n = sysconf(_SC_NPROCESSORS_ONLN); return 0;}\n" 
    SYSCONF_WORKS
  )
  if(SYSCONF_WORKS)
	add_definitions(-DUSE_SYSCONF=1)
  else(SYSCONF_WORKS)
    add_definitions(-DUSE_SYSCONF=0)
  endif(SYSCONF_WORKS)
else(NOT MSVC)
  add_definitions(-DUSE_SYSCONF=0)
endif(NOT MSVC)

# Creates the projects. Note that using init.cpp in Mac crashes Matlab
if (MSVC)
  set(COMMON_SRC init.cpp)
elseif(UNIX)
  if (NOT APPLE)
    set(COMMON_SRC init.cpp ${MEX_VERSION_SRC})
  else()
    set(COMMON_SRC ${MEX_VERSION_SRC})
  endif()
endif()

add_mex(exrread  exrread.cpp  ${COMMON_SRC})
add_mex(exrwrite exrwrite.cpp ${COMMON_SRC})


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
