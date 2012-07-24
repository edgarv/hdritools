/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2012 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#pragma once
#if !defined(PCG_VEC8I_H)
#define PCG_VEC8I_H

#include "StdAfx.h"

#include <cassert>

namespace pcg
{

struct ALIGN32_BEG Vec8i
{
private:
    union {
        __m256i ymm;
        int32_t i32[4];
    };

public:
    // Trivial constructor
    Vec8i() {}

    // Initialize from a raw __m128i value
    Vec8i(__m256i val) : ymm(val) {}

    // Initialize with the same value in all components
    explicit Vec8i(int32_t val) : ymm(_mm256_set1_epi32(val)) {}

    // Initialize with explicit values. The indices reflect the memory layout
    Vec8i(int32_t i7, int32_t i6, int32_t i5, int32_t i4,
          int32_t i3, int32_t i2, int32_t i1, int32_t i0) :
    ymm(_mm256_set_epi32(i7, i6, i5, i4, i3, i2, i1, i0))
    {}

    // Assignment
    Vec8i& operator= (int32_t val) {
        ymm = _mm256_set1_epi32(val);
        return *this;
    }

    // Cast operations
    operator __m256i() const {
        return ymm;
    }

#if PCG_USE_AVX2
    // Logical operators [binary]
    friend Vec8i operator& (const Vec8i& a, const Vec8i& b) {
        return _mm256_and_si256(a, b);
    }
    friend Vec8i operator| (const Vec8i& a, const Vec8i& b) {
        return _mm256_or_si256(a, b);
    }
    friend Vec8i operator^ (const Vec8i& a, const Vec8i& b) {
        return _mm256_xor_si256(a, b);
    }
    friend Vec8i andnot(const Vec8i& a, const Vec8i& b) {
        return _mm256_andnot_si256(a, b);
    }

    // Arithmetic operations [binary]
    friend Vec8i operator+ (const Vec8i& a, const Vec8i& b) {
        return _mm256_add_epi32(a, b);
    }
    friend Vec8i operator- (const Vec8i& a, const Vec8i& b) {
        return _mm256_sub_epi32(a, b);
    }
#endif // PCG_USE_AVX2

    // Element access (slow!) [const version]
    const int32_t& operator[] (size_t i) const {
        assert(i < 8);
        return i32[i];
    }

    // Element access (slow!)
    int32_t& operator[] (size_t i) {
        assert(i < 8);
        return i32[i];
    }



    // Compile time constants
    template <int32_t i7, int32_t i6, int32_t i5, int32_t i4,
              int32_t i3, int32_t i2, int32_t i1, int32_t i0>
    static const __m256i& constant() {
        static const union {
            int32_t i32[8];
            __m256i ymm;
        } u = {{i0, i1, i2, i3, i4, i5, i6, i7}};
        return u.ymm;
    }

    template <int32_t value>
    static const __m256i& constant() {
        static const union {
            int32_t i32[8];
            __m256i ymm;
        } u = {{value, value, value, value, value, value, value, value}};
        return u.ymm;
    }


} ALIGN32_END;



} // namespace pcg


#endif /* PCG_VEC8I_H */
