# CMake project file to build the OpenEXR components
# Define OPENEXR_IS_INTERNAL in order to be
# awere that the library is being built for internal usage
# only, so that it doesn't install everything.
#
# Assumes  that the variables set up by FindZlib are already configured by another entity,
# like the top level directory, and also ZLIB_LIB_FILENAME must
# contain the full path to the zlib import library when building
# this as a Windows DLL.
# In this case it also defines:
#  OpenEXR_INCLUDE_DIR
#  OpenEXR_LIBRARIES
#  OpenEXR_DEFINITIONS
#  OPENEXR_FOUND

if(NOT OPENEXR_IS_INTERNAL)
  cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
  project(OpenEXR)
endif()


option(OPENEXR_SHARED_LIBS
  "Builds the different OpenEXR components as shared libraries." ON)
  
# TODO Find a better place for the platform defines
if (NOT MSVC)
  add_definitions(-DPLATFORM_VISIBILITY_AVAILABLE)
endif()

if(OPENEXR_SHARED_LIBS)
  set(OPENEXR_LIBTYPE "SHARED")
  add_definitions(-DOPENEXR_DLL)
else()
  set(OPENEXR_LIBTYPE "STATIC")
  add_definitions(-DPLATFORM_BUILD_STATIC)
endif()


# Macro to get the version information from the configure.ac file in the
# current directory. Its argument is the name of the library as passed to
# AC_INIT. It will set the variables <LIBNAME>_VERSION and <LIBNAME>_SOVERSION
# with the values read from the file. It also sets the variable CURRENT_PACKAGE
# to the original library name.
macro (GET_VERSION libname)
  set(CURRENT_PACKAGE "${libname}")
  string(TOUPPER "${libname}" libname_upper)
  
  # Read the relevant content from configure.ac
  file (STRINGS configure.ac tmp_configure_ac
    REGEX "(AC_INIT.+)|LIBTOOL_.+=[ \\t]*[0-9]+")
  
  string (REGEX REPLACE ".*AC_INIT\\([ \\t]*([0-9a-zA-z_]+)[ \\t]*,[ \\t]*([0-9\\.]+).*" "\\2"
    ${libname_upper}_VERSION ${tmp_configure_ac})
  if (NOT ${CMAKE_MATCH_1} STREQUAL ${libname})
    message (WARNING "The configure.ac file inits \"${CMAKE_MATCH_1}\" but \"${libname}\" was expected instead.")
  endif()
  string (REGEX REPLACE ".*LIBTOOL_CURRENT[ \\t]*=[ \\t]*([0-9]+).*" "\\1"
    ${libname_upper}_LIBTOOL_CURRENT ${tmp_configure_ac})
  string (REGEX REPLACE ".*LIBTOOL_REVISION[ \\t]*=[ \\t]*([0-9]+).*" "\\1"
    ${libname_upper}_LIBTOOL_REVISION ${tmp_configure_ac})
  string (REGEX REPLACE ".*LIBTOOL_AGE[ \\t]*=[ \\t]*([0-9]+).*" "\\1"
    ${libname_upper}_LIBTOOL_AGE ${tmp_configure_ac})
    
  # Version string for libtool
  set (${libname_upper}_SOVERSION 
    ${${libname_upper}_LIBTOOL_CURRENT}.${${libname_upper}_LIBTOOL_REVISION}.${${libname_upper}_LIBTOOL_AGE}
  )
  
  # Version components
  if (${libname_upper}_VERSION MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(${libname_upper}_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(${libname_upper}_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(${libname_upper}_VERSION_PATCH "${CMAKE_MATCH_3}")
    set(${libname_upper}_VERSION_API "${CMAKE_MATCH_1}_${CMAKE_MATCH_2}")
  else()
    message(FATAL_ERROR
      "${libname} version has an unexpected format: ${${libname_upper}_VERSION}")
  endif()

  # Version strings
  set(${libname_upper}_VERSION_STRING "\"${${libname_upper}_VERSION}\"")
  set(${libname_upper}_PACKAGE_STRING "\"${libname} ${${libname_upper}_VERSION}\"")
  
  # Checks if the string needs to be displayed
  set (${libname_upper}_DISPLAYSTR_AUX 
    "Found ${libname} version ${${libname_upper}_VERSION}, soversion ${${libname_upper}_SOVERSION} from configure.ac"
  )
  if ((NOT ${libname_upper}_DISPLAYSTR) OR (NOT ${libname_upper}_DISPLAYSTR STREQUAL ${libname_upper}_DISPLAYSTR_AUX))
    set (${libname_upper}_DISPLAYSTR ${${libname_upper}_DISPLAYSTR_AUX} 
      CACHE INTERNAL "Version string from ${libname}" FORCE)
    message (STATUS ${${libname_upper}_DISPLAYSTR})
  endif ()

endmacro()

# A small macro to pass extra compiler properties to MSVC (useful to disable warnings)
macro(MSVC_COMPILE_FLAGS targetname flags)
  if(MSVC)
  set_target_properties(${targetname}
    PROPERTIES
    COMPILE_FLAGS "${flags}"
    )
  endif(MSVC)
endmacro()


if (NOT OPENEXR_IS_INTERNAL)
# Configurable target directories
set(INSTALL_INCLUDE_DIR "include/OpenEXR" CACHE STRING
  "Installation path for the public headers (relative to the install prefix.)")
set(INSTALL_LIBRARY_DIR "lib" CACHE STRING
  "Installation path for the link libraries (relative to the install prefix.)")
set(INSTALL_RUNTIME_DIR "bin" CACHE STRING
  "Installation path for the binary runtime (relative to the install prefix.)")

# For install we define a macro
macro(INSTALL_TARGET targetname)
  install(TARGETS ${targetname}
    RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR}
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
    PUBLIC_HEADER DESTINATION ${INSTALL_INCLUDE_DIR}
  )
endmacro()

else()

# Hard-coded install paths for internal setups
macro(INSTALL_TARGET targetname)
    if(OPENEXR_SHARED_LIBS)
      install(TARGETS ${targetname}
        RUNTIME DESTINATION "bin" COMPONENT "openexr"
        LIBRARY DESTINATION "lib" COMPONENT "openexr"
      )
    endif()
endmacro()

endif()


# First we need to create the configuration headers

# Directory for the generated configuration files
set(OPENEXR_CFG_DIR "${CMAKE_CURRENT_BINARY_DIR}/config")

find_package(Threads)

# pthreads (do not use in MinGW)
if(CMAKE_USE_PTHREADS_INIT AND NOT WIN32)
  set(HAVE_PTHREAD 1)
endif()

include(CheckIncludeFile)
CHECK_INCLUDE_FILE("limits.h"   HAVE_LIMITS_H)
CHECK_INCLUDE_FILE("unistd.h"   HAVE_UNISTD_H)
CHECK_INCLUDE_FILE("ucontext.h" HAVE_UCONTEXT_H)

# POSIX semaphores
if(APPLE)
  set(HAVE_POSIX_SEMAPHORES 0)
else()
  CHECK_INCLUDE_FILE("semaphore.h" HAVE_POSIX_SEMAPHORES)
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(HAVE_LINUX_PROCFS 1)
endif()

if(APPLE)
  set(HAVE_DARWIN 1)
endif()

# Detect that std::right if is usable
include(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("
#include <iomanip>
int main (int argc, char **argv) {
  std::right;
  return 0;
}
" HAVE_COMPLETE_IOMANIP)

# Checks for dealing with fpe handling
CHECK_CXX_SOURCE_COMPILES("
#include <stdint.h>
#include <ucontext.h>
int main (int argc, char **argv) {
    ucontext_t ucon;
    uint32_t mxcsr = ucon.uc_mcontext.fpregs->mxcsr;
    uint16_t cw    = ucon.uc_mcontext.fpregs->cwd;
}
" HAVE_CONTROL_REGISTER_SUPPORT)

# Check if the stack is large enough (ie more than 256 KiB)
include(CheckCSourceRuns)
CHECK_C_SOURCE_RUNS("
#include <stddef.h>
#include <string.h>
int main (int argc, char **argv)
{
    size_t arr[0x40000];
    memset(arr, 0, sizeof(arr));
    return 0;
}
" HAVE_LARGE_STACK)

if(HAVE_LARGE_STACK)
  if(WIN32 OR APPLE)
    set(USE_LARGE_STACK_DEFAULT OFF)
    set(USE_LARGE_STACK_EXTRA " (possible compatibility issues.)")
  else()
    set(USE_LARGE_STACK_DEFAULT ON)
    set(USE_LARGE_STACK_EXTRA ".")
  endif()
  option(ENABLE_LARGE_STACK
    "Enable optimizations for systems that support large stack sizes${USE_LARGE_STACK_EXTRA}"
    ${USE_LARGE_STACK_DEFAULT})
endif()

# Check if the system has the Windows Vista sync functions
if(WIN32)
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS("InitOnceExecuteOnce" HAVE_WINNT_VISTA_SYNC)
endif()

# Optionally use functions introduced with Windows Vista for the correct
# initialization on demand of singletons among other things
if(HAVE_WINNT_VISTA_SYNC)
  # Set the default based on the host system (Windows Vista's version is 6.0)
  if("${CMAKE_SYSTEM_VERSION}" VERSION_LESS "6.0")
    set(USE_WINNT_VISTA_SYNC_DEFAULT OFF)
  else()
    set(USE_WINNT_VISTA_SYNC_DEFAULT ON)
  endif()
  option(USE_WINNT_VISTA_SYNC
    "Use synchronization functions introduced in Windows Vista."
    ${USE_WINNT_VISTA_SYNC_DEFAULT})
endif()

# Handle namespacing configuration : internal library namespace
#                                    public namespace
#option(ENABLE_NAMESPACE_VERSIONING
#  "Enable symbol versioning via versioned/custom namespace to prevent runtime conflicts" ON)
set(ENABLE_NAMESPACE_VERSIONING ON)

# Public User namespace
#option(ENABLE_CUSTOM_USER_NAMESPACE
#  "Use a custom namespace into which the library namespace will be exported to" OFF)
set(ENABLE_CUSTOM_USER_NAMESPACE OFF)

# The build requires headers defined within all directories
include_directories( ${OPENEXR_CFG_DIR} )
include_directories( IlmBase/Half )
include_directories( IlmBase/Iex )
include_directories( IlmBase/IexMath )
include_directories( IlmBase/Imath )
include_directories( IlmBase/IlmThread )
include_directories( OpenEXR/IlmImf )


if (WIN32 AND OPENEXR_SHARED_LIBS)
  # Resource template file
  set(OPENEXR_RC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/win32/openexr.rc.cmake")
  
  # Set a human-readable version number for MSVC
  if(MSVC_VERSION)
    # FIXME Support Visual Studio 2015
    math(EXPR MSVC_VER_MAJOR "${MSVC_VERSION}/100 - 6")
    math(EXPR MSVC_VER_MINOR "(${MSVC_VERSION}-(${MSVC_VER_MAJOR}+6)*100)/10")
    if(CMAKE_CL_64)
      set(MSVC_BITS "amd64")
    else()
      set(MSVC_BITS "x86")
    endif()
    set(MSVC_VER_STRING "${MSVC_VER_MAJOR}.${MSVC_VER_MINOR} ${MSVC_BITS}")
  endif()

  # Macro to add the resource file
  macro(ADD_RESOURCE targetname sourceslst_var description version year)
    
    set(RC_DESCRIPTION "${description}")
    set(RC_VERSION "${version}")
    string(REPLACE "." "," RC_VERSION_COMMA "${RC_VERSION}.0")
    set(RC_FILENAME "${targetname}")
    # FIXME Add an extra parameter for the product name.
    # This is coupled with the GET_VERSION macro
    if(CURRENT_PACKAGE)
      set(RC_PRODUCT_NAME "${CURRENT_PACKAGE}")
    else()
      set(RC_PRODUCT_NAME "${targetname}")
    endif()
    set(RC_YEAR "${year}")
    configure_file("${OPENEXR_RC_PATH}"
      "${CMAKE_CURRENT_BINARY_DIR}/${targetname}.rc" ESCAPE_QUOTES)
    
    # MSVC can take the .rc file directly, MINGW needs more magic
    if (MSVC)
      list(APPEND ${sourceslst_var} "${CMAKE_CURRENT_BINARY_DIR}/${targetname}.rc")
    elseif (MINGW)
      # Based on the function from zlib 1.2.7:
      # This gets us DLL resource information when compiling on MinGW.
      if(NOT CMAKE_RC_COMPILER)
        set(CMAKE_RC_COMPILER windres.exe)
      endif()
      add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${targetname}_rc.obj"
                         COMMAND "${CMAKE_RC_COMPILER}"
                              -D GCC_WINDRES
                              -o "${CMAKE_CURRENT_BINARY_DIR}/${targetname}_rc.obj"
                              -i "${CMAKE_CURRENT_BINARY_DIR}/${targetname}.rc")
      list(APPEND ${sourceslst_var} "${CMAKE_CURRENT_BINARY_DIR}/${targetname}_rc.obj")
    endif ()
  
  endmacro()

endif()



# Helper global property to hold the targets created with ADD_OPENEXR_LIBRARY.
set_property(GLOBAL PROPERTY OPENEXR_LIBRARY_TARGETS "")

# Utility macro to add an OpenEXR library. The arguments are:
#   - Name of the target to build
#   - Variable with the public headers list
#   - Variable with all the other sources
#   - Description of the library
#   - Version of the library (e.g. 1.0.1)
#   - soversion of the library, as used by libtool (e.g. 6.0.0)
#   - Copyright year
macro(ADD_OPENEXR_LIBRARY targetname publicheaders_var sources_var description 
                          version soversion year)

  # Adds the resource and module definitions files if necessary
  if (OPENEXR_SHARED_LIBS AND WIN32)
    ADD_RESOURCE (${targetname} ${sources_var} 
      ${description} ${version} ${year})
  endif()
  
  # Adds the target
  add_library(${targetname} ${OPENEXR_LIBTYPE} ${${publicheaders_var}} ${${sources_var}})
  string(TOUPPER ${targetname} targetUpper)
  set_target_properties(${targetname} PROPERTIES 
    VERSION ${version}
    SOVERSION ${soversion}
    DEFINE_SYMBOL "${targetUpper}_EXPORTS"
    DEBUG_POSTFIX "d"
  )
  set_property(GLOBAL APPEND PROPERTY OPENEXR_LIBRARY_TARGETS "${targetname}")
  
  # If built as a static library in gcc we need to add the fpic flag, so that the code will work when linked into a shared lib
  # TODO Remove legacy CMake code when changing the baseline version
  if (NOT CMAKE_VERSION VERSION_LESS 2.8.11 AND OPENEXR_LIBTYPE STREQUAL "STATIC")
    set_target_properties(${targetname} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  else()
    if(OPENEXR_LIBTYPE STREQUAL "STATIC" AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_COMPILER_IS_GNUCC)
      # TODO Use the position-independent target property
      set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS "-fpic")
    endif()
  endif()
  
  # For MSVC static libraries, modify the name so that the shared and static
  # library files may coexists
  if(MSVC AND OPENEXR_LIBTYPE STREQUAL "STATIC")
    set_target_properties(${targetname} PROPERTIES 
      OUTPUT_NAME "${targetname}_static")
  endif()
  
  # TODO Avoid hard-coded macros
  if(NOT OPENEXR_IS_INTERNAL)
    set_target_properties(${targetname} PROPERTIES
      PUBLIC_HEADER "${${publicheaders_var}}")
  else()
    HDRITOOLS_LTCG(${targetname})
    set_target_properties(${targetname} PROPERTIES FOLDER "3rdparty/OpenEXR")
  endif()

  INSTALL_TARGET(${targetname})
endmacro()


# Process each group of libraries
add_subdirectory(IlmBase)
add_subdirectory(OpenEXR)


# And defines the extra variables if it is internal
if(OPENEXR_IS_INTERNAL)

  set(OPENEXR_FOUND 1 PARENT_SCOPE)
  
  # Set the OpenEXR libraries only to IlmImf: cmake will automatically handle all the dependencies
  set(OpenEXR_LIBRARIES IlmImf PARENT_SCOPE)
  set(OpenEXR_INCLUDE_DIR ${OPENEXR_CFG_DIR})
  foreach(dir "IlmBase/Half"   "IlmBase/Iex"       "IlmBase/IexMath" 
               "IlmBase/Imath" "IlmBase/IlmThread" "OpenEXR/IlmImf")
    get_filename_component(fulldir ${dir} ABSOLUTE)
    list(APPEND OpenEXR_INCLUDE_DIR ${fulldir})
  endforeach()
  set(OpenEXR_INCLUDE_DIR ${OpenEXR_INCLUDE_DIR} PARENT_SCOPE)
  
  if(OPENEXR_SHARED_LIBS AND WIN32)
    set(OpenEXR_DEFINITIONS "-DOPENEXR_DLL" PARENT_SCOPE)
  endif()

endif()
