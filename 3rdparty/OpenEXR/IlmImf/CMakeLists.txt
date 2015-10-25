# Description of the IlmImf library

find_package(ZLIB REQUIRED)

# Programs which build the LUTs
add_executable(b44ExpLogTable b44ExpLogTable.cpp)
target_link_libraries(b44ExpLogTable Half)

# Directory with the LUTs
set(LUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lut")
file(MAKE_DIRECTORY "${LUT_DIRECTORY}")
include_directories("${LUT_DIRECTORY}")

# TODO Use CMake 2.8.12 generator expressions instead
# We set the working directory to the one containing the half dll (not an
# issue with the static libs). On gcc there is no problem because CMake uses
# the full rpath to dependent libraries during the build process.
get_target_property(_halfLoc Half "LOCATION")
get_filename_component(HALF_OUT_DIR "${_halfLoc}" PATH)
add_custom_command(OUTPUT ${LUT_DIRECTORY}/b44ExpLogTable.h
  COMMAND b44ExpLogTable > ${LUT_DIRECTORY}/b44ExpLogTable.h
  WORKING_DIRECTORY "${HALF_OUT_DIR}"
  COMMENT "Generating b44 Exp->Log LUT..."
  VERBATIM
)

add_executable(dwaLookups dwaLookups.cpp)
target_link_libraries(dwaLookups Half IlmThread)
if (WIN32 AND OPENEXR_SHARED_LIBS)
  # We need to find Half, IlmThread and Iex at runtime, so we will copy them
  file(MAKE_DIRECTORY "${LUT_DIRECTORY}")
  add_custom_command(OUTPUT "${LUT_DIRECTORY}/dwaLookups.h"
    COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:Half>      "${LUT_DIRECTORY}"
	COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:IlmThread> "${LUT_DIRECTORY}"
	COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:Iex>       "${LUT_DIRECTORY}"
	COMMAND dwaLookups > "${LUT_DIRECTORY}/dwaLookups.h"
	COMMAND "${CMAKE_COMMAND}" -E remove
	        "${LUT_DIRECTORY}/$<TARGET_FILE_NAME:Half>"
			"${LUT_DIRECTORY}/$<TARGET_FILE_NAME:IlmThread>"
	        "${LUT_DIRECTORY}/$<TARGET_FILE_NAME:Iex>"
	COMMENT "Generating acceleration lookup tables for for Imf::DwaCompressor..."
	WORKING_DIRECTORY "${LUT_DIRECTORY}"
	VERBATIM
  )
else()
  # Standard command with static libraries/rpath
  add_custom_command(OUTPUT ${LUT_DIRECTORY}/dwaLookups.h
    COMMAND dwaLookups > ${LUT_DIRECTORY}/dwaLookups.h
    COMMENT "Generating acceleration lookup tables for for Imf::DwaCompressor..."
    VERBATIM
  )
endif()


# Public headers
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
  ImfCompositeDeepScanLine.h
  ImfCompression.h
  ImfCompressionAttribute.h
  ImfConvert.h
  ImfDeepCompositing.h
  ImfDeepFrameBuffer.h
  ImfDeepImageState.h
  ImfDeepImageStateAttribute.h
  ImfDeepScanLineInputFile.h
  ImfDeepScanLineInputPart.h
  ImfDeepScanLineOutputFile.h
  ImfDeepScanLineOutputPart.h
  ImfDeepTiledInputFile.h
  ImfDeepTiledInputPart.h
  ImfDeepTiledOutputFile.h
  ImfDeepTiledOutputPart.h
  ImfDoubleAttribute.h
  ImfEnvmap.h
  ImfEnvmapAttribute.h
  ImfExport.h
  ImfFastHuf.h
  ImfFloatAttribute.h
  ImfFloatVectorAttribute.h
  ImfForward.h
  ImfFrameBuffer.h
  ImfFramesPerSecond.h
  ImfGenericInputFile.h
  ImfGenericOutputFile.h
  ImfHeader.h
  ImfHuf.h
  ImfIO.h
  ImfInputFile.h
  ImfInputPart.h
  ImfInt64.h
  ImfIntAttribute.h
  ImfKeyCode.h
  ImfKeyCodeAttribute.h
  ImfLineOrder.h
  ImfLineOrderAttribute.h
  ImfLut.h
  ImfMatrixAttribute.h
  ImfMisc.h
  ImfMultiPartInputFile.h
  ImfMultiPartOutputFile.h
  ImfMultiView.h
  ImfName.h
  ImfNamespace.h
  ImfOpaqueAttribute.h
  ImfOutputFile.h
  ImfOutputPart.h
  ImfPartHelper.h
  ImfPartType.h
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
  ImfTiledInputPart.h
  ImfTiledOutputFile.h
  ImfTiledOutputPart.h
  ImfTiledRgbaFile.h
  ImfTimeCode.h
  ImfTimeCodeAttribute.h
  ImfVecAttribute.h
  ImfVersion.h
  ImfWav.h
  ImfXdr.h
)

# All sources, including the public headers
set(ILMIMF_SOURCES
  "${LUT_DIRECTORY}/b44ExpLogTable.h"
  "${LUT_DIRECTORY}/dwaLookups.h"
  ImfAcesFile.cpp                 ImfAcesFile.h
  ImfArray.h
  ImfAttribute.cpp                ImfAttribute.h
  ImfAutoArray.h
  ImfB44Compressor.cpp            ImfB44Compressor.h
  ImfBoxAttribute.cpp             ImfBoxAttribute.h
  ImfCRgbaFile.cpp                ImfCRgbaFile.h
  ImfChannelList.cpp              ImfChannelList.h
  ImfChannelListAttribute.cpp     ImfChannelListAttribute.h
  ImfCheckedArithmetic.h
  ImfChromaticities.cpp           ImfChromaticities.h
  ImfChromaticitiesAttribute.cpp  ImfChromaticitiesAttribute.h
  ImfCompositeDeepScanLine.cpp    ImfCompositeDeepScanLine.h
  ImfCompression.h
  ImfCompressionAttribute.cpp     ImfCompressionAttribute.h
  ImfCompressor.cpp               ImfCompressor.h
  ImfConvert.cpp                  ImfConvert.h
  ImfDeepCompositing.cpp          ImfDeepCompositing.h
  ImfDeepFrameBuffer.cpp          ImfDeepFrameBuffer.h
  ImfDeepImageState.h
  ImfDeepImageStateAttribute.cpp  ImfDeepImageStateAttribute.h
  ImfDeepScanLineInputFile.cpp    ImfDeepScanLineInputFile.h
  ImfDeepScanLineInputPart.cpp    ImfDeepScanLineInputPart.h
  ImfDeepScanLineOutputFile.cpp   ImfDeepScanLineOutputFile.h
  ImfDeepScanLineOutputPart.cpp   ImfDeepScanLineOutputPart.h
  ImfDeepTiledInputFile.cpp       ImfDeepTiledInputFile.h
  ImfDeepTiledInputPart.cpp       ImfDeepTiledInputPart.h
  ImfDeepTiledOutputFile.cpp      ImfDeepTiledOutputFile.h
  ImfDeepTiledOutputPart.cpp      ImfDeepTiledOutputPart.h
  ImfDoubleAttribute.cpp          ImfDoubleAttribute.h
  ImfDwaCompressor.cpp            ImfDwaCompressor.h 
  ImfDwaCompressorSimd.h
  ImfEnvmap.cpp                   ImfEnvmap.h
  ImfEnvmapAttribute.cpp          ImfEnvmapAttribute.h
  ImfExport.h
  ImfFastHuf.cpp                  ImfFastHuf.h
  ImfFloatAttribute.cpp           ImfFloatAttribute.h
  ImfFloatVectorAttribute.cpp     ImfFloatVectorAttribute.h
  ImfForward.h
  ImfFrameBuffer.cpp              ImfFrameBuffer.h
  ImfFramesPerSecond.cpp          ImfFramesPerSecond.h
  ImfGenericInputFile.cpp         ImfGenericInputFile.h
  ImfGenericOutputFile.cpp        ImfGenericOutputFile.h
  ImfHeader.cpp                   ImfHeader.h
  ImfHuf.cpp                      ImfHuf.h
  ImfIO.cpp                       ImfIO.h
  ImfInputFile.cpp                ImfInputFile.h
  ImfInputPart.cpp                ImfInputPart.h
  ImfInputPartData.cpp            ImfInputPartData.h
  ImfInputStreamMutex.h
  ImfInt64.h
  ImfIntAttribute.cpp             ImfIntAttribute.h
  ImfKeyCode.cpp                  ImfKeyCode.h
  ImfKeyCodeAttribute.cpp         ImfKeyCodeAttribute.h
  ImfLineOrder.h
  ImfLineOrderAttribute.cpp       ImfLineOrderAttribute.h
  ImfLut.cpp                      ImfLut.h
  ImfMatrixAttribute.cpp          ImfMatrixAttribute.h
  ImfMisc.cpp                     ImfMisc.h
  ImfMultiPartInputFile.cpp       ImfMultiPartInputFile.h
  ImfMultiPartOutputFile.cpp      ImfMultiPartOutputFile.h
  ImfMultiView.cpp                ImfMultiView.h
  ImfName.h
  ImfNamespace.h
  ImfOpaqueAttribute.cpp          ImfOpaqueAttribute.h
  ImfOptimizedPixelReading.h
  ImfOutputFile.cpp               ImfOutputFile.h
  ImfOutputPart.cpp               ImfOutputPart.h
  ImfOutputPartData.cpp           ImfOutputPartData.h
  ImfOutputStreamMutex.h
  ImfPartHelper.h
  ImfPartType.cpp                 ImfPartType.h
  ImfPixelType.h
  ImfPizCompressor.cpp            ImfPizCompressor.h
  ImfPreviewImage.cpp             ImfPreviewImage.h
  ImfPreviewImageAttribute.cpp    ImfPreviewImageAttribute.h
  ImfPxr24Compressor.cpp          ImfPxr24Compressor.h
  ImfRational.cpp                 ImfRational.h
  ImfRationalAttribute.cpp        ImfRationalAttribute.h
  ImfRgba.h
  ImfRgbaFile.cpp                 ImfRgbaFile.h
  ImfRgbaYca.cpp                  ImfRgbaYca.h
  ImfRle.cpp                      ImfRle.h
  ImfRleCompressor.cpp            ImfRleCompressor.h
  ImfSimd.h
  ImfScanLineInputFile.cpp        ImfScanLineInputFile.h
  ImfStandardAttributes.cpp       ImfStandardAttributes.h
  ImfStdIO.cpp                    ImfStdIO.h
  ImfStringAttribute.cpp          ImfStringAttribute.h
  ImfStringVectorAttribute.cpp    ImfStringVectorAttribute.h
  ImfSystemSpecific.cpp           ImfSystemSpecific.h
  ImfTestFile.cpp                 ImfTestFile.h
  ImfThreading.cpp                ImfThreading.h
  ImfTileDescription.h
  ImfTileDescriptionAttribute.cpp ImfTileDescriptionAttribute.h
  ImfTileOffsets.cpp              ImfTileOffsets.h
  ImfTiledInputFile.cpp           ImfTiledInputFile.h
  ImfTiledInputPart.cpp           ImfTiledInputPart.h
  ImfTiledMisc.cpp                ImfTiledMisc.h
  ImfTiledOutputFile.cpp          ImfTiledOutputFile.h
  ImfTiledOutputPart.cpp          ImfTiledOutputPart.h
  ImfTiledRgbaFile.cpp            ImfTiledRgbaFile.h
  ImfTimeCode.cpp                 ImfTimeCode.h
  ImfTimeCodeAttribute.cpp        ImfTimeCodeAttribute.h
  ImfVecAttribute.cpp             ImfVecAttribute.h
  ImfVersion.cpp                  ImfVersion.h
  ImfWav.cpp                      ImfWav.h
  ImfXdr.h
  ImfZip.cpp                      ImfZip.h
  ImfZipCompressor.cpp            ImfZipCompressor.h
)


# Creates the special target 
ADD_OPENEXR_LIBRARY(IlmImf ILMIMF_HEADERS ILMIMF_SOURCES
  "IlmImf is a library that reads and writes OpenEXR images."
  ${OPENEXR_VERSION} ${OPENEXR_SOVERSION} "2002")

# Link against zlib as it was found in the top level CMakeLists.txt
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(IlmImf Half Iex Imath IlmThread ${ZLIB_LIBRARIES})

# Disable recurrent warnings in MSVC
MSVC_COMPILE_FLAGS(IlmImf "/wd4244 /wd4018 /wd4996 /wd4800 /wd4267")
