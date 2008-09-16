# CMake file for the zipfile library

set(SRCS
  crypt.h
  ioapi.h ioapi.c
  unzip.h unzip.c
  ZipFile.h ZipFile.cpp
  zipstream_buf.h zipstream_buf.cpp
  )
  
add_library(zipfile STATIC ${SRCS})

# Note that it's not linked here!
include_directories(${ZLIB_INCLUDE_DIR})
