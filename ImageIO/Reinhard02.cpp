
// Global implementation of the Reinhard02 tone mapper:
// Reinhard, E., Stark, M., Shirley, P., Ferwerda, J.
// "Photographic tone reproduction for digital images", ACM SIGGRAPH 2002
// http://doi.acm.org/10.1145/566570.566575
//
// The automatic parameter selection follows the paper
// "Parameter estimation for photographic tone reproduction" by
// Erik Reinhard, Journal of Graphics Tools Volume 7, Issue 1 (Nov 2002)
// http://www.cs.bris.ac.uk/~reinhard/papers/jgt_reinhard.pdf

#include "Reinhard02.h"
#include "Amaths.h"

#include <cstddef>

#include <limits>
#include <vector>
#include <memory>

#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>


// Flag to use a little LUT packed into a 64-bit integer for the SSE luminance
#define USE_PACKED_LUT 0


#if USE_PACKED_LUT
#if __STDC_VERSION__>=199901 || defined(__GNUC__)
#include <stdint.h>
#elif defined (_MSC_VER)
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;
#endif
#endif // USE_PACKED_LUT



#if defined(_WIN32) && defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
# if defined (_MSC_VER)


namespace {

// Add some required C99 functions
inline float exp2f(float x) {
    return powf(2.0f, x);
}

} // namespace

# endif
#endif

// SSE3 functions are only available as intrinsic in MSVC
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic ( _mm_hadd_ps )
#else
#include <pmmintrin.h>
#endif /* _MSC_VER */

namespace pcg
{




} // namespace pcg



using namespace pcg;

namespace
{

typedef std::numeric_limits<float> float_limits;


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



inline unsigned int floatToBits(float x) {
    union { float f; unsigned int bits; } data;
    data.f = x;
    return data.bits;
}


inline bool isInvalidLuminance(float x) {
    // True for denormalized values, NaNs and infinity
#if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
    return isless(x, std::numeric_limits<float>::min()) || !isfinite(x);
#else
    return floatToBits(x)>=0x7f800000u || x < float_limits::min();
#endif
}



// TBB functor object to fill the array of luminances. It converts the invalid
// values to zero and returns the maximum, minimum and number of invalid values
struct LuminanceFunctor
{
    // Original data
    const Rgba32F * PCG_RESTRICT const pixels;
    afloat_t * PCG_RESTRICT Lw;

    // Data to be reduced
    size_t zero_count;
    float Lmin;
    float Lmax;

    // Helper constants
    static const float LUM_R;
    static const float LUM_G;
    static const float LUM_B;
    static const Rgba32F vec_LUM;
    static const Rgba32F vec_LUM_R;
    static const Rgba32F vec_LUM_G;
    static const Rgba32F vec_LUM_B;
    static const Rgba32F vec_MINVAL;
    static const Rgba32F ZERO;
    static const __m128i MASK_NAN;

    // LUT for counting the number of zero-ed elements given the 4x32bit masks
    // where a 0x0 means the element was converted to zero.
    // The highest 4bits contain the count for 0xFF, the lowest for 0x0
#if USE_PACKED_LUT
    static const uint64_t packed_lut = 0x112122312232334ULL;
#else
    static const int LUT[];
#endif

    // Constructor for the initial phase
    LuminanceFunctor (const Rgba32F * const pixels_, afloat_t * Lw_) :
    pixels(pixels_), Lw(Lw_), zero_count(0), 
    Lmin(float_limits::infinity()), Lmax(-float_limits::infinity()) {}

    // Constructor for each split
    LuminanceFunctor (LuminanceFunctor & l, tbb::split) :
    pixels(l.pixels), Lw(l.Lw), zero_count(0), 
    Lmin(float_limits::infinity()), Lmax(-float_limits::infinity()) {}

    // TBB method: joins this functor with the given one
    void join (LuminanceFunctor & rhs)
    {
        zero_count += rhs.zero_count;
        Lmin = std::min (Lmin, rhs.Lmin);
        Lmax = std::max (Lmax, rhs.Lmax);
    }

    // Method invoked by TBB: accumulates the data for the subrange
    void operator() (const tbb::blocked_range<size_t> & r)
    {
        // Process the first, non-aligned elements (if any)
        const size_t begin_sse = (r.begin() + 3) & ~static_cast<size_t>(0x3);
        computeScalar(r.begin(), begin_sse);

        // Do the 4x, SSE part
        const size_t end_sse = r.end() & ~static_cast<size_t>(0x3);
        computeSSE(begin_sse, end_sse);

        // Process the rest, also in a scalar way
        computeScalar(end_sse, r.end());
    }

private:
    inline void computeScalar(size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            __m128 dot_tmp = pixels[i] * vec_LUM;
            dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
            dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
            _mm_store_ss(Lw+i, dot_tmp);
            // Flush all negatives, denorms, NaNs and infinity values to 0.0
            if (!isInvalidLuminance(Lw[i])) {
                Lmin = std::min (Lw[i], Lmin);
                Lmax = std::max (Lw[i], Lmax);
            } else {
                Lw[i] = 0.0f;
                ++zero_count;
            }
        }
    }

    inline void computeSSE(size_t begin, size_t end) {

        // Raw luminance SSE loop, doing groups of 4 pixels at a time
        assert (reinterpret_cast<size_t>(Lw + begin) % 16 == 0);

        union { __m128 v; float f[4]; } vec_min;
        union { __m128 v; float f[4]; } vec_max;
        vec_min.v = _mm_set_ps1 (Lmin);
        vec_max.v = _mm_set_ps1 (Lmax);

        for (size_t off = begin; off < end; off += 4)
        {
            // Load the next 4 pixels and transpose them
            __m128 p0 = pixels[off];
            __m128 p1 = pixels[off+1];
            __m128 p2 = pixels[off+2];
            __m128 p3 = pixels[off+3];
            PCG_MM_TRANSPOSE4_PS (p0, p1, p2, p3);

            // Now do the scaling (recall the Rgba32F offsets: a=0, r=3)
            const Rgba32F vec_Lw = 
                (vec_LUM_R*p3) + (vec_LUM_G*p2) + (vec_LUM_B*p1);


            ////////////////////////////////////////////////////////////////////
            // Validation and min/max update, doing groups of 4 pixels at a time
            ////////////////////////////////////////////////////////////////////

            // Create a mask to zero out invalid pixels
            // !(vec_Lw < vec_MINVAL) ? 0xffffffff : 0x0
            __m128 mask_min = _mm_cmpnlt_ps (vec_Lw, vec_MINVAL);

            // (0x7f800000 > vec_Lw) ? 0xffff : 0, then expand to 32 bits
            __m128 mask_nan = _mm_cmpneq_ps(ZERO, 
                _mm_castsi128_ps(_mm_cmpgt_epi32(MASK_NAN,
                                 _mm_castps_si128(vec_Lw))));

            // Combine the masks
            __m128 mask = _mm_and_ps(mask_min, mask_nan);

            // Apply the mask and store the result
            const __m128 result = _mm_and_ps (vec_Lw, mask);
            _mm_store_ps (Lw + off, result);

            // Add the number of zeros
            const int lut_index =  _mm_movemask_ps (mask);
    #if USE_PACKED_LUT
            zero_count += (packed_lut >> (lut_index*4)) & 0xF;
    #else
            zero_count += LUT[lut_index];
    #endif

            // Update the minimum and maximum vectors, only the valid elements
            // SSE4.1 has a nice "blendps" instruction, but I can't it here
            __m128 result_min=_mm_or_ps(result, _mm_andnot_ps(mask, vec_min.v));
            vec_min.v = _mm_min_ps (vec_min.v, result_min);
            __m128 result_max=_mm_or_ps(result, _mm_andnot_ps(mask, vec_max.v));
            vec_max.v = _mm_max_ps (vec_max.v, result_max);
        }

        // Accumulate the result for min & max
        for (int i = 0; i < 4; ++i) {
            Lmin = std::min (Lmin, vec_min.f[i]);
            Lmax = std::max (Lmax, vec_max.f[i]);
        }
    }
};

const float LuminanceFunctor::LUM_R = 0.27f;
const float LuminanceFunctor::LUM_G = 0.67f;
const float LuminanceFunctor::LUM_B = 0.06f;
const Rgba32F LuminanceFunctor::vec_LUM(LUM_R, LUM_G, LUM_B, 0.0f);
const Rgba32F LuminanceFunctor::vec_LUM_R(LUM_R);
const Rgba32F LuminanceFunctor::vec_LUM_G(LUM_G);
const Rgba32F LuminanceFunctor::vec_LUM_B(LUM_B);
const Rgba32F LuminanceFunctor::vec_MINVAL(float_limits::min());
const Rgba32F LuminanceFunctor::ZERO(0.0f);
const __m128i LuminanceFunctor::MASK_NAN(_mm_set1_epi32 (0x7f800000));
#if !USE_PACKED_LUT
const int LuminanceFunctor::LUT[] = 
    { 4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0 };
#endif



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



// This method only accumulates the logarithm of the luminance
float accumulateNoHistogram(const float * PCG_RESTRICT Lw,
                            const float * PCG_RESTRICT Lw_end)
{
    // Will process first elements in SSE fashion, 4 at a time
    const ptrdiff_t count_sse = (Lw_end - Lw) & ~0x3;
    // Prepare Kahan summation with 4 elements
    Rgba32F vec_sum = _mm_setzero_ps();
    Rgba32F vec_c   = _mm_setzero_ps();

    for (ptrdiff_t off = 0; off < count_sse; off += 4)
    {
        const __m128 vec_lum = _mm_load_ps(Lw+off);
        const Rgba32F vec_log_lum = am::log_eps (vec_lum);

        // Update the sum with error compensation
        const Rgba32F y = vec_log_lum - vec_c;
        const Rgba32F t = vec_sum + y;
        vec_c   = (t - vec_sum) - y;
        vec_sum = t;
    }

    // Accumulate the sum and then add the rest of the values (0 up to 3)
    float L_sum;
    {
        __m128 sum_tmp = _mm_hadd_ps(vec_sum, vec_sum);
        sum_tmp = _mm_hadd_ps(sum_tmp, sum_tmp);
        _mm_store_ss(&L_sum, sum_tmp);
    }
    if (count_sse != (Lw_end - Lw)) {
        float c = 0.0f;
        for (const float * lum = Lw+count_sse; lum != Lw_end; ++lum) {
            const float log_lum = logf(*lum);

            // Update the sum with error compensation
            const float y = log_lum - c;
            const float t = L_sum + y;
            c     = (t - L_sum) - y;
            L_sum = t;
        }
    }

    return L_sum;
}


// Accumulate the logarithm of the given array of luminances. It builds an
// histogram and also stores the log-luminances corresponding to the 1 and 99
// percentiles thresholds
float accumulateWithHistogram(const float * PCG_RESTRICT Lw,
                              const float * PCG_RESTRICT Lw_end,
                              const float Lmin, const float Lmax,
                              float &L1, float &L99)
{
    assert (Lmax > Lmin);
    
    Rgba32F rangeHelper(Lmin, Lmax, 1.0);
    rangeHelper = am::log_eps (rangeHelper);
    const float Lmin_log = std::min(rangeHelper.r(), logf(Lmin));
    const float Lmax_log = std::max(rangeHelper.g(), logf(Lmax));

    const int resolution = 100;
    const int dynrange = static_cast<int> (ceil(1e-5 + Lmax_log - Lmin_log));
    const int num_bins = std::min(resolution * dynrange, 0x7FFF);
    std::vector<size_t> histogram(num_bins, 0);

    // This makes sure that epsilon is large enough so that it is not necessary
    // to guard for the corner case where Lmax_log will be mapped to N
    // There must be an analytical way of doing this, but this is decent enough
    float epsilon = 1.9073486328125e-6f;
    {
        const float range = Lmax_log - Lmin_log;
        while (static_cast<int>((num_bins/(epsilon+range)) * range) >= num_bins)
            epsilon *= 2.0f;
    }

    const float res_factor = num_bins / (epsilon + (Lmax_log - Lmin_log));

    // Helpful constants
    const Rgba32F vec_res_factor(res_factor);
    const Rgba32F vec_Lmin_log(Lmin_log);

    // Will process first elements in SSE fashion, 4 at a time
    const ptrdiff_t count_sse = (Lw_end - Lw) & ~0x3;
    // Prepare Kahan summation with 4 elements
    Rgba32F vec_sum = _mm_setzero_ps();
    Rgba32F vec_c   = _mm_setzero_ps();

    for (ptrdiff_t off = 0; off < count_sse; off += 4)
    {
        const __m128 vec_lum = _mm_load_ps(Lw+off);
        const Rgba32F vec_log_lum = am::log_eps (vec_lum);

        // Update the sum with error compensation
        const Rgba32F y = vec_log_lum - vec_c;
        const Rgba32F t = vec_sum + y;
        vec_c   = (t - vec_sum) - y;
        vec_sum = t;

        // Update the histogram
        __m128 idx_temp = vec_res_factor * (vec_log_lum - vec_Lmin_log);
        const __m128i bin_idx = _mm_cvttps_epi32 (idx_temp);
        const int index0 = _mm_extract_epi16(bin_idx, 0*2);
        const int index1 = _mm_extract_epi16(bin_idx, 1*2);
		const int index2 = _mm_extract_epi16(bin_idx, 2*2);
		const int index3 = _mm_extract_epi16(bin_idx, 3*2);

        assert (index0 >= 0 && index0 < num_bins);
        assert (index1 >= 0 && index1 < num_bins);
        assert (index2 >= 0 && index2 < num_bins);
        assert (index3 >= 0 && index3 < num_bins);

        ++histogram[index0];
        ++histogram[index1];
        ++histogram[index2];
        ++histogram[index3];
    }

    // Accumulate the sum and then add the rest of the values (0 up to 3)
    float L_sum;
    {
        __m128 sum_tmp = _mm_hadd_ps(vec_sum, vec_sum);
        sum_tmp = _mm_hadd_ps(sum_tmp, sum_tmp);
        _mm_store_ss(&L_sum, sum_tmp);
    }
    if (count_sse != (Lw_end - Lw)) {
        float c = 0.0f;
        for (const float * lum = Lw+count_sse; lum != Lw_end; ++lum) {
            const float log_lum = logf(*lum);

            // Update the sum with error compensation
            const float y = log_lum - c;
            const float t = L_sum + y;
            c     = (t - L_sum) - y;
            L_sum = t;

            int bin_idx = static_cast<int>(res_factor * (log_lum - Lmin_log));
            assert (bin_idx >= 0 && bin_idx < num_bins);
            ++histogram[bin_idx];
        }
    }

    // Consult the histogram to get the L1 and L99 positions
    _mm_prefetch ((char*)(&histogram[num_bins- 8]),   _MM_HINT_T0);
    _mm_prefetch ((char*)(&histogram[num_bins - 16]), _MM_HINT_T0);
    const float inv_res = (epsilon + (Lmax_log - Lmin_log)) / num_bins;
    const ptrdiff_t count = Lw_end - Lw;
    const ptrdiff_t threshold = static_cast<ptrdiff_t> (0.01 * count);
    for (ptrdiff_t sum = 0, i = histogram.size() - 1; i >= 0; --i) {
        sum += histogram[i];
        if (sum > threshold) {
            L99 = static_cast<float>(i)*inv_res + Lmin_log;
            assert (Lmin_log <= L99 && L99 <= Lmax_log);
            break;
        }
    }
    _mm_prefetch ((char*)(&histogram[0]), _MM_HINT_T0);
    for (ptrdiff_t sum = 0, i = 0; (size_t)i < histogram.size() ; ++i) {
        sum += histogram[i];
        if (sum > threshold) {
            L1 = static_cast<float>(i)*inv_res + Lmin_log;
            assert (Lmin_log <= L1 && L1 <= Lmax_log && L1 <= L99);
            break;
        }
    }

    return L_sum;
}



// Helper function: accumulates the log-luminance beyond a given threshold.
// Returns the accumulation of those log-luminances and stores the number
// of elements added
float sumBeyondThreshold(const float * PCG_RESTRICT const Lw,
                         const float * PCG_RESTRICT const Lw_end,
                         const float lum_cutoff, ptrdiff_t &removed_count)
{
    removed_count = 0;
    const ptrdiff_t count = Lw_end - Lw;
    const ptrdiff_t threshold = static_cast<ptrdiff_t> (0.01 * count);

    // Also use a Kahan summation
    float removed_sum = 0.0f;
    float removed_c = 0.0f;

    const float * PCG_RESTRICT lum = Lw;
    // Iterate until finding the first element
    for ( ; lum != Lw_end && removed_count < threshold; ++lum) {
        if (*lum > lum_cutoff) {
             ++removed_count;
             removed_sum = logf (*lum);
             break;
         }
    }

    // Continue using Kahan
    for ( ; lum != Lw_end && removed_count < threshold; ++lum) {
        if (*lum > lum_cutoff) {
             ++removed_count;
             const float val = logf (*lum);
             const float y = val - removed_c;
             const float t = removed_sum + y;
             removed_c = (t - removed_sum) - y;
             removed_sum = t;
         }
    }
    return removed_sum;
}


} // namespace



Reinhard02::Params
Reinhard02::EstimateParams (const Rgba32F * const pixels, size_t count)
{
    // Allocate the array with the luminances
    afloat_t * PCG_RESTRICT Lw = alloc_align<float> (16, count*sizeof(float));  
    if (Lw == NULL) {
        throw RuntimeException("Couldn't allocate the memory for the "
            "luminance buffer");
    }
    // Use an special auto pointer to get rid of the aligned buffer
    auto_afloat_ptr Lw_autoptr (Lw);

    // Compute the luminance
    size_t zero_count;
    float Lmin, Lmax;
    LuminanceFunctor lumFunctor(pixels, Lw);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, count, 4), lumFunctor);
    zero_count = lumFunctor.zero_count;
    Lmin       = lumFunctor.Lmin;
    Lmax       = lumFunctor.Lmax;

    // Abort if all the values are zero
    if (zero_count == count) {
        return Params(0.0f, 0.0f, 0.0f);
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
        accumulateWithHistogram(Lw_nonzero, Lw_end, Lmin, Lmax, L1, L99)
      : accumulateNoHistogram(Lw_nonzero, Lw_end);

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
   
    return Params(key, l_white, l_w);
}
