# - Sets up the version info variables
# This module provides a macro, tested to be included ONLY from the root dir:
#  HDRITOOLS_GET_VERSION_INFO()
# This macro will read the "VERSION.txt" file and execute "hg", setting:
#  HDRITOOLS_VERSION
#  HDRITOOLS_VERSION_MAJOR
#  HDRITOOLS_VERSION_MINOR
#  HDRITOOLS_VERSION_PATCH
#  HDRITOOLS_VERSION_BUILD
#  HDRITOOLS_HAS_VALID_REV
#  HDRITOOLS_REV_ID
#  HDRITOOLS_DATE

macro(HDRITOOLS_GET_VERSION_INFO)

  # Uses hg to get the version string and the date of such revision
  # Based on info from:
  #  http://mercurial.selenic.com/wiki/VersioningWithMake (January 2011)

  # Try to directly get the information assuming the source is within a repo
  find_program(HG_CMD hg DOC "Mercurial command line executable")
  mark_as_advanced(HG_CMD)
  if (HG_CMD)
    execute_process(
      COMMAND "${HG_CMD}" -R "${CMAKE_CURRENT_SOURCE_DIR}"
                          parents --template "{node|short},{date|shortdate}"
	  OUTPUT_VARIABLE HG_INFO
	  OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (HG_INFO)
      # Extract the revision ID and the date
      string(REGEX REPLACE "(.+),.+" "\\1" HDRITOOLS_REV_ID "${HG_INFO}")
      string(REGEX REPLACE ".+,(.+)-(.+)-(.+)" "\\1.\\2.\\3"
        HDRITOOLS_DATE "${HG_INFO}")
      unset(HG_INFO)
    endif()
  endif()
  
  # If that failed, try grabbing the id from .hg_archival.txt, in case a tarball
  # made by "hg archive" is being used
  if (NOT HDRITOOLS_REV_ID)
    set(HG_ARCHIVAL_FILENAME "${CMAKE_CURRENT_SOURCE_DIR}/.hg_archival.txt")
    # Try to read from the file generated by "hg archive"
    if (EXISTS "${HG_ARCHIVAL_FILENAME}")
      file(READ "${HG_ARCHIVAL_FILENAME}" HG_ARCHIVAL_TXT)
      # Extract just the first 12 characters of the node
      string(REGEX REPLACE ".*node:[ \\t]+(............).*" "\\1"
        HDRITOOLS_REV_ID "${HG_ARCHIVAL_TXT}")
      unset(HG_ARCHIVAL_TXT)
    endif()
  endif()

  if (NOT HDRITOOLS_DATE)
    # Windows' date command output depends on the regional settings
    if (WIN32)
      set(GETDATE_CMD "${CMAKE_CURRENT_SOURCE_DIR}/win32/getdate.exe")
    else()
      set(GETDATE_CMD "date")
      set(GETDATE_ARGS "+'%Y.%m.%d'")	
    endif()
    execute_process(COMMAND "${GETDATE_CMD}" ${GETDATE_ARGS}
  	  OUTPUT_VARIABLE HDRITOOLS_DATE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT HDRITOOLS_DATE)
      message(FATAL_ERROR "Unable to get a build date!")
    endif()
  endif()

  if (HDRITOOLS_REV_ID)
    set (HDRITOOLS_HAS_VALID_REV 1)
  else()
    message(WARNING "Unable to find the mercurial revision id.")
    set (HDRITOOLS_HAS_VALID_REV 0)
  endif()


  # Read the version info from the file
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" VERSION_TXT)
  string(REGEX REPLACE ".*hdritools - ([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1"
    HDRITOOLS_VERSION "${VERSION_TXT}")
  unset(VERSION_TXT)
  if (HDRITOOLS_VERSION MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(HDRITOOLS_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(HDRITOOLS_VERSION_MINOR ${CMAKE_MATCH_2})
    set(HDRITOOLS_VERSION_PATCH ${CMAKE_MATCH_3})
  else()
    message(FATAL_ERROR
      "HDRITools version has an unexpected format: ${HDRITOOLS_VERSION}")
  endif()

  # Make a super simple build number from the date
  if (HDRITOOLS_DATE MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(HDRITOOLS_VERSION_BUILD
      "${CMAKE_MATCH_1}${CMAKE_MATCH_2}${CMAKE_MATCH_3}")
  else()
    message(FATAL_ERROR
      "HDRITools date has an unexpected format: ${HDRITOOLS_DATE}")
  endif()

endmacro()
