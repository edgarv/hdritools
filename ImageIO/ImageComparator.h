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

// Class which provides different methods for comparing
// two floating point images with the same scanline order

#if !defined(PCG_IMAGECOMPARATOR_H)
#define PCG_IMAGECOMPARATOR_H

#include "ImageIO.h"
#include "Image.h"
#include "ImageSoA.h"
#include "Rgba32F.h"

namespace pcg {

	class ImageComparator {

	public:
		// Enumeration with the different comparison methods
		enum Type {
			AbsoluteDifference,
			Addition,
			Division,
			RelativeError,
			PositiveNegative,
			PositiveNegativeRelativeError
		};
		
		static IMAGEIO_API void Compare(Type type, Image<Rgba32F, TopDown> &dest, 
			const Image<Rgba32F, TopDown> &src1, const Image<Rgba32F, TopDown> &src2);

		static IMAGEIO_API void Compare(Type type, Image<Rgba32F, BottomUp> &dest, 
			const Image<Rgba32F, BottomUp> &src1, const Image<Rgba32F, BottomUp> &src2);

		static IMAGEIO_API void Compare(Type type, RGBAImageSoA &dest,
			const RGBAImageSoA &src1, const RGBAImageSoA &src2);
		
	private:
		// Just for the sake of knowing what we have
		template <ScanLineMode S>
		static void CompareHelper(Type type, Image<Rgba32F, S> &dest, 
			const Image<Rgba32F, S> &src1, const Image<Rgba32F, S> &src2);
	};

}

#endif /* PCG_IMAGECOMPARATOR_H */
