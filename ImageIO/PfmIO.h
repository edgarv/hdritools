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

/** 
 * IO Functions for Debevec's Portable floatmap (Pfm) format.
 * as documented in 
 * http://gl.ict.usc.edu/HDRShop/PFM/PFM_Image_File_Format.html
 */

#if !defined (PCG_PFMIO_H)
#define PCG_PFMIO_H

#include "ImageIO.h"
#include "Image.h"
#include "Rgba32F.h"
#include "Exception.h"

namespace pcg {

	PCG_DEFINE_EXC(PfmIOException, IOException);

	class PfmIO {

	private:
		enum ByteOrder {
			LittleEndian,
			BigEndian
		};

		struct Header {
			bool isColor;
			int  width;
			int  height;
			ByteOrder order;

			Header();
			Header(const Image<Rgba32F, TopDown> &img);
			Header(const Image<Rgba32F, BottomUp> &img);
			Header(istream &is);

			void write(ostream &os);
		};

		static ByteOrder getNativeOrder();

	public:
		static void IMAGEIO_API Load(Image<Rgba32F, TopDown>  &img, const char *filename, bool closeStream = true);
		static void IMAGEIO_API Load(Image<Rgba32F, BottomUp> &img, const char *filename, bool closeStream = true);
		static void IMAGEIO_API Load(Image<Rgba32F, TopDown>  &img, istream &is);
		static void IMAGEIO_API Load(Image<Rgba32F, BottomUp> &img, istream &is);

		static IMAGEIO_API void Save(const Image<Rgba32F, TopDown>  &img, ostream &os);
		static IMAGEIO_API void Save(const Image<Rgba32F, BottomUp> &img, ostream &os);
		static void IMAGEIO_API Save(const Image<Rgba32F, TopDown>  &img, const char *filename, bool closeStream = true);
		static void IMAGEIO_API Save(const Image<Rgba32F, BottomUp> &img, const char *filename, bool closeStream = true);
	};

}

#endif /* PCG_PFMIO_H */
