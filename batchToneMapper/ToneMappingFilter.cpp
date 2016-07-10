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

#include "ToneMappingFilter.h"
#include "ImageInfo.h"

#include <QImage>
#include <PngIO.h>

#include <ostream>
#include <cstdio>
#include <QTextStream>
#include <QMutex>
#include <QMutexLocker>

namespace
{
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream cout(stdout, QIODevice::WriteOnly);

QMutex write_mutex;
}


ToneMappingFilter::ToneMappingFilter(ToneMapper &toneMapper, bool useBpp16) : 
filter(/*is_serial=*/false),
toneMapper(toneMapper), useBpp16(useBpp16), technique(pcg::EXPOSURE),
key(AutoParam()), whitePoint(AutoParam()), logLumAvg(AutoParam())
{
}


ToneMappingFilter::ToneMappingFilter(ToneMapper &toneMapper, bool useBpp16,
                                     float k, float wp, float lw) :
filter(/*is_serial=*/!isReinhard02Fixed(k, wp, lw)),
toneMapper(toneMapper), useBpp16(useBpp16), technique(pcg::REINHARD02),
key(k), whitePoint(wp), logLumAvg(lw)
{
}


void* ToneMappingFilter::operator()(void* item)
{
    try {
        ImageInfo *info = static_cast<ImageInfo *>(item);
        assert( info != NULL );

        // Abort if its invalid
        if (! info->isValid) {
            delete info;
            return NULL;
        }

        const Image<Rgba32F> &floatImage = *(info->img);
        if (technique == pcg::REINHARD02) {
            pcg::Reinhard02::Params params;
            if (isReinhard02Fixed()) {
                params.key     = key;
                params.l_white = whitePoint;
                params.l_w     = logLumAvg;
            } else {
                params = pcg::Reinhard02::EstimateParams(floatImage);
                if (key        != AutoParam()) params.key     = key;
                if (whitePoint != AutoParam()) params.l_white = whitePoint;
                if (logLumAvg  != AutoParam()) params.l_w     = logLumAvg;
            }
            toneMapper.SetParams(params);
        }

        // Allocates the LDR Image and tonemaps it
        if (!useBpp16) {
            Image<Bgra8> ldrImage(floatImage.Width(), floatImage.Height());
            toneMapper.ToneMap(ldrImage, floatImage, true, technique);

            // Finally wraps the ldrImage into a QImage and saves it 
            // with the specified name
            QImage qImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), 
                ldrImage.Width(), ldrImage.Height(), QImage::Format_RGB32);

            // TODO: The name might contain a path, so should we create it if
            // it doesn't exist?
            if ( !qImage.save(info->filename) ) {
                cerr << "Ooops! unable to save " << info->filename << ". Are you sure it's valid?" << endl;
            }
        }
        else {
            Image<Rgba16> ldrImage(floatImage.Width(), floatImage.Height());
            toneMapper.ToneMap(ldrImage, floatImage, technique);

            try {
                PngIO::Save(ldrImage, info->filename.toLocal8Bit(),
                    toneMapper.isSRGB(), toneMapper.InvGamma());
            }
            catch (std::exception &e) {
                cerr << "Ooops! unable to save " << info->filename << ": " << e.what() << endl;
            }
        }

        {
            QMutexLocker lock(&write_mutex);
            cout << info->originalFile << " -> " << info->filename << endl;
        }

        // Deletes the info structure when it's done
        delete info;
    }
    catch(std::exception &e) {
        cerr << "Ooops! " << e.what() << endl;
    }

    // Always returns null, as it's in the last part of the pipeline
    return NULL;
}
