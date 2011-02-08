# Copyright (C) 2007-2010 Glenn Randers-Pehrson

# This code is released under the libpng license.
# For conditions of distribution and use, see the disclaimer
# and license in png.h

if (NOT PNG_IS_INTERNAL)
  cmake_minimum_required(VERSION 2.4.3)
  set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

  if(UNIX AND NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING 
        "Choose the type of build, options are:
           None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used)
           Debug
           Release
           RelWithDebInfo
           MinSizeRel.")
  endif()

  project(libpng C)
  enable_testing()
endif()

set(PNGLIB_MAJOR 1)
set(PNGLIB_MINOR 2)
set(PNGLIB_RELEASE 44)
set(PNGLIB_NAME libpng${PNGLIB_MAJOR}${PNGLIB_MINOR})
set(PNGLIB_VERSION ${PNGLIB_MAJOR}.${PNGLIB_MINOR}.${PNGLIB_RELEASE})

# needed packages
if(NOT ZLIB_IS_INTERNAL)
  find_package(ZLIB REQUIRED)
endif()
include_directories(${ZLIB_INCLUDE_DIRS})

if(NOT WIN32)
  find_library(M_LIBRARY
    NAMES m
    PATHS /usr/lib /usr/local/lib
  )
  if(NOT M_LIBRARY)
    message(STATUS
      "math library 'libm' not found - floating point support disabled")
  else()
    mark_as_advanced(M_LIBRARY)
  endif()
else()
  # not needed on windows
  set(M_LIBRARY "")
endif()

# COMMAND LINE OPTIONS
if(NOT PNG_IS_INTERNAL)
  if(DEFINED PNG_SHARED)
    option(PNG_SHARED "Build shared lib" ${PNG_SHARED})
  else()
    option(PNG_SHARED "Build shared lib" ON)
  endif()
  if(DEFINED PNG_STATIC)
    option(PNG_STATIC "Build static lib" ${PNG_STATIC})
  else()
    option(PNG_STATIC "Build static lib" ON)
  endif()
else()
  option(PNG_SHARED_LIB "Build shared lib" ${USE_SYSTEM_LIBS_DEFAULT})
  set(PNG_SHARED ${PNG_SHARED_LIB})
  set(PNG_STATIC (NOT ${PNG_SHARED_LIB}))
endif()

if (NOT PNG_IS_INTERNAL)
  if(MINGW)
    option(PNG_TESTS  "Build pngtest" NO)
  else()
    option(PNG_TESTS  "Build pngtest" YES)
  endif()
endif()

if (NOT PNG_IS_INTERNAL)
  option(PNG_NO_CONSOLE_IO "FIXME" YES)
  option(PNG_NO_STDIO      "FIXME" YES)
  option(PNG_DEBUG         "Build with debug output" NO)
  option(PNGARG            "FIXME" YES)
  #TODO:
  # PNG_CONSOLE_IO_SUPPORTED
else()
  set(PNG_NO_CONSOLE_IO YES)
  set(PNG_NO_STDIO      NO)
  set(PNG_DEBUG         NO)
  set(PNGARG            YES)
endif()

# maybe needs improving, but currently I don't know when we can enable what :)
set(png_asm_tmp "OFF")
if(NOT WIN32)
  find_program(uname_executable NAMES uname PATHS /bin /usr/bin /usr/local/bin)
  if(uname_executable)
    mark_as_advanced(uname_executable)
    exec_program(${uname_executable}
                 ARGS --machine OUTPUT_VARIABLE uname_output)
    if("uname_output" MATCHES "^.*i[1-9]86.*$")
      set(png_asm_tmp "ON")
    else()
      set(png_asm_tmp "OFF")
    endif()
  endif()
else()
  # this env var is normally only set on win64
  set(TEXT "ProgramFiles(x86)")
  if("$ENV{${TEXT}}" STREQUAL "")
    set(png_asm_tmp "ON")
  endif("$ENV{${TEXT}}" STREQUAL "")
endif()

# SET LIBNAME
set(PNG_LIB_NAME png${PNGLIB_MAJOR}${PNGLIB_MINOR})

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
set(pngtest_sources
  pngtest.c
)
# SOME NEEDED DEFINITIONS

add_definitions(-DPNG_CONFIGURE_LIBPNG)

if(_AIX)
  add_definitions(-D_ALL_SOURCE)
endif(_AIX)

if(MSVC)
  add_definitions(-DPNG_NO_MODULEDEF -D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

# Warning, this is very weak! There should be a way to properly detect this
if(MSVC AND ZLIB_IS_INTERNAL AND ZLIB_SHARED_LIB)
  set(USE_ZLIB_DLL TRUE)
elseif(PNG_SHARED OR  NOT MSVC)
  set(USE_ZLIB_DLL TRUE)
else()
  set(USE_ZLIB_DLL FALSE)
endif()
if(USE_ZLIB_DLL)
  #if building msvc static this has NOT to be defined
  add_definitions(-DZLIB_DLL)
endif()

add_definitions(-DLIBPNG_NO_MMX)
add_definitions(-DPNG_NO_MMX_CODE)


if(PNG_CONSOLE_IO_SUPPORTED)
  add_definitions(-DPNG_CONSOLE_IO_SUPPORTED)
endif()

if(PNG_NO_CONSOLE_IO)
  add_definitions(-DPNG_NO_CONSOLE_IO)
endif()

if(PNG_NO_STDIO)
  add_definitions(-DPNG_NO_STDIO)
endif()

if(PNG_DEBUG)
  add_definitions(-DPNG_DEBUG)
endif()

if(NOT M_LIBRARY AND NOT WIN32)
  add_definitions(-DPNG_NO_FLOATING_POINT_SUPPORTED)
endif()

# NOW BUILD OUR TARGET
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ZLIB_INCLUDE_DIRS})

if(PNG_SHARED)
  add_library(${PNG_LIB_NAME} SHARED ${libpng_sources})
  if(PNG_IS_INTERNAL)
    set(PNG_LIBRARIES ${PNG_LIB_NAME} PARENT_SCOPE)
  endif()
  if(MSVC)
    # msvc does not append 'lib' - do it here to have consistent name
    set_target_properties(${PNG_LIB_NAME} PROPERTIES PREFIX "lib")
  endif()
  target_link_libraries(${PNG_LIB_NAME} ${ZLIB_LIBRARIES} ${M_LIBRARY})
endif()

if(PNG_STATIC)
# does not work without changing name
  set(PNG_LIB_NAME_STATIC ${PNG_LIB_NAME}_static)
  add_library(${PNG_LIB_NAME_STATIC} STATIC ${libpng_sources})
  if(PNG_IS_INTERNAL)
    set(PNG_LIBRARIES ${PNG_LIB_NAME_STATIC} PARENT_SCOPE)
  endif()
  if(MSVC)
    # msvc does not append 'lib' - do it here to have consistent name
    set_target_properties(${PNG_LIB_NAME_STATIC} PROPERTIES PREFIX "lib")
  endif()
endif()

if(PNG_SHARED AND WIN32)
  set_target_properties(${PNG_LIB_NAME} PROPERTIES DEFINE_SYMBOL PNG_BUILD_DLL)
endif()

if(PNG_TESTS AND PNG_SHARED)
  # does not work with msvc due to png_lib_ver issue
  add_executable(pngtest ${pngtest_sources})
  target_link_libraries(pngtest ${PNG_LIB_NAME})
  add_test(pngtest pngtest ${CMAKE_CURRENT_SOURCE_DIR}/pngtest.png)
endif()


# CREATE PKGCONFIG FILES
# we use the same files like ./configure, so we have to set its vars
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)

if (NOT PNG_IS_INTERNAL)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/libpng.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/libpng-config.in
    ${CMAKE_CURRENT_BINARY_DIR}/libpng-config)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/libpng.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/libpng-config.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config)
endif()

# SET UP LINKS
if(PNG_SHARED)
  set_target_properties(${PNG_LIB_NAME} PROPERTIES
#   VERSION 0.${PNGLIB_RELEASE}.1.2.44
    VERSION 0.${PNGLIB_RELEASE}.0
    SOVERSION 0
    CLEAN_DIRECT_OUTPUT 1)
endif()
if(PNG_STATIC)
  if(NOT WIN32)
    # that's uncool on win32 - it overwrites our static import lib...
    set_target_properties(${PNG_LIB_NAME_STATIC} PROPERTIES
      OUTPUT_NAME ${PNG_LIB_NAME}
      CLEAN_DIRECT_OUTPUT 1)
  endif()
endif()

# INSTALL
if(PNG_IS_INTERNAL AND PNG_SHARED)
  install(TARGETS ${PNG_LIB_NAME}
          RUNTIME DESTINATION bin COMPONENT "libpng"
          LIBRARY DESTINATION lib COMPONENT "libpng")
endif()

if(NOT PNG_IS_INTERNAL AND NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
  if(PNG_SHARED)
    install(TARGETS ${PNG_LIB_NAME}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
  endif()
  if(PNG_STATIC)
    install(TARGETS ${PNG_LIB_NAME_STATIC}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
  endif()
endif()

if(NOT PNG_IS_INTERNAL AND NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
  install(FILES png.h pngconf.h         DESTINATION include)
  install(FILES png.h pngconf.h         DESTINATION include/${PNGLIB_NAME})
endif()
if(NOT PNG_IS_INTERNAL AND NOT SKIP_INSTALL_EXECUTABLES AND NOT SKIP_INSTALL_ALL)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libpng-config DESTINATION bin)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
          DESTINATION bin)
endif()
if(NOT PNG_IS_INTERNAL AND NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL)
  # Install man pages
  install(FILES libpng.3 libpngpf.3             DESTINATION man/man3)
  install(FILES png.5                           DESTINATION man/man5)
  # Install pkg-config files
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc
          DESTINATION lib/pkgconfig)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpng-config
          DESTINATION bin)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc
          DESTINATION lib/pkgconfig)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
          DESTINATION bin)
endif()

# what's with libpng.txt and all the extra files?


# UNINSTALL
# do we need this?


# DIST
# do we need this?

# to create msvc import lib for mingw compiled shared lib
# pexports libpng.dll > libpng.def
# lib /def:libpng.def /machine:x86

