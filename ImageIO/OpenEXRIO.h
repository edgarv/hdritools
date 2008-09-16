// Base functions used to load an OpenEXR file which
// is assumed to be in the RGB format, anything else
// will be supported.

#if !defined (PCG_OPENEXRIO_H)
#define PCG_OPENEXRIO_H

#include "ImageIO.h"
#include "Image.h"
#include "Rgba32F.h"

namespace pcg {

	// The base class has only static method
	class OpenEXRIO {

	public:

		// An enum for the different types of compression available when saving the file
		// (Valid as of OpenEXR 1.6.1)
		enum Compression {
			None,
			RLE,
			ZIPS,
			ZIP,
			PIZ,
			PXR24,
			B44,
			B44A
		};

		static void Load(Image<Rgba32F, TopDown> &img, const char *filename) {
			LoadHelper(img, filename);
		}

		static void Load(Image<Rgba32F, TopDown> &img, istream &is) {
			LoadHelper(img, is);
		}

		static void Load(Image<Rgba32F, BottomUp> &img, const char *filename) {
			// TODO Fix this lazy, lame method
			Image<Rgba32F, TopDown> tmp;
			LoadHelper(tmp, filename);
			img.Alloc(tmp.Width(), tmp.Height());
			
			// Stupid copy!
			for (int i = 0; i < img.Height(); ++i) {

				const Rgba32F *src = tmp.GetScanlinePointer(i, BottomUp);
				Rgba32F *dest = img.GetScanlinePointer(i, BottomUp);

				for (int j = 0; j < img.Width(); ++j) {
					_mm_stream_ps((float*)dest[j], src[j]);
				}
			}
		}

		// To save the images with a different scanline order we only set a flag!
		static void IMAGEIO_API Save(Image<Rgba32F, TopDown> &img, const char *filename, Compression compression = ZIP);
		static void IMAGEIO_API Save(Image<Rgba32F, BottomUp> &img, const char *filename, Compression compression = ZIP);

	private:
		static void IMAGEIO_API LoadHelper(Image<Rgba32F, TopDown> &img, const char *filename);
		static void IMAGEIO_API LoadHelper(Image<Rgba32F, TopDown> &img, istream &is);

		// Declare the super utility function for saving
		template<ScanLineMode S>
		static void SaveHelper(Image<Rgba32F, S> &img, const char *filename, Compression compression = ZIP);

	};
}


#endif
