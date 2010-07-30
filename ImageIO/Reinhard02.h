#if !defined PCG_REINHARD02_H
#define PCG_REINHARD02_H

#include "ImageIO.h"
#include "Image.h"
#include "Rgba32F.h"

namespace pcg
{

class Reinhard02
{
public:
    // Struct to hold the required statistics so that the tone mapper may
    // be applied efficiently. Keeping this values assures that the tone
    // mapping curve will be the same
    struct Params
    {
        // Key, referenced as "a" in the paper's equations
        float key;

        // Luminance of the white point, "L_{white}"
        float l_white;

        // Log average luminance, "L_{w}"
        float l_w;

        Params() : key(0.18f), l_white(1.0f), l_w(0.18f) {}

        Params(float a, float Lwhite, float Lw) : 
        key(a), l_white(Lwhite), l_w(Lw) {}
    };


    // Gets default parameters
    template <ScanLineMode S>
    static Params EstimateParams (const Image<Rgba32F, S> &img)
    {
        return EstimateParams (img.GetDataPointer(), img.Size());
    }


protected:
    static IMAGEIO_API Params
        EstimateParams (const Rgba32F * pixels, size_t count);
};


} // namespace pcg

#endif /* PCG_REINHARD02_H */
