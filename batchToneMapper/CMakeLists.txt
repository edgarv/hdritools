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

# CMake file for the batch tonemapper, multithreaded and with direct zip input

# The full list of sources
set(SRCS
  ImageInfo.h
  Util.h Util.cpp
  FileInputFilter.h FileInputFilter.cpp
  ZipfileInputFilter.h ZipfileInputFilter.cpp
  ToneMappingFilter.h ToneMappingFilter.cpp
  FloatImageProcessor.h FloatImageProcessor.cpp
  BatchToneMapper.h BatchToneMapper.cpp
  main.cpp
  )
  
# Add the Windows Resource
if (WIN32)
  HDRITOOLS_WIN_RC(RCFILE
    "Parallel HDR tone mapper"
    "batchToneMapper" "batchToneMapper.exe")
  list(APPEND SRCS "${RCFILE}")
endif()
  
# Load the QT configuration
include(${QT_USE_FILE})
  
add_executable(batchToneMapper ${SRCS})
HDRITOOLS_LTCG(batchToneMapper)
target_link_libraries(batchToneMapper ImageIO zipfile ${QT_LIBRARIES})
if(WIN32)
  set_target_properties(batchToneMapper PROPERTIES
    VERSION "${HDRITOOLS_VERSION}")
endif()


# Add the image plugins with the fancy macro
QT_ADD_IMG_PLUGINS( batchToneMapper )

# All the include locations
include_directories(../ImageIO)
include_directories(../zipfile)
include_directories(../3rdparty/tclap/include)
include_directories(${TBB_INCLUDE_DIR})

# Qt already links to zlib, so we don't need to add it again

# TODO Fix this nasty hack (adds the batch tone mapper to the qt4Image bundle!)
if (APPLE AND BUILD_QT4IMAGE AND QT4IMAGE_BUNDLE)
  set (DESTINATION_DIR "qt4Image.app/Contents/MacOS")
else()
  set (DESTINATION_DIR "bin")
endif()
install(TARGETS batchToneMapper
  RUNTIME DESTINATION ${DESTINATION_DIR} COMPONENT "batchtonemapper"
)
