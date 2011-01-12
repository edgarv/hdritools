# Cmake specification for building qt4image. 
# It assumes that Qt4 has been already found

# Offer an option to automatically bundle qt4image through BundleUtilities
include(CMakeDependentOption)
if(APPLE OR WIN32)
  set(QT4IMAGE_BUNDLE_DEFAULT ON)
else()
  set(QT4IMAGE_BUNDLE_DEFAULT OFF)
endif()
if(NOT "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" VERSION_LESS "2.8")
  set(HAS_BUNDLE_SUPPORT TRUE)
endif()
CMAKE_DEPENDENT_OPTION(QT4IMAGE_BUNDLE
  "Automatically bundle all qt4image dependencies." ${QT4IMAGE_BUNDLE_DEFAULT}
  "HAS_BUNDLE_SUPPORT" OFF)

# The general sources
set(SRCS
  DoubleSpinSliderConnect.h DoubleSpinSliderConnect.cpp
  HDRImageLabel.h HDRImageLabel.cpp
  ImageDataProvider.h ImageDataProvider.cpp
  MainWindow.h MainWindow.cpp
  PixelInfoDialog.h PixelInfoDialog.cpp
  HDRImageDisplay.h HDRImageDisplay.cpp
  ImageApp.h ImageApp.cpp
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
  ImageApp.h
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
endif()


# Prepare the bundle for mac
if(APPLE AND QT4IMAGE_BUNDLE)
  set(EXECUTABLE_TYPE "MACOSX_BUNDLE")
  set(MACOSX_BUNDLE_BUNDLE_NAME "Qt4Image")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "edu.cornell.graphics.Qt4Image")
  set(MACOSX_BUNDLE_COPYRIGHT 
    "Program of Computer Graphics, Cornell University")
  # Set how it shows up in the Info.plists file
  set(MACOSX_BUNDLE_ICON_FILE "app.icns")
  
  # Set where in the bundle to put the icns file
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/res/app.icns
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # Include the icns file in the target
  list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/res/app.icns)
endif()


# Load the QT configuration
include(${QT_USE_FILE})

# Creates the executable with all the stuff
add_executable(qt4Image ${EXECUTABLE_TYPE} 
  ${SRCS} ${MOC_SRCS} ${UI_FILES} ${UI_SRCS} ${QRC_SRCS})
target_link_libraries(qt4Image ImageIO ${QT_LIBRARIES})

# Add the image plugins with the fancy macro
QT_ADD_IMG_PLUGINS( qt4Image )


# Add the custom Info.plist to the target after configuring it
if(APPLE AND QT4IMAGE_BUNDLE)
  set(MACOSX_BUNDLE_EXECUTABLE_NAME "qt4Image")
  configure_file(res/Info.plist.in "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    set_target_properties(qt4Image PROPERTIES
	  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
endif()



# uic generates header in the build directory which need to be found
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
  
# TODO: there should be a better way to do this...
include_directories(../ImageIO)

# Installs this
install(TARGETS qt4Image
  RUNTIME DESTINATION bin COMPONENT Runtime
  LIBRARY DESTINATION lib COMPONENT Developer
  BUNDLE  DESTINATION .   COMPONENT Runtime
  )

# For windows we also generate and install a .reg file to register the program 
# with Vista's association mechanism.
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

# Stop further processing if there is no bundle-stuff
if(NOT QT4IMAGE_BUNDLE)
  return()
endif()

# Check CMake version before going further
cmake_policy(PUSH)
if (NOT APPLE)
  cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
else()
  cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)
endif()
cmake_policy(POP)

# Additional bunlde installation 
set(plugin_dest_dir bin)
set(qtconf_dest_dir bin)
set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/qt4Image")
if(APPLE)
  set(plugin_dest_dir "qt4Image.app/Contents")
  set(qtconf_dest_dir "qt4Image.app/Contents/Resources")
  set(APPS "\${CMAKE_INSTALL_PREFIX}/qt4Image.app")
elseif(WIN32)
  set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/qt4Image.exe")
endif()

# Install the needed Qt plugins
if(EXISTS "${QT_PLUGINS_DIR}/imageformats" AND NOT(QT_CONFIG MATCHES "static"))
  if(NOT WIN32 OR NOT (QT_QTGUI_LIBRARY_RELEASE AND QT_QTGUI_LIBRARY_DEBUG))
    install(DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
      DESTINATION ${plugin_dest_dir}/plugins
      COMPONENT Runtime
	  REGEX "svg|mng|gif" EXCLUDE)
  else()
    # There are different plugins for release and debug
	install(DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
      DESTINATION ${plugin_dest_dir}/plugins
      COMPONENT Runtime CONFIGURATIONS Debug
	  REGEX ".+d4\\.dll$"
	  REGEX "svg|mng|gif" EXCLUDE)
	
	install(DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
      DESTINATION ${plugin_dest_dir}/plugins
      COMPONENT Runtime CONFIGURATIONS Release MinSizeRel RelWithDebInfo
	  REGEX ".+d4\\.dll$" EXCLUDE
	  REGEX "svg|mng|gif" EXCLUDE)
  endif()
  
endif()

# The Cocoa version needs extra resources
set(QTCOCOA_MENU_NIB ${QT_QTGUI_LIBRARY}/Resources/qt_menu.nib)
if(QT_MAC_USE_COCOA AND EXISTS "${QTCOCOA_MENU_NIB}")

  install(CODE "
    message(STATUS \"Copying qt_menu.nib into the Bundle resources\")
    file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt_menu.nib\")
    execute_process(COMMAND \${CMAKE_COMMAND} -E copy_directory
      \"${QTCOCOA_MENU_NIB}\" 
      \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt_menu.nib\")
    " COMPONENT Runtime)
endif()

# Install a qt.conf file by running cmake code from the install script
install(CODE "
  file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt.conf\" \"\")
  " COMPONENT Runtime)

# Use BundleUtils to get all other dependencies to work. For Windows and Linux
# CMake 2.8 is required

# Directories to look for dependencies
set(DIRS ${QT_LIBRARY_DIR})
if (WIN32)
  get_filename_component(QT_BINARY_DIR ${QT_QMAKE_EXECUTABLE} PATH)
  list(APPEND DIRS ${QT_BINARY_DIR})
endif()

# The default downloaded version of TBB seems to be problematic...
if (EXISTS ${TBB_OPTIMIZED_LIBRARY})
  get_filename_component(TBB_LIB_DIR ${TBB_OPTIMIZED_LIBRARY} PATH)
  list(APPEND DIRS ${TBB_LIB_DIR})
  if (WIN32)
    string(REGEX REPLACE "lib([/\\].+[.//].+)?[/\\]?$" "bin\\1"
	  TBB_BIN_DIR ${TBB_LIB_DIR})
	list(APPEND DIRS ${TBB_BIN_DIR})
  endif()
endif()

install(CODE "
  if(APPLE OR WIN32)
  # gp_item_default_embedded_path item default_embedded_path_var
  #
  # Return the path that others should refer to the item by when the item
  # is embedded inside a bundle.
  #
  # Override on a per-project basis by providing a project-specific
  # gp_item_default_embedded_path_override function
  #
  function(gp_item_default_embedded_path_override item
    default_embedded_path_var)
    # The assumption here is that all executables in the bundle will be
    # in the same-level-directories inside the bundle. The parent directory
    # of an executable inside the bundle should be MacOS or a sibling of
    # MacOS and all embedded paths returned from here will begin with
    # \"@executable_path/../\" and will work from all executables in all
    # such same-level-directories inside the bundle
    #

    # By default, embed things right next to the main bundle executable:
	if(APPLE)
      set(path \"@executable_path/../../Contents/MacOS\")
	elseif(WIN32)
	  set(path \"@executable_path\")
	endif()

    set(overriden FALSE)
    
    # Qt plugins must reside in the appropriate folders within Contents/plugins
    set(qtplugin_regex \"(plugins[\\\\/][^\\\\/]+)[\\\\/][^\\\\/]+\\\\${CMAKE_SHARED_LIBRARY_SUFFIX}\$\")
    if(item MATCHES \"\${qtplugin_regex}\")
	  if(APPLE)
        set(path \"@executable_path/../\${CMAKE_MATCH_1}\")
	  elseif(WIN32)
	    set(path \"@executable_path/\${CMAKE_MATCH_1}\")
	  endif()
      set(overriden TRUE)
    endif()

    # Embed frameworks in the embedded Frameworks directory (sibling of MacOS)
    if(NOT overriden)
      if(item MATCHES \"[^/]+\\\\.framework/\")
        set(path \"@executable_path/../Frameworks\")
	set(overriden TRUE)
      endif()
    endif()

    set(\${default_embedded_path_var} \"\${path}\" PARENT_SCOPE)
  endfunction()
  endif() # APPLE OR WIN32
  
  file(GLOB_RECURSE QTPLUGINS
    \"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
  include(BundleUtilities)
  fixup_bundle(\"${APPS}\" \"\${QTPLUGINS}\" \"${DIRS}\")
  " COMPONENT Runtime)

