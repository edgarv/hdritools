#if !defined(TONEMAPPINGFILTER_H)
#define TONEMAPPINGFILTER_H

#include <ToneMapper.h>

// TBB import for the filter stuff
#include <tbb/pipeline.h>

using tbb::filter;
using pcg::ToneMapper;

// The class in charge of tone mapping. This guy is pretty transparent :)
class ToneMappingFilter : public tbb::filter {

private:

    ToneMapper &toneMapper;
    const bool useBpp16;
    const pcg::TmoTechnique technique;
    const float key;
    const float whitePoint;
    const float logLumAvg;

    inline static bool isReinhard02Fixed(float key,float 
        whitePoint,float logLumAvg) {
        return key != AutoParam() && whitePoint != AutoParam() && 
            logLumAvg != AutoParam();
    }

    inline bool isReinhard02Fixed() {
        return isReinhard02Fixed(key, whitePoint, logLumAvg);
    }

public:

    // Special value for TMO settings to request automatic values
    static inline float AutoParam() {
        return -8192.125f;
    }

    // By default the tone mapped images are RGB with 8 bpp. If the
    // useBpp16 is activated, the saved images will have instead 16 bpp.
    // Note that this makes the tone mapping much slower, and so
    // far the only supported format is PNG.
    ToneMappingFilter(ToneMapper &toneMapper, bool useBpp16);

    // Alternate constructor which uses the Reinhard02 TMO. Given the
    // horribly obfuscated [pcg]ImageIO tonemapping API, unless all
    // parameters are explicitly set the filter will have to become serial
    // since the tone mapper doesn't support re-entrant TMO settings.
    ToneMappingFilter(ToneMapper &toneMapper, bool useBpp16,
        float key, float whitePoint, float logLumAvg);

    // This method receives pointers to ImageInfo structures.
    void* operator()(void* item);
};

#endif /* TONEMAPPINGFILTER_H */
