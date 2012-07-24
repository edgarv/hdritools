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

#include <limits>
#include <vector>
#include <memory>

#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>


// Flag to use a little LUT packed into a 64-bit integer for the SSE luminance
#define USE_PACKED_LUT 0

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


#if USE_PACKED_LUT
#if __STDC_VERSION__>=199901 || defined(__GNUC__)
#include <stdint.h>
#elif defined (_MSC_VER)
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#endif
#endif // USE_PACKED_LUT



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
const Vec4f LUM_R(0.27f);
const Vec4f LUM_G(0.67f);
const Vec4f LUM_B(0.06f);
const Vec4f LUM_MINVAL(float_limits::min());
const Vec4i INT_ONE(1);
const Vec4i MASK_NAN(Vec4i::constant<0x7f800000>());
const Vec4f LUM_TAIL_MASKS[3] = {
    Vec4f(_mm_castsi128_ps(Vec4i::constant<0, 0, 0,-1>())),
    Vec4f(_mm_castsi128_ps(Vec4i::constant<0, 0,-1,-1>())),
    Vec4f(_mm_castsi128_ps(Vec4i::constant<0,-1,-1,-1>()))
};


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



// Helper to get the appropriate tail mask for the templated luminance functors
template <int tailElements>
inline const Vec4f& getTailMask();

template <>
inline const Vec4f& getTailMask<0>() {
    assert("This should never be used" == 0);
    return constants::LUM_TAIL_MASKS[0];
}
template <>
inline const Vec4f& getTailMask<1>() {
    return constants::LUM_TAIL_MASKS[0];
}
template <>
inline const Vec4f& getTailMask<2>() {
    return constants::LUM_TAIL_MASKS[1];
}
template <>
inline const Vec4f& getTailMask<3>() {
    return constants::LUM_TAIL_MASKS[2];
}



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



// Little helper to extract RGB elements from and iterator
template <typename RGBIterator>
inline void extractRGB(RGBIterator it, Vec4f &outR, Vec4f &outG, Vec4f &outB) {
    outR = it->r();
    outG = it->g();
    outB = it->b();
}


template<>
inline void extractRGB<RGBA32FVec4ImageIterator>(RGBA32FVec4ImageIterator it,
    Vec4f &outR, Vec4f &outG, Vec4f &outB) {
    RGBA32FVec4 data = *it;
    outR = data.r();
    outG = data.g();
    outB = data.b();
}


// TBB functor object to fill the array of luminances for image iterators.
// It converts the invalid values to zero and returns the maximum,
// minimum and number of invalid values. The template parameter indicates
// the number of tail elements per vector block: a non-zero value indicates that
// only that many elements in the last vector block are valid.
template <class SourceIterator = RGBA32FVec4ImageSoAIterator>
struct LuminanceFunctor
{
    // Remember where the data starts
    SourceIterator pixelsBegin;
    SourceIterator pixelsEnd;

    // Target luminance array, with extra elements allocated
    Vec4f* const PCG_RESTRICT Lw;

    // Number of tail elements (in the last vector component)
    const size_t numTail;

    // Data to be reduced
    size_t zero_count;
    float Lmin;
    float Lmax;

    // Constructor for the initial phase
    LuminanceFunctor (SourceIterator begin, SourceIterator end, Vec4f* Lw_,
        size_t nTail) :
    pixelsBegin(begin), pixelsEnd(end), Lw(Lw_), numTail(nTail), zero_count(0), 
    Lmin(float_limits::infinity()), Lmax(-float_limits::infinity())
    {
        assert(numTail < 4);
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
            switch (numTail) {
            case 1:
                process<1>(bulkEnd, range.end());
                break;
            case 2:
                process<2>(bulkEnd, range.end());
                break;
            case 3:
                process<3>(bulkEnd, range.end());
                break;
            default:
                assert(0);
            }
        }
    }

private:

    // Create a mask to zero out invalid pixels
    //   0x7f800000u > floatToBits(x) && x >= float_limits::min(), ossia
    //   isnormal(x) && x >= float_limits::min()
    inline Vec4f getValidLuminanceMask(const Vec4f& Lw) {
        const Vec4f& MINVAL(constants::LUM_MINVAL);
        const Vec4i& MASK_NAN(constants::MASK_NAN);
        
        const Vec4f isNotTiny(Lw >= MINVAL);
        const Vec4i LwBits = _mm_castps_si128(Lw);
        const Vec4bi isNotNaN = MASK_NAN > LwBits;
        const Vec4f isValidMask = isNotTiny & Vec4f(_mm_castsi128_ps(isNotNaN));
        return isValidMask;
    }

    // Accumulates the results, using the given number of tail elements
    template <int tailElements>
    inline void process (SourceIterator begin, SourceIterator end)
    {
        // Offset for the output
        Vec4f* dest = Lw + (begin - pixelsBegin);

        // Initialize the working values
        Vec4f vec_min(Lmin);
        Vec4f vec_max(Lmax);
        Vec4i vec_zero_count(0);

        // References to the global constants
        const Vec4f& LUM_R(constants::LUM_R);
        const Vec4f& LUM_G(constants::LUM_G);
        const Vec4f& LUM_B(constants::LUM_B);
        const Vec4i& INT_ONE(constants::INT_ONE);

        for (SourceIterator it = begin; it != end; ++it, ++dest) {
            
            // Raw luminance, with NaN and Inf
            Vec4f pixelR, pixelG, pixelB;
            extractRGB(it, pixelR, pixelG, pixelB);
            const Vec4f Lw = LUM_R*pixelR + LUM_G*pixelG + LUM_B*pixelB;

            // Write the valid luminance values
            Vec4f isValidMask = getValidLuminanceMask(Lw);
            Vec4f validLw = Lw & isValidMask;

            if (tailElements != 0) {
                const Vec4f& tailMask(getTailMask<tailElements>());
                validLw     &= tailMask;
                isValidMask &= tailMask;
            }
            *dest = validLw;

            // Update the min/max
            const Vec4bf isValid(static_cast<__m128>(isValidMask));
            vec_min = select(isValid, simd_min(vec_min, validLw), vec_min);
            vec_max = select(isValid, simd_max(vec_max, validLw), vec_max);

            // Update the zero count
            vec_zero_count = vec_zero_count +
                andnot(_mm_castps_si128(isValid), INT_ONE);
        }

        // Accumulate the totals for min, max and zero_count
        Lmin = horizontal_min(Lmin, vec_min);
        Lmax = horizontal_max(Lmax, vec_max);

        // Avoid the horizontal integer sum if all the values are zero
        int countMask = _mm_movemask_epi8(vec_zero_count == Vec4i::zero());
        if (countMask != 0xFFFF) {
            Vec4iUnion countUnion = {vec_zero_count};
            _mm_store_si128(&countUnion.xmm, vec_zero_count);
            for (int i = 0; i < 4; ++i) {
                zero_count += countUnion.i32[i];
            }
        }

        // Adjust in case of extra tail elements which were marked as invalid
        if (tailElements != 0) {
            assert(zero_count >= (4 - tailElements));
            zero_count -= 4 - tailElements;
        }
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
    // Current total
    inline double Lsum() const {
        return m_Lsum;
    }

    // Constructor for the initial phase
    AccumulateNoHistogramFunctor (const Vec4f* LwEnd, size_t numTail):
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
    void operator() (const tbb::blocked_range<const Vec4f*>& range)
    {
        if (m_numTail == 0 || range.end() != m_LwVecEnd) {
            process<0>(range.begin(), range.end());
        }
        else {
            // The very last element is the problematic one
            const Vec4f* bulkEnd = range.end() - 1;
            process<0>(range.begin(), bulkEnd);

            // This will process a single element
            switch (m_numTail) {
            case 1:
                process<1>(bulkEnd, range.end());
                break;
            case 2:
                process<2>(bulkEnd, range.end());
                break;
            case 3:
                process<3>(bulkEnd, range.end());
                break;
            default:
                assert(0);
            }
        }
    }


    // Helper function which handles everything
    static float accumulate (const float * PCG_RESTRICT Lw,
                             const float * PCG_RESTRICT Lw_end);

private:

    template <int tailElements>
    inline void process(const Vec4f* const PCG_RESTRICT begin,
        const Vec4f* const PCG_RESTRICT end)
    {
        // Prepare Kahan summation with 4 elements
        Vec4f vec_sum = Vec4f::zero();
        Vec4f vec_c   = Vec4f::zero();

        for (const Vec4f* it = begin; it != end; ++it) {
            const Vec4f& vec_lum = *it;
#if USE_AM_LOG
            Vec4f vec_log_lum = am::log_eps (vec_lum);
#else
            Vec4f vec_log_lum = ssemath::log_ps (vec_lum);
#endif
            // Kill the invalid values if required
            if (tailElements != 0) {
                const Vec4f& tailMask(getTailMask<tailElements>());
                vec_log_lum &= tailMask;
            }

            // Update the sum with error compensation
            const Vec4f y = vec_log_lum - vec_c;
            const Vec4f t = vec_sum + y;
            vec_c   = (t - vec_sum) - y;
            vec_sum = t;
        }

        // Accumulate the horizontal result
        const float L_sum_tmp = horizontal_sum(vec_sum);
        m_Lsum += L_sum_tmp;
    }


    // Remember where the data ends
    const Vec4f* const m_LwVecEnd;

    // Number of tail elements at the last vector element    
    const size_t m_numTail;

    double m_Lsum;
};


float 
AccumulateNoHistogramFunctor::accumulate (const float * PCG_RESTRICT Lw,
                                          const float * PCG_RESTRICT Lw_end)
{
    const size_t numElements = Lw_end - Lw;
    const Vec4f* LwVecBegin = reinterpret_cast<const Vec4f*>(Lw);
    const Vec4f* LwVecEnd   =
        reinterpret_cast<const Vec4f*>(Lw + ((numElements + 3) & ~0x3));
    AccumulateNoHistogramFunctor acc (LwVecEnd, numElements % 4);
    
    tbb::blocked_range<const Vec4f*> range(LwVecBegin, LwVecEnd, 4);
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

    // Structure to hold all the common parameters
    struct Params
    {   
        const float res_factor;
        const float Lmin_log;
        const float Lmax_log;
        const float inv_res;

        const Vec4f vec_res_factor;
        const Vec4f vec_Lmin_log;

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
    const Vec4f* const LwVecEnd;

    // Number of tail elements at the last vector element
    const size_t numTail;

    // Reference to the parameters
    Params & params;

    // Variable which is part of the reduce operation
    double L_sum;


    // Constructor for the initial phase
    AccumulateHistogramFunctor (const Vec4f* LwEnd, Params & params_, size_t n):
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
    void operator() (const tbb::blocked_range<const Vec4f*>& range)
    {
        if (numTail == 0 || range.end() != LwVecEnd) {
            process<0>(range.begin(), range.end());
        }
        else {
            // The very last element is the problematic one
            const Vec4f* bulkEnd = range.end() - 1;
            process<0>(range.begin(), bulkEnd);

            // This will process a single element
            switch (numTail) {
            case 1:
                process<1>(bulkEnd, range.end());
                break;
            case 2:
                process<2>(bulkEnd, range.end());
                break;
            case 3:
                process<3>(bulkEnd, range.end());
                break;
            default:
                assert(0);
            }
        }
    }


    // Helper function which handles everything
    static float accumulate ( const float * PCG_RESTRICT Lw,
                              const float * PCG_RESTRICT Lw_end,
                              const float Lmin, const float Lmax,
                              float &L1, float &L99);


private:

    template <int tailElements>
    inline void process(const Vec4f* const PCG_RESTRICT begin,
        const Vec4f* const PCG_RESTRICT end)
    {
        hist_t & histogram = params.localHistogram();

        // Prepare Kahan summation with 4 elements
        Vec4f vec_sum = Vec4f::zero();
        Vec4f vec_c   = Vec4f::zero();

        for (const Vec4f* it = begin; it != end; ++it) {
            const Vec4f& vec_lum = *it;
#if USE_AM_LOG
            Vec4f vec_log_lum = am::log_eps (vec_lum);
#else
            Vec4f vec_log_lum = ssemath::log_ps (vec_lum);
#endif
            // Kill the invalid values if required
            if (tailElements != 0) {
                const Vec4f& tailMask(getTailMask<tailElements>());
                vec_log_lum &= tailMask;
            }

            // Update the sum with error compensation
            const Vec4f y = vec_log_lum - vec_c;
            const Vec4f t = vec_sum + y;
            vec_c   = (t - vec_sum) - y;
            vec_sum = t;

            // Update the histogram
            const Vec4f idx_temp = params.vec_res_factor * 
                (vec_log_lum - params.vec_Lmin_log);
            const __m128i bin_idx = _mm_cvttps_epi32 (idx_temp);
            const int index0 = _mm_extract_epi16(bin_idx, 0*2);
            const int index1 = _mm_extract_epi16(bin_idx, 1*2);
            const int index2 = _mm_extract_epi16(bin_idx, 2*2);
            const int index3 = _mm_extract_epi16(bin_idx, 3*2);

            // Fall-through is intended
            switch (tailElements) {
            case 0:
                assert (index3 >= 0 && index3 < (int)histogram.size());
                ++histogram[index3];
            case 3:
                assert (index2 >= 0 && index2 < (int)histogram.size());
                ++histogram[index2];
            case 2:
                assert (index1 >= 0 && index1 < (int)histogram.size());
                ++histogram[index1];
            case 1:
                assert (index0 >= 0 && index0 < (int)histogram.size());
                ++histogram[index0];
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
#if USE_AM_LOG
    rangeHelper = am::log_eps (rangeHelper);
#else
    rangeHelper = ssemath::log_ps (rangeHelper);
#endif
    const float Lmin_log = rangeHelper[0];
    const float Lmax_log = rangeHelper[1];

    const int resolution = 100;
    const int dynrange = static_cast<int> (ceil(1e-5 + Lmax_log - Lmin_log));
    const int num_bins = std::min(resolution * dynrange, 0x7FFF);

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
    const Vec4f* LwVecBegin = reinterpret_cast<const Vec4f*>(Lw);
    const Vec4f* LwVecEnd   =
        reinterpret_cast<const Vec4f*>(Lw + ((numElements + 3) & ~0x3));
    AccumulateHistogramFunctor acc (LwVecEnd, params, numElements % 4);
    
    tbb::blocked_range<const Vec4f*> range(LwVecBegin, LwVecEnd, 4);
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
    assert(Lw != NULL);
    assert(reinterpret_cast<uintptr_t>(Lw) % 16 == 0);
    assert(tailElements < 4);

    Vec4f * const PCG_RESTRICT LwVec4 = reinterpret_cast<Vec4f*>(Lw);
    tbb::blocked_range<SourceIterator> range(begin, end, 16);

    LuminanceFunctor<SourceIterator> lumFunctor(begin,end,LwVec4, tailElements);
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

    // If necessary move some elements to keep the 16-bytes alignment
    const size_t nonzero_delta = nonzero_off & 0x3;
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
    RGBA32FVec4ImageSoAIterator begin = RGBA32FVec4ImageSoAIterator::begin(img);
    RGBA32FVec4ImageSoAIterator end   = RGBA32FVec4ImageSoAIterator::end(img);
    const size_t numTail = count % 4;
    LuminanceResult lumResult;
    LuminanceHelper(begin, end, Lw, numTail,
        &lumResult.zero_count, &lumResult.Lmin, &lumResult.Lmax);

    // Estimate the values
    Params params = EstimateParams(Lw, count, lumResult);
    return params;
}
