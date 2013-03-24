#include <winresrc.h>

#cmakedefine MSVC_VER_STRING "@MSVC_VER_STRING@"

#ifdef _DEBUG
# define FILENAME_STRING "@RC_FILENAME@d.dll\0"
#else
# define FILENAME_STRING "@RC_FILENAME@.dll\0"
#endif

#ifdef GCC_WINDRES
VS_VERSION_INFO   VERSIONINFO
#else
VS_VERSION_INFO   VERSIONINFO    MOVEABLE IMPURE LOADONCALL DISCARDABLE
#endif
  FILEVERSION     ${RC_VERSION_COMMA}
  PRODUCTVERSION  ${RC_VERSION_COMMA}
  FILEFLAGSMASK   VS_FFI_FILEFLAGSMASK
#ifdef _DEBUG
  FILEFLAGS       VS_FF_DEBUG | VS_FF_PATCHED | VS_FF_SPECIALBUILD
#else
  FILEFLAGS       0 | VS_FF_PATCHED | VS_FF_SPECIALBUILD
#endif
  FILEOS          VOS_NT_WINDOWS32
  FILETYPE        VFT_DLL
  FILESUBTYPE     0    // not used
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904E4"
    //language ID = U.S. English, char set = Windows, Multilingual
    BEGIN
      VALUE "CompanyName",      "Industrial Light & Magic"
      VALUE "FileDescription",  "@RC_DESCRIPTION@\0"
      VALUE "FileVersion",      "@RC_VERSION@\0"
      VALUE "InternalName",     FILENAME_STRING
      VALUE "LegalCopyright",   "Copyright (c) @RC_YEAR@, Industrial Light & Magic, a division of Lucas Digital Ltd. LLC\0"
      VALUE "OriginalFilename", FILENAME_STRING
      VALUE "ProductName",      "@RC_PRODUCT_NAME@\0"
      VALUE "ProductVersion",   "@RC_VERSION@\0"
#ifdef MSVC_VER_STRING
      VALUE "SpecialBuild",     "Created with CMake using Microsoft Visual C++ " MSVC_VER_STRING "\0"
#else
      VALUE "SpecialBuild",     "Created with CMake\0"
#endif
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x0409, 1252
  END
END
