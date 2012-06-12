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

#pragma once
#if !defined(PCG_TONEMAPPERSOA_H)
#define PCG_TONEMAPPERSOA_H

#include "ImageSoA.h"
#include "Reinhard02.h"
///

namespace pcg
{


class IMAGEIO_API ToneMapperSoA
{
public:
    
    // Creates a new tone mapper for SoA images specifying wheter to use sRGB
    // or simple gamma. If sRGB is disabled it will use the specified gamma
    ToneMapperSoA(bool useSRGB, float gamma = 2.2f);

    // Set the exposure. Each pixel will be scaled by 2^exposure prior to sRGB /
    // gamma correction.
    void SetExposure(float exposure);

    // Replaces the current set of parameters for the Reinhard02 global TMO
    void SetParams(const Reinhard02::Params& params);

    // Sets the gamma correction. Each pixel's component p, will be raised to
    // the power of 1/gamma (after the exposure correction and clamping to the
    // range [0,1]); the final value is p^(1/gamma). Therefore gamma must be
    // greater than zero; the typical value for LCDs is 2.2.
    // Calling this method implies disabling sRGB
    void setGamma(float gamma);

    // Enables or disables the sRGB curve
    void SetSRGB(bool enable);

    // Returns the gamma employed when sRGB is not used
    inline float Gamma() const {
        return m_gamma;
    }

    // Returns the inverse of the gamma, in case someone needs it
    inline float InvGamma() const {
        return m_invGamma;
    }

    // Returns the exposure
    inline float Exposure() const {
        return m_exposure;
    }

    // Reference to the current set of Reinhard02 parameters
    inline const Reinhard02::Params& ParamsReinhard02() const {
        return m_paramsTMO;
    }

    // Returs whether we are using sRGB or not
    inline bool isSRGB() const {
        return m_useSRGB;
    }


private:

    // Exposure of the image
    float m_exposure;

    // Exposure compensation factor: 2^exposure
    float m_exposureFactor;

    // Gamma correction
    float m_gamma;

    // The actual exponent for the gamma correction: 1/gamma
    float m_invGamma;

    // A flag to know whether to use the simple gamma curve of the sRGB one
    bool m_useSRGB;

    // Parameters for the global Reinhard02 TMO
    Reinhard02::Params m_paramsTMO;

};



} // namespace pcg

#endif /* PCG_TONEMAPPERSOA_H */
