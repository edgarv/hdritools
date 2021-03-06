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
#if !defined PCG_IMAGEITERATORS_H
#define PCG_IMAGEITERATORS_H

#include "ImageIO.h"
#include "StdAfx.h"
#include "Image.h"
#include "ImageSoA.h"
#include "Rgba32F.h"
#include "LDRPixels.h"

#include <iterator>

#include <cstddef>
#include <cassert>

namespace pcg
{


// Helper struct which represent 4 RGBA values, in SoA fashion
struct RGBA32FVec4
{
    __m128 data[4];

    inline __m128& r() {
        return data[3];
    }

    inline const __m128& r() const {
        return data[3];
    }

    inline __m128& g() {
        return data[2];
    }

    inline const __m128& g() const {
        return data[2];
    }

    inline __m128& b() {
        return data[1];
    }

    inline const __m128& b() const {
        return data[1];
    }

    inline __m128& a() {
        return data[0];
    }

    inline const __m128& a() const {
        return data[0];
    }
};



// Template which represent vectors of RGBA values, in SoA fashion,
// which are backed by external data + offset.
template <class VectorType, class VectorPtrType = VectorType*>
class RGBAVecRef
{
private:
    // Reference to pointers
    VectorPtrType& m_r;
    VectorPtrType& m_g;
    VectorPtrType& m_b;
    VectorPtrType& m_a;
    
    ptrdiff_t &m_offset;


public:
    RGBAVecRef(VectorPtrType& rPtrRef, VectorPtrType& gPtrRef,
               VectorPtrType& bPtrRef, VectorPtrType& aPtrRef,
               ptrdiff_t &offsetRef) :
    m_r(rPtrRef), m_g(gPtrRef), m_b(bPtrRef), m_a(aPtrRef), m_offset(offsetRef)
    {}

    inline VectorType& r() {
        return *(m_r + m_offset);
    }

    inline const VectorType& r() const {
        return *(m_r + m_offset);
    }

    inline VectorType& g() {
        return *(m_g + m_offset);
    }

    inline const VectorType& g() const {
        return *(m_g + m_offset);
    }

    inline VectorType& b() {
        return *(m_b + m_offset);
    }

    inline const VectorType& b() const {
        return *(m_b + m_offset);
    }

    inline VectorType& a() {
        return *(m_a + m_offset);
    }

    inline const VectorType& a() const {
        return *(m_a + m_offset);
    }
};



// Template which represent vectors of RGBA values, in SoA fashion,
// which are backed by external data + offset. This version only allows reads.
template <class VectorType, class VectorPtrType = VectorType*>
class RGBAVecConstRef
{
private:
    // Reference to pointers
    const VectorPtrType& m_r;
    const VectorPtrType& m_g;
    const VectorPtrType& m_b;
    const VectorPtrType& m_a;
    
    const ptrdiff_t &m_offset;


public:
    RGBAVecConstRef(const VectorPtrType& rPtrRef, const VectorPtrType& gPtrRef,
                    const VectorPtrType& bPtrRef, const VectorPtrType& aPtrRef,
                    const ptrdiff_t &offsetRef) :
    m_r(rPtrRef), m_g(gPtrRef), m_b(bPtrRef), m_a(aPtrRef), m_offset(offsetRef)
    {}

    inline const VectorType& r() const {
        return *(m_r + m_offset);
    }

    inline const VectorType& g() const {
        return *(m_g + m_offset);
    }

    inline const VectorType& b() const {
        return *(m_b + m_offset);
    }

    inline const VectorType& a() const {
        return *(m_a + m_offset);
    }
};

// Helper struct which represent 4 RGBA values
typedef RGBAVecRef<__m128> RGBA32FVec4Ref;

// Helper struct to represent 4 const RGBA values
typedef RGBAVecConstRef<__m128> RGBA32FVec4ConstRef;



// RGBA SoA Pixel Iterator concept for standard RGBA32F images. It iterates
// the image in groups of 4 pixels, returning a fresh RGBA32FVec4 with the
// next 4 R,G,B,A values in SoA form. Thus this is only a "read-only" iterator.
// The current implementation only iterates in the original scanlie order of
// an image.
class RGBA32FVec4ImageIterator :
public std::iterator<std::random_access_iterator_tag, RGBA32FVec4>
{
public:

    // Default constructor, which creates an invalid iterator
    RGBA32FVec4ImageIterator() : m_ptr(NULL)
    {}

    // Copy constructor
    RGBA32FVec4ImageIterator(const RGBA32FVec4ImageIterator& other)
    {
        m_ptr = other.m_ptr;
    }

    // Just wrap a pointer, be careful!
    RGBA32FVec4ImageIterator(const pcg::Rgba32F *ptr) : m_ptr(ptr)
    {
        assert(reinterpret_cast<intptr_t>(ptr) % 16 == 0);
    }

    // Equality/inequality comparisons

    inline bool operator== (const RGBA32FVec4ImageIterator& other) const {
        return m_ptr == other.m_ptr;
    }
    inline bool operator!= (const RGBA32FVec4ImageIterator& other) const {
        return m_ptr != other.m_ptr;
    }

    // Inequality comparisons between iterators

    inline bool operator< (const RGBA32FVec4ImageIterator& other) const {
        return m_ptr < other.m_ptr;
    }
    inline bool operator> (const RGBA32FVec4ImageIterator& other) const {
        return m_ptr > other.m_ptr;
    }
    inline bool operator<= (const RGBA32FVec4ImageIterator& other) const {
        return m_ptr <= other.m_ptr;
    }
    inline bool operator>= (const RGBA32FVec4ImageIterator& other) const {
        return m_ptr >= other.m_ptr;
    }

    // Increments

    inline RGBA32FVec4ImageIterator& operator++() {
        m_ptr += 4;
        return *this;
    }
    inline RGBA32FVec4ImageIterator& operator++(int) {
        m_ptr += 4;
        return *this;
    }

    // Decrements

    inline RGBA32FVec4ImageIterator& operator--() {
        m_ptr -= 4;
        return *this;
    }

    inline RGBA32FVec4ImageIterator& operator--(int) {
        m_ptr -= 4;
        return *this;
    }

    // Binary arithmetic operators

    inline friend RGBA32FVec4ImageIterator operator + (
        const RGBA32FVec4ImageIterator& a, difference_type offset)
    {
        RGBA32FVec4ImageIterator it(a);
        it.m_ptr += 4 * offset;
        return it;
    }

    inline friend RGBA32FVec4ImageIterator operator + (
        difference_type offset, const RGBA32FVec4ImageIterator& a)
    {
        RGBA32FVec4ImageIterator it(a);
        it.m_ptr += 4 * offset;
        return it;
    }

    inline friend RGBA32FVec4ImageIterator operator - (
        const RGBA32FVec4ImageIterator& a, difference_type offset)
    {
        RGBA32FVec4ImageIterator it(a);
        it.m_ptr -= 4 * offset;
        return it;
    }

    inline friend difference_type operator- (const RGBA32FVec4ImageIterator& a,
        const RGBA32FVec4ImageIterator& b)
    {
        RGBA32FVec4ImageIterator::difference_type rawDelta = a.m_ptr - b.m_ptr;
        return rawDelta >> 2;
    }

    // Compound assignment

    inline RGBA32FVec4ImageIterator& operator+=(difference_type offset) {
        m_ptr += 4 * offset;
        return *this;
    }

    inline RGBA32FVec4ImageIterator& operator-=(difference_type offset) {
        m_ptr -= 4 * offset;
        return *this;
    }

    // Offset dereference

    // Be aware that this returns a temporary element!
    inline RGBA32FVec4 operator[] (size_t idx) const
    {
        const pcg::Rgba32F* base = m_ptr + (4 * idx);
        RGBA32FVec4 p;
        p.data[0] = base[0];
        p.data[1] = base[1];
        p.data[2] = base[2];
        p.data[3] = base[3];
        PCG_MM_TRANSPOSE4_PS (p.data[0], p.data[1], p.data[2], p.data[3]);
        return p;
    }

    // Builds a RGBA32FVec4 from the current values pointed by the iterator
    inline RGBA32FVec4 operator*() const
    {
        RGBA32FVec4 p;
        p.data[0] = m_ptr[0];
        p.data[1] = m_ptr[1];
        p.data[2] = m_ptr[2];
        p.data[3] = m_ptr[3];
        PCG_MM_TRANSPOSE4_PS (p.data[0], p.data[1], p.data[2], p.data[3]);
        return p;
    }

    // Create an iterator at the beginning of the image, moving in the same
    // direction as the established scanline order
    template <pcg::ScanLineMode S>
    static RGBA32FVec4ImageIterator begin(const pcg::Image<pcg::Rgba32F,S> &src)
    {
        RGBA32FVec4ImageIterator it;
        it.m_ptr = src.GetDataPointer();
        return it;
    }

    // Create an iterator at the end of the image. Note that for this to work
    // the image must have a multiple of 4 number of pixels or have allocated
    // additional elements to avoid segfaults.
    template <pcg::ScanLineMode S>
    static RGBA32FVec4ImageIterator end(const pcg::Image<pcg::Rgba32F,S> &src)
    {
        RGBA32FVec4ImageIterator it;
        it.m_ptr = src.GetDataPointer();
        size_t offset = (src.Size() + 3) & ~0x3;
        assert (offset % 4 == 0);
        it.m_ptr += offset;
        return it;
    }

private:
    // Pointer to AoS data
    const pcg::Rgba32F *m_ptr;
};



// Helper struct which represent 4 packed RGBA values, in AoS fashion
struct PixelBGRA8Vec4
{
    union {
        __m128i xmm;
        PixelBGRA8 pixels[4];
    };

    // Create an iterator at the beginning of the image, moving in the same
    // direction as the established scanline order
    template <ScanLineMode S>
    static PixelBGRA8Vec4* begin(Image<Bgra8, S> &img)
    {
        Bgra8* ptr = img.GetDataPointer();
        assert(reinterpret_cast<intptr_t>(ptr) % 16 == 0);
        return reinterpret_cast<PixelBGRA8Vec4*>(ptr);
    }

    // Create an iterator at the end of the image, moving in the same
    // direction as the established scanline order. Note that for this to work
    // the image must have a multiple of 4 number of pixels or have allocated
    // additional elements to avoid segfaults (alll with the proper alignment).
    template <ScanLineMode S>
    inline PixelBGRA8Vec4* end(Image<Bgra8, S> &img)
    {
        Bgra8* ptr = img.GetDataPointer();
        assert(reinterpret_cast<intptr_t>(ptr) % 16 == 0);
        size_t offset = (img.Size() + 3) & ~0x3;
        assert (offset % 4 == 0);
        ptr += offset;
        return reinterpret_cast<PixelBGRA8Vec4*>(ptr);
    }
};



// Helper traits to find appropriate classes for the iterator
template <int N>
struct RGBA32FVec_traits;

template <>
struct RGBA32FVec_traits<1>
{
    typedef RGBAVecRef<float> VecRef;
    typedef RGBAVecConstRef<float> VecConstRef;
    typedef float  value_type;
    typedef float* pointer_type;
};

template <>
struct RGBA32FVec_traits<4>
{
    typedef RGBA32FVec4Ref VecRef;
    typedef RGBA32FVec4ConstRef VecConstRef;
    typedef __m128  value_type;
    typedef __m128* pointer_type;
};



// RGBA SoA Pixel Iterator concept template for SoA images. It iterates
// the image in groups of N pixels, returning a RGBAVecConstRef<N> with the
// next N R,G,B,A values in SoA form. Thus this is a "read-only" iterator.
// The current implementation only iterates in the original scanlie order of
// an image.
template <int N>
class RGBA32FVecImageSoAIterator :
public std::iterator<std::bidirectional_iterator_tag,
                     typename RGBA32FVec_traits<N>::VecRef>
{
public:

    // Default constructor, which creates an invalid iterator
    RGBA32FVecImageSoAIterator() :
    m_r(NULL), m_g(NULL), m_b(NULL), m_a(NULL), m_offset(0),
    m_pixRef(m_r, m_g, m_b, m_a, m_offset)
    {}

    // Copy constructor
    RGBA32FVecImageSoAIterator(const RGBA32FVecImageSoAIterator& other) :
    m_r(other.m_r), m_g(other.m_g), m_b(other.m_b), m_a(other.m_a),
    m_offset(other.m_offset),
    m_pixRef(m_r, m_g, m_b, m_a, m_offset)
    {}

    // Assignment
    RGBA32FVecImageSoAIterator& operator=(const RGBA32FVecImageSoAIterator& rhs)
    {
        m_r = rhs.m_r;
        m_g = rhs.m_g;
        m_b = rhs.m_b;
        m_a = rhs.m_a;
        m_offset = rhs.m_offset;
        return *this;
    }

    // Equality/inequality comparisons using only the offsets

    inline bool operator== (const RGBA32FVecImageSoAIterator& other) const {
        assert(haveSameBase(other));
        return m_offset == other.m_offset;
    }
    inline bool operator!= (const RGBA32FVecImageSoAIterator& other) const {
        assert(haveSameBase(other));
        return m_offset != other.m_offset;
    }

    // Inequality comparisons between iterators

    inline bool operator< (const RGBA32FVecImageSoAIterator& other) const {
        assert(haveSameBase(other));
        return m_offset < other.m_offset;
    }
    inline bool operator> (const RGBA32FVecImageSoAIterator& other) const {
        assert(haveSameBase(other));
        return m_offset > other.m_offset;
    }
    inline bool operator<= (const RGBA32FVecImageSoAIterator& other) const {
        assert(haveSameBase(other));
        return m_offset <= other.m_offset;
    }
    inline bool operator>= (const RGBA32FVecImageSoAIterator& other) const {
        assert(haveSameBase(other));
        return m_offset >= other.m_offset;
    }

    // Increments

    inline RGBA32FVecImageSoAIterator& operator++() {
        ++m_offset;
        return *this;
    }
    inline RGBA32FVecImageSoAIterator& operator++(int) {
        ++m_offset;
        return *this;
    }

    // Decrements

    inline RGBA32FVecImageSoAIterator& operator--() {
        --m_offset;
        return *this;
    }

    inline RGBA32FVecImageSoAIterator& operator--(int) {
        --m_offset;
        return *this;
    }

    // Binary arithmetic operators

    inline friend RGBA32FVecImageSoAIterator operator + (
        const RGBA32FVecImageSoAIterator& a,
        typename RGBA32FVecImageSoAIterator<N>::difference_type offset)
    {
        RGBA32FVecImageSoAIterator it(a);
        it.m_offset += offset;
        return it;
    }

    inline friend RGBA32FVecImageSoAIterator operator + (
        typename RGBA32FVecImageSoAIterator<N>::difference_type offset,
        const RGBA32FVecImageSoAIterator& a)
    {
        RGBA32FVecImageSoAIterator it(a);
        it.m_offset += offset;
        return it;
    }

    inline friend RGBA32FVecImageSoAIterator operator- (
        const RGBA32FVecImageSoAIterator& a,
        typename RGBA32FVecImageSoAIterator<N>::difference_type offset)
    {
        RGBA32FVecImageSoAIterator it(a);
        it.m_offset -= offset;
        return it;
    }

    inline friend typename RGBA32FVecImageSoAIterator<N>::difference_type
        operator- (const RGBA32FVecImageSoAIterator& a,
                   const RGBA32FVecImageSoAIterator& b)
    {
        assert(a.haveSameBase(b));
        typename RGBA32FVecImageSoAIterator<N>::difference_type d =
            a.m_offset - b.m_offset;
        return d;
    }

    // Compound assignment

    inline RGBA32FVecImageSoAIterator& operator+=(
        typename RGBA32FVecImageSoAIterator<N>::difference_type offset)
    {
        m_offset += offset;
        return *this;
    }

    inline RGBA32FVecImageSoAIterator& operator-=(
        typename RGBA32FVecImageSoAIterator<N>::difference_type offset)
    {
        m_offset -= offset;
        return *this;
    }

    // Element access to the current pixel

    inline typename RGBA32FVec_traits<N>::VecRef & operator*()
    {
        return m_pixRef;
    }

    inline const typename RGBA32FVec_traits<N>::VecRef & operator*() const
    {
        return m_pixRef;
    }

    inline typename RGBA32FVec_traits<N>::VecRef * operator->()
    {
        return &m_pixRef;
    }

    inline const typename RGBA32FVec_traits<N>::VecRef * operator->() const
    {
        return &m_pixRef;
    }


    // Create an iterator at the beginning of the image, moving in the same
    // direction as the established scanline order
    static RGBA32FVecImageSoAIterator begin(const RGBAImageSoA &src)
    {
        typedef typename RGBA32FVec_traits<N>::pointer_type vptr_t;
        RGBA32FVecImageSoAIterator it;
        it.m_r=reinterpret_cast<vptr_t>(src.GetDataPointer<RGBAImageSoA::R>());
        it.m_g=reinterpret_cast<vptr_t>(src.GetDataPointer<RGBAImageSoA::G>());
        it.m_b=reinterpret_cast<vptr_t>(src.GetDataPointer<RGBAImageSoA::B>());
        it.m_a=reinterpret_cast<vptr_t>(src.GetDataPointer<RGBAImageSoA::A>());
        it.m_offset = 0;
        return it;
    }

    // Create an iterator at the end of the image. Note that for this to work
    // the image must have a multiple of 4 number of pixels or have allocated
    // additional elements to avoid segfaults.
    static RGBA32FVecImageSoAIterator end(const RGBAImageSoA &src)
    {
        RGBA32FVecImageSoAIterator it = begin(src);
        it.m_offset = (src.Size() + (N-1)) / N;
        return it;
    }

private:
#ifndef NDEBUG
    inline bool haveSameBase(const RGBA32FVecImageSoAIterator& other) const {
        return m_r == other.m_r && m_g == other.m_g &&
               m_b == other.m_b && m_a == other.m_a;
    }
#endif

    // Pointers to the *base* data
    typename RGBA32FVec_traits<N>::pointer_type m_r;
    typename RGBA32FVec_traits<N>::pointer_type m_g;
    typename RGBA32FVec_traits<N>::pointer_type m_b;
    typename RGBA32FVec_traits<N>::pointer_type m_a;
    
    // Offset
    ptrdiff_t m_offset;

    // Helper reference to pixels
    RGBAVecRef<typename RGBA32FVec_traits<N>::value_type> m_pixRef;
};



// Scalar RGBA SoA Pixel Iterator concept for SoA images.
typedef RGBA32FVecImageSoAIterator<1> RGBA32FScalarImageSoAIterator;

// RGBA SoA Pixel Iterator concept for SoA images. It iterates
// the image in groups of 4 pixels, returning a fresh RGBA32FVec4 with the
// next 4 R,G,B,A values in SoA form. Thus this is only a "read-only" iterator.
// The current implementation only iterates in the original scanlie order of
// an image.
typedef RGBA32FVecImageSoAIterator<4> RGBA32FVec4ImageSoAIterator;



#if PCG_USE_AVX

// Helper struct which represent 8 RGBA values
typedef RGBAVecRef<__m256> RGBA32FVec8Ref;

// Helper struct to represent 8 const RGBA values
typedef RGBAVecConstRef<__m256> RGBA32FVec8ConstRef;

// Helper struct which represent 8 packed RGBA values, in AoS fashion
struct PixelBGRA8Vec8
{
    union {
        __m256i ymm;
        __m128i xmm[2];
        PixelBGRA8 pixels[8];
    };

    // Create an iterator at the beginning of the image, moving in the same
    // direction as the established scanline order
    template <ScanLineMode S>
    static PixelBGRA8Vec8* begin(Image<Bgra8, S> &img)
    {
        Bgra8* ptr = img.GetDataPointer();
        assert(reinterpret_cast<intptr_t>(ptr) % 32 == 0);
        return reinterpret_cast<PixelBGRA8Vec8*>(ptr);
    }

    // Create an iterator at the end of the image, moving in the same
    // direction as the established scanline order. Note that for this to work
    // the image must have a multiple of 8 number of pixels or have allocated
    // additional elements to avoid segfaults (alll with the proper alignment).
    template <ScanLineMode S>
    inline PixelBGRA8Vec8* end(Image<Bgra8, S> &img)
    {
        Bgra8* ptr = img.GetDataPointer();
        assert(reinterpret_cast<intptr_t>(ptr) % 32 == 0);
        size_t offset = (img.Size() + 7) & ~0x7;
        assert (offset % 8 == 0);
        ptr += offset;
        return reinterpret_cast<PixelBGRA8Vec8*>(ptr);
    }
};

template <>
struct RGBA32FVec_traits<8>
{
    typedef RGBA32FVec8Ref VecRef;
    typedef RGBA32FVec8ConstRef VecConstRef;
    typedef __m256  value_type;
    typedef __m256* pointer_type;
};

// RGBA SoA Pixel Iterator concept for SoA images. It iterates
// the image in groups of 8 pixels, returning a fresh RGBA32FVec8 with the
// next 8 R,G,B,A values in SoA form. Thus this is only a "read-only" iterator.
// The current implementation only iterates in the original scanlie order of
// an image.
typedef RGBA32FVecImageSoAIterator<8> RGBA32FVec8ImageSoAIterator;

#endif // PCG_USE_AVX

}

#endif /* PCG_IMAGEITERATORS_H */
