#include <iostream>

//#include <QImage.h>

#include <Rgba32F.h>
#include <rgbe.h>
#include <Image.h>
#include <RgbeImage.h>
#include <RgbeIO.h>
#include <ToneMapper.h>
#include <PngIO.h>
#include <PfmIO.h>

#include <cmath>

#include "Timer.h"

// For TBB initialization
//#include <tbb/task_scheduler_init.h>

using namespace std;
using namespace pcg;


// Convert to RGBE using the standard code
inline void float2Rgbe(const float red, const float green, const float blue, Rgbe &outRgbe)
{
	float v;
	int e;

	v = red;
	if (green > v) v = green;
	if (green > v) v = blue;
	if (v < 1e-32) {
		outRgbe.set(0,0,0,0);
	}
	else {
		v = (float)(frexp(v,&e) * 256.0/v);
		outRgbe.r = (unsigned char) (red * v);
		outRgbe.g = (unsigned char) (green * v);
		outRgbe.b = (unsigned char) (blue * v);
		outRgbe.e = (unsigned char) (e + 128);
	}
}

inline void float2Rgbe(const Rgba32F &pixel, Rgbe &outRgbe)
{
	float v;
	int e;

	v = pixel.r();
	if (pixel.g() > v) v = pixel.g();
	if (pixel.b() > v) v = pixel.b();
	if (v < 1e-32) {
		outRgbe.set(0,0,0,0);
	}
	else {
		v = (float)(frexp(v,&e) * 256.0/v);
		outRgbe.r = (unsigned char) (pixel.r() * v);
		outRgbe.g = (unsigned char) (pixel.g() * v);
		outRgbe.b = (unsigned char) (pixel.b() * v);
		outRgbe.e = (unsigned char) (e + 128);
	}
}


void testConversion() {

	// Generate an artificial 1k-by-1k image
	const int SIZE = 1024*1024*16;
	Rgba32F *image  = new Rgba32F[SIZE];
	Rgba32F *image2 = new Rgba32F[SIZE];
	Rgbe    *iRgbe1 = new Rgbe[SIZE];
	Rgbe    *iRgbe2 = new Rgbe[SIZE];
	Timer timer;

	srand(0);

	cout << "Filling random image..." << endl;

	// Fill it with random pixels
	for(int i = 0; i < SIZE; ++i) {
		if (rand() % 10 < 1) {
			image[i].setAll(0.0f);
		}
		else {
			image[i].set((float)rand()/RAND_MAX*1000.0f, (float)rand()/RAND_MAX*1000.0f, (float)rand()/RAND_MAX*1000.0f);
		}
	}

	cout << "Converting from float to RGBE..." << endl;

	// Convert to RGBE the old fashioned way
	timer.reset();
	timer.start();
	for(int i = 0; i < SIZE; ++i) {
		float2Rgbe(image[i], iRgbe1[i]);
	}
	timer.stop();
	cout << "Normal conversion: " << timer.elapsedSeconds() << endl;

	// The SSE way
	timer.reset();
	timer.start();
	for(int i = 0; i < SIZE; ++i) {
		iRgbe2[i].set(image[i]);
	}
	timer.stop();
	cout << "SSE conversion:    " << timer.elapsedSeconds() << endl;

	int diffPx = 0;
	for(int i = 0; i < SIZE; ++i) {
		if ( abs(iRgbe1[i].r - iRgbe2[i].r)>1 || abs(iRgbe1[i].g - iRgbe2[i].g)>1 ||
			 abs(iRgbe1[i].b - iRgbe2[i].b)>1 || abs(iRgbe1[i].e - iRgbe2[i].e)>1 ) {
				++diffPx;
				//cout << "Different!! " << iRgbe1[i] << " vs " << iRgbe2[i] << endl;
		}
	}
	cout << "Different pixels:  " << diffPx << endl;
	delete [] iRgbe2;


	cout << "Converting from RGBE to float..." << endl;

	// Convert to float the old fashioned way
	timer.reset();
	timer.start();
	for(int i = 0; i < SIZE; ++i) {
		if (iRgbe1[i].e) {   /*nonzero pixel*/
				const float f = (float)(ldexp(1.0,iRgbe1[i].e-(int)(128+8)));
				image[i].set(iRgbe1[i].r * f, iRgbe1[i].g * f, iRgbe1[i].b * f);
			}
			else
				image[i].setAll(0.0f);
	}
	timer.stop();
	cout << "Normal conversion: " << timer.elapsedSeconds() << endl;

	// Convert to float the SSE way
	timer.reset();
	timer.start();
	for(int i = 0; i < SIZE; ++i) {
		image2[i] = (Rgba32F)iRgbe1[i];
	}
	timer.stop();
	cout << "SSE conversion:    " << timer.elapsedSeconds() << endl;

	diffPx = 0;
	const float epsilon = 0.001f;
	for(int i = 0; i < SIZE; ++i) {
		if ( abs(image[i].r() - image2[i].r())> epsilon || abs(image[i].g() - image2[i].g())> epsilon ||
			 abs(image[i].b() - image2[i].b())> epsilon || abs(image[i].a() - image2[i].a())> epsilon ) {
				++diffPx;
				//cout << "Different!! " << iRgbe1[i] << " vs " << iRgbe2[i] << endl;
		}
	}
	cout << "Different pixels:  " << diffPx << endl;



	delete [] image;
	delete [] image2;
	delete [] iRgbe1;
	

}

// Test loading/conversion from RGBE files
void testRGBEImage() {

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
	cout << "Time to load pure RGBE image: " << timer.elapsedSeconds() << endl;
	cout << "Last pixel: " << rgbeImageTest[rgbeImageTest.Size()-1] << endl;

	// Converting the RGBE image into an Rgba32F at once
	{
		Image<Rgba32F> floatImage(rgbeImageTest.Width(), rgbeImageTest.Height());
		const int numPixels = rgbeImageTest.Size();
		timer.reset();
		
		#pragma omp parallel for
		for (int i = 0; i < numPixels; ++i) {
			floatImage[i] = (Rgba32F)rgbeImageTest[i];
		}

		timer.stop();
		cout << "Time to convert loaded RGBE to Rgba32F: " << timer.elapsedSeconds() << endl;
		cout << "Last pixel: " << floatImage[floatImage.Size()-1] << endl;
	}

	// Loading a RGBE image convering on the fly to Rgba32F
	timer.reset();
	RgbeIO::Load(testImg2, "iris.rgbe");
	timer.stop();
	cout << "Rgba32F Image from Rgbe: " << testImg2.Width() << " x " << testImg2.Height() << endl;
	cout << "Time to load image converting on the fly: " << timer.elapsedSeconds() << endl;
	cout << "Last pixel: " << testImg2[testImg2.Size()-1] << endl;

}


int main(int argc, char** argv) {
	
	// Init TBB
	//tbb::task_scheduler_init init;

	cout << "sizeof(Rgba32F): " << sizeof(Rgba32F) << endl;

	// What does the default constructor do?
	Rgba32F pDefault;
	cout << "Default: " << pDefault << endl;

	// A pixel with the same components
	Rgba32F p7(7.0f);
	cout << "All 7's: " << p7 << endl;

	// A pixel with different values
	Rgba32F p1(0.2f, 0.4f, 0.6f, 0.8f);
	cout << "Explicit values: " << p1 << endl;

	// Modifying values
	p1.setR(0.8f); p1.setG(0.6f); p1.setB(0.4f); p1.setA(0.2f);
	cout << "Mod values: " << p1 << endl;
	p1.set(1,2,3,4);
	cout << "More mod: : " << p1 << endl;

	// Testing RGBE pixels
	cout << endl << "Testing RGBE pixels..." << endl;
	cout << "sizeof(Rgbe): " << sizeof(Rgbe) << endl;

	Rgbe rgbe;
	cout << "Default value: " << rgbe << endl;

	rgbe.set(1,2,3,4);
	cout << "Explicit values: " << rgbe << endl;

	// Casting!!
	Rgba32F pCast = rgbe;	/* Here's an implicit (Rgba32F)rgbe */
	cout << "Casting from Rgbe: " << pCast << endl;

	// A real comparison between methods
	{
		Rgba32F pixel(200.f, 0.40f, 80.f);
		//Rgba32F pixel(0.20f, 0.40f, 0.80f);
		Rgbe pRgbe;

		cout << "Real pixel: " << pixel << endl;

		// Convert to RGBE using the standard code
		{
			float v;
			int e;

			v = pixel.r();
			if (pixel.g() > v) v = pixel.g();
			if (pixel.b() > v) v = pixel.b();
			if (v < 1e-32) {
				pRgbe.set(0,0,0,0);
			}
			else {
				v = (float)(frexp(v,&e) * 256.0/v);
				pRgbe.r = (unsigned char) (pixel.r() * v);
				pRgbe.g = (unsigned char) (pixel.g() * v);
				pRgbe.b = (unsigned char) (pixel.b() * v);
				pRgbe.e = (unsigned char) (e + 128);
			}
		}

		Rgbe pRgbe2(pixel);

		cout << "Old fashioned RGBE encoding: " << pRgbe  << endl;
		cout << "New RGBE encoding:           " << pRgbe2 << endl;
		cout << "Casting back to float: "      << (Rgba32F)pRgbe << endl;

		// Convert to float pixels using the old fashioned way
		{
			if (pRgbe.e) {   /*nonzero pixel*/
				const float f = (float)(ldexp(1.0,pRgbe.e-(int)(128+8)));
				pixel.set(pRgbe.r * f, pRgbe.g * f, pRgbe.b * f);
			}
			else
				pixel.setAll(0.0f);

		}
		cout << "Old style conversion:  "      << pixel << endl;
	}



	unsigned char *foo = (unsigned char*)&rgbe;
	cout << (int)foo[0] << " " << (int)foo[1] << " " << (int)foo[2] << " " << (int)foo[3] << endl;

	int index = *foo == 0 ? 0 : 1;

#if 0

	// Timing tests:
	unsigned char randNum[2048];
	srand(0);
	for (int i = 0; i < sizeof(randNum); ++i) {
		randNum[i] = (unsigned char)(0xFF & rand());
	}
	
	// Converting 2M numbers
	Timer oldTimer;
	Timer niceTimer;

	Rgba32F pcF(0);
	oldTimer.start();
	for (int i = 0; i < 900000; ++i) {
		for (int j = 0; j < sizeof(randNum); j+=4) {
			const Rgba32F tmp((float)randNum[j],(float)randNum[j+1],(float)randNum[j+2],(float)randNum[j+3]);
			pcF += tmp;
		}
	}
	oldTimer.stop();

	cout << "Sum " << (pcF.r() + pcF.g() + pcF.b() + pcF.a()) << endl;
	cout << "Normal way: " << oldTimer.elapsedSeconds() << endl;

	pcF.setAll(0);
	niceTimer.start();
	for (int i = 0; i < 900000; ++i) {
		for (int j = 0; j < sizeof(randNum); j+= 4) {
			const Rgba32F tmp = _mm_cvtepi32_ps(_mm_set_epi32(
				randNum[j],randNum[j+1],randNum[j+2],randNum[j+3]));
			pcF += tmp;
		}
	}
	niceTimer.stop();

	cout << "Sum " << (pcF.r() + pcF.g() + pcF.b() + pcF.a()) << endl;
	cout << "Parallel conversion way: " << niceTimer.elapsedSeconds() << endl;

	pcF.setAll(0);
	niceTimer.reset();
	niceTimer.start();
	for (int i = 0; i < 900000; ++i) {
		for (int j = 0; j < sizeof(randNum); j+= 4) {
			const Rgba32F tmp(_rgbeCastLUT[randNum[j]],_rgbeCastLUT[randNum[j+1]],
				_rgbeCastLUT[randNum[j+2]],_rgbeCastLUT[randNum[j+3]]);
			pcF += tmp;
		}
	}
	niceTimer.stop();

	cout << "Sum " << (pcF.r() + pcF.g() + pcF.b() + pcF.a()) << endl;
	cout << "LUT way: " << niceTimer.elapsedSeconds() << endl;

#endif
	// Testing conversion from float to RGBE
	//testConversion();

	// Test loading/conversion from RGBE files
	try {
		testRGBEImage();
	}
	catch(std::exception &e) {
		cerr << "Opps! " << e.what() << endl;
	}

	Timer timer;
	ToneMapper toneMapper(0xFFFF);

	timer.reset();
	toneMapper.SetGamma(1.8f);
	toneMapper.SetGamma(2.0f);
	toneMapper.SetGamma(2.2f);
	toneMapper.SetGamma(2.4f);
	toneMapper.SetGamma(2.6f);
	timer.stop();

	cout << "Time to setup the ToneMapper lut: " << timer.elapsedSeconds()/5 << endl;


	try {

	if(0) {
			// Loads the image converting on the fly
			Image<Rgba32F, TopDown> floatImage;
			RgbeIO::Load(floatImage, "horse.rgbe");

			// Now let's tone map the Rgba32F image
			Image<Bgra8> ldrImage(floatImage.Width(), floatImage.Height());
			toneMapper.SetGamma(2.0f);
			timer.reset();
			toneMapper.ToneMap(ldrImage, floatImage);
			timer.stop();
			cout << "Time to tone map the image: " << timer.elapsedSeconds() << endl;

			// Create a png image from the Bgra8 image
			timer.reset();
			PngIO::Save(ldrImage, "test-horse-bgra8-g2.png", false, 1/2.0f);
			timer.stop();
			cout << "Time to save as PNG 8bpp: " << timer.elapsedSeconds() << endl;

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
			cout << "Time to tone map the image (no LUT): " << timer.elapsedSeconds() << endl;
			PngIO::Save(ldrImage2, "test-horse-rgba8-srgb-noLUT.png", true);

			// Lets try with the high resolution version
			Image<Rgba16> ldrImage16(floatImage.Width(), floatImage.Height());
			timer.reset();
			toneMapper.ToneMap(ldrImage16, floatImage);
			Rgba16 &px = ldrImage16.ElementAt(200,300);
			cout << "High bpp pixel (200,300): " << px.r << ',' << px.g << ',' << px.b << endl;
			timer.stop();
			cout << "Time to tone map the 16bpp image (no LUT): " << timer.elapsedSeconds() << endl;

			timer.reset();
			PngIO::Save(ldrImage16, "test-horse-rgb16-srgb-noLUT.png", true);
			timer.stop();
			cout << "Time to save as PNG 16bpp: " << timer.elapsedSeconds() << endl;


		}

#if 0

		// Create a QImage from the Argb8 Image and save as PNG
		timer.reset();
		
		QImage qImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), ldrImage.Width(), ldrImage.Height(),
			QImage::Format_RGB32);
		timer.stop();
		cout << "Time to wrap in a QImage: " << timer.elapsedSeconds() << endl;
		timer.reset();
		qImage.save("test-horse.png");
		timer.stop();
		cout << "Time to save as PNG from QImage: " << timer.elapsedSeconds() << endl;
		cout << "Sanity: sizeof(long) " << sizeof(long) << endl;
#endif


		// Pfm testing
		if (true) {
			
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
		


	}
	catch(std::exception &e) {
		cerr << "Opps! " << e.what() << endl;
	}
	


	return 0;

}
