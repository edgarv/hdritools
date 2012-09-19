# - Find WiX
# Find the Windows Installer XML compiler tools
# This module defines the following read-only variables:
#  WiX_CANDLE_EXECUTABLE - the full path to the WiX compiler executable.
#  WiX_LIGHT_EXECUTABLE  - the full path to the WiX linker executable.
#  WiX_VERSION_MAJOR     - the major version of the package found.
#  WiX_VERSION_MINOR     - the minor version of the package found.
#  WiX_VERSION_PATCH     - the patch version of the package found.
#  WiX_VERSION_TWEAK     - the tweak version of the package found.
#  WiX_VERSION_STRING    - this is set to: $major.$minor.$patch(.$tweak)
#  WIX_FOUND             - if false, do not try to use WiX.
#
# These variables alter the behavior of the module when defined before calling
# find_package(WiX):
#  WiX_ROOT_DIR - Base location of the WiX installation.
#
#=============================================================================
# Edgar Velázquez-Armendáriz, Cornell University (cs.cornell.edu - eva5)
# Distributed under the OSI-approved MIT License (the "License")
# 
# Copyright (c) 2012 Program of Computer Graphics, Cornell University
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#=============================================================================

# TODO: Eventually handle individual components passed via WiX_FIND_COMPONENTS
# TODO: Add nice FindQt4-style macros to compile and link objects
include(FindPackageHandleStandardArgs)

# Set up the search paths
if (WiX_ROOT_DIR)
  set(_WiX_PATHS_HINTS HINTS "${WiX_ROOT_DIR}")
  set(_WiX_PATHS PATHS "")
  set(_WiX_PATHS_EXTRA "NO_DEFAULT_PATH")
else()
  set(_WiX_PATHS_HINTS "")
  set(_WiX_PATHS_EXTRA "")
  set(_WiX_PATHS PATHS)

  # Fixed search paths
  foreach (version "3.7" "3.6" "3.5" "3.0"  "3")
    # Try to get the value from the registry
    list(APPEND _WiX_PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Installer XML\\${version};InstallRoot]")
    list(APPEND _WiX_PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows Installer XML\\${version};InstallRoot]")
    if (NOT "$ENV{ProgramFiles(x86)}" STREQUAL "")
      list(APPEND _WiX_PATHS "$ENV{ProgramFiles(x86)}\\Windows Installer XML v${version}")
    endif()
    list(APPEND _WiX_PATHS "$ENV{ProgramFiles}\\Windows Installer XML v${version}")
  endforeach()
endif()

set(_WiX_REQUIRED_VARS "")
macro(find_wix_program prog)
  string(TOUPPER "${prog}" prog_upper)
  set(varname WiX_${prog_upper}_EXECUTABLE)
  find_program(${varname} NAMES ${prog}
    ${_WiX_PATHS_HINTS} ${_WiX_PATHS} PATH_SUFFIXES "bin" ${_WiX_PATHS_EXTRA})
  mark_as_advanced(${varname})
  list(APPEND _WiX_REQUIRED_VARS ${varname})
endmacro()

find_wix_program("candle")
find_wix_program("light")

# Extract the version from candle
if (EXISTS "${WiX_CANDLE_EXECUTABLE}")
  execute_process(COMMAND "${WiX_CANDLE_EXECUTABLE}" "-help"
    OUTPUT_VARIABLE _WiX_HELP
    ERROR_QUIET
  )
  if (_WiX_HELP MATCHES "Windows Installer.+[Vv]ersion ([.0-9]+)")
    set(WiX_VERSION_DATA ${CMAKE_MATCH_1})
  endif()
endif()

# If found, get the different version components. Assume major and minor always exist
set(WiX_VERSION_STRING "")
if (WiX_VERSION_DATA)
  if (WiX_VERSION_DATA MATCHES "([0-9]+)\\.([0-9]+).*")
    set(WiX_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(WiX_VERSION_MINOR ${CMAKE_MATCH_2})
    set(WiX_VERSION_STRING "${WiX_VERSION_MAJOR}.${WiX_VERSION_MINOR}")
  endif()
  if (WiX_VERSION_DATA MATCHES "[0-9]+\\.[0-9]+\\.([0-9]+).*")
    set(WiX_VERSION_PATCH ${CMAKE_MATCH_1})
    set(WiX_VERSION_STRING "${WiX_VERSION_STRING}.${WiX_VERSION_PATCH}")
  endif()
  if (WiX_VERSION_DATA MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+).*")
    set(WiX_VERSION_TWEAK ${CMAKE_MATCH_1})
    set(WiX_VERSION_STRING "${WiX_VERSION_STRING}.${WiX_VERSION_TWEAK}")
  endif()
endif()

# Handle the standard arguments
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WiX
  REQUIRED_VARS ${_WiX_REQUIRED_VARS}
  VERSION_VAR WiX_VERSION_STRING)
