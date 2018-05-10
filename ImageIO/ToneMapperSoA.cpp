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


namespace ssemath
{
#include "sse_mathfun.h"
}
#include "Amaths.h"


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

template <typename T>
inline T fastpow(const T& x, const T& y)
{
    return ::pow(x, y);
}

template <>
inline pcg::Vec4f pow(const pcg::Vec4f& x, const pcg::Vec4f& y)
{
#if USE_SSE_POW
    const pcg::Vec4f result = ssemath::pow_ps(x, y);
#else
    const pcg::Vec4f result(::pow(x[3], y[3]), ::pow(x[2], y[2]),
        ::pow(x[1], y[1]), ::pow(x[0], y[0]));
#endif
    return result;
}

template <>
inline pcg::Vec4f fastpow(const pcg::Vec4f& x, const pcg::Vec4f& y)
{
    const pcg::Vec4f result = am::pow_eps(x, y);
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



template <typename T>
inline T zero() {
    return T(0.0f);
}

template <>
inline pcg::Vec4f zero() {
    return pcg::Vec4f::zero();
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
    const pcg::Vec8f result = ssemath::pow_avx(x, y);
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
inline pcg::Vec8f fastpow(const pcg::Vec8f& x, const pcg::Vec8f& y)
{
    const pcg::Vec8f result = am::pow_avx(x, y);
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

template <>
inline pcg::Vec8f zero() {
    return _mm256_setzero_ps();
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
inline const T& MAY_BE_UNUSED getValue(const pcg::Vec4fUnion& value);

template <>
inline const float& MAY_BE_UNUSED getValue(const pcg::Vec4fUnion& value) {
    return value.f[0];
}

template <>
inline const pcg::Vec4f& getValue(const pcg::Vec4fUnion& value) {
    return *reinterpret_cast<const pcg::Vec4f*>(&value.xmm);
}


#if PCG_USE_AVX

// Helper function to set either a scalar or a float from a Vec8fUnion
template <typename T>
inline const T& getValue(const pcg::Vec8fUnion& value);

template <>
inline const float& MAY_BE_UNUSED getValue(const pcg::Vec8fUnion& value) {
    return value.f[0];
}

template <>
inline const pcg::Vec4f& getValue(const pcg::Vec8fUnion& value) {
    return *reinterpret_cast<const pcg::Vec4f*>(&value.xmm[0]);
}

template <>
inline const pcg::Vec8f& getValue(const pcg::Vec8fUnion& value) {
    return *reinterpret_cast<const pcg::Vec8f*>(&value.ymm);
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

static const VecfUnion ONE  = {PCG_TMOSOA_VECF( 1.0f )};
    
static const VecfUnion Q_8bit  = {PCG_TMOSOA_VECF(   255.0f )};
#if TONEMAPPERSOA_HAS_16BIT
static const VecfUnion Q_16bit = {PCG_TMOSOA_VECF( 65535.0f )};
#endif

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
        T& rOut, T& gOut, T& bOut) const throw()
    {
        rOut = m_multiplier * rLinear;
        gOut = m_multiplier * gLinear;
        bOut = m_multiplier * bLinear;
    }

    // Scales each pixel by multiplier
    inline void setExposureFactor(float multiplier) {
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
    m_P(1.0f), m_Q(1.0f), m_multiplier(1.0f)
    {}

    // Scales each pixel by multiplier
    inline void setExposureFactor(float multiplier) {
        m_multiplier = T(multiplier);
    }

    // Setup the internal constants
    inline void SetParams(const pcg::Reinhard02::Params &params)
    {
        m_P = T(params.key / params.l_w);
        m_Q = T(1.0f / (params.l_white*params.l_white));
    }

    inline void operator() (
        const T& rLinear, const T& gLinear, const T& bLinear,
        T& rOut, T& gOut, T& bOut) const throw()
    {
        const T& ONE(constants::getValue<T>(constants::ONE));
        const T& LVec0(constants::getValue<T>(constants::LVec[0]));
        const T& LVec1(constants::getValue<T>(constants::LVec[1]));
        const T& LVec2(constants::getValue<T>(constants::LVec[2]));

        // Get the luminance
        const T r = rLinear * m_multiplier;
        const T g = gLinear * m_multiplier;
        const T b = bLinear * m_multiplier;
        const T Y = LVec0*r + LVec1*g + LVec2*b;

        // Compute the scale
        const T Lp = m_P * Y;
        const T k = (m_P * (ONE + m_Q*Lp)) * ops::rcp(ONE + Lp);

        // And apply
        rOut = k * r;
        gOut = k * g;
        bOut = k * b;
    }

private:
    T m_P;
    T m_Q;
    T m_multiplier;
};



template <typename T>
struct Clamper01
{
    inline T operator() (const T& x) const throw()
    {
        const T& ONE(constants::getValue<T>(constants::ONE));
        return ops::clamp(x, ops::zero<T>(), ONE);
    }
};


// Raises each pixel (already in [0,1]) to 1/gamma. A typical value for gamma
// and current LCD screens in 2.2. Gamma has to be greater than zero.
template <typename T>
struct DisplayTransformer_Gamma
{
    DisplayTransformer_Gamma(float invGamma) : m_invGamma(invGamma)
    {
        assert(invGamma > 0);
    }

    inline T operator() (const T& x) const throw()
    {
        return ops::pow(x, m_invGamma);
    }

private:
    const T m_invGamma;
};

// Use the Intel Approximate Math versions
template <typename T>
struct DisplayTransformer_Gamma_Fast
{
    DisplayTransformer_Gamma_Fast(float invGamma) : m_invGamma(invGamma)
    {
        assert(invGamma > 0);
    }

    inline T operator() (const T& x) const throw()
    {
        return ops::fastpow(x, m_invGamma);
    }

private:
    const T m_invGamma;
};


template <typename T>
struct SRGB_NonLinear_Ref
{
    SRGB_NonLinear_Ref() {}

    inline T operator() (const T& x) const throw()
    {
        const T& FACTOR(constants::getValue<T>(constants::SRGB_FactorHi));
        const T& EXPONENT(constants::getValue<T>(constants::SRGB_Exponent));
        const T& OFFSET(constants::getValue<T>(constants::SRGB_Offset));

        T r = FACTOR * ops::pow(x, EXPONENT) - OFFSET;
        return r;
    }
};



// Rational approximation which should be good enough for 8-bit quantizers
template <typename T>
struct SRGB_NonLinear_Remez44
{
    SRGB_NonLinear_Remez44() {}

    inline T operator() (const T& x) const throw()
    {    
        const T& P0(constants::getValue<T>(constants::Remez44_P[0]));
        const T& P1(constants::getValue<T>(constants::Remez44_P[1]));
        const T& P2(constants::getValue<T>(constants::Remez44_P[2]));
        const T& P3(constants::getValue<T>(constants::Remez44_P[3]));
        const T& P4(constants::getValue<T>(constants::Remez44_P[4]));

        const T& Q0(constants::getValue<T>(constants::Remez44_Q[0]));
        const T& Q1(constants::getValue<T>(constants::Remez44_Q[1]));
        const T& Q2(constants::getValue<T>(constants::Remez44_Q[2]));
        const T& Q3(constants::getValue<T>(constants::Remez44_Q[3]));
        const T& Q4(constants::getValue<T>(constants::Remez44_Q[4]));

        const T num = (P0 + x*(P1 + x*(P2 + x*(P3 + P4*x))));
        const T den = (Q0 + x*(Q1 + x*(Q2 + x*(Q3 + Q4*x))));
        const T result = num * ops::rcp(den);
        return result;
    }
};



// Rational approximation which should be good enough for 16-bit quantizers
template <typename T>
struct SRGB_NonLinear_Remez77
{
    SRGB_NonLinear_Remez77() {}

    inline T operator() (const T& x) const throw()
    {
        const T& P0(constants::getValue<T>(constants::Remez77_P[0]));
        const T& P1(constants::getValue<T>(constants::Remez77_P[1]));
        const T& P2(constants::getValue<T>(constants::Remez77_P[2]));
        const T& P3(constants::getValue<T>(constants::Remez77_P[3]));
        const T& P4(constants::getValue<T>(constants::Remez77_P[4]));
        const T& P5(constants::getValue<T>(constants::Remez77_P[5]));
        const T& P6(constants::getValue<T>(constants::Remez77_P[6]));
        const T& P7(constants::getValue<T>(constants::Remez77_P[7]));

        const T& Q0(constants::getValue<T>(constants::Remez77_Q[0]));
        const T& Q1(constants::getValue<T>(constants::Remez77_Q[1]));
        const T& Q2(constants::getValue<T>(constants::Remez77_Q[2]));
        const T& Q3(constants::getValue<T>(constants::Remez77_Q[3]));
        const T& Q4(constants::getValue<T>(constants::Remez77_Q[4]));
        const T& Q5(constants::getValue<T>(constants::Remez77_Q[5]));
        const T& Q6(constants::getValue<T>(constants::Remez77_Q[6]));
        const T& Q7(constants::getValue<T>(constants::Remez77_Q[7]));

        const T num = (P0 + x*(P1 + x*(P2 + x*(P3 +
                            x*(P4 + x*(P5 + x*(P6 + P7*x)))))));
        const T den = (Q0 + x*(Q1 + x*(Q2 + x*(Q3 +
                            x*(Q4 + x*(Q5 + x*(Q6 + Q7*x)))))));
        const T result = num * ops::rcp(den);
        return result;
    }
};



// Actual sRGB implementation, with the non-linear functor as a template
template <typename T, template<typename> class SRGB_NonLinear>
struct DisplayTransformer_sRGB
{
    DisplayTransformer_sRGB() {}

    inline T operator() (const T& pLinear) const throw()
    {
        const T& CUTOFF_sRGB(constants::getValue<T>(constants::SRGB_Cutoff));
        const T& FACTOR(constants::getValue<T>(constants::SRGB_FactorLow));

        T p = m_nonlinear(pLinear);
        T result = ops::select_gt(pLinear, CUTOFF_sRGB, p, FACTOR * pLinear);
        return result;
    }

private:
    SRGB_NonLinear<T> m_nonlinear;
};



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

    inline value_t operator() (const T& x) const throw()
    {
        const T& FACTOR(constants::getValue<T>(constants::Q_8bit));
        return ops::round<QT>(FACTOR * x);
    }
};


#if TONEMAPPERSOA_HAS_16BIT
template <typename T, typename QT>
struct Quantizer16bit
{
    typedef QT value_t;

    inline value_t operator() (const T& x) const throw()
    {
        const T& FACTOR(constants::getValue<T>(constants::Q_16bit));
        return ops::round<QT>(FACTOR * x);
    }
};
#endif // TONEMAPPERSOA_HAS_16BIT





#if !USE_VECTOR4_ITERATOR
struct PixelAssembler_BGRA8
{
    typedef pcg::PixelBGRA8 pixel_t;
    typedef Quantizer8bit<float, uint8_t> quantizer_t;
    typedef quantizer_t::value_t value_t;

    inline void operator() (
        const value_t& r, const value_t& g, const value_t& b, const value_t& a,
        pixel_t& outPixel) const throw()
    {
        outPixel.argb = (a << 24) | (r << 16) | (g << 8) | (b);
    }
};
#endif





struct PixelAssembler_BGRA8Vec4
{
    typedef pcg::PixelBGRA8Vec4 pixel_t;
    typedef Quantizer8bit<pcg::Vec4f, pcg::Vec4i> quantizer_t;
    typedef quantizer_t::value_t value_t;

    inline void operator() (
        const value_t& r, const value_t& g, const value_t& b, const value_t& a,
        pixel_t& outPixel) const throw()
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

#if !PCG_USE_AVX2
    template <int offset>
    static inline pcg::Vec4i buildExtract(const value_t& r, const value_t& g,
        const value_t& b, const value_t& a) throw()
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
#endif /* !PCG_USE_AVX2 */

    inline void operator() (
        const value_t& r, const value_t& g, const value_t& b, const value_t& a,
        pixel_t& outPixel) const throw()
    {
#if !PCG_USE_AVX2
        // Adequate support for integer operation arrives with AVX2, in the
        // meantime extract 2 128-bit values
        const pcg::Vec4i pix0 = buildExtract<0>(r, g, b, a);
        const pcg::Vec4i pix1 = buildExtract<1>(r, g, b, a);

        // Casting sets the lower bits, then insert the higher ones
        const __m256i pixel =
            _mm256_insertf128_si256(_mm256_castsi128_si256(pix0), pix1, 1);
#else
        pcg::Vec8i aShift = _mm256_slli_epi32(a, 24);
        pcg::Vec8i rShift = _mm256_slli_epi32(r, 16);
        pcg::Vec8i gShift = _mm256_slli_epi32(g, 8);

        const pcg::Vec8i pixel = aShift | rShift | gShift | b;
#endif /* !PCG_USE_AVX2 */

        _mm256_stream_si256(&outPixel.ymm, pixel);
    }
};

#endif // PCG_USE_AVX


template <class LuminanceScaler>
struct BlockSizeTraits
{
    // Assuming that LuminanceScaler::value_t is a vector of k-single precision
    // values, target the block size so that the temporary storage uses 16KiB
    static const size_t BLOCK_SIZE =
        4096 / sizeof(typename LuminanceScaler::value_t);
};


template<class LuminanceScaler, class DisplayTransformer, class PixelAssembler,
         size_t BlockSize = BlockSizeTraits<LuminanceScaler>::BLOCK_SIZE>
struct ToneMappingKernel
{
    typedef typename LuminanceScaler::value_t value_t;

    ToneMappingKernel(const LuminanceScaler& scaler,
        const DisplayTransformer& display, const PixelAssembler& assembler) :
    luminanceScaler(scaler), displayTransformer(display),
    pixelAssembler(assembler)
    {}

    // Operate on a range, going block by block
    template <typename SourceIter>
    void operator() (SourceIter begin, SourceIter end,
        typename PixelAssembler::pixel_t* pixelOut) const throw()
    {
        // Temporary storage
        value_t rTemp[BlockSize];
        value_t gTemp[BlockSize];
        value_t bTemp[BlockSize];
        value_t aTemp[BlockSize];

        for (SourceIter it = begin; it != end;) {
            const size_t numIter = std::min(static_cast<size_t>(end - it),
                                            BlockSize);

            // Scale the luminance according to the current settings and clamp
            for (size_t i = 0; i != numIter; ++i, ++it) {
                typename std::iterator_traits<SourceIter>::value_type pixel=*it;
                luminanceScaler(pixel.r(), pixel.g(), pixel.b(),
                    rTemp[i], gTemp[i], bTemp[i]);
                rTemp[i] = clamper(rTemp[i]);
                gTemp[i] = clamper(gTemp[i]);
                bTemp[i] = clamper(bTemp[i]);
                aTemp[i] = clamper(pixel.a());
            }

            // Nonlinear display transform
            for (size_t i = 0; i != numIter; ++i) {
                rTemp[i] = displayTransformer(rTemp[i]);
                gTemp[i] = displayTransformer(gTemp[i]);
                bTemp[i] = displayTransformer(bTemp[i]);
            }

            // Quantize the values and build the pixel
            for (size_t i = 0; i != numIter; ++i, ++pixelOut) {
                typename PixelAssembler::value_t rQ = quantizer(rTemp[i]);
                typename PixelAssembler::value_t gQ = quantizer(gTemp[i]);
                typename PixelAssembler::value_t bQ = quantizer(bTemp[i]);
                typename PixelAssembler::value_t aQ = quantizer(aTemp[i]);

                pixelAssembler(rQ, gQ, bQ, aQ, *pixelOut);
            }
        }
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

    inline void operator() (tbb::blocked_range<SourceIter>& range) const
    {
        DestIter dest = m_dest + (range.begin() - m_src);
        m_kernel(range.begin(), range.end(), dest);
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

#if !USE_VECTOR4_ITERATOR
template <>
struct pixel_assembler_traits<float, pcg::Bgra8>
{
    typedef PixelAssembler_BGRA8 assembler_t;
};
#endif

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
    EDISPLAY_GAMMA_REF,
    EDISPLAY_GAMMA_FAST,
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
        switch (tm.GammaMethod()) {
        case pcg::ToneMapperSoA::GAMMA_REF:
            return EDISPLAY_GAMMA_REF;
        case pcg::ToneMapperSoA::GAMMA_FAST:
            return EDISPLAY_GAMMA_FAST;
        default:
            throw pcg::RuntimeException("Unexpected gamma method");
        }
    }
}



template <class LuminanceScaler, typename SourceIter, typename DestIter>
void ToneMapAuxDelegate(const LuminanceScaler& scaler, DisplayMethod dMethod,
    float invGamma, SourceIter begin, SourceIter end, DestIter dest)
{
    // Setup the display transforms
    typedef typename LuminanceScaler::value_t value_t;
    const DisplayTransformer_Gamma<value_t> displayGamma(invGamma);
    const DisplayTransformer_Gamma_Fast<value_t> displayGammaFast(invGamma);
    const typename Display_sRGB_Ref<value_t>::display_t   displaySRGB0;
    const typename Display_sRGB_Fast1<value_t>::display_t displaySRGB1;
    const typename Display_sRGB_Fast2<value_t>::display_t displaySRGB2;

    switch(dMethod) {
    case EDISPLAY_GAMMA_REF:
        ToneMapAux(scaler, displayGamma, begin, end, dest);
        break;
    case EDISPLAY_GAMMA_FAST:
        ToneMapAux(scaler, displayGammaFast, begin, end, dest);
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
        sReinhard02.setExposureFactor(this->m_exposureFactor);
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
        sReinhard02.setExposureFactor(this->m_exposureFactor);
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
