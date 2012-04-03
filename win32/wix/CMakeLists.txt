# Build script for the Wix - based MSI package
if (NOT WIN32)
  message(FATAL_ERROR "The WiX package requires Windows")
endif()

find_package(WiX 3 REQUIRED)


# Helper function to try to identify the includes in a wsx file.
# The include must be of the form <?include FILENAME ?>
# limited to one per line. It ignore <?include ?> blocks within
# comment blocks.
function(wxs_implicit_depends wxs out_wxi_var)
  file(STRINGS "${wxs}" wxs_strings
    REGEX ".*<\\?[ ]*include.+")
  if (NOT wxs_strings)
    set(${out_wxi_var} "" PARENT_SCOPE)
    return()
  endif()
  
  get_filename_component(base_dir "${wxs}" PATH)
  set(lst "")
  foreach(inc ${wxs_strings})
    if (inc MATCHES ".*<\\?[ ]*include[ ]+([^ ]+)[ ]+\\?>")
      list(APPEND lst "${base_dir}/${CMAKE_MATCH_1}")
    endif()
  endforeach()
  set(${out_wxi_var} ${lst} PARENT_SCOPE)
endfunction()


# Function to compile .wxs into .wixobj. For now this uses hard-coded parameters
# FIXME Do not rely on the global variable WiX_DEFINES
function(compile_wxs wxs_file out_winobj_var)
  get_filename_component(wxs_absolute "${wxs_file}" ABSOLUTE)
  wxs_implicit_depends("${wxs_absolute}" implicit_depends)
  if (implicit_depends)
    set(wxs_depends DEPENDS ${implicit_depends})
  else()
    set(wxs_depends "")
  endif()
  string(REGEX REPLACE "\\.[wW][xX][sS]$" ".wixobj" winobj "${wxs_file}")
  add_custom_command(OUTPUT "${winobj}"
    COMMAND "${WiX_CANDLE_EXECUTABLE}" -nologo ${WiX_DEFINES} -fips
            "${wxs_absolute}"
    MAIN_DEPENDENCY "${wxs_file}" ${wxs_depends}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Compiling ${wxs_file} -> ${winobj}"
  )
  set(${out_winobj_var} "${winobj}" PARENT_SCOPE)
endfunction()

  
# Function to generate the .MSI from the list of .wxs sources.
# This function will write the final MSI to the root of the build directory
# to mimic the behavior of CPack.
# FIXME Do not rely on the global variable WiX_DEFINES
# FIMXE Assumes the install has been staged in CMAKE_INSTALL_PREFIX
function(add_wix msi_name)
  set(depends "")
  foreach(wxs ${ARGN})
    compile_wxs(${wxs} wixobj)
    list(APPEND depends "${wixobj}")
  endforeach()
  
  # Actually create the MSI
  #set(msi "${PROJECT_BINARY_DIR}/${msi_name}")
  set(msi "${msi_name}")
  add_custom_command(OUTPUT "${msi}"
    COMMAND "${WiX_LIGHT_EXECUTABLE}" -nologo -b "${CMAKE_INSTALL_PREFIX}"
            -ext WixUIExtension ${WiX_DEFINES} -spdb
            -o "${msi}" ${depends}
    DEPENDS ${depends}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Linking ${msi_name}"
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

set(SRCS
  HDRITools.wxs
  runtime.wxs
  pcgimageio.wxs
  qt4image.wxs
  batchtonemapper.wxs
  openexrjni.wxs
  openexrmex.wxs
)

set(HDRITOOLS_MSI HDRITools-${HDRITOOLS_VERSION}-${HDRITOOLS_PLATFORM}.msi)
add_wix(${HDRITOOLS_MSI} ${SRCS})
