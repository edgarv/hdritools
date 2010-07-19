/**
 * A templated image, it's heavily based in Fabio's far codebase
 */

#if !defined( PCG_IMAGE_H )
#define PCG_IMAGE_H

#define IMAGE_CHECKBOUNDS

// Internal flag to know whether to use an older version of the code
// with template specializations. Otherwise a much simpler version
// will be used. Why isn't this removed at all? The old code remains
// as an example of template specialization
#ifndef PCG_IMAGE_CRAZY_TEMPLATES
	#define PCG_IMAGE_CRAZY_TEMPLATES 0
#endif

#include <cstdlib>
#include <cassert>

#include "Exception.h"

namespace pcg {

	// An enumeration for the scanline ordering
	enum ScanLineMode {
		// The first pixel is at the top-left corner (alla DirectX, EXR)
		TopDown,
		
		// The first pixel is at the bottom-left corner (alla OpenGL)
		BottomUp
	};

	template< typename T, ScanLineMode S = TopDown >
	class Image {

#if PCG_IMAGE_CRAZY_TEMPLATES
	private:

		// Specialized templates for returning a pointer to a scanline
		// according to an specific order.
		// Why to do this instead of an extra if? To get slightly better performance,
		// as only the appropriate codepath is taken, also to keep the actual
		// Image code smaller and clear, and as a programming excercise.
		template <ScanLineMode G> class ScanLineGetter;

		// Getter for TopDown images
		template <> class ScanLineGetter<TopDown> {
		public:
			static T* GetScanline(int j, ScanLineMode mode, const Image *img) {
				switch(mode) {
					case TopDown:
						return &(img->d[j * img->w]);
						break;
					case BottomUp:
						return &(img->d[(img->h - j - 1) * img->w]);
						break;
					default:
						assert(0);
						return NULL;	// Make the compiler happy
						break;
				}
			}
			static T& ElementAt(int i, int j, ScanLineMode mode, const Image *img) {
				switch(mode) {
					case TopDown:
						return img->d[img->w*j+i];
						break;
					case BottomUp:
						return img->d[(img->h-j-1)*img->w+i];
						break;
					default:
						assert(0);
						return img->d[0];	// Make the compiler happy
						break;
				}
			}
		};

		// Getter for BottomUp images
		template <> class ScanLineGetter<BottomUp> {
		public:
			static T* GetScanline(int j, ScanLineMode mode, const Image *img) {
				switch(mode) {
					case BottomUp:
						return &(img->d[j * img->w]);
						break;
					case TopDown:
						return &(img->d[(img->h - j - 1) * img->w]);
						break;
					default:
						assert(0);
						return NULL;	// Make the compiler happy
						break;
				}
			}
			static T& ElementAt(int i, int j, ScanLineMode mode, const Image *img) {
				switch(mode) {
					case BottomUp:
						return img->d[img->w*j+i];
						break;
					case TopDown:
						return img->d[(img->h-j-1)*img->w+i];
						break;
					default:
						assert(0);
						return img->d[0];	// Make the compiler happy
						break;
				}
			}
		};

#endif /* PCG_IMAGE_CRAZY_TEMPLATES */

		// Helper function to allocate the memory. It assumes that the alignment
        // is provided by T::operator new[] (size_t)
		inline void alloc() {
			this->d = new T[w*h];
            if (this->d == NULL) {
                throw RuntimeException("Couldn't allocate the memory "
                    "for the image");
            }
		}

	public:


		// Default constructor: creates an empty image. To do anything
		// useful afterwards you need to use the Alloc(int,int) method.
		Image() : w(0), h(0), d(NULL), mode(S) {
		}

		// Creates a new image allocating the required space
		Image(int w, int h) : mode(S) {
			assert(w > 0 && h > 0);
            assert(((long long)w)*h <= 0x7fffffff);
			this->w = w;
			this->h = h;
			alloc();
		}

		// Destructor, it reclaims the space previously allocated
		~Image() {
			Clear();
		}

		// Allocates new space for the image data, deleting the previous one
		void Alloc(int w, int h) {
			assert(w > 0 && h > 0);
			Clear();
			this->w = w;
			this->h = h;
			alloc();
		}

		// Deallocates the memory and resets the image dimensions to 0
		void Clear() {
			if(d != NULL) {
				delete [] d;
			}
			d = 0;
			w = h = 0;
		}

		// Returns a reference to the i-th pixel in the j-th column (indices are zero-based)
		// according to the scanline order of the image.
		T& ElementAt(int i, int j, ScanLineMode mode = S) const {
			assert(w*j+i >=0 && w*j+i < w*h && j >= 0 && j < h && i >=0 && i < w);
#if PCG_IMAGE_CRAZY_TEMPLATES
			return ScanLineGetter<S>::ElementAt(i, j, mode, this);
#else
            return (mode == S) ? d[w*j + i] : d[(h-j-1)*w + i];
#endif
		}

		// Returns a reference to the idx-th pixel of the image, which are in scanline order
		// according to the specified mode.
		T& ElementAt(int idx) const {
			assert(idx >=0 && idx < w*h);
			return d[idx];
		}

		// Writes in the i,j parameters the coordinates necessary to access the idx-pixel in the
		// image using ElementAt(int,int), according to the scanline order of the image.
		void GetIndices(int idx, int &i, int &j) const { 
			i = idx % w; 
			j = idx / w; 
		}
		
		// Returns the index (zero based) of the i-th pixel at the j-th scanline using the
		// scanline order of the image.
		int GetIndex(int i, int j) const { return w*j+i; }

		// Width of the image
		int Width()  const { return w; }

		// Height of the image
		int Height() const { return h; }

		// Number of pixels in the image (Width*Height)
		int Size()   const { return w*h; }

		// Raw pointer to the pixels
		T* GetDataPointer() const { return d; }

		// Returns a reference to the idx-th pixel, as in ElementAt(int)
		T& operator[](int idx) {
			assert(idx >=0 && idx < w*h);
			return d[idx];
		}

		// Returns a const reference to the idx-th pixel, as in ElementAt(int)
		const T& operator[](int idx) const {
			assert(idx >=0 && idx < w*h);
			return d[idx];
		}

		// Provides access to the scanline mode of the image
		ScanLineMode GetMode() const { return S; }

		// Gets a pointer to the beginning of the j-th scanline in the specified mode.
		// By default the mode is the same of the image. You should not use data through
		// this pointer for more than a scanline! Instead get a new pointer to the next
		// scanline using this method
		T* GetScanlinePointer(int j, ScanLineMode mode = S) const {
			assert(j >= 0 && j < h);
			
#if PCG_IMAGE_CRAZY_TEMPLATES
			return ScanLineGetter<S>::GetScanline(j, mode, this);
#else
			// We only have two modes, so this works nicely
            return (mode == S) ? &(d[j * w]) : &(d[(h - j - 1) * w]);
#endif
		}


	protected:
		// Pointer to the raw data
		T* d;

		// Width of the image
		int w;

		// Height of the image
		int h;

		// The scanline mode of this image
		const ScanLineMode mode;
	};

}

#endif /* PCG_IMAGE_H */
