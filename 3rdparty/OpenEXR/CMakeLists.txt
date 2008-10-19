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
# OpenEXR_LIBRARIES
# OpenEXR_DEFINITIONS
# OpenEXR_FOUND

if(NOT OPENEXR_IS_INTERNAL)
  project(OpenEXR)
  cmake_minimum_required(VERSION 2.6)
endif(NOT OPENEXR_IS_INTERNAL)


option(OPENEXR_SHARED_LIBS
  "Builds the different OpenEXR components as shared libraries." OFF)
  
if(OPENEXR_SHARED_LIBS)
  set(OPENEXR_LIBTYPE "SHARED")
else(OPENEXR_SHARED_LIBS)
  set(OPENEXR_LIBTYPE "STATIC")
endif(OPENEXR_SHARED_LIBS)
 
# We use awk to extract the version string from the VERSION file
if(WIN32)
  set(GNUWIN32_PATH "win32")
endif(WIN32)
find_program(AWK "awk" ${GNUWIN32_PATH})
if(NOT AWK)
  message(FATAL_ERROR "awk was not found!")
else(NOT AWK)
  mark_as_advanced(AWK)
endif(NOT AWK)

# Small macro to set the LIBNAME_VERSION variable accoring to the contents of the VERSION
macro(GET_VERSION libname)
  string(TOUPPER "${libname}" libname_upper)
  if(NOT ${libname_upper}_VERSION)
    execute_process(COMMAND "${AWK}" 
      "toupper($1) ~ /^${libname_upper}/ { print $3 }"
	  "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
	  OUTPUT_VARIABLE AUX_VERSION
	  OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${libname_upper}_VERSION "${AUX_VERSION}" 
      CACHE STRING 
	  "Version number for ${libname} as defined in the VERSION file.")
    mark_as_advanced(${libname_upper}_VERSION)
  endif(NOT ${libname_upper}_VERSION)
endmacro(GET_VERSION)

# A small macro to pass extra compiler properties to MSVC (useful to disable warnins)
macro(MSVC_COMPILE_FLAGS targetname flags)
  if(MSVC)
  set_target_properties(${targetname}
    PROPERTIES
    COMPILE_FLAGS "${flags}"
    )
  endif(MSVC)
endmacro(MSVC_COMPILE_FLAGS)

# For install we define a macro
macro(INSTALL_TARGET targetname)
  if(OPENEXR_IS_INTERNAL)
    if(OPENEXR_SHARED_LIBS)
      install(TARGETS ${targetname}
        RUNTIME DESTINATION "bin"
      )
	endif(OPENEXR_SHARED_LIBS)
  else(OPENEXR_IS_INTERNAL)
    install(TARGETS ${targetname}
      RUNTIME DESTINATION "bin"
      LIBRARY DESTINATION "lib"
      ARCHIVE DESTINATION "lib"
      PUBLIC_HEADER DESTINATION "include/OpenEXR"
    )
  endif(OPENEXR_IS_INTERNAL)
endmacro(INSTALL_TARGET)

 
# The version of the ilmbase and openexr libraries.
GET_VERSION("openexr")
GET_VERSION("ilmbase")

# Building Windows DLL's on MSVC requires lots of extra work
if(OPENEXR_SHARED_LIBS AND MSVC)
  set(OPENEXR_DLL ON)
endif(OPENEXR_SHARED_LIBS AND MSVC)

# Enable SSE2 for x86 targets on MSVC
if(MSVC AND NOT CMAKE_CL_64)
  option(OPENEXR_USE_SSE2 "Enables the SSE2 instruction set." ON)
  if(OPENEXR_USE_SSE2)
    set ( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:SSE2" )
    set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE2" )
  endif(OPENEXR_USE_SSE2)
endif(MSVC AND NOT CMAKE_CL_64)


# First we need to create the configuration headers

# Directory for the generated configuration files
set(OPENEXR_CFG_DIR ${CMAKE_CURRENT_BINARY_DIR}/config)

find_package(Threads)

# pthreads
if(CMAKE_USE_PTHREADS_INIT)
  set(HAVE_PTHREAD 1)
endif(CMAKE_USE_PTHREADS_INIT)

# posix semaphores
include(CheckIncludeFile)
if(APPLE)
  set(HAVE_POSIX_SEMAPHORES 0)
else(APPLE)
  CHECK_INCLUDE_FILE("semaphore.h" HAVE_POSIX_SEMAPHORES)
endif(APPLE)


# Large stack is enabled only in linux systems
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(HAVE_LINUX_PROCFS 1)
  set(HAVE_LARGE_STACK  1)
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(HAVE_DARWIN 1)
endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

# Detect that std::right if is usable
include(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES("#include <iostream>\n int main() { std::cout << std::right << \"Works\"; }" 
  HAVE_COMPLETE_IOMANIP)

# Create the configuration headers
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/IlmBaseConfig.h.cmake
  ${OPENEXR_CFG_DIR}/IlmBaseConfig.h
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/OpenEXRConfig.h.cmake
  ${OPENEXR_CFG_DIR}/OpenEXRConfig.h
  )
set(OPENEXR_CFG_HEADERS 
  ${OPENEXR_CFG_DIR}/IlmBaseConfig.h
  ${OPENEXR_CFG_DIR}/OpenEXRConfig.h
  )
  
# We install the common headers if necessary
if(NOT OPENEXR_IS_INTERNAL)
  install(FILES ${OPENEXR_CFG_HEADERS}
    DESTINATION "include/OpenEXR"
    )
endif(NOT OPENEXR_IS_INTERNAL)
  

# The build requires headers defined within all directories
include_directories( ${OPENEXR_CFG_DIR} )
include_directories( Half )
include_directories( Iex )
include_directories( Imath )
include_directories( IlmThread )
include_directories( IlmImf )


# When building a windows dll we need to add some defines and build the create dll tool
if(OPENEXR_DLL)
  add_definitions(-DOPENEXR_DLL)
  add_subdirectory(createDLL)
  
  # Most likely there is only one zlib file. Otherwise, assume that this variable was already set up somehow!
  if(NOT OPENEXR_IS_INTERNAL)
    if(OPENEXR_SYSTEM_ZLIB)
  	  get_filename_component(ZLIB_LIB_FILENAME "${ZLIB_LIBRARIES}" ABSOLUTE)
    else(OPENEXR_SYSTEM_ZLIB)
      get_target_property(ZLIB_LIB_FILENAME zlib "LOCATION")
    endif(OPENEXR_SYSTEM_ZLIB)
  endif(NOT OPENEXR_IS_INTERNAL)
  
  # Store in cache the master resource file
  get_filename_component(OPENEXR_RC_PATH "win32/openexr.rc" 
    ABSOLUTE CACHE)
  mark_as_advanced(OPENEXR_RC_PATH)
  
  # Macro to add the resource file
  macro(ADD_RESOURCE targetname sourceslst_var description version year)
	
	set(RC_DESCRIPTION "${description}")
    set(RC_VERSION "${version}")
	string(REPLACE "." "," RC_VERSION_COMMA ${RC_VERSION})
    set(RC_FILENAME "${targetname}.dll")
    set(RC_NAME "${targetname}")
    set(RC_YEAR "${year}")
    configure_file("${OPENEXR_RC_PATH}"
      "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}.rc"  ESCAPE_QUOTES)
	
    list(APPEND ${sourceslst_var} "${CMAKE_CURRENT_BINARY_DIR}/${RC_NAME}.rc")
  
  endmacro(ADD_RESOURCE)
  
  
  # The DLLs need special handling. It assumes that the resource file has been generated!
  macro(CREATEDLL targetname)
  
    # The createdll program need an export map while keeping all the symbols
    set_target_properties(${targetname} PROPERTIES
      LINK_FLAGS "/MAP /MAPINFO:EXPORTS /OPT:NOREF /OPT:ICF")
		
	if(${ARGC} EQUAL 2)
	  set(EXTRA_LIB "-o${ARGV1}")
	endif(${ARGC} EQUAL 2)
	
	# We also add the zlib directory
	get_filename_component(ZLIB_PATH ${ZLIB_LIB_FILENAME} PATH)
	
    add_custom_command(TARGET ${targetname} POST_BUILD
      COMMAND createDLL
	    "-x${targetname}.res" "-n$(OutDir)\\$(TargetName).map" "-l$(IntDir);..\\Half\\$(OutDir);..\\Iex\\$(OutDir);..\\Imath\\$(OutDir);..\\IlmThread\\$(OutDir);${ZLIB_PATH}" "-i$(OutDir)\\$(TargetName).lib"
		"${EXTRA_LIB}"
	  COMMENT "Exporting all public symbols from $(TargetFileName)..."
	  )
	  
	# If createDLL does not exist before the target is created, horrible things happen
	add_dependencies(${targetname} createDLL)
	
  endmacro(CREATEDLL)
  
endif(OPENEXR_DLL)


# Super macro with all the common stuff. The arguments are:
#   - Name of the target to build
#   - Variable with the public headers list
#   - Variable with all the other sources
#   - Description of the library
#   - Version of the library (e.g. 1.0.1)
#   - Copyright year
#   - Optional extra arg: extra library to add when linking the dll
macro(ADD_OPENEXR_LIBRARY targetname publicheaders_var sources_var description version year)

  # Adds the resource file if necessary
  if(OPENEXR_DLL)
  
    ADD_RESOURCE(${targetname} ${sources_var} 
	  ${description} ${version} ${year})
  endif(OPENEXR_DLL)
  
  # Adds the target
  add_library(${targetname} ${OPENEXR_LIBTYPE} ${${publicheaders_var}} ${${sources_var}})
  set_target_properties(${targetname} PROPERTIES 
    VERSION ${version}
	DEBUG_POSTFIX "d"
  )
  
  if(NOT OPENEXR_IS_INTERNAL)
    set_target_properties(${targetname} PROPERTIES
	  PUBLIC_HEADER "${${publicheaders_var}}"
	)
  endif(NOT OPENEXR_IS_INTERNAL)
  
  # For the windows dll we need extra definitions and use createDLL
  if(OPENEXR_DLL)
    string(TOUPPER ${targetname} targetUpper)
    add_definitions(-D${targetUpper}_EXPORTS)
    CREATEDLL(${targetname} ${ARGV6})
  endif(OPENEXR_DLL)
  
  # Finally installs the target
  INSTALL_TARGET(${targetname})

endmacro(ADD_OPENEXR_LIBRARY)
  
# Process each actual library
add_subdirectory(Half)
add_subdirectory(Iex)
add_subdirectory(Imath)
add_subdirectory(IlmThread)
add_subdirectory(IlmImf)


# And defines the extra variables if it is internal
if(OPENEXR_IS_INTERNAL)

  set(OpenEXR_FOUND 1 PARENT_SCOPE)
  
  # Sets the libraries only to IlmImf: cmake will automatically all the transitive dependencies
  set(OpenEXR_LIBRARIES IlmImf PARENT_SCOPE)
  set(OpenEXR_INCLUDE_DIR ${OPENEXR_CFG_DIR})
  foreach(dir Half Iex Imath IlmThread IlmImf)
    get_filename_component(fulldir ${dir} ABSOLUTE)
	list(APPEND OpenEXR_INCLUDE_DIR ${fulldir})
  endforeach(dir)
  set(OpenEXR_INCLUDE_DIR ${OpenEXR_INCLUDE_DIR} PARENT_SCOPE)
  
  if(OPENEXR_DLL)
    set(OpenEXR_DEFINITIONS "-DOPENEXR_DLL" PARENT_SCOPE)
  endif(OPENEXR_DLL)

endif(OPENEXR_IS_INTERNAL)
