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

// IO operation to create and load RGBE files

#if !defined(PCG_RGBEIO_H)
#define PCG_RGBEIO_H

#include "ImageIO.h"
#include "rgbe.h"
#include "Rgba32F.h"
#include "Rgb32F.h"
#include "Image.h"
#include "ImageSoA.h"

namespace pcg {

	// To keep the code simple, we only allow to load the common formats.
	// This way we can instanciate the templates in the cpp and have slimmer headers
	class RgbeIO {

	public:

		// ### Load functions ###

		// Rgbe pixels
		static IMAGEIO_API void Load(Image<Rgbe,TopDown>  &img, istream &is);
		static IMAGEIO_API void Load(Image<Rgbe,BottomUp> &img, istream &is);
		static IMAGEIO_API void Load(Image<Rgbe,TopDown>  &img, const char *filename);
		static IMAGEIO_API void Load(Image<Rgbe,BottomUp> &img, const char *filename);

		// Rgba32F pixels
		static IMAGEIO_API void Load(Image<Rgba32F,TopDown>  &img, istream &is);
		static IMAGEIO_API void Load(Image<Rgba32F,BottomUp> &img, istream &is);
		static IMAGEIO_API void Load(Image<Rgba32F,TopDown>  &img, const char *filename);
		static IMAGEIO_API void Load(Image<Rgba32F,BottomUp> &img, const char *filename);

		// Rgb32F pixels
		static IMAGEIO_API void Load(Image<Rgb32F,TopDown>  &img, istream &is);
		static IMAGEIO_API void Load(Image<Rgb32F,BottomUp> &img, istream &is);
		static IMAGEIO_API void Load(Image<Rgb32F,TopDown>  &img, const char *filename);
		static IMAGEIO_API void Load(Image<Rgb32F,BottomUp> &img, const char *filename);

		// RGBA SoA Image
		static IMAGEIO_API void Load(RGBAImageSoA& img, istream& is);
		static IMAGEIO_API void Load(RGBAImageSoA& img, const char* filename);


		// ### Save functions ###

		// Rgbe pixels
		static IMAGEIO_API void Save(const Image<Rgbe,TopDown>  &img, ostream &os);
		static IMAGEIO_API void Save(const Image<Rgbe,BottomUp> &img, ostream &os);
		static IMAGEIO_API void Save(const Image<Rgbe,TopDown>  &img, const char *filename);
		static IMAGEIO_API void Save(const Image<Rgbe,BottomUp> &img, const char *filename);

		// Rgba32F pixels
		static IMAGEIO_API void Save(const Image<Rgba32F,TopDown>  &img, ostream &os);
		static IMAGEIO_API void Save(const Image<Rgba32F,BottomUp> &img, ostream &os);
		static IMAGEIO_API void Save(const Image<Rgba32F,TopDown>  &img, const char *filename);
		static IMAGEIO_API void Save(const Image<Rgba32F,BottomUp> &img, const char *filename);

		// Rgb32F pixels
		static IMAGEIO_API void Save(const Image<Rgb32F,TopDown>  &img, ostream &os);
		static IMAGEIO_API void Save(const Image<Rgb32F,BottomUp> &img, ostream &os);
		static IMAGEIO_API void Save(const Image<Rgb32F,TopDown>  &img, const char *filename);
		static IMAGEIO_API void Save(const Image<Rgb32F,BottomUp> &img, const char *filename);

		// RGBA SoA Image
		static IMAGEIO_API void Save(const RGBAImageSoA& img, ostream& os);
		static IMAGEIO_API void Save(const RGBAImageSoA& img, const char* filename);

	};


}


#endif /* PCG_RGBEIO_H */
