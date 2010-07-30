
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
    return floatToBits(x)>=0x7f800000u || x < std::numeric_limits<float>::min();
#endif
}

} // namespace



Reinhard02::Params
Reinhard02::EstimateParams (const Rgba32F * pixels, size_t count)
{
    // Allocate the array with the luminances
    float *luminances = alloc_align<float> (16, count*sizeof(float));  
    if (luminances == NULL) {
        throw RuntimeException("Couldn't allocate the memory for the "
            "luminance buffer");
    }
    afloat_t * PCG_RESTRICT Lw = luminances;

    // Compute the luminance
    size_t zero_count = 0;
    float Lmin =  std::numeric_limits<float>::infinity();
    float Lmax = -std::numeric_limits<float>::infinity();
    
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

    size_t nonzero_off = 0;
    if (zero_count == count) {
        // Abort if all the values are zero
        free_align (luminances);
        return Params(0.0f, 0.0f, 0.0f);
    } else if (zero_count != 0) {
        // Compact the array
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
    }


    // Build a histogram to extract the key using percentiles 1 to 99
    const float Lmin_log = logf (Lmin);
    const float Lmax_log = logf (Lmax);
    const int resolution = 100;
    const int dynrange = static_cast<int> (ceil(1e-5 + Lmax_log - Lmin_log));
    const int num_bins = resolution * dynrange;
    std::vector<size_t> histogram(num_bins, 0);

    float L1  = Lmin_log;
    float L99 = Lmax_log;

    // This makes sure that epsilon is large enough so that it is not necessary
    // to guard for the corner case where Lmax_log will be mapped to N
    // There must be an analytical way of doing this, but is decent enough
    float epsilon = 1.9073486328125e-6f;
    while (static_cast<int>((num_bins/(epsilon+(Lmax_log-Lmin_log))) * 
        (Lmax_log-Lmin_log)) >= num_bins) epsilon *= 2.0f;

    const float res_factor = num_bins / (epsilon + (Lmax_log - Lmin_log));
    // Use a Kahan summation
    float L_sum;
    float L_sum_c = 0.0f;
    {
        const float log_lum = logf(*(Lw + nonzero_off));
        L_sum = log_lum;
        const int bin_idx = static_cast<int>(res_factor * (log_lum - Lmin_log));
        assert (bin_idx >= 0 && bin_idx < num_bins);
        ++histogram[bin_idx];
    }
    for (float * PCG_RESTRICT lum = Lw+nonzero_off+1; lum != Lw+count; ++lum) {
        const float log_lum = logf(*lum);
        const float y = log_lum  - L_sum_c;
        const float t = L_sum + y;
        L_sum_c = (t - L_sum) - y;
        L_sum = t;
        const int bin_idx = static_cast<int>(res_factor * (log_lum - Lmin_log));
        assert (bin_idx >= 0 && bin_idx < num_bins);
        ++histogram[bin_idx];
    }

    const float inv_res = (epsilon + (Lmax_log - Lmin_log)) / num_bins;
    const ptrdiff_t threshold = static_cast<ptrdiff_t> (0.01 * count);
    for (ptrdiff_t sum = 0, i = histogram.size() - 1; i >= 0; --i) {
        sum += histogram[i];
        if (sum > threshold) {
            L99 = static_cast<float>(i)*inv_res + Lmin_log;
            assert (Lmin_log <= L99 && L99 <= Lmax_log);
            break;
        }
    }
    for (ptrdiff_t sum = 0, i = 0; (size_t)i < histogram.size() ; ++i) {
        sum += histogram[i];
        if (sum > threshold) {
            L1 = static_cast<float>(i)*inv_res + Lmin_log;
            assert (Lmin_log <= L1 && L1 <= Lmax_log && L1 <= L99);
            break;
        }
    }

    // Remove the value from the logaritmic total L_sum 
    // if log(luminance) > L99_real ---> luminance > exp(L99_real)
    // where L99_real = exp(L99)
    // We know for sure that all such values are in the last percentile, so
    // can avoid reading everything
    ptrdiff_t removed_count = 0;
    {
        const float lum_cutoff = expf (expf (L99));

        // Also use a Kahan summation
        float removed_sum = 0.0f;
        float removed_c = 0.0f;

        float * PCG_RESTRICT lum = Lw + nonzero_off;
        // Iterate until finding the first element
        for ( ; lum != Lw + count && removed_count < threshold; ++lum) {
            if (*lum > lum_cutoff) {
                 ++removed_count;
                 removed_sum = logf (*lum);
                 break;
             }
        }

        // Continue using Kahan
        for ( ; lum != Lw + count && removed_count < threshold; ++lum) {
            if (*lum > lum_cutoff) {
                 ++removed_count;
                 const float val = logf (*lum);
                 const float y = val - removed_c;
                 const float t = removed_sum + y;
                 removed_c = (t - removed_sum) - y;
                 removed_sum = t;
             }
        }
        L_sum -= removed_sum;
    }

    // The temporary buffer is no longer necessary
    free_align (luminances);
    luminances = NULL;
    Lw = NULL;

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
