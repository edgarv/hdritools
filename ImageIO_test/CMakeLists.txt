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

# Cmake file for running the ImageIO tests. It uses the GTest library.

find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})
include_directories(../ImageIO)

# Hard-coded Mersenne Twister definitions
add_definitions (-DDSFMT_DO_NOT_USE_OLD_NAMES -DDHAVE_SSE2=1 -DDSFMT_MEXP=19937)

set(SRCS
  main.cpp
  Rgba32F_test.cpp
  rgbe_test.cpp
  ImageComparator_test.cpp
  ImageSoA_test.cpp
  ToneMapper_test.cpp
  Reinhard02Params_test.cpp
  
  tableau_f32.h tableau_f32.cpp
  Timer.h Timer.cpp
  TestUtil.h
  
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

if(NOT WIN32)
  find_package(Threads)
  if(CMAKE_USE_PTHREADS_INIT)
    target_link_libraries(ImageIO_Test ${CMAKE_THREAD_LIBS_INIT})
  endif()
endif()


# Run the executable with CTest
add_test(ImageIOUnitTests ImageIO_Test)
