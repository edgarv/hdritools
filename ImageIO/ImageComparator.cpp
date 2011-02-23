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

#include "ImageComparator.h"
#include "Exception.h"


// Intel Threading Blocks 2.0
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

using namespace pcg;
using namespace tbb;

// SSE3 functions are only available as intrinsic in MSVC
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic ( _mm_hadd_ps )
#else
#include <pmmintrin.h>
#endif /* _MSC_VER */


namespace pcg {
	namespace comparator_internal {

		// A class that we will use for the TBB implementation
		template <ScanLineMode S>
		class Comparator {
			
		private:
			const Image<Rgba32F, S> &src1;
			const Image<Rgba32F, S> &src2;
			Image<Rgba32F, S> &dest;

			const ImageComparator::Type type;

			// The different kernels, one for each comparison type

			// Takes the absolute value of the difference of the pixels
			inline void kernel_absoluteDiff(const int &i) const {

				dest[i] = Rgba32F::abs(src1[i] - src2[i]);
			}

			// Not actually a comparision but a combination: just adds both pixels
			inline void kernel_addition(const int &i) const {
				dest[i] = src1[i] + src2[i];
			}

			// Divides the first source by the second one
			inline void kernel_division(const int &i) const {
				// TODO what if both are zero? what if src2[i] is almost zero?
				dest[i] = src1[i] / src2[i];
			}

			// Error relative to the adition of both images
			inline void kernel_relError(const int &i) const {
				dest[i] = Rgba32F(2.0f) * Rgba32F::abs(src1[i] - src2[i]) / (src1[i] + src2[i]);
			}

			// Helper method to add the four elements of a vector and have the result in each section
			FORCEINLINE_BEG __m128 hadd(const __m128 &n) const FORCEINLINE_END {
				// FIXME Use the non-SSE3 equivalent of hadd_ps
				const __m128 r = _mm_hadd_ps(n,n);
				return _mm_hadd_ps(r,r);

#if 0
				// SSE2 version by Chris Walton; http://www.kvraudio.com/forum/viewtopic.php?p=2826733
				__m128 i = n;
				__m128   t;
				t = _mm_movehl_ps(t, i);
				i = _mm_add_ps(i, t);
				t = _mm_shuffle_ps(i, i, 0x55);
				i = _mm_add_ps(i, t);
				return i; 
#endif
			}


			// Helper function for the 2-norm comparisons
			FORCEINLINE_BEG void kernel_2norm(const Rgba32F &val, const int &i, 
				const Rgba32F &alphaKillMask) const FORCEINLINE_END
			{	
				const Rgba32F v = val & alphaKillMask;

				// Use SSE3 to get the horizontal add
				Rgba32F dSqr = v*v;
				dSqr = hadd(dSqr);

				// TODO Use all square roots and then masks, or a single root and shuffle?
				const Rgba32F d = _mm_sqrt_ps(dSqr);

				const Rgba32F pn = ((Rgba32F) hadd(v)) & alphaKillMask;
				const Rgba32F zero = _mm_setzero_ps();

				// Part 1: pn < 0 (just r)
				Rgba32F m1 = _mm_cmplt_ps(pn, zero);
				m1 = _mm_shuffle_ps((__m128)m1, (__m128)m1, _MM_SHUFFLE(3, 0, 0, 0));

				// Part 2: pn > 0 (just g)
				Rgba32F m2 = _mm_cmpgt_ps(pn, zero);
				m2 = _mm_shuffle_ps((__m128)m2, (__m128)m2, _MM_SHUFFLE(0, 2, 0, 0));

				// Part 3: pn == 0 (just b)
				Rgba32F m3 = _mm_cmpeq_ps(pn, zero);
				m3 = _mm_shuffle_ps((__m128)m3, (__m128)zero, _MM_SHUFFLE(0, 0, 1, 0));

				// And finally combine everything
				// TODO: do something useful with the alpha
				dest[i] = d & (m1 | m2 | m3);
			}

			// 2-norm of the rgb diference
			inline void kernel_posNeg(const int &i, const Rgba32F &alphaKillMask) const {

				const Rgba32F delta = (src1[i] - src2[i]);
				kernel_2norm(delta, i, alphaKillMask);
			}

			inline void kernel_posNegRel(const int &i, const Rgba32F &alphaKillMask) const {

				const Rgba32F diff = Rgba32F(2.0f) * (src1[i] - src2[i]) / (src1[i] + src2[i]);
				kernel_2norm(diff, i, alphaKillMask);
			}

		public:
			Comparator(ImageComparator::Type type, Image<Rgba32F, S> &dest, 
				const Image<Rgba32F, S> &src1, const Image<Rgba32F, S> &src2) :
				src1(src1), src2(src2), dest(dest), type(type) {}

			// Linear-style operator (one pixel after the other)
			void operator()(const blocked_range<int>& r) const {
				const __m128i alphaKillInt = _mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x0);
				const Rgba32F alphaKill = _mm_castsi128_ps(alphaKillInt);

				// TODO Use templates to remove the conditional at compile-time
				switch (type) {
					case ImageComparator::AbsoluteDifference:
						for (int i = r.begin(); i != r.end(); ++i) {
							kernel_absoluteDiff(i);
						}
						break;

					case ImageComparator::Addition:
						for (int i = r.begin(); i != r.end(); ++i) {
							kernel_addition(i);
						}
						break;

					case ImageComparator::Division:
						for (int i = r.begin(); i != r.end(); ++i) {
							kernel_division(i);
						}
						break;

					case ImageComparator::RelativeError:
						for (int i = r.begin(); i != r.end(); ++i) {
							kernel_relError(i);
						}
						break;

					case ImageComparator::PositiveNegative:
						for (int i = r.begin(); i != r.end(); ++i) {
							kernel_posNeg(i, alphaKill);
						}
						break;

					case ImageComparator::PositiveNegativeRelativeError:
						for (int i = r.begin(); i != r.end(); ++i) {
							kernel_posNegRel(i, alphaKill);
						}
						break;


					default:
						assert(0);
						break;
				}

			}
		};

	}
}

template <ScanLineMode S>
void ImageComparator::CompareHelper(Type type, Image<Rgba32F, S> &dest, 
			const Image<Rgba32F, S> &src1, const Image<Rgba32F, S> &src2)
{
	// First check that the images have the save size, otherwise it will throw
	// a nasty exception
	if ( dest.Width() != src1.Width() || dest.Height() != src1.Height() ||
		src1.Width() != src2.Width() || src1.Height() != src2.Height() )
	{
		throw IllegalArgumentException("Incompatible images size");
	}

	// And launch the parallel for
	const int numPixels = dest.Size();
	parallel_for(blocked_range<int>(0,numPixels), 
		pcg::comparator_internal::Comparator<S>(type, dest, src1, src2),
		auto_partitioner());
}

// The real instances of the template
void ImageComparator::Compare(Type type, Image<Rgba32F, TopDown> &dest, 
			const Image<Rgba32F, TopDown> &src1, const Image<Rgba32F, TopDown> &src2) {
	CompareHelper(type, dest, src1, src2);
}
void ImageComparator::Compare(Type type, Image<Rgba32F, BottomUp> &dest, 
			const Image<Rgba32F, BottomUp> &src1, const Image<Rgba32F, BottomUp> &src2) {
	CompareHelper(type, dest, src1, src2);
}
