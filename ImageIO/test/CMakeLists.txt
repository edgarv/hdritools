# Cmake file for running the ImageIO tests. It uses the GTest library.

find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})

set(SRCS
  main.cpp
  Rgba32F_test.cpp
  rgbe_test.cpp
  ImageComparator_test.cpp
  ToneMapper_test.cpp
  Reinhard02Params_test.cpp
  
  tableau_f32.h tableau_f32.cpp
  Timer.h Timer.cpp
  
  # Helper MT components
  dSFMT/RandomMT.h dSFMT/RandomMT.cpp
  dSFMT/dSFMT.c dSFMT/dSFMT.h dSFMT/dSFMT-params.h dSFMT/dSFMT-params19937.h
  )
  
# Group the MT stuff
source_group(dSFMT REGULAR_EXPRESSION "dSFMT.+")

# Easily find the ImageIO headers at the parent directory
include_directories(..)

# We link againt ImageIO, so we need to remove that definition
remove_definitions(-DIMAGEIO_EXPORTS)

add_executable(ImageIO_Test ${SRCS})
target_link_libraries(ImageIO_Test ImageIO ${GTEST_LIBRARIES})

# Enable explicitly SSE2 in linux
if(NOT MSVC)
set_target_properties(ImageIO_Test PROPERTIES 
  COMPILE_FLAGS "-msse -msse2 -ftree-vectorizer-verbose=5"
  )
endif(NOT MSVC)


# Run the executable with CTest
enable_testing()
add_test(ImageIOUnitTests ImageIO_Test)
