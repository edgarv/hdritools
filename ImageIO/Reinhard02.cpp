
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

#include <cstddef>

#include <limits>
#include <vector>
#include <memory>


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



// Helper function to fill the array of luminances. It converts the invalid
// values to zero and returns the maximum, minimum and number of invalid values
void 
computeLuminance_scalar (const Rgba32F* PCG_RESTRICT const pixels, size_t count,
                         afloat_t * PCG_RESTRICT Lw,
                         size_t &zero_count, float &Lmin, float &Lmax)
{
    zero_count = 0;
    Lmin =  float_limits::infinity();
    Lmax = -float_limits::infinity();
    
    const Rgba32F luminance_vec(0.27f, 0.67f, 0.06f, 0.0f);
    for (size_t i = 0; i < count; ++i) {
        __m128 dot_tmp = pixels[i] * luminance_vec;
        dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
        dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
        _mm_store_ss(&Lw[i], dot_tmp);
        // Flush all negatives, denorms, NaNs and infinity values to 0.0
        if (isInvalidLuminance(Lw[i])) {
            Lw[i] = 0.0f;
            ++zero_count;
        } else {
            Lmin = Lw[i] < Lmin ? Lw[i] : Lmin;
            Lmax = Lw[i] > Lmax ? Lw[i] : Lmax;
        }
    }
}

// Taken from the Intrinsics Guide for AVX. The same technique was seen as
// a contribution to gcc by Apple (I think). The original macro in MSVC++ uses
// shuffle instead
#define INTEL_MM_TRANSPOSE4_PS(row0, row1, row2, row3) {  \
            __m128 tmp3, tmp2, tmp1, tmp0;                \
                                                          \
            tmp0   = _mm_unpacklo_ps((row0), (row1));     \
            tmp2   = _mm_unpacklo_ps((row2), (row3));     \
            tmp1   = _mm_unpackhi_ps((row0), (row1));     \
            tmp3   = _mm_unpackhi_ps((row2), (row3));     \
                                                          \
            (row0) = _mm_movelh_ps(tmp0, tmp2);           \
            (row1) = _mm_movehl_ps(tmp2, tmp0);           \
            (row2) = _mm_movelh_ps(tmp1, tmp3);           \
            (row3) = _mm_movehl_ps(tmp3, tmp1);           \
        }



// Idem, but with SSE, operating in 4 pixels at a time
void
computeLuminance (const Rgba32F * PCG_RESTRICT const pixels, const size_t count,
                  afloat_t * PCG_RESTRICT Lw,
                  size_t &zero_count, float &Lmin, float &Lmax)
{
    // Factors for computing the luminance (decent compilers should inline this)
    static const float LUM_R = 0.27f;
    static const float LUM_G = 0.67f;
    static const float LUM_B = 0.06f;

    zero_count = 0;
    Lmin =  float_limits::infinity();
    Lmax = -float_limits::infinity();

    // Helper data
    static const Rgba32F vec_LUM_R = Rgba32F(LUM_R);
    static const Rgba32F vec_LUM_G = Rgba32F(LUM_G);
    static const Rgba32F vec_LUM_B = Rgba32F(LUM_B);
    static const __m128i MASK_NAN  = _mm_set1_epi32 (0x7f800000);
    static const __m128 vec_MINVAL = _mm_set1_ps(float_limits::min());
    static const __m128 ZERO = _mm_setzero_ps();

    // LUT for counting the number of zero-ed elements given the 4x32bit masks
    // where a 0x0 means the element was converted to zero.
    // The highest 4bits contain the count for 0xFF, the lowest for 0x0
#if USE_PACKED_LUT
    static const uint64_t packed_lut = 0x112122312232334ULL;
#else
    static const int LUT[] = { 4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0 };
#endif

    const size_t count_sse = count & ~0x3;

    // Raw luminance SSE loop, doing groups of 4 pixels at a time
    for (size_t off = 0; off < count_sse; off += 4)
    {
        // Load the next 4 pixels and transpose them
        __m128 p0 = pixels[off];
        __m128 p1 = pixels[off+1];
        __m128 p2 = pixels[off+2];
        __m128 p3 = pixels[off+3];
        INTEL_MM_TRANSPOSE4_PS (p0, p1, p2, p3);

        // Now do the scaling (recall the Rgba32F offsets: a=0, r=3)
        const Rgba32F vec_Lw = (vec_LUM_R*p3) + (vec_LUM_G*p2) + (vec_LUM_B*p1);

        // Store. Note that it contains NaN and Inf!
        _mm_store_ps(Lw + off, vec_Lw);
    }

    // Validation and min/max update loop, doing groups of 4 pixels at a time
    union { __m128 v; float f[4]; } vec_min;
    union { __m128 v; float f[4]; } vec_max;
    vec_min.v = _mm_set_ps1 (Lmin);
    vec_max.v = _mm_set_ps1 (Lmax);
    for (size_t off = 0; off < count_sse; off += 4)
    {
        const __m128 vec_Lw = _mm_load_ps(Lw + off);

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
        // SSE4.1 has a nice "blendps" instruction, but I can't use here
        __m128 result_min = _mm_or_ps(result, _mm_andnot_ps(mask, vec_min.v));
        vec_min.v = _mm_min_ps (vec_min.v, result_min);
        __m128 result_max = _mm_or_ps(result, _mm_andnot_ps(mask, vec_max.v));
        vec_max.v = _mm_max_ps (vec_max.v, result_max);
    }

    // Accumulate the result for min & max
    for (int i = 0; i < 4; ++i) {
        Lmin = std::min (Lmin, vec_min.f[i]);
        Lmax = std::max (Lmax, vec_max.f[i]);
    }

    // Do the remaining pixels in the normal way
    if (count != count_sse) {
        static const Rgba32F luminance_vec(LUM_R, LUM_G, LUM_B, 0.0f);
        for (size_t i = count_sse; i < count; ++i) {
            __m128 dot_tmp = pixels[i] * luminance_vec;
            dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
            dot_tmp = _mm_hadd_ps(dot_tmp, dot_tmp);
            _mm_store_ss(Lw+i, dot_tmp);
            // Flush all negatives, denorms, NaNs and infinity values to 0.0
            if (!isInvalidLuminance(Lw[i])) {
                Lmin = Lw[i] < Lmin ? Lw[i] : Lmin;
                Lmax = Lw[i] > Lmax ? Lw[i] : Lmax;
            } else {
                Lw[i] = 0.0f;
                ++zero_count;
            }
        }
    }
}



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



// Accumulate the logarithm of the given array of luminances. It builds an
// histogram and also stores the log-luminances corresponding to the 1 and 99
// percentiles thresholds
float accumulateWithHistogram(const float * PCG_RESTRICT Lw,
                              const float * PCG_RESTRICT Lw_end,
                              const float Lmin_log, const float Lmax_log,
                              float &L1, float &L99)
{
    const int resolution = 100;
    const int dynrange = static_cast<int> (ceil(1e-5 + Lmax_log - Lmin_log));
    const int num_bins = resolution * dynrange;
    std::vector<size_t> histogram(num_bins, 0);

    // This makes sure that epsilon is large enough so that it is not necessary
    // to guard for the corner case where Lmax_log will be mapped to N
    // There must be an analytical way of doing this, but this is decent enough
    float epsilon = 1.9073486328125e-6f;
    while (static_cast<int>((num_bins/(epsilon+(Lmax_log-Lmin_log))) * 
        (Lmax_log-Lmin_log)) >= num_bins) epsilon *= 2.0f;

    const float res_factor = num_bins / (epsilon + (Lmax_log - Lmin_log));
    // Use a Kahan summation
    float L_sum   = 0.0f;
    float L_sum_c = 0.0f;
    for (const float * PCG_RESTRICT lum = Lw; lum != Lw_end; ++lum) {
        const float log_lum = logf(*lum);
        const float y = log_lum  - L_sum_c;
        const float t = L_sum + y;
        L_sum_c = (t - L_sum) - y;
        L_sum = t;
        const int bin_idx = static_cast<int>(res_factor * (log_lum - Lmin_log));
        assert (bin_idx >= 0 && bin_idx < num_bins);
        ++histogram[bin_idx];
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
    computeLuminance (pixels, count, Lw, zero_count, Lmin, Lmax);

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
    float L_sum = accumulateWithHistogram(Lw + nonzero_off, Lw + count,
        Lmin_log, Lmax_log, L1, L99);

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
