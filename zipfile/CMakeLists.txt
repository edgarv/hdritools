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

# CMake file for the zipfile library

set(SRCS
  crypt.h
  ioapi.h ioapi.c
  unzip.h unzip.c
  ZipFile.h ZipFile.cpp
  zipstream_buf.h zipstream_buf.cpp
  )

# The mac supports 64-bit IO without extra stuff
if (APPLE)
  add_definitions(-DUSE_FILE32API)
endif()
  
add_library(zipfile STATIC ${SRCS})
HDRITOOLS_LTCG(zipfile)

# Note that it's not linked here!
include_directories(${ZLIB_INCLUDE_DIRS})
