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

#include "HDRImageDisplay.h"

#include "RgbeIO.h"
#include "OpenEXRIO.h"
#include "PfmIO.h"
#include <LoadHDR.h>

#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QClipboard>
#include <QtDebug>

#include <fstream>



HDRImageDisplay::HDRImageDisplay(QWidget *parent) : QWidget(parent), 
    toneMapper(0.0f, 2.2f), dataProvider(hdrImage, ldrImage),
    scaleFactor(1), needsToneMap(true), technique(EXPOSURE)
{
    // By default we want to receive events whenever the mouse moves around
    setMouseTracking(true);
}



HDRImageDisplay::~HDRImageDisplay()
{
}



bool HDRImageDisplay::open(const QString &fileName, HdrResult * result) 
{
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix();		// Suffix without the trailing "."

    try {

        // Try to load the image
        if (!loadHdr(fileName, hdrImage)) {
            // Terrible case: we don't know what kind of file is this one!
            if (result != NULL) { *result = UnknownType; }
            return false;
        }

        // At this point we must have a valid HDR image loaded
        Q_ASSERT(hdrImage.Width() > 0 && hdrImage.Height() > 0);

        // Updates the size of the LDR image, also a tone map will be needed.
        ldrImage.Alloc(hdrImage.Width(), hdrImage.Height());

        qImage = QImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), 
            ldrImage.Width(), ldrImage.Height(), QImage::Format_RGB32);
        
        resize(ldrImage.Width(), ldrImage.Height());
        dataProvider.update();
        // Keep the white point and key specified by the GUI (signal-updated)
        reinhard02Params.l_w = 
            static_cast<float>(dataProvider.avgLogLuminance());
        toneMapper.SetParams(reinhard02Params);

        needsToneMap = true;
        update();

        if (result != NULL) { *result = NoError; }
        return true;

    }
    catch (const std::exception &e) {
        qDebug() << "HDRImageDisplay::open exception: " << e.what();
        if (result != NULL) { *result = ExceptionError; }
        return false;
    }
    catch (...) {
        if (result != NULL) { *result = ExceptionError; }
        return false;
    }
}

bool HDRImageDisplay::compareTo(const QString &fileName, ImageComparator::Type compareMethod,
                                HdrResult *result)
{
    // If nothing has been loaded, that's an illegal state
    if (sizeOrig().isEmpty()) {
        if (result != NULL) { *result = IllegalState; }
        return false;
    }

    RGBAImageSoA other;

    try {

        // If there is actually a file, we try to load it
        if (!loadHdr(fileName, other)) {
            // Terrible case: we don't know what kind of file is this one!
            if (result != NULL) { *result = UnknownType; }
            return false;
        }

        // The sizes must be the same
        if (hdrImage.Width() != other.Width() || hdrImage.Height() != other.Height()) {
            if (result != NULL) { *result = SizeMissmatch; }
            return false;
        }

        // Now we perform the comparison operation in place
        ImageComparator::Compare(compareMethod, hdrImage, hdrImage, other);

        // The sizes have not changed, thus the only thing required is a tone map
        // and an update
        needsToneMap = true;
        update();

        if (result != NULL) { *result = NoError; }
        return true;
    }
    catch(...) {
        if (result != NULL) { *result = ExceptionError; }
        return false;
    }
}


bool HDRImageDisplay::loadHdr(const QString & fileName, RGBAImageSoA &hdr) 
{
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix();		// Suffix without the trailing "."
    
    // Will only work if the suffix is either .rgbe, .hdr, .exr or .pfm
    if (suffix.compare("rgbe", Qt::CaseInsensitive) == 0 ||
        suffix.compare("hdr",  Qt::CaseInsensitive) == 0 ||
        suffix.compare("exr",  Qt::CaseInsensitive) == 0 ||
        suffix.compare("pfm",  Qt::CaseInsensitive) == 0)
    {
        try {
#if !defined(_WIN32)
            pcg::LoadHDR(hdr, qPrintable(fileName));
#else
            // Assume QChar is binary-compatible with wchar_t
#if defined(_MSC_VER) && (_MSC_VER >= 1600)
            static_assert(sizeof(QChar) == sizeof(wchar_t), "Incompatible!");
#endif
            const wchar_t* wFileName =
                reinterpret_cast<const wchar_t*>(fileName.constData());
            pcg::LoadHDR(hdr, wFileName);
#endif
        }
        catch (UnkownFileType&) {
            return false;
        }
    }
    else {
        return false;
    }

    return true;
}


bool HDRImageDisplay::save(const QString & fileName) 
{
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix();

    // We must have something to save
    Q_ASSERT( hdrImage.Width() > 0 && hdrImage.Height() > 0 );

    try {
        // First we try to save it as an HDR File
        enum { HDR_RGBE, HDR_EXR, HDR_PFM, NO_HDR };
        int type = NO_HDR;

        if (suffix.compare("rgbe", Qt::CaseInsensitive) == 0 ||
            suffix.compare("hdr",  Qt::CaseInsensitive) == 0) {
            type = HDR_RGBE;
        }
        else if (suffix.compare("exr", Qt::CaseInsensitive) == 0) {
            type = HDR_EXR;
        }
        else if (suffix.compare("pfm", Qt::CaseInsensitive) == 0) {
            type = HDR_PFM;
        }

        std::ofstream os;
        if (type != NO_HDR) {
#if !defined(_WIN32)
            os.open(qPrintable(fileName), ios_base::binary);
#else
            const wchar_t *wFileName =
                reinterpret_cast<const wchar_t*>(fileName.constData());
            os.open(wFileName, ios_base::binary);
#endif
            if (!os)
                return false;
        }

        switch(type) {
        case HDR_RGBE:
            RgbeIO::Save(hdrImage, os);
            return true;
        case HDR_EXR:
            OpenEXRIO::Save(hdrImage, os);
            return true;
        case HDR_PFM:
            PfmIO::Save(hdrImage, os);
            return true;
        default:
            // We just save the currently displayed image, that's it!
            Q_ASSERT( ldrImage.Width() > 0 && ldrImage.Height() > 0 );
            return qImage.save(fileName);
        }
    }
    catch(...) {
        return false;
    }
}

void HDRImageDisplay::paintEvent(QPaintEvent *event) 
{
    if (needsToneMap && hdrImage.Width() > 0 && hdrImage.Height() > 0) {
        toneMapper.ToneMap(ldrImage, hdrImage, technique);
        needsToneMap = false;
    }

    QPainter painter(this);

    painter.scale(scaleFactor,scaleFactor);
    if (scaleFactor < static_cast<qreal>(1)) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
    }
    painter.drawImage(/*x*/ 0, /*y*/ 0, qImage);
}



void HDRImageDisplay::setGamma(float gamma)
{
    if (gamma != toneMapper.Gamma()) {
        toneMapper.SetGamma(gamma);
        needsToneMap = true;
        update();
    }
}

void HDRImageDisplay::setExposure(float exposure)
{
    if (exposure != toneMapper.Exposure()) {
        toneMapper.SetExposure(exposure);
        needsToneMap = true;
        update();
    }
}

void HDRImageDisplay::setSRGB(bool enable)
{
    if (enable != toneMapper.isSRGB()) {
        toneMapper.SetSRGB(enable);
        needsToneMap = true;
        update();
    }
}


void HDRImageDisplay::setWhitePoint(double value)
{
    // Adjust by the average log luminance to get a more predictable behavior
    float l_white = static_cast<float>(value);
    l_white /= reinhard02Params.l_w;
    if (!qFuzzyCompare(l_white, reinhard02Params.l_white)) {
        qDebug() << "New white point: raw input" << value
                 << ", avg log lum: " << reinhard02Params.l_w
                 << ", adjusted: " << l_white;
        reinhard02Params.l_white = l_white;
        toneMapper.SetParams(reinhard02Params);
        if (technique == REINHARD02) {
            needsToneMap = true;
            update();
        }
    }
}


void HDRImageDisplay::setKey(double value)
{
    const float key = static_cast<float>(value);
    if (!qFuzzyCompare(key, reinhard02Params.key)) {
        reinhard02Params.key = key;
        toneMapper.SetParams(reinhard02Params);
        if (technique == REINHARD02) {
            needsToneMap = true;
            update();
        }
    }
}


void HDRImageDisplay::setReinhard02(bool enabled)
{
    const TmoTechnique newTechnique = enabled ? REINHARD02 : EXPOSURE;
    if (newTechnique != technique) {
        technique = newTechnique;
        toneMapper.SetParams(reinhard02Params);
        needsToneMap = true;
        update();
    }
}



void HDRImageDisplay::mouseMoveEvent(QMouseEvent * event)
{
    // The position of the event is relative to the image: this means that we
    // don't have to worry about scrollbars!
    // However it is possible that when the event is reported the position is
    // outside the boundaries of the image (i.e. the event is reported when
    // the mouse is outside the widget, which happens with rapid movements)
    qreal invScale = static_cast<qreal>(1) / scaleFactor;
    int x = static_cast<int>(event->pos().x() * invScale);
    int y = static_cast<int>(event->pos().y() * invScale);
    x = qBound(0, x, hdrImage.Width() - 1);
    y = qBound(0, y, hdrImage.Height() - 1);
    mouseOverPixel(QPoint(x, y));
}



void HDRImageDisplay::copyToClipboard()
{
    // There must be something valid
    Q_ASSERT( hdrImage.Width() > 0 && hdrImage.Height() > 0 );

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(qImage);    
}
