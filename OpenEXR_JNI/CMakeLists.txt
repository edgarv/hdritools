# CMake file for the OpenEXR JNI binding version 2

set(SRCS
  util.h util.cpp
  jniBinding.cpp
  )
  
# Add the resource file to the windows build
if(WIN32)
  list(APPEND SRCS 
    UnicodeStream.h UnicodeStream.cpp
    openexrjni.rc
	)
  add_definitions(-DUSE_JAVA_UTF8=0)
else(WIN32)
  add_definitions(-DUSE_JAVA_UTF8=1)
endif(WIN32)
  
# Java sources relative to the "java-src" directory
set(JAVA_SRCS
  java-src/edu/cornell/graphics/exr/Attributes.java
  java-src/edu/cornell/graphics/exr/Compression.java
  java-src/edu/cornell/graphics/exr/EXRIOException.java
  java-src/edu/cornell/graphics/exr/EXRSimpleImage.java
  )
  
# The jar which will contain the compiled java files
set(OPENEXRJNI_JAR ${CMAKE_CURRENT_BINARY_DIR}/openexrjni.jar)
  
# The jni headers are generated with javah from the java sources
set(JNI_HEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/edu_cornell_graphics_exr_EXRSimpleImage.h
  )
  
# Of course this module needs Java and JNI
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
get_filename_component (JAVA_BIN_PATH "${JAVA_COMPILE}" PATH)
find_program( JAVA_JAVAH javah PATHS ${JAVA_BIN_PATH} )
mark_as_advanced( JAVA_BIN_PATH JAVA_JAVAH )
if(NOT JAVA_COMPILE OR NOT JAVA_ARCHIVE OR NOT JAVA_JAVAH)
  message(FATAL_ERROR "Not all the Java components (javac,jar,javah) were found.")
endif(NOT JAVA_COMPILE OR NOT JAVA_ARCHIVE OR NOT JAVA_JAVAH)


# Builds the jar file
add_custom_command(OUTPUT ${OPENEXRJNI_JAR}
  COMMAND ${JAVA_COMPILE} -target 1.5
     -d ${CMAKE_CURRENT_BINARY_DIR}
	 ${JAVA_SRCS}
  COMMAND ${JAVA_ARCHIVE} cf ${OPENEXRJNI_JAR} -C ${CMAKE_CURRENT_BINARY_DIR} edu
  DEPENDS ${JAVA_SRCS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Compiling OpenEXR-JNI java files and builing jar..."
  )
  
# Build the jni headers using the jar
add_custom_command(OUTPUT ${JNI_HEADERS}
  COMMAND ${JAVA_JAVAH} -classpath ${OPENEXRJNI_JAR} edu.cornell.graphics.exr.EXRSimpleImage
  MAIN_DEPENDENCY ${OPENEXRJNI_JAR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating OpenEXR JNI headers..."
  )
  
  
# Also provide the jar as a custom command, so that it may be built with "make openexrjni.jar"
add_custom_target(openexrjni.jar
  ${JAVA_COMPILE} -target 1.5
     -d ${CMAKE_CURRENT_BINARY_DIR}
	 ${JAVA_SRCS}
  COMMAND ${JAVA_ARCHIVE} cf ${OPENEXRJNI_JAR} -C ${CMAKE_CURRENT_BINARY_DIR} edu
  DEPENDS ${JAVA_SRCS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Compiling OpenEXR-JNI java files and builing jar..."
  )

 
if(OpenEXR_DEFINITIONS)
  add_definitions(${OpenEXR_DEFINITIONS})
endif(OpenEXR_DEFINITIONS)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR} 
  ${OpenEXR_INCLUDE_DIR} ${JNI_INCLUDE_DIRS}
  )

# Finally adds the actual library targets
add_library(openexrjni SHARED ${SRCS} ${JNI_HEADERS})
target_link_libraries(openexrjni ${OpenEXR_LIBRARIES})

# This is version 2 of the OpenEXR JNI addin
set_target_properties(openexrjni PROPERTIES
  VERSION "2.0.0"
  )

# On Mac OS X the jni libs need an especial suffix
if(APPLE)
  set_target_properties(openexrjni PROPERTIES
    SUFFIX ".jnilib"
    )
endif(APPLE)


# And installs the jar and jnilib
# Installs this
install(TARGETS openexrjni
  RUNTIME DESTINATION "bin"
  LIBRARY DESTINATION "lib"
  )
install(FILES ${OPENEXRJNI_JAR}
  DESTINATION "jar"
  )
