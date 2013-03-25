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

# CMake file for running the ImageIO tests. It uses the GTest library.

# Because of the C++ single definition rule, and since this is the only
# component using gtest, we just add all its sources from the 3rdparty folder
set(GTEST_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/3rdparty/gtest/include")
set(GTEST_SRCS
  "${PROJECT_SOURCE_DIR}/3rdparty/gtest/fused-src/gtest/gtest-all.cc"
  )
if(MSVC AND MSVC_VERSION GREATER 1500)
  # As of Visual C++ 2012, variadic templates are not truly supported.
  # The default VARIADIC_MAX of 5 is not enough for gtest
  add_definitions(-D_VARIADIC_MAX=9)
endif()

include_directories(${GTEST_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/ImageIO")

# Hard-coded Mersenne Twister definitions
add_definitions (-DDSFMT_DO_NOT_USE_OLD_NAMES -DDHAVE_SSE2=1 -DDSFMT_MEXP=19937)

set(SRCS
  main.cpp
  Rgba32F_test.cpp
  rgbe_test.cpp
  ImageComparator_test.cpp
  ImageSoA_test.cpp
  ToneMapper_test.cpp
  ToneMapperSoA_test.cpp
  Reinhard02Params_test.cpp
  Amaths_test.cpp
  
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

# We link against ImageIO, so we need to remove that definition
remove_definitions(-DIMAGEIO_EXPORTS)

add_executable(ImageIO_Test ${SRCS} ${GTEST_SRCS})
target_link_libraries(ImageIO_Test ImageIO)

if(NOT WIN32)
  find_package(Threads)
  if(CMAKE_USE_PTHREADS_INIT)
    target_link_libraries(ImageIO_Test ${CMAKE_THREAD_LIBS_INIT})
  endif()
endif()


# Helper function to add tests. When using the Visual Studio generators with
# CMake 2.8 it will wrap the test in a batch file so that tests may be launched
# from within the IDE
function(add_ImageIO_test testname Exename)  
  if (NOT MSVC_IDE OR ${CMAKE_VERSION} VERSION_LESS "2.8")
    add_test("${testname}" "${Exename}" ${ARGN})
    return()
  endif()
  
  # More involved test specification using generator expressions
  set(extra "")
  foreach (target ImageIO)
    if(TARGET ${target})
      list(APPEND extra "$<TARGET_FILE_DIR:${target}>")
    endif()
  endforeach()
  if(NOT extra)
    set(extra "NONE")
  endif()
  
  if(TARGET "${Exename}")
    set(exe "$<TARGET_FILE:${Exename}>")
  else()
    set(exe "${Exename}")
  endif()
  
  # Protect the semicolons in the path list using batch delayed expansion
  string(REPLACE ";" "!CMAKE_PATH_SEPARATOR!" extra "${extra}")
  add_test(NAME "${testname}"
    COMMAND "${PROJECT_SOURCE_DIR}\\win32\\test_wrapper.cmd"
    "${extra}" "${exe}" ${ARGN}
  )
endfunction()

# Run the executable with CTest
add_ImageIO_test(ImageIOUnitTests ImageIO_Test)
