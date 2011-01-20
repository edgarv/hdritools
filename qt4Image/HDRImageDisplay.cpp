#include "HDRImageDisplay.h"

#include "RgbeIO.h"
#include "OpenEXRIO.h"
#include "PfmIO.h"

#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QMouseEvent>
#include <QtDebug>



HDRImageDisplay::HDRImageDisplay(QWidget *parent) : QWidget(parent), 
    toneMapper(0.0f, 4096), dataProvider(hdrImage, ldrImage),
    scaleFactor(1), needsToneMap(true), technique(EXPOSURE)
{
    // By default we want to receive events whenever the mouse moves around
    setMouseTracking(true);
/*
    sizeAux.setWidth(ldrImage.Width());
    sizeAux.setHeight(ldrImage.Height());

    resize(sizeAux);
    */
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

    Image<Rgba32F> other;

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


bool HDRImageDisplay::loadHdr(const QString & fileName, Image<Rgba32F> &hdr) 
{
    QFileInfo fileInfo(fileName);
    QString suffix = fileInfo.suffix();		// Suffix without the trailing "."
    
    // Will only work if the suffix is .rgbe or .hdr
    if (suffix.compare("rgbe", Qt::CaseInsensitive) == 0 ||
        suffix.compare("hdr",  Qt::CaseInsensitive) == 0) 
    {
        // Loads the RGBE Image
        RgbeIO::Load(hdr,    QFile::encodeName(fileName).constData() );
    }
    else if (suffix.compare("exr", Qt::CaseInsensitive) == 0)
    {
        // Loads from an OpenEXR Image
        OpenEXRIO::Load(hdr, QFile::encodeName(fileName).constData());
    }
    else if (suffix.compare("pfm", Qt::CaseInsensitive) == 0)
    {
        // Loads the Debevec's Pfm file
        PfmIO::Load(hdr, QFile::encodeName(fileName).constData());
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
        if (suffix.compare("rgbe", Qt::CaseInsensitive) == 0 ||
            suffix.compare("hdr",  Qt::CaseInsensitive) == 0) 
        {
            // Saves a RGBE Image
            RgbeIO::Save(hdrImage,    QFile::encodeName(fileName).constData() );
        }
        else if (suffix.compare("exr", Qt::CaseInsensitive) == 0)
        {
            // Saves an OpenEXR Image
            OpenEXRIO::Save(hdrImage, QFile::encodeName(fileName).constData() );
        }
        else if (suffix.compare("pfm", Qt::CaseInsensitive) == 0)
        {
            // Saves a Pfm Image
            PfmIO::Save(hdrImage, QFile::encodeName(fileName).constData() );
        }
        else {

            // We just save the currently displayed image, that's it!
            Q_ASSERT( ldrImage.Width() > 0 && ldrImage.Height() > 0 );
            return qImage.save(fileName);
        }
        return true;
    }
    catch(...) {
        return false;
    }

}

void HDRImageDisplay::paintEvent(QPaintEvent *event) 
{
    if (needsToneMap && hdrImage.Width() > 0 && hdrImage.Height() > 0) {
        toneMapper.ToneMap(ldrImage, hdrImage, true, technique);
        needsToneMap = false;
    }

    QPainter painter(this);

    painter.scale(scaleFactor,scaleFactor);

    painter.drawImage(/*x*/ 0, /*y*/ 0, qImage);

}



void HDRImageDisplay::setWhitePoint(double value)
{
    const float l_white = static_cast<float>(value);
    if (!qFuzzyCompare(l_white, reinhard02Params.l_white)) {
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
    mouseOverPixel(QPoint((int)floor(event->pos().x()/scaleFactor), 
        (int)floor(event->pos().y()/scaleFactor)));
}
