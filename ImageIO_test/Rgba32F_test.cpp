#include <Rgba32F.h>

#include "dSFMT/RandomMT.h"

#include <gtest/gtest.h>

#include <iostream>

class Rgba32FTest : public ::testing::Test
{
protected:
    virtual void SetUp() {
        const unsigned int seed[] = {1464395057, 1537798260, 525912928, 
            61198902, 168516396, 1263369332, 236029447, 592821895, 1908544494, 
            1648229894, 754843437, 1938494521, 481618578, 1559278412,
            1803740399, 1416930839};
        rnd.setSeed (seed);
    }

    virtual void TearDown() {

    }

    // Helper function to initialize with random values
    inline void setRnd(pcg::Rgba32F &v, const float rgbScale = 1.0f) {
        float r = rgbScale * rnd.nextFloat();
        float g = rgbScale * rnd.nextFloat();
        float b = rgbScale * rnd.nextFloat();
        float a = rnd.nextFloat();
        v.set (r,g,b,a);
    }

    static const int NUM_RUNS = 2000000;

    RandomMT rnd;
};



namespace
{

// Helper struct to view data as Rgba32F and uint32[4]
struct PixHelper
{
    union {
        __m128 pix;
        unsigned int m[4];
        float f[4];
    };

    PixHelper() {}

    PixHelper(const __m128 &val) {
        pix = val;
    }
};

} // namespace



TEST_F(Rgba32FTest, Constructors)
{
    pcg::Rgba32F v1(7.5f);
    ASSERT_EQ (7.5f, v1.r());
    ASSERT_EQ (7.5f, v1.g());
    ASSERT_EQ (7.5f, v1.b());
    ASSERT_EQ (7.5f, v1.a());

    pcg::Rgba32F v2(2.0f, 3.0f, 4.0f);
    ASSERT_EQ (2.0f, v2.r());
    ASSERT_EQ (3.0f, v2.g());
    ASSERT_EQ (4.0f, v2.b());
    ASSERT_EQ (1.0f, v2.a());

    pcg::Rgba32F v3(2.0f, 3.0f, 4.0f, 0.5f);
    ASSERT_EQ (2.0f, v3.r());
    ASSERT_EQ (3.0f, v3.g());
    ASSERT_EQ (4.0f, v3.b());
    ASSERT_EQ (0.5f, v3.a());

    pcg::Rgba32F v4(v3);
    ASSERT_EQ (2.0f, v4.r());
    ASSERT_EQ (3.0f, v4.g());
    ASSERT_EQ (4.0f, v4.b());
    ASSERT_EQ (0.5f, v4.a());
}



TEST_F(Rgba32FTest, Setters)
{
    pcg::Rgba32F v;

    v.set(4.0f, 3.0f, 2.0f);
    ASSERT_EQ(4.0f, v.r());
    ASSERT_EQ(3.0f, v.g());
    ASSERT_EQ(2.0f, v.b());
    ASSERT_EQ(1.0f, v.a());

    v.set(4.5f, 3.5f, 2.5f, 0.5f);
    ASSERT_EQ(4.5f, v.r());
    ASSERT_EQ(3.5f, v.g());
    ASSERT_EQ(2.5f, v.b());
    ASSERT_EQ(0.5f, v.a());

    v.setR(4.0f);
    ASSERT_EQ(4.0f, v.r());
    ASSERT_EQ(3.5f, v.g());
    ASSERT_EQ(2.5f, v.b());
    ASSERT_EQ(0.5f, v.a());

    v.setG(3.0f);
    ASSERT_EQ(4.0f, v.r());
    ASSERT_EQ(3.0f, v.g());
    ASSERT_EQ(2.5f, v.b());
    ASSERT_EQ(0.5f, v.a());

    v.setB(2.0f);
    ASSERT_EQ(4.0f, v.r());
    ASSERT_EQ(3.0f, v.g());
    ASSERT_EQ(2.0f, v.b());
    ASSERT_EQ(0.5f, v.a());

    v.setA(1.0f);
    ASSERT_EQ(4.0f, v.r());
    ASSERT_EQ(3.0f, v.g());
    ASSERT_EQ(2.0f, v.b());
    ASSERT_EQ(1.0f, v.a());

    v.setAll(0.125f);
    ASSERT_EQ(0.125f, v.r());
    ASSERT_EQ(0.125f, v.g());
    ASSERT_EQ(0.125f, v.b());
    ASSERT_EQ(0.125f, v.a());

    v.zero();
    ASSERT_EQ(0.0f, v.r());
    ASSERT_EQ(0.0f, v.g());
    ASSERT_EQ(0.0f, v.b());
    ASSERT_EQ(0.0f, v.a());
}


#if defined(__clang__)
#warning "Rgba32FTest::ApplyAlpha test disabled because it causes clang 2.8 to crash"
#else
TEST_F(Rgba32FTest, ApplyAlpha)
{
    pcg::Rgba32F v;
    
    v.set(4.0f, 3.0f, 2.0f);
    v.applyAlpha();
    ASSERT_EQ(4.0f, v.r());
    ASSERT_EQ(3.0f, v.g());
    ASSERT_EQ(2.0f, v.b());
    ASSERT_EQ(1.0f, v.a());

    v.setA(0.5f);
    v.applyAlpha();
    ASSERT_EQ(2.0f, v.r());
    ASSERT_EQ(1.5f, v.g());
    ASSERT_EQ(1.0f, v.b());
    ASSERT_EQ(1.0f, v.a());

    v.setA(0.0f);
    v.applyAlpha();
    ASSERT_EQ(0.0f, v.r());
    ASSERT_EQ(0.0f, v.g());
    ASSERT_EQ(0.0f, v.b());
    ASSERT_EQ(0.0f, v.a());

    // Lets stress it for a while
    for (int i = 0; i < NUM_RUNS; ++i) {
        float rgbScale = static_cast<float> (rnd.nextInt (16) + 1);
        assert (rgbScale >= 1.0f && rgbScale <= 16.0f);
        setRnd(v, rgbScale);
        
        float r = v.r(), g = v.g(), b = v.b(), a = v.a();
        if (a != 0.0f) {
            r *= a;
            g *= a;
            b *= a;
            a = 1.0f;
        } else {
            r = g = b = a = 0.0f;
        }

        v.applyAlpha();
        ASSERT_EQ(r, v.r());
        ASSERT_EQ(g, v.g());
        ASSERT_EQ(b, v.b());
        ASSERT_EQ(a, v.a());
    }
}
#endif // defined(__clang__)



TEST_F(Rgba32FTest, FloatCast)
{
    pcg::Rgba32F v;
    for (int i = 0; i < NUM_RUNS; ++i) {
        float rgbScale = static_cast<float> (rnd.nextInt (16) + 1);
        assert (rgbScale >= 1.0f && rgbScale <= 16.0f);
        setRnd(v, rgbScale);
        
        float *ptr = v;
        ASSERT_EQ(v.r(), ptr[3]);
        ASSERT_EQ(v.g(), ptr[2]);
        ASSERT_EQ(v.b(), ptr[1]);
        ASSERT_EQ(v.a(), ptr[0]);
    }
}



TEST_F(Rgba32FTest, ComparisonOperators)
{
    pcg::Rgba32F a, b;
    setRnd(a);
    b.set(a.r(), a.g(), a.b(), a.a());

    ASSERT_TRUE  (a == a);
    ASSERT_FALSE (a != a);
    ASSERT_TRUE  (b == b);
    ASSERT_FALSE (b != b);
    ASSERT_TRUE  (a == b);
    ASSERT_FALSE (a != b);
    ASSERT_TRUE  (b == a);
    ASSERT_FALSE (b != a);
    ASSERT_EQ (a, b);
    ASSERT_EQ (b, a);
    ASSERT_EQ (a, a);
    ASSERT_EQ (b, b);

    setRnd(b);
    ASSERT_TRUE  (a == a);
    ASSERT_FALSE (a != a);
    ASSERT_TRUE  (b == b);
    ASSERT_FALSE (b != b);
    ASSERT_TRUE  (a != b);
    ASSERT_FALSE (a == b);
    ASSERT_TRUE  (b != a);
    ASSERT_FALSE (b == a);
    ASSERT_NE (a, b);
    ASSERT_NE (b, a);
    ASSERT_EQ (a, a);
    ASSERT_EQ (b, b);


    // Stress run
    for (int i = 0; i < NUM_RUNS; ++i) {
        setRnd(a);
        b.set(a.r(), a.g(), a.b(), a.a());
        ASSERT_TRUE (a == b);
        ASSERT_EQ (a, b);

        // Modify a value
        size_t idx;
        float oldVal;
        
        for (int j = 0; j < 8; ++j) {
            idx = static_cast<size_t>(rnd.nextInt(4));
            oldVal = static_cast<float*>(b)[idx];
            do {
                static_cast<float*>(b)[idx] = rnd.nextFloat();
            } while (oldVal == static_cast<float*>(b)[idx]);
            ASSERT_TRUE (a != b);
            ASSERT_TRUE (b != a);
            ASSERT_NE (a, b);
            ASSERT_NE (b, a);
        }
    }
}



// Helper macro to define the operators test
#define PCG_ARITHMETIC_OPERATOR_TEST(op, name)                \
    TEST_F(Rgba32FTest, Operator ## name)                     \
    {                                                         \
        pcg::Rgba32F m, n;                                    \
        for (int i = 0; i < NUM_RUNS; ++i) {                  \
            setRnd(m, static_cast<float>(rnd.nextInt(16)+1)); \
            setRnd(n, static_cast<float>(rnd.nextInt(16)+1)); \
            pcg::Rgba32F res = m op n;                        \
            ASSERT_EQ(m.r() op n.r(), res.r());               \
            ASSERT_EQ(m.g() op n.g(), res.g());               \
            ASSERT_EQ(m.b() op n.b(), res.b());               \
            ASSERT_EQ(m.a() op n.a(), res.a());               \
                                                              \
            m op ## = n;                                      \
            ASSERT_EQ(res, m);                                \
                                                              \
            const float s = rnd.nextFloat() * 8.0f;           \
            setRnd(m, static_cast<float>(rnd.nextInt(8)+1));  \
            res = m op s;                                     \
            ASSERT_EQ(m.r() op s, res.r());                   \
            ASSERT_EQ(m.g() op s, res.g());                   \
            ASSERT_EQ(m.b() op s, res.b());                   \
            ASSERT_EQ(m.a() op s, res.a());                   \
            res = s op m;                                     \
            ASSERT_EQ(m.r() op s, res.r());                   \
            ASSERT_EQ(m.g() op s, res.g());                   \
            ASSERT_EQ(m.b() op s, res.b());                   \
            ASSERT_EQ(m.a() op s, res.a());                   \
                                                              \
            m op ## = s;                                      \
            ASSERT_EQ(res, m);                                \
        }                                                     \
    }  

#define PCG_LOGIC_OPERATOR_TEST(op, name)                     \
    TEST_F(Rgba32FTest, Operator ## name)                     \
    {                                                         \
        pcg::Rgba32F m, n;                                    \
        for (int i = 0; i < NUM_RUNS; ++i) {                  \
            setRnd(m, static_cast<float>(rnd.nextInt(16)+1)); \
            setRnd(n, static_cast<float>(rnd.nextInt(16)+1)); \
            pcg::Rgba32F res = m op n;                        \
            PixHelper x(m), y(n), res_h(res), tmp;            \
            for (int j = 0; j < 4; ++j) {                     \
                tmp.m[j] = x.m[j] op y.m[j];                  \
                ASSERT_EQ(tmp.m[j], res_h.m[j]);              \
            }                                                 \
            m op ## = n;                                      \
            tmp.pix = m;                                      \
            for (int j = 0; j < 4; ++j)                       \
                ASSERT_EQ(tmp.m[j], res_h.m[j]);              \
        }                                                     \
    } 

PCG_ARITHMETIC_OPERATOR_TEST(+, Plus)
PCG_ARITHMETIC_OPERATOR_TEST(-, Minus)
PCG_ARITHMETIC_OPERATOR_TEST(*, Mult)
PCG_ARITHMETIC_OPERATOR_TEST(/, Div)

PCG_LOGIC_OPERATOR_TEST(&, And)
PCG_LOGIC_OPERATOR_TEST(|, Or)
PCG_LOGIC_OPERATOR_TEST(^, Xor)

#undef PCG_ARITHMETIC_OPERATOR_TEST
#undef PCG_LOGIC_OPERATOR_TEST



TEST_F(Rgba32FTest, Abs)
{
    pcg::Rgba32F res, v(-1.0f, 2.0f, -3.0f, -0.5f);
    res = pcg::Rgba32F::abs(v);
    ASSERT_EQ(1.0f, res.r());
    ASSERT_EQ(2.0f, res.g());
    ASSERT_EQ(3.0f, res.b());
    ASSERT_EQ(0.5f, res.a());

    for (int i = 0; i < NUM_RUNS; ++i) {
        setRnd(v, 8.0f);
        PixHelper pix(v);
        float *ptr = v;
        for (int k = 0; k < 4; ++k) {
            ptr[k] *= (rnd.nextDouble() < 0.25 ? -1.0f : 1.0f);
        }

        res = pcg::Rgba32F::abs(v);
        ASSERT_EQ(pix.f[3], res.r());
        ASSERT_EQ(pix.f[2], res.g());
        ASSERT_EQ(pix.f[1], res.b());
        ASSERT_EQ(pix.f[0], res.a());
    }
}



TEST_F(Rgba32FTest, new_delete)
{
    const pcg::Rgba32F zero(0.0f);
    const pcg::Rgba32F ones(1.0f);

    // Allocate 1 element
    {
        pcg::Rgba32F *ptr = new pcg::Rgba32F;
        ASSERT_EQ (0, (size_t)ptr & 0xF);
        ptr->setAll(1.0f);
        *ptr += zero;
        ASSERT_EQ (ones, *ptr);
        delete ptr;
    }

    // Allocate arrays
    {
        pcg::Rgba32F *ptr = new pcg::Rgba32F[1];
        ASSERT_EQ (0, (size_t)ptr & 0xF);
        for (size_t i = 0; i < 1; ++i) {
            ptr[i].setAll(1.0f);
            ptr[i] += zero;
            ASSERT_EQ (ones, ptr[i]);
        }
        delete [] ptr;
    }

    {
        pcg::Rgba32F *ptr = new pcg::Rgba32F[2];
        ASSERT_EQ (0, (size_t)ptr & 0xF);
        for (size_t i = 0; i < 2; ++i) {
            ptr[i].setAll(1.0f);
            ptr[i] += zero;
            ASSERT_EQ (ones, ptr[i]);
        }
        delete [] ptr;
    }

    {
        const size_t count = 4096*3072;
        pcg::Rgba32F *ptr = new pcg::Rgba32F[count];
        ASSERT_EQ (0, (size_t)ptr & 0xF);
        for (size_t i = 0; i < count; ++i) {
            ptr[i].setAll(1.0f);
            ptr[i] += zero;
            ASSERT_EQ (ones, ptr[i]);
        }
        delete [] ptr;
    }
}
