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

// Basic tests for the automatic parameter estimation of the Reinhard02 global
// tone mapping parameters


#include <gtest/gtest.h>

#include <Reinhard02.h>
#include <Image.h>

#include <limits>

#include "dSFMT/RandomMT.h"
#include "tableau_f32.h"


class Reinhard02ParamsTest : public ::testing::Test
{
protected:
    virtual void SetUp() {
        // Python generated: [random.randint(0,0x7fffffff) for i in range(16)]
        const unsigned int seed[] = {1936425658, 1802924535, 830072669,
            1844043852, 1240130396, 1868429525, 369914000, 1235711335,
            1981009527, 1419188630, 1965503171, 1631567956, 1799983148,
            406042851, 74520290, 1750333730};
        rnd.setSeed (seed);
    }

    virtual void TearDown() {

    }

    static const int IMG_W = 640;
    static const int IMG_H = 480;

    static const int NUM_RUNS = 64;

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
};



typedef pcg::Image<pcg::Rgba32F> FloatImage;
typedef std::numeric_limits<float> float_limits;

using pcg::Reinhard02;


TEST_F(Reinhard02ParamsTest, Tableau)
{
    FloatImage img;
    pcg::Tableau::fill (img);
    Reinhard02::Params p = Reinhard02::EstimateParams(img);

    // Values calculated in matlab
    ASSERT_NEAR (p.key,      0.1977469f, 5e-6);
    ASSERT_NEAR (p.l_white, 53.3945084f, 5e-6);
    ASSERT_NEAR (p.l_w,      0.2085711f, 5e-6);
}



TEST_F(Reinhard02ParamsTest, Basic)
{
    FloatImage img(IMG_W, IMG_H);
    for (int i = 0; i < NUM_RUNS; ++i) {
        fillImage(img);
        Reinhard02::Params p = Reinhard02::EstimateParams(img);

        ASSERT_LT (p.l_w, p.l_white);
        ASSERT_GT (p.key, 0.0f);
        ASSERT_LT (p.key, 1.0f);
    }
}



TEST_F(Reinhard02ParamsTest, TinyImg)
{
    for (int i = 0; i < NUM_RUNS; ++i) {
        FloatImage img(1+rnd.nextInt(8), 1+rnd.nextInt(4));
        fillImage(img, 2.0f);
        Reinhard02::Params p = Reinhard02::EstimateParams(img);

        ASSERT_LT (p.l_w, p.l_white);
        ASSERT_GT (p.key, 0.0f);
        ASSERT_LT (p.key, 1.0f);
    }
}



TEST_F(Reinhard02ParamsTest, AllInvalid)
{
    FloatImage img(IMG_W, IMG_H);
    for (int k = 0; k < NUM_RUNS; ++k) {
        for (int i = 0; i < img.Size(); ++i) {
            float val = 0.0f;
            const double p = rnd.nextDouble();
            if (p < 0.25) {
                val = -rnd.nextFloat();
            } else if (p < 0.5) {
                val = float_limits::quiet_NaN();
            } else if (p < 0.75) {
                val = -float_limits::infinity();
            } else if (p < 0.9) {
                val = float_limits::infinity();
            }
            img[i].setAll (val);
        }

        Reinhard02::Params p = Reinhard02::EstimateParams(img);
        ASSERT_EQ (0.0f, p.key);
        ASSERT_EQ (0.0f, p.l_w);
        ASSERT_EQ (0.0f, p.l_white);
    }
}



TEST_F(Reinhard02ParamsTest, SomeInvalid)
{
    FloatImage img(IMG_W, IMG_H);
    for (int k = 0; k < NUM_RUNS; ++k) {
        fillImage (img);
        int numBad=static_cast<int> ((1e-1 - 5e-2*rnd.nextDouble())*img.Size());
        for (int i = 0; i < numBad; ++i) {
            float val = 0.0f;
            const double p = rnd.nextDouble();
            if (p < 0.25) {
                val = -rnd.nextFloat();
            } else if (p < 0.5) {
                val = float_limits::quiet_NaN();
            } else if (p < 0.75) {
                val = -float_limits::infinity();
            } else if (p < 0.9) {
                val = float_limits::infinity();
            }
            img[i].setAll (val);
        }

        Reinhard02::Params p = Reinhard02::EstimateParams(img);
        ASSERT_LT (p.l_w, p.l_white);
        ASSERT_GT (p.key, 0.1f);
        ASSERT_LT (p.key, 0.4f);
    }
}



TEST_F(Reinhard02ParamsTest, OneValid)
{
    FloatImage img(IMG_W, IMG_H);
    for (int k = 0; k < NUM_RUNS; ++k) {
        for (int i = 0; i < img.Size(); ++i) {
            float val = 0.0f;
            const double p = rnd.nextDouble();
            if (p < 0.25) {
                val = -rnd.nextFloat();
            } else if (p < 0.5) {
                val = float_limits::quiet_NaN();
            } else if (p < 0.75) {
                val = -float_limits::infinity();
            } else if (p < 0.9) {
                val = float_limits::infinity();
            }
            img[i].setAll (val);
        }
        int idx = 0;
        if (k == 1) {
            idx = img.Size() - 1;
        } else if (k > 1) {
            idx = rnd.nextInt (img.Size());
        }
        img[idx].setAll (1.0f);

        Reinhard02::Params p = Reinhard02::EstimateParams(img);
        ASSERT_FLOAT_EQ (0.18f, p.key);
        ASSERT_FLOAT_EQ (1.0f, p.l_w);
        ASSERT_FLOAT_EQ (1.5f, p.l_white);
    }
}



TEST_F(Reinhard02ParamsTest, TwoValid)
{
    FloatImage img(IMG_W, IMG_H);
    for (int k = 0; k < NUM_RUNS; ++k) {
        for (int i = 0; i < img.Size(); ++i) {
            float val = 0.0f;
            const double p = rnd.nextDouble();
            if (p < 0.25) {
                val = -rnd.nextFloat();
            } else if (p < 0.5) {
                val = float_limits::quiet_NaN();
            } else if (p < 0.75) {
                val = -float_limits::infinity();
            } else if (p < 0.9) {
                val = float_limits::infinity();
            }
            img[i].setAll (val);
        }
        int idx1 = rnd.nextInt (img.Size());
        int idx2 = rnd.nextInt (img.Size());
        while (idx1 == idx2) {
            idx2 = rnd.nextInt (img.Size());
        }

        float val = 0.0f;
        const double prob = rnd.nextDouble();
        if (prob > 0.1) {
            img[idx1].setAll (rnd.nextFloat());
            img[idx2].setAll (rnd.nextFloat());
        } else {
            val = rnd.nextFloat();
            img[idx1].setAll (val);
            img[idx2].setAll (val);
        }

        Reinhard02::Params p = Reinhard02::EstimateParams(img);
        if (prob > 0.1) {
            ASSERT_LT (p.l_w, p.l_white);
            ASSERT_NEAR (0.18f, p.key, 5e-3f);
            ASSERT_GT (p.key, 0.0f);
            ASSERT_LT (p.key, 1.0f);
        } else {
            ASSERT_FLOAT_EQ (0.18f, p.key);
            ASSERT_FLOAT_EQ (val, p.l_w);
            ASSERT_FLOAT_EQ (1.5f * val, p.l_white);
        }
    }
}



TEST_F(Reinhard02ParamsTest, AllSame)
{
    FloatImage img(IMG_W, IMG_H);
    for (int k = 0; k < NUM_RUNS; ++k) {
        const float val = rnd.nextFloat();
        for (int i = 0; i < img.Size(); ++i) {
            img[i].setAll (val);
        }

        Reinhard02::Params p = Reinhard02::EstimateParams(img);
        ASSERT_NEAR (0.18f, p.key, 1e-4f);
        ASSERT_NEAR (val, p.l_w, 1e-1f);
        ASSERT_NEAR (1.5f*val, p.l_white, 1e-1f);
    }
}



TEST_F(Reinhard02ParamsTest, HistogramKey)
{
    FloatImage img(IMG_W, IMG_H);
    for (int k = 0; k < NUM_RUNS; ++k) {
        fillImage (img, 1.0f);
        // Add a little less than 1% of super bright pixels.
        const int num_pixels = static_cast<int> (0.009f * img.Size());
        for (int i = 0; i < num_pixels; ++i) {
            img[rnd.nextInt(img.Size())] *= 1e20f;
        }

        Reinhard02::Params p = Reinhard02::EstimateParams(img);
        ASSERT_LT (p.l_w, p.l_white);
        ASSERT_LT (0.1f, p.key);
        ASSERT_LT (p.key, 0.35f);
    }
}



TEST_F(Reinhard02ParamsTest, Benchmark)
{
    {
        FloatImage img;
        pcg::Tableau::fill (img);
        for (int i = 0; i < NUM_RUNS; ++i) {
            Reinhard02::Params p = Reinhard02::EstimateParams(img);

            // Values calculated in matlab
            ASSERT_NEAR (p.key,      0.1977469f, 5e-6);
            ASSERT_NEAR (p.l_white, 53.3945084f, 5e-6);
            ASSERT_NEAR (p.l_w,      0.2085711f, 5e-6);
        }
    }

    {
        FloatImage img(IMG_W*4, IMG_H*4);
        fillImage(img);
        for (int i = 0; i < NUM_RUNS; ++i) {
            Reinhard02::Params p = Reinhard02::EstimateParams(img);

            ASSERT_LT (p.l_w, p.l_white);
            ASSERT_GT (p.key, 0.0f);
            ASSERT_LT (p.key, 1.0f);
        }
    }
}
