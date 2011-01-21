#include <ImageComparator.h>

#include "dSFMT/RandomMT.h"
#include "Timer.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <vector>


using pcg::ImageComparator;

namespace
{

// Functor object implementing reference versions of the comparison methods
template <ImageComparator::Type T>
class Compare
{
private:
    static inline float norm2(float a, float b, float c) {
        return sqrt (a*a + b*b + c*c);    
    }

    void testFail(const char *msg) const {
        FAIL() << "Unexpected pcg::ImageComparator::Type";
    }

public:
    pcg::Rgba32F operator() (const pcg::Rgba32F &m, const pcg::Rgba32F &n) const
    {
        float r,g,b,a;
        r = g = b = a = std::numeric_limits<float>::quiet_NaN();

        switch (T) {
        case ImageComparator::AbsoluteDifference:
            r = fabs (m.r() - n.r());
            g = fabs (m.g() - n.g());
            b = fabs (m.b() - n.b());
            a = fabs (m.a() - n.a());
            break;

        case ImageComparator::Addition:
            r = m.r() + n.r();
            g = m.g() + n.g();
            b = m.b() + n.b();
            a = m.a() + n.a();
            break;

        case ImageComparator::Division:
            r = m.r() / n.r();
            g = m.g() / n.g();
            b = m.b() / n.b();
            a = m.a() / n.a();
            break;

        case ImageComparator::RelativeError:
            r = 2.0f * fabs (m.r() - n.r()) / (m.r() + n.r());
            g = 2.0f * fabs (m.g() - n.g()) / (m.g() + n.g());
            b = 2.0f * fabs (m.b() - n.b()) / (m.b() + n.b());
            a = 2.0f * fabs (m.a() - n.a()) / (m.a() + n.a());
            break;

        case ImageComparator::PositiveNegative:
            {
                float dr, dg, db, norm, pn;
                dr = m.r() - n.r();
                dg = m.g() - n.g();
                db = m.b() - n.b();
                norm = norm2 (dr, dg, db);
                pn = dr + dg + db;
                
                r = pn < 0.0f ? norm : 0.0f;
                g = pn > 0.0f ? norm : 0.0f;
                b = pn == 0.0f ? norm : 0.0f;
                a = norm;
            }
            break;

        case ImageComparator::PositiveNegativeRelativeError:
            {
                float dr, dg, db, norm, pn;
                dr = 2.0f * (m.r() - n.r()) / (m.r() + n.r());
                dg = 2.0f * (m.g() - n.g()) / (m.g() + n.g());
                db = 2.0f * (m.b() - n.b()) / (m.b() + n.b());
                norm = norm2 (dr, dg, db);
                pn = dr + dg + db;
                
                r = pn < 0.0f ? norm : 0.0f;
                g = pn > 0.0f ? norm : 0.0f;
                b = pn == 0.0f ? norm : 0.0f;
                a = norm;
            }
            break;

        default:
            testFail("Unexpected pcg::ImageComparator::Type");
        }

        pcg::Rgba32F result (r,g,b,a);
        return result;
    }
};


typedef pcg::Image<pcg::Rgba32F, pcg::TopDown>  Image;
typedef pcg::Image<pcg::Rgba32F, pcg::BottomUp> ImageBU;

// Helper struct to define parametrized tests
struct TestType
{
    pcg::ScanLineMode scanlineorder;
    pcg::ImageComparator::Type type;
    int width;
    int height;

    TestType() {}

    TestType (pcg::ScanLineMode s, pcg::ImageComparator::Type t, int w, int h) :
    scanlineorder(s), type(t), width(w), height(h) {}
};

} // namespace



class ImageComparatorTest : public ::testing::TestWithParam<TestType>
{
private:

    template <pcg::ImageComparator::Type T, pcg::ScanLineMode S>
    void refCompareTemplate(pcg::Image<pcg::Rgba32F, S> &dest,
        const pcg::Image<pcg::Rgba32F, S> &src1, 
        const pcg::Image<pcg::Rgba32F, S> &src2)
    {
        Compare<T> cmp;
        for (int i = 0; i < dest.Size(); ++i) {
            dest[i] = cmp(src1[i], src2[i]);
        }
    }

protected:
    virtual void SetUp() {
        // Python generated: [random.randint(0,0x7fffffff) for i in range(16)]
        const unsigned int seed[] = {741476574, 1946599781, 2039327122, 
            1219764310, 1029554009, 1696678380, 847124887, 1692745584, 
            510105363, 905870339, 900295777, 429890015, 1339278515, 438248757, 
            864835168, 427334422};
        rnd.setSeed (seed);
    }

    virtual void TearDown() {

    }

    template <pcg::ScanLineMode S>
    void fillRnd (pcg::Image<pcg::Rgba32F, S> &img) {
        for (int i = 0; i < img.Size(); ++i) {
            float r = 64.0f * rnd.nextFloat();
            float g = 64.0f * rnd.nextFloat();
            float b = 64.0f * rnd.nextFloat();
            float a = rnd.nextFloat();
            if (rnd.nextDouble() < 0.03125) a *= 64.0f;

            // Have negative values here and there
            if (rnd.nextDouble() < 0.03125) r *= 1.0f;
            if (rnd.nextDouble() < 0.03125) g *= 1.0f;
            if (rnd.nextDouble() < 0.03125) b *= 1.0f;
            if (rnd.nextDouble() < 0.03125) a *= 1.0f;
            img[i].set (r,g,b,a);
        }
    }

    // The actual method to use
    template <pcg::ScanLineMode S>
    void referenceCompare(ImageComparator::Type type, 
        pcg::Image<pcg::Rgba32F, S> &dest, 
        const pcg::Image<pcg::Rgba32F, S> &src1, 
        const pcg::Image<pcg::Rgba32F, S> &src2)
    {
        switch (type) {
        case ImageComparator::AbsoluteDifference:
            refCompareTemplate<ImageComparator::AbsoluteDifference> 
                (dest, src1, src2);
            break;

        case ImageComparator::Addition:
            refCompareTemplate<ImageComparator::Addition> 
                (dest, src1, src2);
            break;

        case ImageComparator::Division:
            refCompareTemplate<ImageComparator::Division> 
                (dest, src1, src2);
            break;

        case ImageComparator::RelativeError:
            refCompareTemplate<ImageComparator::RelativeError> 
                (dest, src1, src2);
            break;

        case ImageComparator::PositiveNegative:
            refCompareTemplate<ImageComparator::PositiveNegative> 
                (dest, src1, src2);
            break;

        case ImageComparator::PositiveNegativeRelativeError:
            refCompareTemplate<ImageComparator::PositiveNegativeRelativeError> 
                (dest, src1, src2);
            break;

        default:
            FAIL() << "Unexpected pcg::ImageComparator::Type";
        }
    }



    // Helper function which will actually do the tests
    template <pcg::ScanLineMode S>
    void compareTest(ImageComparator::Type type, int width, int height)
    {
        pcg::Image <pcg::Rgba32F, S> result (width, height);
        pcg::Image <pcg::Rgba32F, S> reference (width, height);
        pcg::Image <pcg::Rgba32F, S> src1 (width, height);
        pcg::Image <pcg::Rgba32F, S> src2 (width, height);

        // Paranoid test
        const int numPixels = width * height;
        ASSERT_EQ (numPixels, result.Size());
        ASSERT_EQ (numPixels, reference.Size());
        ASSERT_EQ (numPixels, src1.Size());
        ASSERT_EQ (numPixels, src2.Size());

        // Fill the sources with random pixels
        fillRnd (src1);
        fillRnd (src2);

        // Run the implemented compare
        ASSERT_NO_THROW (ImageComparator::Compare (type, result, src1, src2));

        // Calculate the reference
        referenceCompare (type, reference, src1, src2);

        // Compare all the pixels
        for (int i = 0; i < numPixels; ++i) {
            ASSERT_EQ (reference[i], result[i]);
        }
    }



    const char* str (const pcg::ScanLineMode order) {
        switch (order) {
        case pcg::TopDown:  return "TopDown";
        case pcg::BottomUp: return "BottomUp";
        default: return "unknown";
        }
    }



    const char* str (const ImageComparator::Type type) {
        switch (type) {
        case ImageComparator::AbsoluteDifference:
            return "AbsoluteDifference";
        case ImageComparator::Addition:
            return "Addition";
        case ImageComparator::Division:
            return "Division";
        case ImageComparator::RelativeError:
            return "RelativeError";
        case ImageComparator::PositiveNegative:
            return "PositiveNegative";
        case ImageComparator::PositiveNegativeRelativeError:
            return "PositiveNegativeRelativeError";

        default: return "unknown";
        }
    }



    static const int NUM_RUNS = 2000000;
    static const ptrdiff_t NUM_TEST_PIXELS = 2000000;

    RandomMT rnd;
};


TEST_F(ImageComparatorTest, InvalidSizes)
{
    Image img1x1(1,1);
    Image img512x1(512,1);
    Image img1x512(1,512);
    Image img512(512,512);
    Image img640x480(640,480);
    Image img480x640(480,640);

    Image* imgs[] = 
        { &img1x1, &img512x1, &img1x512, &img512, &img640x480, &img480x640 };
    const int len = static_cast<int> (sizeof(imgs)/sizeof(Image*));

    const pcg::ImageComparator::Type type = pcg::ImageComparator::Addition;

    // Select all permutations
    for (int i = 0; i < len; ++i) {
        for (int j = 0; j < len; ++j) {
            if (j == i) continue;
            for (int k = 0; k < len; ++k) {
                if ((k == i) || (k == j)) continue;

                Image &dest = *imgs[i];
                Image &src1 = *imgs[j];
                Image &src2 = *imgs[k];

                if (dest.Width()  != src1.Width() || 
                    dest.Height() != src1.Height() ||
                    src1.Width() != src2.Width() || 
                    src1.Height() != src2.Height() )
                {
                    ASSERT_THROW (pcg::ImageComparator::Compare(type,
                        dest, src1, src2), pcg::IllegalArgumentException);
                } else {
                    ASSERT_NO_THROW (pcg::ImageComparator::Compare(type,
                        dest, src1, src2));
                }
            }
        }
    }    
}



TEST_P(ImageComparatorTest, Compare)
{
    const TestType &params = GetParam();

    printf("  Test params: %8s, %dx%d\n", str(params.scanlineorder),
        params.width, params.height);

    switch (params.scanlineorder) {
    case pcg::TopDown:
        compareTest<pcg::TopDown> (params.type, 
            params.width, params.height);
        break;

    case pcg::BottomUp:
        compareTest<pcg::BottomUp> (params.type, 
            params.width, params.height);
        break;

    default:
        FAIL() << "Unknown pcg::ScanLineMode";
    }
}



// Workaround to the lack of custom parameter generators as of gtest 1.5
namespace
{
template <typename T, size_t N>
size_t len (const T (&arr)[N]) {
    return N;
}

template <typename T, size_t N>
const T* const_begin (const T (&arr)[N]) {
    return &arr[0];
}

template <typename T, size_t N>
const T* const_end (const T (&arr)[N]) {
    return &arr[0] + len(arr);
}

struct Generator
{
    std::vector<TestType> values;

    Generator(const ImageComparator::Type type)
    {
        pcg::ScanLineMode modes[] = { pcg::TopDown,
            pcg::BottomUp };
        int sizes[] = {1, 480, 640, 512};

        for (const pcg::ScanLineMode *mode = const_begin(modes); 
            mode != const_end(modes); ++mode) {
            for (const int *width = const_begin(sizes); 
            width != const_end(sizes); ++width) {
                for (const int *height = const_begin(sizes);
                height != const_end(sizes); ++height) 
                {
                    if (*width != *height || *width == 1 || *width == 512) {
                        const int numPixels = (*height) * (*width);
                        if (numPixels == (480*512) || numPixels == (640*512)) 
                            continue;
                        TestType params(*mode, type, *width, *height);
                        values.push_back (params);
                    }
                }
            }
        }
    }
};

// Global instances
Generator paramsAbsoluteDifference (ImageComparator::AbsoluteDifference);
Generator paramsAddition (ImageComparator::Addition);
Generator paramsDivision (ImageComparator::Division);
Generator paramsRelativeError (ImageComparator::RelativeError);
Generator paramsPositiveNegative (ImageComparator::PositiveNegative);
Generator paramsPositiveNegativeRelativeError (
    ImageComparator::PositiveNegativeRelativeError);

} // namespace

// Instanciate the tests
INSTANTIATE_TEST_CASE_P(AbsoluteDifference, ImageComparatorTest,
    ::testing::ValuesIn(paramsAbsoluteDifference.values));

INSTANTIATE_TEST_CASE_P(Addition, ImageComparatorTest,
    ::testing::ValuesIn(paramsAddition.values));

INSTANTIATE_TEST_CASE_P(Division, ImageComparatorTest,
    ::testing::ValuesIn(paramsDivision.values));

INSTANTIATE_TEST_CASE_P(RelativeError, ImageComparatorTest,
    ::testing::ValuesIn(paramsRelativeError.values));

INSTANTIATE_TEST_CASE_P(PositiveNegative, ImageComparatorTest,
    ::testing::ValuesIn(paramsPositiveNegative.values));

INSTANTIATE_TEST_CASE_P(PositiveNegativeRelativeError, ImageComparatorTest,
    ::testing::ValuesIn(paramsPositiveNegativeRelativeError.values));
