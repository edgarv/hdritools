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
    // As implemented in Mitsuba 0.3, with re-derives matrices with extra digits
    void mitsuba(const float& r, const float& g, const float& b,
        float *rOut, float *gOut, float *bOut) const
    {
        // from sRGB to XYZ
        float X = 0.4123908f*r + 0.3575843f*g + 0.1804808f*b;
        float Y = 0.2126390f*r + 0.7151687f*g + 0.0721923f*b;
        float Z = 0.0193308f*r + 0.1191948f*g + 0.9505321f*b;
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
        *rOut =  3.2409699f*X + -1.5373832f*Y + -0.4986108f*Z;
        *gOut = -0.9692436f*X +  1.8759675f*Y +  0.0415551f*Z;
        *bOut =  0.0556301f*X + -0.2039770f*Y +  1.0569715f*Z;
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
        static const float ONE = float(1.0f);

        // Get the luminance
        const float Y = LVec[0] * r + LVec[1] * g + LVec[2] * b;

        // Compute the scale
        const float Lp = P * Y;
        const float k = (P * (ONE + Q*Lp)) / (ONE + Lp);

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

        P = float(params.key / params.l_w);
        Q = float(invWpSqr);
    }

private:
    // For mitsuba
    float key, invWpSqr;

    // For ImageIO
    float P, Q;
};



class ReferenceToneMapper
{
public:
    ReferenceToneMapper() :
    m_exposureFactor(1.0f), m_useSRGB(true), m_invGamma(1.0f/2.2f)
    {}
    
    void ToneMap(pcg::Image<pcg::Bgra8, pcg::TopDown>& dest,
        const pcg::Image<pcg::Rgba32F, pcg::TopDown>& src,
        pcg::TmoTechnique technique = pcg::EXPOSURE) const
    {
        typedef pcg::Bgra8::pixel_t pixel_t;

        assert(src.Width()  == dest.Width());
        assert(src.Height() == dest.Height());
        const int size = src.Size();

        for (int i = 0; i != size; ++i) {
            const pcg::Rgba32F pix = technique == pcg::EXPOSURE ?
                m_exposureFactor * src[i] : m_reinhard02.mitsuba(src[i]);
            float r = std::min(1.0f, std::max(0.0f, pix.r()));
            float g = std::min(1.0f, std::max(0.0f, pix.g()));
            float b = std::min(1.0f, std::max(0.0f, pix.b()));

            // Display correction
            if (m_useSRGB) {
                r = sRGB(r);
                g = sRGB(g);
                b = sRGB(b);
            } else {
                r = gamma(r);
                g = gamma(g);
                b = gamma(b);
            }

            dest[i].set(
                static_cast<pixel_t>(255*r + 0.5f),
                static_cast<pixel_t>(255*g + 0.5f),
                static_cast<pixel_t>(255*b + 0.5f));
        }
    }

    inline void setExposure(float exposure) {
        m_exposureFactor = pow(2.0f, exposure);
    }

    inline void SetSRGB(bool enable) {
        m_useSRGB = enable;
    }

    inline void SetParams(const pcg::Reinhard02::Params& params) {
        m_reinhard02.setParams(params);
    }


private:

    inline float gamma(float x) const {
        return pow(x, m_invGamma);
    }

    inline float sRGB(float x) const {
        const static float CUTOFF_sRGB = float(0.00304f);
        return x > CUTOFF_sRGB ?
            (1.055f * pow(x, 1.0f/2.4f) - 0.055f) :
            (12.92f * x);
    }

    float m_exposureFactor;
    float m_useSRGB;
    float m_invGamma;
    Reinhard02_Method m_reinhard02;
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



TEST_F(ToneMapperSoATest, Process1)
{
    pcg::Image<pcg::Rgba32F> img(3, 3);
    pcg::Image<pcg::Bgra8> outImg(img.Width(), img.Height());
    pcg::Image<pcg::Bgra8> outImgOld(img.Width(), img.Height());
    pcg::Image<pcg::Bgra8> outImgRef(img.Width(), img.Height());
    fillRnd(img);
    pcg::Reinhard02::Params params = pcg::Reinhard02::EstimateParams(img);
    const int size = img.Size();
    
    // There should be a better way...
    for (int i = 0; i != size; ++i) {
        outImg[i].set(0, 0, 0, 0);
        outImgOld[i].set(0, 0, 0, 0);
        outImgRef[i].set(0, 0, 0, 0);
    }

    pcg::ToneMapperSoA tm;
    tm.SetParams(params);
    tm.SetSRGB(true);
    

    pcg::ToneMapper tmOld(0.0f, 4096); // QtImage settings
    tmOld.SetParams(params);
    tmOld.SetSRGB(true);
    

    ReferenceToneMapper tmRef;
    tmRef.SetParams(params);
    tmRef.SetSRGB(true);
    

    Timer tSoA;
    Timer tOld;
    Timer tRef;
    const int N = 10;
    for (int i = 0; i != N; ++i) {
        tSoA.start();
        tm.ToneMap(outImg, img, pcg::REINHARD02);
        tSoA.stop();

        tOld.start();
        tmOld.ToneMap(outImgOld, img, true, pcg::REINHARD02);
        tOld.stop();

        tRef.start();
        tmRef.ToneMap(outImgRef, img, pcg::REINHARD02);
        tRef.stop();
    }

    // Conversion factor to get the average time in ms
    const double factor = 1e-6 / N;

    // Only print if the size is small enough
    if (size <= 10) {
        for (int i = 0; i != size; ++i) {
            cout << img[i] << endl;
            cout << "   " << outImg[i]    << endl;
            cout << "   " << outImgOld[i] << endl;
            cout << "  *" << outImgRef[i] << endl;
        }
    }

    cout << "Time SoA: " << tSoA.nanoTime()*factor << " ms" << endl;
    cout << "Time Old: " << tOld.nanoTime()*factor << " ms" << endl;
    cout << "Time Ref: " << tRef.nanoTime()*factor << " ms" << endl;
}
