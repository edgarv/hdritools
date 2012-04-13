/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2012 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

/*
 * Helper functions to load HDR files by taking a guess on the magic numbers,
 * throwing an exception if there is an error.
 */

#pragma once
#if !defined (PCG_LOADHDR_H)
#define PCG_LOADHDR_H

#include "ImageIO.h"
#include "Image.h"
#include "Rgba32F.h"

#include <istream>
#include <string>

namespace pcg
{
    // The Load(...) methods will throw this to signal an unsupported file
    PCG_DEFINE_EXC(UnkownFileType, IOException)

    // Load the image from an already open binary stream. The stream must
    // support the seek operation. Notice that the stream will not be
    // closed after using this function.
	IMAGEIO_API void LoadHDR(Image<Rgba32F,TopDown> &img, std::istream &is);

    // Load the image from a file.
	IMAGEIO_API void LoadHDR(Image<Rgba32F,TopDown> &img, const char *filename);

    inline static void LoadHDR(Image<Rgba32F,TopDown> &img, const std::string& filename) {
        LoadHDR(img, filename.c_str());
    }

} // namespace pcg

#endif /* PCG_LOADHDR_H */
