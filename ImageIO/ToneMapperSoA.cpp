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



template <typename T>
inline T pow(const T& x, const T& y)
{
    return ::pow(x, y);
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



// Select
// (a OP b) ? c : d
template <typename T>
inline T select_gt(const T& a, const T& b, const T& c, const T& d)
{
    return (a > b) ? c : d;
}



template <typename T>
inline T min(const T& a, const T& b) {
    return std::min(a, b);
}




template <typename T>
inline T max(const T& a, const T& b) {
    return std::max(a, b);
}



template <typename T>
inline T clamp(const T& x, const T& minValue, const T& maxValue) {
    return ops::max(ops::min(x, maxValue), minValue);
}

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
 *      P:pixel_t& outPixel ) const
 *                       Takes the quantized pixels and saves the packed
 *                       LDR pixel into outPixel.
 */



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
        static const T LVec[] = {
            T(0.212639005871510f),
            T(0.715168678767756f),
            T(0.072192315360734f)
        };
        static const T ONE = T(1.0f);

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
};



template <typename T>
struct Clamper01
{
    inline T operator() (const T& x) const
    {
        const static T ONE  = T(1.0f);
        const static T ZERO = T(0.0f);
        return ops::clamp(x, ZERO, ONE);
    }
};



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



// Supporting modules for the linear bits of sRGB
enum EsRGB_MODE
{
    // Reference implementation
    SRGB_REFERENCE,
    // Fast approximation, reasonably accurate
    SRGB_FAST1,
    // Yet faster approximation, but not very accurate
    SRGB_FAST2
};


template <typename T>
struct SRGB_NonLinear_Ref
{
    inline T operator() (const T& x) const
    {
        const static T FACTOR   = T(1.055f);
        const static T EXPONENT = T(1.0f/2.4f);
        const static T OFFSET   = T(0.055f);
        T r = FACTOR * ops::pow(x, EXPONENT) - OFFSET;
        return r;
    }
};



// Rational approximation which should be good enough for 8-bit quantizers
template <typename T>
struct SRGB_NonLinear_Remez44
{
    inline T operator() (const T& x) const
    {
        static const T P[] = {
            T(-0.01997304708470295f),
            T(24.95173169159651f),
            T(3279.752175439042f),
            T(39156.546674561556f),
            T(42959.451119871745f)
        };

        static const T Q[] = {
            T(1.f),
            T(361.5384894448744f),
            T(13090.206953080155f),
            T(55800.948825871434f),
            T(16180.833742684188f)
        };
    
        const T num = (P[0] + x*(P[1] + x*(P[2] + x*(P[3] + P[4]*x))));
        const T den = (Q[0] + x*(Q[1] + x*(Q[2] + x*(Q[3] + Q[4]*x))));
        const T result = num * ops::rcp(den);
        return result;
    }
};



// Rational approximation which should be good enough for 16-bit quantizers
template <typename T>
struct SRGB_NonLinear_Remez77
{
    inline T operator() (const T& x) const
    {
        static const T P[] = {
            T(-0.031852703288410084f),
            T(18.553896638433446f),
            T(22006.0672110147f),
            T(2.635850360294788e6f),
            T(7.352843882592331e7f),
            T(5.330866283442694e8f),
            T(9.261676939514283e8f),
            T(2.632919307024597e8f)
        };

        static const T Q[] = {
            T(1.f),
            T(1280.3496360781705f),
            T(274007.5886695005f),
            T(1.4492562384924464e7f),
            T(2.1029015319992256e8f),
            T(8.142158667694515e8f),
            T(6.956059106558038e8f),
            T(6.3853076877794705e7f)
        };

        const T num = (P[0] + x*(P[1] + x*(P[2] + x*(P[3] +
                                  x*(P[4] + x*(P[5] + x*(P[6] + P[7]*x)))))));
        const T den = (Q[0] + x*(Q[1] + x*(Q[2] + x*(Q[3] +
                                  x*(Q[4] + x*(Q[5] + x*(Q[6] + Q[7]*x)))))));
        const T result = num * ops::rcp(den);
        return result;
    }
};



// Actual sRGB implementation, with the non-linear functor as a template
template <typename T, template<typename> class SRGB_NonLinear>
struct DisplayTransformer_sRGB
{
    inline T operator() (const T& pLinear) const
    {
        const static T CUTOFF_sRGB = T(0.003041229589676f);
        const static T FACTOR      = T(12.92f);
        T p = m_nonlinear(pLinear);

        // Here comes the blend
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

    value_t operator() (const T& x) const
    {
        const static T FACTOR = T(255.0f);
        return ops::round<QT>(FACTOR * x);
    }
};


template <typename T, typename QT>
struct Quantizer16bit
{
    typedef QT value_t;

    value_t operator() (const T& x) const
    {
        const static T FACTOR = T(65535.0f);
        return ops::round<QT>(FACTOR * x);
    }
};






struct PixelAssembler_BGRA8
{
    typedef pcg::PixelBGRA8 pixel_t;
    typedef Quantizer8bit<float, uint8_t> quantizer_t;
    typedef quantizer_t::value_t value_t;

    void operator() (
        const value_t& r, const value_t& g, const value_t& b,
        pixel_t& outPixel) const
    {
        outPixel.argb = (0xff000000) | (r << 16) | (g << 8) | (b);
    }
};




template<class LuminanceScaler, class DisplayTransformer, class PixelAssembler>
struct ToneMappingKernel
{
    typedef typename LuminanceScaler::value_t value_t;

    ToneMappingKernel(const LuminanceScaler& scaler,
        const DisplayTransformer& display, const PixelAssembler& assembler) :
    luminanceScaler(scaler), displayTransformer(display),
    pixelAssembler(assembler)
    {}

    void operator() (
        const value_t& rLinear, const value_t& gLinear, const value_t& bLinear,
            typename PixelAssembler::pixel_t& pixelOut) const
    {
        value_t rScaled, gScaled, bScaled;

        // Scale the luminance according to the current settings
        luminanceScaler(rLinear, gLinear, bLinear,
            rScaled, gScaled, bScaled);

        // Clamp to [0,1]
        const value_t rClamped = clamper(rScaled);
        const value_t gClamped = clamper(gScaled);
        const value_t bClamped = clamper(bScaled);

        // Nonlinear display transform
        const value_t rDisplay = displayTransformer(rClamped);
        const value_t gDisplay = displayTransformer(gClamped);
        const value_t bDisplay = displayTransformer(bClamped);

        // Quantize the values
        const typename PixelAssembler::value_t rQ = quantizer(rDisplay);
        const typename PixelAssembler::value_t gQ = quantizer(gDisplay);
        const typename PixelAssembler::value_t bQ = quantizer(bDisplay);

        pixelAssembler(rQ, gQ, bQ, pixelOut);
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
            m_kernel(pixel.r(), pixel.g(), pixel.b(), *dest);
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



template <class LuminanceScaler, typename SourceIter, typename DestIter>
void ToneMapAuxDelegate(const LuminanceScaler& scaler, DisplayMethod dMethod,
    float invGamma, SourceIter begin, SourceIter end, DestIter dest)
{
    // Setup the display transforms
    typedef typename LuminanceScaler::value_t value_t;
    DisplayTransformer_Gamma<value_t> displayGamma(invGamma);
    typename Display_sRGB_Ref<value_t>::display_t   displaySRGB0;
    typename Display_sRGB_Fast1<value_t>::display_t displaySRGB1;
    typename Display_sRGB_Fast2<value_t>::display_t displaySRGB2;

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

    const pcg::Rgba32F* begin = src.GetDataPointer();
    const pcg::Rgba32F* end   = begin + src.Size();
    PixelBGRA8* out = reinterpret_cast<PixelBGRA8*>(dest.GetDataPointer());

    DisplayMethod dMethod;
    if (this->isSRGB()) {
        switch (m_sRGBMethod) {
        case ToneMapperSoA::SRGB_REF:
            dMethod = EDISPLAY_SRGB_REF;
            break;
        case ToneMapperSoA::SRGB_FAST1:
            dMethod = EDISPLAY_SRGB_FAST1;
            break;
        case ToneMapperSoA::SRGB_FAST2:
            dMethod = EDISPLAY_SRGB_FAST2;
            break;
        default:
            throw RuntimeException("Unexpected sRGB method");
        }
    }
    else {
        dMethod = EDISPLAY_GAMMA;
    }

    LuminanceScaler_Reinhard02<float> sReinhard02;
    LuminanceScaler_Exposure<float> sExposure;

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
