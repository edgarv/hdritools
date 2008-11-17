// Implementation for the Image Data Providers
#include "ImageDataProvider.h"
#include <Exception.h>

void ImageDataProvider::setSize( const QSize &otherSize ) {
	if (otherSize != _size) {
		_size = otherSize;
		sizeChanged(_size);
	}
}

// ----------------------------------------------------------------------------

ImageIODataProvider::ImageIODataProvider(const Image<Rgba32F> &hdrImage, const Image<Bgra8> &ldrImage) :
	hdr(hdrImage), ldr(ldrImage)
{
	update();
}

void ImageIODataProvider::update()
{
	// Validates that they are the same size
	if (hdr.Width() != ldr.Width() || hdr.Height() != ldr.Height()) {
		throw IllegalArgumentException("Incongruent sizes!");
	}

	// Now we can safely set the size of this guy
	QSize size(hdr.Width(), hdr.Height());
	setSize(size);
}


void ImageIODataProvider::getLdrPixel(int x, int y, 
	unsigned char &rOut, unsigned char &gOut, unsigned char &bOut) const 
{
	const Bgra8 &pix = ldr.ElementAt(x,y);
	rOut = pix.r;
	gOut = pix.g;
	bOut = pix.b;
}


void ImageIODataProvider::getHdrPixel(int x, int y, 
	float &rOut, float &gOut, float &bOut) const
{
	const Rgba32F &pix = hdr.ElementAt(x,y);
	rOut = pix.r();
	gOut = pix.g();
	bOut = pix.b();
}
