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

// Just a convenience class

#pragma once
#if !defined(PCG_RGBEIMAGE_H)
#define PCG_RGBEIMAGE_H

#include "Image.h"
#include "rgbe.h"
#include "RgbeIO.h"

namespace pcg {

	// An RgbeImage: its scanline order is always TopDown:
	// The RGBE file spec says its order is bottom up, but the scanlines
	// are stored top-down by default (that's why the header says "-Y")
	class RgbeImage : public Image<Rgbe, TopDown> {

	public:

		// Create a new image with the specified dimensions. The content of the pixels
		// is undefined! The space for them is already allocated though, you only
		// need to fill them with something useful
		RgbeImage(int width, int height) : Image<Rgbe, TopDown>(width, height) {}

		// Creates a new instance by reading from the specified stream.
		// If something fails an exception is thrown
		RgbeImage(istream &is) {
			RgbeIO::Load(*this, is);
		}

		// Creates a new instance by reading from the specified file.
		// If something fails an exception is thrown.
		RgbeImage(const char *filename) {
			RgbeIO::Load(*this, filename);
		}
	};

}


#endif /* PCG_RGBEIMAGE_H */
