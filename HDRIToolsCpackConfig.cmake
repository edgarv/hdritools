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

# Commands used to set up CPack. This file is to be included by the top level
# CMakeLists.txt only!

if (NOT DEFINED HDRITOOLS_VERSION OR NOT DEFINED HDRITOOLS_VERSION_MAJOR OR
    NOT DEFINED HDRITOOLS_VERSION_MINOR OR NOT DEFINED HDRITOOLS_VERSION_PATCH
	OR NOT DEFINED HDRITOOLS_VERSION_BUILD)
  message(FATAL_ERROR "The required variables are not set!")
endif()

set(HDRITOOLS_URL "https://bitbucket.org/edgarv/hdritools/")

# The mantainer field must have a valid email address, let's obfuscate it!
string(REGEX REPLACE "1i34(.+)3984(.+)k34kf(.+)38mwe(.+)34k3.*23987(.+)3k32j"
  "\\5 <\\3@\\1.\\4.\\2>" CPACK_PACKAGE_CONTACT
  "1i34cs3984eduk34kfeva538mwecornell34k3
  23987Edgar Velazquez-Armendariz3k32j")

set(CPACK_PACKAGE_VERSION       ${HDRITOOLS_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${HDRITOOLS_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HDRITOOLS_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HDRITOOLS_VERSION_PATCH})

set(CPACK_PACKAGE_NAME   "HDRITools")
set(CPACK_PACKAGE_VENDOR "Cornell PCG")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "Simple tools for HDR images used at the Cornell PCG")
set(CPACK_PACKAGE_DESCRIPTION
"HDRITools is a set of simple tools to manipulate HDR images. They include a
viewer, a batch tonemapper, basic JNI bindings for OpenEXR and Matlab
extensions to read and save OpenEXR files.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.text")

# Use a version of the license which doesn't have explicit line breaks
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" LICENSE_TXT)
string(REGEX REPLACE "([^ \t\n\r])\r?\n([^\n\r])" "\\1 \\2" LICENSE_TXT "${LICENSE_TXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" "${LICENSE_TXT}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")


if (CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 8 OR CMAKE_CL_64)
  set(HDRITOOLS_64 1)
endif()

# Platform specific names
if (WIN32)
  if (HDRITOOLS_64)
    set(CPACK_SYSTEM_NAME "win64-amd64")
  else()
    set(CPACK_SYSTEM_NAME "win32-x86")
  endif()
elseif (APPLE)
  if (CMAKE_OSX_ARCHITECTURES)
    foreach(osx_arch ${CMAKE_OSX_ARCHITECTURES})
      if (osx_arch STREQUAL "i386")
        set(HDRITOOLS_MACI32 ON)
      elseif (osx_arch STREQUAL "x86_64")
        set(HDRITOOLS_MACI64 ON)
      endif()
    endforeach()
	if (HDRITOOLS_MACI32 AND HDRITOOLS_MACI64)
	  set(CPACK_SYSTEM_NAME "Darwin-Universal")
	elseif(HDRITOOLS_MACI64)
	  set(CPACK_SYSTEM_NAME "Darwin-x86_64")
	elseif(HDRITOOLS_MACI32)
	  set(CPACK_SYSTEM_NAME "Darwin-i386")
	else()
	  message(AUTHOR_WARNING "Not buiding for neither i386 nor amd64!")
	  set(CPACK_SYSTEM_NAME "Darwin")
	endif()
  else()
    if (HDRITOOLS_64)
	  set(CPACK_SYSTEM_NAME "Darwin-x86_64")
	else()
	  set(CPACK_SYSTEM_NAME "Darwin-i386")
	endif()
  endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if (HDRITOOLS_64)
    set(CPACK_SYSTEM_NAME "Linux-x86_64")
  else()
    set(CPACK_SYSTEM_NAME "Linux-i686")
  endif()
endif()

# Default system name if it hasn't been set yet
if (NOT CPACK_SYSTEM_NAME)
  if (NOT HDRITOOLS_64)
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
  else()
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}64")
  endif()
endif()

set (CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${HDRITOOLS_VERSION_BUILD}-${CPACK_SYSTEM_NAME})
if (MSVC)
  math(EXPR MSVC_SIMPLE_VER "(${MSVC_VERSION} - 600) / 100")
  set (CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME}-msvc${MSVC_SIMPLE_VER})
endif()


# Strip files by default, since we only intend to distribute release builds
if (NOT DEFINED CPACK_STRIP_FILES)
  set(CPACK_STRIP_FILES ON)
endif()


# Extra stuff for NSIS
set(CPACK_NSIS_URL_INFO_ABOUT ${HDRITOOLS_URL})
if (HDRITOOLS_64)
  set(CPACK_NSIS_INSTALL_ROOT "\$PROGRAMFILES64")
  set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (Win64)")
  set(CPACK_CUSTOM_INITIAL_DEFINITIONS "${CPACK_CUSTOM_INITIAL_DEFINITIONS}\n"
    " Require a 64-bit system\n!define CPACK_REQUIRIRE_64BIT\n")
else()
  set(CPACK_NSIS_INSTALL_ROOT "\$PROGRAMFILES")
  set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
endif()

# Add the extra configuration stuff from Qt4Image
if (QT4IMAGE_CPACK_CONFIG_FILE)
  include("${QT4IMAGE_CPACK_CONFIG_FILE}")
endif()


# Drag'N Drop installer for Mac (PackageMaker is overkill)
set(CPACK_DMG_VOLUME_NAME ${CPACK_PACKAGE_NAME})
set(CPACK_DMG_FORMAT "UDBZ") # UDIF bzip2-compressed image (OS X 10.4+ only)


# Additional variables for Debian packages
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${HDRITOOLS_URL})


# RPM has strict requisites about version strings. Also each different release
# should increment the counter. Note that "0" means a pre-release version.
set(RPM_RELEASE_NUM 0)
set(RPM_PRERELEASE_INCREMENT 1)
set(CPACK_RPM_PACKAGE_VERSION ${HDRITOOLS_VERSION})
if (HDRITOOLS_HAS_VALID_REV)
  set(CPACK_RPM_PACKAGE_RELEASE
    ${RPM_RELEASE_NUM}.${RPM_PRERELEASE_INCREMENT}.hg${HDRITOOLS_REV_ID})
else()
  set(CPACK_RPM_PACKAGE_RELEASE
    ${RPM_RELEASE_NUM}.${RPM_PRERELEASE_INCREMENT}.${HDRITOOLS_VERSION_BUILD})
endif()
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_URL ${HDRITOOLS_URL})
if (CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 8)
  set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
else()
  # Note that Fedora is using i686 instead for the 32-bit builds
  set(CPACK_RPM_PACKAGE_ARCHITECTURE "i386")
endif()


# Newer distributions use OpenEXR 2.2, try to detect it via the DWA compression enum
if (USE_SYSTEM_OPENEXR AND NOT OpenEXR_VERSION)
  set(OpenEXR_VERSION "1.6")
  set(_EXR_IMFCOMPRESSION_H "${OpenEXR_INCLUDE_DIR}/ImfCompression.h")
  if (EXISTS "${_EXR_IMFCOMPRESSION_H}")
    file(STRINGS "${_EXR_IMFCOMPRESSION_H}" _EXR_COMPRESSION_DWA REGEX "DWA[AB]")
    if (_EXR_COMPRESSION_DWA)
      set(OpenEXR_VERSION "2.2")
    endif()
  endif()
  unset(_EXR_IMFCOMPRESSION_H)
  unset(_EXR_COMPRESSION_DWA)
endif()


# Linux Package Dependencies (for Debian and RPM)
# TODO The package names here are only based on Fedora and Ubuntu
set(PACKAGE_RPM_REQUIRES_LIST "glibc >= 2" "libstdc++ >= 4" "tbb >= 2.2")
set(PACKAGE_DEB_REQUIRES_LIST "libc6" "libstdc++6" "libtbb2")
if (USE_SYSTEM_OPENEXR)
  if (NOT OpenEXR_VERSION VERSION_LESS 2.2)
    list(APPEND PACKAGE_RPM_REQUIRES_LIST "OpenEXR-libs >= 2.2" "ilmbase >= 1.2")
    list(APPEND PACKAGE_DEB_REQUIRES_LIST "libopenexr22 (>= 2.2)" "libilmbase12 (>= 2.2)")
  else()
    list(APPEND PACKAGE_RPM_REQUIRES_LIST "OpenEXR-libs >= 1.6" "ilmbase >= 1.0")
    list(APPEND PACKAGE_DEB_REQUIRES_LIST "libopenexr6" "libilmbase6")
  endif()
endif()
if (USE_SYSTEM_PNG)
  list(APPEND PACKAGE_RPM_REQUIRES_LIST "libpng >= 1.2")
  list(APPEND PACKAGE_DEB_REQUIRES_LIST "libpng12-0")
endif()
if (USE_SYSTEM_ZLIB)
  list(APPEND PACKAGE_RPM_REQUIRES_LIST "zlib >= 1.2")
  list(APPEND PACKAGE_DEB_REQUIRES_LIST "zlib1g (>= 1:1.2)")
endif()
if (BUILD_BATCH_TONEMAPPER OR BUILD_QT4IMAGE)
  # Assume that we are still under the scope of find_package(Qt5)
  if (TARGET Qt5::Core)
    # New version, assumes qt >= 5.2 on Ubuntu. Based on the Ubuntu cmake-qt-gui package.
    # We have conditional HiDPI code for Qt 5.6
    if (Qt5_VERSION VERSION_LESS 5.6)
      set(PACKAGE_MIN_QT_VERSION "5.2")
    else()
      set(PACKAGE_MIN_QT_VERSION "5.6")
    endif()
    list(APPEND PACKAGE_RPM_REQUIRES_LIST "qt >= ${PACKAGE_MIN_QT_VERSION}")
    list(APPEND PACKAGE_DEB_REQUIRES_LIST
      "libqt5core5a (>= ${PACKAGE_MIN_QT_VERSION})"
      "libqt5gui5 (>= ${PACKAGE_MIN_QT_VERSION})"
      "libqt5widgets5 (>= ${PACKAGE_MIN_QT_VERSION})")
  else()
    # Legacy, requiring qt 4.5
    list(APPEND PACKAGE_RPM_REQUIRES_LIST "qt >= 4.5")
    list(APPEND PACKAGE_DEB_REQUIRES_LIST "libqtcore4 (>= 4:4.5)" "libqtgui4 (>= 4:4.5)")
  endif()
endif()
string(REGEX REPLACE ";" ", " CPACK_RPM_PACKAGE_REQUIRES
  "${PACKAGE_RPM_REQUIRES_LIST}")
string(REGEX REPLACE ";" ", " CPACK_DEBIAN_PACKAGE_DEPENDS
  "${PACKAGE_DEB_REQUIRES_LIST}")
