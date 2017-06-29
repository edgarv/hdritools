# Non-standalone Cmake driver for OpenEXR. Assumes that the IlmBase
# libraries have been previously defined as targets.

# Extract the version information from configure.ac
GET_VERSION (OpenEXR)

set(OPENEXR_IMF_HAVE_LINUX_PROCFS     ${HAVE_LINUX_PROCFS})
set(OPENEXR_IMF_HAVE_DARWIN           ${HAVE_DARWIN})
set(OPENEXR_IMF_HAVE_COMPLETE_IOMANIP ${HAVE_COMPLETE_IOMANIP})
set(OPENEXR_IMF_HAVE_LARGE_STACK      ${ENABLE_LARGE_STACK})
set(OPENEXR_IMF_USE_WINNT_VISTA_SYNC  ${USE_WINNT_VISTA_SYNC})

if (ENABLE_NAMESPACE_VERSIONING)
  set(OPENEXR_IMF_INTERNAL_NAMESPACE_CUSTOM 1)
  set(OPENEXR_IMF_INTERNAL_NAMESPACE Imf_${OPENEXR_VERSION_API})
else()
  set(OPENEXR_IMF_INTERNAL_NAMESPACE_CUSTOM 0)
  set(OPENEXR_IMF_INTERNAL_NAMESPACE Imf)
endif()

if (ENABLE_CUSTOM_USER_NAMESPACE)
  set(OPENEXR_IMF_NAMESPACE_CUSTOM 1)
  message(FATAL_ERROR "NOT IMPLEMENTED!")
else()
  set(OPENEXR_IMF_NAMESPACE_CUSTOM 0)
  set(OPENEXR_IMF_NAMESPACE Imf)
endif()

# Create the configuration header and install it
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/OpenEXRConfig.h.cmake
  ${OPENEXR_CFG_DIR}/OpenEXRConfig.h
  @ONLY
)

if (NOT OPENEXR_IS_INTERNAL)
install(FILES ${OPENEXR_CFG_DIR}/OpenEXRConfig.h
  DESTINATION ${INSTALL_INCLUDE_DIR}
)
endif()

# Add the actual OpenEXR library
add_subdirectory(IlmImf)

if (NOT OPENEXR_IS_INTERNAL)

# Add the tests
if (BUILD_TESTING)
  add_subdirectory(IlmImfTest)

  option(OPENEXR_BUILD_FUZZTEST 
    "Build IlmImf damaged input resilience test" OFF)
  if (OPENEXR_BUILD_FUZZTEST)
    add_subdirectory(IlmImfFuzzTest)
  endif()
endif()

option(OPENEXR_BUILD_EXAMPLE
  "Build IlmImf example program" OFF)
if (OPENEXR_BUILD_EXAMPLE)
  add_subdirectory(IlmImfExamples)
endif()


##########################
# Installation
##########################

# Documentation
install (FILES
  doc/TechnicalIntroduction.pdf
  doc/ReadingAndWritingImageFiles.pdf
  doc/OpenEXRFileLayout.pdf
  doc/MultiViewOpenEXR.pdf
  doc/InterpretingDeepPixels.pdf
  doc/TheoryDeepPixels.pdf
  DESTINATION
  share/doc/OpenEXR-${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}
)

# Examples

install (FILES
  IlmImfExamples/main.cpp
  IlmImfExamples/drawImage.cpp
  IlmImfExamples/rgbaInterfaceExamples.cpp
  IlmImfExamples/rgbaInterfaceTiledExamples.cpp
  IlmImfExamples/generalInterfaceExamples.cpp
  IlmImfExamples/lowLevelIoExamples.cpp
  IlmImfExamples/previewImageExamples.cpp
  IlmImfExamples/generalInterfaceTiledExamples.cpp
  IlmImfExamples/generalInterfaceTiledExamples.h
  IlmImfExamples/drawImage.h
  IlmImfExamples/rgbaInterfaceExamples.h
  IlmImfExamples/generalInterfaceExamples.h
  IlmImfExamples/rgbaInterfaceTiledExamples.h
  IlmImfExamples/lowLevelIoExamples.h
  IlmImfExamples/previewImageExamples.h
  IlmImfExamples/namespaceAlias.h
  DESTINATION
  share/doc/OpenEXR-${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH}/examples
)

endif()
