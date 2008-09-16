# Master CMake Project file for the HDR Image Tools
# Edgar Velazquez-Armendariz - eva5@cs.cornell.edu
# August 2008

project(HDRImageTools)


cmake_minimum_required(VERSION 2.6)

# General Configuration: find all the required libraries.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

# Options to select which components to build

# TODO: How to build packages with mixed C++ and Java code?
#             And for this particular one: how to specify the full  java prefix?
option(BUILD_OPENEXR_JNI 
  "Builds the OpenEXR JNI binding." ON)

option(BUILD_OPENEXR_MATLAB
  "Builds the OpenEXR Matlab MEX binaries." ON)
  
option(BUILD_BATCH_TONEMAPPER
  "Builds the command line batch tonemapper." ON)
  
option(BUILD_QT4IMAGE
  "Builds the QT4 successor of glimage." ON)
  
# Sets the required zlib variables if we are using the bundled OpenEXR or if building the batch tonemapper
if(NOT USE_SYSTEM_OPENEXR OR BUILD_BATCH_TONEMAPPER)
  
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
  
endif(NOT USE_SYSTEM_OPENEXR OR BUILD_BATCH_TONEMAPPER)

# At the bare minimum we need to build the imageio library, which depends on
# OpenEXR and Intel Threading Building Blocks
find_package(TBB 2.0 REQUIRED)

if(USE_SYSTEM_OPENEXR)
  find_package(OpenEXR 1.6 REQUIRED)
else(USE_SYSTEM_OPENEXR)
  set(OPENEXR_IS_INTERNAL TRUE)
  add_subdirectory(3rdparty/OpenEXR)
endif(USE_SYSTEM_OPENEXR)

  
# Both the batch renderer and qt4Image depend on qt4
if(BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  find_package(Qt4 REQUIRED)
  include(QtAddImgPlugins)
endif(BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  
# We always build ImageIO
add_subdirectory(ImageIO)

# With the basic configuration options, proceed to the individual configuration
if(BUILD_OPENEXR_JNI)
  #add_subdirectory(OpenEXR_JNI)
endif(BUILD_OPENEXR_JNI)

if(BUILD_OPENEXR_MATLAB)
  #add_subdirectory(OpenEXR_Matlab)
endif(BUILD_OPENEXR_MATLAB)

if(BUILD_BATCH_TONEMAPPER)
  add_subdirectory(zipfile)
  add_subdirectory(batchToneMapper)
endif(BUILD_BATCH_TONEMAPPER)

if(BUILD_QT4IMAGE)
  add_subdirectory(qt4Image)
endif(BUILD_QT4IMAGE)
