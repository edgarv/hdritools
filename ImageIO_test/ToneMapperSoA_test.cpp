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

#include "dSFMT/RandomMT.h"
#include "Timer.h"

#include <ToneMapperSoA.h>
#include <ImageSoA.h>
#include <Image.h>

#include <gtest/gtest.h>

#include <iostream>
#include <algorithm>



class ToneMapperSoATest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        // Python generated
        // ['{0:#010x}'.format(random.randint(0,0x7fffffff)) for i in range(16)]
        static const unsigned int seed[] = {    0x3df63c67, 0x40f9d120,
            0x74dfd181, 0x34d69375, 0x2628a340, 0x7dd742ac, 0x4fe340d0,
            0x4f8324f2, 0x40fa92fe, 0x3ba0c30e, 0x7c4dc300, 0x269d5d0e,
            0x1132e5d4, 0x13cc7fd9, 0x12a4c86c, 0x34ed5c8b
        };
        m_rnd.setSeed(seed);
    }

    virtual void TearDown()
    {
    }

    template <pcg::ScanLineMode S>
    void fillRnd (pcg::Image<pcg::Rgba32F, S> &img)
    {
        for (int i = 0; i < img.Size(); ++i) {
            const float s = static_cast<float>(512 + 32 * m_rnd.nextGaussian());
            const float r = s * m_rnd.nextFloat();
            const float g = s * m_rnd.nextFloat();
            const float b = s * m_rnd.nextFloat();
            img[i].set (r, g, b);
        }
    }

    void fillRnd(pcg::RGBImageSoA &img)
    {
        float *rVals = img.GetDataPointer<pcg::RGBImageSoA::R>();
        float *gVals = img.GetDataPointer<pcg::RGBImageSoA::G>();
        float *bVals = img.GetDataPointer<pcg::RGBImageSoA::B>();
        
        for (int i = 0; i < img.Size(); ++i) {
            const float s = static_cast<float>(512 + 32 * m_rnd.nextGaussian());
            const float r = s * m_rnd.nextFloat();
            const float g = s * m_rnd.nextFloat();
            const float b = s * m_rnd.nextFloat();

            rVals[i] = r;
            gVals[i] = g;
            bVals[i] = b;
        }
    }

    RandomMT m_rnd;
};



namespace
{

// sRGB
struct Reinhard02_Method
{
    // As implemented in Mitsuba 0.3
    void mitsuba(const float& r, const float& g, const float& b,
        float *rOut, float *gOut, float *bOut) const
    {
        // from sRGB to XYZ
        float X = 0.412453f*r + 0.357580f*g + 0.180423f*b;
        float Y = 0.212671f*r + 0.715160f*g + 0.072169f*b;
        float Z = 0.019334f*r + 0.119193f*g + 0.950227f*b;
        const float normalization = 1.0f/(X+Y+Z);

        // from XYZ to xyY
        const float x = X*normalization;
        const float y = Y*normalization;

        // Reinhard02 curve
        const float Lp = Y * key;
        Y = Lp * (1.0f + Lp*invWpSqr) / (1.0f+Lp);

        // from xyY to XYZ
        X = (Y/y) * x;
        Z = (Y/y) * (1.0f - x - y);

        // to sRGB
        *rOut =  3.240479f*X + -1.537150f*Y + -0.498535f*Z;
        *gOut = -0.969256f*X +  1.875991f*Y +  0.041556f*Z;
        *bOut =  0.055648f*X + -0.204043f*Y +  1.057311f*Z;
    }

    inline pcg::Rgba32F mitsuba(const pcg::Rgba32F& pix) const
    {
        float r,g,b;
        mitsuba(pix.r(), pix.g(), pix.b(), &r, &g, &b);
        pcg::Rgba32F result(r, g, b);
        return result;
    }


    // As implemented here
    void imageio(const float& r, const float& g, const float& b,
        float *rOut, float *gOut, float *bOut) const
    {
        static const float LVec[] = {
            float(0.212639005871510f), float(0.715168678767756f), float(0.072192315360734f)
        };

        // Get the luminance
        const float Y = LVec[0] * r + LVec[1] * g + LVec[2] * b;

        // Compute the scale
        const float k = (P + R * Y) / (Q + P * Y);

        // And apply
        *rOut = k * r;
        *gOut = k * g;
        *bOut = k * b;
    }

    inline pcg::Rgba32F imageio(const pcg::Rgba32F& pix) const
    {
        float r,g,b;
        imageio(pix.r(), pix.g(), pix.b(), &r, &g, &b);
        pcg::Rgba32F result(r, g, b);
        return result;
    }

    
    // Setup the internal constants
    void setParams(const pcg::Reinhard02::Params& params)
    {
        key = params.key / params.l_w;
        invWpSqr = 1.0f / (params.l_white * params.l_white);

        P = float(params.l_w * params.key   * (params.l_white * params.l_white));
        Q = float((params.l_w * params.l_w) * (params.l_white * params.l_white));
        R = float(params.key * params.key);
    }

private:
    // For mitsuba
    float key, invWpSqr;

    // For ImageIO
    float P, Q, R;
};


} // namespace



TEST_F(ToneMapperSoATest, Reinhard02Scaling)
{
    using std::cout;
    using std::endl;
    const int N = 10;

    double maxRelError = 0.0;
    double maxAbsError = 0.0;

    for (int count = 0; count < N; ++count) {
        pcg::Image<pcg::Rgba32F> img(1024, 1024);
        fillRnd(img);
        
        pcg::Reinhard02::Params params = pcg::Reinhard02::EstimateParams(img);
        Reinhard02_Method m;
        m.setParams(params);
        
        const pcg::Rgba32F* pixels = img.GetDataPointer();
        for (int i = 0; i < img.Size(); ++i) {
            pcg::Rgba32F mts     = m.mitsuba(pixels[i]);
            pcg::Rgba32F imageio = m.imageio(pixels[i]);
    #if 0
            cout << pixels[i] << endl
                 << "  mts:     " << mts << endl
                 << "  imageio: " << imageio << endl;
    #endif
            // Component-wise error for R,G,B
            for (int k = 1; k < 4; ++k) {
                double ref = mts[k];
                double val = imageio[k];
                double absError = fabs(ref - val);
                double relError = ref != 0 ? absError / ref : 0.0f;

                EXPECT_TRUE(absError < 1e-4) << "Error for " << pixels[i] \
                    << " at idx=" << k << ", relerror=" << relError << endl \
                    << "  mts:     " << mts << endl \
                    << "  imageio: " << imageio << endl;

                maxAbsError = std::max(absError, maxAbsError);
                maxRelError = std::max(relError, maxRelError);
            }
        }
    }

    cout << "Max absolute error: " << maxAbsError << endl;
    cout << "Max relative error: " << maxRelError << endl;
}



TEST_F(ToneMapperSoATest, Reinhard02Benchmark1)
{
    using std::cout;
    using std::endl;
    const int N = 10000;
    Timer tMts, tImageIO;

    // Tiny image which easily fits in the L2 cache
    pcg::Image<pcg::Rgba32F> img(64, 128);
    fillRnd(img);

    pcg::Reinhard02::Params params = pcg::Reinhard02::EstimateParams(img);
    Reinhard02_Method m;
    m.setParams(params);

    const pcg::Rgba32F* pixels = img.GetDataPointer();

    // Something silly just to make sure it is not optimized away
    pcg::Rgba32F dummy(0.0f);

    // Warm up
    for (int k = 0; k < 100; ++k) {
        for (int i = 0; i < img.Size(); ++i) {
            pcg::Rgba32F p = m.mitsuba(pixels[i]);
            dummy *= p;
         }
    }
    EXPECT_FLOAT_EQ(0.0f, dummy.a());

    // Real one
    tMts.start();
    for (int k = 0; k < N; ++k) {
        for (int i = 0; i < img.Size(); ++i) {
            pcg::Rgba32F p = m.mitsuba(pixels[i]);
            dummy *= p;
         }
    }
    tMts.stop();
    EXPECT_FLOAT_EQ(0.0f, dummy.a());


    // Warm up
    for (int k = 0; k < 100; ++k) {
        for (int i = 0; i < img.Size(); ++i) {
            pcg::Rgba32F p = m.imageio(pixels[i]);
            dummy *= p;
         }
    }
    EXPECT_FLOAT_EQ(0.0f, dummy.a());

    // Real one
    tImageIO.start();
    for (int k = 0; k < N; ++k) {
        for (int i = 0; i < img.Size(); ++i) {
            pcg::Rgba32F p = m.imageio(pixels[i]);
            dummy *= p;
         }
    }
    tImageIO.stop();
    EXPECT_FLOAT_EQ(0.0f, dummy.a());

    cout << "Time mitsuba:    " << tMts.nanoTime()*1e-9 << "s" << endl;
    cout << "Time ImageIO:    " << tImageIO.nanoTime()*1e-9 << "s" << endl;
    cout << "ImageIO/mitsuba: "
         << (100.0*tImageIO.nanoTime())/tMts.nanoTime() << "%" << endl;
}
