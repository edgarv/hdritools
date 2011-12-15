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

#if defined(_MSC_VER)
typedef __int32 int32_t;
#else
#include <stdint.h>
#endif


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
_PS_EXTERN_CONST_TYPE(am_min_norm_pos, int32_t, 0x00800000)
_PS_EXTERN_CONST_TYPE(am_inv_mant_mask, int32_t, ~0x7f800000)

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
