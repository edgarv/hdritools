# Copyright (C) 2007 Glenn Randers-Pehrson
# For conditions of distribution and use, see copyright notice in png.h

# Standalone vergion!

set(PNGLIB_MAJOR 1)
set(PNGLIB_MINOR 2)
set(PNGLIB_RELEASE 35)
set(PNGLIB_NAME libpng${PNGLIB_MAJOR}${PNGLIB_MINOR})
set(PNGLIB_VERSION ${PNGLIB_MAJOR}.${PNGLIB_MINOR}.${PNGLIB_RELEASE})

# needed packages
if(NOT ZLIB_IS_INTERNAL)
  find_package(ZLIB REQUIRED)
endif(NOT ZLIB_IS_INTERNAL)

if(NOT WIN32)
 find_library(M_LIBRARY
     NAMES m
     PATHS /usr/lib /usr/local/lib
 )
 if(NOT M_LIBRARY)
   message(STATUS
     "math library 'libm' not found - floating point support disabled")
 endif(NOT M_LIBRARY)
else(NOT WIN32)
 # not needed on windows
 set(M_LIBRARY "")
endif(NOT WIN32)


# COMMAND LINE OPTIONS
option(PNG_SHARED_LIB "Build shared lib" ${USE_SYSTEM_LIBS_DEFAULT})

set(PNG_NO_CONSOLE_IO YES)
set(PNG_NO_STDIO      NO)
set(PNG_DEBUG         NO)

#TODO:
# PNG_CONSOLE_IO_SUPPORTED

# maybe needs improving, but currently I don't know when we can enable what :)
set(png_asm_tmp "OFF")
if(NOT WIN32)
 find_program(uname_executable NAMES uname PATHS /bin /usr/bin /usr/local/bin)
 if(uname_executable)
   EXEC_PROGRAM(${uname_executable} ARGS --machine OUTPUT_VARIABLE uname_output)
   if("uname_output" MATCHES "^.*i[1-9]86.*$")
      set(png_asm_tmp "ON")
   else("uname_output" MATCHES "^.*i[1-9]86.*$")
      set(png_asm_tmp "OFF")
   endif("uname_output" MATCHES "^.*i[1-9]86.*$")
 endif(uname_executable)
else(NOT WIN32)
 # this env var is normally only set on win64
 SET(TEXT "ProgramFiles(x86)")
 if("$ENV{${TEXT}}" STREQUAL "")
  set(png_asm_tmp "ON")
 endif("$ENV{${TEXT}}" STREQUAL "")
endif(NOT WIN32)

# SET LIBNAME
# msvc does not append 'lib' - do it here to have consistent name
if(MSVC)
 set(PNG_LIB_NAME lib)
endif(MSVC)
set(PNG_LIB_NAME ${PNG_LIB_NAME}png${PNGLIB_MAJOR}${PNGLIB_MINOR})

# to distinguish between debug and release lib
set(CMAKE_DEBUG_POSTFIX "d")


# OUR SOURCES
set(libpng_sources
 png.h
 pngconf.h
 png.c
 pngerror.c
 pngget.c
 pngmem.c
 pngpread.c
 pngread.c
 pngrio.c
 pngrtran.c
 pngrutil.c
 pngset.c
 pngtrans.c
 pngwio.c
 pngwrite.c
 pngwtran.c
 pngwutil.c
)

# SOME NEEDED DEFINITIONS
if(MSVC)
  add_definitions(-DPNG_NO_MODULEDEF -D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

# Warning, this is very weak! There should be a way to properly detect this
if(WIN32 AND ZLIB_IS_INTERNAL AND ZLIB_SHARED_LIB)
  add_definitions(-DZLIB_DLL)
endif(WIN32 AND ZLIB_IS_INTERNAL AND ZLIB_SHARED_LIB)

add_definitions(-DLIBPNG_NO_MMX)
add_definitions(-DPNG_NO_MMX_CODE)

if(PNG_CONSOLE_IO_SUPPORTED)
 add_definitions(-DPNG_CONSOLE_IO_SUPPORTED)
endif(PNG_CONSOLE_IO_SUPPORTED)

if(PNG_NO_CONSOLE_IO)
 add_definitions(-DPNG_NO_CONSOLE_IO)
endif(PNG_NO_CONSOLE_IO)

if(PNG_NO_STDIO)
 add_definitions(-DPNG_NO_STDIO)
endif(PNG_NO_STDIO)

if(PNG_DEBUG)
 add_definitions(-DPNG_DEBUG)
endif(PNG_DEBUG)

if(NOT M_LIBRARY AND NOT WIN32)
 add_definitions(-DPNG_NO_FLOATING_POINT_SUPPORTED)
endif(NOT M_LIBRARY AND NOT WIN32)

# NOW BUILD OUR TARGET
include_directories(${PNG_SOURCE_DIR} ${ZLIB_INCLUDE_DIR})

if(PNG_SHARED_LIB)
 add_library(${PNG_LIB_NAME} SHARED ${libpng_sources})
 target_link_libraries(${PNG_LIB_NAME} ${ZLIB_LIBRARIES} ${M_LIBRARY})
else(PNG_SHARED_LIB)
 add_library(${PNG_LIB_NAME} STATIC ${libpng_sources})
endif(PNG_SHARED_LIB)

if(PNG_SHARED_LIB AND WIN32)
 set_target_properties(${PNG_LIB_NAME} PROPERTIES DEFINE_SYMBOL PNG_BUILD_DLL)
endif(PNG_SHARED_LIB AND WIN32)

# SET UP LINKS
set_target_properties(${PNG_LIB_NAME} PROPERTIES
#    VERSION 0.${PNGLIB_RELEASE}.1.2.35
     VERSION 0.${PNGLIB_RELEASE}.0
     SOVERSION 0
     CLEAN_DIRECT_OUTPUT 1)
	 
# Disable truncation warnings
if(MSVC)
  set_target_properties(${PNG_LIB_NAME}
    PROPERTIES
    COMPILE_FLAGS "/wd4267"
    )
endif(MSVC)

# INSTALL
if(PNG_SHARED_LIB)
  install(TARGETS ${PNG_LIB_NAME}
      RUNTIME DESTINATION "bin"
    )
endif(PNG_SHARED_LIB)


set(PNG_LIBRARIES ${PNG_LIB_NAME} PARENT_SCOPE)
