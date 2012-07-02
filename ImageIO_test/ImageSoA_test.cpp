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

#include "dSFMT/RandomMT.h"
#include "Timer.h"

#include <ImageSoA.h>
#include <Image.h>
#include <Rgba32F.h>

#include <gtest/gtest.h>

#if defined(_MSC_VER) && _MSC_VER < 1600
# ifdef _WIN64
typedef unsigned __int64 uintptr_t;
# else /* _WIN64 */
typedef _W64 unsigned int uintptr_t;
# endif /* _WIN64 */
#else
# include <stdint.h>
#endif


using std::cout;
using std::endl;



class ImageSoATest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        // Python generated: 
        // ['{0:#010x}'.format(random.randint(0,0x7fffffff)) for i in range(16)]
        static const unsigned int seed[] = {    0x4c1ac32f, 0x46151297,
            0xe7ae1f87, 0xfe99af7a, 0x86170530, 0x9e2a55bf, 0xa4cd8189,
            0xd6eadcab, 0x1ac48a9a, 0xa986c6fd, 0xe1a51ba0, 0x5714b8e3,
            0x18360e72, 0x34f9703b, 0x2ad9fd5b, 0x6fa25e53
        };
        m_rnd.setSeed(seed);
    }

    virtual void TearDown()
    {

    }

    template <pcg::ScanLineMode S>
    void fillRnd (pcg::Image<pcg::Rgba32F, S> &img) {
        for (int i = 0; i < img.Size(); ++i) {
            const float s = 1000.0f * m_rnd.nextFloat();
            const float r = s * m_rnd.nextFloat();
            const float g = s * m_rnd.nextFloat();
            const float b = s * m_rnd.nextFloat();
            const float a = m_rnd.nextFloat();
            img[i].set (r, g, b, a);
        }
    }

    // Offset between from a to b in bytes
    template <typename T1, typename T2>
    inline uintptr_t ptrDiff(const T1 *a, const T2 *b) const {
        const uintptr_t ptr1 = reinterpret_cast<const uintptr_t>(a);
        const uintptr_t ptr2 = reinterpret_cast<const uintptr_t>(b);
        return ptr1 > ptr2 ? (ptr1-ptr2) : (ptr2-ptr1);
    }

    typedef pcg::ImageBaseSoA<float, float, float> Image;
    typedef Image::Channel_1 R;
    typedef Image::Channel_2 G;
    typedef Image::Channel_3 B;

    RandomMT m_rnd;
};



TEST_F(ImageSoATest, Basic)
{
    // Test the most basic methods
    Image img;
    ASSERT_EQ(0, img.Width());
    ASSERT_EQ(0, img.Height());
    ASSERT_EQ(0, img.Size());
    ASSERT_EQ(pcg::TopDown, img.GetMode());

    img.Alloc(640, 480);
    ASSERT_EQ(640, img.Width());
    ASSERT_EQ(480, img.Height());
    ASSERT_EQ(640*480, img.Size());
    ASSERT_EQ(pcg::TopDown, img.GetMode());

    img.Clear();
    ASSERT_EQ(0, img.Width());
    ASSERT_EQ(0, img.Height());
    ASSERT_EQ(0, img.Size());
    ASSERT_EQ(pcg::TopDown, img.GetMode());
}


TEST_F(ImageSoATest, BasicAccess)
{
    // Test the most basic methods
    const int N = 100;

    Timer t1, t2;

    // Try with different sizes
    t1.start();
    for (int runIdx = 0; runIdx < N; ++runIdx) {
        // Restrict to ~2GB per test image
        const int w = 1 + m_rnd.nextInt(2048/*13377*/);
        const int h = 1 + m_rnd.nextInt(2048/*13377*/);
        Image img(w, h);
        ASSERT_EQ(w, img.Width());
        ASSERT_EQ(h, img.Height());
        ASSERT_EQ(w*h, img.Size());

        t2.start();
        float * rPtr = img.GetDataPointer<R>();
        float * gPtr = img.GetDataPointer<G>();
        float * bPtr = img.GetDataPointer<B>();

        ASSERT_TRUE(rPtr != NULL);
        ASSERT_TRUE(gPtr != NULL);
        ASSERT_TRUE(bPtr != NULL);
        ASSERT_TRUE(rPtr != gPtr);
        ASSERT_TRUE(gPtr != bPtr);

        // Alignment
        ASSERT_EQ(0, reinterpret_cast<uintptr_t>(rPtr) % 16);
        ASSERT_EQ(0, reinterpret_cast<uintptr_t>(gPtr) % 16);
        ASSERT_EQ(0, reinterpret_cast<uintptr_t>(bPtr) % 16);

        // Padding is a multiple of 64
        ASSERT_EQ(0, ptrDiff(rPtr, gPtr) % 64);
        ASSERT_EQ(0, ptrDiff(gPtr, bPtr) % 64);

        // Fill each item with a deterministic number
        for (int i = 0; i != img.Size(); ++i) {
            rPtr[i] = 0.5f   * (i+1);
            gPtr[i] = 0.25f  * (i+1);
            bPtr[i] = 0.125f * (i+1);
        }

        // Scanline access in Top-Down order slow
        for (int j = 0, val = 1; j != h; ++j) {
            const float *r = img.GetScanlinePointer<R> (j, pcg::TopDown);
            const float *g = img.GetScanlinePointer<G> (j, pcg::TopDown);
            const float *b = img.GetScanlinePointer<B> (j, pcg::TopDown);

            for (int i = 0; i != w; ++i, ++val) {

                const float rExp = 0.5f   * val;
                const float gExp = 0.25f  * val;
                const float bExp = 0.125f * val;

                ASSERT_EQ(rExp, r[i]);
                ASSERT_EQ(gExp, g[i]);
                ASSERT_EQ(bExp, b[i]);
            }
        }
        t2.stop();
    }
    t1.stop();

    //cout << "> Total time:    " << t1.milliTime()*1e-3 << " s" << endl;
    //cout << ">   Access time: " << t2.milliTime()*1e-3 << " s" << endl;
}


TEST_F(ImageSoATest, ElementAccess)
{
    const int N = 100;

    Timer t1, t2;

    // Try with different sizes
    t1.start();
    for (int runIdx = 0; runIdx < N; ++runIdx) {
        // Restrict to ~2GB per test image
        const int w = 1 + m_rnd.nextInt(2048/*13377*/);
        const int h = 1 + m_rnd.nextInt(2048/*13377*/);
        Image img(w, h);
        ASSERT_EQ(w, img.Width());
        ASSERT_EQ(h, img.Height());
        ASSERT_EQ(w*h, img.Size());

        t2.start();
        float * rPtr = img.GetDataPointer<R>();
        float * gPtr = img.GetDataPointer<G>();
        float * bPtr = img.GetDataPointer<B>();

        // Fill each item with a deterministic number
        for (int i = 0; i != img.Size(); ++i) {
            rPtr[i] = 0.5f   * (i+1);
            gPtr[i] = 0.25f  * (i+1);
            bPtr[i] = 0.125f * (i+1);
        }

        // Per element access, *VERY* slow
        for (int j = 0, val = 1; j != h; ++j) {
            for (int i = 0; i != w; ++i, ++val) {
                const int idx = img.GetIndex(i, j);
                ASSERT_EQ(val-1, idx);
                int x = -1, y = -1;
                img.GetIndices(idx, x, y);
                ASSERT_EQ(i, x);
                ASSERT_EQ(j, y);

                const float rExp = 0.5f   * val;
                const float gExp = 0.25f  * val;
                const float bExp = 0.125f * val;

                ASSERT_EQ(rExp, img.ElementAt<R>(idx));
                ASSERT_EQ(gExp, img.ElementAt<G>(idx));
                ASSERT_EQ(bExp, img.ElementAt<B>(idx));

                ASSERT_EQ(rExp, img.ElementAt<R>(i, j, pcg::TopDown));
                ASSERT_EQ(gExp, img.ElementAt<G>(i, j, pcg::TopDown));
                ASSERT_EQ(bExp, img.ElementAt<B>(i, j, pcg::TopDown));
            }
        }
        t2.stop();
    }
    t1.stop();

    //cout << "> Total time:    " << t1.milliTime()*1e-3 << " s" << endl;
    //cout << ">   Access time: " << t2.milliTime()*1e-3 << " s" << endl;
}



TEST_F(ImageSoATest, CopyConstruct)
{
    const int N = 100;
    Timer t1;

    for (int runIdx = 0; runIdx < N; ++runIdx) {
        const int width  = 1 + m_rnd.nextInt(2048);
        const int height = 1 + m_rnd.nextInt(2048);
        pcg::Image<pcg::Rgba32F> imgOrig(width, height);

        fillRnd(imgOrig);

        t1.start();
        pcg::RGBImageSoA img(imgOrig);
        t1.stop();

        for (int j = 0; j != height; ++j) {
            const float *r = img.GetScanlinePointer<R> (j, pcg::TopDown);
            const float *g = img.GetScanlinePointer<G> (j, pcg::TopDown);
            const float *b = img.GetScanlinePointer<B> (j, pcg::TopDown);
            const pcg::Rgba32F *p = imgOrig.GetScanlinePointer(j, pcg::TopDown);

            for (int i = 0; i != width; ++i) {
                pcg::Rgba32F pixel(p[i]);
                pixel.applyAlpha();
                ASSERT_EQ(pixel.r(), r[i]);
                ASSERT_EQ(pixel.g(), g[i]);
                ASSERT_EQ(pixel.b(), b[i]);
            }
        }
    }

    cout << "> Copy-constructor time: " << t1.milliTime()*1e-3 << " s" << endl;
}
