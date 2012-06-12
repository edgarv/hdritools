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
#include "ToneMapper.h"
#include "ImageSoA.h"

#include <algorithm>

#include <cassert>


namespace
{
// Computes the reciprocal
float rcp(float x)
{
    return 1.0f / x;
}


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
                        T* rOut, T* gOut, T* bOut ) const
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
 *   Q::quantized_t      typedef with the target integral type
 *   Q::quantized_t operator() ( const T& ) const
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
 *   P::pixel_t operator() (
 *      const Q::quantized_t& r,
 *      const Q::quantized_t& g,
 *      const Q::quantized_t& b ) const
 *                       Takes the quantized pixels and returns the packed
 *                       LDR pixel.
 */



// Simple scaler which only multiplies all pixels by a constant
struct LuminanceScaler_Exposure
{
    inline void operator() (
        const float& rLinear, const float& gLinear, const float& bLinear,
        float *rOut, float *gOut, float *bOut) const
    {
        *rOut = m_multiplier * rLinear;
        *gOut = m_multiplier * gLinear;
        *bOut = m_multiplier * bLinear;
    }

    // Scales each pixel by multiplier
    inline void setExposureFactor(float multiplier)
    {
        m_multiplier = float(multiplier);
    }

private:
    float m_multiplier;
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
//         (avgLogLum*key*pow(Lwhite,2) + pow(key,2)*Y)
//    k == ------------------------------------------------
//         (pow(avgLogLum,2)*pow(Lwhite,2) + avgLogLum*key*pow(Lwhite,2)*Y)
//
//    k == (P + R*Y) / (Q + P*Y)
//    P == avgLogLum*key*pow(Lwhite,2)
//    Q == (pow(avgLogLum,2)*pow(Lwhite,2)
//    R == pow(key,2)
//    
struct LuminanceScaler_Reinhard02
{
    // Initial values as for key = 0.18, avgLogLum = 0.18, Lwhite = 0.18
    LuminanceScaler_Reinhard02() :
    m_P(0.0324f), m_Q(0.0324f), m_R(0.0324f)
    {}

    // Setup the internal constants
    inline void SetParams(const pcg::Reinhard02::Params &params)
    {
        m_P = float((params.l_w*params.key) * (params.l_white*params.l_white));
        m_Q = float((params.l_w*params.l_w) * (params.l_white*params.l_white));
        m_R = float(params.key * params.key);
    }

    inline void operator() (
        const float& rLinear, const float& gLinear, const float& bLinear,
        float *rOut, float *gOut, float *bOut) const
    {
        static const float LVec[] = {
            float(0.212639005871510f),
            float(0.715168678767756f),
            float(0.072192315360734f)
        };

        // Get the luminance
        const float Y = LVec[0]*rLinear + LVec[1]*gLinear + LVec[2]*bLinear;

        // Compute the scale
        const float k = (m_P + m_R * Y) * rcp(m_Q + m_P * Y);

        // And apply
        *rOut = k * rLinear;
        *gOut = k * gLinear;
        *bOut = k * bLinear;
    }

private:

    float m_P;
    float m_Q;
    float m_R;
};



struct Clamper01
{
    inline float operator() (const float& x) const
    {
        return std::max(std::min(x, 1.0f), 0.0f);
    }
};



// Raises each pixel (already in [0,1]) to 1/gamma. A typical value for gamma
// and current LCD screens in 2.2. Gamma has to be greater than zero.
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
        m_invGamma = invGamma;
    }

    inline float operator() (const float& x) const
    {
        return pow(x, m_invGamma);
    }


private:
    float m_invGamma;
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


struct SRGB_NonLinear_Ref
{
    inline float operator() (const float& x) const
    {
        float r = 1.055f * pow(x, 1.0f/2.4f) - 0.055f;
        return r;
    }
};



// Rational approximation which should be good enough for 8-bit quantizers
struct SRGB_NonLinear_Remez44
{
    inline float operator() (const float& x) const
    {
        static const float P[] = {
            float(-0.01997304708470295f),
            float(24.95173169159651f),
            float(3279.752175439042f),
            float(39156.546674561556f),
            float(42959.451119871745f)
        };

        static const float Q[] = {
            float(1.f),
            float(361.5384894448744f),
            float(13090.206953080155f),
            float(55800.948825871434f),
            float(16180.833742684188f)
        };
    
        const float num = (P[0] + x*(P[1] + x*(P[2] + x*(P[3] + P[4]*x))));
        const float den = (Q[0] + x*(Q[1] + x*(Q[2] + x*(Q[3] + Q[4]*x))));
        const float result = num * rcp(den);
        return result;
    }
};



// Rational approximation which should be good enough for 16-bit quantizers
struct SRGB_NonLinear_Remez77
{
    inline float operator() (const float& x) const
    {
        static const float P[] = {
            float(-0.031852703288410084f),
            float(18.553896638433446f),
            float(22006.0672110147f),
            float(2.635850360294788e6f),
            float(7.352843882592331e7f),
            float(5.330866283442694e8f),
            float(9.261676939514283e8f),
            float(2.632919307024597e8f)
        };

        static const float Q[] = {
            float(1.f),
            float(1280.3496360781705f),
            float(274007.5886695005f),
            float(1.4492562384924464e7f),
            float(2.1029015319992256e8f),
            float(8.142158667694515e8f),
            float(6.956059106558038e8f),
            float(6.3853076877794705e7f)
        };

        const float num = (P[0] + x*(P[1] + x*(P[2] + x*(P[3] +
                                  x*(P[4] + x*(P[5] + x*(P[6] + P[7]*x)))))));
        const float den = (Q[0] + x*(Q[1] + x*(Q[2] + x*(Q[3] +
                                  x*(Q[4] + x*(Q[5] + x*(Q[6] + Q[7]*x)))))));
        const float result = num * rcp(den);
        return result;
    }
};



// Actual sRGB implementation, with the non-linear functor as a template
template <class SRGB_NonLinear>
struct DisplayTransformer_sRGB
{
    inline float operator() (const float &pLinear) const
    {
        const static float CUTOFF_sRGB = float(0.00304f);
        float p = m_nonlinear(pLinear);

        // Here comes the blend
        float result = pLinear > CUTOFF_sRGB ? p : 12.92f * pLinear;
        return result;
    }

private:
    SRGB_NonLinear m_nonlinear;
};

typedef DisplayTransformer_sRGB<SRGB_NonLinear_Ref>     Display_sRGB_Ref;
typedef DisplayTransformer_sRGB<SRGB_NonLinear_Remez77> Display_sRGB_Fast1;
typedef DisplayTransformer_sRGB<SRGB_NonLinear_Remez44> Display_sRGB_Fast2;



struct Quantizer8bit
{
    typedef unsigned char quantized_t;

    quantized_t operator() (const float& x) const
    {
        return static_cast<unsigned char>(255.0f * x + 0.5f);
    }
};


struct Quantizer16bit
{
    typedef unsigned short quantized_t;

    quantized_t operator() (const float& x) const
    {
        return static_cast<unsigned short>(65535.0f * x + 0.5f);
    }
};


struct PixelAssembler_BGRA8
{
    typedef pcg::Bgra8 pixel_t;

    pixel_t operator() (
        const Quantizer8bit::quantized_t& r,
        const Quantizer8bit::quantized_t& g,
        const Quantizer8bit::quantized_t& b) const
    {
        pcg::Bgra8 pixel;
        pixel.set(r, g, b);
        return pixel;
    }
};



template<class LuminanceScaler, class DisplayTransformer, class Quantizer,
         class PixelAssembler>
struct ToneMappingKernel
{
    void operator() (
        const float& rLinear, const float& gLinear, const float& bLinear,
            typename PixelAssembler::pixel_t *pixelOut) const
    {
        float rScaled, gScaled, bScaled;

        // Scale the luminance according to the current settings
        luminanceScaler(rLinear, gLinear, bLinear,
            &rScaled, &gScaled, &bScaled);

        // Clamp to [0,1]
        const float rClamped = clamper(rScaled);
        const float gClamped = clamper(gScaled);
        const float bClamped = clamper(bScaled);

        // Nonlinear display transform
        const float rDisplay = displayTransformer(rClamped);
        const float gDisplay = displayTransformer(gClamped);
        const float bDisplay = displayTransformer(bClamped);

        // Quantize the values
        const typename Quantizer::quantized_t rQ = quantizer(rDisplay);
        const typename Quantizer::quantized_t gQ = quantizer(gDisplay);
        const typename Quantizer::quantized_t bQ = quantizer(bDisplay);

        *pixelOut = pixelAssembler(rQ, gQ, bQ);
    }

    // Functors which implement the actual functionality
    LuminanceScaler luminanceScaler;
    Clamper01 clamper;
    DisplayTransformer displayTransformer;
    Quantizer quantizer;
    PixelAssembler pixelAssembler;
};


// Move the processing here, to avoid having way too many parameters
template <class Kernel>
struct ProcessorA
{
    void operator() (const pcg::Rgba32F &pixel, pcg::Bgra8 &outPixel)
    {
        kernel(pixel.r(), pixel.g(), pixel.b(), &outPixel);
    }


    Kernel kernel;
};


template <class Kernel>
void processPixels(const Kernel& kernel,
    const pcg::Rgba32F* begin, const pcg::Rgba32F* end, pcg::Bgra8 *dest)
{
    ProcessorA<Kernel> p;
    p.kernel = kernel;

    for (const pcg::Rgba32F* it = begin; it != end; ++it) {
        p(*it, *dest++);
    }
}


template<class LuminanceScaler, class DisplayTransformer, class Quantizer,
         class PixelAssembler>
ToneMappingKernel<LuminanceScaler,DisplayTransformer,Quantizer,PixelAssembler>
setupKernel(const LuminanceScaler& luminanceScaler,
            const DisplayTransformer& displayTransformer,
            const Quantizer& quantizer,
            const PixelAssembler& pixelAssembler)
{
    ToneMappingKernel<LuminanceScaler, DisplayTransformer,
        Quantizer, PixelAssembler> kernel;

    kernel.luminanceScaler = luminanceScaler;
    kernel.displayTransformer = displayTransformer;
    kernel.quantizer = quantizer;
    kernel.pixelAssembler = pixelAssembler;

    return kernel;
}




template <class LuminanceScaler, class DisplayTransform>
void ToneMapAux(const LuminanceScaler &scaler, const DisplayTransform &display,
    const pcg::Rgba32F* begin, const pcg::Rgba32F* end, pcg::Bgra8 *dest)
{
    // Fixed quantization!! (that can be fixed at compile time)
    Quantizer8bit quantizer;
    PixelAssembler_BGRA8 assembler;
    typedef ToneMappingKernel<LuminanceScaler, DisplayTransform,
        Quantizer8bit, PixelAssembler_BGRA8> kernel_t;

    kernel_t kernel=setupKernel(scaler, display, quantizer, assembler);
    processPixels(kernel, begin, end, dest);
}


enum DisplayMethod
{
    EDISPLAY_GAMMA,
    EDISPLAY_SRGB_REF,
    EDISPLAY_SRGB_FAST1,
    EDISPLAY_SRGB_FAST2
};



template <class LuminanceScaler>
void ToneMapAuxDelegate(const LuminanceScaler& scaler, DisplayMethod dMethod,
    float invGamma,
    const pcg::Rgba32F* begin, const pcg::Rgba32F* end, pcg::Bgra8 *dest)
{
    // Setup the display transforms
    DisplayTransformer_Gamma displayGamma(invGamma);
    Display_sRGB_Ref displaySRGB0;
    Display_sRGB_Fast1 displaySRGB1;
    Display_sRGB_Fast2 displaySRGB2;

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
        // ERROR!!
        break;
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
    pcg::Bgra8* out = dest.GetDataPointer();

    // TODO Select an specific sRGB method
    const DisplayMethod dMethod = this->isSRGB() ?
        EDISPLAY_SRGB_FAST2 : EDISPLAY_GAMMA;

    LuminanceScaler_Reinhard02 sReinhard02;
    LuminanceScaler_Exposure sExposure;

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
        // ERROR!!
        break;
    }
}
