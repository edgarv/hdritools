// A simple tone mapper class, using a LUT for fast exponentiation,
// inspired on the original glimage code.
// To keep things simple, this guy only know how to tone map Rgba32F images

#if !defined(PCG_TONEMAPPER_H)
#define PCG_TONEMAPPER_H

#include "Image.h"
#include "Rgba32F.h"
#include "LDRPixels.h"
#include "ImageIO.h"
#include "Exception.h"
#include "Reinhard02.h"

namespace pcg
{
// Enum to flag which technique to use: the simple scaling, using the
// exposure settings, or the global version of Reinhard02
enum TmoTechnique {
    EXPOSURE,
    REINHARD02
};

// Forward declaration
namespace tonemapper_internal {
    template <typename T, ScanLineMode S1, ScanLineMode S2 = S1, 
        bool useLUT = true, bool isSRGB = true, TmoTechnique tmo = EXPOSURE>
    class ApplyToneMap;
}



class ToneMapper {

    template <class T, ScanLineMode S1, ScanLineMode S2, 
        bool useLUT, bool isSRGB, TmoTechnique tmo>
    friend class tonemapper_internal::ApplyToneMap;

protected:

    // Utility vector for the clamping
    const static IMAGEIO_API Rgba32F ZEROS;
    const static IMAGEIO_API Rgba32F ONES;

    // Pointer to the lookup table
    unsigned char *lut;

    // Number of elements in the LUT. It must be a multiple of four!
    const unsigned short lutSize;

    // Exposure of the image
    float exposure;

    // Exposure compensation: 2^exposure
    float exposureFactor;

    // Gamma correction
    float gamma;

    // The actual exponent for the gamma correction: 1/gamma
    float invGamma;

    // A flag to know whether to use the simple gamma curve or the sRGB color space
    bool useSRGB;

    // Parameters for applying the global Reinhard02 tone mapper
    Reinhard02::Params params_Reinhard02;

    // The lut contains the values of:  round(255 * (p^(1/gamma))),
    // where p is the pixel value after applying the exposure compensation
    // and whose value is in the range [0,1]
    void IMAGEIO_API UpdateLUT();


public:

    // Creates a new Tone Mapper instance which will use a LUT of the given size to speed
    // up the query of the gamma exponentiation. That LUT will be forced to have a size
    // multiple of four, and of course if it's lenght is ridiculously small everything
    // is likely to crash in terrible ways.
    ToneMapper(unsigned short size = 2048) : 
    lut(NULL), lutSize(size & (~0x3)), useSRGB(false) 
    {
        lut = new unsigned char[lutSize];
        SetExposure(0.0f);
        SetGamma(1.0f);
    }

    // Constructor which takes an specific exposure, and uses sRGB instead of a gamma curve.
    // If sRGB is disabled afterwards, the default gamma is 2.2
    ToneMapper(float exposure, unsigned short size) : 
    lut(NULL), lutSize(size & (~0x3)),
    gamma(2.2f), invGamma(1.0f/2.2f), useSRGB(true)
    {
        if (lutSize == 0) {
            throw IllegalArgumentException("Illegal LUT size of 0");
        }
        lut = new unsigned char[lutSize];
        SetExposure(exposure);
        UpdateLUT();
    }

    // The destructor only deletes the LUT
    ~ToneMapper() {
        if (lut != NULL) {
            delete [] lut;
        }
    }

    // Sets the exposure. Each pixel will be scaled by 2^exposure prior to gamma correction
    void IMAGEIO_API SetExposure(float exposure);

    // Replaces the current set of parameters for the Reinhard02 TMO
    void SetParams(const Reinhard02::Params &params) {
        this->params_Reinhard02 = params;
    }

    // Sets the gamma correction. Each pixel's component p, after the exposure correction and clamping to
    // the range [0,1] will be raised to the power of (1/gamma), thus the final value is p^(1/gamma).
    // Therefore gamma must be greater than zero; the typical value for LCD's is 2.2.
    // Calling this method implies disabling sRGB.
    void SetGamma(float gamma) {
        if (gamma <= 0) {
            throw IllegalArgumentException("The gamma must be greater that zero");
        }
        this->gamma    = gamma;
        this->invGamma = 1.0f/gamma;
        this->useSRGB  = false;
        UpdateLUT();
    }

    // Enables or disables the sRGB curve
    void SetSRGB(bool enable) {
        useSRGB = enable;
        UpdateLUT();
    }

    // Returns the gamma employed when sRGB is not used
    float Gamma() const { return gamma; }

    // Returns the inverse of the gamma, in case someone needs it
    float InvGamma() const { return invGamma; }

    // Returns the exposure
    float Exposure() const { return exposure; }

    // Returns a const reference to the Reinhard02 parameters
    const Reinhard02::Params & ParamsReinhard02() const {
        return params_Reinhard02;
    }

    // Returns a reference to the Reinhard02 parameters
    Reinhard02::Params & ParamsReinhard02() {
        return params_Reinhard02;
    }

    // Returns wheather it's using sRGB or nor
    bool isSRGB() const { return useSRGB; }

    // Returns the maximum error in the LUT: the max among all values.
    // This method doesn't cache any values, so use with care!
    int IMAGEIO_API MaxLUTError() const;


    // The real tone mapping operations: support for 
    // LDR pixels, in the different type of scanline orders
    void IMAGEIO_API ToneMap(Image<Bgra8, TopDown> &dest,
        const Image<Rgba32F, TopDown> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Bgra8, TopDown> &dest,
        const Image<Rgba32F, BottomUp> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Bgra8, BottomUp> &dest,
        const Image<Rgba32F, TopDown> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Bgra8, BottomUp> &dest,
        const Image<Rgba32F, BottomUp> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;

    void IMAGEIO_API ToneMap(Image<Rgba8, TopDown> &dest,
        const Image<Rgba32F, TopDown> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Rgba8, TopDown> &dest,
        const Image<Rgba32F, BottomUp> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Rgba8, BottomUp> &dest,
        const Image<Rgba32F, TopDown> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Rgba8, BottomUp> &dest,
        const Image<Rgba32F, BottomUp> &src,
        bool useLut = true, TmoTechnique technique = EXPOSURE) const;

    // The 16-bit LDR pixels won't use the LUT
    void IMAGEIO_API ToneMap(Image<Rgba16, TopDown> &dest,
        const Image<Rgba32F, TopDown> &src,
        TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Rgba16, TopDown> &dest,
        const Image<Rgba32F, BottomUp> &src,
        TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Rgba16, BottomUp> &dest,
        const Image<Rgba32F, TopDown> &src,
        TmoTechnique technique = EXPOSURE) const;
    void IMAGEIO_API ToneMap(Image<Rgba16, BottomUp> &dest,
        const Image<Rgba32F, BottomUp> &src,
        TmoTechnique technique = EXPOSURE) const;
};
} // namespace pcg

#endif /* PCG_TONEMAPPER_H */
