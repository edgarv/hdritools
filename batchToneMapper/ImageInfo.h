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

#if !defined IMAGEINFO_H
#define IMAGEINFO_H

#include <Image.h>
#include <Rgba32F.h>
#include <QString>

using namespace pcg;

/** Small Transfer object to pass info between the pipeline stages */

struct ImageInfo {
    const Image<Rgba32F> *img;
    const QString originalFile;
    const QString filename;
    const bool isValid;

    ImageInfo(Image<Rgba32F> *image, const char *input, const char *outname) :
        img(image), originalFile(input), filename(outname), isValid(true) {}

    ImageInfo(Image<Rgba32F> *image, const QString &input, const QString &outname) :
        img(image), originalFile(input), filename(outname), isValid(true) {}

    ImageInfo() : img(NULL), isValid(false) {}

    ~ImageInfo() {
        if (img != NULL) {
            delete img;
        }
    }
};

#endif /* IMAGEINFO_H */
