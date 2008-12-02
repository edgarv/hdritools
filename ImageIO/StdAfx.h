/*
 * Commonly used includes, which will go to the precompiled header
 */

#if !defined (PCG_STDAFX_H)
#define PCG_STDAFX_H

#include <xmmintrin.h> /* Streaming SIMD Extensions Intrinsics include file */
#include <emmintrin.h> /* SSE2 Include file */

/* 16 byte alignment for SSE */
#if defined(_MSC_VER) || defined(__ICC)
# define ALIGN16_BEG _MM_ALIGN16
# define ALIGN16_END 
#else
# define ALIGN16_BEG
# define ALIGN16_END __attribute__((aligned(16)))
#endif

/* Forcing inlining */
#if defined(_MSC_VER) || defined(__ICC)
# define FORCEINLINE_BEG __forceinline
# define FORCEINLINE_END 
#else
# define FORCEINLINE_BEG
# define FORCEINLINE_END __attribute__((always_inline))
#endif

#include <ostream>
#include <cassert>


#endif /* PCG_STDAFX_H */
