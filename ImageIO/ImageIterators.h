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
#include "Rgba32F.h"
#include "LDRPixels.h"

#include <iterator>

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

}

#endif /* PCG_IMAGEITERATORS_H */
