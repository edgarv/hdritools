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
#if !defined(PCG_VEC4I_H)
#define PCG_VEC4I_H

#include "StdAfx.h"

#include <cassert>

namespace pcg
{

// Helper class intended to represent the mask produced by logical operations
struct ALIGN16_BEG Vec4bi
{
private:
    __m128i xmm;

public:
    // Initialize from raw values
    Vec4bi(__m128i b) : xmm(b) {}

    // Cast operations
    operator __m128i() const {
        return xmm;
    }

    friend Vec4bi operator& (const Vec4bi& a, const Vec4bi& b) {
        return _mm_and_si128(a, b);
    }
    friend Vec4bi operator| (const Vec4bi& a, const Vec4bi& b) {
        return _mm_or_si128(a, b);
    }
    friend Vec4bi operator^ (const Vec4bi& a, const Vec4bi& b) {
        return _mm_xor_si128(a, b);
    }

} ALIGN16_END;



// Helper union to provide compile time constants, by initializing the first
// member of the union. Remember that values are loaded into the XMM registers
// in reverse order: f3,f2,f1,f0
union ALIGN16_BEG Vec4iUnion
{
    __m128i xmm;
    int32_t i32[4];
} ALIGN16_END;



struct ALIGN16_BEG Vec4i
{
private:
    union {
        __m128i xmm;
        int32_t i32[4];
    };

public:
    // Trivial constructor
    Vec4i() {}

    // Initialize from a raw __m128i value
    Vec4i(__m128i val) : xmm(val) {}

    // Initialize with the same value in all components
    explicit Vec4i(int32_t val) : xmm(_mm_set1_epi32(val)) {}

    // Initialize with explicit values. The indices reflect the memory layout
    Vec4i(int32_t i3, int32_t i2, int32_t i1, int32_t i0) :
    xmm(_mm_set_epi32(i3, i2, i1, i0))
    {}

    // Assignment
    Vec4i& operator= (int32_t val) {
        xmm = _mm_set1_epi32(val);
        return *this;
    }

    // Zero vector. Useful during code generation
    inline static Vec4i zero() {
        return _mm_setzero_si128();
    }

    // Cast operations
    operator __m128i() const {
        return xmm;
    }

    // Logical operators [binary]
    friend Vec4i operator& (const Vec4i& a, const Vec4i& b) {
        return _mm_and_si128(a, b);
    }
    friend Vec4i operator| (const Vec4i& a, const Vec4i& b) {
        return _mm_or_si128(a, b);
    }
    friend Vec4i operator^ (const Vec4i& a, const Vec4i& b) {
        return _mm_xor_si128(a, b);
    }
    friend Vec4i andnot(const Vec4i& a, const Vec4i& b) {
        return _mm_andnot_si128(a, b);
    }

    // Arithmetic operations [binary]
    friend Vec4i operator+ (const Vec4i& a, const Vec4i& b) {
        return _mm_add_epi32(a, b);
    }
    friend Vec4i operator- (const Vec4i& a, const Vec4i& b) {
        return _mm_sub_epi32(a, b);
    }

    // Element access (slow!) [const version]
    const int32_t& operator[] (size_t i) const {
        assert(i < 4);
        return i32[i];
    }

    // Element access (slow!) [const version]
    int32_t& operator[] (size_t i) {
        assert(i < 4);
        return i32[i];
    }

    // Comparisons, return a mask
    #define PCG_VEC4I_COMP(op)                               \
    friend Vec4bi cmp##op (const Vec4i& a, const Vec4i& b) { \
        return _mm_cmp##op##_epi32(a, b);                       \
    }
        PCG_VEC4I_COMP(eq)   // cmpeq(a,b)
        PCG_VEC4I_COMP(lt)   // cmplt(a,b)
        PCG_VEC4I_COMP(gt)   // cmpgt(a,b)
    #undef PCG_VEC4I_COMP

    friend Vec4bi operator==(const Vec4i& a, const Vec4i& b) {
        return cmpeq(a, b);
    }

    friend Vec4bi operator<(const Vec4i& a, const Vec4i& b) {
        return cmplt(a, b);
    }

    friend Vec4bi operator>(const Vec4i& a, const Vec4i& b) {
        return cmpgt(a, b);
    }

    // Select (mask) ? a : b
    friend inline Vec4i select(const Vec4bi& mask,
        const Vec4i& a, const Vec4i& b)
    {
        return _mm_or_si128(_mm_and_si128(mask, a), _mm_andnot_si128(mask, b));
    }


    // Compile time constants
    template <int32_t i3, int32_t i2, int32_t i1, int32_t i0>
    static const __m128i& constant() {
        static const union {
            int32_t i32[4];
            __m128i xmm;
        } u = {{i0, i1, i2, i3}};
        return u.xmm;
    }

    template <int32_t value>
    static const __m128i& constant() {
        static const union {
            int32_t i32[4];
            __m128i xmm;
        } u = {{value, value, value, value}};
        return u.xmm;
    }


} ALIGN16_END;



} // namespace pcg


#endif /* PCG_VEC4I_H */
