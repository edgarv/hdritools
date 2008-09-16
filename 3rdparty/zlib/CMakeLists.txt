# CMake build of zlib. Intended only for windows!

# Define ZLIB_IS_INTERNAL if this is to be used as an internal component.
# IN this case, define ZLIB_SHARED_LIB to build it as a shared component.
# Otherwise the internal library (targetname is still zlib) will be static

if(NOT ZLIB_IS_INTERNAL)
  project(ZlibProj)
  cmake_minimum_required(VERSION 2.6)
else(NOT ZLIB_IS_INTERNAL)
  option(ZLIB_SHARED_LIB
    "Builds zlib as a shared library." OFF)
endif(NOT ZLIB_IS_INTERNAL)


set(HEADERS
  zconf.h
  zlib.h
  )

set(SRCS
  adler32.c
  compress.c 
  crc32.h crc32.c
  deflate.h deflate.c
  gzio.c
  infback.c
  inffast.h inffast.c
  inflate.h inflate.c
  inffixed.h
  inftrees.h inftrees.c
  trees.h trees.c
  uncompr.c
  zutil.h zutil.c
  )
   
if(WIN32)
  list(APPEND SRCS win32/zlib1.rc)
endif(WIN32)

  
# Create our targets: a static and a dll version of zlib (just for windows) and
if(NOT ZLIB_IS_INTERNAL)
  add_library(zlib STATIC ${HEADERS} ${SRCS})
  if(WIN32)
    add_library(zlib1 SHARED ${HEADERS} ${SRCS} win32/zlib.def)
	set(ZLIB_TARGETS zlib zlib1)
  else(WIN32)
    set(ZLIB_TARGETS zlib)
  endif(WIN32)
  
  set_target_properties(${ZLIB_TARGETS}
    PROPERTIES
	PUBLIC_HEADER "${HEADERS}"
    )
  
else(NOT ZLIB_IS_INTERNAL)

  # For the internal version, only the static or the debug version is used, both with the target name zlib

  if(ZLIB_SHARED_LIB)
    if(WIN32)
	  list(APPEND SRCS "win32/zlib.def")
	endif(WIN32)
    add_library(zlib SHARED ${HEADERS} ${SRCS})
	if(WIN32)
	  set_target_properties(zlib PROPERTIES
	    OUTPUT_NAME "zlib1")
	endif(WIN32)
	
  else(ZLIB_SHARED_LIB)
    add_library(zlib STATIC ${HEADERS} ${SRCS})
  endif(ZLIB_SHARED_LIB)
  
  set(ZLIB_TARGETS zlib)

endif(NOT ZLIB_IS_INTERNAL)

# Disable the secure crt warnings in MSVC
if(MSVC)
  set_target_properties(${ZLIB_TARGETS}
    PROPERTIES
    COMPILE_FLAGS "/wd4996"
    )
endif(MSVC)

# Distinguish between release and debug builds with a postfix
# except on the internal, static version for windows, to avoid OpenEXR errors.
if(NOT WIN32 OR NOT ZLIB_IS_INTERNAL OR ZLIB_SHARED_LIB)
  set_target_properties(${ZLIB_TARGETS}
    PROPERTIES
    DEBUG_POSTFIX "d"
    )
endif(NOT WIN32 OR NOT ZLIB_IS_INTERNAL OR ZLIB_SHARED_LIB)

  
# Installs the stuff
if(ZLIB_IS_INTERNAL)
  if(ZLIB_SHARED_LIB)
    install(TARGETS ${ZLIB_TARGETS}
      RUNTIME DESTINATION "bin"
    )
  endif(ZLIB_SHARED_LIB)
else(ZLIB_IS_INTERNAL)
  install(TARGETS ${ZLIB_TARGETS}
    RUNTIME DESTINATION "bin"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    PUBLIC_HEADER DESTINATION "include"
  )
endif(ZLIB_IS_INTERNAL)
