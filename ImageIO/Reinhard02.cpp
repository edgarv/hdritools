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

// Global implementation of the Reinhard02 tone mapper:
// Reinhard, E., Stark, M., Shirley, P., Ferwerda, J.
// "Photographic tone reproduction for digital images", ACM SIGGRAPH 2002
// http://doi.acm.org/10.1145/566570.566575
//
// The automatic parameter selection follows the paper
// "Parameter estimation for photographic tone reproduction" by
// Erik Reinhard, Journal of Graphics Tools Volume 7, Issue 1 (Nov 2002)
// http://www.cs.bris.ac.uk/~reinhard/papers/jgt_reinhard.pdf

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# if !defined(_MSC_VER)
#  include <cmath>
# endif
#endif

#include "Reinhard02.h"
#include "ImageIterators.h"
#include "Vec4f.h"
#include "Vec4i.h"
#if PCG_USE_AVX
# include "Vec8f.h"
# include "Vec8i.h"
#endif

#include <limits>
#include <vector>
#include <memory>

#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>


// Flag to use Intel's fast log routine. Very fast but has a terrible accuracy,
// yet makes the whole process run about 4x faster (in MSVC++ 2008)
#define USE_AM_LOG 0

#if USE_AM_LOG
#include "Amaths.h"
#else
namespace ssemath {
#include "sse_mathfun.h"
}
#endif



#if !defined(_WIN32) || !defined(__INTEL_COMPILER)
# include <cmath>
# if defined (_MSC_VER)

namespace {

// Add some required C99 functions
inline float exp2f(float x) {
    return powf(2.0f, x);
}

inline float fminf(const float& x, const float& y) {
    __m128 tmp = _mm_min_ss(_mm_load_ss(&x), _mm_load_ss(&y));
    return _mm_cvtss_f32(tmp);
}
inline float fmaxf(const float& x, const float& y) {
    __m128 tmp = _mm_max_ss(_mm_load_ss(&x), _mm_load_ss(&y));
    return _mm_cvtss_f32(tmp);
}

} // namespace

# endif
#endif

// SSE3 functions are only available as intrinsic in older versions of MSVC
#if defined(_MSC_VER) && _MSC_VER < 1500 && !defined(__INTEL_COMPILER)
#include <intrin.h>
#pragma intrinsic ( _mm_hadd_ps )
#else
#include <pmmintrin.h>
#endif // _MSC_VER


using namespace pcg;

namespace
{

typedef std::numeric_limits<float> float_limits;



// Constants used by different functors, so that for templated classes they
// are not instantiated multiple times
namespace constants
{

// Writing manually the same constant many times is error prone
#if PCG_USE_AVX
#define PCG_VEC_UNION(x) {x, x, x, x, x, x, x, x}
typedef pcg::Vec8fUnion VecfUnion;
typedef pcg::Vec8iUnion VeciUnion;
#else
#define PCG_VEC_UNION(x) {x, x, x, x}
typedef pcg::Vec4fUnion VecfUnion;
typedef pcg::Vec4iUnion VeciUnion;
#endif

// Helper function to set either a scalar or a float from a Vec4fUnion
template <typename T, class VecUnionType>
inline const T& get(const VecUnionType& value) {
    return *reinterpret_cast<const T*>(&value);
}


static const VecfUnion LUM_R = {PCG_VEC_UNION( 0.27f )};
static const VecfUnion LUM_G = {PCG_VEC_UNION( 0.67f )};
static const VecfUnion LUM_B = {PCG_VEC_UNION( 0.06f )};
static const VecfUnion LUM_MINVAL = {PCG_VEC_UNION( float_limits::min() )};
static const VeciUnion INT_ONE = {PCG_VEC_UNION( 1 )};
static const VeciUnion MASK_NAN = {PCG_VEC_UNION( 0x7f800000 )};

const Vec4f LUM_TAIL_MASKS_V4[3] = {
    Vec4f(_mm_castsi128_ps(Vec4i::constant<0, 0, 0,-1>())),
    Vec4f(_mm_castsi128_ps(Vec4i::constant<0, 0,-1,-1>())),
    Vec4f(_mm_castsi128_ps(Vec4i::constant<0,-1,-1,-1>()))
};

#if PCG_USE_AVX
static const VecfUnion LUM_MAXVAL = {PCG_VEC_UNION( float_limits::max() )};
const Vec8f LUM_TAIL_MASKS_V8[7] = {
    Vec8f(_mm256_castsi256_ps(Vec8i::constant<0, 0, 0, 0, 0, 0, 0,-1>())),
    Vec8f(_mm256_castsi256_ps(Vec8i::constant<0, 0, 0, 0, 0, 0,-1,-1>())),
    Vec8f(_mm256_castsi256_ps(Vec8i::constant<0, 0, 0, 0, 0,-1,-1,-1>())),
    Vec8f(_mm256_castsi256_ps(Vec8i::constant<0, 0, 0, 0,-1,-1,-1,-1>())),
    Vec8f(_mm256_castsi256_ps(Vec8i::constant<0, 0, 0,-1,-1,-1,-1,-1>())),
    Vec8f(_mm256_castsi256_ps(Vec8i::constant<0, 0,-1,-1,-1,-1,-1,-1>())),
    Vec8f(_mm256_castsi256_ps(Vec8i::constant<0,-1,-1,-1,-1,-1,-1,-1>()))
};
#endif


} // namespace constants



// Super basic auto_ptr kind of thing for aligned float memory
class auto_afloat_ptr : public std::auto_ptr<float>
{
public:
    auto_afloat_ptr(float * ptr = NULL) : std::auto_ptr<float>(ptr) {}

    ~auto_afloat_ptr() {
        if (get() != NULL) {
            free_align (release());
        }
    }
};



///////////////////////////////////////////////////////////////////////////////
// Traits for source iterators
template <class SourceIterator>
struct iterator_traits;

template <>
struct iterator_traits<RGBA32FVec4ImageSoAIterator>
{
    typedef Vec4f  vf;
    typedef Vec4bf vbf;
    typedef Vec4i  vi;
    typedef Vec4bi vbi;

    enum Constants {
        VEC_LEN = 4
    };
};

template <>
struct iterator_traits<RGBA32FVec4ImageIterator>
{
    typedef Vec4f  vf;
    typedef Vec4bf vbf;
    typedef Vec4i  vi;
    typedef Vec4bi vbi;
    
    enum Constants {
        VEC_LEN = 4
    };
};

#if PCG_USE_AVX
template <>
struct iterator_traits<RGBA32FVec8ImageSoAIterator>
{
    typedef Vec8f  vf;
    typedef Vec8bf vbf;
    typedef Vec8i  vi;
    
    enum Constants {
        VEC_LEN = 8
    };
};
#endif



///////////////////////////////////////////////////////////////////////////////
// Traits for vector types
template <typename V>
struct vector_traits;

template <>
struct vector_traits<Vec4f>
{
    enum Constants {
        VEC_LEN    = 4,
        BLOCK_SIZE = 1024
    };
};

#if PCG_USE_AVX
template <>
struct vector_traits<Vec8f>
{
    enum Constants {
        VEC_LEN    = 8,
        BLOCK_SIZE = 512
    };
};
#endif



///////////////////////////////////////////////////////////////////////////////
// Traits for the masks for tail elements
template <typename VecType>
struct tail_mask_traits;

template <>
struct tail_mask_traits<Vec4f>
{
    template <int tailElements>
    static inline const Vec4f& getTailMask() {
        return constants::LUM_TAIL_MASKS_V4[tailElements-1];
    }
};

#if PCG_USE_AVX
template <>
struct tail_mask_traits<Vec8f>
{
    template <int tailElements>
    static inline const Vec8f& getTailMask() {
        return constants::LUM_TAIL_MASKS_V8[tailElements-1];
    }
};
#endif


// Helper to get the appropriate tail mask for the templated luminance functors
template <typename VecType, int tailElements>
inline const VecType& getTailMask() {
    return tail_mask_traits<VecType>::template getTailMask<tailElements>();
}

template <>
inline const Vec4f& getTailMask<Vec4f,0>() {
    assert("This should never be used" == 0);
    return constants::LUM_TAIL_MASKS_V4[0];
}

#if PCG_USE_AVX
template <>
inline const Vec8f& getTailMask<Vec8f,0>() {
    assert("This should never be used" == 0);
    return constants::LUM_TAIL_MASKS_V8[0];
}
#endif




// Minimum between the given value and all the elements of the vector
inline float horizontal_min(const float& x, const Vec4f& vec) {
    Vec4f tmpMin = simd_min(vec, simd_shuffle<1,0,3,2>(vec));
    tmpMin = simd_min(tmpMin, simd_shuffle<2,3,0,1>(tmpMin));
    tmpMin = _mm_min_ss(_mm_load_ss(&x), tmpMin);
    return _mm_cvtss_f32(tmpMin);
}


// Maximum between the given value and all the elements of the vector
inline float horizontal_max(const float& x, const Vec4f& vec) {
    Vec4f tmpMax = simd_max(vec, simd_shuffle<1,0,3,2>(vec));
    tmpMax = simd_max(tmpMax, simd_shuffle<2,3,0,1>(tmpMax));
    tmpMax = _mm_max_ss(_mm_load_ss(&x), tmpMax);
    return _mm_cvtss_f32(tmpMax);
}


// Sum of each element in the vector using SSE3
inline float horizontal_sum(const __m128& vec) {
    __m128 sum_tmp = _mm_hadd_ps(vec, vec);
    sum_tmp = _mm_hadd_ps(sum_tmp, sum_tmp);
    return _mm_cvtss_f32(sum_tmp);
}


// Convert a floating point vector to integers using truncate
inline Vec4i truncate(const Vec4f& v) {
    return _mm_cvttps_epi32(v);
}


// SIMD logarithm: this method might only work correctly for valid values
inline Vec4f simd_log(const Vec4f& v) {
#if USE_AM_LOG
    return am::log_eps(v);
#else
    return ssemath::log_ps(v);
#endif
}



#if PCG_USE_AVX

inline float horizontal_min(const float& x, const Vec8f& vec) {
    Vec8f xIn = _mm256_castps128_ps256(_mm_load_ss(&x));
    Vec8f tmpMin = simd_min(vec, simd_permute<2,3,0,1>(vec));
    tmpMin = simd_min(tmpMin, simd_permute<1,0,3,2>(tmpMin));
    tmpMin = simd_min(tmpMin, simd_permuteHiLo(tmpMin));
    tmpMin = simd_min(tmpMin, xIn);
    return _mm_cvtss_f32(_mm256_castps256_ps128(tmpMin));
}

inline float horizontal_max(const float& x, const Vec8f& vec) {
    Vec8f xIn = _mm256_castps128_ps256(_mm_load_ss(&x));
    Vec8f tmpMax = simd_max(vec, simd_permute<2,3,0,1>(vec));
    tmpMax = simd_max(tmpMax, simd_permute<1,0,3,2>(tmpMax));
    tmpMax = simd_max(tmpMax, simd_permuteHiLo(tmpMax));
    tmpMax = simd_max(tmpMax, xIn);
    return _mm_cvtss_f32(_mm256_castps256_ps128(tmpMax));
}

inline float horizontal_sum(const Vec8f& vec) {
    Vec8f tmp  = _mm256_hadd_ps(vec, vec);
    tmp        = _mm256_hadd_ps(tmp, tmp);
    Vec8f pTmp = simd_permuteHiLo(tmp);
    Vec8f r    = tmp + pTmp;
    return _mm_cvtss_f32(_mm256_castps256_ps128(r));
}

inline Vec8i truncate(const Vec8f& v) {
    return _mm256_cvttps_epi32(v);
}

inline Vec8f simd_log(const Vec8f& v) {
#if USE_AM_LOG
    return am::log_avx(v);
#else
    return ssemath::log_avx(v);
#endif
}

#endif // PCG_USE_AVX



// Little helper to extract RGB elements from an iterator
template <class RGBIterator, typename VecT>
inline void extractRGB(RGBIterator it, VecT &outR, VecT &outG, VecT &outB) {
    outR = it->r();
    outG = it->g();
    outB = it->b();
}


template<>
inline void
extractRGB<RGBA32FVec4ImageIterator, Vec4f>(RGBA32FVec4ImageIterator it,
    Vec4f &outR, Vec4f &outG, Vec4f &outB) {
    RGBA32FVec4 data = *it;
    outR = data.r();
    outG = data.g();
    outB = data.b();
}



// Create a mask to zero out invalid pixels
//   0x7f800000u > floatToBits(x) && x >= float_limits::min(), ossia
//   isnormal(x)
inline Vec4f getValidLuminanceMask(const Vec4f& Lw) {
    const Vec4f& MINVAL(constants::get<Vec4f>(constants::LUM_MINVAL));
    const Vec4i& MASK_NAN(constants::get<Vec4i>(constants::MASK_NAN));
        
    const Vec4f isNotTiny(Lw >= MINVAL);
    const Vec4i LwBits = _mm_castps_si128(Lw);
    const Vec4bi isNotNaN = MASK_NAN > LwBits;
    const Vec4f isValidMask = isNotTiny & Vec4f(_mm_castsi128_ps(isNotNaN));
    return isValidMask;
}

template <int tailElements>
inline int32_t updateZeroCount(const Vec4i& vecZeroCount)
{
    // Compensate in case of extra tail elements which are marked as invalid
    int32_t zeroCount = tailElements == 0 ? 0 : tailElements - 4;
    
    // Avoid the horizontal integer sum if all the values are zero
    if (!vecZeroCount.isZero()) {
        union { __m128i xmmi; int32_t i32[4]; } u = {vecZeroCount};
        for (int i = 0; i < 4; ++i) {
            zeroCount += u.i32[i];
        }
    }

    assert(zeroCount >= 0);
    return zeroCount;
}

// Compute [per component] a + (testMask & b)
inline Vec4i addMasked(const Vec4bf& testMask, const Vec4i& a, const Vec4i& b) {
    const Vec4i result = a + andnot(_mm_castps_si128(testMask), b);
    return result;
}

#if PCG_USE_AVX

inline Vec8f getValidLuminanceMask(const Vec8f& Lw) {
    // We cannot use the bit tricks with AVX (it needs AVX2), but we can test
    // for NaNs using the comparison intrinsics
    const Vec8f& MINVAL(constants::get<Vec8f>(constants::LUM_MINVAL));
    const Vec8f& MAXVAL(constants::get<Vec8f>(constants::LUM_MAXVAL));

    const Vec8f isNotTiny(Lw >= MINVAL);
    const Vec8f isFinite (Lw <= MAXVAL);
    const Vec8f isNotNan (_mm256_cmp_ps(Lw, Lw, _CMP_ORD_Q));
    const Vec8f isValidMask = isNotTiny & isFinite & isNotNan;
    return isValidMask;
}

template <int tailElements>
inline int32_t updateZeroCount(const Vec8i& vecZeroCount)
{
    // Compensate in case of extra tail elements which are marked as invalid
    int32_t zeroCount = tailElements == 0 ? 0 : tailElements - 8;
    
    // Avoid the horizontal integer sum if all the values are zero
    if (!vecZeroCount.isZero()) {
        union { __m256i ymmi; int32_t i32[8]; } u = {vecZeroCount};
        for (int i = 0; i < 8; ++i) {
            zeroCount += u.i32[i];
        }
    }

    assert(zeroCount >= 0);
    return zeroCount;
}

inline Vec8i addMasked(const Vec8bf& testMask, const Vec8i& a, const Vec8i& b) {
#if !PCG_USE_AVX2
    // Add using SSE
    Vec4i mask0 = _mm256_extractf128_si256(_mm256_castps_si256(testMask), 0);
    Vec4i mask1 = _mm256_extractf128_si256(_mm256_castps_si256(testMask), 1);
    Vec4i a0 = _mm256_extractf128_si256(a, 0);
    Vec4i a1 = _mm256_extractf128_si256(a, 1);
    Vec4i b0 = _mm256_extractf128_si256(b, 0);
    Vec4i b1 = _mm256_extractf128_si256(b, 1);

    const Vec4i r0 = a0 + andnot(mask0, b0);
    const Vec4i r1 = a1 + andnot(mask1, b1);
    Vec8i result = _mm256_insertf128_si256(_mm256_castsi128_si256(r0), r1, 1);
#else
    const Vec8i result = a + andnot(_mm256_castps_si256(testMask), b);
#endif
    return result;
}

#endif // PCG_USE_AVX



// Helper functor to call a "process" function using only the needed cases
template <int FullVectorSize>
struct TailProcess;

template <>
struct TailProcess<4>
{
    template <typename PFunctor, class Iterator, typename T>
    static inline void
    process(PFunctor& f, Iterator begin, Iterator end, const T& numTail) {
        switch (numTail) {
        case 1:
            f.process<1>(begin, end);
            break;
        case 2:
            f.process<2>(begin, end);
            break;
        case 3:
            f.process<3>(begin, end);
            break;
        default:
            assert(0);
        }
    }
};

#if PCG_USE_AVX

template <>
struct TailProcess<8>
{
    template <typename PFunctor, class Iterator, typename T>
    static inline void
    process(PFunctor& f, Iterator begin, Iterator end, const T& numTail) {
        switch (numTail) {
        case 1:
            f.process<1>(begin, end);
            break;
        case 2:
            f.process<2>(begin, end);
            break;
        case 3:
            f.process<3>(begin, end);
            break;
        case 4:
            f.process<4>(begin, end);
            break;
        case 5:
            f.process<5>(begin, end);
            break;
        case 6:
            f.process<6>(begin, end);
            break;
        case 7:
            f.process<7>(begin, end);
            break;
        default:
            assert(0);
        }
    }
};

#endif // PCG_USE_AVX



///////////////////////////////////////////////////////////////////////////////
// Actual Functors
///////////////////////////////////////////////////////////////////////////////


// TBB functor object to fill the array of luminances for image iterators.
// It converts the invalid values to zero and returns the maximum,
// minimum and number of invalid values. The template parameter indicates
// the number of tail elements per vector block: a non-zero value indicates that
// only that many elements in the last vector block are valid.
template <class SourceIterator = RGBA32FVec4ImageSoAIterator>
struct LuminanceFunctor
{
    typedef typename iterator_traits<SourceIterator>::vf  Vecf;
    typedef typename iterator_traits<SourceIterator>::vbf Vecbf;
    typedef typename iterator_traits<SourceIterator>::vi  Veci;

    // Remember where the data starts
    SourceIterator pixelsBegin;
    SourceIterator pixelsEnd;

    // Target luminance array, with extra elements allocated
    Vecf* const PCG_RESTRICT Lw;

    // Number of tail elements (in the last vector component)
    const size_t numTail;

    // Data to be reduced
    size_t zero_count;
    float Lmin;
    float Lmax;

    // Constructor for the initial phase
    LuminanceFunctor (SourceIterator begin, SourceIterator end, Vecf* Lw_,
        size_t nTail) :
    pixelsBegin(begin), pixelsEnd(end), Lw(Lw_), numTail(nTail), zero_count(0), 
    Lmin(float_limits::infinity()), Lmax(-float_limits::infinity())
    {
        assert(numTail < iterator_traits<SourceIterator>::VEC_LEN);
    }

    // Constructor for each split
    LuminanceFunctor (LuminanceFunctor& l, tbb::split) :
    pixelsBegin(l.pixelsBegin), pixelsEnd(l.pixelsEnd), Lw(l.Lw),
    numTail(l.numTail), zero_count(0), 
    Lmin(float_limits::infinity()), Lmax(-float_limits::infinity()) {}

    // TBB method: joins this functor with the given one
    void join (LuminanceFunctor& rhs)
    {
        zero_count += rhs.zero_count;
        Lmin = fminf (Lmin, rhs.Lmin);
        Lmax = fmaxf (Lmax, rhs.Lmax);
    }

    // Method invoked by TBB
    void operator() (const tbb::blocked_range<SourceIterator> &range)
    {
        if (numTail == 0 || range.end() != pixelsEnd) {
            process<0>(range.begin(), range.end());
        }
        else {
            // The very last element is the problematic one
            SourceIterator bulkEnd = range.end() - 1;
            process<0>(range.begin(), bulkEnd);

            // This will process a single element
            typedef TailProcess<iterator_traits<SourceIterator>::VEC_LEN> TP;
            TP::process(*this, bulkEnd, range.end(), numTail);
        }
    }


private:
    friend struct TailProcess<iterator_traits<SourceIterator>::VEC_LEN>;

    // Accumulates the results, using the given number of tail elements
    template <int tailElements>
    inline void process(SourceIterator begin, SourceIterator end)
    {
        // Offset for the output
        Vecf* dest = Lw + (begin - pixelsBegin);

        // Initialize the working values
        Vecf vec_min(Lmin);
        Vecf vec_max(Lmax);
        Veci vec_zero_count(0);

        // References to the global constants
        const Vecf& LUM_R(constants::get<Vecf>(constants::LUM_R));
        const Vecf& LUM_G(constants::get<Vecf>(constants::LUM_G));
        const Vecf& LUM_B(constants::get<Vecf>(constants::LUM_B));
        const Veci& INT_ONE(constants::get<Veci>(constants::INT_ONE));

        for (SourceIterator it = begin; it != end; ++it, ++dest) {
            
            // Raw luminance, with NaN and Inf
            Vecf pixelR, pixelG, pixelB;
            extractRGB(it, pixelR, pixelG, pixelB);
            const Vecf Lw = LUM_R*pixelR + LUM_G*pixelG + LUM_B*pixelB;

            // Write the valid luminance values
            Vecf isValidMask = getValidLuminanceMask(Lw);
            Vecf validLw = Lw & isValidMask;

            if (tailElements != 0) {
                const Vecf& tailMask(getTailMask<Vecf, tailElements>());
                validLw     &= tailMask;
                isValidMask &= tailMask;
            }
            *dest = validLw;

            // Update the min/max
            const Vecbf isValid(isValidMask);
            vec_min = select(isValid, simd_min(vec_min, validLw), vec_min);
            vec_max = select(isValid, simd_max(vec_max, validLw), vec_max);

            // Update the zero count
            vec_zero_count = addMasked(isValid, vec_zero_count, INT_ONE);
        }

        // Accumulate the totals for min, max and zero_count
        Lmin = horizontal_min(Lmin, vec_min);
        Lmax = horizontal_max(Lmax, vec_max);

        const int32_t localZeros= updateZeroCount<tailElements>(vec_zero_count);
        zero_count += localZeros;
        assert(zero_count <= static_cast<size_t>((pixelsEnd - pixelsBegin) *
            iterator_traits<SourceIterator>::VEC_LEN - tailElements));
    }
};



// Helper function to compact an array, moving all the zeros together.
// Returns the position of the first non-zero element.
// NOTE: The function assumes there is at least one zero in the array
size_t compactZeros (afloat_t * Lw, const size_t count)
{
    size_t nonzero_off = 0;
    float *begin = Lw;
    const float *end = Lw + count;
    
    // Find the first-non zero element
    while (*begin == 0.0f) {
        ++begin;
        assert (begin != end);
    }
    for (float *next = begin + 1 ; ; ) {
        // Find the next zero
        while (next != end && *next != 0.0f) ++next;
        if (next == end) {
            break;
        } else {
            // Swap and advance begin
            std::swap(*begin, *next);
            ++begin;
            assert (begin != end);
        }
    }
    assert (end - begin > 0);
    nonzero_off = begin - Lw;
    return nonzero_off;
}



class AccumulateNoHistogramFunctor
{
public:
#if !PCG_USE_AVX
    typedef Vec4f Vecf;
    typedef __m128i VecInt32;
#else
    typedef Vec8f Vecf;
    typedef __m256i VecInt32;
#endif

    // Current total
    inline double Lsum() const {
        return m_Lsum;
    }

    // Constructor for the initial phase
    AccumulateNoHistogramFunctor (const Vecf* LwEnd, size_t numTail):
    m_LwVecEnd(LwEnd), m_numTail(numTail), m_Lsum(0) {}

    // Constructor for each split
    AccumulateNoHistogramFunctor (AccumulateNoHistogramFunctor& ach,tbb::split):
    m_LwVecEnd(ach.m_LwVecEnd), m_numTail(ach.m_numTail), m_Lsum(0)
    {}

    // TBB method: joins this functor with the given one
    inline void join (AccumulateNoHistogramFunctor & rhs) {
        m_Lsum += rhs.m_Lsum;
    }

    // Method invoked by TBB: accumulates the data for the subrange
    void operator() (const tbb::blocked_range<const Vecf*>& range)
    {
        if (m_numTail == 0 || range.end() != m_LwVecEnd) {
            process<0>(range.begin(), range.end());
        }
        else {
            // The very last element is the problematic one
            const Vecf* bulkEnd = range.end() - 1;
            process<0>(range.begin(), bulkEnd);

            // This will process a single element
            TailProcess<vector_traits<Vecf>::VEC_LEN>::process(*this,
                bulkEnd, range.end(), m_numTail);
        }
    }


    // Helper function which handles everything
    static float accumulate (const float * PCG_RESTRICT Lw,
                             const float * PCG_RESTRICT Lw_end);

private:
    friend struct TailProcess<vector_traits<Vecf>::VEC_LEN>;

    template <int tailElements>
    inline void process(const Vecf* const PCG_RESTRICT begin,
        const Vecf* const PCG_RESTRICT end)
    {
        // Prepare Kahan summation with 4 elements
        Vecf vec_sum = Vecf::zero();
        Vecf vec_c   = Vecf::zero();

        for (const Vecf* it = begin; it != end; ++it) {
            const Vecf& vec_lum = *it;
            Vecf vec_log_lum = simd_log(vec_lum);

            // Kill the invalid values if required
            if (tailElements != 0) {
                const Vecf& tailMask(getTailMask<Vecf, tailElements>());
                vec_log_lum &= tailMask;
            }

            // Update the sum with error compensation
            const Vecf y = vec_log_lum - vec_c;
            const Vecf t = vec_sum + y;
            vec_c   = (t - vec_sum) - y;
            vec_sum = t;
        }

        // Accumulate the horizontal result
        const float L_sum_tmp = horizontal_sum(vec_sum);
        m_Lsum += L_sum_tmp;
    }


    // Remember where the data ends
    const Vecf* const m_LwVecEnd;

    // Number of tail elements at the last vector element    
    const size_t m_numTail;

    double m_Lsum;
};


float 
AccumulateNoHistogramFunctor::accumulate (const float * PCG_RESTRICT Lw,
                                          const float * PCG_RESTRICT Lw_end)
{
    const size_t numElements = Lw_end - Lw;
    const size_t VEC_LEN   = vector_traits<Vecf>::VEC_LEN;
    const Vecf* LwVecBegin = reinterpret_cast<const Vecf*>(Lw);
    const Vecf* LwVecEnd   = reinterpret_cast<const Vecf*>(Lw +
        ((numElements + (VEC_LEN-1)) & ~(VEC_LEN-1)));
    AccumulateNoHistogramFunctor acc (LwVecEnd, numElements % VEC_LEN);
    
    tbb::blocked_range<const Vecf*> range(LwVecBegin, LwVecEnd, 32/VEC_LEN);
    tbb::parallel_reduce (range, acc);
    return static_cast<float>(acc.Lsum());
}






// Accumulate the logarithm of the given array of luminances. It builds an
// histogram and also stores the log-luminances corresponding to the 1 and 99
// percentiles thresholds
struct AccumulateHistogramFunctor
{
    typedef std::vector<int, tbb::cache_aligned_allocator<int> > hist_t;
    typedef tbb::enumerable_thread_specific<hist_t> threadhist_t;

#if !PCG_USE_AVX
    typedef Vec4f Vecf;
    typedef __m128i VecInt32;
#else
    typedef Vec8f Vecf;
    typedef __m256i VecInt32;
#endif

    // Structure to hold all the common parameters
    struct Params
    {   
        const float res_factor;
        const float Lmin_log;
        const float Lmax_log;
        const float inv_res;

        const Vecf vec_res_factor;
        const Vecf vec_Lmin_log;

        // Initializes the parameters with the appropriate values. It receives
        // the maximum and minimum [lineal] luminance
        static Params init(float Lmin, float Lmax);

        // Returns a reference to the thread local histogram
        hist_t & localHistogram() {
            return tls_histogram.local();
        }

        // Accumulates all the local histograms. After using this method this
        // set of params should be read only!!
        hist_t & flatHistogram() {
            for (threadhist_t::const_iterator it = tls_histogram.begin(); 
                 it != tls_histogram.end(); ++it) {

                 const hist_t & curr = *it;
                 for (size_t i = 0; i < histogram.size(); ++i) {
                     histogram[i] += curr[i];
                 }
            }

            // Collapse the last helper bucket
            histogram[histogram.size() - 2] += histogram[histogram.size() - 1];
            histogram.resize(histogram.size() - 1);
            return histogram;
        }

    private:
        // Add an extra bucket to account for roundoff error with large values
        Params(hist_t::size_type count, float res_factor_,
            float Lmin_log_, float Lmax_log_, float inv_res_):
        res_factor(res_factor_), Lmin_log(Lmin_log_), Lmax_log(Lmax_log_),
        inv_res(inv_res_),
        vec_res_factor(res_factor_), vec_Lmin_log(Lmin_log_),
        histogram(count+1, 0), tls_histogram(histogram)
        {}

        hist_t histogram;
        threadhist_t tls_histogram;
    };

    // Remember where the data ends
    const Vecf* const LwVecEnd;

    // Number of tail elements at the last vector element
    const size_t numTail;

    // Reference to the parameters
    Params & params;

    // Variable which is part of the reduce operation
    double L_sum;


    // Constructor for the initial phase
    AccumulateHistogramFunctor (const Vecf* LwEnd, Params & params_, size_t n):
    LwVecEnd(LwEnd), numTail(n), params(params_), L_sum(0) {}

    // Constructor for each split
    AccumulateHistogramFunctor (AccumulateHistogramFunctor & ach, tbb::split) :
    LwVecEnd(ach.LwVecEnd), numTail(ach.numTail), params(ach.params), L_sum(0)
    {}

    // TBB method: joins this functor with the given one
    void join (AccumulateHistogramFunctor & rhs) {
        L_sum += rhs.L_sum;
    }

    // Method invoked by TBB: accumulates the data for the subrange
    void operator() (const tbb::blocked_range<const Vecf*>& range)
    {
        if (numTail == 0 || range.end() != LwVecEnd) {
            process<vector_traits<Vecf>::VEC_LEN>(range.begin(), range.end());
        }
        else {
            // The very last element is the problematic one
            const Vecf* bulkEnd = range.end() - 1;
            process<vector_traits<Vecf>::VEC_LEN>(range.begin(), bulkEnd);

            // This will process a single element
            TailProcess<vector_traits<Vecf>::VEC_LEN>::process(*this,
                bulkEnd, range.end(), numTail);
        }
    }


    // Helper function which handles everything
    static float accumulate ( const float * PCG_RESTRICT Lw,
                              const float * PCG_RESTRICT Lw_end,
                              const float Lmin, const float Lmax,
                              float &L1, float &L99);


private:
    friend struct TailProcess<vector_traits<Vecf>::VEC_LEN>;

    template <int validPerVector>
    inline void process(const Vecf* const PCG_RESTRICT begin,
        const Vecf* const PCG_RESTRICT end)
    {
        hist_t & histogram = params.localHistogram();

        // Prepare Kahan summation with 4 elements
        Vecf vec_sum = Vecf::zero();
        Vecf vec_c   = Vecf::zero();

        // Temporary storage for the indices
        const size_t BLOCK_SIZE = vector_traits<Vecf>::BLOCK_SIZE;
        union {
            VecInt32 indices_vec[BLOCK_SIZE];
            int32_t  indices_i32[4*BLOCK_SIZE];
        };

        for (const Vecf* it = begin; it != end;) {
            const size_t numIter = std::min(static_cast<size_t>(end - it),
                                            BLOCK_SIZE);
            for (size_t i = 0; i != numIter; ++i, ++it) {
                const Vecf& vec_lum = *it;
                Vecf vec_log_lum = simd_log(vec_lum);

                // Kill the invalid values if required
                if (validPerVector != vector_traits<Vecf>::VEC_LEN) {
                    const Vecf& tailMask(getTailMask<Vecf,
                        validPerVector % vector_traits<Vecf>::VEC_LEN>());
                    vec_log_lum &= tailMask;
                }

                // Update the sum with error compensation
                const Vecf y = vec_log_lum - vec_c;
                const Vecf t = vec_sum + y;
                vec_c   = (t - vec_sum) - y;
                vec_sum = t;

                // Get the histogram bin indices
                const Vecf idx_temp = params.vec_res_factor * 
                    (vec_log_lum - params.vec_Lmin_log);
                indices_vec[i] = truncate(idx_temp);
            }

            // Update the histogram
            for (size_t i = 0; i != numIter; ++i) {
                const int32_t* const indices_base =
                    &indices_i32[vector_traits<Vecf>::VEC_LEN * i];
                for (int k = 0; k != validPerVector; ++k) {
                    const int32_t& index = indices_base[k];
                    assert (index >= 0 && index < (int32_t)histogram.size());
                    ++histogram[index];
                }
            }
        }

        // Accumulate the horizontal result
        const float L_sum_tmp = horizontal_sum(vec_sum);
        L_sum += L_sum_tmp;
    }
};


AccumulateHistogramFunctor::Params
AccumulateHistogramFunctor::Params::init(float Lmin, float Lmax)
{
    assert (Lmax > Lmin);

    Vec4f rangeHelper(1.0f, 1.0f, Lmax, Lmin);
    rangeHelper = simd_log(rangeHelper);
    const float Lmin_log = rangeHelper[0];
    const float Lmax_log = rangeHelper[1];

    const int resolution = 100;
    const int dynrange = static_cast<int> (ceil(1e-5 + Lmax_log - Lmin_log));
    const int num_bins = std::min(resolution * dynrange, 2048);

    const float range = Lmax_log - Lmin_log;
    const float res_factor = num_bins / range;
    const float inv_res = range / num_bins;

    // Construct and return the object
    Params p (num_bins, res_factor, Lmin_log, Lmax_log, inv_res);
    return p;
}


float 
AccumulateHistogramFunctor::accumulate (const float * PCG_RESTRICT Lw,
                                        const float * PCG_RESTRICT Lw_end,
                                        const float Lmin, const float Lmax,
                                        float &L1, float &L99)
{
    AccumulateHistogramFunctor::Params params = 
        AccumulateHistogramFunctor::Params::init (Lmin, Lmax);

    const size_t numElements = Lw_end - Lw;
    const size_t VEC_LEN   = vector_traits<Vecf>::VEC_LEN;
    const Vecf* LwVecBegin = reinterpret_cast<const Vecf*>(Lw);
    const Vecf* LwVecEnd   = reinterpret_cast<const Vecf*>(Lw +
        ((numElements + (VEC_LEN-1)) & ~(VEC_LEN-1)));
    AccumulateHistogramFunctor acc (LwVecEnd, params, numElements % VEC_LEN);
    
    tbb::blocked_range<const Vecf*> range(LwVecBegin, LwVecEnd, 32/VEC_LEN);
    tbb::parallel_reduce (range, acc);

    AccumulateHistogramFunctor::hist_t & histogram = params.flatHistogram();
    const float & Lmin_log = params.Lmin_log;
    const float & inv_res  = params.inv_res;

    // Consult the histogram to get the L1 and L99 positions
    _mm_prefetch ((char*)(&histogram[histogram.size() -  8]), _MM_HINT_T0);
    _mm_prefetch ((char*)(&histogram[histogram.size() - 16]), _MM_HINT_T0);
    const ptrdiff_t count = Lw_end - Lw;
    const ptrdiff_t threshold = static_cast<ptrdiff_t> (0.01 * count);
    for (ptrdiff_t sum = 0, i = histogram.size() - 1; i >= 0; --i) {
        sum += histogram[i];
        if (sum > threshold) {
            L99 = static_cast<float>(i)*inv_res + Lmin_log;
            assert (Lmin_log <= L99 && L99 <= params.Lmax_log);
            break;
        }
    }
    _mm_prefetch ((char*)(&histogram[0]), _MM_HINT_T0);
    for (ptrdiff_t sum = 0, i = 0; (size_t)i < histogram.size() ; ++i) {
        sum += histogram[i];
        if (sum > threshold) {
            L1 = static_cast<float>(i)*inv_res + Lmin_log;
            assert (Lmin_log <= L1 && L1 <= params.Lmax_log && L1 <= L99);
            break;
        }
    }

    return static_cast<float> (acc.L_sum);
}



// Functor for accumulating the log-luminance beyond a threshold
struct SumThresholdFunctor
{
    // Range type
    typedef tbb::blocked_range<const float *> range_t;

    // Max number of expected elements
    const float lum_cutoff;
    const ptrdiff_t threshold;

    // Values to be returned
    double removed_sum;
    ptrdiff_t removed_count;

    // Initial constructor
    SumThresholdFunctor (float lum_cutoff_, ptrdiff_t threshold_) :
    lum_cutoff(lum_cutoff_), threshold(threshold_),
    removed_sum(0.0f), removed_count(0) {}

    // Splitting constructor
    SumThresholdFunctor (SumThresholdFunctor &s, tbb::split) :
    lum_cutoff(s.lum_cutoff), threshold(s.threshold),
    removed_sum(0.0f), removed_count(0) {}

    // Accumulate
    void operator() (const range_t &range)
    {
        // Continue using Kahan
        for (const float * PCG_RESTRICT lum = range.begin(); 
             lum != range.end() && removed_count < threshold; ++lum) {
            if (*lum > lum_cutoff) {
                 ++removed_count;
                 const double log_lum = log (static_cast<double> (*lum));
                 removed_sum += log_lum;
             }
        }
    }

    // Merge
    void join (SumThresholdFunctor &rhs)
    {
        removed_sum   += rhs.removed_sum;
        removed_count += rhs.removed_count;
    }

private:
    // Compensation value
    float sum_c;
};



// Helper function: accumulates the log-luminance beyond a given threshold.
// Returns the accumulation of those log-luminances and stores the number
// of elements added
float sumBeyondThreshold(const float * Lw, const float * Lw_end,
                         const float lum_cutoff, ptrdiff_t &removed_count)
{
    const ptrdiff_t count = Lw_end - Lw;
    const ptrdiff_t threshold = static_cast<ptrdiff_t> (0.01 * count);

    // Run in parallel
    SumThresholdFunctor stf(lum_cutoff, threshold);
    tbb::parallel_reduce(SumThresholdFunctor::range_t(Lw, Lw_end, 4), stf);
    removed_count = stf.removed_count;
    return static_cast<float> (stf.removed_sum);
}



// Helper to call the appropriate instantiation of the luminance helper:
// Stores the luminance in the destination Lw array, zeroing invalid values.
// Returns the count of zero values and the non-zero minimum and maximum 
// luminance (in the same units as the original image)
template <typename SourceIterator>
void LuminanceHelper(SourceIterator begin, SourceIterator end,
    afloat_t * PCG_RESTRICT Lw, size_t tailElements,
    size_t* outZeroCount, float* outLmin, float* outLmax)
{
    typedef typename iterator_traits<SourceIterator>::vf vf;
    const size_t VEC_LEN = iterator_traits<SourceIterator>::VEC_LEN;

    assert(Lw != NULL);
    assert(reinterpret_cast<uintptr_t>(Lw) % (VEC_LEN * sizeof(float)) == 0);
    assert(tailElements < VEC_LEN);

    vf* const PCG_RESTRICT LwVec = reinterpret_cast<vf*>(Lw);
    tbb::blocked_range<SourceIterator> range(begin, end, VEC_LEN);

    LuminanceFunctor<SourceIterator> lumFunctor(begin,end,LwVec, tailElements);
    tbb::parallel_reduce(range, lumFunctor);
    *outZeroCount = lumFunctor.zero_count;
    *outLmin      = lumFunctor.Lmin;
    *outLmax      = lumFunctor.Lmax;
}

} // namespace



Reinhard02::Params
Reinhard02::EstimateParams (afloat_t * const PCG_RESTRICT Lw, size_t count,
    const LuminanceResult& lumResult)
{
    assert (lumResult.zero_count <= count);
    const size_t& zero_count = lumResult.zero_count;
    const float& Lmin        = lumResult.Lmin;
    const float& Lmax        = lumResult.Lmax;

    // Abort if all the values are zero
    if (zero_count == count) {
        return Params(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    const size_t nonzero_off = zero_count==0 ? 0 : compactZeros(Lw, count);

    // If necessary move some elements to keep the 32-bytes alignment
    const size_t nonzero_delta = nonzero_off & 0x7;
    _mm_prefetch ((char*)(Lw + nonzero_off - nonzero_delta), _MM_HINT_T0);
    _mm_prefetch ((char*)(Lw + count - nonzero_delta), _MM_HINT_T0);
    afloat_t * Lw_nonzero = Lw + nonzero_off;
    float * Lw_end = Lw + count;
    if (nonzero_delta != 0) {
        for (size_t i = 0; i < nonzero_delta; ++i) {
            *(--Lw_nonzero) = *(--Lw_end);
        }
    }

    // Build a histogram to extract the key using percentiles 1 to 99
    const float Lmin_log = logf (Lmin);
    const float Lmax_log = logf (Lmax);
    float L1  = Lmin_log;
    float L99 = Lmax_log;
    float L_sum = (Lmax_log - Lmin_log) > 5e-8 ?
        AccumulateHistogramFunctor::accumulate(Lw_nonzero, Lw_end,
            Lmin, Lmax, L1, L99)
      : AccumulateNoHistogramFunctor::accumulate(Lw_nonzero, Lw_end);

    // Remove the value from the logaritmic total L_sum 
    // if log(luminance) > L99_real ---> luminance > exp(L99_real)
    // where L99_real = exp(L99)
    // We know for sure that all such values are in the last percentile, so
    // can avoid reading everything
    ptrdiff_t removed_count = 0;
    const float lum_cutoff = expf (expf (L99));
    const float removed_sum = sumBeyondThreshold (Lw_nonzero, Lw_end,
        lum_cutoff, removed_count);
    L_sum -= removed_sum;

    // Average log luminance (equation 1 of the JGT paper)
    const float Lw_log = L_sum / (count - nonzero_off - removed_count);
    const float l_w = expf (Lw_log);

    // Extimate the key using the reduced range (equation 4 of the JGT paper)
    // Note that the equation requires the log2 of Lmin, Lmax and Lw. At this
    // point L1 = ln(Lmin), L99 = ln(Lmax) and also Lw_log is expressed in
    // terms of the natural logarithm. Given that 
    //   log2(exp(x)) == x/ln(x) ~= 1.4427 x
    // that constant factor cancels out from Equation 4 therefore it is
    // possible to use the ln-based values.
    const float key = (L99-L1) > std::numeric_limits<float>::min() ?
        (0.18f * powf (4.0f, (2.0f*Lw_log - L1-L99) / (L99 - L1))) : 0.18f;

    // Use the full range for the white point (equation 5 of the JGT paper)
    // This computes log2(exp(Lmax_log)) - log2(exp(Lmin_log))
    // The expression checks that the formula will be larger than the average
    // log luminance
    const float full_range = 1.442695040888963f * (Lmax_log - Lmin_log);
    const float l_white = full_range > 1.4426950408f*Lw_log + 4.415037499278f ?
        (1.5f * exp2f(full_range - 5.0f)) : (1.5f * expf(Lmax_log));
    assert (l_white >= l_w);
   
    return Params(key, l_white, l_w, Lmin, Lmax);
}


Reinhard02::Params
Reinhard02::EstimateParams (const Rgba32F * const pixels, size_t count)
{
    assert(pixels != NULL);   
    assert(reinterpret_cast<uintptr_t>(pixels) % 16 == 0);

    // Allocate the array with the luminances with AVX[2]-friendly alignment
    afloat_t * PCG_RESTRICT Lw = alloc_align<float> (32, (count+7) & ~0x7);  
    if (Lw == NULL) {
        throw RuntimeException("Couldn't allocate the memory for the "
            "luminance buffer");
    }
    // Use a special auto pointer to get rid of the aligned buffer
    auto_afloat_ptr Lw_autoptr (Lw);

    // Compute the luminance
    RGBA32FVec4ImageIterator begin(pixels);
    RGBA32FVec4ImageIterator end(pixels + ((count + 3) & ~0x3));
    const size_t numTail = count % 4;
    LuminanceResult lumResult;
    LuminanceHelper(begin, end, Lw, numTail,
        &lumResult.zero_count, &lumResult.Lmin, &lumResult.Lmax);

    // Estimate the values
    Params params = EstimateParams(Lw, count, lumResult);
    return params;
}


Reinhard02::Params
Reinhard02::EstimateParams (const RGBAImageSoA& img)
{
    if (img.Size() == 0) {
        throw IllegalArgumentException("Empty image");
    }

    // Allocate the array with the luminances with AVX[2]-friendly alignment
    const size_t count = static_cast<size_t>(img.Size());
    afloat_t * PCG_RESTRICT Lw = alloc_align<float> (32, (count+7) & ~0x7);  
    if (Lw == NULL) {
        throw RuntimeException("Couldn't allocate the memory for the "
            "luminance buffer");
    }
    // Use a special auto pointer to get rid of the aligned buffer
    auto_afloat_ptr Lw_autoptr (Lw);

    // Compute the luminance
#if !PCG_USE_AVX
    typedef RGBA32FVec4ImageSoAIterator ImageIterator;
#else
    typedef RGBA32FVec8ImageSoAIterator ImageIterator;
#endif
    ImageIterator begin = ImageIterator::begin(img);
    ImageIterator end   = ImageIterator::end(img);
    const size_t numTail = count % iterator_traits<ImageIterator>::VEC_LEN;
    LuminanceResult lumResult;
    LuminanceHelper(begin, end, Lw, numTail,
        &lumResult.zero_count, &lumResult.Lmin, &lumResult.Lmax);

    // Estimate the values
    Params params = EstimateParams(Lw, count, lumResult);
    return params;
}
