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

#if defined(_MSC_VER)
#define _SCL_SECURE_NO_WARNINGS
#endif

#include "Timer.h"

#include <Rgba32F.h>
#include <rgbe.h>
#include <Image.h>
#include <RgbeImage.h>
#include <RgbeIO.h>
#include <ToneMapper.h>
#include <PngIO.h>
#include <PfmIO.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <cmath>

// For TBB initialization
//#include <tbb/task_scheduler_init.h>

using namespace std;
using namespace pcg;



// Test loading/conversion from RGBE files
TEST(LegacyTests, DISABLED_RGBEImage)
{
    // Just to check how Image behaves
    Image<Rgbe> testImg(1,1);
    Image<Rgba32F, TopDown> testImg2;
    //RgbeImage::foo(testImg2, std::cin);
    cout << "Size of the Images: " << testImg.Size() << " " << testImg2.Size() << endl;
    cout << "First pixel: "       << testImg[0] << endl;
    cout << "Pointer to the first scanline: " << testImg.GetScanlinePointer(0) << endl;
    cout << "Scanline order: " << testImg.GetMode() << endl;

    Timer timer;

    // Loading a real RGBE image
    timer.reset();
    RgbeImage rgbeImageTest("iris.rgbe");
    timer.stop();
    cout << "Rgbe Image: " << rgbeImageTest.Width() << " x " << rgbeImageTest.Height() << endl;
    cout << "Time to load pure RGBE image: " << timer.milliTime() << " ms" << endl;
    cout << "Last pixel: " << rgbeImageTest[rgbeImageTest.Size()-1] << endl;

    // Converting the RGBE image into an Rgba32F at once
    {
        Image<Rgba32F> floatImage(rgbeImageTest.Width(), rgbeImageTest.Height());
        const int numPixels = rgbeImageTest.Size();
        timer.reset();
        
        for (int i = 0; i < numPixels; ++i) {
            floatImage[i] = (Rgba32F)rgbeImageTest[i];
        }

        timer.stop();
        cout << "Time to convert loaded RGBE to Rgba32F: " << timer.milliTime() << " ms" << endl;
        cout << "Last pixel: " << floatImage[floatImage.Size()-1] << endl;
    }

    // Loading a RGBE image convering on the fly to Rgba32F
    timer.reset();
    RgbeIO::Load(testImg2, "iris.rgbe");
    timer.stop();
    cout << "Rgba32F Image from Rgbe: " << testImg2.Width() << " x " << testImg2.Height() << endl;
    cout << "Time to load image converting on the fly: " << timer.milliTime() << " ms" << endl;
    cout << "Last pixel: " << testImg2[testImg2.Size()-1] << endl;
}



TEST(LegacyTests, DISABLED_ToneMapper)
{
    Timer timer;
    ToneMapper toneMapper(0xFFFF);

    timer.reset();
    toneMapper.SetGamma(1.8f);
    toneMapper.SetGamma(2.0f);
    toneMapper.SetGamma(2.2f);
    toneMapper.SetGamma(2.4f);
    toneMapper.SetGamma(2.6f);
    timer.stop();

    cout << "Time to setup the ToneMapper lut: " << timer.milliTime()/5 << " ms" << endl;

    // Loads the image converting on the fly
    Image<Rgba32F, TopDown> floatImage;
    RgbeIO::Load(floatImage, "horse.rgbe");

    // Now let's tone map the Rgba32F image
    Image<Bgra8> ldrImage(floatImage.Width(), floatImage.Height());
    toneMapper.SetGamma(2.0f);
    timer.reset();
    toneMapper.ToneMap(ldrImage, floatImage);
    timer.stop();
    cout << "Time to tone map the image: " << timer.milliTime() << " ms" << endl;

    // Create a png image from the Bgra8 image
    timer.reset();
    PngIO::Save(ldrImage, "test-horse-bgra8-g2.png", false, 1/2.0f);
    timer.stop();
    cout << "Time to save as PNG 8bpp: " << timer.milliTime() << " ms" << endl;

    // Tonemap again and save, but as sRGB
    toneMapper.SetSRGB(true);
    toneMapper.ToneMap(ldrImage, floatImage);
    PngIO::Save(ldrImage, "test-horse-bgra8-srgb.png", true);


    // Lets change the pixel order
    Image<Rgba8> ldrImage2(floatImage.Width(), floatImage.Height());
    toneMapper.ToneMap(ldrImage2, floatImage);
    PngIO::Save(ldrImage2, "test-horse-rgba8-srgb.png", true);

    // Tonemap without the LUT
    timer.reset();
    toneMapper.ToneMap(ldrImage2, floatImage, false);
    timer.stop();
    cout << "Time to tone map the image (no LUT): " << timer.milliTime() << " ms" << endl;
    PngIO::Save(ldrImage2, "test-horse-rgba8-srgb-noLUT.png", true);

    // Lets try with the high resolution version
    Image<Rgba16> ldrImage16(floatImage.Width(), floatImage.Height());
    timer.reset();
    toneMapper.ToneMap(ldrImage16, floatImage);
    Rgba16 &px = ldrImage16.ElementAt(200,300);
    cout << "High bpp pixel (200,300): " << px.r << ',' << px.g << ',' << px.b << endl;
    timer.stop();
    cout << "Time to tone map the 16bpp image (no LUT): " << timer.milliTime() << " ms" << endl;

    timer.reset();
    PngIO::Save(ldrImage16, "test-horse-rgb16-srgb-noLUT.png", true);
    timer.stop();
    cout << "Time to save as PNG 16bpp: " << timer.milliTime() << " ms" << endl;

#if 0
    // Create a QImage from the Argb8 Image and save as PNG
    timer.reset();
    
    QImage qImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), ldrImage.Width(), ldrImage.Height(),
        QImage::Format_RGB32);
    timer.stop();
    cout << "Time to wrap in a QImage: " << timer.milliTime() << " ms" << endl;
    timer.reset();
    qImage.save("test-horse.png");
    timer.stop();
    cout << "Time to save as PNG from QImage: " << timer.milliTime() << " ms" << endl;
    cout << "Sanity: sizeof(long) " << sizeof(long) << endl;
#endif

}



TEST(LegacyTests, DISABLED_PfmImage)
{
    Image<Rgba32F> pfmImg(640, 480);
    const Rgba32F zero(0.0f);
    std::fill_n(pfmImg.GetDataPointer(), pfmImg.Size(), zero);
    srand(0);
    for(int i = 0; i < pfmImg.Size(); ++i) {
        Rgba32F &px = pfmImg.GetDataPointer()[i];
        const float f = 1.0f / (float)RAND_MAX;
        px.set(rand()*f, rand()*f, rand()*f);
    }

    PfmIO::Save(pfmImg, "test.pfm");

    // Now read back the image
    Image<Rgba32F> img;
    PfmIO::Load(img, "test.pfm");

    assert(img.Width()  == pfmImg.Width());
    assert(img.Height() == pfmImg.Height());
    for(int i = 0; i < img.Size(); ++i) {
        assert(img.GetDataPointer()[i] == pfmImg.GetDataPointer()[i]);
    }

    cout << "Pfm Image test OK" << endl;
}



int main(int argc, char** argv) 
{
    ::testing::InitGoogleTest (&argc, argv);
    return RUN_ALL_TESTS();
}
