# IexMath library

set(IEXMATH_HEADERS
  IexMathExport.h
  IexMathFloatExc.h
  IexMathFpu.h
  IexMathIeeeExc.h
)

set(IEXMATH_SOURCES
  IexMathFloatExc.cpp
  IexMathFpu.cpp
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")  

# Creates the special target  
ADD_OPENEXR_LIBRARY(IexMath IEXMATH_HEADERS IEXMATH_SOURCES
  "IexMath is a floating point math exception-handling library."
  ${ILMBASE_VERSION} ${ILMBASE_SOVERSION} "2002")

target_link_libraries(IexMath Iex)
