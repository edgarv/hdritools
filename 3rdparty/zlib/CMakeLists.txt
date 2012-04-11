# Define ZLIB_IS_INTERNAL if this is to be used as an internal component.
# IN this case, define ZLIB_SHARED_LIB to build it as a shared component.
# Otherwise the internal library (targetname is still zlib) will be static

if(NOT ZLIB_IS_INTERNAL)
cmake_minimum_required(VERSION 2.4.4)
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS ON)

project(zlib C)

if(NOT DEFINED BUILD_SHARED_LIBS)
    option(BUILD_SHARED_LIBS "Build a shared library form of zlib" ON)
endif()

else() # ZLIB_IS_INTERNAL
  option(ZLIB_SHARED_LIB
    "Build a shared library form of zlib" OFF)
  set(BUILD_SHARED_LIBS ${ZLIB_SHARED_LIB})
endif()

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
if(NOT ZLIB_IS_INTERNAL)
enable_testing()
endif()

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
if(HAVE_SYS_TYPES_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDINT_H)
endif()
if(HAVE_STDDEF_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDDEF_H)
endif()
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

#
# Check for unistd.h
#
check_include_file(unistd.h Z_HAVE_UNISTD_H)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if (NOT DEFINED ZLIB_ZCONF_CMAKE_DIR)
  set(ZLIB_ZCONF_CMAKE_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    # If we're doing an out of source build and the user has a zconf.h
    # in their source tree...
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h)
        message(FATAL_ERROR
            "You must remove ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h "
            "from the source tree.  This file is included with zlib "
            "but CMake generates this file for you automatically "
            "in the build directory.")
  endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zconf.h.cmakein
               ${ZLIB_ZCONF_CMAKE_DIR}/zconf.h @ONLY)
include_directories(${ZLIB_ZCONF_CMAKE_DIR})


#============================================================================
# zlib
#============================================================================

set(ZLIB_PUBLIC_HDRS
    ${ZLIB_ZCONF_CMAKE_DIR}/zconf.h
    zlib.h
)
set(ZLIB_PRIVATE_HDRS
    crc32.h
    deflate.h
    gzguts.h
    inffast.h
    inffixed.h
    inflate.h
    inftrees.h
    trees.h
    zutil.h
)
set(ZLIB_SRCS
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    inflate.c
    infback.c
    inftrees.c
    inffast.c
    trees.c
    uncompr.c
    zutil.c
)

if(NOT MINGW AND BUILD_SHARED_LIBS)
    set(ZLIB_SRCS ${ZLIB_SRCS}
        win32/zlib1.rc # If present will override custom build rule below.
    )
endif()

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([0-9A-Za-z.]+)\".*"
    "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})

if(MINGW AND BUILD_SHARED_LIBS)
    # This gets us DLL resource information when compiling on MinGW.
    if(NOT CMAKE_RC_COMPILER)
        SET(CMAKE_RC_COMPILER windres.exe)
    endif()

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
                       COMMAND ${CMAKE_RC_COMPILER}
                            -D GCC_WINDRES
                            -I ${CMAKE_CURRENT_SOURCE_DIR}
                            -I ${CMAKE_CURRENT_BINARY_DIR}
                            -o ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj
                            -i ${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib1.rc)
    set(ZLIB_SRCS ${ZLIB_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/zlib1rc.obj)
endif()

add_library(zlib ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
if(BUILD_SHARED_LIBS)
set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
endif()

set_target_properties(zlib PROPERTIES SOVERSION 1)

# Use the fpic flag in linux so that this can go into shared libraries
if(ZLIB_IS_INTERNAL AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND 
   CMAKE_COMPILER_IS_GNUCC)
  set_target_properties(zlib PROPERTIES COMPILE_FLAGS "-fpic")
endif()

if(ZLIB_IS_INTERNAL)
  HDRITOOLS_LTCG(zlib)
endif()

if(NOT CYGWIN)
    # This property causes shared libraries on Linux to have the full version
    # encoded into their final filename.  We disable this on Cygwin because
    # it causes cygz-${ZLIB_FULL_VERSION}.dll to be created when cygz.dll
    # seems to be the default.
    #
    # This has no effect with MSVC, on that platform the version info for
    # the DLL comes from the resource file win32/zlib1.rc
    set_target_properties(zlib PROPERTIES VERSION ${ZLIB_FULL_VERSION})
endif()

if(UNIX)
    # On unix-like platforms the library is almost always called libz
   set_target_properties(zlib PROPERTIES OUTPUT_NAME z)
elseif(BUILD_SHARED_LIBS AND WIN32)
    # Creates zlib1.dll when building shared library version
    set_target_properties(zlib PROPERTIES SUFFIX "1.dll")
endif()

if (ZLIB_IS_INTERNAL AND ZLIB_SHARED_LIB)
  install(TARGETS zlib
    RUNTIME DESTINATION "bin" COMPONENT "zlib"
    LIBRARY DESTINATION "lib" COMPONENT "zlib"
  )
endif()

if(NOT ZLIB_IS_INTERNAL)
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
    install(TARGETS zlib
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib )
endif()
if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
    install(FILES ${ZLIB_PUBLIC_HDRS} DESTINATION include)
endif()
if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
    install(FILES zlib.3 DESTINATION share/man/man3)
endif()

#============================================================================
# Example binaries
#============================================================================

add_executable(example test/example.c)
target_link_libraries(example zlib)
add_test(example example)

add_executable(minigzip test/minigzip.c)
target_link_libraries(minigzip zlib)

if(HAVE_OFF64_T)
    add_executable(example64 test/example.c)
    target_link_libraries(example64 zlib)
    set_target_properties(example64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
    add_test(example64 example64)

    add_executable(minigzip64 test/minigzip.c)
    target_link_libraries(minigzip64 zlib)
    set_target_properties(minigzip64 PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64")
endif()

endif() # NOT ZLIB_IS_INTERNAL
