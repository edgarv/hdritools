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



void
pcg::RGBImageSoA::copyImage(const pcg::Image<pcg::Rgba32F, pcg::TopDown> &img)
{
    // Copy four elements at the time
    const int size      = img.Size();
    const int size4     = size >> 2;
    const int sizeExtra = size &  0x3;

    float * r = GetDataPointer<R>();
    float * g = GetDataPointer<G>();
    float * b = GetDataPointer<B>();
    const pcg::Rgba32F* src = img.GetDataPointer();

    for (int idx = 0; idx < size4; ++idx) {
        __m128 p0 = src[0];
        __m128 p1 = src[1];
        __m128 p2 = src[2];
        __m128 p3 = src[3];
        PCG_MM_TRANSPOSE4_PS (p0, p1, p2, p3);

        // FIXME Is it OK to ignore alpha?
        _mm_stream_ps(r, p3);
        _mm_stream_ps(g, p2);
        _mm_stream_ps(b, p1);

        // Advance all pointers
        r   += 4;
        g   += 4;
        b   += 4;
        src += 4;
    }

    // Copy for the remaining elements, zeroing the padding
    if (sizeExtra != 0) {
        __m128 values[4];
        for (int idx = 0; idx < sizeExtra; ++idx) {
            values[idx] = src[idx];
        }
        for (int idx = sizeExtra; idx < 4; ++idx) {
            values[idx] = _mm_setzero_ps();
        }

        __m128 p0 = values[0];
        __m128 p1 = values[1];
        __m128 p2 = values[2];
        __m128 p3 = values[3];
        PCG_MM_TRANSPOSE4_PS (p0, p1, p2, p3);

        // FIXME Is it OK to ignore alpha?
        _mm_stream_ps(r, p3);
        _mm_stream_ps(g, p2);
        _mm_stream_ps(b, p1);
    }
}
