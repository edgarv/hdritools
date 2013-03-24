//
// Define and set to 1 if the target system supports a proc filesystem
// compatible with the Linux kernel's proc filesystem.  Note that this
// is only used by a program in the IlmImfTest test suite, it's not
// used by any OpenEXR library or application code.
//

#cmakedefine HAVE_LINUX_PROCFS 1

//
// Define and set to 1 if the target system is a Darwin-based system
// (e.g., OS X).
//

#cmakedefine HAVE_DARWIN 1

//
// Define and set to 1 if the target system has a complete <iomanip>
// implementation, specifically if it supports the std::right
// formatter.
//

#cmakedefine HAVE_COMPLETE_IOMANIP 1

//
// Define and set to 1 if the target system has support for large
// stack sizes.
//

#cmakedefine HAVE_LARGE_STACK 1

//
// Version string for runtime access
//
#cmakedefine OPENEXR_VERSION_STRING @OPENEXR_VERSION_STRING@
#cmakedefine OPENEXR_PACKAGE_STRING @OPENEXR_PACKAGE_STRING@
