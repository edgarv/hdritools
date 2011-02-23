/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#ifndef IMAGEIO_H
#define IMAGEIO_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the IMAGEIO_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// IMAGEIO_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
  #ifdef IMAGEIO_EXPORTS
  #define IMAGEIO_API __declspec(dllexport)
  #else
  #define IMAGEIO_API __declspec(dllimport)
  #endif
#else
  #define IMAGEIO_API
#endif /* WIN32 */


// For proper memory alignment
#if defined(_MSC_VER)
#define PCG_USE_MEMALIGN 0
#include <malloc.h>
#elif _XOPEN_SOURCE >= 600 || defined(__APPLE__)
#define PCG_USE_MEMALIGN 1
#include <cstdlib>
#else
#define PCG_USE_MEMALIGN 0
#include <cstdlib>
#endif

namespace pcg {

template <typename T>
inline T* alloc_align (size_t alignment, size_t size = sizeof(T))
{
    T* ptr = 0;
#if PCG_USE_MEMALIGN
    if (posix_memalign ((void**)&ptr, alignment, size) != 0) {
		ptr = 0;
    }
#elif defined(_MSC_VER)
    ptr = (T*) _aligned_malloc (size, alignment);
#else
    // warning: not actually aligned!
    ptr = (T*) malloc (size);
#endif
    return ptr;
}

template <typename T>
inline void free_align (T *ptr)
{
#if PCG_USE_MEMALIGN || !defined(_MSC_VER)
    free (ptr);
#else
    _aligned_free (ptr);
#endif
}



} // namespace pcg


#endif
