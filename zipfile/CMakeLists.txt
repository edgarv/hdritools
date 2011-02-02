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

# Note that it's not linked here!
include_directories(${ZLIB_INCLUDE_DIRS})
