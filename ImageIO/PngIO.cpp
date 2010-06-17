#include "PngIO.h"
#include "LDRPixels.h"
#include "Image.h"
#include "Exception.h"

#include <iostream>
#include <cassert>
#include <time.h>

#include <png.h>


#ifndef png_jmpbuf
#   define png_jmpbuf(png_ptr)  ((png_ptr)->jmpbuf)
#endif

// PNG_TRANSFORM_STRIP_FILLER_AFTER was added in libpng 1.2.34
#ifndef PNG_TRANSFORM_STRIP_FILLER_AFTER
#define PNG_TRANSFORM_STRIP_FILLER_AFTER 0x1000
#endif

namespace pcg {
namespace pngio_internal {

	// invGamma is as used in the tone mapper: stored_value = actual_value^invGamma
	template <typename T, ScanLineMode S>
	void Save(const Image<T, S> &img, const char *filename, 
		const bool isSrgb, const float invGamma,
		const int transformFlags = PNG_TRANSFORM_IDENTITY)
	{
		bool err;
		FILE *fp = NULL;
	#if defined(_MSC_VER) && _MSC_VER >= 1500
		err = fopen_s(&fp, filename, "wb") != 0;
	#else
		fp = fopen(filename, "wb");
		err = fp == 0;
	#endif

		if (err) {
			throw IOException("Cannot open the file.");
		}

		// set up writing buffer 
		png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if ( !pngPtr )
		{
			fclose(fp);
			throw RuntimeException("Error: Fail to call png_create_write_struct.");
		}

		// allocate memory for image information
		png_infop infoPtr = png_create_info_struct(pngPtr);
		if ( !infoPtr )
		{
			fclose(fp);
			png_destroy_write_struct(&pngPtr, NULL);
			throw RuntimeException("Error: Fail to call png_create_info_struct.");
		}

		// set up default error handling
		if ( setjmp(png_jmpbuf(pngPtr)) )
		{
			fclose(fp);
			png_destroy_write_struct(&pngPtr, &infoPtr);
			throw RuntimeException("Error: Couldn't setup the error handler.");
		}

		const int bitDepth = sizeof(typename T::pixel_t) * 8;
		if (bitDepth != 8 && bitDepth != 16) {
			fclose(fp);
			png_destroy_write_struct(&pngPtr, &infoPtr);
			throw RuntimeException("Error: Invalid bit depth (only 8 & 16 accepted).");
		}

		// set up the I/O function
		png_init_io(pngPtr, fp);
		png_set_IHDR(pngPtr, infoPtr, img.Width(), img.Height(), bitDepth, 
			PNG_COLOR_TYPE_RGB, 
			PNG_INTERLACE_NONE, 
			PNG_COMPRESSION_TYPE_DEFAULT, 
			PNG_FILTER_TYPE_DEFAULT);

		// Add the info chunks
		{
			if (isSrgb) {
				// From the png book, section 10.6:
				// value 0 for perceptual, 1 for relative colorimetric, 
				// 2 for saturation-preserving, and 3 for absolute colorimetric. 
				png_set_sRGB_gAMA_and_cHRM(pngPtr, infoPtr, PNG_sRGB_INTENT_ABSOLUTE);
			}
			else {
				png_set_gAMA(pngPtr, infoPtr, invGamma);
			}

			time_t modtime = time(NULL);
			png_time pngtime;

			png_convert_from_time_t(&pngtime, modtime);
			png_set_tIME(pngPtr, infoPtr, &pngtime);
		}

		// Setup the scanlines pointers
		png_bytep * rowPtr = new png_bytep[img.Height()];
		assert(rowPtr);
		for(int i = 0; i < img.Height(); ++i) {
			rowPtr[i] = reinterpret_cast<png_byte*>(img.GetScanlinePointer(i, TopDown));
		}
		
		
#if (PNG_LIBPNG_VER >= 10234)

		// Write the whole data, we do the stripping because we always use RGB only
		png_set_rows(pngPtr, infoPtr, rowPtr);
		png_write_png(pngPtr, infoPtr, transformFlags, NULL);

#else 

		// Write the information header
		png_write_info(pngPtr, infoPtr);

		// Ugly compatibility fixes
		if (transformFlags & PNG_TRANSFORM_STRIP_FILLER_AFTER) {
			png_set_filler(pngPtr, 0, PNG_FILLER_AFTER);
		}
		if (transformFlags & PNG_TRANSFORM_SWAP_ENDIAN) {
			png_set_swap(pngPtr);

		}
		if (transformFlags & PNG_TRANSFORM_BGR) {
			png_set_bgr(pngPtr);
		}
		
		png_write_image(pngPtr, rowPtr);
		png_write_end(pngPtr, NULL);

#endif /* PNG_LIBPNG_VER >= 10234 */
		
		// clean up memory and structures
		png_destroy_write_struct(&pngPtr, &infoPtr);
		delete [] rowPtr;

		fclose(fp);

	}

	// Helper functions
	inline bool isLittleEndian()
	{
		const char swapTest[2] = { 1, 0 };
		return (*reinterpret_cast<const short*>(swapTest)) == 1;
	}

}} /* End of private namespace */

void pcg::PngIO::Save(Image<Rgba16,TopDown> &img, 
	const char *filename, const bool isSrgb, const float invGamma)
{
	pcg::pngio_internal::Save(img, filename, isSrgb, invGamma,
		PNG_TRANSFORM_STRIP_FILLER_AFTER | 
		(pcg::pngio_internal::isLittleEndian() ? PNG_TRANSFORM_SWAP_ENDIAN : 0));
}
void pcg::PngIO::Save(Image<Rgba16,BottomUp> &img, 
	const char *filename, const bool isSrgb, const float invGamma)
{
	pcg::pngio_internal::Save(img, filename, isSrgb, invGamma,
		PNG_TRANSFORM_STRIP_FILLER_AFTER | 
		(pcg::pngio_internal::isLittleEndian() ? PNG_TRANSFORM_SWAP_ENDIAN : 0));
}

void pcg::PngIO::Save(Image<Rgba8,TopDown> &img, 
	const char *filename, const bool isSrgb, const float invGamma)
{
	pcg::pngio_internal::Save(img, filename, isSrgb, invGamma,
		PNG_TRANSFORM_STRIP_FILLER_AFTER);
}
void pcg::PngIO::Save(Image<Rgba8,BottomUp> &img, 
	const char *filename, const bool isSrgb, const float invGamma)
{
	pcg::pngio_internal::Save(img, filename, isSrgb, invGamma,
		PNG_TRANSFORM_STRIP_FILLER_AFTER);
}

void pcg::PngIO::Save(Image<Bgra8,TopDown> &img, 
	const char *filename, const bool isSrgb, const float invGamma)
{
	pcg::pngio_internal::Save(img, filename, isSrgb, invGamma,
		PNG_TRANSFORM_BGR | PNG_TRANSFORM_STRIP_FILLER_AFTER);
}
void pcg::PngIO::Save(Image<Bgra8,BottomUp> &img, 
	const char *filename, const bool isSrgb, const float invGamma)
{
	pcg::pngio_internal::Save(img, filename, isSrgb, invGamma,
		PNG_TRANSFORM_BGR | PNG_TRANSFORM_STRIP_FILLER_AFTER);
}
