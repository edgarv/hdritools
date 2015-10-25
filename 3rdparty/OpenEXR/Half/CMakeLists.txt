# Description for the Half Library

# Programs which build the LUTs
add_executable(eLut eLut.cpp)
add_executable(toFloat toFloat.cpp)

# Directory with the LUTs
set(LUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lut")
file(MAKE_DIRECTORY "${LUT_DIRECTORY}")
include_directories("${LUT_DIRECTORY}")
  
# All the actual sources
set(HALF_HEADERS
  half.h
  halfFunction.h
  halfLimits.h
  halfExport.h
)
set(HALF_SOURCES
  "${LUT_DIRECTORY}/eLut.h"
  "${LUT_DIRECTORY}/toFloat.h"
  half.cpp
)
  
add_custom_command(OUTPUT "${LUT_DIRECTORY}/eLut.h"
  COMMAND eLut > "${LUT_DIRECTORY}/eLut.h"
  COMMENT "Generating float-to-half LUT..."
  VERBATIM
)

add_custom_command(OUTPUT "${LUT_DIRECTORY}/toFloat.h"
  COMMAND toFloat > "${LUT_DIRECTORY}/toFloat.h"
  COMMENT "Generating half-to-float LUT..."
  VERBATIM
)

# Creates the special target  
ADD_OPENEXR_LIBRARY(Half HALF_HEADERS HALF_SOURCES
    "Half is a class that encapsulates the IEEE 754-2008 half precision (16-bit floating-point) format."
    ${ILMBASE_VERSION} ${ILMBASE_SOVERSION} "2002")
