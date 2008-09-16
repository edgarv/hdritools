// A simple tone mapper class, using a LUT for fast exponentiation,
// inspired on the original glimage code.
// To keep things simple, this guy only know how to tone map Rgba32F images

#if !defined(PCG_TONEMAPPER_H)
#define PCG_TONEMAPPER_H

#include <cmath>
#include <emmintrin.h>

#include "Image.h"
#include "Rgba32F.h"
#include "LDRPixels.h"
#include "ImageIO.h"

namespace pcg {

	// Forward declaration
	namespace tonemapper_internal {
		template <class T, ScanLineMode S1, ScanLineMode S2 = S1>
		class ApplyToneMap;
	}

	class ToneMapper {

		template <class T, ScanLineMode S1, ScanLineMode>
		friend class tonemapper_internal::ApplyToneMap;

	protected:

		// Utility vector for the clamping
		const static IMAGEIO_API Rgba32F ZEROS;
		const static IMAGEIO_API Rgba32F ONES;

		// Pointer to the lookup table
		unsigned char *lut;

		// Number of elements in the LUT. It must be a multiple of four!
		const unsigned short lutSize;

		// A vector which will hold the value lutSize-1, for quantization before querying the LUT
		const Rgba32F qLutV;

		// Exposure of the image
		float exposure;

		// Exposure compensation: 2^exposure
		float exposureFactor;

		// Gamma correction
		float gamma;

		// The actual exponent for the gamma correction: 1/gamma
		float invGamma;

		// The lut contains the values of:  round(255 * (p^(1/gamma))),
		// where p is the pixel value after applying the exposure compensation
		// and whose value is in the range [0,1]
		void IMAGEIO_API UpdateLUT();


	public:

		// Creates a new Tone Mapper instance which will use a LUT of the given size to speed
		// up the query of the gamma exponentiation. That LUT will be forced to have a size
		// multiple of four, and of course if it's lenght is ridiculously small everything
		// is likely to crash in terrible ways.
		ToneMapper(unsigned short size = 2048) : lut(NULL), lutSize(size & (~0x3)), qLutV((float)lutSize-1) {

			lut = new unsigned char[lutSize];
			SetExposure(0.0f);
			SetGamma(1.0f);
		}

		// The destructor only deletes the LUT
		~ToneMapper() {
			if (lut != NULL) {
				delete [] lut;
			}
		}

		// Sets the exposure. Each pixel will be scaled by 2^exposure prior to gamma correction
		void SetExposure(float exposure) {
			this->exposure = exposure;
			this->exposureFactor = pow(2.0f, exposure);
		}

		// Sets the gamma correction. Each pixel's component p, after the exposure correction and clamping to
		// the range [0,1] will be raised to the power of (1/gamma), thus the final value is p^(1/gamma).
		// Therefore gamma must be greater than zero; the typical value for LCD's is 2.2
		void SetGamma(float gamma) {
			if (gamma <= 0) {
				throw std::exception("Something terrible happens!");
			}
			this->gamma    = gamma;
			this->invGamma = 1.0f/gamma;
			UpdateLUT();
		}

		// Returns the gamma
		float Gamma() const { return gamma; }

		// Returns the exposure
		float Exposure() const { return exposure; }


		// The real tone mapping operations: support for the two types
		// of LDR pixels, in the different type of scanline orders
		void IMAGEIO_API ToneMap(Image<Bgra8, TopDown> &dest,  const Image<Rgba32F, TopDown> &src) const;
		void IMAGEIO_API ToneMap(Image<Bgra8, TopDown> &dest,  const Image<Rgba32F, BottomUp> &src) const;
		void IMAGEIO_API ToneMap(Image<Bgra8, BottomUp> &dest, const Image<Rgba32F, TopDown> &src) const;
		void IMAGEIO_API ToneMap(Image<Bgra8, BottomUp> &dest, const Image<Rgba32F, BottomUp> &src) const;

		void IMAGEIO_API ToneMap(Image<Rgba8, TopDown> &dest,  const Image<Rgba32F, TopDown> &src) const;
		void IMAGEIO_API ToneMap(Image<Rgba8, TopDown> &dest,  const Image<Rgba32F, BottomUp> &src) const;
		void IMAGEIO_API ToneMap(Image<Rgba8, BottomUp> &dest, const Image<Rgba32F, TopDown> &src) const;
		void IMAGEIO_API ToneMap(Image<Rgba8, BottomUp> &dest, const Image<Rgba32F, BottomUp> &src) const;

	};
}


#endif /* PCG_TONEMAPPER_H */
