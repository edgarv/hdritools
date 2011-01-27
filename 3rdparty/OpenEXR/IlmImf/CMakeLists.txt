# Description of the IlmImf library

# Programs which build the LUTs
add_executable(b44ExpLogTable b44ExpLogTable.cpp)
target_link_libraries(b44ExpLogTable Half)

# Directory with the LUTs
set(LUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${LUT_DIRECTORY} )

# We set the working directory to the one containing the half dll (not an issue with the static libs)
# TODO This might fail in Linux! should we build a static version of half for this?
get_target_property(_halfLoc Half "LOCATION")
get_filename_component(HALF_OUT_DIR "${_halfLoc}" PATH)
add_custom_command(OUTPUT ${LUT_DIRECTORY}/b44ExpLogTable.h
  COMMAND b44ExpLogTable > ${LUT_DIRECTORY}/b44ExpLogTable.h
  WORKING_DIRECTORY "${HALF_OUT_DIR}"
  COMMENT "Generating b44 Exp->Log LUT..."
  )

set(ILMIMF_HEADERS
  ImfAttribute.h ImfBoxAttribute.h
  ImfCRgbaFile.h ImfChannelList.h
  ImfChannelListAttribute.h
  ImfCompressionAttribute.h
  ImfDoubleAttribute.h ImfFloatAttribute.h
  ImfFrameBuffer.h ImfHeader.h ImfIO.h
  ImfInputFile.h ImfIntAttribute.h
  ImfLineOrderAttribute.h ImfMatrixAttribute.h
  ImfOpaqueAttribute.h ImfOutputFile.h
  ImfRgbaFile.h ImfStringAttribute.h
  ImfVecAttribute.h ImfHuf.h ImfWav.h ImfLut.h
  ImfArray.h ImfCompression.h ImfLineOrder.h
  ImfName.h ImfPixelType.h ImfVersion.h ImfXdr.h
  ImfConvert.h ImfPreviewImage.h
  ImfPreviewImageAttribute.h ImfChromaticities.h
  ImfChromaticitiesAttribute.h
  ImfKeyCode.h ImfKeyCodeAttribute.h
  ImfTimeCode.h ImfTimeCodeAttribute.h
  ImfRational.h ImfRationalAttribute.h
  ImfFramesPerSecond.h
  ImfStandardAttributes.h
  ImfEnvmap.h
  ImfEnvmapAttribute.h
  ImfInt64.h ImfRgba.h
  ImfTileDescription.h
  ImfTileDescriptionAttribute.h
  ImfTiledInputFile.h
  ImfTiledOutputFile.h ImfTiledRgbaFile.h
  ImfRgbaYca.h
  ImfTestFile.h ImfThreading.h
  ImfB44Compressor.h
  ImfAcesFile.h
  ImfCheckedArithmetic.h
  ImfMultiView.h
  ImfStringVectorAttribute.h
  )
  
set(ILMIMF_SOURCES
  ImfAttribute.cpp ImfBoxAttribute.cpp ImfCRgbaFile.cpp
  ImfChannelList.cpp ImfChannelListAttribute.cpp
  ImfFloatAttribute.cpp ImfFrameBuffer.cpp
  ImfHeader.cpp ImfIO.cpp ImfInputFile.cpp
  ImfIntAttribute.cpp ImfLineOrderAttribute.cpp
  ImfMatrixAttribute.cpp ImfOpaqueAttribute.cpp
  ImfOutputFile.cpp ImfRgbaFile.cpp
  ImfStringAttribute.cpp ImfVecAttribute.cpp ImfHuf.cpp
  ImfThreading.cpp
  ImfWav.cpp ImfLut.cpp ImfCompressor.cpp
  ImfRleCompressor.cpp ImfZipCompressor.cpp
  ImfPizCompressor.cpp ImfB44Compressor.cpp ImfMisc.cpp
  ImfCompressionAttribute.cpp ImfDoubleAttribute.cpp
  ImfCompressor.h ImfRleCompressor.h ImfZipCompressor.h
  ImfPizCompressor.h ImfMisc.h ImfAutoArray.h
  ImfPxr24Compressor.cpp ImfPxr24Compressor.h
  ImfConvert.cpp ImfPreviewImage.cpp
  ImfPreviewImageAttribute.cpp
  ImfVersion.cpp
  ImfChromaticities.cpp
  ImfChromaticitiesAttribute.cpp
  ImfKeyCode.cpp
  ImfKeyCodeAttribute.cpp
  ImfTimeCode.cpp
  ImfTimeCodeAttribute.cpp
  ImfRational.cpp
  ImfRationalAttribute.cpp
  ImfFramesPerSecond.cpp
  ImfStandardAttributes.cpp
  ImfStdIO.cpp ImfStdIO.h ImfEnvmap.cpp
  ImfEnvmapAttribute.cpp
  ImfScanLineInputFile.cpp
  ImfScanLineInputFile.h ImfTiledInputFile.cpp
  ImfTiledMisc.cpp ImfTiledOutputFile.cpp
  ImfTiledRgbaFile.cpp ImfTileDescriptionAttribute.cpp
  ImfTileOffsets.cpp
  ImfRgbaYca.cpp
  ImfTestFile.cpp
  ImfAcesFile.cpp
  ImfMultiView.cpp
  ImfStringVectorAttribute.cpp
  ${LUT_DIRECTORY}/b44ExpLogTable.h
  )


# To create the DLL on windows we also depend on zlib. If it is static we need to add the
# library explictly
if(WIN32 AND ZLIB_LIB_FILENAME MATCHES "zlib(d)?\\.lib")
  set(ZLIB_EXTRA_ARG ${ZLIB_LIB_FILENAME})
endif()
  
# Creates the special target 
ADD_OPENEXR_LIBRARY(IlmImf ILMIMF_HEADERS ILMIMF_SOURCES
  "IlmImf is a library that reads and writes OpenEXR images."
  ${OPENEXR_VERSION} "2002" ${ZLIB_EXTRA_ARG})

# Link against zlib as it was found in the top level CMakeLists.txt
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(IlmImf Half Iex IlmThread "${ZLIB_LIBRARIES}")

# Disable recurrent warnings in MSVC
MSVC_COMPILE_FLAGS(IlmImf "/wd4244 /wd4018 /wd4996 /wd4800 /wd4267")
