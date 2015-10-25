#ifndef HALFEXPORT_H
#define HALFEXPORT_H

//
//  Copyright (c) 2008 Lucasfilm Entertainment Company Ltd.
//  All rights reserved.   Used under authorization.
//  This material contains the confidential and proprietary
//  information of Lucasfilm Entertainment Company and
//  may not be copied in whole or in part without the express
//  written permission of Lucasfilm Entertainment Company.
//  This copyright notice does not imply publication.
//

#if defined(_WIN32)
#  if defined(OPENEXR_DLL)
#    define HALF_EXPORT_DEFINITION __declspec(dllexport) 
#    define HALF_IMPORT_DEFINITION __declspec(dllimport)
#  else
#    define HALF_EXPORT_DEFINITION 
#    define HALF_IMPORT_DEFINITION
#  endif
#else   // linux/macos
#  if defined(PLATFORM_VISIBILITY_AVAILABLE)
#    define HALF_EXPORT_DEFINITION __attribute__((visibility("default")))
#    define HALF_IMPORT_DEFINITION
#  else
#    define HALF_EXPORT_DEFINITION 
#    define HALF_IMPORT_DEFINITION
#  endif
#endif

#if defined(HALF_EXPORTS)                          // create library
#  define HALF_EXPORT HALF_EXPORT_DEFINITION
#else                                              // use library
#  define HALF_EXPORT HALF_IMPORT_DEFINITION
#endif

#endif // #ifndef HALFEXPORT_H

