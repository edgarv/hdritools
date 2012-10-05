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

# Cmake file for the ImageIO library. It assumes that TBB and OpenEXR
# have been found and the proper variables are set

# Starting with TBB 2.2, the scheduler initialization is automatic. We detect
# that through the version as previously set up by find_package(TBB)
if (DEFINED TBB_VERSION AND NOT TBB_VERSION VERSION_LESS "2.2")
  add_definitions("-DIMAGEIO_TBB_DEFAULT_INIT")
endif()

# Generate the rgbe LUT
add_executable(rgbeLUT rgbeLUT.cpp)

# Directory with the LUTs
set(LUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${LUT_DIRECTORY} )

add_custom_command(OUTPUT ${LUT_DIRECTORY}/rgbeLUT.h
  COMMAND rgbeLUT > ${LUT_DIRECTORY}/rgbeLUT.h
  COMMENT "Generating rgbe LUT..."
  )

# The full list of sources
set(SRCS
  dllmain.cpp StdAfx.h
  Image.h
  ImageSoA.h ImageSoA.cpp
  ImageComparator.h ImageComparator.cpp
  ImageIO.h ImageIO.cpp
  ImageIterators.h
  LDRPixels.h
  OpenEXRIO.h OpenEXRIO.cpp
  Rgb32F.h
  Rgba32F.h Rgba32F.cpp
  rgbe.h rgbe.cpp
  RgbeImage.h
  RgbeIO.h RgbeIO.cpp
  RgbeIOPrivate.h
  sse_mathfun.h
  Amaths.h Amaths.inl
  ToneMapper.h ToneMapper.cpp
  ToneMapperSoA.h ToneMapperSoA.cpp
  Reinhard02.h Reinhard02.cpp
  PngIO.h PngIO.cpp
  ${LUT_DIRECTORY}/rgbeLUT.h
  Exception.h
  PfmIO.h PfmIO.cpp
  LoadHDR.h LoadHDR.cpp
  Vec4f.h
  Vec4i.h
  
  # Also include the version header, defined in the root directory
  "${HDRITOOLS_VERSION_FILENAME}"
  )
  
# USE_AVX option is defined in the top level CMakeLists.txt
if (USE_AVX)
  list (APPEND SRCS Vec8f.h Vec8i.h)
endif()
  
# Subset of the sources which are the public headers
set(SRCS_PUBLIC
  Image.h
  ImageSoA.h
  ImageComparator.h
  ImageIO.h
  ImageIterators.h
  LDRPixels.h
  OpenEXRIO.h
  Rgb32F.h
  Rgba32F.h
  rgbe.h
  RgbeImage.h
  RgbeIO.h
  ToneMapper.h
  ToneMapperSoA.h
  Reinhard02.h
  StdAfx.h
  Exception.h
  PngIO.h
  PfmIO.h
  LoadHDR.h
  
  # Also include the version header, defined in the root directory
  "${HDRITOOLS_VERSION_FILENAME}"
  )

# Add the Windows Resource
if (WIN32)
  HDRITOOLS_WIN_RC(RCFILE
    "Utility library for image operations, tone mapping and HDR files IO."
    "pcgImageIO" "pcgImageIO.dll")
  list(APPEND SRCS "${RCFILE}")
endif()
  
add_library(ImageIO SHARED ${SRCS})
HDRITOOLS_LTCG(ImageIO)
target_link_libraries(ImageIO ${TBB_LIBRARIES} ${OpenEXR_LIBRARIES} ${PNG_LIBRARIES})
include_directories(${PNG_INCLUDE_DIR} ${OpenEXR_INCLUDE_DIR} ${TBB_INCLUDE_DIR})

set_target_properties(ImageIO PROPERTIES
  OUTPUT_NAME "pcgImageIO"
  DEBUG_POSTFIX "d"
  VERSION "${HDRITOOLS_VERSION}"
  SOVERSION "${HDRITOOLS_VERSION_MAJOR}.${HDRITOOLS_VERSION_MINOR}"
)

if(IMAGEIO_DEV_INSTALL)
  set_target_properties(ImageIO PROPERTIES
    PUBLIC_HEADER "${SRCS_PUBLIC}"
	FRAMEWORK TRUE
  )
endif()
  
# TODO This should be done only when building it as a shared library in Windows
add_definitions(-DIMAGEIO_EXPORTS)

# Per Appendix C of the TBB documentation, make windows.h not to define min/max
if(WIN32)
  add_definitions(-DNOMINMAX)
endif()

# Set up the Info.plist variables
if (APPLE)
  set(MACOSX_FRAMEWORK_IDENTIFIER "edu.cornell.graphics.pcgImageIO")
  set(MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${HDRITOOLS_VERSION})
  set(MACOSX_FRAMEWORK_BUNDLE_VERSION ${HDRITOOLS_MACLS_VERSION})
endif()

# OpenEXR definitions (only used in windows dll anyway)
if(OpenEXR_DEFINITIONS)
  add_definitions(${OpenEXR_DEFINITIONS})
endif()

# Installs this
if(IMAGEIO_DEV_INSTALL)
  install(TARGETS ImageIO
    RUNTIME DESTINATION "bin"  COMPONENT "imageio"
    LIBRARY DESTINATION "lib"  COMPONENT "imageio"
    FRAMEWORK DESTINATION "."  COMPONENT "imageio_dev"
    ARCHIVE DESTINATION "lib"  COMPONENT "imageio_dev"
    PUBLIC_HEADER DESTINATION "include/pcgImageIO"  COMPONENT "imageio_dev"
  )
else()
  install(TARGETS ImageIO
    RUNTIME DESTINATION "bin"  COMPONENT "imageio"
    LIBRARY DESTINATION "lib"  COMPONENT "imageio"
  )
endif()
