// A simple interface for floating point pixels which only have the RGB channels

#if !defined (PCG_RGB32F_H)
#define PCG_RGB32F_H

using namespace std;

#include "Rgba32F.h"

namespace pcg {

	struct Rgb32F {

		float r;
		float g;
		float b;

		/* ########### Constructors ########### */

		// Boring default constructor: sets a null pixel
		Rgb32F() : r(0.0f), g(0.0f), b(0.0f) {}

		// Explicit initialization from components
		Rgb32F(const float r, const float g, const float b) : 
			r(r), g(g), b(b) {}

		// Explicit copy constructor
		Rgb32F(const Rgb32F &pixel) {
			r = pixel.r;
			g = pixel.g;
			b = pixel.b;
		}

		// Construct from a full float pixel
		Rgb32F(const Rgba32F &pixel) {
			Rgba32F tmp = pixel;
			tmp.applyAlpha();
			set(tmp.r(), tmp.g(), tmp.b());
		}

		/* ########### An explicit setter ########### */
		void set(const float r, const float g, const float b) {
			this->r = r;
			this->g = g;
			this->b = b;
		}



		/* ########### Operators ########### */

		// Some applications require accessing the data as an array of float.
		// Be careful of the ordering!!
		//   result[0] = r
		//   result[1] = g
		//   result[2] = b
		operator const float* () const 		{
			return reinterpret_cast<const float*>(this);
		}
	};


}


#endif /* PCG_RGB32F_H */
