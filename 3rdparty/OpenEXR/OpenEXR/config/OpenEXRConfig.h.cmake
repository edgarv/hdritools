/* config/OpenEXRConfig.h.  Generated from OpenEXRConfig.h.cmake by CMake. */
#ifndef INCLUDED_OPENEXR_CONFIG_H
#define INCLUDED_OPENEXR_CONFIG_H

//
// Define and set to 1 if the target system supports a proc filesystem
// compatible with the Linux kernel's proc filesystem.  Note that this
// is only used by a program in the IlmImfTest test suite, it's not
// used by any OpenEXR library or application code.
//

#cmakedefine OPENEXR_IMF_HAVE_LINUX_PROCFS 1

//
// Define and set to 1 if the target system is a Darwin-based system
// (e.g., OS X).
//

#cmakedefine OPENEXR_IMF_HAVE_DARWIN 1

//
// Define and set to 1 if the target system has a complete <iomanip>
// implementation, specifically if it supports the std::right
// formatter.
//

#cmakedefine OPENEXR_IMF_HAVE_COMPLETE_IOMANIP 1

//
// Define and set to 1 if the target system has support for large
// stack sizes.
//

#cmakedefine OPENEXR_IMF_HAVE_LARGE_STACK 1

//
// Define and set to 1 to use the synchronization functions
// introduced with Windows Vista.
//

#cmakedefine OPENEXR_IMF_USE_WINNT_VISTA_SYNC 1

//
// Current internal library namepace name
//
#cmakedefine OPENEXR_IMF_INTERNAL_NAMESPACE_CUSTOM @OPENEXR_IMF_INTERNAL_NAMESPACE_CUSTOM@
#cmakedefine OPENEXR_IMF_INTERNAL_NAMESPACE @OPENEXR_IMF_INTERNAL_NAMESPACE@

//
// Current public user namepace name
//

#cmakedefine OPENEXR_IMF_NAMESPACE_CUSTOM @OPENEXR_IMF_NAMESPACE_CUSTOM@
#cmakedefine OPENEXR_IMF_NAMESPACE @OPENEXR_IMF_NAMESPACE@

//
// Version string for runtime access
//

#define OPENEXR_VERSION_STRING @OPENEXR_VERSION_STRING@
#define OPENEXR_PACKAGE_STRING @OPENEXR_PACKAGE_STRING@

#define OPENEXR_VERSION_MAJOR @OPENEXR_VERSION_MAJOR@
#define OPENEXR_VERSION_MINOR @OPENEXR_VERSION_MINOR@
#define OPENEXR_VERSION_PATCH @OPENEXR_VERSION_PATCH@

// Version as a single hex number, e.g. 0x01000300 == 1.0.3
#define OPENEXR_VERSION_HEX ((OPENEXR_VERSION_MAJOR << 24) | \
                             (OPENEXR_VERSION_MINOR << 16) | \
                             (OPENEXR_VERSION_PATCH <<  8))

#endif // INCLUDED_OPENEXR_CONFIG_H
