# Master CMake Project file for the HDR Image Tools
# Edgar Velazquez-Armendariz - eva5@cs.cornell.edu
# August 2008

project(HDRImageTools)


cmake_minimum_required(VERSION 2.6.4)
enable_testing()

# General Configuration: find all the required libraries.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# Also add the compatibility modules for CMake < 2.8.3
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_LESS "2.8.3")
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/compat")
endif()

# Options on how to use zlib and openexr
if(WIN32)
  set(USE_SYSTEM_LIBS_DEFAULT OFF)
else(WIN32)
  set(USE_SYSTEM_LIBS_DEFAULT ON)
endif(WIN32)
option(USE_SYSTEM_ZLIB
  "Use the system version of zlib instead of the bundled one."
  ${USE_SYSTEM_LIBS_DEFAULT})
option(USE_SYSTEM_OPENEXR
  "Use the system version of OpenEXR instead of the bundled one."
  ${USE_SYSTEM_LIBS_DEFAULT})

# For UNIX-flavored systems, options for the RPATH
if(UNIX)
  option(USE_FULL_RPATH
    "Use full RPATH information for the installed binaries." OFF)

  if(USE_FULL_RPATH)
    
    # the RPATH to be used when installing
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")

    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	
  else(USE_FULL_RPATH)
    # Use the default settings
    set(CMAKE_INSTALL_RPATH "")
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
  endif(USE_FULL_RPATH)
  
endif()
  
# Options to select which components to build
option(BUILD_OPENEXR_JNI 
  "Builds the OpenEXR JNI binding." OFF)

option(BUILD_OPENEXR_MATLAB
  "Builds the OpenEXR Matlab MEX binaries." OFF)
  
option(BUILD_BATCH_TONEMAPPER
  "Builds the command line batch tonemapper." OFF)
  
option(BUILD_QT4IMAGE
  "Builds the QT4 successor of glimage." OFF)
  
# Extra compiler flags for enabling SSE & SSE2. These will always be enabled.
if(MSVC AND NOT CMAKE_CL_64)
  set(SSE2_COMPILER_FLAGS "/arch:SSE2")
elseif(CMAKE_COMPILER_IS_GNUCXX)
  set(SSE2_COMPILER_FLAGS "-msse -msse2 -msse3 -mfpmath=sse")
endif(MSVC AND NOT CMAKE_CL_64)
set ( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${SSE2_COMPILER_FLAGS}" )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SSE2_COMPILER_FLAGS}" )

# For GCC we always want to use the source fortify options
if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-D_FORTIFY_SOURCE=2")
endif(CMAKE_COMPILER_IS_GNUCXX)
  
# Sets the required zlib variables if we are using the bundled OpenEXR or if building the batch tonemapper
if(NOT USE_SYSTEM_OPENEXR OR BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  
  if(USE_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
	get_filename_component(ZLIB_LIB_FILENAME "${ZLIB_LIBRARIES}" ABSOLUTE)
  else(USE_SYSTEM_ZLIB)
    set(ZLIB_IS_INTERNAL TRUE)
    add_subdirectory(3rdparty/zlib)
    get_filename_component(ZLIB_INCLUDE_DIR "3rdparty/zlib" ABSOLUTE)
    set(ZLIB_LIBRARIES "zlib")
	get_target_property(ZLIB_LIB_FILENAME zlib "LOCATION")
    message(STATUS "Using the internal zlib.")
  endif(USE_SYSTEM_ZLIB)
  
endif(NOT USE_SYSTEM_OPENEXR OR BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)

# We always need to use OpenEXR
if(USE_SYSTEM_OPENEXR)
  find_package(OpenEXR 1.6 REQUIRED)
else(USE_SYSTEM_OPENEXR)
  set(OPENEXR_IS_INTERNAL TRUE)
  add_subdirectory(3rdparty/OpenEXR)
  
  # Also clear the cache variable that might have seet previously by the FindOpenEXR script
  set(OpenEXR_PREFIX_PATH 
    CACHE INTERNAL "Prefix for external OpenEXR not used." FORCE)
  
endif(USE_SYSTEM_OPENEXR)

  
# Both the batch renderer and qt4Image depend on qt4
if(BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  find_package(Qt4 REQUIRED)
  include(QtAddImgPlugins)
  
  # ImageIO depends on TBB
  find_package(TBB 2.0 REQUIRED)
  
  # ImageIO requred libpng
  option(USE_SYSTEM_PNG
	"Use the system version of libpng instead of the bundled one."
	${USE_SYSTEM_LIBS_DEFAULT})
	
  if(USE_SYSTEM_PNG)
    find_package(PNG REQUIRED)
  else(USE_SYSTEM_PNG)
    set(PNG_IS_INTERNAL TRUE)
    add_subdirectory(3rdparty/libpng)
    get_filename_component(PNG_INCLUDE_DIR "3rdparty/libpng" ABSOLUTE)
	set(PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})
    # PNG_LIBRARIES is set within the subdir with the proper target name, but it still depends on zlib
	set(PNG_LIBRARIES ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
    message(STATUS "Using the internal libpng.")
  endif(USE_SYSTEM_PNG)
  
  # We need to build ImageIO
  add_subdirectory(ImageIO)
  
  # Add the subdirectory with the tests if enabled
  option(IMAGEIO_BUILD_TEST "Build ImageIO unit test suite" OFF)
  if(IMAGEIO_BUILD_TEST)
    add_subdirectory(ImageIO_test)
  endif()
  
endif(BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  
# With the basic configuration options, proceed to the individual configuration
if(BUILD_OPENEXR_JNI)
  add_subdirectory(OpenEXR_JNI)
endif(BUILD_OPENEXR_JNI)

if(BUILD_OPENEXR_MATLAB)
  add_subdirectory(OpenEXR_Matlab)
endif(BUILD_OPENEXR_MATLAB)

if(BUILD_BATCH_TONEMAPPER)
  add_subdirectory(zipfile)
  add_subdirectory(batchToneMapper)
endif(BUILD_BATCH_TONEMAPPER)

if(BUILD_QT4IMAGE)
  add_subdirectory(qt4Image)
endif(BUILD_QT4IMAGE)



# Packaging with CPack

# Currently set a faux version, based on the build date
add_executable(getdate getdate.c)
add_custom_command(
  OUTPUT currentdate.txt 
  COMMAND getdate currentdate.txt
  DEPENDS getdate)
add_custom_target(getdate_drive ALL DEPENDS currentdate.txt)
if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/currentdate.txt)
  file(READ ${CMAKE_CURRENT_BINARY_DIR}/currentdate.txt CURRENT_DATE_STR)
  if (CURRENT_DATE_STR MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    message(STATUS "Using package version: ${CURRENT_DATE_STR}")
    set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
    set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
  endif()
endif()

set(CPACK_PACKAGE_VENDOR "Cornell PCG")
set(CPACK_DMG_FORMAT "UDBZ") # UDIF bzip2-compressed image (OS X 10.4+ only)

include(CPack)
