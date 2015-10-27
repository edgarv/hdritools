/* config/IlmBaseConfig.h.  Generated from IlmBaseConfig.h.cmake by cmake. */
#ifndef INCLUDED_ILM_BASE_CONFIG_H
#define INCLUDED_ILM_BASE_CONFIG_H

//
// Define and set to 1 if the target system has POSIX thread support
// and you want IlmBase to use it for multithreaded file I/O.
//

#cmakedefine HAVE_PTHREAD 1

//
// Define and set to 1 if the target system supports POSIX semaphores
// and you want OpenEXR to use them; otherwise, OpenEXR will use its
// own semaphore implementation.
//

#cmakedefine HAVE_POSIX_SEMAPHORES 1


#cmakedefine HAVE_UCONTEXT_H 1


//
// Dealing with FPEs
//
#cmakedefine ILMBASE_HAVE_CONTROL_REGISTER_SUPPORT 1


//
// Define and set to 1 if the target system has support for large
// stack sizes.
//

#cmakedefine ILMBASE_HAVE_LARGE_STACK 1


//
// Define and set to 1 to use the synchronization functions
// introduced with Windows Vista.
//

#cmakedefine ILMBASE_USE_WINNT_VISTA_SYNC 1


//
// Current (internal) library namepace name and corresponding public
// client namespaces.
//
#cmakedefine ILMBASE_INTERNAL_NAMESPACE_CUSTOM 1
#cmakedefine IMATH_INTERNAL_NAMESPACE @IMATH_INTERNAL_NAMESPACE@
#cmakedefine IEX_INTERNAL_NAMESPACE @IEX_INTERNAL_NAMESPACE@
#cmakedefine ILMTHREAD_INTERNAL_NAMESPACE @ILMTHREAD_INTERNAL_NAMESPACE@

#cmakedefine ILMBASE_NAMESPACE_CUSTOM 1
#cmakedefine IMATH_NAMESPACE @IMATH_NAMESPACE@
#cmakedefine IEX_NAMESPACE @IEX_NAMESPACE@
#cmakedefine ILMTHREAD_NAMESPACE @ILMTHREAD_NAMESPACE@


//
// Version string for runtime access
//
#define ILMBASE_VERSION_STRING @ILMBASE_VERSION_STRING@
#define ILMBASE_PACKAGE_STRING @ILMBASE_PACKAGE_STRING@

#define ILMBASE_VERSION_MAJOR @ILMBASE_VERSION_MAJOR@
#define ILMBASE_VERSION_MINOR @ILMBASE_VERSION_MINOR@
#define ILMBASE_VERSION_PATCH @ILMBASE_VERSION_PATCH@

// Version as a single hex number, e.g. 0x01000300 == 1.0.3
#define ILMBASE_VERSION_HEX ((ILMBASE_VERSION_MAJOR << 24) | \
                             (ILMBASE_VERSION_MINOR << 16) | \
                             (ILMBASE_VERSION_PATCH <<  8))

#endif // INCLUDED_ILM_BASE_CONFIG_H
