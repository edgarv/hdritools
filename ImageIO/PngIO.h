#if !defined (PCG_PNGIO_H)
#define PCG_PNGIO_H

#include "ImageIO.h"
#include "Image.h"
#include "LDRPixels.h"

namespace pcg {

	class PngIO {
	public:

		static IMAGEIO_API void Save(Image<Rgba16,TopDown>   &img, 
			const char *filename, const bool isSrgb = true, const float invGamma = 1.0f/2.2f);
		static IMAGEIO_API void Save(Image<Rgba16,BottomUp>  &img, 
			const char *filename, const bool isSrgb = true, const float invGamma = 1.0f/2.2f);

		static IMAGEIO_API void Save(Image<Rgba8,TopDown>   &img, 
			const char *filename, const bool isSrgb = true, const float invGamma = 1.0f/2.2f);
		static IMAGEIO_API void Save(Image<Rgba8,BottomUp>  &img, 
			const char *filename, const bool isSrgb = true, const float invGamma = 1.0f/2.2f);
		
		static IMAGEIO_API void Save(Image<Bgra8,TopDown>   &img, 
			const char *filename, const bool isSrgb = true, const float invGamma = 1.0f/2.2f);
		static IMAGEIO_API void Save(Image<Bgra8,BottomUp>  &img, 
			const char *filename, const bool isSrgb = true, const float invGamma = 1.0f/2.2f);
	};

}

#endif /* PCG_PNGIO_H */
