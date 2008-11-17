#include "ImageComparator.h"
#include "Exception.h"


// Intel Threading Blocks 2.0
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

using namespace pcg;
using namespace tbb;

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

		public:
			Comparator(ImageComparator::Type type, Image<Rgba32F, S> &dest, 
				const Image<Rgba32F, S> &src1, const Image<Rgba32F, S> &src2) :
				type(type), dest(dest), src1(src1), src2(src2) {}

			// Linear-style operator (one pixel after the other)
			void operator()(const blocked_range<int>& r) const {

				switch (type) {
					case ImageComparator::AbsoluteDifference:
						for (int i = r.begin(); i != r.end(); ++i) {
							kernel_absoluteDiff(i);
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
