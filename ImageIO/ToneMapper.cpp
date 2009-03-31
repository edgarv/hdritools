#include "ToneMapper.h"


// Intel Threading Blocks 2.0
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>

// FIXME Make this a setup flag
#define USE_SSE_POW 0

#if USE_SSE_POW
namespace ssemath {
	#include "sse_mathfun.h"
}
#endif

using namespace pcg;
using namespace tbb;

// Global static variables of the tone mapper
const Rgba32F ToneMapper::ZEROS = Rgba32F(0.0f);
const Rgba32F ToneMapper::ONES  = Rgba32F(1.0f);


namespace pcg {
	namespace tonemapper_internal {

		#if _WIN32||_WIN64
		// define the parts of stdint.h that are needed
		typedef __int8 int8_t;
		typedef __int16 int16_t;
		typedef __int32 int32_t;
		typedef __int64 int64_t;
		typedef unsigned __int8 uint8_t;
		typedef unsigned __int16 uint16_t;
		typedef unsigned __int32 uint32_t;
		typedef unsigned __int64 uint64_t;
		#else
		#include <stdint.h>
		#endif


		// Helper class for the TBB version of the LUT update
		template < bool useSRGB >
		class ToneMapperLUTBody {

		protected:

			// 1/gamma
			const float invGamma;

			// Pointer to the lut
			uint32_t *lutPtr;

			// References to the helper constants
			const Rgba32F &base4;
			const Rgba32F &delta;
			const Rgba32F &qFactor;

			// Constants used in sRGB
			const Rgba32F srgbThreshold; // 0.0031308f
			const Rgba32F srgbLowScale;  // 12.92f
			const Rgba32F srgb1plusA;	 // 1.0f + 0.055f
			const Rgba32F srgbA;		 // 0.055f

			// Helper method for the LUT kernels
			FORCEINLINE_BEG void quantizeAndStore(const int i, 
				const Rgba32F &vals, const Rgba32F &qFactor) const FORCEINLINE_END
			{
				// Multiply by 255
				const Rgba32F values = vals * qFactor;
				
				// Convert back to integers with rounding
				const __m128i valuesI = _mm_cvtps_epi32(values);

				// Copy to the LUT. Each value is only 8 bits long, so we can extract them using SSE2
				const int A = _mm_extract_epi16(valuesI, 3*2);
				const int B = _mm_extract_epi16(valuesI, 2*2);
				const int C = _mm_extract_epi16(valuesI, 1*2);
				const int D = _mm_extract_epi16(valuesI, 0);

				// Combine everything in place. Because of using the previous intrinsics, this code
				// can run entirely on registers, doing a single memory copy
				lutPtr[i] = (D << 24) | (C << 16) | (B << 8) | A;

			}

			// Computes the LUT values, 4 at a time
			inline void kernel(const int &i, 
				const Rgba32F &base4, const Rgba32F &delta, const Rgba32F &qFactor) const {

				// Update values
				Rgba32F values = Rgba32F((float)(i))*base4 + delta;

				// Exponentiate by 1/gamma (this is slow!!!)
#if USE_SSE_POW
				values = ssemath::exp_ps(ssemath::log_ps(values) * Rgba32F(invGamma));
#else
				values.set(pow(values.r(), invGamma), pow(values.g(), invGamma), 
						   pow(values.b(), invGamma), pow(values.a(), invGamma));
#endif

				quantizeAndStore(i, values, qFactor);
			}

			// Computes the LUT for sRGB
			inline void kernelSRGB(const int &i, 
				const Rgba32F &base4, const Rgba32F &delta, const Rgba32F &qFactor) const
			{
				// Update the linear values
				Rgba32F values = Rgba32F((float)(i))*base4 + delta;

				// Gets a maks (filled with 0xffffffff where values[i] < threshold (0.0031308)
				const Rgba32F mask = _mm_cmple_ps(values, srgbThreshold);

				// First case result: when its less than the threshold
				const Rgba32F below = srgbLowScale * values;

				// Second case:
#if USE_SSE_POW
				Rgba32F above(ssemath::exp_ps(ssemath::log_ps(values) * Rgba32F(1.0f/2.4f)));
#else
				Rgba32F above(pow(values.r(), 1.0f/2.4f), pow(values.g(), 1.0f/2.4f),
					pow(values.b(), 1.0f/2.4f), pow(values.a(), 1.0f/2.4f));
#endif
				above *= srgb1plusA;
				above -= srgbA;

				// Select values according to the mask (to avoid conditionals)
				values = (mask & below) | (_mm_andnot_ps(mask, above));

				
				quantizeAndStore(i, values, qFactor);

			}

		public:

			ToneMapperLUTBody(uint32_t *lutPtr, 
				const Rgba32F &_base4, const Rgba32F &_delta, const Rgba32F &_qFactor,
				float invGamma = 0.0f) :
			  invGamma(invGamma), lutPtr(lutPtr), base4(_base4), delta(_delta), qFactor(_qFactor),
			  srgbThreshold(0.0031308f), srgbLowScale(12.92f), srgb1plusA(1.0f+0.055f), srgbA(0.055f)
			{

			}

			// The actual method to execute. Because useSRGB is a template, the generated
			// code won't have conditionals
			void operator()(const blocked_range<int>& r) const {

				// Local copies of the variables
				const Rgba32F base4   = this->base4;
				const Rgba32F delta   = this->delta;
				const Rgba32F qFactor = this->qFactor;

				if (!useSRGB) {
					for (int i = r.begin(); i != r.end(); ++i) {
						kernel(i, base4, delta, qFactor);
					}
				}
				else {
					for (int i = r.begin(); i != r.end(); ++i) {
						kernelSRGB(i, base4, delta, qFactor);
					}
				}
			}

                        /*
                        void operator()(int s, int t) const {
				// Local copies of the variables
				const Rgba32F base4   = this->base4;
				const Rgba32F delta   = this->delta;
				const Rgba32F qFactor = this->qFactor;

				if (!useSRGB) {
					for(int i = s; i < t; ++ i) {
						kernel(i, base4, delta, qFactor);
					}
				}
				else {
                                        for(int i = s;i < t; ++ i) {
						kernelSRGB(i, base4, delta, qFactor);
					}
				}
                        }
                        */
		};

		// The class to be used by TBB
		template <class T, ScanLineMode S1, ScanLineMode S2/* = S1*/>
		class ApplyToneMap {

			// Pointer to the destination Image
			Image<T,S1> &dest;

			// Pointer to the source image
			const Image<Rgba32F, S2> &src;

			// Pointer to the tone mapper to use
			const ToneMapper &tm;

			// The exponent factor <2^exposure>{4}
			const Rgba32F expF;

			// Read-only pointer to the tone mapper's LUT
			const unsigned char *lut;


			// This is the real meat of the tone mapping operation. It receives
			// all the inputs as references so that it can be fully inlined
			// by the compiler. For this to work the type T must have a method:
			//   T.set(unsigned char r, unsigned char g, unsigned char b)
			void ToneMapKernel(const Rgba32F &srcPix, T &destPix, const Rgba32F &expF,
				const __m128 &ones, const __m128 &zeros, const __m128 &lutQ) const {


				// Applies the exposure correction (note that we don't care about the alpha!)
				Rgba32F pix = srcPix * expF;

				// Clamp the values to the interval [0,1]
				pix = _mm_min_ps(ones, _mm_max_ps(zeros, pix));

				// Prepares for querying the LUT: multiplies by the (LUT size-1) and
				// converts to integer with rounding
				pix *= lutQ;
				const __m128i pixIndices = _mm_cvtps_epi32(pix);

				// After this conversion, each value is in the range [0, LUT size), which
				// by design is a 16 byte unsigned integer. Because of that we don't need
				// SSE4 to extract the elements from there, we can use the SSE2 _mm_extract_epi16
				// intrinsic to extract each element independently and query the lut from there
				// (remember, the indices refer to 16-bit integers!)
				const int indexR = _mm_extract_epi16(pixIndices, 3*2);
				const int indexG = _mm_extract_epi16(pixIndices, 2*2);
				const int indexB = _mm_extract_epi16(pixIndices, 1*2);

				// Finally sets the pixel values using the LUT
				destPix.set(lut[indexR], lut[indexG], lut[indexB]);
			}


		public:

			// Default constructor: initializes all the pointers and the exposure factor vector
			ApplyToneMap(Image<T,S1> &dest, const Image<Rgba32F, S2> &src, const ToneMapper &tm) :
			  dest(dest), src(src), tm(tm), expF(tm.exposureFactor), lut(tm.lut) {}

			// Linear-style operator. This should only be used on files using the same scanline mode
			void operator()(const blocked_range<int>& r) const {

				// Local copies of the variables
				const __m128 ones  = pcg::ToneMapper::ONES;
				const __m128 zeros = pcg::ToneMapper::ZEROS;
				const __m128 lutQ  = _mm_set1_ps((float)(tm.lutSize-1));
				const Rgba32F expF = this->expF;

				for (int i = r.begin(); i != r.end(); ++i) {
					ToneMapKernel(src[i], dest[i], expF, ones, zeros, lutQ);
				}
			}

			// 2D style operator. This guy is safe no mather which scanline mode is used
			void operator()(const blocked_range2d<int>& r) const {

				// Local copies of the variables
				const __m128 ones  = pcg::ToneMapper::ONES;
				const __m128 zeros = pcg::ToneMapper::ZEROS;
				const __m128 lutQ  = _mm_set1_ps((float)(tm.lutSize-1));
				const Rgba32F expF = this->expF;


				for (int j = r.rows().begin(); j != r.rows().end(); ++j) {
					for (int i = r.cols().begin(); i != r.cols().end(); ++i) {
						ToneMapKernel(src.ElementAt(i,j, dest.GetMode()), dest.ElementAt(i,j), 
							expF, ones, zeros, lutQ);
					}
				}

			}

		};


		// Tones map the given source image and stores the result in the given destination image.
		// the images must have the same size!
		// This template gets instanciated when both images have the same scanline order
		template<class T, ScanLineMode M>
		void ToneMap(Image<T, M> &dest, const Image<Rgba32F, M> &src, const ToneMapper &tm) {

			if (dest.Width() != src.Width() || dest.Height() != dest.Height()) {
				throw IllegalArgumentException("The images dimensions' don't match");
			}			

			const int numPixels = src.Size();
			parallel_for(blocked_range<int>(0,numPixels), ApplyToneMap<T,M>(dest, src, tm), auto_partitioner());
		}

		// The method to use when whe scanline order is different
		template<class T, ScanLineMode M1, ScanLineMode M2>
		void ToneMap(Image<T, M1> &dest, const Image<Rgba32F, M2> &src, const ToneMapper &tm) {

			if (dest.Width() != src.Width() || dest.Height() != dest.Height()) {
				throw IllegalArgumentException("The images dimensions' don't match");
			}

			parallel_for(blocked_range2d<int>(0,src.Height(), 0,src.Width()), 
				ApplyToneMap<T,M1,M2>(dest, src, tm), auto_partitioner());
		}

	}
}

using namespace pcg::tonemapper_internal;


void ToneMapper::UpdateLUT() {

	const float stepSize = 1.0f/lutSize;
	const float halfStep = stepSize/2.0f;

	// At each group i, we wil fill the LUT for (i*base+delta), i = 0,4,8,...
	const Rgba32F base4(4.0f * stepSize);
	const Rgba32F delta(halfStep, halfStep + stepSize, halfStep + 2*stepSize, halfStep + 3*stepSize);
	const Rgba32F qFactor(255.0f);

	// With some SSE trickery we will be running in 4 elements at a time, that's the
	// reason for the LUT to be a multiple of 4
	const int numIter = lutSize >> 2;
	// Alias the LUT: we will write quadwords at a time
	uint32_t *lutPtr = reinterpret_cast<uint32_t *>(lut);

	// Instanciate the proper templates
	if (isSRGB()) {
		parallel_for(blocked_range<int>(0,numIter),
			ToneMapperLUTBody<true>(lutPtr, base4, delta, qFactor) );
	}
	else {
		parallel_for(blocked_range<int>(0,numIter),
			ToneMapperLUTBody<false>(lutPtr, base4, delta, qFactor, invGamma) );

		// However the first one is always 0
		lut[0] = 0;
	}

}


// #################################################
// # Instanciate the templates to get the real stuff
// #################################################

// Bgra8 pixels
void ToneMapper::ToneMap(Image<Bgra8, TopDown> &dest,  const Image<Rgba32F, TopDown> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}
void ToneMapper::ToneMap(Image<Bgra8, TopDown> &dest,  const Image<Rgba32F, BottomUp> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}
void ToneMapper::ToneMap(Image<Bgra8, BottomUp> &dest, const Image<Rgba32F, TopDown> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}
void ToneMapper::ToneMap(Image<Bgra8, BottomUp> &dest, const Image<Rgba32F, BottomUp> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}

// Rgba8 pixels
void ToneMapper::ToneMap(Image<Rgba8, TopDown> &dest,  const Image<Rgba32F, TopDown> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}
void ToneMapper::ToneMap(Image<Rgba8, TopDown> &dest,  const Image<Rgba32F, BottomUp> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}
void ToneMapper::ToneMap(Image<Rgba8, BottomUp> &dest, const Image<Rgba32F, TopDown> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}
void ToneMapper::ToneMap(Image<Rgba8, BottomUp> &dest, const Image<Rgba32F, BottomUp> &src) const {
	pcg::tonemapper_internal::ToneMap(dest, src, *this);
}
