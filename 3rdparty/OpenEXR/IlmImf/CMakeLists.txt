# Description of the IlmImf library

# Programs which build the LUTs
add_executable(b44ExpLogTable b44ExpLogTable.cpp)
target_link_libraries(b44ExpLogTable Half)
OPENEXR_TOOL_FOLDER(b44ExpLogTable)

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
  ImfAcesFile.h
  ImfArray.h
  ImfAttribute.h
  ImfB44Compressor.h
  ImfBoxAttribute.h
  ImfCRgbaFile.h
  ImfChannelList.h
  ImfChannelListAttribute.h
  ImfChromaticities.h
  ImfChromaticitiesAttribute.h
  ImfCompression.h
  ImfCompressionAttribute.h
  ImfConvert.h
  ImfDoubleAttribute.h
  ImfEnvmap.h
  ImfEnvmapAttribute.h
  ImfFloatAttribute.h
  ImfFrameBuffer.h
  ImfFramesPerSecond.h
  ImfHeader.h
  ImfHuf.h
  ImfIO.h
  ImfInputFile.h
  ImfInt64.h
  ImfIntAttribute.h
  ImfKeyCode.h
  ImfKeyCodeAttribute.h
  ImfLineOrder.h
  ImfLineOrderAttribute.h
  ImfLut.h
  ImfMatrixAttribute.h
  ImfMultiView.h
  ImfName.h
  ImfOpaqueAttribute.h
  ImfOutputFile.h
  ImfPixelType.h
  ImfPreviewImage.h
  ImfPreviewImageAttribute.h
  ImfRational.h
  ImfRationalAttribute.h
  ImfRgba.h
  ImfRgbaFile.h
  ImfRgbaYca.h
  ImfStandardAttributes.h
  ImfStringAttribute.h
  ImfStringVectorAttribute.h
  ImfTestFile.h
  ImfThreading.h
  ImfTileDescription.h
  ImfTileDescriptionAttribute.h
  ImfTiledInputFile.h
  ImfTiledOutputFile.h
  ImfTiledRgbaFile.h
  ImfTimeCode.h
  ImfTimeCodeAttribute.h
  ImfVecAttribute.h
  ImfVersion.h
  ImfWav.h
  ImfXdr.h
  )
  
set(ILMIMF_SOURCES
  ImfAcesFile.cpp
  ImfAttribute.cpp
  ImfAutoArray.h
  ImfB44Compressor.cpp
  ImfBoxAttribute.cpp
  ImfCRgbaFile.cpp
  ImfChannelList.cpp
  ImfChannelListAttribute.cpp
  ImfCheckedArithmetic.h
  ImfChromaticities.cpp
  ImfChromaticitiesAttribute.cpp
  ImfCompressionAttribute.cpp
  ImfCompressor.h ImfCompressor.cpp
  ImfConvert.cpp
  ImfDoubleAttribute.cpp
  ImfEnvmap.cpp
  ImfEnvmapAttribute.cpp
  ImfFloatAttribute.cpp
  ImfFrameBuffer.cpp
  ImfFramesPerSecond.cpp
  ImfHeader.cpp
  ImfHuf.cpp
  ImfIO.cpp
  ImfInputFile.cpp
  ImfIntAttribute.cpp
  ImfKeyCode.cpp
  ImfKeyCodeAttribute.cpp
  ImfLineOrderAttribute.cpp
  ImfLut.cpp
  ImfMatrixAttribute.cpp
  ImfMisc.h ImfMisc.cpp
  ImfMultiView.cpp
  ImfOpaqueAttribute.cpp
  ImfOutputFile.cpp
  ImfPizCompressor.h ImfPizCompressor.cpp
  ImfPreviewImage.cpp
  ImfPreviewImageAttribute.cpp
  ImfPxr24Compressor.h ImfPxr24Compressor.cpp
  ImfRational.cpp
  ImfRationalAttribute.cpp
  ImfRgbaFile.cpp
  ImfRgbaYca.cpp
  ImfRleCompressor.h ImfRleCompressor.cpp
  ImfScanLineInputFile.h ImfScanLineInputFile.cpp
  ImfStandardAttributes.cpp
  ImfStdIO.h ImfStdIO.cpp
  ImfStringAttribute.cpp
  ImfStringVectorAttribute.cpp
  ImfTestFile.cpp
  ImfThreading.cpp
  ImfTileDescriptionAttribute.cpp
  ImfTileOffsets.cpp
  ImfTiledInputFile.cpp
  ImfTiledMisc.cpp
  ImfTiledOutputFile.cpp
  ImfTiledRgbaFile.cpp
  ImfTimeCode.cpp
  ImfTimeCodeAttribute.cpp
  ImfVecAttribute.cpp
  ImfVersion.cpp
  ImfWav.cpp
  ImfZipCompressor.h ImfZipCompressor.cpp
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
  ${OPENEXR_VERSION} ${OPENEXR_SOVERSION} "2002" ${ZLIB_EXTRA_ARG})

# Link against zlib as it was found in the top level CMakeLists.txt
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(IlmImf Half Iex IlmThread ${ZLIB_LIBRARIES})

# Disable recurrent warnings in MSVC
MSVC_COMPILE_FLAGS(IlmImf "/wd4244 /wd4018 /wd4996 /wd4800 /wd4267")
