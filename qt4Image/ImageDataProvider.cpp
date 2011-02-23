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

// Implementation for the Image Data Providers
#include "ImageDataProvider.h"
#include <Exception.h>
#include <Reinhard02.h>

void ImageDataProvider::setSize( const QSize &otherSize ) {
    if (otherSize != _size) {
        _size = otherSize;
        sizeChanged(_size);
    }
}

void ImageDataProvider::setWhitePointRange( const range_t &otherRange ) {
    if (otherRange != _whitePointRange) {
        _whitePointRange = otherRange;
        whitePointRangeChanged(_whitePointRange.first,
            _whitePointRange.second);
    }
}

// ----------------------------------------------------------------------------

ImageIODataProvider::ImageIODataProvider(const Image<Rgba32F> &hdrImage,
                                         const Image<Bgra8> &ldrImage)
: hdr(hdrImage), ldr(ldrImage), whitePoint(0.0), key(0.0), lw(0.0)
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

    // Get also the tone mapping settings
    Reinhard02::Params params = Reinhard02::EstimateParams(hdr);
    whitePoint = params.l_white;
    key = params.key;
    lw  = params.l_w;
    setWhitePointRange(params.l_min, 1.125*qMax(params.l_max,params.l_white));
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


void ImageIODataProvider::getToneMapDefaults(double &whitePointOut,
                                             double &keyOut) const
{
    whitePointOut = whitePoint;
    keyOut = key;
}


double ImageIODataProvider::avgLogLuminance() const
{
    return lw;
}
