# - Finds both release and debug versions of a library
# FIND_RELEASE_AND_DEBUG(<prefix> NAMES <library names> [DEFAULT_SUFFIXES | DBG_SUFFIXES <suffixes> ] [PATHS <search paths>])
# Help in finding distinct release and debug versions of a library so that
# the appropriate version is selected according to the build configuration.
# Assumes that the name of the debug version is that of the release version
# plus a certain suffix such as "_debug". The effective set of names used
# to find the debug version is the cartesian product of the library's names
# and the debug suffixes.
#
# Requires the CMake modules SelectLibraryConfigurations (introduced in CMake
# 2.8.0) and CMakeParseArguments (introduced in CMake 2.8.3).
#
# Macro argument:
#  <prefix> - A prefix for the variables that will be generated.
#  <library names> - non-empty list with the possible names for a library.
#  DEFAULT_SUFFIXES - use the default list of suffixes for finding the debug
#                     libraries. The suffixes are: -d, -debug, d, _d, debug
#  <suffixes> - custom non-empty list with specific debug suffixes.
#  <search paths> - optional list with additional path in which the libraries
#                   will be search. By default the "lib" suffix for each path
#                   is also searched.
#
# This macro will generate the following advanced variables:
#  <prefix>_LIBRARY_RELEASE - the release version of the library
#  <prefix>_LIBRARY_DEBUG - the debug version of the library
#  <prefix>_LIBRARY, <prefix>_LIBRARIES - set according to the CMake macro
#                                         select_library_configurations(...)
#
# A minimal example, to look for the library "foo" using the default debug
# suffixes:
#  FIND_RELEASE_AND_DEBUG(FOO NAMES foo DEFAULT_SUFFIXES)
#
# Search for the "foo" library with custom names and suffixes. In this example
# the release version can be either "foo" or "myfoo". The actual debug version
# searched is one of "foo-d", "myfoo-d", "foo-dbg" or "myfoo-dbg". The paths
# "/path1" and "/path2/foo" are also searched.
#  FIND_RELEASE_AND_DEBUG(FOO NAMES foo myfoo DBG_SUFFIXES -d -dbg PATHS /path1 /path2/foo)
#
#=============================================================================
# Edgar Vel�zquez-Armend�riz, Cornell University (cs.cornell.edu - eva5)
# Distributed under the OSI-approved MIT License (the "License")
# 
# Copyright (c) 2008-2010 Program of Computer Graphics, Cornell University
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

include(SelectLibraryConfigurations)
include(CMakeParseArguments)

macro(FIND_RELEASE_AND_DEBUG)
  
  # Parse the options. The syntax of the macro is:
  # CMAKE_PARSE_ARGUMENTS(<prefix> <options> <one_value_keywords> <multi_value_keywords> args...)
  CMAKE_PARSE_ARGUMENTS(LIBRELDBG "DEFAULT_SUFFIXES" "DBG_SUFFIXES" "NAMES;PATHS" ${ARGN})

  # Verify that everything makes sense
  if (LIBRELDBG_UNPARSED_ARGUMENTS)
    list(GET LIBRELDBG_UNPARSED_ARGUMENTS 0 LIBPREFIX)
    list(LENGTH LIBRELDBG_UNPARSED_ARGUMENTS _len)
    if (NOT ${_len} EQUAL 1)
      message(WARNING "Too many arguments, prefix assumed to be \"${LIBPREFIX}\"")
    endif()
  else()
    message(SEND_ERROR "Missing library prefix")
  endif()
  unset(_len)
  if (NOT LIBRELDBG_NAMES)
    message(FATAL_ERROR "Missing library names")
  endif()
  if (LIBRELDBG_DEFAULT_SUFFIXES)
    set(LIBRELDBG_DBG_SUFFIXES "-d" "-debug" "d" "_d" "_debug")
  elseif(NOT LIBRELDBG_DBG_SUFFIXES)
    message(FATAL_ERROR "Missing custom debug suffixes")
  endif()

 
  # List with the debug suffixes to use
  

  # Construct the possible debug names {names}x{suffixes}
  set(LIBRELDBG_NAMES_DEBUG "")
  foreach(suffix ${LIBRELDBG_DBG_SUFFIXES})
    foreach(name ${LIBRELDBG_NAMES})
      set(LIBRELDBG_NAMES_DEBUG ${LIBRELDBG_NAMES_DEBUG} "${name}${suffix}")
    endforeach()
  endforeach()

  find_library(${LIBPREFIX}_LIBRARY_RELEASE
    NAMES ${LIBRELDBG_NAMES}
    HINTS ${LIBRELDBG_PATHS}
    PATH_SUFFIXES lib
  )
  find_library(${LIBPREFIX}_LIBRARY_DEBUG
    NAMES ${LIBRELDBG_NAMES_DEBUG}
    HINTS ${LIBRELDBG_PATHS}	
    PATH_SUFFIXES lib
  )

  SELECT_LIBRARY_CONFIGURATIONS(${LIBPREFIX})

  # We don't want to pollute the gui with non-user friendly entries
  mark_as_advanced(${LIBPREFIX}_RELEASE ${LIBPREFIX}_DEBUG)

endmacro()
