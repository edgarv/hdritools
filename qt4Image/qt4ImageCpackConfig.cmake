# The following stuff related to file associations in Win Vista+ for qt4Image,
# but it could be factorized for a more generalized setting
if (NOT DEFINED CPACK_PACKAGE_VENDOR OR NOT DEFINED CPACK_PACKAGE_NAME OR
    NOT DEFINED CPACK_PACKAGE_VERSION OR NOT DEFINED HDRITOOLS_VERSION_MAJOR)
  message(FATAL_ERROR "The required variables are not set!")
endif()


# Use the qt4Image.exe icon for "Add or Remove Programs"
if (NOT CPACK_NSIS_INSTALLED_ICON_NAME)
  set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\qt4Image.exe")
endif()

# This seems to affect only Windows installers
set(CPACK_PACKAGE_EXECUTABLES ${CPACK_PACKAGE_EXECUTABLES} qt4Image "Qt4Image")


macro(NSIS_DOC_CLASS outvar_install outvar_uninstall classname iconidx)
  # The "double" escaping is because this variable will be used to generate the
  # 'CPackConfig.cmake' file, and then those values are used in 'configure_file'
  set(${outvar_install} 
    "\\n; Register the ${classname} type\\n"
    " Perceived type\\n  WriteRegStr SHCTX 'SOFTWARE\\\\Classes\\\\${classname}' 'PerceivedType' 'Image'\\n"
    " Default icon\\n  WriteRegStr SHCTX 'SOFTWARE\\\\Classes\\\\${classname}\\\\DefaultIcon' '' '\\\$INSTDIR\\\\bin\\\\qt4Image.exe,${iconidx}'\\n"
    " Edit command\\n  WriteRegStr SHCTX 'SOFTWARE\\\\Classes\\\\${classname}\\\\shell\\\\edit\\\\command' '' '\\\"\\\$INSTDIR\\\\bin\\\\qt4Image.exe\\\" \\\"%1\\\"'\\n"
    " Open command\\n  WriteRegStr SHCTX 'SOFTWARE\\\\Classes\\\\${classname}\\\\shell\\\\open\\\\command' '' '\\\"\\\$INSTDIR\\\\bin\\\\qt4Image.exe\\\" \\\"%1\\\"'\\n"
    " Preview command\\n  WriteRegStr SHCTX 'SOFTWARE\\\\Classes\\\\${classname}\\\\shell\\\\preview\\\\command' '' '\\\"\\\$INSTDIR\\\\bin\\\\qt4Image.exe\\\" \\\"%1\\\"'\\n\\n"
  )
  set(${outvar_uninstall}
    "\\n; Delete the whole registry tree for ${classname}\\n  DeleteRegKey SHCTX 'SOFTWARE\\\\Classes\\\\${classname}'\\n"
  )
endmacro()


# Create the key for the document classes
set(QT4IMAGE_PROG_KEY   "PCG.qt4Image.${HDRITOOLS_VERSION_MAJOR}")
set(QT4IMAGE_EXR_CLASS  "PCG.qt4Image.OpenEXRFile.${HDRITOOLS_VERSION_MAJOR}")
set(QT4IMAGE_RGBE_CLASS "PCG.qt4Image.RGBEFile.${HDRITOOLS_VERSION_MAJOR}")
set(QT4IMAGE_HDR_CLASS  "PCG.qt4Image.RadianceFile.${HDRITOOLS_VERSION_MAJOR}")
set(QT4IMAGE_PBM_CLASS  "PCG.qt4Image.PBMFile.${HDRITOOLS_VERSION_MAJOR}")

# TODO Make a way to keep the indices and the resource file in sync
NSIS_DOC_CLASS(NSIS_EXR_INSTALL  NSIS_EXR_UNINSTALL  ${QT4IMAGE_EXR_CLASS}  1)
NSIS_DOC_CLASS(NSIS_RGBE_INSTALL NSIS_RGBE_UNINSTALL ${QT4IMAGE_RGBE_CLASS} 2)
NSIS_DOC_CLASS(NSIS_HDR_INSTALL  NSIS_HDR_UNINSTALL  ${QT4IMAGE_HDR_CLASS}  3)
NSIS_DOC_CLASS(NSIS_PBM_INSTALL  NSIS_PBM_UNINSTALL  ${QT4IMAGE_PBM_CLASS}  4)

# Create the keys for the Win Vista+ capabilities (for "Default Programs")
set(NSIS_QT4IMAGE_CAPABILITIES_KEY
  "SOFTWARE\\\\${CPACK_PACKAGE_VENDOR}\\\\${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}\\\\qt4Image"
)
# TODO Factorize and use a macro to add the file types
set(NSIS_QT4IMAGE_CAPABILITIES_INSTALL
  "\\n; Qt4Image capabilities for Default Programs\\n"
  " Basic key\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}' '' 'qt4Image'\\n"
  " Friendly application name\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\Capabilities' 'ApplicationName' 'Qt4Image'\\n"
  " Resource with the description\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\Capabilities' 'ApplicationDescription' '@\\\$INSTDIR\\\\bin\\\\qt4Image.exe,-20'\\n"
  
  " OpenEXR files\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\Capabilities\\\\FileAssociations' '.exr' '${QT4IMAGE_EXR_CLASS}'\\n"
  " RGBE files\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\Capabilities\\\\FileAssociations' '.rgbe' '${QT4IMAGE_RGBE_CLASS}'\\n"
  " Radiance files\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\Capabilities\\\\FileAssociations' '.hdr' '${QT4IMAGE_HDR_CLASS}'\\n"
  " PBM files\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\Capabilities\\\\FileAssociations' '.pfm' '${QT4IMAGE_PBM_CLASS}'\\n"
  
  " Default icon (general)\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\DefaultIcon' '' '\\\$INSTDIR\\\\bin\\\\qt4Image.exe,0'\\n"
  " Open command\\n  WriteRegStr SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\shell\\\\open\\\\command' '' '\\\"\\\$INSTDIR\\\\bin\\\\qt4Image.exe\\\" \\\"%1\\\"'\\n"
  
  " Inform the system of the program\\n  WriteRegStr SHCTX 'SOFTWARE\\\\RegisteredApplications' '${QT4IMAGE_PROG_KEY}' '${NSIS_QT4IMAGE_CAPABILITIES_KEY}\\\\Capabilities'\\n"
)

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
  ${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
  ${NSIS_EXR_INSTALL} ${NSIS_RGBE_INSTALL}
  ${NSIS_HDR_INSTALL} ${NSIS_PBM_INSTALL}
  ${NSIS_QT4IMAGE_CAPABILITIES_INSTALL}
  "\\n  !insertmacro UPDATEFILEASSOC\\n"
)


# Remove those keys from the registry
set(NSIS_QT4IMAGE_CAPABILITIES_UNINSTALL
  "\\n; Unregister the application\\n  DeleteRegValue SHCTX 'SOFTWARE\\\\RegisteredApplications' '${QT4IMAGE_PROG_KEY}'\\n"
  " Delete the whole qt4image subtree\\n  DeleteRegKey SHCTX '${NSIS_QT4IMAGE_CAPABILITIES_KEY}'\\n"
)

set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
  ${NSIS_QT4IMAGE_CAPABILITIES_UNINSTALL}
  ${NSIS_PBM_UNINSTALL} ${NSIS_HDR_UNINSTALL} 
  ${NSIS_RGBE_UNINSTALL} ${NSIS_EXR_UNINSTALL}
  "\\n  !insertmacro UPDATEFILEASSOC\\n"
  ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
)