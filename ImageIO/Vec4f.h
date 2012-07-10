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
#if !defined(PCG_VEC4F_H)
#define PCG_VEC4F_H

#include "StdAfx.h"

#include <cassert>

namespace pcg
{

// Helper class intended to represent the mask produced by logical operations
// applied to the floating point numbers.
struct ALIGN16_BEG Vec4bf
{
private:
    __m128 xmm;

public:
    // Initialize from raw values
    Vec4bf(__m128 b) : xmm(b) {}

    // Cast operations
    operator __m128() const {
        return xmm;
    }
};



// Helper union to provide compile time constants, by initializing the first
// member of the union. Remember that values are loaded into the XMM registers
// in reverse order: f3,f2,f1,f0
union ALIGN16_BEG Vec4fUnion
{
    float f[4];
    __m128 xmm;
} ALIGN16_END;



// Abstraction of a vector of 4 single precision floating point numbers,
// using SSE instrinsics
struct ALIGN16_BEG Vec4f
{
private:
    union {
        __m128 xmm;
        float f[4];
    };

public:
    // Trivial constructor
    Vec4f() {}

    // Initialize from a raw __m128 value
    Vec4f(__m128 val) : xmm(val) {}

    // Initialize from the helper union
    Vec4f(const Vec4fUnion& vu) : xmm(vu.xmm) {}

    // Initialize with the same value in all components
    explicit Vec4f(float val) : xmm(_mm_set_ps1(val)) {}

    // Initialize with explicit values. The indices reflect the memory layout
    Vec4f(float f3, float f2, float f1, float f0) :
    xmm(_mm_set_ps(f3, f2, f1, f0))
    {}

    // Assignment
    Vec4f& operator= (float val) {
        xmm = _mm_set_ps1(val);
        return *this;
    }

    // Cast operations
    operator __m128() const {
        return xmm;
    }

    // Logical operators [binary]
    friend Vec4f operator& (const Vec4f& a, const Vec4f& b) {
        return _mm_and_ps(a, b);
    }
    friend Vec4f operator| (const Vec4f& a, const Vec4f& b) {
        return _mm_or_ps(a, b);
    }
    friend Vec4f operator^ (const Vec4f& a, const Vec4f& b) {
        return _mm_xor_ps(a, b);
    }

    // Arithmetic operations [binary]
    friend Vec4f operator+ (const Vec4f& a, const Vec4f& b) {
        return _mm_add_ps(a, b);
    }
    friend Vec4f operator- (const Vec4f& a, const Vec4f& b) {
        return _mm_sub_ps(a, b);
    }
    friend Vec4f operator* (const Vec4f& a, const Vec4f& b) {
        return _mm_mul_ps(a, b);
    }
    friend Vec4f operator/ (const Vec4f& a, const Vec4f& b) {
        return _mm_div_ps(a, b);
    }

    // Newton-Rhapson Reciprocal:
    // [2 * rcp(x) - (x * rcp(x) * rcp(x))]
    friend inline Vec4f rcp_nr(const Vec4f& v) {
        Vec4f x0 = _mm_rcp_ps(v);
        return _mm_sub_ps(_mm_add_ps(x0,x0), _mm_mul_ps(_mm_mul_ps(x0,v), x0));
    }

    // Element access (slow!) [const version]
    const float& operator[] (size_t i) const {
        assert(0 <= i && i < 4);
        return f[i];
    }

    // Element access (slow!) [const version]
    float& operator[] (size_t i) {
        assert(0 <= i && i < 4);
        return f[i];
    }

    // Min and max
    friend Vec4f simd_min(const Vec4f& a, const Vec4f& b) {
        return _mm_min_ps(a, b);
    }
    friend Vec4f simd_max(const Vec4f& a, const Vec4f& b) {
        return _mm_max_ps(a, b);
    }

    // Comparisons, return a mask
    #define PCG_VEC4F_COMP(op)                               \
    friend Vec4bf cmp##op (const Vec4f& a, const Vec4f& b) { \
        return _mm_cmp##op##_ps(a, b);                       \
    }
        PCG_VEC4F_COMP(eq)   // cmpeq(a,b)
        PCG_VEC4F_COMP(lt)   // cmplt(a,b)
        PCG_VEC4F_COMP(le)   // cmple(a,b)
        PCG_VEC4F_COMP(gt)   // cmpgt(a,b)
        PCG_VEC4F_COMP(ge)   // cmpge(a,b)
        PCG_VEC4F_COMP(neq)  // cmpneq(a,b)
        PCG_VEC4F_COMP(nlt)  // cmpnlt(a,b)
        PCG_VEC4F_COMP(nle)  // cmpnle(a,b)
        PCG_VEC4F_COMP(ngt)  // cmpngt(a,b)
        PCG_VEC4F_COMP(nge)  // cmpnge(a,b)
    #undef PCG_VEC4F_COMP

    friend Vec4bf operator==(const Vec4f& a, const Vec4f& b) {
        return cmpeq(a, b);
    }
    friend Vec4bf operator!=(const Vec4f& a, const Vec4f& b) {
        return cmpneq(a, b);
    }
    friend Vec4bf operator<(const Vec4f& a, const Vec4f& b) {
        return cmplt(a, b);
    }
    friend Vec4bf operator<=(const Vec4f& a, const Vec4f& b) {
        return cmple(a, b);
    }
    friend Vec4bf operator>(const Vec4f& a, const Vec4f& b) {
        return cmpgt(a, b);
    }
    friend Vec4bf operator>=(const Vec4f& a, const Vec4f& b) {
        return cmpge(a, b);
    }

    // Select (mask) ? a : b
    friend inline Vec4f select(const Vec4bf& mask,
        const Vec4f& a, const Vec4f& b)
    {
        return _mm_or_ps(_mm_and_ps(mask, a), _mm_andnot_ps(mask, b));
    }


} ALIGN16_END;


} // namespace pcg


#endif /* PCG_VEC4F_H */