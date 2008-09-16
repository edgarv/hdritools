// Simple header file with the definitions of some
// common low dynaic range pixel formats

#if !defined(PCG_LDRPIXELS_H)
#define PCG_LDRPIXELS_H

namespace pcg {

	// This is the pixel format used by Qt: When read as a packed int, alpha is the MSB:
	//    pixel = 0xAARRGGBB,
	// thus in the Intel guy, the layout appears kind of inverted!
	struct Bgra8 {
		unsigned char b;
		unsigned char g;
		unsigned char r;
		unsigned char a;

		Bgra8() : b(0),g(0), r(0), a(0xFF) {}

		void set(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF) {
			this->b = b;
			this->g = g;
			this->r = r;
			this->a = a;
		}

	};

	// This is the classic pixel format used by OpenGL
	struct Rgba8 {

		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;

		Rgba8() : r(0), g(0), b(0), a(0xFF) {}

		void set(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF) {
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}

	};


}

#endif  /* PCG_LDRPIXELS_H */
