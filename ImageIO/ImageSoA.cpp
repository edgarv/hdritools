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

#include "ImageSoA.h"
#include "StdAfx.h"
#include "Rgba32F.h"

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>


namespace
{
struct CopyFunctor
{
    typedef tbb::blocked_range<int> Range;

    struct Args
    {
        const pcg::Image<pcg::Rgba32F, pcg::TopDown> &src;
        pcg::RGBImageSoA &dest;

        Args(const pcg::Image<pcg::Rgba32F, pcg::TopDown> &source,
            pcg::RGBImageSoA &target) : src(source), dest(target)
        {}
    };


    CopyFunctor(const Args &args) : m_args(args) {}
    CopyFunctor(const CopyFunctor &other) : m_args(other.m_args) {}


    inline void
    copy1(float& r, float& g, float& b, const pcg::Rgba32F& src) const
    {
        pcg::Rgba32F pixel(src);
        pixel.applyAlpha();
        r = pixel.r();
        g = pixel.g();
        b = pixel.b();
    }

    inline void
    copy4(float* r, float* g, float *b, const pcg::Rgba32F* src, int off) const
    {
        __m128 p0 = src[off];
        __m128 p1 = src[off + 1];
        __m128 p2 = src[off + 2];
        __m128 p3 = src[off + 3];
        PCG_MM_TRANSPOSE4_PS (p0, p1, p2, p3);

        // Multiply by alpha
        _mm_stream_ps(r + off, _mm_mul_ps(p3, p0));
        _mm_stream_ps(g + off, _mm_mul_ps(p2, p0));
        _mm_stream_ps(b + off, _mm_mul_ps(p1, p0));
    }

    void operator() (Range& range) const
    {
        assert(range.begin() >= 0);
        assert(range.end() <= m_args.src.Size());
        assert(range.begin() <= range.end());

        // The range might not be in appropriate multiples of four
        const int beginSSE = (range.begin() + 3) & ~0x3;
        const int endSSE   = range.end() & ~0x3;
        assert((endSSE - beginSSE) % 4 == 0);
        assert((beginSSE-range.begin())+(endSSE-beginSSE)+(range.end()-endSSE)\
            ==range.size());

        float * r = m_args.dest.GetDataPointer<pcg::RGBImageSoA::R>();
        float * g = m_args.dest.GetDataPointer<pcg::RGBImageSoA::G>();
        float * b = m_args.dest.GetDataPointer<pcg::RGBImageSoA::B>();
        const pcg::Rgba32F* src = m_args.src.GetDataPointer();

        // Copy individual items until the first multiple of 4, so that they
        // are aligned to 16 bytes (1 to 3 elements)
        for (int i = range.begin(); i < beginSSE; ++i) {
            copy1(r[i], g[i], b[i], src[i]);
        }

        // Copy all the blocks of 4 pixels. This is the heaviest loop
        for (int offset = beginSSE; offset < endSSE; offset += 4)
        {
            copy4(r, g, b, src, offset);
        }

        // Copy any remaining elements (1 to 3)
        for (int i = endSSE; i < range.end(); ++i) {
            copy1(r[i], g[i], b[i], src[i]);
        }
    }

private:
    Args m_args;
};

} // Namespace



void
pcg::RGBImageSoA::copyImage(const pcg::Image<pcg::Rgba32F, pcg::TopDown> &img)
{
    CopyFunctor::Range range(0, img.Size(), 4);
    CopyFunctor functor(CopyFunctor::Args(img, *this));
    tbb::parallel_for(range, functor);
}
