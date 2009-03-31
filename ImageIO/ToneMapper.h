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
#include "Exception.h"

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

		// Exposure of the image
		float exposure;

		// Exposure compensation: 2^exposure
		float exposureFactor;

		// Gamma correction
		float gamma;

		// The actual exponent for the gamma correction: 1/gamma
		float invGamma;

		// A flag to know whether to use the simple gamma curve or the sRGB color space
		bool useSRGB;

		// The lut contains the values of:  round(255 * (p^(1/gamma))),
		// where p is the pixel value after applying the exposure compensation
		// and whose value is in the range [0,1]
		void IMAGEIO_API UpdateLUT();


	public:

		// Creates a new Tone Mapper instance which will use a LUT of the given size to speed
		// up the query of the gamma exponentiation. That LUT will be forced to have a size
		// multiple of four, and of course if it's lenght is ridiculously small everything
		// is likely to crash in terrible ways.
		ToneMapper(unsigned short size = 2048) : lut(NULL), lutSize(size & (~0x3)), 
			useSRGB(false) 
		{
			lut = new unsigned char[lutSize];
			SetExposure(0.0f);
			SetGamma(1.0f);
		}

		// Constructor which takes an specific exposure, and uses sRGB instead of a gamma curve.
		// If sRGB is disabled afterwards, the default gamma is 2.2
		ToneMapper(float exposure, unsigned short size) : lut(NULL), lutSize(size & (~0x3)),
			useSRGB(true),
			gamma(2.2f), invGamma(1.0f/2.2f)
		{
			if (lutSize == 0) {
				throw IllegalArgumentException("Illegal LUT size of 0");
			}
			lut = new unsigned char[lutSize];
			SetExposure(exposure);
			UpdateLUT();
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
		// Therefore gamma must be greater than zero; the typical value for LCD's is 2.2.
		// Calling this method implies disabling sRGB.
		void SetGamma(float gamma) {
			if (gamma <= 0) {
				throw IllegalArgumentException("The gamma must be greater that zero");
			}
			this->gamma    = gamma;
			this->invGamma = 1.0f/gamma;
			this->useSRGB  = false;
			UpdateLUT();
		}

		// Enables or disables the sRGB curve
		void SetSRGB(bool enable) {
			useSRGB = enable;
			UpdateLUT();
		}

		// Returns the gamma employed when sRGB is not used
		float Gamma() const { return gamma; }

		// Returns the exposure
		float Exposure() const { return exposure; }

		// Returns wheather it's using sRGB or nor
		bool isSRGB() const { return useSRGB; }


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
