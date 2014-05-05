# ============================================================================
#   HDRITools - High Dynamic Range Image Tools
#   Copyright 2008-2012 Program of Computer Graphics, Cornell University
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

# Setup some basics. Requires CMake 2.8 and Matlab
cmake_policy(VERSION 2.8)


# Try to download the test multichannel OpenEXR file
set(x35_URL "http://www.fnordware.com/OpenEXR/x35_3a.exr")
set(x35_MD5 "901a11ad9d9113b1f28265abcd51bd64")
set(x35_exr "${CMAKE_CURRENT_BINARY_DIR}/x35_3a.exr")

macro(CHECK_DOWNLOAD ${status} ${msg})
  if(NOT ${status} EQUAL 0)
    message(WARNING ${msg})
  endif()
endmacro()


# Dowload the file if it does not exist or matches the hash (if supported)
if(NOT EXISTS ${x35_exr})
  set(NEEDS_X35 TRUE)
else()
  set(NEEDS_X35 FALSE)
  if(NOT CMAKE_VERSION VERSION_LESS 2.8.7)
    file(MD5 ${x35_exr} CURRENT_MD5)
    if(NOT CURRENT_MD5 STREQUAL "${x35_MD5}")
      set(NEEDS_X35 TRUE)
    endif()
  endif()
endif()

if(NEEDS_X35)
  file(DOWNLOAD ${x35_URL} ${x35_exr}
    INACTIVITY_TIMEOUT 15 STATUS x35_status_pair SHOW_PROGRESS 
    EXPECTED_MD5 ${x35_MD5})
  CHECK_DOWNLOAD(${x35_status_pair})
endif()

set(test_dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(test_script "x35_3a_test")
configure_file("launch_test.m.cmake" "launch_test.m" ESCAPE_QUOTES @ONLY)

add_test(NAME openexrmex_x35
  COMMAND matlab -nosplash -nodesktop -r "launch_test('$<TARGET_FILE_DIR:exrinfo>')")
