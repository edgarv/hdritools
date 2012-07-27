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

#pragma once
#if !defined PCG_REINHARD02_H
#define PCG_REINHARD02_H

#include "ImageIO.h"
#include "Image.h"
#include "ImageSoA.h"
#include "Rgba32F.h"

namespace pcg
{

class Reinhard02
{
public:
    // Struct to hold the required statistics so that the tone mapper may
    // be applied efficiently. Keeping this values assures that the tone
    // mapping curve will be the same
    struct IMAGEIO_API Params
    {
        // Key, referenced as "a" in the paper's equations
        float key;

        // Luminance of the white point, "L_{white}"
        float l_white;

        // Log average luminance, "L_{w}"
        float l_w;

        // Minimum luminance
        float l_min;

        // Maximum luminance
        float l_max;

        Params() :
        key(0.18f), l_white(1.0f), l_w(0.18f), l_min(0.0f), l_max(1.0f) {}

        Params(float a, float Lwhite, float Lw, float Lmin, float Lmax) : 
        key(a), l_white(Lwhite), l_w(Lw), l_min(Lmin), l_max(Lmax) {}
    };


    // Gets default parameters
    template <ScanLineMode S>
    static Params EstimateParams (const Image<Rgba32F, S> &img)
    {
        if (img.Size() == 0) {
            throw IllegalArgumentException("Empty image");
        }
        return EstimateParams (img.GetDataPointer(), img.Size());
    }

    static IMAGEIO_API Params EstimateParams (const RGBAImageSoA& img);


private:

    struct LuminanceResult
    {
        size_t zero_count;
        float Lmin;
        float Lmax;
    };

    static IMAGEIO_API Params EstimateParams (afloat_t * const PCG_RESTRICT Lw,
        size_t count, const LuminanceResult& lumResult);

    static IMAGEIO_API Params
        EstimateParams (const Rgba32F * pixels, size_t count);
};


} // namespace pcg

#endif /* PCG_REINHARD02_H */
