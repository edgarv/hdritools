/////////////////////////////////////////////////////////////////////////////
// The Software is provided "AS IS" and possibly with with faults. Intel
// disclaims any and all warranties and guarantees, express, implied or
// otherwise, arising, with respect to the software delivered hereunder,
// including but not limited to the warranty of merchantability, the warranty
// of fitness for a particular purpose, and any warranty of non-infringement
// of the intellectual property rights of any third party.
// Intel neither assumes nor authorizes any person to assume for it any other
// liability.  Customer will use the software at its own risk.  Intel will not
// be liable to customer for any direct or indirect damages incurred in using
// the software.  In no event will Intel be liable for loss of profits, loss of
// use, loss of data, business interruption, nor for punitive, incidental,
// consequential, or special damages of any kind, even if advised of
// the possibility of such damages.
// 
// Copyright (c) 1998 - 2000 Intel Corporation
//
// Third-party brands and names are the property of their respective owners
//
/////////////////////////////////////////////////////////////////////////////
// Approximate Math Library for SSE / SSE2
//  Implementation File
//  Version 2.0
//  Author Alex Klimovitski, Intel GmbH
/////////////////////////////////////////////////////////////////////////////

#include "Amaths.h"
#include "StdAfx.h"

#include <emmintrin.h>

#if PCG_USE_AVX
# include "Vec8f.h"
# include "Vec8i.h"
#endif
# include "Vec4f.h"
# include "Vec4i.h"


// Intel AM constants
#define _PS_CONST(Name, Val) \
static const ALIGN16_BEG float _ps_##Name[4] ALIGN16_END = { Val, Val, Val, Val }

#define _PS_EXTERN_CONST(Name, Val) \
const ALIGN16_BEG float _ps_##Name[4] ALIGN16_END = { Val, Val, Val, Val }

#define _PS_EXTERN_CONST_TYPE(Name, Type, Val) \
const ALIGN16_BEG Type _ps_##Name[4] ALIGN16_END = { Val, Val, Val, Val }; \

#define _EPI32_CONST(Name, Val) \
static const ALIGN16_BEG int32_t _epi32_##Name[4] ALIGN16_END = { Val, Val, Val, Val }

_PS_EXTERN_CONST(am_1, 1.0f);
_PS_EXTERN_CONST(am_0p5, 0.5f);
_PS_EXTERN_CONST_TYPE(am_min_norm_pos, int32_t, 0x00800000)
_PS_EXTERN_CONST_TYPE(am_inv_mant_mask, int32_t, ~0x7f800000)

_EPI32_CONST(1, 1);
_EPI32_CONST(0x7f, 0x7f);


/////////////////////////////////////////////////////////////////////////////
// Helper functions
namespace
{

inline __m128 sse_load(const float (&arr)[4]) {
    return _mm_load_ps (&arr[0]);
}

inline __m128 sse_load(const int (&arr)[4]) {
    return _mm_load_ps (reinterpret_cast<const float*>(&arr[0]));
}

inline __m128i sse_load_epi32(const int (&arr)[4]) {
    return _mm_load_si128 (reinterpret_cast<const __m128i*>(&arr[0]));
}

} // namespace


/////////////////////////////////////////////////////////////////////////////
// log functions

_PS_CONST(log_p0, -7.89580278884799154124e-1f);
_PS_CONST(log_p1, 1.63866645699558079767e1f);
_PS_CONST(log_p2, -6.41409952958715622951e1f);

_PS_CONST(log_q0, -3.56722798256324312549e1f);
_PS_CONST(log_q1, 3.12093766372244180303e2f);
_PS_CONST(log_q2, -7.69691943550460008604e2f);

_PS_CONST(log_c0, 0.693147180559945f);

_PS_CONST(log2_c0, 1.44269504088896340735992f);

/////////////////////////////////////////////////////////////////////////////
// exp2 functions

_PS_CONST(exp2_hi,  127.4999961853f);
_PS_CONST(exp2_lo, -127.4999961853f);

_PS_CONST(exp2_p0, 2.30933477057345225087e-2f);
_PS_CONST(exp2_p1, 2.02020656693165307700e1f);
_PS_CONST(exp2_p2, 1.51390680115615096133e3f);

_PS_CONST(exp2_q0, 2.33184211722314911771e2f);
_PS_CONST(exp2_q1, 4.36821166879210612817e3f);

/////////////////////////////////////////////////////////////////////////////
// am_log_eps

__m128 am::log_eps(__m128 x)
{
    // Constants
    const __m128 am_1          = sse_load(_ps_am_1);
    const __m128 min_norm_pos  = sse_load(_ps_am_min_norm_pos);
    const __m128 inv_mant_mask = sse_load(_ps_am_inv_mant_mask);
    const __m128i epi32_0x7f   = sse_load_epi32(_epi32_0x7f);

    const __m128 log_p0 = sse_load(_ps_log_p0);
    const __m128 log_p1 = sse_load(_ps_log_p1);
    const __m128 log_p2 = sse_load(_ps_log_p2);

    const __m128 log_q0 = sse_load(_ps_log_q0);
    const __m128 log_q1 = sse_load(_ps_log_q1);
    const __m128 log_q2 = sse_load(_ps_log_q2);

    const __m128 log_c0 = sse_load(_ps_log_c0);


    // Use variables named like the registers to keep the code close
    __m128 xmm0, xmm1, xmm2, xmm4, xmm5, xmm6, xmm7;
    __m128i xmm3i;

    xmm0 = _mm_max_ps(x, min_norm_pos); // cut off denormalized stuff
    xmm1 = am_1;
    xmm3i= _mm_castps_si128(xmm0);

    xmm0 = _mm_and_ps(xmm0, inv_mant_mask);
    xmm0 = _mm_or_ps (xmm0, xmm1);

    xmm4 = xmm0;
    xmm0 = _mm_sub_ps(xmm0, xmm1);
    xmm4 = _mm_add_ps(xmm4, xmm1);

    xmm3i= _mm_srli_epi32(xmm3i, 23);
    xmm4 = _mm_rcp_ps(xmm4);
    xmm0 = _mm_mul_ps(xmm0, xmm4);
    xmm3i= _mm_sub_epi32(xmm3i, epi32_0x7f);
    xmm0 = _mm_add_ps(xmm0, xmm0);

    xmm2 = xmm0;
    xmm0 = _mm_mul_ps(xmm0, xmm0);

    xmm4 = log_p0;
    xmm6 = log_q0;

    xmm4 = _mm_mul_ps(xmm4, xmm0);
    xmm5 = log_p1;
    xmm6 = _mm_mul_ps(xmm6, xmm0);
    xmm7 = log_q1;

    xmm4 = _mm_add_ps(xmm4, xmm5);
    xmm6 = _mm_add_ps(xmm6, xmm7);

    xmm5 = log_p2;
    xmm4 = _mm_mul_ps(xmm4, xmm0);
    xmm7 = log_q2;
    xmm6 = _mm_mul_ps(xmm6, xmm0);

    xmm4 = _mm_add_ps(xmm4, xmm5);
    xmm5 = log_c0;
    xmm6 = _mm_add_ps(xmm6, xmm7);
    xmm1 = _mm_cvtepi32_ps(xmm3i);

    xmm0 = _mm_mul_ps(xmm0, xmm4);
    xmm6 = _mm_rcp_ps(xmm6);

    xmm0 = _mm_mul_ps(xmm0, xmm6);
    xmm0 = _mm_mul_ps(xmm0, xmm2);

    xmm1 = _mm_mul_ps(xmm1, xmm5);

    xmm0 = _mm_add_ps(xmm0, xmm2);
    xmm0 = _mm_add_ps(xmm0, xmm1);

    return xmm0;
}



/////////////////////////////////////////////////////////////////////////////
// am_pow_eps

__m128 am::pow_eps(__m128 x, __m128 y)
{
    // Constants
    const __m128 am_1          = sse_load(_ps_am_1);
    const __m128 am_0p5        = sse_load(_ps_am_0p5);
    const __m128 min_norm_pos  = sse_load(_ps_am_min_norm_pos);
    const __m128 inv_mant_mask = sse_load(_ps_am_inv_mant_mask);
    const __m128i epi32_1      = sse_load_epi32(_epi32_1);
    const __m128i epi32_0x7f   = sse_load_epi32(_epi32_0x7f);

    const __m128 log_p0 = sse_load(_ps_log_p0);
    const __m128 log_p1 = sse_load(_ps_log_p1);
    const __m128 log_p2 = sse_load(_ps_log_p2);

    const __m128 log_q0 = sse_load(_ps_log_q0);
    const __m128 log_q1 = sse_load(_ps_log_q1);
    const __m128 log_q2 = sse_load(_ps_log_q2);

    const __m128 log2_c0 = sse_load(_ps_log2_c0);

    const __m128 exp2_hi = sse_load(_ps_exp2_hi);
    const __m128 exp2_lo = sse_load(_ps_exp2_lo);

    const __m128 exp2_p0 = sse_load(_ps_exp2_p0);
    const __m128 exp2_p1 = sse_load(_ps_exp2_p1);
    const __m128 exp2_p2 = sse_load(_ps_exp2_p2);

    const __m128 exp2_q0 = sse_load(_ps_exp2_q0);
    const __m128 exp2_q1 = sse_load(_ps_exp2_q1);

    // Use variables named like the registers to keep the code close
    __m128 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, ecx_16;
    __m128i xmm2i, xmm3i;

    xmm0 = x;
    xmm1 = y;

    xmm5  = _mm_setzero_ps();
    xmm5  = _mm_cmplt_ps(xmm5, xmm0);
    xmm0  = _mm_max_ps(xmm0, min_norm_pos); // cut off denormalized stuff
    xmm7  = am_1;
    xmm3i = _mm_castps_si128(xmm0);

    xmm0 = _mm_and_ps(xmm0, inv_mant_mask);
    xmm0 = _mm_or_ps(xmm0, xmm7);

    ecx_16 = xmm5;

    xmm4  = xmm0;
    xmm0  = _mm_sub_ps(xmm0, xmm7);
    xmm4  = _mm_add_ps(xmm4, xmm7);
    xmm3i = _mm_srli_epi32(xmm3i, 23);
    xmm4  = _mm_rcp_ps(xmm4);
    xmm0  = _mm_mul_ps(xmm0, xmm4);
    xmm3i = _mm_sub_epi32(xmm3i, epi32_0x7f);
    xmm0  = _mm_add_ps(xmm0, xmm0);

    xmm2 = xmm0;
    xmm0 = _mm_mul_ps(xmm0, xmm0);

    xmm4 = log_p0;
    xmm6 = log_q0;

    xmm4 = _mm_mul_ps(xmm4, xmm0);
    xmm5 = log_p1;
    xmm6 = _mm_mul_ps(xmm6, xmm0);
    xmm7 = log_q1;

    xmm4 = _mm_add_ps(xmm4, xmm5);
    xmm6 = _mm_add_ps(xmm6, xmm7);

    xmm5 = log_p2;
    xmm4 = _mm_mul_ps(xmm4, xmm0);
    xmm7 = log_q2;
    xmm6 = _mm_mul_ps(xmm6, xmm0);

    xmm4 = _mm_add_ps(xmm4, xmm5);
    xmm5 = log2_c0;
    xmm6 = _mm_add_ps(xmm6, xmm7);
    xmm7 = _mm_cvtepi32_ps(xmm3i);

    xmm0 = _mm_mul_ps(xmm0, xmm4);
    xmm6 = _mm_rcp_ps(xmm6);

    xmm0 = _mm_mul_ps(xmm0, xmm6);
    xmm4 = exp2_hi;
    xmm0 = _mm_mul_ps(xmm0, xmm2);
    xmm6 = exp2_lo;
    xmm2 = _mm_mul_ps(xmm2, xmm5);
    xmm0 = _mm_mul_ps(xmm0, xmm5);
    xmm2 = _mm_add_ps(xmm2, xmm7);
    xmm3 = am_0p5;
    xmm0 = _mm_add_ps(xmm0, xmm2);
    xmm2 = _mm_setzero_ps();

    xmm0 = _mm_mul_ps(xmm0, xmm1);

    xmm0 = _mm_min_ps(xmm0, xmm4);
    xmm4 = exp2_p0;
    xmm0 = _mm_max_ps(xmm0, xmm6);
    xmm6 = exp2_q0;

    xmm3 = _mm_add_ps(xmm3, xmm0);

    xmm2i = _mm_castps_si128(_mm_cmpnlt_ps(xmm2, xmm3));
    xmm2i = _mm_and_si128(xmm2i, epi32_1);

    xmm3i = _mm_cvttps_epi32(xmm3);

    xmm3i = _mm_sub_epi32(xmm3i, xmm2i);
    xmm5  = exp2_p1;

    xmm2 = _mm_cvtepi32_ps(xmm3i);
    xmm7 = exp2_q1;

    xmm0 = _mm_sub_ps(xmm0, xmm2);

    xmm2 = xmm0;
    xmm0 = _mm_mul_ps(xmm0, xmm0);

    xmm3i = _mm_add_epi32(xmm3i, epi32_0x7f);

    xmm4 = _mm_mul_ps(xmm4, xmm0);
    xmm6 = _mm_mul_ps(xmm6, xmm0);
    xmm4 = _mm_add_ps(xmm4, xmm5);
    xmm6 = _mm_add_ps(xmm6, xmm7);

    xmm4  = _mm_mul_ps(xmm4, xmm0);
    xmm5  = ecx_16;
    xmm3i = _mm_slli_epi32(xmm3i, 23);
    xmm4  = _mm_add_ps(xmm4, exp2_p2);

    xmm2 = _mm_mul_ps(xmm2, xmm4);

    xmm0 = am_1;
    xmm6 = _mm_sub_ps(xmm6, xmm2);
    xmm3 = _mm_and_ps(_mm_castsi128_ps(xmm3i), xmm5);
    xmm6 = _mm_rcp_ps(xmm6);
    xmm2 = _mm_mul_ps(xmm2, xmm6);
    xmm2 = _mm_add_ps(xmm2, xmm2);
    xmm0 = _mm_add_ps(xmm0, xmm2);

    xmm0 = _mm_mul_ps(xmm0, xmm3);
    
    return xmm0;
}


#if PCG_USE_AVX

/////////////////////////////////////////////////////////////////////////////
// am_log_eps [AVX Version]
// Based on a straightforward reinterpretation of the original code
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constants
namespace
{
namespace avx
{
typedef pcg::Vec8i v8i;
typedef pcg::Vec8f v8f;


const v8f inv_mantissa_mask(_mm256_castsi256_ps(v8i::constant<~0x7f800000>()));
const v8f min_normal(_mm256_castsi256_ps(v8i::constant<0x00800000>()));

const v8f const_1(1.0f);
const v8f const_127(127.0f);
const v8f const_0p5(0.5f);

const v8f log_p0( -7.89580278884799154124e-1f);
const v8f log_p1(  1.63866645699558079767e1f);
const v8f log_p2( -6.41409952958715622951e1f);

const v8f log_q0( -3.56722798256324312549e1f);
const v8f log_q1(  3.12093766372244180303e2f);
const v8f log_q2( -7.69691943550460008604e2f);

const v8f log_c0(  0.693147180559945f);
const v8f log2_c0(1.44269504088896340735992f);

const v8f exp2_hi( 127.4999961853f);
const v8f exp2_lo(-127.4999961853f);

const v8f exp2_p0(2.30933477057345225087e-2f);
const v8f exp2_p1(2.02020656693165307700e1f);
const v8f exp2_p2(1.51390680115615096133e3f);

const v8f exp2_q0(2.33184211722314911771e2f);
const v8f exp2_q1(4.36821166879210612817e3f);



inline v8f toFloat(const v8i& x) {
    return _mm256_cvtepi32_ps(x);
}

inline v8i toInt(const v8f& x) {
    return _mm256_cvttps_epi32(x);
}

inline v8f roundTruncate(const v8f& x) {
    return _mm256_round_ps(x, 0x0B);
}

inline v8i castAsInt(const v8f& x) {
    return _mm256_castps_si256(x);
}

inline v8f castAsFloat(const v8i& x) {
    return _mm256_castsi256_ps(x);
}

// Shift right by "count" bits while shifting in zeros. Full efficiency
// requires AVX2, but for now this provides completeness
inline v8i srl(const __m256i& a, const int& count)
{
    __m128i a0 = _mm256_castsi256_si128(a);
    __m128i a1 = _mm256_extractf128_si256(a, 1);

    a0 = _mm_srli_epi32(a0, count);
    a1 = _mm_srli_epi32(a1, count);

    __m256i r = _mm256_insertf128_si256(_mm256_castsi128_si256(a0), a1, 1);
    return r;
}

// Shift left by "count" bits while shifting in zeros. Full efficiency
// requires AVX2, but for now this provides completeness
inline v8i sll(const v8i& a, const int& count)
{
    __m128i a0 = _mm256_castsi256_si128(a);
    __m128i a1 = _mm256_extractf128_si256(a, 1);

    a0 = _mm_slli_epi32(a0, count);
    a1 = _mm_slli_epi32(a1, count);

    __m256i r = _mm256_insertf128_si256(_mm256_castsi128_si256(a0), a1, 1);
    return r;
}

} // namespace avx
} // namespace



__m256 am::log_avx(__m256 x)
{
    typedef pcg::Vec8i v8i;
    typedef pcg::Vec8f v8f;

    // Kill invalid values (ignoring NaN and Inf)
    const v8f x0 = simd_max(x, avx::min_normal);

    // Kill the exponent and combine with the exponent of 1.0f to get the
    // actual embedded mantissa as a valid floating point value:
    // a value in the range [1.0, 2.0)
    const v8f mantissa = (x0 & avx::inv_mantissa_mask) | avx::const_1;

    const v8f v_min1  = mantissa - avx::const_1;
    const v8f v_plus1 = mantissa + avx::const_1;

    // Extract the original exponent and undo the bias
    // The original formulation uses all integer operations for this, but
    // since those aren't supported on AVX, we can safely do the substraction
    // using floating point: single precision can represent all possible values
    const v8f biasedExponent =
        _mm256_cvtepi32_ps(avx::srl(_mm256_castps_si256(x0), 23));
    const v8f origExponent = biasedExponent - avx::const_127;
    
    v8f vFrac = v_min1 * simd_rcp(v_plus1); // Is it worth it to use rcp_nr?
    vFrac += vFrac;
    const v8f vFracSqr = vFrac * vFrac;

    // Evaluate the polynomial
    const v8f polyP = ((((avx::log_p0 * vFracSqr) + avx::log_p1) *
                                        vFracSqr) + avx::log_p2) * vFracSqr;
    const v8f polyQ =  (((avx::log_q0 * vFracSqr) + avx::log_q1) * 
                                        vFracSqr) + avx::log_q2;

    const v8f poly = polyP * simd_rcp(polyQ); // Use rcp_nr?
    const v8f logApprox = poly * vFrac;

    // Scale by log(2) to get the natural logarithm of the exponent part
    const v8f logExpPart = origExponent * avx::log_c0;

    // Combine the different parts
    const v8f result = logApprox + vFrac + logExpPart;

    return result;
}



/////////////////////////////////////////////////////////////////////////////
// am_pow_eps [AVX Version]
// Based on a straightforward reinterpretation of the original code
/////////////////////////////////////////////////////////////////////////////
__m256 am::pow_avx(__m256 x, __m256 y)
{
    typedef pcg::Vec8i  v8i;
    typedef pcg::Vec8f  v8f;
    typedef pcg::Vec8bf v8bf;

    // Remember negative values
    const v8f negative_mask(v8f::zero() < x);

    // Cutoff denormalized stuff (preserving NaN and Infinity)
    const v8f x0 = simd_max(x, avx::min_normal);

    // First step: compute log(x)

    // Kill the exponent and combine with the exponent of 1.0f to get the
    // actual embedded mantissa as a valid floating point value:
    // a value in the range [1.0, 2.0)
    const v8f mantissa = (x0 & avx::inv_mantissa_mask) | avx::const_1;

    const v8f v_min1  = mantissa - avx::const_1;
    const v8f v_plus1 = mantissa + avx::const_1;

    // Extract the original exponent and undo the bias
    // The original formulation uses all integer operations for this, but
    // since those aren't supported on AVX, we can safely do the substraction
    // using floating point: single precision can represent all possible values
    const v8f biasedExponent = avx::toFloat(avx::srl(avx::castAsInt(x0), 23));
    const v8f origExponent = biasedExponent - avx::const_127;

    v8f vFrac = v_min1 * simd_rcp(v_plus1); // Is it worth it to use rcp_nr?
    vFrac += vFrac;
    const v8f vFracSqr = vFrac * vFrac;

    // Evaluate the polynomial
    const v8f polyP = ((((avx::log_p0 * vFracSqr) + avx::log_p1) *
                                        vFracSqr) + avx::log_p2) * vFracSqr;
    const v8f polyQ =  (((avx::log_q0 * vFracSqr) + avx::log_q1) *
                                        vFracSqr) + avx::log_q2;
    const v8f logApprox = (polyP * simd_rcp(polyQ)) * vFrac;
    const v8f log2Val =
        (logApprox * avx::log2_c0) + ((vFrac * avx::log2_c0) + origExponent);

    // y * log2(x)
    v8f exponent = y * log2Val;

    // Clamp the exponent
    exponent = simd_max(simd_min(exponent, avx::exp2_hi), avx::exp2_lo);

    // More floating point tricks: normalize the mantissa to [1.0 - 1.5]
    const v8f normExponent = exponent + avx::const_0p5;

    // Build the biased exponent. The original formulation uses integer
    // arithmetic, but since that is not available in AVX use floating point
    // as it can handle all valid exponents: (-127.5 127.5)
    const v8bf expNegExponentMask = cmpnlt(v8f::zero(), normExponent);
    const v8f expNormalization = v8f(expNegExponentMask) & avx::const_1;
    const v8f truncExp = avx::roundTruncate(normExponent);
    const v8f resExp = truncExp - expNormalization;
    v8i biasedExp = avx::toInt(resExp + avx::const_127);
    biasedExp = avx::sll(biasedExp, 23);
    const v8f exponentPart = avx::castAsFloat(biasedExp) & negative_mask;

    // Get the fractional part of the exponent
    exponent -= resExp;
    const v8f exponentSqr = exponent * exponent;

    // Exp polynomial
    const v8f EPolyP = ((((avx::exp2_p0 * exponentSqr) + avx::exp2_p1) *
                                          exponentSqr) + avx::exp2_p2)*exponent;
    const v8f EPolyQ =   ((avx::exp2_q0 * exponentSqr) + avx::exp2_q1) - EPolyP;
    v8f expApprox = EPolyP * simd_rcp(EPolyQ);
    expApprox += expApprox;
    expApprox += avx::const_1;

    v8f result = expApprox * exponentPart;
    return result;
}

#endif
