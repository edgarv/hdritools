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

#include <QDebug>

void ImageDataProvider::setSize( const QSize &otherSize ) {
    if (otherSize != _size) {
        _size = otherSize;
        sizeChanged(_size);
    }
}

void ImageDataProvider::setWhitePointRange( const range_t &otherRange, double otherAvgLuminance ) {
    if (otherRange != _whitePointRange || qFuzzyCompare(otherAvgLuminance, _avgLuminance)) {
        _whitePointRange = otherRange;
        _avgLuminance    = otherAvgLuminance;
        qDebug() << "Setting white point range: " << _whitePointRange.first << ", " << _whitePointRange.second
                 << ". Average: " << _avgLuminance;
        whitePointRangeChanged(_whitePointRange.first, _avgLuminance, _whitePointRange.second);
    }
}

// ----------------------------------------------------------------------------

ImageIODataProvider::ImageIODataProvider(const RGBAImageSoA &hdrImage,
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
    if (!size.isEmpty()) {
        Reinhard02::Params params = Reinhard02::EstimateParams(hdr);
        whitePoint = params.l_white;
        key = params.key;
        lw  = params.l_w;
        setWhitePointRange(0.875*params.l_min, params.l_w,
            1.125*qMax(params.l_max,params.l_white));
    }
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
    rOut = hdr.ElementAt<RGBAImageSoA::R>(x, y);
    gOut = hdr.ElementAt<RGBAImageSoA::G>(x, y);
    bOut = hdr.ElementAt<RGBAImageSoA::B>(x, y);
}


void ImageIODataProvider::getToneMapDefaults(double &whitePointOut,
                                             double &keyOut) const
{
    whitePointOut = whitePoint;
    keyOut = key;
}
