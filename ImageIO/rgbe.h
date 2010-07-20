#ifndef PCG_RGBE_H
#define PCG_RGBE_H


/* THIS CODE CARRIES NO GUARANTEE OF USABILITY OR FITNESS FOR ANY PURPOSE.
 * WHILE THE AUTHORS HAVE TRIED TO ENSURE THE PROGRAM WORKS CORRECTLY,
 * IT IS STRICTLY USE AT YOUR OWN RISK.  */

/* utility for reading and writing Ward's rgbe image format.
   See rgbe.txt file for more details.
*/

#include "StdAfx.h"
#include "Rgba32F.h"
#include "Rgb32F.h"
#include "ImageIO.h"
#include "Image.h"

namespace pcg {

	// RGBE enconded pixel in Watts/sr/m^2
	struct Rgbe {
			
		unsigned char r;
		unsigned char g;
		unsigned char b;

		// The segmented exponent
		unsigned char e;

		/* ########### Constructors ########### */

		// Boring default constructor: sets a null pixel
		Rgbe() : r(0), g(0), b(0), e(0) {}

		// Explicit initialization from components
		Rgbe(const unsigned char r, const unsigned char g, 
             const unsigned char b, const unsigned char e) : 
			r(r), g(g), b(b), e(e) {}

		// Construct from a full float pixel
		Rgbe(const Rgba32F &pixel) {
			set(pixel);
		}

		// Construct from a float pixel without alpha
		Rgbe(const Rgb32F &pixel) {
			set(pixel);
		}

        // Construct from explicit float RGB values
        Rgbe(const float r, const float g, const float b) {
            set(r, g, b);
        }

		
		/* ########### Setters ########### */

		// Set all values at once
		inline void set(const unsigned char r, const unsigned char g, 
                        const unsigned char b, const unsigned char e = 128) {
			this->r = r;
			this->g = g;
			this->b = b;
			this->e = e;
		}
		
        // Explicit setting each value
        void IMAGEIO_API set(const float r, const float g, const float b);

		// Convert directly from a Rgb96 pixel
		inline void set(const Rgb32F &pixel) {
			Rgba32F px (pixel.r, pixel.g, pixel.b);
            set (px);
		}

		// One way of magic: set the value from a Rgba128.
		// Assumes that the alpha has been previously applied!
        void IMAGEIO_API set(const Rgba32F &pixel);

		/* ########### Operators ########### */

		Rgbe & operator = (const Rgbe &other)
		{
			r = other.r;
			g = other.g;
			b = other.b;
			e = other.e;

			return *this;
		}

		Rgbe & operator = (const Rgba32F &pixel) {
			set(pixel);
			return *this;
		}

		Rgbe & operator = (const Rgb32F &pixel) {
			set(pixel);
			return *this;
		}

		// This one does one way of magic: from RGBE to full float pixel by C++ 
        // casting
		operator Rgba32F() const	
		{
			Rgba32F rgba;
			if (e) {   /*nonzero pixel*/
				const float f = rgbeExpLUT[e];
				rgba.set(rgbeCastLUT[r]*f, rgbeCastLUT[g]*f, rgbeCastLUT[b]*f);
			 }
			 else
				 rgba.zero();

			return rgba;
		}

		operator Rgb32F() const	
		{
			Rgb32F rgb;	// Zero by default
			if (e) {    /*nonzero pixel*/
				const float f = rgbeExpLUT[e];
				rgb.set(rgbeCastLUT[r]*f, rgbeCastLUT[g]*f, rgbeCastLUT[b]*f);
			 }
			return rgb;
		}

		// Some applications require accessing the data as an array of unsigned
		// chars, in the order rgbe
		operator const unsigned char* () const
		{
			return reinterpret_cast<const unsigned char*>(this);
		}

		friend std::ostream & operator<<(std::ostream & os, const Rgbe &p) 
		{
			os  << "{ [R]:" << (int)p.r
				<<  " [G]:" << (int)p.g
				<<  " [B]:" << (int)p.b
				<<  " [E]:" << (int)p.e
				<< " }";
			return os;
		}

	private:

		// Super lookup table for (unsigned char)->float conversions
		// and the exponent factor operation ldexp(1.0f,i-(128+8))
		const static IMAGEIO_API unsigned int rgbeLUT_UI[2][256]; 

		const static IMAGEIO_API float* rgbeCastLUT;
		const static IMAGEIO_API float* rgbeExpLUT;
	};
}

#endif /* PCG_RGBE_H */
