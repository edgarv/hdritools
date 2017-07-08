# ============================================================================
#   HDRITools - High Dynamic Range Image Tools
#   Copyright 2008-2011 Program of Computer Graphics, Cornell University
#
#   Distributed under the OSI-approved MIT License (the "License");
#   see accompanying file LICENSE for details.
#
#   This software is distributed WITHOUT ANY WARRANTY; without even the
#   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#   See the License for more information.
#  ---------------------------------------------------------------------------
#  Primary author:
#      Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
# ============================================================================

project(HDRImageTools)


cmake_minimum_required(VERSION 2.8.12)
enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# General Configuration: find all the required libraries.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Prefer new behavior for visibility when using CMake >= 3.3
if (POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif()

# Set the version-related variables
include(HDRIToolsGetVersionInfo)
HDRITOOLS_GET_VERSION_INFO()

# Create the version header
set(HDRITOOLS_VERSION_FILENAME
  "${CMAKE_CURRENT_BINARY_DIR}/headers/HDRITools_version.h")
configure_file(HDRITools_version.h.cmake "${HDRITOOLS_VERSION_FILENAME}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/headers")


# Options on how to use zlib and openexr
if(WIN32)
  set(USE_SYSTEM_ZLIB_DEFAULT OFF)
  set(USE_SYSTEM_OPENEXR_DEFAULT OFF)
else()
  set(USE_SYSTEM_ZLIB_DEFAULT ON)
  if (APPLE)
    set(USE_SYSTEM_OPENEXR_DEFAULT OFF)
  else()
    set(USE_SYSTEM_OPENEXR_DEFAULT ON)
  endif()
endif()
option(USE_SYSTEM_ZLIB
  "Use the system version of zlib instead of the bundled one."
  ${USE_SYSTEM_ZLIB_DEFAULT})
option(USE_SYSTEM_OPENEXR
  "Use the system version of OpenEXR instead of the bundled one."
  ${USE_SYSTEM_OPENEXR_DEFAULT})

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
    
  else()
    # Use the default settings
    set(CMAKE_INSTALL_RPATH "")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
  endif()
  
endif()


# Unix-style system should install additional components under "share"
if (APPLE OR (WIN32 AND NOT CYGWIN))
  set(HDRITOOLS_SHARE_DIR ".")
else()
  set(HDRITOOLS_SHARE_DIR "share/hdritools")
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
  
option(BUILD_UTILS
  "Builds small, limited function utilities." OFF)


# Check for the AVX intrinsics header
include(CheckIncludeFile)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
CHECK_INCLUDE_FILE("immintrin.h" HAVE_IMMINTRIN_H)
CMAKE_DEPENDENT_OPTION(USE_AVX "Compile using AVX instructions." OFF
  HAVE_IMMINTRIN_H OFF)
if (USE_AVX)
  add_definitions("-DPCG_USE_AVX")
endif()

# Enable AVX2 as well
CMAKE_DEPENDENT_OPTION(USE_AVX2 "Compile using AVX2 instructions." OFF
  USE_AVX OFF)
if (USE_AVX2)
  add_definitions("-DPCG_USE_AVX2")
endif()
  
# Extra compiler flags for enabling SSE & SSE2. These will always be enabled.
if(MSVC)
  # MSVC 2013 Update 2 introduced full compilation using AVX2
  if (NOT "${MSVC_VERSION}" LESS 1800)
    CHECK_CXX_COMPILER_FLAG("/arch:AVX2" HAVE_MSVC_AVX2)
  endif()
  
  if (USE_AVX2 AND HAVE_MSVC_AVX2)
    set(SIMD_COMPILER_FLAGS "/arch:AVX2")
  elseif (USE_AVX)
    set(SIMD_COMPILER_FLAGS "/arch:AVX")
  elseif(NOT CMAKE_CL_64)
    set(SIMD_COMPILER_FLAGS "/arch:SSE2")
  endif()
elseif(NOT MSVC OR NOT WIN32)
  CHECK_CXX_COMPILER_FLAG("-march=core-avx2" HAVE_CXX_CORE_AVX2)

  if (USE_AVX2 AND HAVE_CXX_CORE_AVX2)
    set(SIMD_COMPILER_FLAGS "-march=core-avx2")
  elseif (USE_AVX)
    set(SIMD_COMPILER_FLAGS "-mavx")
  else()
    set(SIMD_COMPILER_FLAGS "-msse3")
  endif()
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(SIMD_COMPILER_FLAGS "${SIMD_COMPILER_FLAGS} -mfpmath=sse")
  endif()
endif()
set (CMAKE_C_FLAGS   "${SIMD_COMPILER_FLAGS} ${CMAKE_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${SIMD_COMPILER_FLAGS} ${CMAKE_CXX_FLAGS}")

# For GCC 4.1+ we always want to use the source fortify options
if(CMAKE_COMPILER_IS_GNUCXX)
  include(CheckCSourceCompiles)
  CHECK_C_SOURCE_COMPILES("
#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#if GCC_VERSION < 401
#error \"gcc is not 4.1 or newer\"
#endif

int main(int argc, char **argv) { return 0; }
" HAS_GCC41)

  if(HAS_GCC41)
    add_definitions("-D_FORTIFY_SOURCE=2")
  endif()
endif()


# Option and helper macro to enable global optimization on targets.
# As of CMake 2.8.4 this seems to work only on the command line Intel compiler.
# CMake 3.9 introduced support for (at least) gcc, however the configuration will
# fail if CMake does not know how to enable LTCG for the current toolchain
option(USE_INTERPROCEDURAL_OPTIMIZATION
  "Enable global optimization/LTCG on Release builds." OFF)
macro(HDRITOOLS_LTCG targetname)
  set_target_properties(${targetname} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ${USE_INTERPROCEDURAL_OPTIMIZATION})
endmacro()

  
# Sets the required zlib variables if we are using the bundled OpenEXR or if
# building the batch tonemapper
if(NOT USE_SYSTEM_OPENEXR OR BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  
  if(USE_SYSTEM_ZLIB)
    find_package(ZLIB REQUIRED)
    get_filename_component(ZLIB_LIB_FILENAME "${ZLIB_LIBRARIES}" ABSOLUTE)
  else()
    set(ZLIB_IS_INTERNAL TRUE)
    set(ZLIB_ZCONF_CMAKE_DIR "${CMAKE_CURRENT_BINARY_DIR}/headers/")
    add_subdirectory(3rdparty/zlib)
    get_filename_component(ZLIB_MAIN_DIR "3rdparty/zlib" ABSOLUTE)
    set(ZLIB_INCLUDE_DIRS ${ZLIB_MAIN_DIR} ${ZLIB_ZCONF_CMAKE_DIR})
    set(ZLIB_LIBRARIES "zlib")
    get_target_property(ZLIB_LIB_FILENAME zlib "LOCATION")
    set_target_properties(zlib PROPERTIES FOLDER "3rdparty")
    message(STATUS "Using the internal zlib.")
  endif()
  
endif()

# We always need to use OpenEXR
if(USE_SYSTEM_OPENEXR)
  find_package(OpenEXR 1.6 REQUIRED)
else()
  set(OPENEXR_IS_INTERNAL TRUE)
  add_subdirectory(3rdparty/OpenEXR)
  
  # Also clear the cache variable that might have been previously set by
  # the FindOpenEXR script
  set(OpenEXR_ROOT_DIR 
    CACHE INTERNAL "Root dir for external OpenEXR not used." FORCE)
endif()


if (WIN32)
  # Helper macro to add the generic resource info for Windows. It sets the 
  # "out_variable" to the path of the generated resource file
  get_filename_component(HDRITOOLS_RC_TEMPLATE
    "win32/info.rc.template.in" ABSOLUTE)
  function(HDRITOOLS_WIN_RC out_variable description product_name filename)
    set(RC_VERSION_COMMA ${HDRITOOLS_VERSION_MAJOR},${HDRITOOLS_VERSION_MINOR},${HDRITOOLS_VERSION_PATCH},0)
    if(HDRITOOLS_HAS_VALID_REV)
      set(RC_VERSION ${HDRITOOLS_VERSION}-${HDRITOOLS_VERSION_BUILD}hg${HDRITOOLS_REV_ID})
    else()
      set(RC_VERSION ${HDRITOOLS_VERSION})
    endif()
    set(RC_COMPANY "Program of Computer Graphics, Cornell University")
    set(RC_COPYRIGHT "Copyright (c) 2008-2012, Program of Computer Graphics, Cornell University")
    set(RC_DESCRIPTION ${description})
    set(RC_NAME ${product_name})
    set(RC_FILENAME ${filename})
    if(filename MATCHES ".+\\.[eE][xX][eE]$")
      set(RC_FILETYPE "VFT_APP")
    elseif(filename MATCHES ".+\\.[dD][lL][lL]$")
      set(RC_FILETYPE "VFT_DLL")
    endif()
    
    configure_file("${HDRITOOLS_RC_TEMPLATE}"
      "${CMAKE_CURRENT_BINARY_DIR}/res/${product_name}.rc")
    set(${out_variable}
      "${CMAKE_CURRENT_BINARY_DIR}/res/${product_name}.rc" PARENT_SCOPE)
  endfunction()
endif()

  
# Both the batch renderer and qt4Image depend on qt4
if(BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  set(QT_MIN_VERSION "4.5.0")
  find_package(Qt4 REQUIRED QtCore QtGui)
  include(QtAddImgPlugins)
endif()

# Allow to build ImageIO even if not required by anything else
CMAKE_DEPENDENT_OPTION(BUILD_IMAGEIO "Build the ImageIO library" OFF
  "NOT BUILD_BATCH_TONEMAPPER;NOT BUILD_QT4IMAGE;NOT BUILD_UTILS" ON)
if (BUILD_IMAGEIO)

  # ImageIO depends on TBB
  find_package(TBB 2.0 REQUIRED)
  
  # ImageIO requires libpng
  if (WIN32 OR APPLE)
    set(USE_SYSTEM_PNG_DEFAULT OFF)
  else()
    set(USE_SYSTEM_PNG_DEFAULT ON)
  endif()
  option(USE_SYSTEM_PNG
    "Use the system version of libpng instead of the bundled one."
    ${USE_SYSTEM_PNG_DEFAULT})
    
  if(USE_SYSTEM_PNG)
    find_package(PNG REQUIRED)
  else()
    set(PNG_IS_INTERNAL TRUE)
    add_subdirectory(3rdparty/libpng)
    get_filename_component(PNG_INCLUDE_DIR "3rdparty/libpng" ABSOLUTE)
    set(PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
    # PNG_LIBRARIES is set within the subdir with the proper target name, but it still depends on zlib
    set_target_properties(${PNG_LIBRARIES} PROPERTIES FOLDER "3rdparty")
    set(PNG_LIBRARIES ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
    message(STATUS "Using the internal libpng.")
  endif()
  
  # Option to install the development files. Default to false in MSVC
  if (MSVC)
    set (IMAGEIO_DEV_INSTALL_DEFAULT OFF)
  else()
    set (IMAGEIO_DEV_INSTALL_DEFAULT ON)
  endif()
  option(IMAGEIO_DEV_INSTALL "Install the development components for ImageIO."
    ${IMAGEIO_DEV_INSTALL_DEFAULT})
  
  # We need to build ImageIO
  add_subdirectory(ImageIO)
  
  # Add the subdirectory with the tests if enabled
  option(IMAGEIO_BUILD_TEST "Build ImageIO unit test suite" OFF)
  if(IMAGEIO_BUILD_TEST)
    add_subdirectory(ImageIO_test)
  endif()
  
endif()
  
# With the basic configuration options, proceed to the individual configuration
if(BUILD_OPENEXR_JNI)
  add_subdirectory(OpenEXR_JNI)
endif()

if(BUILD_OPENEXR_MATLAB)
  add_subdirectory(OpenEXR_Matlab)
endif()

if(BUILD_BATCH_TONEMAPPER)
  add_subdirectory(zipfile)
  add_subdirectory(batchToneMapper)
endif()

if(BUILD_UTILS)
  add_subdirectory(utils)
endif()

if(BUILD_QT4IMAGE)
  add_subdirectory(qt4Image)
endif()



# WiX Package for Windows
# Currently this functionally makes lots of assumption about the final
# layout of the different features.
if(WIN32)
  option(BUILD_WINDOWS_MSI_PACKAGE "Build Windows Installer package using WiX" OFF)
  if(BUILD_WINDOWS_MSI_PACKAGE)
    add_subdirectory("win32/wix")
  endif()
endif()


# Packaging with CPack

# Inform about the version
if (HDRITOOLS_HAS_VALID_REV)
  message(STATUS "HDRITools package version: ${HDRITOOLS_VERSION}-hg${HDRITOOLS_REV_ID} (${HDRITOOLS_DATE})")
else()
  message(STATUS "HDRITools package version: ${HDRITOOLS_VERSION} (${HDRITOOLS_DATE})")
endif()

# The rest of the CPack config is in a different file
include(HDRIToolsCpackConfig.cmake)
include(CPack)

# Define the groups of components
cpack_add_component_group("apps"  DISPLAY_NAME "Applications")
cpack_add_component_group("bindings" DISPLAY_NAME "Bindings")

# Components description
if (ZLIB_IS_INTERNAL AND ZLIB_SHARED_LIB)
  set(ZLIB_COMPONENT "zlib")
  cpack_add_component(${ZLIB_COMPONENT} HIDDEN)
endif()

if (OPENEXR_IS_INTERNAL AND OPENEXR_SHARED_LIBS)
  set(OPENEXR_COMPONENT "openexr")
  cpack_add_component(${OPENEXR_COMPONENT} HIDDEN  DEPENDS ${ZLIB_COMPONENT})
endif()

if (PNG_IS_INTERNAL AND PNG_SHARED_LIB)
  set(PNG_COMPONENT "libpng")
  cpack_add_component(${PNG_COMPONENT} HIDDEN DEPENDS ${ZLIB_COMPONENT})
endif()


cpack_add_component("imageio"
  DISPLAY_NAME "ImageIO"
  HIDDEN
  DEPENDS ${OPENEXR_COMPONENT} ${PNG_COMPONENT}
)

if (IMAGEIO_DEV_INSTALL)
  cpack_add_component("imageio_dev"
    DISABLED
    DISPLAY_NAME "ImageIO-dev"
    DEPENDS "imageio"
  )
endif()

cpack_add_component("batchtonemapper"
  DISPLAY_NAME "Batch tone mapper"
  GROUP   "apps"
  DEPENDS "imageio"
)
cpack_add_component("qt4image"
  DISPLAY_NAME "Qt4Image"
  GROUP   "apps"
  DEPENDS "imageio"
)

cpack_add_component("utils"
  DISPLAY_NAME "Simple command line utilities"
  GROUP   "apps"
  DEPENDS "imageio"
)

cpack_add_component("openexrjni"
  DISPLAY_NAME "OpenEXR-JNI"
  GROUP   "bindings"
  DEPENDS ${OPENEXR_COMPONENT}
)
cpack_add_component("openexrmex"
  DISPLAY_NAME "OpenEXR-Matlab"
  GROUP   "bindings"
  DEPENDS ${OPENEXR_COMPONENT}
)
