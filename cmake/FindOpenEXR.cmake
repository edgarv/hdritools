# - Find OpenEXR
# Find the native OpenEXR includes and libraries
# This module defines the following read-only variables:
#  OpenEXR_INCLUDE_DIR - where to find OpenEXR/ImfRgbaFile.h, etc.
#  OpenEXR_LIBRARIES   - libraries to link against to use OpenEXR.
#  OPENEXR_FOUND       - if false, do not try to use OpenEXR.
#
# These variables alter the behavior of the module when defined before calling
# find_package(OpenEXR):
#  OpenEXR_ROOT_DIR - Base location of the OpenEXR installation.
#
#=============================================================================
# Edgar Velázquez-Armendáriz, Cornell University (cs.cornell.edu - eva5)
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

# TODO: Add support for pkgconfig

find_path(OpenEXR_INCLUDE_DIR ImfRgbaFile.h
  HINTS ${OpenEXR_ROOT_DIR}
  PATHS /usr/include /opt/local
  PATH_SUFFIXES OpenEXR
  )
mark_as_advanced(OpenEXR_INCLUDE_DIR)

# Finds the five openexr libraries libraries
include(FindReleaseAndDebug)
include(FindPackageHandleStandardArgs)
set(SEARCH_PATH /usr/lib64 /opt/local)

FIND_RELEASE_AND_DEBUG(OpenEXR_HALF NAMES Half DEFAULT_SUFFIXES
  PATHS ${OpenEXR_ROOT_DIR})
FIND_RELEASE_AND_DEBUG(OpenEXR_IEX NAMES Iex DEFAULT_SUFFIXES
  PATHS ${OpenEXR_ROOT_DIR})
FIND_RELEASE_AND_DEBUG(OpenEXR_IMATH NAMES Imath DEFAULT_SUFFIXES
  PATHS ${OpenEXR_ROOT_DIR})
FIND_RELEASE_AND_DEBUG(OpenEXR_ILMTHREAD NAMES IlmThread DEFAULT_SUFFIXES
  PATHS ${OpenEXR_ROOT_DIR})
FIND_RELEASE_AND_DEBUG(OpenEXR_ILMIMF NAMES IlmImf DEFAULT_SUFFIXES
  PATHS ${OpenEXR_ROOT_DIR})


# Handle the result
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenEXR DEFAULT_MSG OpenEXR_INCLUDE_DIR
  OpenEXR_HALF_LIBRARY OpenEXR_IEX_LIBRARY OpenEXR_IMATH_LIBRARY
  OpenEXR_ILMTHREAD_LIBRARY OpenEXR_ILMIMF_LIBRARY)

if (OPENEXR_FOUND)
  set(OpenEXR_LIBRARIES
    ${OpenEXR_HALF_LIBRARY} ${OpenEXR_IEX_LIBRARY}
    ${OpenEXR_IMATH_LIBRARY} ${OpenEXR_ILMTHREAD_LIBRARY}
    ${OpenEXR_ILMIMF_LIBRARY})
endif()
