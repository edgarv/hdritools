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
  
# Load the QT configuration
include(${QT_USE_FILE})
  
add_executable(batchToneMapper ${SRCS})
target_link_libraries(batchToneMapper ImageIO zipfile ${QT_LIBRARIES})

# Add the image plugins with the fancy macro
QT_ADD_IMG_PLUGINS( batchToneMapper )

# All the include locations
include_directories(../ImageIO)
include_directories(../zipfile)
include_directories(../3rdparty/tclap/include)
include_directories(${TBB_INCLUDE_DIR})

# Qt already links to zlib, so we don't need to add it again

# Installs this
install(TARGETS batchToneMapper
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  )
