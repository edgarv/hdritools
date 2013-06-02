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
  EXRInputFile.cpp
  EXRJavaStream.h EXRJavaStream.cpp
  EXRSimpleImage.h EXRSimpleImage.cpp
  StdSStream.h StdSStream.cpp
  util.h util.cpp
  jniBinding.cpp
)
  
# Add the resource file to the windows build
if(WIN32)
  set(RC_PRODUCTVERSION_COMMA ${HDRITOOLS_VERSION_MAJOR},${HDRITOOLS_VERSION_MINOR},${HDRITOOLS_VERSION_PATCH},0)
  if(HDRITOOLS_HAS_VALID_REV)
    set(RC_PRODUCTVERSION ${HDRITOOLS_VERSION}-${HDRITOOLS_VERSION_BUILD}hg${HDRITOOLS_REV_ID})
  else()
    set(RC_PRODUCTVERSION ${HDRITOOLS_VERSION})
  endif()
  set(RC_FILEVERSION ${OPENEXRJNI_VERSION})
  string(REPLACE "." "," RC_FILEVERSION_COMMA "${OPENEXRJNI_VERSION}.0")
  configure_file("openexrjni.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/openexrjni.rc")

  list(APPEND SRCS 
    UnicodeStream.h UnicodeStream.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/openexrjni.rc")
  add_definitions(-DUSE_JAVA_UTF8=0)
else()
  add_definitions(-DUSE_JAVA_UTF8=1)
endif()
  
# Java sources relative to the "java-src" directory
set(JAVA_SRCS
  java-src/edu/cornell/graphics/exr/Attributes.java
  java-src/edu/cornell/graphics/exr/BasicInputFile.java
  java-src/edu/cornell/graphics/exr/Channel.java
  java-src/edu/cornell/graphics/exr/ChannelList.java
  java-src/edu/cornell/graphics/exr/Chromaticities.java
  java-src/edu/cornell/graphics/exr/Compression.java
  java-src/edu/cornell/graphics/exr/EnvMap.java
  java-src/edu/cornell/graphics/exr/EXRInputFile.java
  java-src/edu/cornell/graphics/exr/EXROutputFile.java
  java-src/edu/cornell/graphics/exr/EXRIOException.java
  java-src/edu/cornell/graphics/exr/EXRSimpleImage.java
  java-src/edu/cornell/graphics/exr/EXRTypeException.java
  java-src/edu/cornell/graphics/exr/EXRVersion.java
  java-src/edu/cornell/graphics/exr/FrameBuffer.java
  java-src/edu/cornell/graphics/exr/Half.java
  java-src/edu/cornell/graphics/exr/Header.java
  java-src/edu/cornell/graphics/exr/KeyCode.java
  java-src/edu/cornell/graphics/exr/LineOrder.java
  java-src/edu/cornell/graphics/exr/NoSuchChannelException.java
  java-src/edu/cornell/graphics/exr/PixelType.java
  java-src/edu/cornell/graphics/exr/PreviewImage.java
  java-src/edu/cornell/graphics/exr/Rational.java
  java-src/edu/cornell/graphics/exr/Slice.java
  java-src/edu/cornell/graphics/exr/TileDescription.java
  java-src/edu/cornell/graphics/exr/TimeCode.java
  
  java-src/edu/cornell/graphics/exr/apps/EXRHeaderApp.java
  
  java-src/edu/cornell/graphics/exr/attributes/Attribute.java
  java-src/edu/cornell/graphics/exr/attributes/AttributeFactory.java
  java-src/edu/cornell/graphics/exr/attributes/Box2fAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/Box2iAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/ChannelListAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/ChromaticitiesAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/CompressionAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/DoubleAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/EnvMapAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/FloatAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/IntegerAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/KeyCodeAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/LineOrderAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/M33dAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/M33fAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/M44dAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/M44fAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/OpaqueAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/PreviewImageAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/RationalAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/StringAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/StringVectorAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/TileDescriptionAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/TimeCodeAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/TypedAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/V2dAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/V2fAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/V2iAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/V3dAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/V3fAttribute.java
  java-src/edu/cornell/graphics/exr/attributes/V3iAttribute.java
  
  java-src/edu/cornell/graphics/exr/ilmbaseto/Box2.java
  java-src/edu/cornell/graphics/exr/ilmbaseto/Matrix33.java
  java-src/edu/cornell/graphics/exr/ilmbaseto/Matrix44.java
  java-src/edu/cornell/graphics/exr/ilmbaseto/Vector2.java
  java-src/edu/cornell/graphics/exr/ilmbaseto/Vector3.java
  
  java-src/edu/cornell/graphics/exr/io/ByteBufferInputStream.java
  java-src/edu/cornell/graphics/exr/io/ByteChannelInputStream.java
  java-src/edu/cornell/graphics/exr/io/EXRByteArrayOutputStream.java
  java-src/edu/cornell/graphics/exr/io/EXRFileInputStream.java
  java-src/edu/cornell/graphics/exr/io/EXRInputStream.java
  java-src/edu/cornell/graphics/exr/io/EXROutputStream.java
  java-src/edu/cornell/graphics/exr/io/InputFileInfo.java
  java-src/edu/cornell/graphics/exr/io/XdrInput.java
  java-src/edu/cornell/graphics/exr/io/XdrOutput.java
)
  
# The jar which will contain the compiled java files
set(OPENEXRJNI_JAR
  "${CMAKE_CURRENT_BINARY_DIR}/openexrjni-${OPENEXRJNI_VERSION}.jar")
set(OPENEXRJNI_JAVADOC_JAR
  "${CMAKE_CURRENT_BINARY_DIR}/openexrjni-${OPENEXRJNI_VERSION}-javadoc.jar")
  
# The jni headers are generated with javah from the java sources
set(JNI_HEADERS
  "${CMAKE_CURRENT_BINARY_DIR}/include/edu_cornell_graphics_exr_EXRSimpleImage.h"
  "${CMAKE_CURRENT_BINARY_DIR}/include/edu_cornell_graphics_exr_EXRSimpleImage_Channels.h"
  "${CMAKE_CURRENT_BINARY_DIR}/include/edu_cornell_graphics_exr_EXRSimpleImage_OpenEXRTo.h"
  "${CMAKE_CURRENT_BINARY_DIR}/include/edu_cornell_graphics_exr_EXRInputFile.h"
  "${CMAKE_CURRENT_BINARY_DIR}/include/edu_cornell_graphics_exr_EXROutputFile.h"
)
  
# Of course this module needs Java and JNI
find_package(Java 1.7 REQUIRED)
find_package(JNI REQUIRED)

# Another workaround: as of Cmake 2.6.3 FindJNI ignores the "REQUIRED" flag
if(NOT JNI_INCLUDE_DIRS)
  message(FATAL_ERROR "Could NOT find JNI (missing header files)")
endif()

# JAVA_RUNTIME, JAVA_COMPILE, JAVA_ARCHIVE were deprecated in CMake 2.8.1
if(JAVA_RUNTIME AND NOT Java_JAVA_EXECUTABLE)
  set(Java_JAVA_EXECUTABLE "${JAVA_RUNTIME}")
endif()
if(JAVA_COMPILE AND NOT Java_JAVAC_EXECUTABLE)
  set(Java_JAVAC_EXECUTABLE "${JAVA_COMPILE}")
endif()
if(JAVA_ARCHIVE AND NOT Java_JAR_EXECUTABLE)
  set(Java_JAR_EXECUTABLE "${JAVA_ARCHIVE}")
endif()

# Java_JAVAH_EXECUTABLE, Java_JAVADOC_EXECUTABLE were added in CMake 2.8.6
if(CMAKE_VERSION VERSION_LESS "2.8.6")
  get_filename_component(_JAVA_BIN_PATH "${Java_JAVAC_EXECUTABLE}" PATH)
  find_program(Java_JAVAH_EXECUTABLE   javah
    HINTS "${_JAVA_BIN_PATH}" NO_DEFAULT_PATH)
  find_program(Java_JAVADOC_EXECUTABLE javadoc 
    HINTS "${_JAVA_BIN_PATH}" NO_DEFAULT_PATH)
  mark_as_advanced(Java_JAVAH_EXECUTABLE Java_JAVADOC_EXECUTABLE)
endif()

if(NOT Java_JAVAC_EXECUTABLE OR NOT Java_JAR_EXECUTABLE OR
   NOT Java_JAVAH_EXECUTABLE OR NOT Java_JAVADOC_EXECUTABLE)
  message(FATAL_ERROR "Not all of the Java components were found.")
endif()


# Commands to builds the jar file
add_custom_command(OUTPUT "${OPENEXRJNI_JAR}"
  COMMAND "${Java_JAVAC_EXECUTABLE}" -source 1.7 -Xlint:unchecked
    -d "${CMAKE_CURRENT_BINARY_DIR}"
    ${JAVA_SRCS}
  COMMAND "${Java_JAR_EXECUTABLE}"
    cMf "${OPENEXRJNI_JAR}" -C "${CMAKE_CURRENT_BINARY_DIR}" edu
  DEPENDS ${JAVA_SRCS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Compiling OpenEXR-JNI java files and builing jar..."
)
# Adds a target so that dependencies are properly tracked
add_custom_target(openexrjni_jar ALL DEPENDS "${OPENEXRJNI_JAR}")
set_target_properties(openexrjni_jar PROPERTIES FOLDER "Java-JNI")

# Build the javadoc and package it in a jar
set(JAVADOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/javadoc")
if(NOT EXISTS "${JAVADOC_DIR}/doc")
  file(MAKE_DIRECTORY "${JAVADOC_DIR}/doc")
endif()
configure_file("java-src/overview.html" "${JAVADOC_DIR}/overview.html" @ONLY)
add_custom_command(OUTPUT "${OPENEXRJNI_JAVADOC_JAR}"
  COMMAND "${Java_JAVADOC_EXECUTABLE}" -d "${JAVADOC_DIR}/doc"
    -doctitle "Java bindings for OpenEXR ${OPENEXRJNI_VERSION}"
    -overview "${JAVADOC_DIR}/overview.html"
    -link "http://docs.oracle.com/javase/6/docs/api/" ${JAVA_SRCS}
  COMMAND "${Java_JAR_EXECUTABLE}" cMf "${OPENEXRJNI_JAVADOC_JAR}"
    -C "${JAVADOC_DIR}" doc
  DEPENDS ${JAVA_SRCS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Creating OpenEXR-JNI javadoc and its jar..."
)
add_custom_target(openexrjni_javadoc_jar ALL DEPENDS "${OPENEXRJNI_JAVADOC_JAR}")
set_target_properties(openexrjni_javadoc_jar PROPERTIES FOLDER "Java-JNI")
 
# Build the jni headers using the jar
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
add_custom_command(OUTPUT ${JNI_HEADERS}
  COMMAND "${Java_JAVAH_EXECUTABLE}" -classpath "${OPENEXRJNI_JAR}"
    edu.cornell.graphics.exr.EXRSimpleImage
    edu.cornell.graphics.exr.EXRInputFile
    edu.cornell.graphics.exr.EXROutputFile
  DEPENDS openexrjni_jar
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include"
  COMMENT "Generating OpenEXR JNI headers..."
)


if(OpenEXR_DEFINITIONS)
  add_definitions(${OpenEXR_DEFINITIONS})
endif()

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}/include"
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
install(FILES "${OPENEXRJNI_JAR}" "${OPENEXRJNI_JAVADOC_JAR}"
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
  java_test-src/jp/ac/hiroshima_u/sci/math/saito/tinymt/F2Polynomial.java
  java_test-src/jp/ac/hiroshima_u/sci/math/saito/tinymt/TinyMT32.java
  java_test-src/jp/ac/hiroshima_u/sci/math/saito/tinymt/TinyMT32Parameter.java
  
  java_test-src/edu/cornell/graphics/exr/AllTests.java
  java_test-src/edu/cornell/graphics/exr/EXRSimpleImageTest.java
  java_test-src/edu/cornell/graphics/exr/EXRInputFileBasicTest.java
  java_test-src/edu/cornell/graphics/exr/HalfTest.java
  java_test-src/edu/cornell/graphics/exr/HeaderBasicTest.java
  java_test-src/edu/cornell/graphics/exr/PixelTypeTest.java
  java_test-src/edu/cornell/graphics/exr/io/InputFileInfoBasicTest.java
)

# Tests resources to be included in the jar, relative to "java_test-src"
set(TEST_RESOURCES
  edu/cornell/graphics/exr/resources/test-piz-rgb.exr
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
  COMMAND "${Java_JAVAC_EXECUTABLE}" -source 1.7 -Xlint:unchecked
    -d ${TEST_OUT_DIR} -cp "${OPENEXRJNI_JAR}${PATH_SEPARATOR}${JUNIT4_JAR}"
    ${JAVA_TEST_SRCS}
  COMMAND "${Java_JAR_EXECUTABLE}"
    cMf ${OPENEXRJNI_TEST_JAR} -C ${TEST_OUT_DIR} edu -C ${TEST_OUT_DIR} jp
    -C "java_test-src" ${TEST_RESOURCES}
  DEPENDS ${JAVA_TEST_SRCS} ${OPENEXRJNI_JAR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Compiling OpenEXR-JNI JUnit4 test java files and building jar..."
)
# Adds a target so that dependencies are properly tracked
add_custom_target(openexrjni_test_jar DEPENDS ${OPENEXRJNI_TEST_JAR})
add_dependencies(openexrjni openexrjni_test_jar)
set_target_properties(openexrjni_test_jar PROPERTIES
  FOLDER "Java-JNI/test")

add_test(NAME JUnit4_OpenEXR COMMAND
  "${Java_JAVA_EXECUTABLE}"
  -cp "${OPENEXRJNI_JAR}${PATH_SEPARATOR}${OPENEXRJNI_TEST_JAR}${PATH_SEPARATOR}${JUNIT4_JAR}"
  -Xcheck:jni -Djava.library.path=$<TARGET_FILE_DIR:openexrjni>
  org.junit.runner.JUnitCore edu.cornell.graphics.exr.AllTests
)
