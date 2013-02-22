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
  project(OpenEXR)
  cmake_minimum_required(VERSION 2.6)
endif()


option(OPENEXR_SHARED_LIBS
  "Builds the different OpenEXR components as shared libraries." OFF)
  
if(OPENEXR_SHARED_LIBS)
  set(OPENEXR_LIBTYPE "SHARED")
else()
  set(OPENEXR_LIBTYPE "STATIC")
endif()

# Building Windows DLL's on MSVC requires lots of extra work
if(OPENEXR_SHARED_LIBS AND MSVC)
  set(OPENEXR_DLL ON)
endif()

# The SOVERSION is very different from the marketing version
set(ILMBASE_SOVERSION "7.0.0")
set(OPENEXR_SOVERSION "7.0.0")

# Small macro to set the LIBNAME_VERSION variable accoring to the contents of the VERSION
macro(GET_VERSION libname)
  string(TOUPPER "${libname}" libname_upper)
  if(NOT ${libname_upper}_VERSION)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" AUX_VERSION)
    string(REGEX MATCH "${libname} - ([0-9]+\\.[0-9]+\\.[0-9]+)"
      AUX_VERSION "${AUX_VERSION}")
    unset(AUX_VERSION)
    set(${libname_upper}_VERSION ${CMAKE_MATCH_1} 
      CACHE STRING 
	  "Version number for ${libname} as defined in the VERSION file.")
    mark_as_advanced(${libname_upper}_VERSION)
  endif(NOT ${libname_upper}_VERSION)
endmacro(GET_VERSION)

# A small macro to pass extra compiler properties to MSVC (useful to disable warnings)
macro(MSVC_COMPILE_FLAGS targetname flags)
  if(MSVC)
  set_target_properties(${targetname}
    PROPERTIES
    COMPILE_FLAGS "${flags}"
    )
  endif(MSVC)
endmacro()

# For install we define a macro
macro(INSTALL_TARGET targetname)
  if(OPENEXR_IS_INTERNAL)
    if(OPENEXR_SHARED_LIBS)
      install(TARGETS ${targetname}
        RUNTIME DESTINATION "bin" COMPONENT "openexr"
        LIBRARY DESTINATION "lib" COMPONENT "openexr"
      )
	endif()
  else()
    install(TARGETS ${targetname}
      RUNTIME DESTINATION "bin"
      LIBRARY DESTINATION "lib"
      ARCHIVE DESTINATION "lib"
      PUBLIC_HEADER DESTINATION "include/OpenEXR"
    )
  endif()
endmacro()

 
# The version of the ilmbase and openexr libraries.
GET_VERSION("openexr")
GET_VERSION("ilmbase")


# First we need to create the configuration headers

# Directory for the generated configuration files
set(OPENEXR_CFG_DIR ${CMAKE_CURRENT_BINARY_DIR}/config)

find_package(Threads)

# pthreads
if(CMAKE_USE_PTHREADS_INIT)
  set(HAVE_PTHREAD 1)
endif()

# POSIX semaphores
include(CheckIncludeFile)
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

# Check if the stack is large enough (ie more than 256 KiB)
# On Windows this only works if the executables request a large stack, as is
# the case with CMake default options for executables. However this is likely
# to cause problems with other programs (Java, Matlab) when using PIZ
if (NOT WIN32)
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
endif()

# Create the configuration headers
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/IlmBaseConfig.h.cmake
  ${OPENEXR_CFG_DIR}/IlmBaseConfig.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/OpenEXRConfig.h.cmake
  ${OPENEXR_CFG_DIR}/OpenEXRConfig.h)
set(OPENEXR_CFG_HEADERS 
  ${OPENEXR_CFG_DIR}/IlmBaseConfig.h
  ${OPENEXR_CFG_DIR}/OpenEXRConfig.h)
  
# We install the common headers if necessary
if(NOT OPENEXR_IS_INTERNAL)
  install(FILES ${OPENEXR_CFG_HEADERS}
    DESTINATION "include/OpenEXR")
endif()
  

# The build requires headers defined within all directories
include_directories( ${OPENEXR_CFG_DIR} )
include_directories( Half )
include_directories( Iex )
include_directories( Imath )
include_directories( IlmThread )
include_directories( IlmImf )


# Flag to indicate whether this needs the special createDLL tool
set (NEEDS_CREATEDLL FALSE)

# When building a windows dll we need to add some defines and build the create dll tool
if(OPENEXR_DLL)
  add_definitions(-DOPENEXR_DLL)
  
  if (MSVC)
    set (NEEDS_CREATEDLL TRUE)
  endif()
  
  if (NEEDS_CREATEDLL)
    if (MSVC_IDE)
      add_subdirectory(createDLL)
    else ()
      message (FATAL_ERROR "The createDLL toolchain is only supported using Visual Studio.")
    endif ()
  endif ()
  
  # The DLLs need special handling. It assumes that the resource file has been generated!
  if (NEEDS_CREATEDLL)
  
    # List of the paths of the different version of the zlib to use
    set(ZLIB_PATHS "")
  
    # Extract the full filename of zlib, both the debug and release versions if they are present
    list(LENGTH ZLIB_LIBRARIES zlib_libraries_len)
    if (zlib_libraries_len GREATER 0)
      # Loop over all libraries
      foreach(zlib_current ${ZLIB_LIBRARIES})
        if (NOT ("${zlib_current}" STREQUAL "debug" OR 
            "${zlib_current}" STREQUAL "optimized" OR
              "${zlib_current}" STREQUAL "general"))
          get_filename_component(zlib_filename "${zlib_current}" ABSOLUTE)
          get_filename_component(zlib_path "${zlib_filename}" PATH)
          list (APPEND ZLIB_PATHS ${zlib_path})
        endif()
        list(REMOVE_DUPLICATES ZLIB_PATHS)
      endforeach()
    else()
      get_target_property(ZLIB_PATHS zlib "LOCATION")
    endif()
  
    macro(CREATEDLL targetname)
  
      # The createdll program needs an export map while keeping all the symbols
      set_target_properties(${targetname} PROPERTIES
        LINK_FLAGS "/MAP /MAPINFO:EXPORTS /OPT:NOREF /OPT:ICF")
        
      if(${ARGC} EQUAL 2)
        set(EXTRA_LIB "-o${ARGV1}")
      endif()
      
      if (CMAKE_GENERATOR MATCHES "Visual Studio")
        set (createDLL_ARGS
          "-x${targetname}.res" "-M$(PlatformName)"
          "-n$(OutDir)\\$(TargetName).map"
          "-l$(IntDir)"
          "-l$(SolutionDir)Half\\${CMAKE_CFG_INTDIR}"
          "-l$(SolutionDir)Iex\\${CMAKE_CFG_INTDIR}"
          "-l$(SolutionDir)Imath\\${CMAKE_CFG_INTDIR}"
          "-l$(SolutionDir)IlmThread\\${CMAKE_CFG_INTDIR}"
          "-i$(OutDir)\\$(TargetName).lib"
        )
      else()
        message(AUTHOR_WARNING "Check that this path works properly")
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
          set (createDLL_Platform "X64")
        else()
          set (createDLL_Platform "X86")
        endif()
        set (createDLL_ARGS
          "-x${targetname}.res" "-M${createDLL_Platform}"
          "-n${CMAKE_CURRENT_BINARY_DIR}\\${CMAKE_CFG_INTDIR}\\${targetname}.map"
          "-l${CMAKE_CURRENT_BINARY_DIR}\\${targetname}.dir\\${CMAKE_CFG_INTDIR}"
          "-l${PROJECT_BINARY_DIR}\\Half\\${CMAKE_CFG_INTDIR}"
          "-l${PROJECT_BINARY_DIR}\\Iex\\${CMAKE_CFG_INTDIR}"
          "-l${PROJECT_BINARY_DIR}\\Imath\\${CMAKE_CFG_INTDIR}"
          "-l${PROJECT_BINARY_DIR}\\IlmThread\\${CMAKE_CFG_INTDIR}"
          "-i${CMAKE_CURRENT_BINARY_DIR}\\${CMAKE_CFG_INTDIR}\\${targetname}.lib"
        )
      endif()
      if (ZLIB_PATHS)
        set (createDLL_ARGS ${createDLL_ARGS} "-l${ZLIB_PATHS}")
      endif()
      add_custom_command(TARGET ${targetname} POST_BUILD
        COMMAND createDLL
          ${createDLL_ARGS}
          "${EXTRA_LIB}"
        COMMENT "Exporting all public symbols from $(TargetFileName)..."
      )
      
      # If createDLL does not exist before the target is created, horrible things happen
      add_dependencies(${targetname} createDLL)
    endmacro()
    
  else()
  
    # Define the macro as no-ops
    macro(CREATEDLL targetname)
    endmacro()
  endif () # NEEDS_CREATEDLL
  
endif()


if (WIN32 AND OPENEXR_SHARED_LIBS)
  # Store in cache the master resource file
  get_filename_component(OPENEXR_RC_PATH "win32/openexr.rc" 
    ABSOLUTE CACHE)
  mark_as_advanced(OPENEXR_RC_PATH)

  # Macro to add the resource file
  macro(ADD_RESOURCE targetname sourceslst_var description version year)
    
    set(RC_DESCRIPTION "${description}")
    set(RC_VERSION "${version}.0")
    string(REPLACE "." "," RC_VERSION_COMMA ${RC_VERSION})
    set(RC_FILENAME "${targetname}.dll")
    set(RC_NAME "${targetname}")
    set(RC_YEAR "${year}")
    configure_file("${OPENEXR_RC_PATH}"
      "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}.rc"  ESCAPE_QUOTES)
    
    # MSVC can take the .rc file directly, MINGW needs more magic
    if (MSVC)
      list(APPEND ${sourceslst_var} "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}.rc")
    elseif (MINGW)
      # Based on the function from zlib 1.2.5:
      # This gets us DLL resource information when compiling on MinGW.
      add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}_rc.obj"
                         COMMAND windres.exe
                              -D GCC_WINDRES
                              -o "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}_rc.obj"
                              -i "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}.rc")
      list(APPEND ${sourceslst_var} "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}_rc.obj")
    endif ()
  
  endmacro()

endif()



# Helper global property to hold the targets created with ADD_OPENEXR_LIBRARY.
set_property(GLOBAL PROPERTY OPENEXR_LIBRARY_TARGETS "")

# Super macro with all the common stuff. The arguments are:
#   - Name of the target to build
#   - Variable with the public headers list
#   - Variable with all the other sources
#   - Description of the library
#   - Version of the library (e.g. 1.0.1)
#   - soversion of the library, as used by libtool (e.g. 6.0.0)
#   - Copyright year
#   - Optional extra arg: extra library to add when linking the dll
macro(ADD_OPENEXR_LIBRARY targetname publicheaders_var sources_var description 
                          version soversion year)

  # Adds the resource and module definitions files if necessary
  if (OPENEXR_SHARED_LIBS AND WIN32)
    ADD_RESOURCE (${targetname} ${sources_var} 
      ${description} ${version} ${year})
  endif()
  
  # Adds the target
  add_library(${targetname} ${OPENEXR_LIBTYPE} ${${publicheaders_var}} ${${sources_var}})
  set_target_properties(${targetname} PROPERTIES 
    VERSION ${version}
    SOVERSION ${soversion}
    DEBUG_POSTFIX "d"
  )
  set_property(GLOBAL APPEND PROPERTY OPENEXR_LIBRARY_TARGETS "${targetname}")
  
  # If built as a static library in gcc we need to add the fpic flag, so that the code will work when linked into a shared lib
  if(OPENEXR_LIBTYPE STREQUAL "STATIC" AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_COMPILER_IS_GNUCC)
    set_target_properties(${targetname} PROPERTIES 
      COMPILE_FLAGS "-fpic"
    )
  endif()
  
  # For MSVC static libraries, modify the name for easier identification
  if(MSVC AND OPENEXR_LIBTYPE STREQUAL "STATIC")
    set_target_properties(${targetname} PROPERTIES 
      OUTPUT_NAME "${targetname}_static"
    )
  endif()
  
  if(NOT OPENEXR_IS_INTERNAL)
    set_target_properties(${targetname} PROPERTIES
      PUBLIC_HEADER "${${publicheaders_var}}")
  else()
    HDRITOOLS_LTCG(${targetname})
    set_target_properties(${targetname} PROPERTIES FOLDER "3rdparty/OpenEXR")
  endif()
  
  # For the windows dll we need extra definitions and use createDLL
  if(OPENEXR_DLL)
    string(TOUPPER ${targetname} targetUpper)
    add_definitions(-D${targetUpper}_EXPORTS)
    CREATEDLL(${targetname} ${ARGV7})
  endif()
  
  # Finally installs the target
  INSTALL_TARGET(${targetname})

endmacro()

# Helper macro to set up a target folder for the extra build tools
macro(OPENEXR_TOOL_FOLDER)
  if (OPENEXR_IS_INTERNAL)
    set_target_properties(${ARGN} PROPERTIES
      FOLDER "3rdparty/OpenEXR/Build Tools")
  endif()
endmacro()

  
# Process each actual library
add_subdirectory(Half)
add_subdirectory(Iex)
add_subdirectory(Imath)
add_subdirectory(IlmThread)
add_subdirectory(IlmImf)


# And defines the extra variables if it is internal
if(OPENEXR_IS_INTERNAL)

  set(OPENEXR_FOUND 1 PARENT_SCOPE)
  
  # Set the OpenEXR libraries only to IlmImf: cmake will automatically handle all the dependencies
  set(OpenEXR_LIBRARIES IlmImf PARENT_SCOPE)
  set(OpenEXR_INCLUDE_DIR ${OPENEXR_CFG_DIR})
  foreach(dir Half Iex Imath IlmThread IlmImf)
    get_filename_component(fulldir ${dir} ABSOLUTE)
	list(APPEND OpenEXR_INCLUDE_DIR ${fulldir})
  endforeach()
  set(OpenEXR_INCLUDE_DIR ${OpenEXR_INCLUDE_DIR} PARENT_SCOPE)
  
  if(OPENEXR_DLL)
    set(OpenEXR_DEFINITIONS "-DOPENEXR_DLL" PARENT_SCOPE)
  endif()

endif()
