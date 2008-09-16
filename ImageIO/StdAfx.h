/*
 * Commonly used includes, which will go to the precompiled header
 */

#if !defined (PCG_STDAFX_H)
#define PCG_STDAFX_H

#include <xmmintrin.h> /* Streaming SIMD Extensions Intrinsics include file */
#include <emmintrin.h> /* SSE2 Include file */

/* 16 byte alignment for SSE */
#if defined(_MSC_VER) || defined(__ICC)
#define ALIGN16 _MM_ALIGN16
#else
#define ALIGN16 __attribute__((aligned(16)))
#endif


#include <ostream>
#include <cassert>


#endif /* PCG_STDAFX_H */
