/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#include "ToneMapper.h"

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

// Intel Threading Blocks 2.0
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>

// SSE3 functions are only available as intrinsic in older versions of MSVC
#if defined(_MSC_VER) && _MSC_VER < 1500 && !defined(__INTEL_COMPILER)
#include <intrin.h>
#pragma intrinsic ( _mm_hadd_ps )
#else
#include <pmmintrin.h>
#endif // _MSC_VER


// FIXME Make this a setup flag
#define USE_SSE_POW 1

#if USE_SSE_POW
namespace ssemath {
    #include "sse_mathfun.h"
}
#endif

using namespace pcg;
using namespace tbb;

// Global static variables of the tone mapper
const Rgba32F ToneMapper::ZEROS = Rgba32F(0.0f);
const Rgba32F ToneMapper::ONES  = Rgba32F(1.0f);

namespace
{

// Helper function to set all fields in a vector with the same value
template <int idx>
inline __m128 broadcast_idx(const __m128 n)
{
    __m128 res = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(n), 
        _MM_SHUFFLE(idx,idx,idx,idx)));
    return res;
}

// Helper function to calculate a dot product
inline float dot_float(const Rgba32F &a, const Rgba32F &b)
{
    float res;
    Rgba32F dot_tmp = a * b;
    dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
    dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
    _mm_store_ss(&res, dot_tmp);
    return res;
}


// Helper LUT to translate from the 4-bit mask of _mm_movemask_ps to a 32-bit 
const uint32_t MOVEMASK_LUT[] = {
           0x0,
          0xff,
        0xff00,
        0xffff,
      0xff0000,
      0xff00ff,
      0xffff00,
      0xffffff,
    0xff000000,
    0xff0000ff,
    0xff00ff00,
    0xff00ffff,
    0xffff0000,
    0xffff00ff,
    0xffffff00,
    0xffffffff
};


#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
inline float exp2f(float x)
{
    return powf(2.0f, x);
}
#endif

} // namespace


namespace pcg {
namespace tonemapper_internal {

// Helper class for the TBB version of the LUT update
template < bool useSRGB >
class ToneMapperLUTBody {

protected:

    // 1/gamma
    const float invGamma;

    // Pointer to the lut
    uint32_t *lutPtr;

    // References to the helper constants
    const Rgba32F &base4;
    const Rgba32F &delta;
    const Rgba32F &qFactor;

    // Constants used in sRGB
    const Rgba32F srgbThreshold; // 0.0031308f
    const Rgba32F srgbLowScale;  // 12.92f
    const Rgba32F srgb1plusA;	 // 1.0f + 0.055f
    const Rgba32F srgbA;		 // 0.055f

    // Helper method for the LUT kernels
    FORCEINLINE_BEG void quantizeAndStore(const int i, 
        const Rgba32F &vals, const Rgba32F &qFactor) const FORCEINLINE_END
    {
        // Multiply by 255
        const Rgba32F values = vals * qFactor;
        
        // Convert back to integers with rounding
        const __m128i valuesI = _mm_cvtps_epi32(values);

        // Copy to the LUT. Each value is only 8 bits long, so we can extract them using SSE2
        const int A = _mm_extract_epi16(valuesI, 3*2);
        const int B = _mm_extract_epi16(valuesI, 2*2);
        const int C = _mm_extract_epi16(valuesI, 1*2);
        const int D = _mm_extract_epi16(valuesI, 0);

        // Combine everything in place. Because of using the previous intrinsics, this code
        // can run entirely on registers, doing a single memory copy
        lutPtr[i] = (D << 24) | (C << 16) | (B << 8) | A;

    }

    // Computes the LUT values, 4 at a time
    inline void kernel(const int &i, 
        const Rgba32F &base4, const Rgba32F &delta, const Rgba32F &qFactor) const {

        // Update values
        Rgba32F values = Rgba32F(static_cast<float>(i))*base4 + delta;

        // Exponentiate by 1/gamma (this is slow!!!)
#if USE_SSE_POW
        const Rgba32F vecInvGamma(invGamma);
        values = ssemath::pow_ps(values, vecInvGamma);
#else
        values.set(pow(values.r(), invGamma), pow(values.g(), invGamma), 
                   pow(values.b(), invGamma), pow(values.a(), invGamma));
#endif

        quantizeAndStore(i, values, qFactor);
    }

    // Computes the LUT for sRGB
    inline void kernelSRGB(const int &i, 
        const Rgba32F &base4, const Rgba32F &delta, const Rgba32F &qFactor) const
    {
        // Update the linear values
        Rgba32F values = Rgba32F((float)(i))*base4 + delta;

        // Gets a maks (filled with 0xffffffff where values[i] < threshold (0.0031308)
        const Rgba32F mask = _mm_cmple_ps(values, srgbThreshold);

        // First case result: when its less than the threshold
        const Rgba32F below = srgbLowScale * values;

        // Second case:
#if USE_SSE_POW
        Rgba32F above(ssemath::exp_ps(static_cast<Rgba32F>(ssemath::log_ps(values))
            * Rgba32F(1.0f/2.4f)));
#else
        Rgba32F above(pow(values.r(), 1.0f/2.4f), pow(values.g(), 1.0f/2.4f),
            pow(values.b(), 1.0f/2.4f), pow(values.a(), 1.0f/2.4f));
#endif
        above *= srgb1plusA;
        above -= srgbA;

        // Select values according to the mask (to avoid conditionals)
        values = (mask & below) | ((Rgba32F) (_mm_andnot_ps(mask, above)));

        
        quantizeAndStore(i, values, qFactor);

    }

public:

    ToneMapperLUTBody(uint32_t *lutPtr, 
        const Rgba32F &_base4, const Rgba32F &_delta, const Rgba32F &_qFactor,
        float invGamma = 0.0f) :
      invGamma(invGamma), lutPtr(lutPtr), base4(_base4), delta(_delta), qFactor(_qFactor),
      srgbThreshold(0.0031308f), srgbLowScale(12.92f), srgb1plusA(1.0f+0.055f), srgbA(0.055f)
    {

    }

    // The actual method to execute. Because useSRGB is a template, the generated
    // code won't have conditionals
    void operator()(const blocked_range<int>& r) const {

        // Local copies of the variables
        const Rgba32F base4   = this->base4;
        const Rgba32F delta   = this->delta;
        const Rgba32F qFactor = this->qFactor;

        if (!useSRGB) {
            for (int i = r.begin(); i != r.end(); ++i) {
                kernel(i, base4, delta, qFactor);
            }
        }
        else {
            for (int i = r.begin(); i != r.end(); ++i) {
                kernelSRGB(i, base4, delta, qFactor);
            }
        }
    }
};

// The class to be used by TBB
template <typename T, ScanLineMode S1, ScanLineMode S2/* = S1*/, 
          bool useLUT/* = true*/, bool isSRGB/* = true */,
          TmoTechnique tmo/* = EXPOSURE*/>
class ApplyToneMap {

    // Pointer to the destination Image
    Image<T,S1> &dest;

    // Pointer to the source image
    const Image<Rgba32F, S2> &src;

    // Pointer to the tone mapper to use
    const ToneMapper &tm;

    // The exponent factor <2^exposure>{4}
    const Rgba32F expF;

    // Helper value to know when to ignore the lut and set the value to zero
    const Rgba32F threshold;

    // Read-only pointer to the tone mapper's LUT
    const unsigned char *lut;


    // This is the real meat of the tone mapping operation. It receives
    // all the inputs as references so that it can be fully inlined
    // by the compiler. For this to work the type T must have a method:
    //   T.set(unsigned char r, unsigned char g, unsigned char b)
    void ToneMapKernel(const Rgba32F &srcPix, T &destPix, const Rgba32F &expF,
        const __m128 &ones, const __m128 &zeros, const __m128 &lutQ) const 
    {
        // Applies the exposure correction 
        // (note that we don't care about the alpha!)
        Rgba32F pix = srcPix * expF;

        // Clamp the values to the interval [0,1]
        pix = _mm_min_ps(ones, _mm_max_ps(zeros, pix));

        // Extract the mask
        const uint32_t mask =
            MOVEMASK_LUT[_mm_movemask_ps(_mm_cmpgt_ps (pix, threshold))];

        // Prepares for querying the LUT: multiplies by the (LUT size-1) and
        // converts to integer with rounding
        pix *= lutQ;
        const __m128i pixIndices = _mm_cvtps_epi32(pix);

        // After this conversion, each value is in the range [0, LUT size), 
        // which by design is a 16 byte unsigned integer. Because of that we
        // don't need SSE4 to extract the elements from there, we can use the
        // SSE2 _mm_extract_epi16 ntrinsic to extract each element
        // independently and query the lut from there (remember, the indices
        // refer to 16-bit integers!)
        const int indexR = _mm_extract_epi16(pixIndices, 3*2);
        const int indexG = _mm_extract_epi16(pixIndices, 2*2);
        const int indexB = _mm_extract_epi16(pixIndices, 1*2);

        // Temporarly copy the values here so that the mask may be applied
        union { unsigned char vec[4]; uint32_t val; } result;
        result.vec[3] = lut[indexR];
        result.vec[2] = lut[indexG];
        result.vec[1] = lut[indexB];
        result.val &= mask;

        // Finally sets the pixel values using the LUT
        destPix.set(result.vec[3], result.vec[2], result.vec[1]);
    }

    // Alternative kernel which doesn't use the LUT. It needs that the pixel
    // type provided an integral typefed "pixel_t"
    void ToneMapKernel_gamma(const Rgba32F &srcPix, T &destPix,
        const Rgba32F &expF, const __m128 &ones, const __m128 &zeros,
        const Rgba32F &qFactor, const float invGamma) const 
    {
        // Applies the exposure correction
        // (note that we don't care about the alpha!)
        Rgba32F pix = srcPix * expF;

        // Clamp the values to the interval [0,1]
        pix = _mm_min_ps(ones, _mm_max_ps(zeros, pix));

        // Hope that this gets vectorized!
        pix.setR(powf(pix.r(), invGamma));
        pix.setG(powf(pix.g(), invGamma));
        pix.setB(powf(pix.b(), invGamma));


        // Multiply the normalized number by the quantization value
        pix *= qFactor;

        // Convert to integral type and store
        const __m128i valuesI = _mm_cvtps_epi32(pix);

        const typename T::pixel_t r = 
            static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 3*2));
        const typename T::pixel_t g =
            static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 2*2));
        const typename T::pixel_t b =
            static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 1*2));
        destPix.set(r, g, b);
    }

    // Version without LUT, for sRGB
    void ToneMapKernel_sRGB(const Rgba32F &srcPix, T &destPix,
        const Rgba32F &expF, const __m128 &ones, const __m128 &zeros,
        const Rgba32F &qFactor) const 
    {
        // Applies the exposure correction
        // (note that we don't care about the alpha!)
        Rgba32F pix = srcPix * expF;

        // Clamp the values to the interval [0,1]
        pix = _mm_min_ps(ones, _mm_max_ps(zeros, pix));

        // Hope that this gets vectorized!
        for (int i = 0; i < 4; ++i) {
            float &v = *((float*)&pix + i);
            v = v > 0.0031308f ? 1.055f*powf(v, 1.0f/2.4f) - 0.055f: 12.92f*v;
        }

        // Multiply the normalized number by the quantization value
        pix *= qFactor;

        // Convert to integral type and store
        const __m128i valuesI = _mm_cvtps_epi32(pix);

        const typename T::pixel_t r =
            static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 3*2));
        const typename T::pixel_t g =
            static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 2*2));
        const typename T::pixel_t b =
            static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 1*2));
        destPix.set(r, g, b);
    }


public:

    // Default constructor: initializes all the pointers and the exposure
    // factor vector
    ApplyToneMap(Image<T,S1> &dest, const Image<Rgba32F, S2> &src, 
        const ToneMapper &tm) :
    dest(dest), src(src), tm(tm), expF(tm.exposureFactor), 
    threshold(isSRGB ? (1.0f/(12.92f*512.0f)) : pow(1.0f/512.0f, tm.Gamma())),
    lut(useLUT ? tm.lut : NULL) {}

    // Linear-style operator. This should only be used on files using the same
    // scanline mode
    void operator()(const blocked_range<int>& r) const {

        // Local copies of the variables
        const __m128 ones  = pcg::ToneMapper::ONES;
        const __m128 zeros = pcg::ToneMapper::ZEROS;
        const __m128 lutQ  = _mm_set1_ps((float)(tm.lutSize-1));
        const Rgba32F expF = this->expF;

        if (lut != NULL) {
            for (int i = r.begin(); i != r.end(); ++i) {
                ToneMapKernel(src[i], dest[i], expF, ones, zeros, lutQ);
            }
        } else {

            const Rgba32F qFactor (static_cast<float>(
                (1<<(sizeof(typename T::pixel_t)<<3))-1));

            if (isSRGB) {
                for (int i = r.begin(); i != r.end(); ++i) {
                    ToneMapKernel_sRGB(src[i], dest[i],
                        expF, ones, zeros, qFactor);
                }
            } else {
                for (int i = r.begin(); i != r.end(); ++i) {
                    ToneMapKernel_gamma(src[i], dest[i],
                        expF, ones, zeros, qFactor, tm.InvGamma());
                }
            }
        }
    }

    // 2D style operator. This guy is safe no mather which scanline mode is used
    void operator()(const blocked_range2d<int>& r) const {

        // Local copies of the variables
        const __m128 ones  = pcg::ToneMapper::ONES;
        const __m128 zeros = pcg::ToneMapper::ZEROS;
        const __m128 lutQ  = _mm_set1_ps((float)(tm.lutSize-1));
        const Rgba32F expF = this->expF;

        if (lut != NULL) {
            for (int j = r.rows().begin(); j != r.rows().end(); ++j) {
                for (int i = r.cols().begin(); i != r.cols().end(); ++i) {
                    ToneMapKernel(src.ElementAt(i,j, dest.GetMode()),
                        dest.ElementAt(i,j), expF, ones, zeros, lutQ);
                }
            }
        } else {

            const Rgba32F qFactor (static_cast<float>(
                (1<<(sizeof(typename T::pixel_t)<<3))-1));

            if (isSRGB) {
                for (int j = r.rows().begin(); j != r.rows().end(); ++j) {
                    for (int i = r.cols().begin(); i != r.cols().end(); ++i) {
                        ToneMapKernel_sRGB(src.ElementAt(i,j, dest.GetMode()),
                            dest.ElementAt(i,j),
                            expF, ones, zeros, qFactor);
                    }
                }
            }
            else {
                for (int j = r.rows().begin(); j != r.rows().end(); ++j) {
                    for (int i = r.cols().begin(); i != r.cols().end(); ++i) {
                        ToneMapKernel_gamma(src.ElementAt(i,j, dest.GetMode()),
                            dest.ElementAt(i,j), expF,
                            ones, zeros, qFactor, tm.InvGamma());
                    }
                }
            }
        }

    }

};



// Specialization of the ApplyToneMap class to handle Reinhard02
//
// The canonical approach is
//   a. Transform sRGB to xyY
//   b. Apply the TMO to Y
//   c. Transform x,y,TMO(Y) back to sRGB
//
// However, having only Y and assuming that TMO(Y) == k*Y, then the
// result of all the transformation is just k*[r,g,b]
// Thus:
//         (key/avgLogLum) * (1 + (key/avgLogLum)/pow(Lwhite,2) * Y)
//    k == ---------------------------------------------------------
//                        1 + (key/avgLogLum)*Y
//
//    k == (P * (R + Q*(P*Y)) / (R + P*Y)
//    P == key / avgLogLum
//    Q == 1 / pow(Lwhite,2)
//    R == 1
// 
template <typename T, ScanLineMode S1, ScanLineMode S2/* = S1*/, 
bool useLUT, bool isSRGB>
class ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>
{
public:
    // Default constructor: initializes all the pointers and the exposure factor vector
    ApplyToneMap(Image<T,S1> &dest, const Image<Rgba32F, S2> &src, 
        const ToneMapper &tm) :
    dest(dest), src(src),
    tm(tm), expF(tm.exposureFactor), lutQ(static_cast<float>(tm.lutSize-1)),
    qFactor(static_cast<float>((1<<(sizeof(typename T::pixel_t)<<3))-1)),
    invGamma(tm.InvGamma()),
    Lwhite2(tm.ParamsReinhard02().l_white * tm.ParamsReinhard02().l_white),
    Lwp(tm.ParamsReinhard02().l_w),
    key(tm.ParamsReinhard02().key),
    partP(key / Lwp),
    partQ(1.0f / Lwhite2),
    partR(1.0f),
    vec_partP(partP), vec_partQ(partQ), vec_partR(partR),
    threshold(isSRGB ? (1.0f/(12.92f*512.0f)) : pow(1.0f/512.0f, tm.Gamma())),
    lut(useLUT ? tm.lut : NULL)
    {
    }

    // Linear-style operator. This should only be used on files using the
    // same scanline mode
    void operator()(const blocked_range<int>& r) const
    {
        const int end_sse = r.begin() + (r.size() & ~0x3);
        assert (end_sse <= r.end());
        for (int i = r.begin(); i < end_sse; i += 4) {
            ToneMapKernel4 (&src[i], &dest[i]);
        }

        for (int i = end_sse; i < r.end(); ++i) {
            ToneMapKernel (src[i], dest[i]);
        }
    }

    // 2D style operator. This guy is safe no mather which scanline mode is used
    void operator()(const blocked_range2d<int>& r) const
    {
        const int end_sse = r.cols().begin() + (r.cols().size() & ~0x3);
        assert (end_sse <= r.cols().end());
        for (int j = r.rows().begin(); j != r.rows().end(); ++j) {
            const Rgba32F * srcPtr = src.GetScanlinePointer(j, dest.GetMode());
            T * destPtr = dest.GetScanlinePointer(j);
            for (int i = r.cols().begin(); i != end_sse; i += 4) {
                ToneMapKernel4 (srcPtr+i, destPtr+i);
            }

            for (int i = end_sse; i < r.cols().end(); ++i) {
                ToneMapKernel (srcPtr[i], destPtr[i]);
            }
        }
    }


private:

    FORCEINLINE_BEG Rgba32F reinhard02(const Rgba32F &pix) const FORCEINLINE_END
    {
        const float Lw = dot_float(pix, vec_LUM);
        const float Lp = partP * Lw;
        const float Ls = (partP * (partR + partQ*Lp)) / (partR + Lp);

        const Rgba32F res = pix * Ls;
        return res;
    }

    // After the pixel is properly mapped into the LDR range (ie after exposure
    // or applying any curve) converts into the LDR type
    inline void PixelToLDR(const Rgba32F &src, T &dest) const
    {
        // Clamp the values to the interval [0,1]
        Rgba32F pix = _mm_min_ps(ToneMapper::ONES, 
            _mm_max_ps(ToneMapper::ZEROS, src));

        if (useLUT) {

            // Extract the mask
            const uint32_t mask =
                MOVEMASK_LUT[_mm_movemask_ps(_mm_cmpgt_ps (pix, threshold))];

            // Prepares for querying the LUT: multiplies by the (LUT size-1) and
            // converts to integer with rounding
            pix *= lutQ;
            const __m128i pixIndices = _mm_cvtps_epi32(pix);

            // After this conversion, each value is in the range [0, LUT size),
            // which by design is a 16 byte unsigned integer. Because of that we
            // don't need SSE4 to extract the elements from there, we can use the
            // SSE2 _mm_extract_epi16 intrinsic to extract each element
            // independently and query the lut from there (remember, the indices
            // refer to 16-bit integers!)
            const int indexR = _mm_extract_epi16(pixIndices, 3*2);
            const int indexG = _mm_extract_epi16(pixIndices, 2*2);
            const int indexB = _mm_extract_epi16(pixIndices, 1*2);

            // Temporarly copy the values here so that the mask may be applied
            union { unsigned char vec[4]; uint32_t val; } result;
            result.vec[3] = lut[indexR];
            result.vec[2] = lut[indexG];
            result.vec[1] = lut[indexB];
            result.val &= mask;

            // Finally sets the pixel values using the LUT
            dest.set(result.vec[3], result.vec[2], result.vec[1]);
        } 
        else {
            if (isSRGB) {
                
                // Hope that this gets vectorized!
                for (int i = 0; i < 4; ++i) {
                    float &v = *((float*)&pix + i);
                    v = v > 0.0031308f ?
                        1.055f*powf(v, 1.0f/2.4f) - 0.055f: 12.92f*v;
                }

                // Multiply the normalized number by the quantization value
                pix *= qFactor;

                // Convert to integral type and store
                const __m128i valuesI = _mm_cvtps_epi32(pix);

                const typename T::pixel_t r =
                    static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 3*2));
                const typename T::pixel_t g =
                    static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 2*2));
                const typename T::pixel_t b =
                    static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 1*2));
                dest.set(r, g, b);
            } 
            else {

                // Hope that this gets vectorized!
                pix.setR(powf(pix.r(), invGamma));
                pix.setG(powf(pix.g(), invGamma));
                pix.setB(powf(pix.b(), invGamma));


                // Multiply the normalized number by the quantization value
                pix *= qFactor;

                // Convert to integral type and store
                const __m128i valuesI = _mm_cvtps_epi32(pix);

                const typename T::pixel_t r =
                    static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 3*2));
                const typename T::pixel_t g =
                    static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 2*2));
                const typename T::pixel_t b =
                    static_cast<typename T::pixel_t> (_mm_extract_epi16(valuesI, 1*2));
                dest.set(r, g, b);
            }
        }
    }

    // For this to work the type T must have a method:
    //   T.set(unsigned char r, unsigned char g, unsigned char b)
    // Assumes that the compiler will use the templates to inline and
    // generate decent code
    inline void ToneMapKernel(const Rgba32F &srcPix, T &destPix) const 
    {
        // Applies the exposure correction (note that we don't care about the alpha!)
        Rgba32F pix = srcPix * expF;

        // Does the actual Reinhard02 mapping for a single pixel
        pix = reinhard02(pix);

        PixelToLDR (pix, destPix);
    }


    // Kernel which handled groups of 4 contiguous pixels
    inline void ToneMapKernel4(const Rgba32F * PCG_RESTRICT src, 
        T * PCG_RESTRICT dest) const 
    {
         // Load the next 4 pixels and transpose them (after expore)
        Rgba32F pix0 = src[0] * expF;
        Rgba32F pix1 = src[1] * expF;
        Rgba32F pix2 = src[2] * expF;
        Rgba32F pix3 = src[3] * expF;
        Rgba32F p0 = pix0, p1 = pix1, p2 = pix2, p3 = pix3;
        PCG_MM_TRANSPOSE4_PS (p0, p1, p2, p3);

        // Now do the scaling (recall the Rgba32F offsets: a=0, r=3)
        const Rgba32F vec_Lw = (vec_LUM_R*p3) + (vec_LUM_G*p2) + (vec_LUM_B*p1);

        // Apply the Reinhard02 scalling on the vector
        const Rgba32F vec_Lp = vec_partP * vec_Lw;
        const Rgba32F vec_Ls = (vec_partP * (vec_partR + vec_partQ * vec_Lp)) /
                               (vec_partR + vec_Lp);

        // Scale each pixel by the appropriate value
        const Rgba32F Ls0 = broadcast_idx<0> (vec_Ls);
        const Rgba32F Ls1 = broadcast_idx<1> (vec_Ls);
        const Rgba32F Ls2 = broadcast_idx<2> (vec_Ls);
        const Rgba32F Ls3 = broadcast_idx<3> (vec_Ls);

        pix0 *= Ls0;
        pix1 *= Ls1;
        pix2 *= Ls2;
        pix3 *= Ls3;

        // Finish each pixel individually
        PixelToLDR (pix0, dest[0]);
        PixelToLDR (pix1, dest[1]);
        PixelToLDR (pix2, dest[2]);
        PixelToLDR (pix3, dest[3]);
    }



    // Reference to the destination Image
    Image<T,S1> &dest;

    // Reference to the source image
    const Image<Rgba32F, S2> &src;

    // Reference to the tone mapper to use
    const ToneMapper &tm;

    // The exponent factor <2^exposure>{4}
    const Rgba32F expF;

    // Quantization factor for the LUT (lutsize-1)
    const Rgba32F lutQ;

    const Rgba32F qFactor;

    const float invGamma;

    // White point squared
    const float Lwhite2;

    // Average log-intensity
    const float Lwp;

    // Key
    const float key;

    // Helper constant parts of the computations
    const float partP;
    const float partQ;
    const float partR;

    // Vector version of the helper constants
    const Rgba32F vec_partP;
    const Rgba32F vec_partQ;
    const Rgba32F vec_partR;

    // Helper value to know when to ignore the lut and set the value to zero
    const Rgba32F threshold;

    // Read-only pointer to the tone mapper's LUT
    const unsigned char *lut;

    // Helper Constants
    static const float LUM_R;
    static const float LUM_G;
    static const float LUM_B;

    static const Rgba32F vec_LUM_R;
    static const Rgba32F vec_LUM_G;
    static const Rgba32F vec_LUM_B;
    static const Rgba32F vec_LUM;
};

// Initialize the class constants
template <typename T, ScanLineMode S1, ScanLineMode S2, bool useLUT, bool isSRGB>
const float ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>::LUM_R = 0.212639005871510f;

template <typename T, ScanLineMode S1, ScanLineMode S2, bool useLUT, bool isSRGB>
const float ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>::LUM_G = 0.715168678767756f;

template <typename T, ScanLineMode S1, ScanLineMode S2, bool useLUT, bool isSRGB>
const float ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>::LUM_B = 0.072192315360734f;

template <typename T, ScanLineMode S1, ScanLineMode S2, bool useLUT, bool isSRGB>
const Rgba32F
ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>::vec_LUM_R(LUM_R);

template <typename T, ScanLineMode S1, ScanLineMode S2, bool useLUT, bool isSRGB>
const Rgba32F
ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>::vec_LUM_G(LUM_G);

template <typename T, ScanLineMode S1, ScanLineMode S2, bool useLUT, bool isSRGB>
const Rgba32F
ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>::vec_LUM_B(LUM_B);

template <typename T, ScanLineMode S1, ScanLineMode S2, bool useLUT, bool isSRGB>
const Rgba32F
ApplyToneMap<T, S1, S2, useLUT, isSRGB, REINHARD02>::vec_LUM(LUM_R, LUM_G, LUM_B, 0.0f);



// Helper method to instanciate the appropriate instance of the
// ApplyTonemaper
template <bool useLUT, bool isSRGB, 
    typename T, ScanLineMode M1, ScanLineMode M2, typename R>
void ToneMapHelper(Image<T, M1> &dest, const Image<Rgba32F, M2> &src, 
    const ToneMapper &tm, TmoTechnique technique, 
    const R &range)
{
    if (dest.Width() != src.Width() || dest.Height() != dest.Height()) {
        throw IllegalArgumentException("The images dimensions' "
            "don't match");
    }

    const auto_partitioner partitioner;
    switch (technique) {
    case REINHARD02:
        parallel_for(range,
            ApplyToneMap<T,M1,M2,useLUT,isSRGB,REINHARD02>(dest,src,tm),
            partitioner);
        break;
    default:
        parallel_for(range, 
            ApplyToneMap<T,M1,M2,useLUT,isSRGB>(dest,src,tm),
            partitioner);
    }
}

template <typename T, ScanLineMode M1, ScanLineMode M2, typename R>
void ToneMapHelper(Image<T, M1> &dest, const Image<Rgba32F, M2> &src, 
    const ToneMapper &tm, bool useLut, TmoTechnique technique,
    const R &range)
{
    if (useLut) {
        if (tm.isSRGB())
            ToneMapHelper<true,true>(dest, src, tm, technique, range);
        else
            ToneMapHelper<true,false>(dest, src, tm, technique, range);
    } else {
        if (tm.isSRGB())
            ToneMapHelper<false,true>(dest, src, tm, technique, range);
        else
            ToneMapHelper<false,false>(dest, src, tm, technique, range);
    }
}


// Tones map the given source image and stores the result in the given
// destination image. The images must have the same size!
// This template gets instanciated when both images have the same scan order
template<class T, ScanLineMode M>
void ToneMap(Image<T, M> &dest, const Image<Rgba32F, M> &src, 
    const ToneMapper &tm, bool useLut, TmoTechnique technique) 
{
    const int numPixels = src.Size();
    const blocked_range<int> range = blocked_range<int>(0,numPixels,4);
    ToneMapHelper(dest, src, tm, useLut, technique, range);
}

// The method to use when whe scanline order is different
template<class T, ScanLineMode M1, ScanLineMode M2>
void ToneMap(Image<T, M1> &dest, const Image<Rgba32F, M2> &src, 
    const ToneMapper &tm, bool useLut, TmoTechnique technique) 
{
    const blocked_range2d<int> range = 
        blocked_range2d<int>(0,src.Height(),1, 0,src.Width(),4);
    ToneMapHelper(dest, src, tm, useLut, technique, range);
}

} // namespace tonemapper_internal
} // namespace pcg

using namespace pcg::tonemapper_internal;


void ToneMapper::SetExposure(float exposure) {
    this->exposure = exposure;
    this->exposureFactor = exp2f(exposure);
}


void ToneMapper::UpdateLUT() {

    const float stepSize = 1.0f/lutSize;
    const float halfStep = 0.5f/lutSize;

    // At each group i, we wil fill the LUT for (i*base+delta), i = 0,4,8,...
    const Rgba32F base4(4.0f * stepSize);
    const Rgba32F delta(halfStep,              halfStep + stepSize,
                        halfStep + 2*stepSize, halfStep + 3*stepSize);
    const Rgba32F qFactor(255.0f);

    // With some SSE trickery we will be running in 4 elements at a time,
    // that's the reason for the LUT to be a multiple of 4
    const int numIter = lutSize >> 2;
    // Alias the LUT: we will write quadwords at a time
    uint32_t *lutPtr = reinterpret_cast<uint32_t *>(lut);

    // Instanciate the proper templates
    if (isSRGB()) {
        parallel_for(blocked_range<int>(0,numIter),
            ToneMapperLUTBody<true>(lutPtr, base4, delta, qFactor));
    }
    else {
        parallel_for(blocked_range<int>(0,numIter),
            ToneMapperLUTBody<false>(lutPtr, base4, delta, qFactor, invGamma));
    }
}



int ToneMapper::MaxLUTError() const
{
    const bool srgb = isSRGB();
    int res = 0;

    for (int i = 0, low = 0; i < lutSize; ++i) {
        float x = static_cast<float>(i+1) / static_cast<float>(lutSize);
        if (srgb) {
            x = (x>0.0031308f) ? (1.055f)*powf(x, 1.0f/2.4f)-0.055f : 12.92f*x;
        } else {
            x = pow(x, invGamma);
        }
        const int hi = static_cast<int> (255.0f * x);
        const int lutVal = lut[i];
        res = std::max(res, std::max (abs(hi-lutVal), abs(low-lutVal)));

        // Prepare for the next iteration
        low = hi;
    }
    return res;
}




// #################################################
// # Instanciate the templates to get the real stuff
// #################################################

// Bgra8 pixels
void ToneMapper::ToneMap(Image<Bgra8, TopDown> &dest,
                         const Image<Rgba32F, TopDown> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}
void ToneMapper::ToneMap(Image<Bgra8, TopDown> &dest,
                         const Image<Rgba32F, BottomUp> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}
void ToneMapper::ToneMap(Image<Bgra8, BottomUp> &dest,
                         const Image<Rgba32F, TopDown> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}
void ToneMapper::ToneMap(Image<Bgra8, BottomUp> &dest,
                         const Image<Rgba32F, BottomUp> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}

// Rgba8 pixels
void ToneMapper::ToneMap(Image<Rgba8, TopDown> &dest,
                         const Image<Rgba32F, TopDown> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}
void ToneMapper::ToneMap(Image<Rgba8, TopDown> &dest,
                         const Image<Rgba32F, BottomUp> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}
void ToneMapper::ToneMap(Image<Rgba8, BottomUp> &dest,
                         const Image<Rgba32F, TopDown> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}
void ToneMapper::ToneMap(Image<Rgba8, BottomUp> &dest,
                         const Image<Rgba32F, BottomUp> &src,
                         bool useLut, TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, useLut, technique);
}

// Rgba16 pixels
void ToneMapper::ToneMap(Image<Rgba16, TopDown> &dest,
                         const Image<Rgba32F, TopDown> &src,
                         TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, false, technique);
}
void ToneMapper::ToneMap(Image<Rgba16, TopDown> &dest,
                         const Image<Rgba32F, BottomUp> &src,
                         TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, false, technique);
}
void ToneMapper::ToneMap(Image<Rgba16, BottomUp> &dest,
                         const Image<Rgba32F, TopDown> &src,
                         TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, false, technique);
}
void ToneMapper::ToneMap(Image<Rgba16, BottomUp> &dest,
                         const Image<Rgba32F, BottomUp> &src,
                         TmoTechnique technique) const {
    pcg::tonemapper_internal::ToneMap(dest, src, *this, false, technique);
}
