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

# CMake file for the OpenEXR JNI binding version 2

# Read the version info from the version file at the root directory
file(READ "${PROJECT_SOURCE_DIR}/VERSION.txt" VERSION_TXT)
string(REGEX REPLACE ".*openexrjni - ([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1"
  OPENEXRJNI_VERSION "${VERSION_TXT}")
unset(VERSION_TXT)

set(SRCS
  util.h util.cpp
  jniBinding.cpp
)
  
# Add the resource file to the windows build
if(WIN32)
  list(APPEND SRCS 
    UnicodeStream.h UnicodeStream.cpp
    openexrjni.rc)
  add_definitions(-DUSE_JAVA_UTF8=0)
else()
  add_definitions(-DUSE_JAVA_UTF8=1)
endif()
  
# Java sources relative to the "java-src" directory
set(JAVA_SRCS
  java-src/edu/cornell/graphics/exr/Attributes.java
  java-src/edu/cornell/graphics/exr/Compression.java
  java-src/edu/cornell/graphics/exr/EXRIOException.java
  java-src/edu/cornell/graphics/exr/EXRSimpleImage.java
)
  
# The jar which will contain the compiled java files
set(OPENEXRJNI_JAR ${CMAKE_CURRENT_BINARY_DIR}/openexrjni-${OPENEXRJNI_VERSION}.jar)
  
# The jni headers are generated with javah from the java sources
set(JNI_HEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/edu_cornell_graphics_exr_EXRSimpleImage.h
  ${CMAKE_CURRENT_BINARY_DIR}/edu_cornell_graphics_exr_EXRSimpleImage_Channels.h
  ${CMAKE_CURRENT_BINARY_DIR}/edu_cornell_graphics_exr_EXRSimpleImage_OpenEXRTo.h
)
  
# Of course this module needs Java and JNI
find_package(Java REQUIRED)
find_package(JNI REQUIRED)

# Workaround for Cmake 2.6.0, as it doesn't set JNI_INCLUDE_DIRS
if(NOT JNI_INCLUDE_DIRS)
  set(JNI_INCLUDE_DIRS 
      ${JAVA_INCLUDE_PATH} 
      ${JAVA_INCLUDE_PATH2} 
      ${JAVA_AWT_INCLUDE_PATH}
  )
endif()

# Another workaround: as of Cmake 2.6.3 FindJNI ignores the "REQUIRED" flag
if(NOT JNI_INCLUDE_DIRS)
  message(FATAL_ERROR "Could NOT find JNI (missing header files)")
endif()

get_filename_component (JAVA_BIN_PATH "${JAVA_COMPILE}" PATH)
find_program( JAVA_JAVAH javah PATHS ${JAVA_BIN_PATH} )
mark_as_advanced( JAVA_BIN_PATH JAVA_JAVAH )
if(NOT JAVA_COMPILE OR NOT JAVA_ARCHIVE OR NOT JAVA_JAVAH)
  message(FATAL_ERROR "Not all the Java components (javac,jar,javah) were found.")
endif()


# Commands to builds the jar file
add_custom_command(OUTPUT ${OPENEXRJNI_JAR}
  COMMAND "${JAVA_COMPILE}" -source 1.6
    -d "${CMAKE_CURRENT_BINARY_DIR}"
    ${JAVA_SRCS}
  COMMAND "${JAVA_ARCHIVE}" cf "${OPENEXRJNI_JAR}" -C "${CMAKE_CURRENT_BINARY_DIR}" edu
  DEPENDS ${JAVA_SRCS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Compiling OpenEXR-JNI java files and builing jar..."
)
# Adds a target so that dependencies are properly tracked
add_custom_target(openexrjni_jar DEPENDS ${OPENEXRJNI_JAR})
set_target_properties(openexrjni_jar PROPERTIES FOLDER "Java-JNI")
  
# Build the jni headers using the jar
add_custom_command(OUTPUT ${JNI_HEADERS}
  COMMAND ${JAVA_JAVAH} -classpath ${OPENEXRJNI_JAR} edu.cornell.graphics.exr.EXRSimpleImage
  DEPENDS openexrjni_jar
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating OpenEXR JNI headers..."
)


if(OpenEXR_DEFINITIONS)
  add_definitions(${OpenEXR_DEFINITIONS})
endif()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR} 
  ${OpenEXR_INCLUDE_DIR} ${JNI_INCLUDE_DIRS}
)

# Finally adds the actual library targets
add_library(openexrjni SHARED ${SRCS} ${JNI_HEADERS})
HDRITOOLS_LTCG(openexrjni)
target_link_libraries(openexrjni ${OpenEXR_LIBRARIES})
add_dependencies(openexrjni openexrjni_jar)

# This is version 2 of the OpenEXR JNI addin
set_target_properties(openexrjni PROPERTIES
  VERSION "${OPENEXRJNI_VERSION}"
  FOLDER "Java-JNI"
)

# On Mac OS X the jni libs need an especial suffix
if(APPLE)
  set_target_properties(openexrjni PROPERTIES
    SUFFIX ".jnilib"
  )
endif()


# And installs the jar and jnilib
install(TARGETS openexrjni
  RUNTIME DESTINATION "${HDRITOOLS_SHARE_DIR}/java" COMPONENT "openexrjni"
  LIBRARY DESTINATION "${HDRITOOLS_SHARE_DIR}/java" COMPONENT "openexrjni"
)
install(FILES ${OPENEXRJNI_JAR}
  DESTINATION "${HDRITOOLS_SHARE_DIR}/java" COMPONENT "openexrjni"
)


################################# Tests with JUnit4 ##########################
option(BUILD_OPENEXR_JNI_TEST
  "Builds the JUnit4 tests for the OpenEXR JNI binding." OFF)
if (NOT BUILD_OPENEXR_JNI_TEST)
  return()
endif()

# TODO Refactor this into a FindJUnit4.cmake module, alla GTest
find_file(JUNIT4_JAR
  NAMES junit4.jar junit-4.jar
  PATHS /usr/share
  PATH_SUFFIXES java
)
if (NOT JUNIT4_JAR)
  message(FATAL_ERROR "The JUnit4 jar is required for the tests.")
else()
  mark_as_advanced(JUNIT4_JAR)
endif()


# Commands to builds the jar file with the tests
set(OPENEXRJNI_TEST_JAR ${CMAKE_CURRENT_BINARY_DIR}/openexrjni-test.jar)
set(JAVA_TEST_SRCS
  java_test-src/edu/cornell/graphics/exr/EXRSimpleImageTest.java
)

# Separator for the classpath
if (WIN32 AND NOT CYGWIN)
  set (PATH_SEPARATOR ";")
else()
  set (PATH_SEPARATOR ":")
endif()

# Create the output directory, otherwise javac freaks out
set(TEST_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/test)
if (NOT EXISTS ${TEST_OUT_DIR})
  file(MAKE_DIRECTORY ${TEST_OUT_DIR})
endif()
add_custom_command(OUTPUT ${OPENEXRJNI_TEST_JAR}
  COMMAND ${JAVA_COMPILE} -source 1.5
    -d ${TEST_OUT_DIR} -cp "${OPENEXRJNI_JAR}${PATH_SEPARATOR}${JUNIT4_JAR}"
    ${JAVA_TEST_SRCS}
  COMMAND ${JAVA_ARCHIVE} cf ${OPENEXRJNI_TEST_JAR} -C ${TEST_OUT_DIR} edu
  DEPENDS ${JAVA_TEST_SRCS} ${OPENEXRJNI_JAR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Compiling OpenEXR-JNI JUnit4 test java files and builing jar..."
)
# Adds a target so that dependencies are properly tracked
add_custom_target(openexrjni_test_jar DEPENDS ${OPENEXRJNI_TEST_JAR})
add_dependencies(openexrjni openexrjni_test_jar)
set_target_properties(openexrjni_test_jar PROPERTIES
  FOLDER "Java-JNI/test")

add_test(NAME JUnit4_EXRSimpleImageTest COMMAND
  "${Java_JAVA_EXECUTABLE}"
  -cp "${OPENEXRJNI_JAR}${PATH_SEPARATOR}${OPENEXRJNI_TEST_JAR}${PATH_SEPARATOR}${JUNIT4_JAR}"
  -Xcheck:jni -Djava.library.path=$<TARGET_FILE_DIR:openexrjni>
  org.junit.runner.JUnitCore edu.cornell.graphics.exr.EXRSimpleImageTest
)
