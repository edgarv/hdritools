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
#if !defined(PCG_VEC8F_H)
#define PCG_VEC8F_H

#include "StdAfx.h"

#include <cassert>

namespace pcg
{

// Helper class intended to represent the mask produced by logical operations
// applied to the floating point numbers.
struct ALIGN32_BEG Vec8bf
{
private:
    __m256 ymm;

public:
    // Initialize from raw values
    Vec8bf(__m256 b) : ymm(b) {}

    // Cast operations
    operator __m256() const {
        return ymm;
    }
} ALIGN32_END;



// Helper union to provide compile time constants, by initializing the first
// member of the union. Remember that values are loaded into the YMM registers
// in reverse order: f7,f6,f5,f5,f3,f2,f1,f0
union ALIGN32_BEG Vec8fUnion
{
    float f[8];
    __m128 xmm[2];
    __m256 ymm;
} ALIGN32_END;



// Abstraction of a vector of 4 single precision floating point numbers,
// using SSE instrinsics
struct ALIGN32_BEG Vec8f
{
private:
    union {
        __m256 ymm;
        __m128 xmm[2];
        float f[8];
    };

public:
    // Trivial constructor
    Vec8f() {}

    // Initialize from a raw __m128 value
    Vec8f(__m256 val) : ymm(val) {}

    // Initialize from the helper union
    Vec8f(const Vec8fUnion& vu) : ymm(vu.ymm) {}

    // Initialize with the same value in all components
    explicit Vec8f(float val) : ymm(_mm256_set1_ps(val)) {}

    // Initialize with explicit values. The indices reflect the memory layout
    Vec8f(float f7, float f6, float f5, float f4,
          float f3, float f2, float f1, float f0) :
    ymm(_mm256_set_ps(f7, f6, f5, f4, f3, f2, f1, f0))
    {}

    // Assignment
    Vec8f& operator= (float val) {
        ymm = _mm256_set1_ps(val);
        return *this;
    }

    // Zero vector. Useful during code generation
    inline static Vec8f zero() {
        return _mm256_setzero_ps();
    }

    // Cast operations
    operator __m256() const {
        return ymm;
    }

    // Logical operators [binary]
    friend Vec8f operator& (const Vec8f& a, const Vec8f& b) {
        return _mm256_and_ps(a, b);
    }
    friend Vec8f operator| (const Vec8f& a, const Vec8f& b) {
        return _mm256_or_ps(a, b);
    }
    friend Vec8f operator^ (const Vec8f& a, const Vec8f& b) {
        return _mm256_xor_ps(a, b);
    }

    // Arithmetic operations [binary]
    friend Vec8f operator+ (const Vec8f& a, const Vec8f& b) {
        return _mm256_add_ps(a, b);
    }
    friend Vec8f operator- (const Vec8f& a, const Vec8f& b) {
        return _mm256_sub_ps(a, b);
    }
    friend Vec8f operator* (const Vec8f& a, const Vec8f& b) {
        return _mm256_mul_ps(a, b);
    }
    friend Vec8f operator/ (const Vec8f& a, const Vec8f& b) {
        return _mm256_div_ps(a, b);
    }

    // Newton-Rhapson Reciprocal:
    // [2 * rcp(x) - (x * rcp(x) * rcp(x))]
    friend inline Vec8f rcp_nr(const Vec8f& v) {
        Vec8f x0 = _mm256_rcp_ps(v);
        return _mm256_sub_ps(_mm256_add_ps(x0,x0),
                             _mm256_mul_ps(_mm256_mul_ps(x0,v), x0));
    }

    // Element access (slow!) [const version]
    const float& operator[] (size_t i) const {
        assert(i < 8);
        return f[i];
    }

    // Element access (slow!)
    float& operator[] (size_t i) {
        assert(i < 8);
        return f[i];
    }

    // Min and max
    friend Vec8f simd_min(const Vec8f& a, const Vec8f& b) {
        return _mm256_min_ps(a, b);
    }
    friend Vec8f simd_max(const Vec8f& a, const Vec8f& b) {
        return _mm256_max_ps(a, b);
    }

    // Comparisons, return a mask. Ordered/unordered, quiet or signaling refer
    // to the behavior with respect to NaN as per IEEE 754-2008 Section 5.11.
    #define PCG_VEC8F_COMP(op, pred)                         \
    friend Vec8bf cmp##op (const Vec8f& a, const Vec8f& b) { \
        return _mm256_cmp_ps(a, b, pred);                    \
    }
        PCG_VEC8F_COMP(eq,  _CMP_EQ_OQ)   // cmpeq(a,b)
        PCG_VEC8F_COMP(lt,  _CMP_LT_OQ)   // cmplt(a,b)
        PCG_VEC8F_COMP(le,  _CMP_LE_OQ)   // cmple(a,b)
        PCG_VEC8F_COMP(gt,  _CMP_GT_OQ)   // cmpgt(a,b)
        PCG_VEC8F_COMP(ge,  _CMP_GE_OQ)   // cmpge(a,b)
        PCG_VEC8F_COMP(neq, _CMP_NEQ_UQ)  // cmpneq(a,b)
        PCG_VEC8F_COMP(nlt, _CMP_NLT_UQ)  // cmpnlt(a,b)
        PCG_VEC8F_COMP(nle, _CMP_NLE_UQ)  // cmpnle(a,b)
        PCG_VEC8F_COMP(ngt, _CMP_NGT_UQ)  // cmpngt(a,b)
        PCG_VEC8F_COMP(nge, _CMP_NGE_UQ)  // cmpnge(a,b)
    #undef PCG_VEC8F_COMP

    friend Vec8bf operator==(const Vec8f& a, const Vec8f& b) {
        return cmpeq(a, b);
    }
    friend Vec8bf operator!=(const Vec8f& a, const Vec8f& b) {
        return cmpneq(a, b);
    }
    friend Vec8bf operator<(const Vec8f& a, const Vec8f& b) {
        return cmplt(a, b);
    }
    friend Vec8bf operator<=(const Vec8f& a, const Vec8f& b) {
        return cmple(a, b);
    }
    friend Vec8bf operator>(const Vec8f& a, const Vec8f& b) {
        return cmpgt(a, b);
    }
    friend Vec8bf operator>=(const Vec8f& a, const Vec8f& b) {
        return cmpge(a, b);
    }

    // Select (mask) ? a : b
    friend inline Vec8f select(const Vec8bf& mask,
        const Vec8f& a, const Vec8f& b) {
        return _mm256_blendv_ps(b, a, mask);
    }


} ALIGN32_END;


} // namespace pcg


#endif /* PCG_VEC8F_H */
