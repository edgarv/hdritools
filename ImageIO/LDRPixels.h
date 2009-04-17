// Simple header file with the definitions of some
// common low dynaic range pixel formats

#if !defined(PCG_LDRPIXELS_H)
#define PCG_LDRPIXELS_H

namespace pcg {

	// This is the pixel format used by Qt: When read as a packed int, alpha is the MSB:
	//    pixel = 0xAARRGGBB,
	// thus in the Intel guy, the layout appears kind of inverted!
	struct Bgra8 {

		typedef unsigned char pixel_t;

		pixel_t b;
		pixel_t g;
		pixel_t r;
		pixel_t a;

		Bgra8() : b(0),g(0), r(0), a(0xFF) {}

		void set(pixel_t r, pixel_t g, pixel_t b, pixel_t a = 0xFF) {
			this->b = b;
			this->g = g;
			this->r = r;
			this->a = a;
		}

	};

	// This is the classic pixel format used by OpenGL
	struct Rgba8 {

		typedef unsigned char pixel_t;

		pixel_t r;
		pixel_t g;
		pixel_t b;
		pixel_t a;

		Rgba8() : r(0), g(0), b(0), a(0xFF) {}

		void set(pixel_t r, pixel_t g, pixel_t b, pixel_t a = 0xFF) {
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}

	};


	// Pixels for saving in 16-bit LDR formats such as PNG (without alpha)
	struct Rgba16 {

		typedef unsigned short pixel_t;
		
		pixel_t r;
		pixel_t g;
		pixel_t b;
		pixel_t a;

		Rgba16() : r(0), g(0), b(0), a(0xFFFF) {}

		void set(pixel_t r, pixel_t g, pixel_t b, pixel_t a = 0xFFFF) {
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}

	};

}

#endif  /* PCG_LDRPIXELS_H */
