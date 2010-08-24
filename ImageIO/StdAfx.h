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

typedef float ALIGN16_BEG afloat_t ALIGN16_END;

#if (!defined(__STDC_VERSION__)|| __STDC_VERSION__<199901) && !defined(__INTEL_COMPILER)
# if defined(_MSC_VER)
#  define PCG_RESTRICT __restrict
# elif defined(__GNUC__)
#  define PCG_RESTRICT __restrict__
# else
#  define PCG_RESTRICT
# endif
#else
# define PCG_RESTRICT restrict
#endif



// Taken from the Intrinsics Guide for AVX. The same technique was seen as
// a contribution to gcc by Apple (I think). The original macro in MSVC++ uses
// shuffle instead
#define PCG_MM_TRANSPOSE4_PS(row0, row1, row2, row3) {  \
            __m128 tmp3, tmp2, tmp1, tmp0;                \
                                                          \
            tmp0   = _mm_unpacklo_ps((row0), (row1));     \
            tmp2   = _mm_unpacklo_ps((row2), (row3));     \
            tmp1   = _mm_unpackhi_ps((row0), (row1));     \
            tmp3   = _mm_unpackhi_ps((row2), (row3));     \
                                                          \
            (row0) = _mm_movelh_ps(tmp0, tmp2);           \
            (row1) = _mm_movehl_ps(tmp2, tmp0);           \
            (row2) = _mm_movelh_ps(tmp1, tmp3);           \
            (row3) = _mm_movehl_ps(tmp3, tmp1);           \
        }


#endif /* PCG_STDAFX_H */
