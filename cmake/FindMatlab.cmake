# - Find Matlab
# Find Matlab and the required libraries to build MEX files.
# Once done this will define
#
#  MATLAB_INCLUDE_DIRS    - where to find mex.h, engine.h, etc.
#  MATLAB_LIBRARIES       - required libraries for MEX files: libmex, etc.
#  MATLAB_MEX_EXT         - extension (without the dot) for the mex files
#
#  MATLAB_UNIVERSAL       - defined only in OSX, indicates the system is
#                           set up for building both 64-bit and 32-binaries.
#                           Matlab MEX files cannot be universal binaries,
#                           Furthermore support for 32-bit Matlab on OSX
#                           ended in 2011.
#
# Only defined in Unix
#  MATLAB_MEX_MAPFILE     - gcc version script for MEX files.
#  MATLAB_MEX_VERSION_SRC - source file for the MEX version information.
#                           This file was removed in Matlab R2009a.
#
# The following variables are defined if and only if MATLAB_UNIVERSAL is
# defined and set to ON
#
#  MATLAB_LIBRARIES_EXTRA - required libraries for MEX files for the
#                           non-primary OSX architechture.
#  MATLAB_MEX_EXT_EXTRA   - extension (without the dot) for the mex files
#                           for the non-primary OSX architecture.
#  MATLAB_MEX_MAPFILE_EXTRA - gcc version script for the non primar arch.
#
# An includer may set MATLAB_ROOT to a Matlab installation root to tell
# this module where to look.
#
# ============================================================================
#   HDRITools - High Dynamic Range Image Tools
#   Copyright 2008-2012 Program of Computer Graphics, Cornell University
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

#
# Tweaked FindMatlab module based on the provided with CMake 2.6.2
#

include(FindPackageHandleStandardArgs)

if(NOT DEFINED _MATLAB_DEBUG)
  set(_MATLAB_DEBUG OFF)
endif()

macro(_MATLAB_DBG_MSG)
  if(_MATLAB_DEBUG)
    message(${ARGN})
  endif()
endmacro()



# Use multiple search paths as done in FindZLIB as of CMake 2.8.7
set(_MATLAB_SEARCHES)

# Use MATLAB_ROOT first, without default paths, if set
if(MATLAB_ROOT)
  set(_MATLAB_SEARCH_ROOT PATHS ${MATLAB_ROOT} NO_DEFAULT_PATH)
  list(APPEND _MATLAB_SEARCHES _MATLAB_SEARCH_ROOT)
endif()

# Prepare the predefined paths
if (WIN32)
  # FIXME This should query only up to the current year
  set(_MATLAB_REGKEYS)
  foreach(m_minor RANGE 31 0 -1)
    list(APPEND _MATLAB_REGKEYS
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MathWorks\\MATLAB\\7.${m_minor};MATLABROOT]" 
    )
  endforeach()
  
  # get_filename_component doesn't handle the Wow6432Node registry issues, but find_* do
  find_path(MATLAB_BIN_DIR "matlab.exe" 
    HINTS ${_MATLAB_REGKEYS}
    PATH_SUFFIXES "bin" 
    DOC "Matlab binaries directory for Windows")
  if(MATLAB_BIN_DIR)
    # The root directory is just the parent of this path
    get_filename_component(_MATLAB_WIN_ROOT "${MATLAB_BIN_DIR}/../" ABSOLUTE)
    set(_MATLAB_SEARCH_WIN HINTS ${_MATLAB_WIN_ROOT})
    list(APPEND _MATLAB_SEARCHES _MATLAB_SEARCH_WIN)
  endif()
    
  mark_as_advanced(MATLAB_BIN_DIR)
else()
  set(_MATLAB_UNIX_PATHS)
  # TODO This should query only up to the current year
  foreach(m_year RANGE 2020 2006 -1)
    if(APPLE)
      list(APPEND _MATLAB_UNIX_PATHS
        "/Applications/MATLAB_R${m_year}b.app"
        "/Applications/MATLAB_R${m_year}a.app"
      )
    endif()
    list(APPEND _MATLAB_UNIX_PATHS
      "/opt/MATLAB/R${m_year}b"
      "/opt/MATLAB/R${m_year}a"
    )
  endforeach()

  # Old Standard Unix directories
  list(APPEND _MATLAB_UNIX_PATHS
    /usr/local/matlab-7sp1/
    /opt/matlab-7sp1/
    $ENV{HOME}/matlab-7sp1/
    $ENV{HOME}/redhat-matlab/
    /opt/matlab/
    /usr/local/matlab/
    $ENV{MATLABROOT}
  )

  # In Linux and Mac the matlab binary found on the path should be
  # symlink of to the actual binary location. We try to use first
  # the default matlab and then use the guesses
  find_program(MATLAB_BIN matlab)
  find_program(MATLAB_BIN matlab HINTS ${_MATLAB_UNIX_PATHS}
    PATH_SUFFIXES "bin" NO_DEFAULT_PATH)
  if (MATLAB_BIN)
    _MATLAB_DBG_MSG("Found matlab binary: \"${MATLAB_BIN}\"")
    find_program(READLINK_BIN readlink PATHS /bin /usr/bin /usr/local/bin /sbin)
    mark_as_advanced(MATLAB_BIN READLINK_BIN)
    if(READLINK_BIN)
      execute_process(COMMAND "${READLINK_BIN}" "${MATLAB_BIN}"
        RESULT_VARIABLE out_ret OUTPUT_VARIABLE out_tmp)
      if(NOT out_ret)
        # At this point ${out_ret} should have the shape ${MATLABROOT}/bin/matlab
        get_filename_component(out_tmp "${out_tmp}" PATH)
        get_filename_component(out_tmp "${out_tmp}/../" ABSOLUTE)
        set(_MATLAB_SEARCH_UNIX HINTS ${out_tmp})
        list(APPEND _MATLAB_SEARCHES _MATLAB_SEARCH_UNIX)
      endif()
    endif()

    if(NOT _MATLAB_SEARCH_UNIX)
      # Assume MATLAB_BIN has the shape ${MATLAB_ROOT}/bin/matlab
      # when it was not possible to read the symlink
      get_filename_component(_MATLAB_BIN_DIR "${MATLAB_BIN}" PATH)
      get_filename_component(_MATLAB_UNIX_ROOT "${_MATLAB_BIN_DIR}/../" ABSOLUTE)
      set(_MATLAB_SEARCH_UNIX HINTS ${_MATLAB_UNIX_ROOT})
      list(APPEND _MATLAB_SEARCHES _MATLAB_SEARCH_UNIX)
    endif()

  endif()
endif ()


# Set the appropriate suffixes according to the platform
if(MSVC)
  if(CMAKE_CL_64)
    set(_MATLABLIB extern/lib/win64/microsoft)
    set(MATLAB_MEX_EXT "mexw64")
  else()
    set(_MATLABLIB extern/lib/win32/microsoft)
    set(MATLAB_MEX_EXT "mexw32")
  endif()
  
elseif(UNIX)

  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # Regular x86
    if(NOT APPLE)
      set(_MATLAB_ARCH glnx86)
      set(MATLAB_MEX_EXT "mexglx")
    else()
      set(MATLAB_OSX_ARCH i386)
      set(_MATLAB_ARCH maci)
      set(MATLAB_MEX_EXT "mexmaci")
    endif()
  else()
    # AMD64 aka EMT64 aka Intel64
    if(NOT APPLE)
      set(_MATLAB_ARCH glnxa64)
      set(MATLAB_MEX_EXT "mexa64")
    else()
      set(MATLAB_OSX_ARCH x86_64)
      set(_MATLAB_ARCH maci64)
      set(MATLAB_MEX_EXT "mexmaci64")
    endif()
  endif()
  
  set(_MATLABLIB bin/${_MATLAB_ARCH})
  
  # Matlab doesn't support universal binaries. We'll build both architectures.
  # Note that as of Matlab 2011, 32-bit Macs are no longer supported.
  if (APPLE)
    set(MATLAB_UNIVERSAL OFF)
  endif()
  if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    foreach(osx_arch ${CMAKE_OSX_ARCHITECTURES})
      if (osx_arch STREQUAL "i386")
        set(_MATLAB_MACI32 ON)
      elseif (osx_arch STREQUAL "x86_64")
        set(_MATLAB_MACI64 ON)
      endif()
    endforeach()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT _MATLAB_MACI64)
      message(AUTHOR_WARNING
        "Expecting a 64-bit build, but a 32-bit one was requested via CMAKE_OSX_ARCHITECTURES")
    endif()
    if (_MATLAB_MACI32 AND _MATLAB_MACI64)
      set(MATLAB_UNIVERSAL ON) # Where "Universal" indicates both architectures
      if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(MATLAB_OSX_ARCH_EXTRA x86_64)
        set(_MATLAB_ARCH_EXTRA maci64)
        set(MATLAB_MEX_EXT_EXTRA     mexmaci64)
      else()
        set(MATLAB_OSX_ARCH_EXTRA i386)
        set(_MATLAB_ARCH_EXTRA maci)
        set(MATLAB_MEX_EXT_EXTRA     mexmaci)
      endif()
    endif()
  endif()

else()
  message(FATAL_ERROR "TODO: Add support for the ${CMAKE_SYSTEM_NAME} platform!")
endif()

_MATLAB_DBG_MSG("Matlab mex extension: \"${MATLAB_MEX_EXT}\"")



if(_MATLAB_DEBUG)
  message("Matlab search variables: ${_MATLAB_SEARCHES}")
  foreach(search ${_MATLAB_SEARCHES})
    message("  ${search}: ${${search}}")
  endforeach()
endif()



set(_MATLAB_EXTRA_VARS)

# Try each search configuration.
foreach(search ${_MATLAB_SEARCHES})
  # Directory of the includes
  find_path(MATLAB_INCLUDE_DIR "mex.h" ${${search}}
    PATH_SUFFIXES extern/include
    DOC "Matlab mex include file")
    
   # Find the basic mex libraries
  find_library(MATLAB_MX_LIBRARY  NAMES mx libmx
    ${${search}} PATH_SUFFIXES ${_MATLABLIB})
  find_library(MATLAB_MEX_LIBRARY NAMES mex libmex
    ${${search}} PATH_SUFFIXES ${_MATLABLIB})
  find_library(MATLAB_MAT_LIBRARY NAMES mat libmat
    ${${search}} PATH_SUFFIXES ${_MATLABLIB})
    
  # Also find the extra stuff if building both architectures on the mac
  if (MATLAB_UNIVERSAL)
    find_library(MATLAB_MX_LIBRARY_EXTRA  NAMES mx libmx 
      ${${search}} PATH_SUFFIXES ${MATLABLIB_EXTRA})
    find_library(MATLAB_MEX_LIBRARY_EXTRA NAMES mex libmex
      ${${search}} PATH_SUFFIXES ${MATLABLIB_EXTRA})
    find_library(MATLAB_MAT_LIBRARY_EXTRA NAMES mat libmat
      ${${search}} PATH_SUFFIXES ${MATLABLIB_EXTRA})
      
    list(APPEND _MATLAB_EXTRA_VARS
      MATLAB_MX_LIBRARY_EXTRA
      MATLAB_MEX_LIBRARY_EXTRA
      MATLAB_MAT_LIBRARY_EXTRA
    )
  endif()
  
  # Unix-specific files
  if(UNIX)
    find_file(MATLAB_MEX_MAPFILE mexFunction.map
      ${${search}} PATH_SUFFIXES extern/lib/${_MATLAB_ARCH}
      DOC "gcc version script for mex files"
    )
    list(APPEND _MATLAB_EXTRA_VARS MATLAB_MEX_MAPFILE)
    
    if (MATLAB_UNIVERSAL)
      set(MATLABLIB_EXTRA bin/${_MATLAB_ARCH_EXTRA})
      find_file(MATLAB_MEX_MAPFILE_EXTRA mexFunction.map
        ${${search}} PATH_SUFFIXES extern/lib/${_MATLAB_ARCH_EXTRA}
        DOC "gcc version script for mex files (for extra architecture)"
      )
      list(APPEND _MATLAB_EXTRA_VARS MATLAB_MEX_MAPFILE_EXTRA)
    endif()

    # Also include the version information file (this was removed on R2009a)
    find_path(MATLAB_MEX_VERSION_SRC "mexversion.c"
      ${${search}} PATH_SUFFIXES extern/src
      DOC "Matlab mex version source file"
    )
    mark_as_advanced(MATLAB_MEX_VERSION_SRC)
  endif()
endforeach()



_MATLAB_DBG_MSG("Matlab extra variables: \"${_MATLAB_EXTRA_VARS}\"")
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MATLAB DEFAULT_MSG 
  MATLAB_INCLUDE_DIR
  MATLAB_MX_LIBRARY
  MATLAB_MEX_LIBRARY
  MATLAB_MAT_LIBRARY
  ${_MATLAB_EXTRA_VARS}
)

mark_as_advanced(
  MATLAB_INCLUDE_DIR
  MATLAB_MX_LIBRARY
  MATLAB_MEX_LIBRARY
  MATLAB_MAT_LIBRARY
  ${_MATLAB_EXTRA_VARS}
)

if(MATLAB_FOUND)
  set(MATLAB_INCLUDE_DIRS ${MATLAB_INCLUDE_DIR})
  set(MATLAB_LIBRARIES
    ${MATLAB_MX_LIBRARY} ${MATLAB_MEX_LIBRARY} ${MATLAB_MAT_LIBRARY})
  if (MATLAB_UNIVERSAL)
    set(MATLAB_LIBRARIES_EXTRA
      ${MATLAB_MX_LIBRARY_EXTRA} ${MATLAB_MEX_LIBRARY_EXTRA}
      ${MATLAB_MAT_LIBRARY_EXTRA})
  endif()
endif()
