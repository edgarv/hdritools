# ============================================================================
#   HDRITools - High Dynamic Range Image Tools
#   Copyright 2008-2011 Program of Computer Graphics, Cornell University
#
#   Distributed under the OSI-approved MIT License (the "License");
#   see accompanying file LICENSE for details.
#
#   This software is distributed WITHOUT ANY WARRANTY; without even the
#   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#   See the License for more information.
#  ---------------------------------------------------------------------------
#  Primary author:
#      Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
# ============================================================================

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
      endif()
    endforeach()
    mark_as_advanced(MATLAB_BIN_DIR MATLABROOT)
  else()
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

  endif ()
endif()

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
  else()
    set(MATLABLIB extern/lib/win32/microsoft)
    set(MEX_EXT "mexw32")
  endif()

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
    else()
      set(MATLAB_OSX_ARCH i386)
      set(MATLAB_ARCH maci)
      set(MEX_EXT "mexmaci")
    endif()
  else()
    # AMD64 aka EMT64 aka Intel64
    if(NOT APPLE)
      set(MATLAB_ARCH glnxa64)
      set(MEX_EXT "mexa64")
    else()
      set(MATLAB_OSX_ARCH x86_64)
      set(MATLAB_ARCH maci64)
      set(MEX_EXT "mexmaci64")
    endif()
  endif()
  
  # Matlab doesn't support universal binaries. We'll build both architectures
  if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    foreach(osx_arch ${CMAKE_OSX_ARCHITECTURES})
      if (osx_arch STREQUAL "i386")
        set(MATLAB_MACI32 ON)
      elseif (osx_arch STREQUAL "x86_64")
        set(MATLAB_MACI64 ON)
      endif()
    endforeach()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT MATLAB_MACI64)
      message(AUTHOR_WARNING
        "Expecting a 64-bit build, but a 32-bit one was requested via CMAKE_OSX_ARCHITECTURES")
    endif()
    if (MATLAB_MACI32 AND MATLAB_MACI64)
      set(MATLAB_UNIVERSAL ON) # Where "Universal" indicates both architectures
      if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(MATLAB_OSX_ARCH_EXTRA x86_64)
        set(MATLAB_ARCH_EXTRA maci64)
        set(MEX_EXT_EXTRA     mexmaci64)
      else()
        set(MATLAB_OSX_ARCH_EXTRA i386)
        set(MATLAB_ARCH_EXTRA maci)
        set(MEX_EXT_EXTRA     mexmaci)
      endif()
    endif()
  endif()

  set(MATLABLIB bin/${MATLAB_ARCH})
  find_file(MEX_MAPFILE mexFunction.map
    PATHS ${MATLABROOT}
    PATH_SUFFIXES extern/lib/${MATLAB_ARCH}
    DOC "gcc version script for mex files"
  )
  if (MATLAB_UNIVERSAL)
    set(MATLABLIB_EXTRA bin/${MATLAB_ARCH_EXTRA})
    find_file(MEX_MAPFILE_EXTRA mexFunction.map
      PATHS ${MATLABROOT}
      PATH_SUFFIXES extern/lib/${MATLAB_ARCH_EXTRA}
      DOC "gcc version script for mex files (for extra architecture)"
    )
  endif()

  # Also include the version information file (this was removed on R2009a)
  find_path(MEX_VERSION_SRC "mexversion.c"
    PATHS ${MATLABROOT}
    PATH_SUFFIXES extern/src
    DOC "Matlab mex version source file"
  )
  mark_as_advanced(MEX_VERSION_SRC)

  # Whole set of linker flags
  if(NOT APPLE)
    set(MEX_LDFLAGS "-pthread -Wl,--version-script,${MEX_MAPFILE} -Wl,--no-undefined")
  else()
   set(MEX_LDFLAGS "-Wl,-twolevel_namespace -undefined error")
   set(MEX_LDFLAGS "${MEX_LDFLAGS} -Wl,-exported_symbols_list,${MEX_MAPFILE}")
   if (MATLAB_UNIVERSAL)
     set(MEX_LDFLAGS_EXTRA "-Wl,-twolevel_namespace -undefined error")
     set(MEX_LDFLAGS_EXTRA "${MEX_LDFLAGS_EXTRA} -Wl,-exported_symbols_list")
     set(MEX_LDFLAGS_EXTRA "${MEX_LDFLAGS_EXTRA},${MEX_MAPFILE_EXTRA}")
   endif()
  endif()
else()
  message(FATAL_ERROR "TODO: Add support for the ${CMAKE_SYSTEM_NAME} platform!")
endif()

# Type of library target
set(MEX_TARGET_TYPE "MODULE")

# Find the basic mex libraries
find_library(MATLAB_MX_LIBRARY  NAMES mx libmx 
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
  NO_DEFAULT_PATH)
find_library(MATLAB_MEX_LIBRARY NAMES mex libmex
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
  NO_DEFAULT_PATH)
find_library(MATLAB_MAT_LIBRARY NAMES mat libmat
  PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB} 
  NO_DEFAULT_PATH)
mark_as_advanced(MATLAB_MX_LIBRARY MATLAB_MEX_LIBRARY MATLAB_MAT_LIBRARY MEX_MAPFILE)

# Also find the extra stuff if building both architectures on the mac
if (MATLAB_UNIVERSAL)
  find_library(MATLAB_MX_LIBRARY_EXTRA  NAMES mx libmx 
    PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB_EXTRA} 
    NO_DEFAULT_PATH)
  find_library(MATLAB_MEX_LIBRARY_EXTRA NAMES mex libmex
    PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB_EXTRA} 
    NO_DEFAULT_PATH)
  find_library(MATLAB_MAT_LIBRARY_EXTRA NAMES mat libmat
    PATHS ${MATLABROOT} PATH_SUFFIXES ${MATLABLIB_EXTRA} 
    NO_DEFAULT_PATH)
  mark_as_advanced(MATLAB_MX_LIBRARY_EXTRA MATLAB_MEX_LIBRARY_EXTRA
    MATLAB_MAT_LIBRARY_EXTRA MEX_MAPFILE_EXTRA)
endif()

# Fail if the required stuff wasn't found
set(MATLAB_FIND_REQUIRED TRUE)
INCLUDE(FindPackageHandleStandardArgs)
if (NOT MATLAB_UNIVERSAL)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATLAB DEFAULT_MSG 
    MATLAB_INCLUDE_DIR
    MATLAB_MX_LIBRARY
    MATLAB_MEX_LIBRARY
    MATLAB_MAT_LIBRARY
    MEX_MAPFILE
  )
else()
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATLAB DEFAULT_MSG 
    MATLAB_INCLUDE_DIR
    MATLAB_MX_LIBRARY  MATLAB_MX_LIBRARY_EXTRA
    MATLAB_MEX_LIBRARY MATLAB_MEX_LIBRARY_EXTRA
    MATLAB_MAT_LIBRARY MATLAB_MAT_LIBRARY_EXTRA
    MEX_MAPFILE        MEX_MAPFILE_EXTRA
  )
endif()

set(MATLAB_LIBRARIES
  ${MATLAB_MX_LIBRARY} ${MATLAB_MEX_LIBRARY} ${MATLAB_MAT_LIBRARY}
)
if (MATLAB_UNIVERSAL)
  set(MATLAB_LIBRARIES_EXTRA
    ${MATLAB_MX_LIBRARY_EXTRA} ${MATLAB_MEX_LIBRARY_EXTRA}
    ${MATLAB_MAT_LIBRARY_EXTRA}
  )
endif()

# The include directory directive applies to all modules
include_directories(${MATLAB_INCLUDE_DIR} ${OpenEXR_INCLUDE_DIR})


# Super function to create mex projects. All the arguments to the macro are the source files.
# Note that rpath is always enabled for mex files (this is what Matlab does)
function(add_mex name)
  set(sources ${ARGV})
  list(REMOVE_AT sources 0)
  add_library(${name} ${MEX_TARGET_TYPE} ${sources})
  HDRITOOLS_LTCG(${name})
  set_target_properties(${name} PROPERTIES
    SUFFIX ".${MEX_EXT}"
    PREFIX ""
    COMPILE_DEFINITIONS "MATLAB_MEX_FILE"
    LINK_FLAGS "${MEX_LDFLAGS}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
    OSX_ARCHITECTURES "${MATLAB_OSX_ARCH}"
    FOLDER "Matlab-MEX"
  )
  target_link_libraries(${name} ${MATLAB_LIBRARIES})
  
  # Same set of things for the extra architexture
  if (MATLAB_UNIVERSAL)
    add_library(${name}_extra ${MEX_TARGET_TYPE} ${sources})
    HDRITOOLS_LTCG(${name}_extra)
    set_target_properties(${name}_extra PROPERTIES
      OUTPUT_NAME "${name}"
      SUFFIX ".${MEX_EXT_EXTRA}"
      PREFIX ""
      COMPILE_DEFINITIONS "MATLAB_MEX_FILE"
      LINK_FLAGS "${MEX_LDFLAGS_EXTRA}"
      INSTALL_RPATH_USE_LINK_PATH TRUE
      OSX_ARCHITECTURES "${MATLAB_OSX_ARCH_EXTRA}"
      FOLDER "Matlab-MEX"
    )
    target_link_libraries(${name}_extra ${MATLAB_LIBRARIES_EXTRA})
  endif()
endfunction()

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
  else()
    add_definitions(-DUSE_SYSCONF=0)
  endif()
else()
  add_definitions(-DUSE_SYSCONF=0)
endif()

# Creates the projects.
set(COMMON_SRC util.h util.cpp)
if(UNIX AND MEX_VERSION_SRC)
  list(APPEND COMMON_SRC ${MEX_VERSION_SRC})
endif()

# OpenEXR definitions (only used in windows dll anyway)
if(OpenEXR_DEFINITIONS)
  add_definitions(${OpenEXR_DEFINITIONS})
endif()


# Create the MEX modules
add_mex(exrread  exrread.cpp  ${COMMON_SRC})
add_mex(exrwrite exrwrite.cpp ${COMMON_SRC})
add_mex(exrinfo  exrinfo.cpp  ImfToMatlab.h ImfToMatlab.cpp ${COMMON_SRC})
add_mex(exrreadchannels exrreadchannels.cpp ${COMMON_SRC})
add_mex(exrwritechannels exrwritechannels.cpp 
  MatlabToImf.h MatlabToImf.cpp ImfToMatlab.h ${COMMON_SRC})


# Links them against OpenEXR
set(MATLAB_MEX_TARGETS exrread exrwrite exrinfo
  exrreadchannels exrwritechannels)
if (MATLAB_UNIVERSAL)
  set(mex_targets_extra "")
  foreach(mextarget ${MATLAB_MEX_TARGETS})
    list(APPEND mex_targets_extra "${mextarget}_extra")
  endforeach()
  list(APPEND MATLAB_MEX_TARGETS ${mex_targets_extra})
endif()
foreach(mextarget ${MATLAB_MEX_TARGETS})
  target_link_libraries(${mextarget} ${OpenEXR_LIBRARIES})
endforeach()


# And installs the mex binary and the .m file
install(TARGETS ${MATLAB_MEX_TARGETS}
  RUNTIME DESTINATION "${HDRITOOLS_SHARE_DIR}/matlab" COMPONENT "openexrmex"
  LIBRARY DESTINATION "${HDRITOOLS_SHARE_DIR}/matlab" COMPONENT "openexrmex"
  )
install(FILES exrread.m exrwrite.m exrinfo.m exrreadchannels.m exrwritechannels.m
  DESTINATION "${HDRITOOLS_SHARE_DIR}/matlab" COMPONENT "openexrmex"
  )
