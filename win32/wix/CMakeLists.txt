# Build script for the Wix - based MSI package
if (NOT WIN32)
  message(FATAL_ERROR "The WiX package requires Windows")
endif()

find_package(WiX 3 REQUIRED)

# Function to compile .wxs into .wixobj. For now this uses hard-coded parameters
# FIXME Do not rely on the global variable WiX_DEFINES
function(compile_wxs wxs_file out_winobj_var)
  get_filename_component(wxs_absolute "${wxs_file}" ABSOLUTE)
  string(REGEX REPLACE "\\.[wW][xX][sS]$" ".winobj" winobj "${wxs_file}")
  set(winobj_out "${CMAKE_CURRENT_BINARY_DIR}/${winobj}")
  add_custom_command(OUTPUT "${winobj_out}"
    COMMAND "${WiX_CANDLE_EXECUTABLE}" -nologo ${WiX_DEFINES} -fips
            -o "${winobj_out}"
            "${wxs_absolute}"
    MAIN_DEPENDENCY "${wxs_absolute}"
  )
  set(${out_winobj_var} "${winobj_out}" PARENT_SCOPE)
endfunction()
  #add_custom_command(OUTPUT
  
# Function to generate the .MSI from the list of .wxs sources.
# This function will write the final MSI to the root of the build directory
# to mimic the behavior of CPack.
# FIXME Do not rely on the global variable WiX_DEFINES
# FIMXE Assumes the install has been staged in CMAKE_INSTALL_PREFIX
function(add_wix msi_name)
  set(depends "")
  foreach(wxs ${ARGN})
    compile_wxs(${wxs} winobj)
    list(APPEND depends "${winobj}")
  endforeach()
  
  # Actually create the MSI
  set(msi "${PROJECT_BINARY_DIR}/${msi_name}")
  add_custom_command(OUTPUT "${msi}"
    COMMAND "${WiX_LIGHT_EXECUTABLE}" -nologo -b "${CMAKE_INSTALL_PREFIX}"
            -ext WixUIExtension ${WiX_DEFINES} -spdb
            -o "${msi}" ${depends}
    DEPENDS ${depends}
  )
  add_custom_target(WIX_PACKAGE DEPENDS "${msi}")
endfunction()
  
if (CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 8 OR CMAKE_CL_64)
  set(HDRITOOLS_PLATFORM "x64")
else()
  set(HDRITOOLS_PLATFORM "x86")
endif()

# Set up the defines
set(WiX_DEFINES "")
list(APPEND WiX_DEFINES -dPlatform=${HDRITOOLS_PLATFORM})
list(APPEND WiX_DEFINES -dVersion=${HDRITOOLS_VERSION})

set(HDRITOOLS_MSI HDRITools-${HDRITOOLS_VERSION}-${HDRITOOLS_VERSION_BUILD}-${HDRITOOLS_PLATFORM}.msi)
add_wix(${HDRITOOLS_MSI} HDRITools.wxs)
