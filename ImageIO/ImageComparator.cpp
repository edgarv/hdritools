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

#include "ImageComparator.h"
#include "Exception.h"
#include "ImageIterators.h"
#if !PCG_USE_AVX
# include "Vec4f.h"
# include "Vec4i.h"
#else
# include "Vec8f.h"
# include "Vec8i.h"
#endif


// Intel Threading Buiding Blocks
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

using namespace pcg;
using namespace tbb;

// SSE3 functions are only available as intrinsic in older versions of MSVC
#if defined(_MSC_VER) && _MSC_VER < 1500 && !defined(__INTEL_COMPILER)
#include <intrin.h>
#pragma intrinsic ( _mm_hadd_ps )
#else
#include <pmmintrin.h>
#endif // _MSC_VER


// When enabled the SoA comparisons use approximation to the reciprocal and the
// square root which provide about 18 bits of mantissa accuracy
#define FAST_COMPARE 1



namespace
{

// A class that we will use for the TBB implementation
template <ScanLineMode S>
class Comparator {
            
private:
    const Image<Rgba32F, S> &src1;
    const Image<Rgba32F, S> &src2;
    Image<Rgba32F, S> &dest;

    const ImageComparator::Type type;

    // The different kernels, one for each comparison type

    // Takes the absolute value of the difference of the pixels
    inline void kernel_absoluteDiff(const int &i) const {

        dest[i] = Rgba32F::abs(src1[i] - src2[i]);
    }

    // Not actually a comparision but a combination: just adds both pixels
    inline void kernel_addition(const int &i) const {
        dest[i] = src1[i] + src2[i];
    }

    // Divides the first source by the second one
    inline void kernel_division(const int &i) const {
        // TODO what if both are zero? what if src2[i] is almost zero?
        dest[i] = src1[i] / src2[i];
    }

    // Error relative to the adition of both images
    inline void kernel_relError(const int &i) const {
        dest[i] = Rgba32F(2.0f) * Rgba32F::abs(src1[i] - src2[i]) / (src1[i] + src2[i]);
    }

    // Helper method to add the four elements of a vector and have the result in each section
    FORCEINLINE_BEG __m128 hadd(const __m128 &n) const FORCEINLINE_END {
        // FIXME Use the non-SSE3 equivalent of hadd_ps
        const __m128 r = _mm_hadd_ps(n,n);
        return _mm_hadd_ps(r,r);
    }


    // Helper function for the 2-norm comparisons
    FORCEINLINE_BEG void kernel_2norm(const Rgba32F &val, const int &i, 
        const Rgba32F &alphaKillMask) const FORCEINLINE_END
    {	
        const Rgba32F v = val & alphaKillMask;

        // Use SSE3 to get the horizontal add
        Rgba32F dSqr = v*v;
        dSqr = hadd(dSqr);

        // TODO Use all square roots and then masks, or a single root and shuffle?
        const Rgba32F d = _mm_sqrt_ps(dSqr);

        const Rgba32F pn = ((Rgba32F) hadd(v)) & alphaKillMask;
        const Rgba32F zero = _mm_setzero_ps();

        // Part 1: pn < 0 (just r)
        Rgba32F m1 = _mm_cmplt_ps(pn, zero);
        m1 = _mm_shuffle_ps((__m128)m1, (__m128)m1, _MM_SHUFFLE(3, 0, 0, 0));

        // Part 2: pn > 0 (just g)
        Rgba32F m2 = _mm_cmpgt_ps(pn, zero);
        m2 = _mm_shuffle_ps((__m128)m2, (__m128)m2, _MM_SHUFFLE(0, 2, 0, 0));

        // Part 3: pn == 0 (just b)
        Rgba32F m3 = _mm_cmpeq_ps(pn, zero);
        m3 = _mm_shuffle_ps((__m128)m3, (__m128)zero, _MM_SHUFFLE(0, 0, 1, 0));

        // And finally combine everything
        // TODO: do something useful with the alpha
        dest[i] = d & (m1 | m2 | m3);
    }

    // 2-norm of the rgb diference
    inline void kernel_posNeg(const int &i, const Rgba32F &alphaKillMask) const {

        const Rgba32F delta = (src1[i] - src2[i]);
        kernel_2norm(delta, i, alphaKillMask);
    }

    inline void kernel_posNegRel(const int &i, const Rgba32F &alphaKillMask) const {

        const Rgba32F diff = Rgba32F(2.0f) * (src1[i] - src2[i]) / (src1[i] + src2[i]);
        kernel_2norm(diff, i, alphaKillMask);
    }

public:
    Comparator(ImageComparator::Type type, Image<Rgba32F, S> &dest, 
        const Image<Rgba32F, S> &src1, const Image<Rgba32F, S> &src2) :
        src1(src1), src2(src2), dest(dest), type(type) {}

    // Linear-style operator (one pixel after the other)
    void operator()(const blocked_range<int>& r) const {
        const __m128i alphaKillInt = _mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x0);
        const Rgba32F alphaKill = _mm_castsi128_ps(alphaKillInt);

        // TODO Use templates to remove the conditional at compile-time
        switch (type) {
            case ImageComparator::AbsoluteDifference:
                for (int i = r.begin(); i != r.end(); ++i) {
                    kernel_absoluteDiff(i);
                }
                break;

            case ImageComparator::Addition:
                for (int i = r.begin(); i != r.end(); ++i) {
                    kernel_addition(i);
                }
                break;

            case ImageComparator::Division:
                for (int i = r.begin(); i != r.end(); ++i) {
                    kernel_division(i);
                }
                break;

            case ImageComparator::RelativeError:
                for (int i = r.begin(); i != r.end(); ++i) {
                    kernel_relError(i);
                }
                break;

            case ImageComparator::PositiveNegative:
                for (int i = r.begin(); i != r.end(); ++i) {
                    kernel_posNeg(i, alphaKill);
                }
                break;

            case ImageComparator::PositiveNegativeRelativeError:
                for (int i = r.begin(); i != r.end(); ++i) {
                    kernel_posNegRel(i, alphaKill);
                }
                break;


            default:
                assert(0);
                break;
        }
    }
};



// Comparator implementation for SoA Images
class ComparatorSoA
{
#if !PCG_USE_AVX
    typedef RGBA32FVec4ImageSoAIterator IteratorSoA;
    typedef Vec4f vf;
    typedef Vec4i vi;

    static FORCEINLINE_BEG vf castAsFloat(const vi& a) FORCEINLINE_END {
        return _mm_castsi128_ps(a);
    }

#if FAST_COMPARE
    static FORCEINLINE_BEG vf simd_rsqrt(const vf& a) FORCEINLINE_END {
        return _mm_rsqrt_ps(a);
    }
#else
    static FORCEINLINE_BEG vf simd_sqrt(const vf& a) FORCEINLINE_END {
        return _mm_sqrt_ps(a);
    }
#endif

#else
    typedef RGBA32FVec8ImageSoAIterator IteratorSoA;
    typedef Vec8f vf;
    typedef Vec8i vi;

    static FORCEINLINE_BEG vf castAsFloat(const vi& a) FORCEINLINE_END {
        return _mm256_castsi256_ps(a);
    }

#if FAST_COMPARE
    static FORCEINLINE_BEG vf simd_rsqrt(const vf& a) FORCEINLINE_END {
        return _mm256_rsqrt_ps(a);
    }
#else
    static FORCEINLINE_BEG vf simd_sqrt(const vf& a) FORCEINLINE_END {
        return _mm256_sqrt_ps(a);
    }

#endif

#endif
    typedef IteratorSoA::difference_type diff_t;

    const ImageComparator::Type type;
    const IteratorSoA destBegin;
    const IteratorSoA src1Begin;
    const IteratorSoA src2Begin;

    static FORCEINLINE_BEG vf absDiff(const vf& a, const vf& b) FORCEINLINE_END{
        const vf mask(castAsFloat(vi::constant<0x7fffffff>()));
        return mask & (a - b);
    }

    static FORCEINLINE_BEG vf
    norm2(const vf& a, const vf& b, const vf& c) FORCEINLINE_END {
        const vf n2 = a*a + b*b + c*c;

#if FAST_COMPARE
        // From Eigen 3.0 (MathFunctions.h)
        // This is based on Quake3's fast inverse square root.
        // For detail see here: http://www.beyond3d.com/content/articles/8/
        const vf negHalf = n2 * vf(-0.5f);
        
        // Select only the inverse sqrt of non-zero inputs (using FLT_EPSILON)
        const vf non_zero_mask(n2 > vf(1.192092896e-07f));
        vf x = non_zero_mask & simd_rsqrt(n2);
        
        x = x * (vf(1.5f) + (negHalf * (x * x)));
        // at this point x == 1/sqrt(n2)
        x *= n2;
        return x;
#else
        vf x = simd_sqrt(n2);
        return x;
#endif
    }



    void AbsoluteDifference(diff_t begin, diff_t end) const
    {
        IteratorSoA src1 = src1Begin + begin;
        IteratorSoA src2 = src2Begin + begin;
        IteratorSoA dest = destBegin + begin;
        for (diff_t i = begin; i != end; ++i, ++src1, ++src2, ++dest) {
            dest->r() = absDiff(src1->r(), src2->r());
            dest->g() = absDiff(src1->g(), src2->g());
            dest->b() = absDiff(src1->b(), src2->b());
            dest->a() = absDiff(src1->a(), src2->a());
        }
    }

    void Addition(diff_t begin, diff_t end) const
    {
        IteratorSoA src1 = src1Begin + begin;
        IteratorSoA src2 = src2Begin + begin;
        IteratorSoA dest = destBegin + begin;
        for (diff_t i = begin; i != end; ++i, ++src1, ++src2, ++dest) {
            dest->r() = vf(src1->r()) + vf(src2->r());
            dest->g() = vf(src1->g()) + vf(src2->g());
            dest->b() = vf(src1->b()) + vf(src2->b());
            dest->a() = vf(src1->a()) + vf(src2->a());
        }
    }

    void Division(diff_t begin, diff_t end) const
    {
        IteratorSoA src1 = src1Begin + begin;
        IteratorSoA src2 = src2Begin + begin;
        IteratorSoA dest = destBegin + begin;
        for (diff_t i = begin; i != end; ++i, ++src1, ++src2, ++dest) {
#if FAST_COMPARE
            dest->r() = vf(src1->r()) * rcp_nr(vf(src2->r()));
            dest->g() = vf(src1->g()) * rcp_nr(vf(src2->g()));
            dest->b() = vf(src1->b()) * rcp_nr(vf(src2->b()));
            dest->a() = vf(src1->a()) * rcp_nr(vf(src2->a()));
#else
            dest->r() = vf(src1->r()) / (vf(src2->r()));
            dest->g() = vf(src1->g()) / (vf(src2->g()));
            dest->b() = vf(src1->b()) / (vf(src2->b()));
            dest->a() = vf(src1->a()) / (vf(src2->a()));
#endif
        }
    }
    
    void RelativeError(diff_t begin, diff_t end) const
    {
        IteratorSoA src1 = src1Begin + begin;
        IteratorSoA src2 = src2Begin + begin;
        IteratorSoA dest = destBegin + begin;

        const vf const_2(2.0f);
        for (diff_t i = begin; i != end; ++i, ++src1, ++src2, ++dest) {
#if FAST_COMPARE
            dest->r() = const_2 * absDiff(src1->r(), src2->r()) *
                rcp_nr(vf(src1->r()) + vf(src2->r()));
            dest->g() = const_2 * absDiff(src1->g(), src2->g()) *
                rcp_nr(vf(src1->g()) + vf(src2->g()));
            dest->b() = const_2 * absDiff(src1->b(), src2->b()) *
                rcp_nr(vf(src1->b()) + vf(src2->b()));
            dest->a() = const_2 * absDiff(src1->a(), src2->a()) *
                rcp_nr(vf(src1->a()) + vf(src2->a()));
#else
            dest->r() = const_2 * absDiff(src1->r(), src2->r()) /
                (vf(src1->r()) + vf(src2->r()));
            dest->g() = const_2 * absDiff(src1->g(), src2->g()) /
                (vf(src1->g()) + vf(src2->g()));
            dest->b() = const_2 * absDiff(src1->b(), src2->b()) /
                (vf(src1->b()) + vf(src2->b()));
            dest->a() = const_2 * absDiff(src1->a(), src2->a()) /
                (vf(src1->a()) + vf(src2->a()));
#endif
        }
    }

    void PositiveNegative(diff_t begin, diff_t end) const
    {
        IteratorSoA src1 = src1Begin + begin;
        IteratorSoA src2 = src2Begin + begin;
        IteratorSoA dest = destBegin + begin;
        for (diff_t i = begin; i != end; ++i, ++src1, ++src2, ++dest) {
            const vf dr = vf(src1->r()) - vf(src2->r());
            const vf dg = vf(src1->g()) - vf(src2->g());
            const vf db = vf(src1->b()) - vf(src2->b());
            const vf norm = norm2(dr, dg, db);
            const vf pn = dr + dg + db;

            dest->r() = norm & vf(pn < vf::zero());
            dest->g() = norm & vf(pn > vf::zero());
            dest->b() = norm & vf(pn == vf::zero());
            dest->a() = norm;
        }
    }

    void PositiveNegativeRelativeError(diff_t begin, diff_t end) const
    {
        IteratorSoA src1 = src1Begin + begin;
        IteratorSoA src2 = src2Begin + begin;
        IteratorSoA dest = destBegin + begin;

        const vf const_2(2.0f);
        for (diff_t i = begin; i != end; ++i, ++src1, ++src2, ++dest) {
#if FAST_COMPARE
            const vf dr = const_2 * (vf(src1->r()) - vf(src2->r())) *
                rcp_nr(vf(src1->r()) + vf(src2->r()));
            const vf dg = const_2 * (vf(src1->g()) - vf(src2->g())) *
                rcp_nr(vf(src1->g()) + vf(src2->g()));
            const vf db = const_2 * (vf(src1->b()) - vf(src2->b())) *
                rcp_nr(vf(src1->b()) + vf(src2->b()));
#else
            const vf dr = const_2 * (vf(src1->r()) - vf(src2->r())) /
                (vf(src1->r()) + vf(src2->r()));
            const vf dg = const_2 * (vf(src1->g()) - vf(src2->g())) /
                (vf(src1->g()) + vf(src2->g()));
            const vf db = const_2 * (vf(src1->b()) - vf(src2->b())) /
                (vf(src1->b()) + vf(src2->b()));
#endif
            const vf norm = norm2(dr, dg, db);
            const vf pn = dr + dg + db;

            dest->r() = norm & vf(pn < vf::zero());
            dest->g() = norm & vf(pn > vf::zero());
            dest->b() = norm & vf(pn == vf::zero());
            dest->a() = norm;
        }
    }


public:
    ComparatorSoA(ImageComparator::Type cmpType, RGBAImageSoA& dest, 
        const RGBAImageSoA& src1, const RGBAImageSoA& src2) :
    type(cmpType), destBegin(IteratorSoA::begin(dest)),
    src1Begin(IteratorSoA::begin(src1)), src2Begin(IteratorSoA::begin(src2))
    {}

    // Linear-style operator (one pixel after the other)
    void operator()(const blocked_range<diff_t>& r) const {

        switch (type) {
            case ImageComparator::AbsoluteDifference:
                AbsoluteDifference(r.begin(), r.end());
                break;
            case ImageComparator::Addition:
                Addition(r.begin(), r.end());
                break;
            case ImageComparator::Division:
                Division(r.begin(), r.end());
                break;
            case ImageComparator::RelativeError:
                RelativeError(r.begin(), r.end());
                break;
            case ImageComparator::PositiveNegative:
                PositiveNegative(r.begin(), r.end());
                break;
            case ImageComparator::PositiveNegativeRelativeError:
                PositiveNegativeRelativeError(r.begin(), r.end());
                break;

            default:
                assert(0);
                break;
        }
    }

};

} // namespace



template <ScanLineMode S>
void ImageComparator::CompareHelper(Type type, Image<Rgba32F, S> &dest, 
            const Image<Rgba32F, S> &src1, const Image<Rgba32F, S> &src2)
{
    // First check that the images have the save size, otherwise it will throw
    // a nasty exception
    if (dest.Width() != src1.Width() || dest.Height() != src1.Height() ||
        src1.Width() != src2.Width() || src1.Height() != src2.Height() )
    {
        throw IllegalArgumentException("Incompatible images size");
    }

    // And launch the parallel for
    const int numPixels = dest.Size();
    parallel_for(blocked_range<int>(0, numPixels, 4),
        Comparator<S>(type, dest, src1, src2));
}

// The real instances of the template
void ImageComparator::Compare(Type type, Image<Rgba32F, TopDown> &dest, 
            const Image<Rgba32F, TopDown> &src1, const Image<Rgba32F, TopDown> &src2) {
    CompareHelper(type, dest, src1, src2);
}
void ImageComparator::Compare(Type type, Image<Rgba32F, BottomUp> &dest, 
            const Image<Rgba32F, BottomUp> &src1, const Image<Rgba32F, BottomUp> &src2) {
    CompareHelper(type, dest, src1, src2);
}


void ImageComparator::Compare(Type type, RGBAImageSoA &dest,
            const RGBAImageSoA &src1, const RGBAImageSoA &src2)
{
    // First check that the images have the save size, otherwise it will throw
    // a nasty exception
    if (dest.Width() != src1.Width() || dest.Height() != src1.Height() ||
        src1.Width() != src2.Width() || src1.Height() != src2.Height() )
    {
        throw IllegalArgumentException("Incompatible images size");
    }

#if !PCG_USE_AVX
    typedef RGBA32FVec4ImageSoAIterator IteratorSoA;
#else
    typedef RGBA32FVec8ImageSoAIterator IteratorSoA;
#endif

    typedef IteratorSoA::difference_type diff_t;
    const diff_t count = IteratorSoA::end(src1) - IteratorSoA::begin(src1);
    const blocked_range<diff_t> range(0, count, 4);
    parallel_for(range, ComparatorSoA(type, dest, src1, src2));
}
