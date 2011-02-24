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

#include <rgbe.h>

#include "dSFMT/RandomMT.h"
#include "Timer.h"
#include "TestUtil.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <limits>


class RgbeTest : public ::testing::Test
{
protected:
    virtual void SetUp() {
        // Python generated: [random.randint(0,0x7fffffff) for i in range(16)]
        const unsigned int seed[] = {274633028, 432980773, 191609407, 
            1927611825, 706090662, 410199945, 236054990, 1101184465, 1778191207,
            1348078452, 818332507, 2078156496, 141535889, 841923516, 1795007374,
            709633697};
        rnd.setSeed (seed);
    }

    virtual void TearDown() {

    }

    static const int NUM_RUNS = 2000000;
    static const ptrdiff_t NUM_TEST_PIXELS = 2000000;

    RandomMT rnd;
};

namespace
{



// Helper struct to calculate a hash code for a sequence of values
template <typename T, int M = 41>
struct Hash
{
public:
    Hash(int initialVal = 17) : m_data(initialVal) {}

    void update(const T& value)
    {
        const int *ptr = reinterpret_cast<const int*>((const void*)&value);
        for (size_t i = 0; i < sizeof(T)/sizeof(int); ++i) {
            m_data += M * ptr[i];
        }
    }

    int hashCode() const {
        return m_data;
    }

private:
    int m_data;
};

typedef Hash<pcg::Rgbe> hash_rgbe;



struct DeltaRgbe
{
    int dR;
    int dG;
    int dB;
    int total;

    DeltaRgbe() : dR(0), dG(0), dB(0), total(0) {}

    void update(const pcg::Rgbe &m, const pcg::Rgbe &n) {
        int oldR = dR, oldG = dG, oldB = dB;
        dR += abs((int)m.r - (int)n.r);
        dG += abs((int)m.g - (int)n.g);
        dB += abs((int)m.b - (int)n.b);
        if (oldR != dR || oldG != dG || oldB != dB) {
            ++total;
            ASSERT_LE(total, (dR+dG+dB));
        }
    }

    bool isEmpty() const {
        return dR == 0 && dG == 0 && dB == 0;
    }
};



// Convert to RGBE using the standard code
void float2Rgbe(float red, float green, float blue, 
                       pcg::Rgbe &outRgbe)
{
	float v;
	int e;

    // Clamp to zero
    red   = std::max(red,   0.0f);
    green = std::max(green, 0.0f);
    blue  = std::max(blue,  0.0f);

	v = red;
	if (green > v) v = green;
	if (blue > v) v = blue;
	if (v < 1e-32) {
		outRgbe.set(0,0,0,0);
	}
	else {
		double nFactor = (frexp(v,&e) * 256.0/v);
        int r, g, b;
        r = static_cast<int>(red   * nFactor + 0.5);
        g = static_cast<int>(green * nFactor + 0.5);
        b = static_cast<int>(blue  * nFactor + 0.5);

        // Guard for overflow
        if (r > 255 || g > 255 || b > 255) {
            if (e < 127) {
                ++e;
                nFactor *= 0.5;
                r = static_cast<int>(red   * nFactor + 0.5);
                g = static_cast<int>(green * nFactor + 0.5);
                b = static_cast<int>(blue  * nFactor + 0.5);
            } else {
                // Overflow
                outRgbe.set(0,0,0,0);
                return;
            }
        }
        outRgbe.r = static_cast<unsigned char> (r);
		outRgbe.g = static_cast<unsigned char> (g);
		outRgbe.b = static_cast<unsigned char> (b);
		outRgbe.e = static_cast<unsigned char> (e + 128);  // Add the bias
	}
}



inline int floatToIntBits(float f) {
    union { int ival; float fval; };
    fval = f;
    return ival;
}



inline float intBitsToFloat(int n) {
    union { int ival; float fval; };
    ival = n;
    return fval;
}


// Conversion using Bruce's rtgi2 code
void float2RgbeRtgi2(float red, float green, float blue, 
                       pcg::Rgbe &outRgbe)
{
    // Abort when NaNs are detected
    if (red != red || green != green || blue != blue) {
        outRgbe.set(0,0,0,0);
        return;
    }

    //negative values cannot be encoded, so we truncate them to zero
    if (red < 0.0f)   red   = 0.0f;
    if (green < 0.0f) green = 0.0f;
    if (blue < 0.0f)  blue  = 0.0f;

    //find the largest value of the three color components
    float maxValue = (red > green) ? red : green;
    maxValue = (blue > maxValue) ? blue : maxValue;

    //consider all values less than this to be zero.  This constant comes from 
    // Ward's definition in "Real Pixels" Graphics Gems II
    if (maxValue < 1e-32f) {
        outRgbe.set(0,0,0,0);
        return;
    }

    //extract the exponent from the IEEE single precision floating point number
    int biasedExponent = ((floatToIntBits(maxValue)>>23) & 0x0FF);
    if (biasedExponent > 253) {
        // Overflow
        outRgbe.set(0,0,0,0);
        return;
    }

    // construct a additive normalizer which is just 2^(exp+1).
    // Adding this to each float will move the relevant mantissa bits to a known 
    // fixed location for easy extraction
    float additiveNormalizer = intBitsToFloat((biasedExponent+1)<<23);
    //initially we keep an extra bit (9-bits) so that we can perform rounding to 
    //8-bits in the next step
    int rawR = (floatToIntBits(red+additiveNormalizer)>>14)&0x1FF;
    int rawG = (floatToIntBits(green+additiveNormalizer)>>14)&0x1FF;
    int rawB = (floatToIntBits(blue+additiveNormalizer)>>14)&0x1FF;
    // rgbeBiasedExponent = (ieeeBiasedExponent-127) + 129  since IEEE single 
    // float and rgbe have different exponent bias values
    int e = biasedExponent + 2;
    //round to nearest representable 8 bit value
    int r = (rawR+1)>>1;
    int g = (rawG+1)>>1;
    int b = (rawB+1)>>1;

    //check to see if rounding causes an overflow condition and fix if necessary
    if ((r>255)||(g>255)||(b>255)) {
        // ooops rounding caused overflow, need to use larger exponent and redo 
        // the rounding
        e += 1;
        r = (rawR+2)>>2;
        g = (rawG+2)>>2;
        b = (rawB+2)>>2;
        if (e > 255) {
            // Overflow after rounding
            outRgbe.set(0,0,0,0);
            return;
        }
    }

    outRgbe.r = static_cast<unsigned char> (r);
    outRgbe.g = static_cast<unsigned char> (g);
    outRgbe.b = static_cast<unsigned char> (b);
    outRgbe.e = static_cast<unsigned char> (e);
}



// Original transform from RGBE to float
inline void rgbe2float(const pcg::Rgbe &rgbe, 
                       float &outR, float &outG, float &outB)
{
    if (rgbe.e) {   /*nonzero pixel*/
        const float f = static_cast<float>(ldexp(1.0, 
            static_cast<int>(rgbe.e)-(128+8)));
        outR = rgbe.r * f;
        outG = rgbe.g * f;
        outB = rgbe.b * f;
    } else {
        outR = outG = outB = 0.0f;
    }
}



// Bruce's new code from rtgi2
inline void rgbe2floatRtgi2(const pcg::Rgbe &rgbe, 
                       float &outR, float &outG, float &outB)
{
    // Values in the range 1 to 9 would require "denormal" multipliers and are 
    // below minimum values for RGBE exponents anyway so we truncate them to 0
    const float m = intBitsToFloat(rgbe.e > 9 ? ((rgbe.e - 9)<<23) : 0);
    outR = rgbe.r * m;
    outG = rgbe.g * m;
    outB = rgbe.b * m;
}



inline void rgbe2float(const pcg::Rgbe &rgbe, pcg::Rgba32F &outRgba)
{
    float r,g,b;
    rgbe2float(rgbe, r,g,b);
    outRgba.set(r,g,b);
}


inline void rgbe2floatRtgi2(const pcg::Rgbe &rgbe, pcg::Rgba32F &outRgba)
{
    float r,g,b;
    rgbe2floatRtgi2(rgbe, r,g,b);
    outRgba.set(r,g,b);
}



bool close_equals (const pcg::Rgbe &m, const pcg::Rgbe &n) {
    return abs((int)m.r-n.r) <= 1 && abs((int)m.g-n.g) <= 1 && 
           abs((int)m.b-n.b) <= 1 && (m.e == n.e);
}



bool equals (const pcg::Rgbe &m, const pcg::Rgbe &n) {
    return (m.r == n.r) && (m.g == n.g) && (m.b == n.b) && (m.e == n.e);
}


} // namespace



// Test the implemented set methods against a reference implementation
TEST_F(RgbeTest, Set)
{
    pcg::Rgbe zero, expected;
    zero.r = zero.g = zero.b = zero.e = 0;
    {
        pcg::Rgbe v;
        ASSERT_PRED2(equals, zero, v);
    }

    {
        expected.r = 128;
        expected.g =  64;
        expected.b =  32;
        expected.e = 128;

        pcg::Rgbe v(0.5f, 0.25f, 0.125f);
        ASSERT_PRED2(equals, expected, v);

        expected.e = 129;
        v.set(1.0f, 0.5f, 0.25f);
        ASSERT_PRED2(equals, expected, v);

        v.set(0.0f, 0.0f, 0.0f);
        ASSERT_PRED2(equals, zero, v);

        // Test NaN and Infinity
        v.set(1.0f, 0.0f, std::numeric_limits<float>::quiet_NaN());
        ASSERT_PRED2(equals, zero, v);

        v.set(1.0f, 0.0f, std::numeric_limits<float>::infinity());
        ASSERT_PRED2(equals, zero, v);
    }

    // Stress test
    DeltaRgbe dRgbe;
    for (int i = 0; i < NUM_RUNS; ++i) {
        float r = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float g = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float b = static_cast<float>(rnd.nextDouble() * 0xFFFE);

        pcg::Rgbe v(r, g, b);
        float2Rgbe (r, g, b, expected);
        ASSERT_PRED2 (close_equals, expected, v);
        dRgbe.update (expected, v);
        
        float2RgbeRtgi2 (r, g, b, expected);
        ASSERT_PRED2 (equals, expected, v);
        dRgbe.update (expected, v);

        pcg::Rgba32F test (r, g, b);
        v.set (test);
        ASSERT_PRED2 (equals, expected, v);
        dRgbe.update (expected, v);
    }

    if (!dRgbe.isEmpty()) {
        printf("  Different values: {%d, %d, %d}\n",dRgbe.dR,dRgbe.dG,dRgbe.dB);
    }
}



TEST_F(RgbeTest, SetPosNeg)
{
    pcg::Rgbe expected, v;

    expected.r =   0;
    expected.g = 128;
    expected.b =  64;
    expected.e = 127;
    v.set(-1.0f, 0.25f, 0.125f);
    ASSERT_PRED2(equals, expected, v);

    // Stress test
    DeltaRgbe dRgbe;
    for (int i = 0; i < NUM_RUNS; ++i) {
        float r = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float g = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float b = static_cast<float>(rnd.nextDouble() * 0xFFFE);

        r *= (rnd.nextDouble() > 0.8) ? -1.0f : 1.0f;
        g *= (rnd.nextDouble() > 0.8) ? -1.0f : 1.0f;
        b *= (rnd.nextDouble() > 0.8) ? -1.0f : 1.0f;

        pcg::Rgbe v (r, g, b);
        float2Rgbe (r, g, b, expected);
        ASSERT_PRED2 (close_equals, expected, v);
        dRgbe.update (expected, v);

        float2RgbeRtgi2 (r, g, b, expected);
        ASSERT_PRED2 (equals, expected, v);
        dRgbe.update (expected, v);

        pcg::Rgba32F test (r, g, b);
        v.set (test);
        ASSERT_PRED2 (equals, expected, v);
        dRgbe.update (expected, v);
    }

    if (!dRgbe.isEmpty()) {
        printf("  Different values: {%d, %d, %d}\n",dRgbe.dR,dRgbe.dG,dRgbe.dB);
    }
}



// Really simple test case
TEST_F(RgbeTest, UcharPtrCast)
{
    pcg::Rgbe v(0.85f, 0.9532f, 0.25f);
    const unsigned char *ptr = v;

    ASSERT_EQ(v.r, ptr[0]);
    ASSERT_EQ(v.g, ptr[1]);
    ASSERT_EQ(v.b, ptr[2]);
    ASSERT_EQ(v.e, ptr[3]);
}



TEST_F(RgbeTest, Rgba32FCast)
{
    {
        const pcg::Rgba32F expected(0.5f, 0.25f, 0.125f);
        const pcg::Rgbe v(128, 64, 32, 128);
        const pcg::Rgba32F result = v;
        ASSERT_RGBA32F_EQ(expected, result);
    }

    // Stress run
    for (int i = 0; i < NUM_RUNS; ++i) {
        float r = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float g = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float b = static_cast<float>(rnd.nextDouble() * 0xFFFE);

        pcg::Rgba32F result, expected;
        pcg::Rgbe encoded(r, g, b);
        result = encoded;
        rgbe2float(encoded, expected);

        ASSERT_FLOAT_EQ(expected.r(), result.r());
        ASSERT_FLOAT_EQ(expected.g(), result.g());
        ASSERT_FLOAT_EQ(expected.b(), result.b());
    }
}



TEST_F(RgbeTest, Performance2Rgbe)
{
    std::pair<pcg::Rgba32F*, ptrdiff_t> P = 
        std::get_temporary_buffer<pcg::Rgba32F> (NUM_TEST_PIXELS);
    ASSERT_TRUE(P.second > 0);
    ASSERT_TRUE(P.first != static_cast<pcg::Rgba32F*>(0));

    for (ptrdiff_t i = 0; i < P.second; ++i) {
        float r = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float g = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float b = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        P.first[i].set (r, g, b);
    }

    pcg::Rgbe rgbe;
    Timer timerRef, timerRtgi2, timerImp;
    hash_rgbe hashRef, hashRtgi2, hashImp;

    timerRef.start();
    for (ptrdiff_t i = 0; i < P.second; ++i) {
        const pcg::Rgba32F &rgba = P.first[i];
        float2Rgbe (rgba.r(), rgba.g(), rgba.b(), rgbe);
        hashRef.update (rgbe);
    }
    timerRef.stop();

    timerRtgi2.start();
    for (ptrdiff_t i = 0; i < P.second; ++i) {
        const pcg::Rgba32F &rgba = P.first[i];
        float2RgbeRtgi2 (rgba.r(), rgba.g(), rgba.b(), rgbe);
        hashRtgi2.update (rgbe);
    }
    timerRtgi2.stop();

    timerImp.start();
    for (ptrdiff_t i = 0; i < P.second; ++i) {
        const pcg::Rgba32F &rgba = P.first[i];
        rgbe.set (rgba);
        hashImp.update (rgbe);
    }
    timerImp.stop();
    ASSERT_EQ (hashRtgi2.hashCode(), hashImp.hashCode());

    std::cout <<"  Reference:   "<< timerRef.milliTime() <<" ms"<< std::endl;
    std::cout <<"  Rtgi2 imp:   "<<timerRtgi2.milliTime()<<" ms"<< std::endl;
    std::cout <<"  ImageIO:     "<< timerImp.milliTime() <<" ms"<< std::endl;
    printf("  ImageIO/Ref: %.2f%%\n", 
        100.0*timerRef.milliTime()/timerImp.milliTime());
    printf("  ImgIO/Rtgi2: %.2f%%\n", 
        100.0*timerRtgi2.milliTime()/timerImp.milliTime());

    std::return_temporary_buffer (P.first);
}



TEST_F(RgbeTest, Performance2Float)
{
    std::pair<pcg::Rgbe*, ptrdiff_t> P = 
        std::get_temporary_buffer<pcg::Rgbe> (NUM_TEST_PIXELS);
    ASSERT_TRUE(P.second > 0);
    ASSERT_TRUE(P.first != static_cast<pcg::Rgbe*>(0));

    for (ptrdiff_t i = 0; i < P.second; ++i) {
        float r = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float g = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        float b = static_cast<float>(rnd.nextDouble() * 0xFFFE);
        P.first[i].set (r, g, b);
    }

    pcg::Rgba32F rgba, sumRef, sumRtgi2, sumImp;
    Timer timerRef, timerRtgi2, timerImp;

    sumRef.zero();
    timerRef.start();
    for (ptrdiff_t i = 0; i < P.second; ++i) {
        const pcg::Rgbe &rgbe = P.first[i];
        rgbe2float (rgbe, rgba);
        sumRef += rgba;
    }
    timerRef.stop();

    sumRtgi2.zero();
    timerRtgi2.start();
    for (ptrdiff_t i = 0; i < P.second; ++i) {
        const pcg::Rgbe &rgbe = P.first[i];
        rgbe2floatRtgi2 (rgbe, rgba);
        sumRtgi2 += rgba;
    }
    timerRtgi2.stop();

    sumImp.zero();
    timerImp.start();
    for (ptrdiff_t i = 0; i < P.second; ++i) {
        rgba = P.first[i];
        sumImp += rgba;
    }
    timerImp.stop();

    ASSERT_FLOAT_EQ(sumRef.r(), sumImp.r());
    ASSERT_FLOAT_EQ(sumRef.g(), sumImp.g());
    ASSERT_FLOAT_EQ(sumRef.b(), sumImp.b());
    ASSERT_FLOAT_EQ(sumRef.a(), sumImp.a());

    ASSERT_FLOAT_EQ(sumRtgi2.r(), sumImp.r());
    ASSERT_FLOAT_EQ(sumRtgi2.g(), sumImp.g());
    ASSERT_FLOAT_EQ(sumRtgi2.b(), sumImp.b());
    ASSERT_FLOAT_EQ(sumRtgi2.a(), sumImp.a());

    std::cout <<"  Reference:   "<< timerRef.milliTime()  <<" ms"<< std::endl;
    std::cout <<"  Rtgi2 imp:   "<< timerRtgi2.milliTime()<<" ms"<< std::endl;
    std::cout <<"  ImageIO:     "<< timerImp.milliTime()  <<" ms"<< std::endl;
    printf("  ImageIO/Ref: %.2f%%\n", 
        100.0*timerRef.milliTime()/timerImp.milliTime());
    printf("  ImgIO/Rtgi2: %.2f%%\n", 
        100.0*timerRtgi2.milliTime()/timerImp.milliTime());

    std::return_temporary_buffer (P.first);
}
