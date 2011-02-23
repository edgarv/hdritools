/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary authors:
     Bruce Walter <bjw@graphics.cornell.edu>
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

/* THIS CODE CARRIES NO GUARANTEE OF USABILITY OR FITNESS FOR ANY PURPOSE.
 * WHILE THE AUTHORS HAVE TRIED TO ENSURE THE PROGRAM WORKS CORRECTLY,
 * IT IS STRICTLY USE AT YOUR OWN RISK.  */

#include "StdAfx.h"

#include "rgbe.h"

#include <emmintrin.h>

/* This file contains code to read and write four byte rgbe file format
 developed by Greg Ward.  It handles the conversions between rgbe and
 pixels consisting of floats.  The data is assumed to be an array of floats.
 By default there are three floats per pixel in the order red, green, blue.
 (RGBE_DATA_??? values control this.)  Only the mimimal header reading and 
 writing is implemented.  Each routine does error checking and will return
 a status value as defined below.  This code is intended as a skeleton so
 feel free to modify it to suit your needs.

 (Place notice here if you modified the code.)

 2008.02.10 edgar   Adapted to C++ and decoupled file reading
            and pixel conversion operations.
 2002.10.29	westin	Allow additional header after FORMAT= line; such
			files are routinely emitted by RADIANCE tools.

 posted to http://www.graphics.cornell.edu/~bjw/
 written by Bruce Walter  (bjw@graphics.cornell.edu)  5/26/95
 based on code written by Greg Ward
*/


using namespace pcg;

const unsigned int Rgbe::rgbeLUT_UI[2][256] =
	#include "rgbeLUT.h"

const float * Rgbe::rgbeCastLUT = 
    reinterpret_cast<const float*>(&Rgbe::rgbeLUT_UI[0][0]);
const float * Rgbe::rgbeExpLUT  = 
    reinterpret_cast<const float*>(&Rgbe::rgbeLUT_UI[1][0]);

namespace
{

inline void zero (pcg::Rgbe *v) {
    *reinterpret_cast<int*>(v) = 0;
}

} // namespace



void
Rgbe::set(const float red, const float green, const float blue) 
{
    Rgba32F pixel (red, green, blue);
    set (pixel);
}



void
Rgbe::set(const Rgba32F &pixel)
{
    //__m128 vals = _mm_set_ps (0.0f, blue, green, red);
    // Shuffles the value into this order:
    // r3 = r2: blue, r1 = green, r0 = red
    __m128 vals =  _mm_shuffle_ps((__m128)pixel, (__m128)pixel, 
        _MM_SHUFFLE(1, 1, 2, 3));
    
    // Test for NaNs
    if (_mm_movemask_ps (_mm_cmpeq_ps (vals, vals)) != 0xF) {
        zero (this);
        return;
    }

    // Clamp to zero
    vals = _mm_max_ps(vals, _mm_setzero_ps());

    // After clamping (r,g,b) to zero, this function implements
    // essentially the following:
    //
    // float v;
    // int e;
    //
    // v = red;
    // if (green > v) v = green;
    // if (blue > v) v = blue;
    // if (v < 1e-32) {
    //     this->set(0,0,0,0);
    // }
    // else {
    //     double nFactor = (frexp(v,&e) * 256.0/v);
    //     this->r = (unsigned char) std::min(255.0, 
    //         (red   * nFactor + 0.5));
    //     this->g = (unsigned char) std::min(255.0, 
    //         (green * nFactor + 0.5));
    //     this->b = (unsigned char) std::min(255.0, 
    //         (blue  * nFactor + 0.5));
    //     this->e = (unsigned char) (e + 128);
    // }

    // Calculate max(red, max(green, blue))
    const __m128 valsShuffled = _mm_shuffle_ps(vals, vals, 
        _MM_SHUFFLE(0, 0, 2, 1));
    __m128 max1 = _mm_max_ps(vals, valsShuffled);
    __m128 max2 = _mm_max_ps(max1, 
        _mm_shuffle_ps(max1, max1, _MM_SHUFFLE(0, 0, 0, 1)));

    // At this point the maximum is in r0
    if (_mm_ucomige_ss(max2, _mm_set1_ps(1e-32f))) {
        union { float fval; int bits; };
        _mm_store_ss(&fval, max2);

        // Extract the IEEE 754 exponent using bit magic
        int biasedExponent = (bits >> 23) & 0x0FF;
        // Guard for overflow
        if (biasedExponent <= 253) {

            // Construct a additive normalizer which is just 2^(exp+1).
            // Adding this to each float will move the relevant mantissa 
            // bits to a known fixed location for easy extraction
            bits = (biasedExponent+1) << 23;
            // Initially we keep an extra bit (9-bits) so that we can 
            // perform rounding to 8-bits in the next step
            union { __m128 rawVals; __m128i rawValsi; int iVals[4]; };
            rawVals  = _mm_add_ps (vals, _mm_set1_ps(fval));
            // The shift accounts for the multiplication by 256
            rawValsi = _mm_srli_epi32 (rawValsi, 14);
            __m128i preValsi = 
                _mm_and_si128 (rawValsi, _mm_set1_epi32(0x1FF));
            //// Prevent overflow
            //rawValsi = _mm_min_epi16 (rawValsi, _mm_set1_epi32(0x1FE));
            // Round to nearest representable 8-bit value
            rawValsi = _mm_add_epi32 (preValsi, _mm_set1_epi32(1));
            rawValsi = _mm_srli_epi32 (rawValsi, 1);

            // Check to see if rounding causes an overflow
            __m128i overflowMask = 
                _mm_cmpgt_epi32 (rawValsi, _mm_set1_epi32(255));
            if (_mm_movemask_epi8 (overflowMask) == 0) {
                // No value was larger than 255
                e = static_cast<unsigned char> (biasedExponent + 2);
                r = static_cast<unsigned char> (iVals[0]);
                g = static_cast<unsigned char> (iVals[1]);
                b = static_cast<unsigned char> (iVals[2]);
            } else {
                // Rounding caused overflow, need to use larger exponent 
                // and redo the rounding
                if (biasedExponent < 253) {
                    rawValsi =
                        _mm_add_epi32 (preValsi, _mm_set1_epi32(2));
                    rawValsi = _mm_srli_epi32 (rawValsi, 2);

                    e = static_cast<unsigned char> (biasedExponent + 3);
                    r = static_cast<unsigned char> (iVals[0]);
                    g = static_cast<unsigned char> (iVals[1]);
                    b = static_cast<unsigned char> (iVals[2]);
                } else {
                    // Overflow
                    zero (this);
                }
            }
        } else {
            zero (this);
        }
    } else {
        zero (this);
    }
}
