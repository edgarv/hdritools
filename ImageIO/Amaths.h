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
//  Header File
//  Version 2.0
//  Author Alex Klimovitski, Intel GmbH
/////////////////////////////////////////////////////////////////////////////

#pragma once
#if !defined(_AMATHS_H_)
#define _AMATHS_H_

#include <emmintrin.h>
#include "Vec4f.h"
#include "Vec4i.h"
#if PCG_USE_AVX
# include <immintrin.h>
# include "Vec8f.h"
# include "Vec8i.h"
#endif

namespace am
{
__m128 log_eps(__m128 x);
__m128 pow_eps(__m128 x, __m128 y);

#if PCG_USE_AVX
__m256 log_avx(__m256 x);
__m256 pow_avx(__m256 x, __m256 y);
#endif
}

// Actual implementation
#include "Amaths.inl"

#endif // _AMATHS_H_
