# Description for the Half Library

# Programs which build the LUTs
add_executable(eLUT eLut.cpp)
add_executable(toFloat toFloat.cpp)

# Directory with the LUTs
set(LUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${LUT_DIRECTORY} )
  
# All the actual sources
set(HALF_HEADERS
  half.h halfFunction.h halfLimits.h
  )
set(HALF_SOURCES
  ${LUT_DIRECTORY}/eLUT.h
  ${LUT_DIRECTORY}/toFloat.h
  half.cpp
  )
  
add_custom_command(OUTPUT ${LUT_DIRECTORY}/eLUT.h
  COMMAND eLUT > ${LUT_DIRECTORY}/eLUT.h
  COMMENT "Generating float-to-half LUT..."
  )

add_custom_command(OUTPUT ${LUT_DIRECTORY}/toFloat.h
  COMMAND toFloat > ${LUT_DIRECTORY}/toFloat.h
  COMMENT "Generating half-to-float LUT..."
  )

# Creates the special target  
ADD_OPENEXR_LIBRARY(Half HALF_HEADERS HALF_SOURCES
    "Half is a class that encapsulates the ilm 16-bit floating-point format."
	${ILMBASE_VERSION} "2002")
