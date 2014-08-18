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

#include "FloatImageProcessor.h"
#include "ImageInfo.h"

#include <istream>
#include <Rgba32F.h>
#include <Image.h>
#include <RgbeIO.h>
#include <OpenEXRIO.h>
#include <PfmIO.h>

#include <QString>
#include <QRegExp>

#include <cstdio>
#include <QTextStream>
namespace
{
QTextStream qcerr(stderr, QIODevice::WriteOnly);
QTextStream qcout(stdout, QIODevice::WriteOnly);
}


ImageInfo* FloatImageProcessor::load(const QString& filenameStr,
                                     std::istream &is,
                                     const QString& formatStr, int offset)
{
    // Creates the used regular expressions
    QRegExp rgbeRegex(".+\\.(rgbe|hdr)$", Qt::CaseInsensitive);
    QRegExp exrRegex(".+\\.exr$", Qt::CaseInsensitive);
    QRegExp pfmRegex(".+\\.pfm$", Qt::CaseInsensitive);

    // Local copy of the filename
    QString filename(filenameStr);

    // Pointer with the result image
    Image<Rgba32F> *floatImage = new Image<Rgba32F>();

    // Tries to find the type of image based on the extension
    try {
        if (rgbeRegex.exactMatch(filename)) {

            // Creates the RGBE Image from the stream
            RgbeIO::Load(*floatImage, is);
        }
        else if (exrRegex.exactMatch(filename)) {

            // Loads the OpenEXR file
            OpenEXRIO::Load(*floatImage, is);
        }
        else if (pfmRegex.exactMatch(filename)) {

            // Loads the Pfm file
            PfmIO::Load(*floatImage, is);
        }
        else {
            qcerr << "Ooops! Unrecognized file : " << filename << endl;
            // Returns an invalid handle
            return new ImageInfo;
        }
    }
    catch (std::exception e) {
        qcerr << "Ooops! While loading " << filename << ": " << e.what() << endl;
        return new ImageInfo;
    }

    assert(floatImage->Height() > 0 && floatImage->Width() > 0);

    // Gets the output name
    setTargetName(filename, formatStr, offset);

    // The data is ready for the next stage, just return it
    ImageInfo *info = new ImageInfo(floatImage, filenameStr, filename);
    return info;

}

void FloatImageProcessor::setTargetName(QString & filename,
                                        const QString & formatStr, int offset)
{
    QRegExp trailingDigit("(\\d+)\\.(\\w+)$");
    int pos = -1;
    if(offset != 0 && (pos = trailingDigit.indexIn(filename)) > -1) {
        
        // cap(1) corresponds to (\d+) : a sequence of digits
        QString numValue = trailingDigit.cap(1);
        // By construction this must yield a valid number
        const int val = numValue.toInt() + offset;
        const int fieldWidth = numValue.length();
        // Replace the trailing number with the appropriate padding while keeping
        // the original extension and what was before the number.
        filename.replace(pos, trailingDigit.matchedLength(), QString("%1.%2")
            .arg(val, fieldWidth, /*base=*/10, /*fillChar=*/ QLatin1Char('0'))
            .arg(formatStr) );
    }
    else {
        // Create the output filename just by replacing the extension
        QRegExp extRegex("\\.\\w+$");
        filename.replace( extRegex, QString(".%1").arg(formatStr) );
    }

}
