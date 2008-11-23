# Cmake specification for building qt4image. It assumes that Qt4 has been already found

# The general sources
# Headers to be moc-ified (a subset of the sources)
set(SRCS
  DoubleSpinSliderConnect.h DoubleSpinSliderConnect.cpp
  HDRImageLabel.h HDRImageLabel.cpp
  ImageDataProvider.h ImageDataProvider.cpp
  MainWindow.h MainWindow.cpp
  PixelInfoDialog.h PixelInfoDialog.cpp
  HDRImageDisplay.h HDRImageDisplay.cpp
  main.cpp
  )

# Headers to be moc-ified (a subset of the sources)
set(MOC_HEADERS
  DoubleSpinSliderConnect.h
  HDRImageLabel.h
  ImageDataProvider.h
  MainWindow.h
  PixelInfoDialog.h
  HDRImageDisplay.h
  )
  
# Run moc on the appropriate headers
QT4_WRAP_CPP(MOC_SRCS ${MOC_HEADERS})

# The ui files
set(UI_FILES
  ui/info.ui
  ui/main.ui
  )
  
# Prepare the ui files
QT4_WRAP_UI(UI_SRCS ${UI_FILES})

# Create a source group for the ui files
source_group("UI Files" FILES ${UI_FILES})


# Compile the resources also
SET(QRC_FILES
  res/icons.qrc
  )
QT4_ADD_RESOURCES(QRC_SRCS ${QRC_FILES})
source_group("Qt Resources" FILES ${QRC_FILES})


# Create a source group for the generated files
source_group("Qt Generated" FILES ${MOC_SRCS} ${UI_SRCS} ${QRC_SRCS})


if(WIN32)
  set(QT_USE_QTMAIN 1)
  set(EXECUTABLE_TYPE "WIN32")
  list(APPEND SRCS "res/qt4Image.rc")
endif(WIN32)

# Load the QT configuration
include(${QT_USE_FILE})

# Creates the executable with all the stuff
add_executable(qt4Image ${EXECUTABLE_TYPE} ${SRCS} ${MOC_SRCS} ${UI_FILES} ${UI_SRCS} ${QRC_SRCS})
target_link_libraries(qt4Image ImageIO ${QT_LIBRARIES})

# Add the image plugins with the fancy macro
QT_ADD_IMG_PLUGINS( qt4Image )

# uic generates header in the build directory which need to be found
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
  
# TODO: there should be a better way to do this...
include_directories(../ImageIO)

# Installs this
install(TARGETS qt4Image
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  )

# For windows we also generate and install .reg file to register the program with Vista's association mechanism.
# This should be an option in the program, but for now this should do the trick
if(WIN32)
  set(QT4_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/bin/qt4Image.exe")
  string(REPLACE "/" "\\\\" QT4_INSTALL_PATH ${QT4_INSTALL_PATH})
  
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/win32/InstallVista.reg.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/InstallVista.reg
	ESCAPE_QUOTES
  )
  
  install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/InstallVista.reg
    ${CMAKE_CURRENT_SOURCE_DIR}/win32/UninstallVista.reg
	${CMAKE_CURRENT_SOURCE_DIR}/win32/Types.reg
	${CMAKE_CURRENT_SOURCE_DIR}/win32/Types-Photoshop.reg
	${CMAKE_CURRENT_SOURCE_DIR}/win32/UninstallTypes.reg
	${CMAKE_CURRENT_SOURCE_DIR}/win32/UninstallTypes-Photoshop.reg
	DESTINATION win32
	)


endif(WIN32)