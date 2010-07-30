# Cmake file for the ImageIO library. It assumes that TBB and OpenEXR
# have been found and the proper variables are set

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
  ImageComparator.h ImageComparator.cpp
  ImageIO.h ImageIO.cpp
  LDRPixels.h
  OpenEXRIO.h OpenEXRIO.cpp
  Rgb32F.h
  Rgba32F.h Rgba32F.cpp
  rgbe.h rgbe.cpp
  RgbeImage.h
  RgbeIO.h RgbeIO.cpp
  RgbeIOPrivate.h
  sse_mathfun.h
  ToneMapper.h ToneMapper.cpp
  Reinhard02.h Reinhard02.cpp
  PngIO.h PngIO.cpp
  ${LUT_DIRECTORY}/rgbeLUT.h
  Exception.h
  PfmIO.h PfmIO.cpp
  )
  
# Subset of the sources which are the public headers
set(SRCS_PUBLIC
  Image.h
  ImageComparator.h
  ImageIO.h
  LDRPixels.h
  OpenEXRIO.h
  Rgb32F.h
  Rgba32F.h
  rgbe.h
  RgbeImage.h
  RgbeIO.h
  ToneMapper.h
  Reinhard02.h
  StdAfx.h
  Exception.h
  PngIO.h
  PfmIO.h
  )
  
add_library(ImageIO SHARED ${SRCS})
target_link_libraries(ImageIO ${TBB_LIBRARIES} ${OpenEXR_LIBRARIES} ${PNG_LIBRARIES})
include_directories(${TBB_INCLUDE_DIR} ${OpenEXR_INCLUDE_DIR} ${PNG_INCLUDE_DIR})

set_target_properties(ImageIO PROPERTIES 
  PUBLIC_HEADER "${SRCS_PUBLIC}"
  DEBUG_POSTFIX "d"
  )
  
# TODO This should be done only when building it as a shared library in Windows
add_definitions(-DIMAGEIO_EXPORTS)

# OpenEXR definitions (only used in windows dll anyway)
if(OpenEXR_DEFINITIONS)
  add_definitions(${OpenEXR_DEFINITIONS})
endif(OpenEXR_DEFINITIONS)

# Installs this
install(TARGETS ImageIO
  RUNTIME DESTINATION "bin"
  LIBRARY DESTINATION "lib"
  ARCHIVE DESTINATION "lib"
  PUBLIC_HEADER DESTINATION "include"
  )

# Finally adds the subdirectory with the tests if enabled
option(IMAGEIO_BUILD_TEST "Build ImageIO unit test suite" OFF)
if(IMAGEIO_BUILD_TEST)
  add_subdirectory(test)
endif()
