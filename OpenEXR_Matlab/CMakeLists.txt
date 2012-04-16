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

# Test for checking for openexrdll
# Under windows, if OpenEXR was compiled as a DLL,  this test will fail because of linking errors.
#include(CheckCXXSourceCompiles)
#set(CMAKE_REQUIRED_INCLUDES "${OpenEXR_INCLUDE_DIR}/OpenEXR")
#set(CMAKE_REQUIRED_LIBRARIES ${OpenEXR_LIBRARIES})
#CHECK_CXX_SOURCE_COMPILES("#include <half.h>\nint main(){ half h1=1.0f;float f1=h1-1.0f;return 0;}\n" OpenEXR_WIN_STATIC)

# When not using the built-in OpenEXR, check if it supports the features
# introduced in OpenEXR 1.7
if(USE_SYSTEM_OPENEXR)
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES  ${OpenEXR_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${OpenEXR_LIBRARIES})
  CHECK_CXX_SOURCE_COMPILES("
#include <ImathVec.h>
#include <ImathMatrix.h>
#include <ImfVecAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfStringVectorAttribute.h>

Imath::V2d v2;
Imath::V3d v3;
Imath::M33d m33;
Imath::M44d m44;

Imf::StringVector svec;
Imf::V2dAttribute v2attr;
Imf::V3dAttribute v3attr;
Imf::M33dAttribute m33attr;
Imf::M44dAttribute m44attr;
Imf::StringVectorAttribute svecattr;

int main(int argc, char** argv) { return 0; }
" HAVE_OPENEXR_1_7)

  if(NOT HAVE_OPENEXR_1_7)
    add_definitions(-DHAVE_OPENEXR_1_7=0)
  endif()
endif()


find_package(Matlab REQUIRED)

# Set the appropriate suffixes according to the platform
if(MSVC)
  # Linker settings: it just avoids dll_export stuff
  set(MEX_LDFLAGS /export:mexFunction)
elseif(UNIX)

  # Whole set of linker flags
  if(NOT APPLE)
    set(MEX_LDFLAGS "-pthread -Wl,--version-script,${MATLAB_MEX_MAPFILE} -Wl,--no-undefined")
  else()
   set(MEX_LDFLAGS "-Wl,-twolevel_namespace -undefined error")
   set(MEX_LDFLAGS "${MEX_LDFLAGS} -Wl,-exported_symbols_list,${MATLAB_MEX_MAPFILE}")
   if (MATLAB_UNIVERSAL)
     set(MEX_LDFLAGS_EXTRA "-Wl,-twolevel_namespace -undefined error")
     set(MEX_LDFLAGS_EXTRA "${MEX_LDFLAGS_EXTRA} -Wl,-exported_symbols_list")
     set(MEX_LDFLAGS_EXTRA "${MEX_LDFLAGS_EXTRA},${MATLAB_MEX_MAPFILE_EXTRA}")
   endif()
  endif()

endif()

# The include directory directive applies to all modules
include_directories(${MATLAB_INCLUDE_DIRS} ${OpenEXR_INCLUDE_DIR})

# Type of library target
set(MEX_TARGET_TYPE "MODULE")


# Super function to create mex projects. All the arguments to the macro are the source files.
# Note that rpath is always enabled for mex files (this is what Matlab does)
function(add_mex name)
  set(sources ${ARGN})
  add_library(${name} ${MEX_TARGET_TYPE} ${sources})
  HDRITOOLS_LTCG(${name})
  set_target_properties(${name} PROPERTIES
    SUFFIX ".${MATLAB_MEX_EXT}"
    PREFIX ""
    COMPILE_DEFINITIONS "MATLAB_MEX_FILE"
    LINK_FLAGS "${MEX_LDFLAGS}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
    OSX_ARCHITECTURES "${MATLAB_OSX_ARCH}"
    FOLDER "Matlab-MEX"
  )
  target_link_libraries(${name} ${MATLAB_LIBRARIES})
  
  # Same set of things for the extra architexture
  if (MATLAB_UNIVERSAL)
    add_library(${name}_extra ${MEX_TARGET_TYPE} ${sources})
    HDRITOOLS_LTCG(${name}_extra)
    set_target_properties(${name}_extra PROPERTIES
      OUTPUT_NAME "${name}"
      SUFFIX ".${MATLAB_MEX_EXT_EXTRA}"
      PREFIX ""
      COMPILE_DEFINITIONS "MATLAB_MEX_FILE"
      LINK_FLAGS "${MEX_LDFLAGS_EXTRA}"
      INSTALL_RPATH_USE_LINK_PATH TRUE
      OSX_ARCHITECTURES "${MATLAB_OSX_ARCH_EXTRA}"
      FOLDER "Matlab-MEX"
    )
    target_link_libraries(${name}_extra ${MATLAB_LIBRARIES_EXTRA})
  endif()
endfunction()

# For automatic multithreading support, find out a way to get the number of CPUs
# For non-msvc platforms try with the POSIX sysconf
if(NOT MSVC)
  include(CheckCSourceRuns)
  CHECK_C_SOURCE_RUNS(
    "#include <unistd.h>\n int main() { int n = sysconf(_SC_NPROCESSORS_ONLN); return 0;}\n" 
    SYSCONF_WORKS
  )
  if(SYSCONF_WORKS)
    add_definitions(-DUSE_SYSCONF=1)
  else()
    add_definitions(-DUSE_SYSCONF=0)
  endif()
else()
  add_definitions(-DUSE_SYSCONF=0)
endif()

# Avoid secure warnings with exr headers
add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1
                -D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT=1)

# Creates the projects.
set(COMMON_SRC util.h util.cpp)
if(UNIX AND MATLAB_MEX_VERSION_SRC)
  list(APPEND COMMON_SRC ${MATLAB_MEX_VERSION_SRC})
endif()

# OpenEXR definitions (only used in windows dll anyway)
if(OpenEXR_DEFINITIONS)
  add_definitions(${OpenEXR_DEFINITIONS})
endif()


# Create the MEX modules
add_mex(exrread  exrread.cpp  ${COMMON_SRC})
add_mex(exrwrite exrwrite.cpp ${COMMON_SRC})
add_mex(exrinfo  exrinfo.cpp  ImfToMatlab.h ImfToMatlab.cpp ${COMMON_SRC})
add_mex(exrreadchannels exrreadchannels.cpp ${COMMON_SRC})
add_mex(exrwritechannels exrwritechannels.cpp 
  MatlabToImf.h MatlabToImf.cpp ImfToMatlab.h ${COMMON_SRC})


# Links them against OpenEXR
set(MATLAB_MEX_TARGETS exrread exrwrite exrinfo
  exrreadchannels exrwritechannels)
if (MATLAB_UNIVERSAL)
  set(mex_targets_extra "")
  foreach(mextarget ${MATLAB_MEX_TARGETS})
    list(APPEND mex_targets_extra "${mextarget}_extra")
  endforeach()
  list(APPEND MATLAB_MEX_TARGETS ${mex_targets_extra})
endif()
foreach(mextarget ${MATLAB_MEX_TARGETS})
  target_link_libraries(${mextarget} ${OpenEXR_LIBRARIES})
endforeach()


# And installs the mex binary and the .m file
install(TARGETS ${MATLAB_MEX_TARGETS}
  RUNTIME DESTINATION "${HDRITOOLS_SHARE_DIR}/matlab" COMPONENT "openexrmex"
  LIBRARY DESTINATION "${HDRITOOLS_SHARE_DIR}/matlab" COMPONENT "openexrmex"
  )
install(FILES exrread.m exrwrite.m exrinfo.m exrreadchannels.m exrwritechannels.m
  DESTINATION "${HDRITOOLS_SHARE_DIR}/matlab" COMPONENT "openexrmex"
  )
