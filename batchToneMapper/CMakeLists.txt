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
set_target_properties(batchToneMapper PROPERTIES
  VERSION "${HDRITOOLS_VERSION}")

# To expands the command line arguments in Windows, see:
# http://msdn.microsoft.com/en-us/library/8bch7bkk.aspx
# Does this work in MSVC older than 2008?
if(MSVC)
  set_target_properties(batchToneMapper PROPERTIES 
    LINK_FLAGS "setargv.obj"
  )
endif(MSVC)

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
