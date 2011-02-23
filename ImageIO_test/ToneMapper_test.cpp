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

// Basic sanity checks for the tone mapper

#include <gtest/gtest.h>

#include <ToneMapper.h>

#include "dSFMT/RandomMT.h"

#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>


// Helper functions
namespace
{

template <typename T>
inline T clamp (T x, T min_val, T max_val) {
    return std::min(std::max (x, min_val), max_val);
}


// Quantizes the given pixel values
// The template is to be used as pcg::Bgra8 or pcg::Bgra16
template <typename T>
inline void quantize (float r, float g, float b, T &out)
{
    const float qFactor = static_cast<float>((1<<(sizeof(typename T::pixel_t) * 8)) - 1);

    const typename T::pixel_t rQ =
        static_cast<typename T::pixel_t>(clamp(r,0.0f,1.0f) * qFactor);
    const typename T::pixel_t gQ =
        static_cast<typename T::pixel_t>(clamp(g,0.0f,1.0f) * qFactor);
    const typename T::pixel_t bQ =
        static_cast<typename T::pixel_t>(clamp(b,0.0f,1.0f) * qFactor);

    out.set (rQ, gQ, bQ);
}

// Raises the pixel value to invGamma, ie 1/2.2
// The template is to be used as pcg::Bgra8 or pcg::Bgra16
template <typename T>
void applyGamma (const pcg::Rgba32F &p, float invGamma, T &out, 
                 float scaleFactor = 1.0f)
{
    const float r = powf(p.r() * scaleFactor, invGamma);
    const float g = powf(p.g() * scaleFactor, invGamma);
    const float b = powf(p.b() * scaleFactor, invGamma);
    
    quantize (r,g,b, out);
}

// Applies the sRGB transform
inline float sRGB(float x) {
    return (x > 0.0031308f ? (1.055f)*powf(x, 1.0f/2.4f)-0.055f : 12.92f*x);
}

// The template is to be used as pcg::Bgra8 or pcg::Bgra16
template <typename T>
void applySRGB (const pcg::Rgba32F &p, T &out, float scaleFactor = 1.0f)
{
    const float r = sRGB(p.r() * scaleFactor);
    const float g = sRGB(p.g() * scaleFactor);
    const float b = sRGB(p.b() * scaleFactor);

    quantize (r,g,b, out);
}

template <typename T>
inline int abs_diff (T a, T b)
{
    return abs(static_cast<int>(a) - static_cast<int>(b));
}

// Applies the Reinhard02 operations to the given pixel (inout parameter)
void applyReinhard02(pcg::Rgba32F &p, const pcg::Reinhard02::Params &params)
{
    const float Lwhite2 = params.l_white * params.l_white;
    const float &Lwp = params.l_w;
    const float &a = params.key;
    const float Lw = 0.27f*p.r() + 0.67f*p.g() + 0.06f*p.b();

    const float partA = Lwp / (a * Lwhite2);
    const float partB = (a*a*Lwhite2 - Lwp*Lwp) / (a * Lwhite2);
    const float Ls = partA + partB / (Lwp + a*Lw);

    const pcg::Rgba32F res = p * Ls;
    p = res;
}


template <typename T>
struct TypeHelper {
    inline static bool isRgba16() { return false; }
};

template <>
struct TypeHelper<pcg::Rgba16> {
    inline static bool isRgba16() { return true; }
};

} // namespace


// The types for the template are Bgra8 and Bgra16
template <typename T>
class ToneMapperTest : public ::testing::Test
{
protected:
    virtual void SetUp() {
        // Python generated: [random.randint(0,0x7fffffff) for i in range(16)]
        const unsigned int seed[] = {535466026, 1343170414, 1683786703, 
            1995014758, 10984600, 2015400080, 1669735235, 530613480, 1568481079,
            1399928611, 568229577, 752577014, 1185509304, 1120863553,
            1877047419, 1751260945};
        rnd.setSeed (seed);
    }

    virtual void TearDown() {

    }

    static const int NUM_RUNS = 2000000;
    static const ptrdiff_t NUM_TEST_PIXELS = 2000000;

    static const int IMG_W = 640;
    static const int IMG_H = 480;

    static const int LUT_SIZE = 8192;
    static const int NON_LUT_TOL = 1;

    RandomMT rnd;

public:
    template <pcg::ScanLineMode S>
    void fillImage (pcg::Image<pcg::Rgba32F, S> &img, float scale = 8.0f)
    {
        for (int i = 0; i < img.Size(); ++i) {
            img[i].setR(rnd.nextFloat() * scale);
            img[i].setG(rnd.nextFloat() * scale);
            img[i].setB(rnd.nextFloat() * scale);
            img[i].setA(1.0f);
        }
    }


    // Helper struct to create tests for gamma and exposure
    template <typename M, pcg::ScanLineMode Src_m, pcg::ScanLineMode Dest_m>
    struct TestHelper
    {
    private:
        template <typename N, int dummy=0>
        struct RunHelper {
            static void run(TestHelper &t, bool useLut, 
                pcg::TmoTechnique technique) {
                t.tm.ToneMap (t.dest, t.src, useLut, technique);
            }
        };

        template <int dummy>
        struct RunHelper<pcg::Rgba16, dummy> {
            static void run(TestHelper &t, bool useLut, 
                pcg::TmoTechnique technique) {
                if (!useLut) {
                    t.tm.ToneMap (t.dest, t.src, technique);
                } else {
                    const std::string msg("Rgba16 never uses the LUT!");
                    throw std::runtime_error(msg);
                }

            }
        };

        // Helper method to run the actual code
        template <bool isSRGB, pcg::TmoTechnique technique>
        void doToneMapActual(const float scaleFactor, const float invGamma)
        {
            if (technique == pcg::REINHARD02) {
                pcg::Reinhard02::Params params = 
                        pcg::Reinhard02::EstimateParams(src);
                    tm.SetParams(params);
            }

            for (int h = 0; h < IMG_H; ++h) {
                pcg::Rgba32F * src_ptr = 
                    src.GetScanlinePointer (h, pcg::TopDown);
                M * dest_ptr = 
                    reference.GetScanlinePointer (h, pcg::TopDown);
                
                if (technique == pcg::REINHARD02) {

                    for (int w = 0; w < IMG_W; ++w) {
                        pcg::Rgba32F pix = src_ptr[w] * scaleFactor;
                        ::applyReinhard02(pix, tm.ParamsReinhard02());
                        if (isSRGB)
                            ::applySRGB(pix, dest_ptr[w]);
                        else
                            ::applyGamma(pix, invGamma, dest_ptr[w]);
                    }
                } else {
                    for (int w = 0; w < IMG_W; ++w) {
                        if (isSRGB)
                            ::applySRGB(src_ptr[w], dest_ptr[w], scaleFactor);
                        else
                            ::applyGamma(src_ptr[w], invGamma, dest_ptr[w],scaleFactor);
                    }
                }
            }
        }

        template <bool isSRGB>
        void doToneMap(const float scaleFactor, const float invGamma = 1.0f)
        {
            // Call the appropriate method
            switch (T::tmo()) {
            case pcg::REINHARD02:
                doToneMapActual<isSRGB, pcg::REINHARD02> (scaleFactor,invGamma);
                break;

            default:
                doToneMapActual<isSRGB, pcg::EXPOSURE> (scaleFactor,invGamma);
            }
        }

    public:

        pcg::Image <pcg::Rgba32F, Src_m> src;
        pcg::Image <M, Dest_m> dest;
        pcg::Image <M, Dest_m> reference;

        pcg::ToneMapper tm;

        TestHelper (ToneMapperTest &test,
            float gamma, float exposure,
            unsigned short lutSize) : 
        src(IMG_W,IMG_H), dest(IMG_W,IMG_H), reference(IMG_W,IMG_H),
        tm(lutSize)
        {
            // Initialize with random data
            test.fillImage (src);

            // Setup the tonemapper
            tm.SetExposure(exposure);
            tm.SetGamma(gamma);
            tm.SetSRGB(false);

            // Run the reference
            const float scaleFactor = powf(2.0f, exposure);
            const float invGamma = static_cast<float>(1.0/gamma);
            doToneMap<false> (scaleFactor, invGamma);
        }


        // This is for testing sRGB
        TestHelper (ToneMapperTest &test, float exposure,
            unsigned short lutSize) : 
        src(IMG_W,IMG_H), dest(IMG_W,IMG_H), reference(IMG_W,IMG_H),
        tm(lutSize)
        {
            // Initialize with random data
            test.fillImage (src);

            // Setup the tonemapper
            tm.SetExposure(exposure);
            tm.SetSRGB(true);

            // Setup the reference image, scanline by scanline
            const float scaleFactor = powf(2.0f, exposure);
            doToneMap<true> (scaleFactor);
        }

        void toneMap(bool useLut) {
            // Run the tone mapper
            RunHelper<M>::run(*this, useLut, T::tmo());
        }
    };
};



// Helper stuff to create tests combining types and TMOs
template <typename T, pcg::TmoTechnique technique>
struct TmoTest
{
    typedef T pix_t;

    static pcg::TmoTechnique tmo() {
        return technique;
    }

    static bool isRgba16() {
        return ::TypeHelper<T>::isRgba16();
    }
};



typedef ::testing::Types< 
    TmoTest<pcg::Bgra8,  pcg::EXPOSURE>,
    TmoTest<pcg::Rgba8,  pcg::EXPOSURE>,
    TmoTest<pcg::Rgba16, pcg::EXPOSURE>,
    TmoTest<pcg::Bgra8,  pcg::REINHARD02>,
    TmoTest<pcg::Bgra8,  pcg::REINHARD02>,
    TmoTest<pcg::Rgba16, pcg::REINHARD02> 
> TmoTestTypes;

TYPED_TEST_CASE(ToneMapperTest, TmoTestTypes);

namespace 
{

// Actual comparison kernel for the tests
template <typename T>
struct Comparator {
    template <typename M, pcg::ScanLineMode Src_m, pcg::ScanLineMode Dest_m>
    static void
    compare(typename ToneMapperTest<T>::template TestHelper<M, Src_m, Dest_m> & test,
            int tolerance = -1)
    {
        if (tolerance < 0) {
            tolerance = test.tm.MaxLUTError();
        }

        // Compare the reference and the destination
        for (int i = 0; i < test.reference.Size(); ++i) {
            const M &expected = test.reference[i];
            const M &result   = test.dest[i];

            ASSERT_LE(::abs_diff(expected.r, result.r), tolerance);
            ASSERT_LE(::abs_diff(expected.g, result.g), tolerance);
            ASSERT_LE(::abs_diff(expected.b, result.b), tolerance);
        }
    }
};

} // namespace

using pcg::TopDown;
using pcg::BottomUp;

TYPED_TEST(ToneMapperTest, TT_Gamma22_EXP_0)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, TopDown, TopDown>
      test(*this, 2.2f, 0.0f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}



TYPED_TEST(ToneMapperTest, BB_Gamma10_EXP_M2)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, BottomUp, BottomUp>
      test(*this, 1.f, -2.0f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}



TYPED_TEST(ToneMapperTest, TB_Gamma22_EXP_0)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, TopDown, BottomUp>
      test(*this, 2.2f, 0.0f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}



TYPED_TEST(ToneMapperTest, BT_Gamma10_EXP_M2)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, BottomUp, TopDown>
      test(*this, 1.f, -2.0f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}



TYPED_TEST(ToneMapperTest, TB_SRGB_EXP_M2)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, TopDown, BottomUp>
      test(*this, -2.0f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}



TYPED_TEST(ToneMapperTest, BT_SRGB_EXP_0)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, BottomUp, TopDown>
      test(*this, 0.0f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}



TYPED_TEST(ToneMapperTest, TT_SRGB_EXP_3M)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, TopDown, TopDown>
      test(*this, -3.0f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}



TYPED_TEST(ToneMapperTest, BB_SRGB_EXP_25M)
{
    typedef ToneMapperTest<TypeParam> test_t;
    typename test_t::template
      TestHelper<typename TypeParam::pix_t, BottomUp, BottomUp>
      test(*this, -2.5f, test_t::LUT_SIZE);

    // Without the LUT
    test.toneMap(false);
    ::Comparator<TypeParam>::compare (test, test_t::NON_LUT_TOL);
    
    // Run with LUT
    if (!TypeParam::isRgba16()) {
        test.toneMap(true);
        ::Comparator<TypeParam>::compare (test);
    }
}

