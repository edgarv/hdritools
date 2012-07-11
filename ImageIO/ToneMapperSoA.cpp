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

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

#include "ToneMapperSoA.h"
#include "StdAfx.h"
#include "ToneMapper.h"
#include "ImageSoA.h"
#include "ImageIterators.h"
#include "Vec4f.h"
#include "Vec4i.h"

#if PCG_USE_AVX
# include "Vec8f.h"
# include "Vec8i.h"
#endif


// FIXME Make this a setup flag
#if !defined(USE_SSE_POW)
#define USE_SSE_POW 1
#endif

#define USE_VECTOR4_ITERATOR 1

#if defined(__clang__)
# define MAY_BE_UNUSED __attribute ((unused))
#else
# define MAY_BE_UNUSED
#endif


#if USE_SSE_POW
namespace ssemath
{
#include "sse_mathfun.h"
}
#endif


#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <algorithm>
#include <iterator>

#include <cassert>


namespace
{

namespace ops
{

// Computes the reciprocal
template <typename T>
inline T rcp(const T& x)
{
    return 1.0f / x;
}

// Use the SSE approximation and a Newton-Rhapson step:
// [2 * rcpps(x) - (x * rcpps(x) * rcpps(x))]
template <>
inline pcg::Vec4f rcp(const pcg::Vec4f& x)
{
    return rcp_nr(x);
}



template <typename T>
inline T pow(const T& x, const T& y)
{
    return ::pow(x, y);
}

template <>
inline pcg::Vec4f pow(const pcg::Vec4f& x, const pcg::Vec4f& y)
{
#if USE_SSE_POW
    const pcg::Vec4f result =
        ssemath::exp_ps(static_cast<pcg::Vec4f>(ssemath::log_ps(x)) * y);
#else
    const pcg::Vec4f result(::pow(x[3], y[3]), ::pow(x[2], y[2]),
        ::pow(x[1], y[1]), ::pow(x[0], y[0]));
#endif
    return result;
}



template <typename IntT, typename T>
inline IntT round(const T& x)
{
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    const static T OFFSET = T(0.5f);
    return static_cast<IntT> (x < 0 ? x - OFFSET : x + OFFSET);
#else
    return static_cast<IntT> (roundf(x));
#endif
}

// Note that this does not take into account overflow!
template <>
inline pcg::Vec4i round(const pcg::Vec4f& x)
{
    return _mm_cvtps_epi32(x);
}



// Select
// (a OP b) ? c : d
template <typename T>
inline T select_gt(const T& a, const T& b, const T& c, const T& d)
{
    return (a > b) ? c : d;
}

template <>
inline pcg::Vec4f select_gt(const pcg::Vec4f& a, const pcg::Vec4f& b,
    const pcg::Vec4f& c, const pcg::Vec4f& d)
{
    return select(a > b, c, d);
}



template <typename T>
inline T min(const T& a, const T& b) {
    return std::min(a, b);
}

template <>
inline pcg::Vec4f min(const pcg::Vec4f& a, const pcg::Vec4f& b) {
    return simd_min(a, b);
}



template <typename T>
inline T max(const T& a, const T& b) {
    return std::max(a, b);
}

template <>
inline pcg::Vec4f max(const pcg::Vec4f& a, const pcg::Vec4f& b) {
    return simd_max(a, b);
}



template <typename T>
inline T clamp(const T& x, const T& minValue, const T& maxValue) {
    return ops::max(ops::min(x, maxValue), minValue);
}



#if PCG_USE_AVX

template <>
inline pcg::Vec8f rcp(const pcg::Vec8f& x)
{
    return rcp_nr(x);
}

template <>
inline pcg::Vec8f pow(const pcg::Vec8f& x, const pcg::Vec8f& y)
{
#if USE_SSE_POW
    // The current implementation doesn't support AVX natively
    const pcg::Vec4f x0 = _mm256_castps256_ps128(x);
    const pcg::Vec4f y0 = _mm256_castps256_ps128(y);
    const pcg::Vec4f x1 = _mm256_extractf128_ps(x, 1);
    const pcg::Vec4f y1 = _mm256_extractf128_ps(y, 1);
    
    const pcg::Vec4f r0 = ssemath::exp_ps(ssemath::log_ps(x0) * y0);
    const pcg::Vec4f r1 = ssemath::exp_ps(ssemath::log_ps(x1) * y1);

    __m256 result = _mm256_insertf128_ps(_mm256_castps128_ps256(r0), r1, 1);
    return result;
#else
    const pcg::Vec8f result(
        ::pow(x[7], y[7]), ::pow(x[6], y[6]),
        ::pow(x[5], y[5]), ::pow(x[4], y[4]),
        ::pow(x[3], y[3]), ::pow(x[2], y[2]),
        ::pow(x[1], y[1]), ::pow(x[0], y[0]));
#endif
    return result;
}

template <>
inline pcg::Vec8i round(const pcg::Vec8f& x)
{
    return _mm256_cvtps_epi32(x);
}

template <>
inline pcg::Vec8f select_gt(const pcg::Vec8f& a, const pcg::Vec8f& b,
    const pcg::Vec8f& c, const pcg::Vec8f& d)
{
    return select(a > b, c, d);
}

template <>
inline pcg::Vec8f min(const pcg::Vec8f& a, const pcg::Vec8f& b) {
    return simd_min(a, b);
}


template <>
inline pcg::Vec8f max(const pcg::Vec8f& a, const pcg::Vec8f& b) {
    return simd_max(a, b);
}

#endif // PCG_USE_AVX

} // namespace ops



// ******************************************************

/**
 * A tone mapping kernel is a composition of several concepts, which may be
 * interchangeable as long as they follow the appropriate semantics:
 *
 * 0. Pixel type concept
 * This represent a single component of HDR pixels. It may be a scalar or a
 * vector type. In the later all operations are defined component-wise.
 *
 *   Requirements
 *   --------------------------------------------------------------------------
 *     Pseudo-signature | Semantics
 *   --------------------------------------------------------------------------
 *   T::T( const T& )     Copy constructor
 *   T::T( float x )      Initialize from a single float scalar
 *
 *
 * 1. Luminance Scaler Concept
 * Functor which scales the luminance on the input linear HDR pixel, assuming
 * sRGB primaries, aiming to reduce the dynamic range towards the range [0,1]
 *
 *   Requirements
 *   --------------------------------------------------------------------------
 *     Pseudo-signature | Semantics
 *   --------------------------------------------------------------------------
 *   L::L( const L& )    Copy constructor
 *   L::~L()             Destructor
 *   void L::opeator() ( const T& r, const T& g const T& b,
                        T& rOut, T& gOut, T& bOut ) const
 *                       Scales the input HDR pixel elements and writes the
 *                       scaled values.
 *
 *
 * 2. Clamper [0,1] Concept
 * Functor which clamps a single element to the range [0,1]
 *
 *   Requirements
 *   --------------------------------------------------------------------------
 *     Pseudo-signature | Semantics
 *   --------------------------------------------------------------------------
 *   C::C( const C& )    Copy constructor
 *   C::~C()             Destructor
 *   T operator() ( const T& ) const
 *                       Returns the clamped version of the input
 *
 *
 * 3. Display Transformer Concept
 * Functor which transform a linear value in the range [0,1] according to a
 * non-linear transfer function.
 *
 *   Requirements
 *   --------------------------------------------------------------------------
 *     Pseudo-signature | Semantics
 *   --------------------------------------------------------------------------
 *   D::D( const D& )    Copy constructor
 *   D::~D()             Destructor
 *   T operator() ( const T& ) const
 *                       Returns the transformed version of the input.
 *                       The result is also in the range [0,1]
 *
 *
 * 4. Quantizer Concept
 * Functor which takes a floating point value in the range [0,1] and quantizes
 * it to an integral type.
 *
 *   Requirements
 *   --------------------------------------------------------------------------
 *     Pseudo-signature | Semantics
 *   --------------------------------------------------------------------------
 *   Q::Q( const Q& )    Copy constructor
 *   Q::~Q()             Destructor
 *   Q::value_t          typedef with the target integral type
 *   Q::value_t operator() ( const T& ) const
 *                       Returns the quantized version of the input
 *
 *
 * 5. Pixel Assembler concept
 * Functor which takes quantized R,G,B values and packs them into an LDR pixel.
 *
 *   Requirements
 *   --------------------------------------------------------------------------
 *     Pseudo-signature | Semantics
 *   --------------------------------------------------------------------------
 *   P::P( const P& )    Copy constructor
 *   P::~P()             Destructor
 *   P::pixel_t          typedef with the target packed LDR pixel type
 *   P::quantizer_t      typedef with the quantizer used to generate pixels
 *   P::value_t          typedef with the type generated by the quantizer
 *   void operator() (
 *      const P::value_t& r,
 *      const P::value_t& g,
 *      const P::value_t& b,
 *      const P::value_t& a,
 *      P:pixel_t& outPixel ) const
 *                       Takes the quantized pixels and saves the packed
 *                       LDR pixel into outPixel.
 */




// Constants for SSE Code. Not the prettiest approach, however the static
// local variables generate inefficient code with non-scalar types as
// it add guards to know when to execute those methods. Compile-time
// constants for integral types are much simpler than this.
namespace constants
{

// Helper function to set either a scalar or a float from a Vec4fUnion
template <typename T>
inline T getValue(const pcg::Vec4fUnion& value);

template <>
inline float MAY_BE_UNUSED getValue(const pcg::Vec4fUnion& value) {
    return value.f[0];
}

template <>
inline pcg::Vec4f getValue(const pcg::Vec4fUnion& value) {
    return pcg::Vec4f(value);
}


#if PCG_USE_AVX

// Helper function to set either a scalar or a float from a Vec8fUnion
template <typename T>
inline T getValue(const pcg::Vec8fUnion& value);

template <>
inline float getValue(const pcg::Vec8fUnion& value) {
    return value.f[0];
}

template <>
inline pcg::Vec4f getValue(const pcg::Vec8fUnion& value) {
    return pcg::Vec4f(value.xmm[0]);
}

template <>
inline pcg::Vec8f getValue(const pcg::Vec8fUnion& value) {
    return pcg::Vec8f(value.ymm);
}

#endif // PCG_USE_AVX



// Writing manually the same constant many times is error prone
#if PCG_USE_AVX
#define PCG_TMOSOA_VECF(x) {x, x, x, x, x, x, x, x}
typedef pcg::Vec8fUnion VecfUnion;
#else
#define PCG_TMOSOA_VECF(x) {x, x, x, x}
typedef pcg::Vec4fUnion VecfUnion;
#endif

static const VecfUnion ZERO = {PCG_TMOSOA_VECF( 0.0f )};
static const VecfUnion ONE  = {PCG_TMOSOA_VECF( 1.0f )};
    
static const VecfUnion Q_8bit  = {PCG_TMOSOA_VECF(   255.0f )};
static const VecfUnion Q_16bit = {PCG_TMOSOA_VECF( 65535.0f )};

// Luminance conversion
static const VecfUnion LVec[3] = {
    {PCG_TMOSOA_VECF( 0.212639005871510f )},
    {PCG_TMOSOA_VECF( 0.715168678767756f )},
    {PCG_TMOSOA_VECF( 0.072192315360734f )}
};

// Rational approximation for sRGB P/Q order 4/4
static const VecfUnion Remez44_P[5] = {
    {PCG_TMOSOA_VECF(    -0.01997304708470295f )},
    {PCG_TMOSOA_VECF(    24.95173169159651f )},
    {PCG_TMOSOA_VECF(  3279.752175439042f )},
    {PCG_TMOSOA_VECF( 39156.546674561556f )},
    {PCG_TMOSOA_VECF( 42959.451119871745f )}
};
static const VecfUnion Remez44_Q[5] = {
    {PCG_TMOSOA_VECF(     1.f )},
    {PCG_TMOSOA_VECF(   361.5384894448744f )},
    {PCG_TMOSOA_VECF( 13090.206953080155f  )},
    {PCG_TMOSOA_VECF( 55800.948825871434f  )},
    {PCG_TMOSOA_VECF( 16180.833742684188f  )}
};

// Rational approximation for sRGB P/Q order 7/7
static const VecfUnion Remez77_P[8] = {
    {PCG_TMOSOA_VECF(-0.031852703288410084f )},
    {PCG_TMOSOA_VECF( 1.8553896638433446e1f )},
    {PCG_TMOSOA_VECF( 2.20060672110147e4f   )},
    {PCG_TMOSOA_VECF( 2.635850360294788e6f  )},
    {PCG_TMOSOA_VECF( 7.352843882592331e7f  )},
    {PCG_TMOSOA_VECF( 5.330866283442694e8f  )},
    {PCG_TMOSOA_VECF( 9.261676939514283e8f  )},
    {PCG_TMOSOA_VECF( 2.632919307024597e8f  )}
};
static const VecfUnion Remez77_Q[8] = {
    {PCG_TMOSOA_VECF( 1.f )},
    {PCG_TMOSOA_VECF( 1.2803496360781705e3f )},
    {PCG_TMOSOA_VECF( 2.740075886695005e5f  )},
    {PCG_TMOSOA_VECF( 1.4492562384924464e7f )},
    {PCG_TMOSOA_VECF( 2.1029015319992256e8f )},
    {PCG_TMOSOA_VECF( 8.142158667694515e8f  )},
    {PCG_TMOSOA_VECF( 6.956059106558038e8f  )},
    {PCG_TMOSOA_VECF( 6.3853076877794705e7f )}
};

static const VecfUnion SRGB_Cutoff    = {PCG_TMOSOA_VECF( 0.003041229589676f )};
static const VecfUnion SRGB_FactorHi  = {PCG_TMOSOA_VECF( 1.055f )};
static const VecfUnion SRGB_Exponent  = {PCG_TMOSOA_VECF( 0.4166666667f )};
static const VecfUnion SRGB_Offset    = {PCG_TMOSOA_VECF( 0.055f )};
static const VecfUnion SRGB_FactorLow = {PCG_TMOSOA_VECF( 12.92f )};


// Remove the temporary macro
#undef PCG_TMOSOA_VECF


} // namespace constants




// Simple scaler which only multiplies all pixels by a constant
template <typename T>
struct LuminanceScaler_Exposure
{
    typedef T value_t;

    inline void operator() (
        const T& rLinear, const T& gLinear, const T& bLinear,
        T& rOut, T& gOut, T& bOut) const
    {
        rOut = m_multiplier * rLinear;
        gOut = m_multiplier * gLinear;
        bOut = m_multiplier * bLinear;
    }

    // Scales each pixel by multiplier
    inline void setExposureFactor(float multiplier)
    {
        m_multiplier = T(multiplier);
    }

private:
    T m_multiplier;
};



// Applies the global Reinhard-2002 TMO. The parameters are calculated
// separately by a different process.
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
template <typename T>
struct LuminanceScaler_Reinhard02
{
    typedef T value_t;

    // Initial values as for key = 0.18, avgLogLum = 0.18, Lwhite = 1.0f
    LuminanceScaler_Reinhard02() :
    m_P(1.0f), m_Q(1.0f)
    {}

    // Setup the internal constants
    inline void SetParams(const pcg::Reinhard02::Params &params)
    {
        m_P = T(params.key / params.l_w);
        m_Q = T(1.0f / (params.l_white*params.l_white));
    }

    inline void operator() (
        const T& rLinear, const T& gLinear, const T& bLinear,
        T& rOut, T& gOut, T& bOut) const
    {
        // Get the luminance
        const T Y = LVec[0]*rLinear + LVec[1]*gLinear + LVec[2]*bLinear;

        // Compute the scale
        const T Lp = m_P * Y;
        const T k = (m_P * (ONE + m_Q*Lp)) * ops::rcp(ONE + Lp);

        // And apply
        rOut = k * rLinear;
        gOut = k * gLinear;
        bOut = k * bLinear;
    }

private:
    T m_P;
    T m_Q;

    static const T ONE;
    static const T LVec[3];
};
template <typename T>
const T LuminanceScaler_Reinhard02<T>::ONE = constants::getValue<T>(constants::ONE);
template <typename T>
const T LuminanceScaler_Reinhard02<T>::LVec[3] = {
    constants::getValue<T>(constants::LVec[0]),
    constants::getValue<T>(constants::LVec[1]),
    constants::getValue<T>(constants::LVec[2])
};



template <typename T>
struct Clamper01
{
    inline T operator() (const T& x) const
    {
        return ops::clamp(x, ZERO, ONE);
    }

    static const T ONE;
    static const T ZERO;
};
template <typename T>
const T Clamper01<T>::ONE  = constants::getValue<T>(constants::ONE);
template <typename T>
const T Clamper01<T>::ZERO = constants::getValue<T>(constants::ZERO);


// Raises each pixel (already in [0,1]) to 1/gamma. A typical value for gamma
// and current LCD screens in 2.2. Gamma has to be greater than zero.
template <typename T>
struct DisplayTransformer_Gamma
{
    DisplayTransformer_Gamma() :
    m_invGamma(1.0f/2.2f)
    {}

    DisplayTransformer_Gamma(float invGamma) : m_invGamma(invGamma)
    {
        assert(invGamma > 0);
    }

    inline void setInvGamma(float invGamma)
    {
        assert(invGamma > 0);
        m_invGamma = T(invGamma);
    }

    inline T operator() (const T& x) const
    {
        return ops::pow(x, m_invGamma);
    }


private:
    T m_invGamma;
};



template <typename T>
struct SRGB_NonLinear_Ref
{
    SRGB_NonLinear_Ref() {}

    inline T operator() (const T& x) const
    {
        T r = FACTOR * ops::pow(x, EXPONENT) - OFFSET;
        return r;
    }

    static const T FACTOR;
    static const T EXPONENT;
    static const T OFFSET;
};
template <typename T>
const T SRGB_NonLinear_Ref<T>::FACTOR =
    constants::getValue<T>(constants::SRGB_FactorHi);
template <typename T>
const T SRGB_NonLinear_Ref<T>::EXPONENT =
    constants::getValue<T>(constants::SRGB_Exponent);
template <typename T>
const T SRGB_NonLinear_Ref<T>::OFFSET =
    constants::getValue<T>(constants::SRGB_Offset);



// Rational approximation which should be good enough for 8-bit quantizers
template <typename T>
struct SRGB_NonLinear_Remez44
{
    SRGB_NonLinear_Remez44() {}

    inline T operator() (const T& x) const
    {    
        const T num = (P[0] + x*(P[1] + x*(P[2] + x*(P[3] + P[4]*x))));
        const T den = (Q[0] + x*(Q[1] + x*(Q[2] + x*(Q[3] + Q[4]*x))));
        const T result = num * ops::rcp(den);
        return result;
    }

private:
    static const T P[5];
    static const T Q[5];
};
template <typename T>
const T SRGB_NonLinear_Remez44<T>::P[5] = {
     constants::getValue<T>(constants::Remez44_P[0]),
     constants::getValue<T>(constants::Remez44_P[1]),
     constants::getValue<T>(constants::Remez44_P[2]),
     constants::getValue<T>(constants::Remez44_P[3]),
     constants::getValue<T>(constants::Remez44_P[4])
};
template <typename T>
const T SRGB_NonLinear_Remez44<T>::Q[5] = {
     constants::getValue<T>(constants::Remez44_Q[0]),
     constants::getValue<T>(constants::Remez44_Q[1]),
     constants::getValue<T>(constants::Remez44_Q[2]),
     constants::getValue<T>(constants::Remez44_Q[3]),
     constants::getValue<T>(constants::Remez44_Q[4])
};



// Rational approximation which should be good enough for 16-bit quantizers
template <typename T>
struct SRGB_NonLinear_Remez77
{
    SRGB_NonLinear_Remez77() {}

    inline T operator() (const T& x) const
    {
        const T num = (P[0] + x*(P[1] + x*(P[2] + x*(P[3] +
                                  x*(P[4] + x*(P[5] + x*(P[6] + P[7]*x)))))));
        const T den = (Q[0] + x*(Q[1] + x*(Q[2] + x*(Q[3] +
                                  x*(Q[4] + x*(Q[5] + x*(Q[6] + Q[7]*x)))))));
        const T result = num * ops::rcp(den);
        return result;
    }

private:
    static const T P[8];
    static const T Q[8];
};
template <typename T>
const T SRGB_NonLinear_Remez77<T>::P[8] = {
     constants::getValue<T>(constants::Remez77_P[0]),
     constants::getValue<T>(constants::Remez77_P[1]),
     constants::getValue<T>(constants::Remez77_P[2]),
     constants::getValue<T>(constants::Remez77_P[3]),
     constants::getValue<T>(constants::Remez77_P[4]),
     constants::getValue<T>(constants::Remez77_P[5]),
     constants::getValue<T>(constants::Remez77_P[6]),
     constants::getValue<T>(constants::Remez77_P[7])
};
template <typename T>
const T SRGB_NonLinear_Remez77<T>::Q[8] = {
     constants::getValue<T>(constants::Remez77_Q[0]),
     constants::getValue<T>(constants::Remez77_Q[1]),
     constants::getValue<T>(constants::Remez77_Q[2]),
     constants::getValue<T>(constants::Remez77_Q[3]),
     constants::getValue<T>(constants::Remez77_Q[4]),
     constants::getValue<T>(constants::Remez77_Q[5]),
     constants::getValue<T>(constants::Remez77_Q[6]),
     constants::getValue<T>(constants::Remez77_Q[7])
};



// Actual sRGB implementation, with the non-linear functor as a template
template <typename T, template<typename> class SRGB_NonLinear>
struct DisplayTransformer_sRGB
{
    DisplayTransformer_sRGB() {}

    inline T operator() (const T& pLinear) const
    {
        T p = m_nonlinear(pLinear);

        // Here comes the blend
        T result = ops::select_gt(pLinear, CUTOFF_sRGB, p, FACTOR * pLinear);
        return result;
    }

private:
    SRGB_NonLinear<T> m_nonlinear;
    static const T CUTOFF_sRGB;
    static const T FACTOR;
};
template <typename T, template<typename> class SRGB_NonLinear>
const T DisplayTransformer_sRGB<T, SRGB_NonLinear>::CUTOFF_sRGB =
    constants::getValue<T>(constants::SRGB_Cutoff);
template <typename T, template<typename> class SRGB_NonLinear>
const T DisplayTransformer_sRGB<T, SRGB_NonLinear>::FACTOR =
    constants::getValue<T>(constants::SRGB_FactorLow);



// Workaround to template aliases (introduced in C++11)
template <typename T>
struct Display_sRGB_Ref {
    typedef DisplayTransformer_sRGB<T, SRGB_NonLinear_Ref> display_t;
};

template <typename T>
struct Display_sRGB_Fast1 {
    typedef DisplayTransformer_sRGB<T, SRGB_NonLinear_Remez77> display_t;
};

template <typename T>
struct Display_sRGB_Fast2 {
    typedef DisplayTransformer_sRGB<T, SRGB_NonLinear_Remez44> display_t;
};


template <typename T, typename QT>
struct Quantizer8bit
{
    typedef QT value_t;

    inline value_t operator() (const T& x) const
    {
        return ops::round<QT>(FACTOR * x);
    }

private:
    static const T FACTOR;
};
template <typename T, typename QT>
const T Quantizer8bit<T,QT>::FACTOR = constants::getValue<T>(constants::Q_8bit);


template <typename T, typename QT>
struct Quantizer16bit
{
    typedef QT value_t;

    inline value_t operator() (const T& x) const
    {
        return ops::round<QT>(FACTOR * x);
    }

private:
    static const T FACTOR;
};
template <typename T, typename QT>
const T Quantizer16bit<T,QT>::FACTOR=constants::getValue<T>(constants::Q_16bit);






struct PixelAssembler_BGRA8
{
    typedef pcg::PixelBGRA8 pixel_t;
    typedef Quantizer8bit<float, uint8_t> quantizer_t;
    typedef quantizer_t::value_t value_t;

    inline void operator() (
        const value_t& r, const value_t& g, const value_t& b, const value_t& a,
        pixel_t& outPixel) const
    {
        outPixel.argb = (a << 24) | (r << 16) | (g << 8) | (b);
    }
};





struct PixelAssembler_BGRA8Vec4
{
    typedef pcg::PixelBGRA8Vec4 pixel_t;
    typedef Quantizer8bit<pcg::Vec4f, pcg::Vec4i> quantizer_t;
    typedef quantizer_t::value_t value_t;

    inline void operator() (
        const value_t& r, const value_t& g, const value_t& b, const value_t& a,
        pixel_t& outPixel) const
    {
        // For some stupid reason the operator<< overload doesn't seem to work
        pcg::Vec4i aShift = _mm_slli_epi32(a, 24);
        pcg::Vec4i rShift = _mm_slli_epi32(r, 16);
        pcg::Vec4i gShift = _mm_slli_epi32(g, 8);

        const pcg::Vec4i pixel = aShift | rShift | gShift | b;
        _mm_stream_si128(&outPixel.xmm, pixel);
    }
};



#if PCG_USE_AVX

struct PixelAssembler_BGRA8Vec8
{
    typedef pcg::PixelBGRA8Vec8 pixel_t;
    typedef Quantizer8bit<pcg::Vec8f, pcg::Vec8i> quantizer_t;
    typedef quantizer_t::value_t value_t;

    template <int offset>
    static inline pcg::Vec4i buildExtract(const value_t& r, const value_t& g,
        const value_t& b, const value_t& a)
    {
        assert(offset == 0 || offset == 1);

        pcg::Vec4i a0 = _mm256_extractf128_si256(a, offset);
        pcg::Vec4i r0 = _mm256_extractf128_si256(r, offset);
        pcg::Vec4i g0 = _mm256_extractf128_si256(g, offset);
        pcg::Vec4i b0 = _mm256_extractf128_si256(b, offset);

        pcg::Vec4i a0Shift = _mm_slli_epi32(a0, 24);
        pcg::Vec4i r0Shift = _mm_slli_epi32(r0, 16);
        pcg::Vec4i g0Shift = _mm_slli_epi32(g0, 8);

        const pcg::Vec4i pixel = a0Shift | r0Shift | g0Shift | b0;
        return pixel;
    }

    inline void operator() (
        const value_t& r, const value_t& g, const value_t& b, const value_t& a,
        pixel_t& outPixel) const
    {
        // Adequate support for integer operation arrives with AVX2, in the
        // meantime extract 2 128-bit values
        const pcg::Vec4i pix0 = buildExtract<0>(r, g, b, a);
        const pcg::Vec4i pix1 = buildExtract<1>(r, g, b, a);

        // Casting sets the lower bits, then insert the higher ones
        const __m256i pixel =
            _mm256_insertf128_si256(_mm256_castsi128_si256(pix0), pix1, 1);

        _mm256_stream_si256(&outPixel.ymm, pixel);
    }
};

#endif // PCG_USE_AVX




template<class LuminanceScaler, class DisplayTransformer, class PixelAssembler>
struct ToneMappingKernel
{
    typedef typename LuminanceScaler::value_t value_t;

    ToneMappingKernel(const LuminanceScaler& scaler,
        const DisplayTransformer& display, const PixelAssembler& assembler) :
    luminanceScaler(scaler), displayTransformer(display),
    pixelAssembler(assembler)
    {}

    inline void operator() (
        const value_t& rLinear, const value_t& gLinear, const value_t& bLinear,
        const value_t& alpha, typename PixelAssembler::pixel_t& pixelOut) const
    {
        value_t rScaled, gScaled, bScaled;

        // Scale the luminance according to the current settings
        luminanceScaler(rLinear, gLinear, bLinear,
            rScaled, gScaled, bScaled);

        // Clamp to [0,1]
        const value_t rClamped = clamper(rScaled);
        const value_t gClamped = clamper(gScaled);
        const value_t bClamped = clamper(bScaled);
        const value_t aClamped = clamper(alpha);

        // Nonlinear display transform
        const value_t rDisplay = displayTransformer(rClamped);
        const value_t gDisplay = displayTransformer(gClamped);
        const value_t bDisplay = displayTransformer(bClamped);

        // Quantize the values
        const typename PixelAssembler::value_t rQ = quantizer(rDisplay);
        const typename PixelAssembler::value_t gQ = quantizer(gDisplay);
        const typename PixelAssembler::value_t bQ = quantizer(bDisplay);
        const typename PixelAssembler::value_t aQ = quantizer(aClamped);

        pixelAssembler(rQ, gQ, bQ, aQ, pixelOut);
    }

    // Functors which implement the actual functionality
    const LuminanceScaler& luminanceScaler;
    const DisplayTransformer& displayTransformer;
    const PixelAssembler& pixelAssembler;

    Clamper01<value_t> clamper;
    typename PixelAssembler::quantizer_t quantizer;
};


// Move the processing here, to avoid having way too many parameters
template <typename SourceIter, typename DestIter, class Kernel>
class ProcessorTBB
{
public:
    ProcessorTBB(SourceIter src, DestIter dest, const Kernel &k) :
    m_src(src), m_dest(dest), m_kernel(k)
    {}

    void operator() (tbb::blocked_range<SourceIter>& range) const
    {
        DestIter dest = m_dest + (range.begin() - m_src);
        for (SourceIter it = range.begin(); it != range.end(); ++it, ++dest) {
            typename std::iterator_traits<SourceIter>::value_type pixel = *it;
            m_kernel(pixel.r(), pixel.g(), pixel.b(), pixel.a(), *dest);
        }
    }

private:
    SourceIter m_src;
    DestIter   m_dest;
    const Kernel& m_kernel;
};



template <class Kernel, typename SourceIter, typename DestIter>
void processPixels(const Kernel& kernel, 
    SourceIter begin, SourceIter end, DestIter dest)
{
    ProcessorTBB<SourceIter, DestIter, Kernel> pTBB(begin, dest, kernel);
    tbb::blocked_range<SourceIter> range(begin, end);
    
#if 1
    tbb::parallel_for(range, pTBB);
#else
    pTBB(range);
#endif
}


template<class LuminanceScaler, class DisplayTransformer, class PixelAssembler>
ToneMappingKernel<LuminanceScaler, DisplayTransformer, PixelAssembler>
setupKernel(const LuminanceScaler& luminanceScaler,
            const DisplayTransformer& displayTransformer,
            const PixelAssembler& pixelAssembler)
{
    ToneMappingKernel<LuminanceScaler,DisplayTransformer,PixelAssembler>
        kernel(luminanceScaler, displayTransformer, pixelAssembler);
    return kernel;
}




// Traits to choose a pixel assembler
template <typename LuminanceScalerValueType, typename DestinationType>
struct pixel_assembler_traits;

template <>
struct pixel_assembler_traits<float, pcg::Bgra8>
{
    typedef PixelAssembler_BGRA8 assembler_t;
};

template <>
struct pixel_assembler_traits<pcg::Vec4f, pcg::Bgra8>
{
    typedef PixelAssembler_BGRA8Vec4 assembler_t;
};

#if PCG_USE_AVX
template <>
struct pixel_assembler_traits<pcg::Vec8f, pcg::Bgra8>
{
    typedef PixelAssembler_BGRA8Vec8 assembler_t;
};
#endif



template <class LuminanceScaler, class DisplayTransform, typename SourceIter, typename DestIter>
void ToneMapAux(const LuminanceScaler &scaler, const DisplayTransform &display,
    SourceIter begin, SourceIter end, DestIter dest)
{
    typedef typename pixel_assembler_traits<typename LuminanceScaler::value_t,
        pcg::Bgra8>::assembler_t assembler_t;
    assembler_t assembler;
    typedef ToneMappingKernel<LuminanceScaler, DisplayTransform,
        assembler_t> kernel_t;

    kernel_t kernel=setupKernel(scaler, display, assembler);
    processPixels(kernel, begin, end, dest);
}


enum DisplayMethod
{
    EDISPLAY_GAMMA,
    EDISPLAY_SRGB_REF,
    EDISPLAY_SRGB_FAST1,
    EDISPLAY_SRGB_FAST2
};

inline DisplayMethod getDisplayMethod(const pcg::ToneMapperSoA& tm)
{
    if (tm.isSRGB()) {
        switch (tm.SRGBMethod()) {
        case pcg::ToneMapperSoA::SRGB_REF:
            return EDISPLAY_SRGB_REF;
        case pcg::ToneMapperSoA::SRGB_FAST1:
            return EDISPLAY_SRGB_FAST1;
        case pcg::ToneMapperSoA::SRGB_FAST2:
            return EDISPLAY_SRGB_FAST2;
        default:
            throw pcg::RuntimeException("Unexpected sRGB method");
        }
    }
    else {
        return EDISPLAY_GAMMA;
    }
}



template <class LuminanceScaler, typename SourceIter, typename DestIter>
void ToneMapAuxDelegate(const LuminanceScaler& scaler, DisplayMethod dMethod,
    float invGamma, SourceIter begin, SourceIter end, DestIter dest)
{
    // Setup the display transforms
    typedef typename LuminanceScaler::value_t value_t;
    const DisplayTransformer_Gamma<value_t> displayGamma(invGamma);
    const typename Display_sRGB_Ref<value_t>::display_t   displaySRGB0;
    const typename Display_sRGB_Fast1<value_t>::display_t displaySRGB1;
    const typename Display_sRGB_Fast2<value_t>::display_t displaySRGB2;

    switch(dMethod) {
    case EDISPLAY_GAMMA:
        ToneMapAux(scaler, displayGamma, begin, end, dest);
        break;
    case EDISPLAY_SRGB_REF:
        ToneMapAux(scaler, displaySRGB0, begin, end, dest);
        break;
    case EDISPLAY_SRGB_FAST1:
        ToneMapAux(scaler, displaySRGB1, begin, end, dest);
        break;
    case EDISPLAY_SRGB_FAST2:
        ToneMapAux(scaler, displaySRGB2, begin, end, dest);
        break;
    default:
        throw pcg::IllegalArgumentException("Unknown display method");
    }
}

} // namespace




void pcg::ToneMapperSoA::SetExposure(float exposure)
{
    m_exposure = exposure;
    m_exposureFactor = pow(2.0f, exposure);
}



void pcg::ToneMapperSoA::ToneMap(
    pcg::Image<pcg::Bgra8, pcg::TopDown>& dest,
    const pcg::Image<pcg::Rgba32F, pcg::TopDown>& src,
    pcg::TmoTechnique technique) const
{
    assert(src.Width()  == dest.Width());
    assert(src.Height() == dest.Height());

    const DisplayMethod dMethod(getDisplayMethod(*this));

#if !USE_VECTOR4_ITERATOR
    const pcg::Rgba32F* begin = src.GetDataPointer();
    const pcg::Rgba32F* end   = begin + src.Size();
    PixelBGRA8* out = reinterpret_cast<PixelBGRA8*>(dest.GetDataPointer());
    typedef float ScalerValueType;
#else
    RGBA32FVec4ImageIterator begin = RGBA32FVec4ImageIterator::begin(src);
    RGBA32FVec4ImageIterator end   = RGBA32FVec4ImageIterator::end(src);
    PixelBGRA8Vec4* out            = PixelBGRA8Vec4::begin(dest);
    typedef Vec4f ScalerValueType;
#endif
    LuminanceScaler_Reinhard02<ScalerValueType> sReinhard02;
    LuminanceScaler_Exposure<ScalerValueType>   sExposure;

    switch(technique) {
    case pcg::REINHARD02:
        sReinhard02.SetParams(this->ParamsReinhard02());
        ToneMapAuxDelegate(sReinhard02, dMethod, m_invGamma, begin, end, out);
        break;
    case pcg::EXPOSURE:
        sExposure.setExposureFactor(this->m_exposureFactor);
        ToneMapAuxDelegate(sExposure, dMethod, m_invGamma, begin, end, out);
        break;
    default:
        throw IllegalArgumentException("Invalid tone mapping technique");
        break;
    }
}



void pcg::ToneMapperSoA::ToneMap(
    pcg::Image<pcg::Bgra8, pcg::TopDown>& dest,
    const pcg::RGBAImageSoA& src,
    pcg::TmoTechnique technique) const
{
    assert(src.Width()  == dest.Width());
    assert(src.Height() == dest.Height());

    const DisplayMethod dMethod(getDisplayMethod(*this));
    
#if PCG_USE_AVX
    typedef RGBA32FVec8ImageSoAIterator IteratorSoA;
    typedef PixelBGRA8Vec8 PixelVec;
    typedef Vec8f ScalerValueType;
#else
    typedef RGBA32FVec4ImageSoAIterator IteratorSoA;
    typedef PixelBGRA8Vec4 PixelVec;
    typedef Vec4f ScalerValueType;
#endif

    IteratorSoA begin = IteratorSoA::begin(src);
    IteratorSoA end   = IteratorSoA::end(src);
    PixelVec* out     = PixelVec::begin(dest);

    LuminanceScaler_Reinhard02<ScalerValueType> sReinhard02;
    LuminanceScaler_Exposure<ScalerValueType> sExposure;

    switch(technique) {
    case pcg::REINHARD02:
        sReinhard02.SetParams(this->ParamsReinhard02());
        ToneMapAuxDelegate(sReinhard02, dMethod, m_invGamma, begin, end, out);
        break;
    case pcg::EXPOSURE:
        sExposure.setExposureFactor(this->m_exposureFactor);
        ToneMapAuxDelegate(sExposure, dMethod, m_invGamma, begin, end, out);
        break;
    default:
        throw IllegalArgumentException("Invalid tone mapping technique");
        break;
    }
}
