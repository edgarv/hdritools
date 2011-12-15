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

// A structure which represents a RGBA pixel which uses 32-bit floating
// point components and has 16 byte alignment to make it SSE compatible
//
// This uses code from the Intel vector classes included with MS Visual Studio
// in the header fvec.h

#include "StdAfx.h"
#include "ImageIO.h"

#if !defined (PCG_RGBA32F_H)
#define PCG_RGBA32F_H

using namespace std;

namespace pcg {


	struct ALIGN16_BEG Rgba32F {
		
	private:
		__m128 rgba;

		// This mask will clear the sign
		static const ALIGN16_BEG IMAGEIO_API unsigned int abs_mask[4] ALIGN16_END;

		// A mask for testing for 0-alpha
		// Remember the endianess!
		static const ALIGN16_BEG IMAGEIO_API unsigned int alpha_zero_mask[4] ALIGN16_END;

	public:

		/* ########### Constructors ########### */

		// Default constructor. It does nothing and therefore it contains trash!
		Rgba32F() {}

		// Construct from a singe element
		explicit Rgba32F(const float f)
		{ 
			rgba = _mm_set_ps1(f);
		}


		// Construct from explicit values
		Rgba32F(const float r, const float g, const float b, const float a = 1.0f)
		{
			rgba = _mm_set_ps(r,g,b,a);
		}

		// Explicitly initialize from a __m128 value 
		Rgba32F(const __m128 &m)
		{
			rgba = m;
		}

		/* ########### Individual component getters ########### */

		// Red value of the pixel (read only)
		inline const float& r() const {
			return *((float*)&rgba + 3);
		}

		// Green value of the pixel (read only)
		inline const float& g() const {
			return *((float*)&rgba + 2);
		}

		// Blue value of the pixel (read only)
		inline const float& b() const {
			return *((float*)&rgba + 1);
		}

		// Alpha value of the pixel (read only)
		inline const float& a() const {
			return *((float*)&rgba);
		}


		/* ########### Component Setters ########### */

		// Sets the red value of the pixel
		inline void setR(const float r) {
			*((float*)&rgba + 3) = r;
		}

		// Sets the green value of the pixel
		inline void setG(const float g) {
			*((float*)&rgba + 2) = g;
		}

		// Sets the blue value of the pixel
		inline void setB(const float b) {
			*((float*)&rgba + 1) = b;
		}

		// Sets the alpha value of the pixel
		inline void setA(const float a) {
			*((float*)&rgba) = a;
		}

		// Sets all values of an existing pixel from individual floats
		inline void set(const float r, const float g, const float b, const float a = 1.0f) {
			rgba = _mm_set_ps(r,g,b,a);
		}

		// Sets all components to the same value (including alpha!)
		inline void setAll(const float f) {
			rgba = _mm_set_ps1(f);
		}

		// Sets everything to zero
		inline void zero() {
			rgba = _mm_setzero_ps();
		}

		// Multiplies the rgb component by alpha and sets alpha to 1... but if alpha was 0, everything is 0
		inline void applyAlpha() {

			// Multiply everything by alpha. 
			// The shuffle operation replicates the exponent into all components, then do a component-wise multiply
			// rgba = {rgba.r, rgba.r, rgba.r, rgba.a} .* {rgba.a, rgba.a, rgba.a, rgba.a}
			const Rgba32F alphaTemp = _mm_shuffle_ps((__m128)rgba,(__m128)rgba,_MM_SHUFFLE(0,0,0,0));
			const Rgba32F mask      = _mm_cmpneq_ss(*reinterpret_cast<const __m128*>(&alpha_zero_mask), alphaTemp);
			*this *= alphaTemp;

			// Set the alpha to 1.0f
			setA(1.0f);

			// And apply the mask, which will clear the alpha
			*this &= mask;
		}


		/* ########### Operators ########### */

		operator  __m128() const	{ return rgba; }		/* Convert (cast) to __m128 */


		// Some applications require accessing the data as an array of float.
		// Be careful of the ordering!!
		//   result[0] = a
		//   result[1] = b
		//   result[2] = g
		//   result[3] = r
		operator const float* () const 		{
			return reinterpret_cast<const float*>(&rgba);
		}

		operator float* () {
			return reinterpret_cast<float*>(&rgba);
		}

		/* Logical Operators */
		friend Rgba32F operator &(const Rgba32F &a, const Rgba32F &b) { return _mm_and_ps(a,b); }
		friend Rgba32F operator |(const Rgba32F &a, const Rgba32F &b) { return _mm_or_ps (a,b); }
		friend Rgba32F operator ^(const Rgba32F &a, const Rgba32F &b) { return _mm_xor_ps(a,b); }

		/* Comparison Operators */
		friend bool operator ==(const Rgba32F &a, const Rgba32F &b) {
			// Macro to flag 64-bit integer literals as extensions
			#if defined(__GNUC__) || defined(__clang__)
			# define PCG_EXTENSION(expr) __extension__ expr
			#else
			# define PCG_EXTENSION(expr) expr
			#endif
			union { __m128 mask; PCG_EXTENSION(unsigned long long val[2]); };
			mask = _mm_cmpeq_ps(a, b);
			return (val[0] & val[1]) == PCG_EXTENSION(0xffffffffffffffffull);
			#undef PCG_EXTENSION
		}
		friend bool operator !=(const Rgba32F &a, const Rgba32F &b) {
            return !(operator== (a, b));
		}

		///* Arithmetic Operators */
		friend Rgba32F operator +(const Rgba32F &a, const Rgba32F &b) { return _mm_add_ps(a,b); }
		friend Rgba32F operator -(const Rgba32F &a, const Rgba32F &b) { return _mm_sub_ps(a,b); }
		friend Rgba32F operator *(const Rgba32F &a, const Rgba32F &b) { return _mm_mul_ps(a,b); }
		friend Rgba32F operator /(const Rgba32F &a, const Rgba32F &b) { return _mm_div_ps(a,b); }

        friend Rgba32F operator +(const Rgba32F &a, float b) { return _mm_add_ps(a, _mm_set_ps1(b)); }
		friend Rgba32F operator -(const Rgba32F &a, float b) { return _mm_sub_ps(a, _mm_set_ps1(b)); }
		friend Rgba32F operator *(const Rgba32F &a, float b) { return _mm_mul_ps(a, _mm_set_ps1(b)); }
		friend Rgba32F operator /(const Rgba32F &a, float b) { return _mm_div_ps(a, _mm_set_ps1(b)); }
        friend Rgba32F operator +(float a, const Rgba32F &b) { return _mm_add_ps(b, _mm_set_ps1(a)); }
		friend Rgba32F operator -(float a, const Rgba32F &b) { return _mm_sub_ps(b, _mm_set_ps1(a)); }
		friend Rgba32F operator *(float a, const Rgba32F &b) { return _mm_mul_ps(b, _mm_set_ps1(a)); }
		friend Rgba32F operator /(float a, const Rgba32F &b) { return _mm_div_ps(b, _mm_set_ps1(a)); }

		Rgba32F& operator =(const Rgba32F &p)    { rgba = p.rgba; return *this; }
		Rgba32F& operator *=(const Rgba32F &a) { return *this = _mm_mul_ps(rgba,a); }
		Rgba32F& operator +=(const Rgba32F &a) { return *this = _mm_add_ps(rgba,a); }
		Rgba32F& operator -=(const Rgba32F &a) { return *this = _mm_sub_ps(rgba,a); }
		Rgba32F& operator /=(const Rgba32F &a) { return *this = _mm_div_ps(rgba,a); }
		Rgba32F& operator &=(const Rgba32F &a) { return *this = _mm_and_ps(rgba,a); }
		Rgba32F& operator |=(const Rgba32F &a) { return *this = _mm_or_ps (rgba,a); }
		Rgba32F& operator ^=(const Rgba32F &a) { return *this = _mm_xor_ps(rgba,a); }
        
        Rgba32F& operator *=(float x) { return *this = _mm_mul_ps(rgba, _mm_set_ps1(x)); }
        Rgba32F& operator +=(float x) { return *this = _mm_add_ps(rgba, _mm_set_ps1(x)); }
        Rgba32F& operator -=(float x) { return *this = _mm_sub_ps(rgba, _mm_set_ps1(x)); }
        Rgba32F& operator /=(float x) { return *this = _mm_div_ps(rgba, _mm_set_ps1(x)); }

        ///* new and delete, so that they provide the proper alignment */
        static void* operator new (size_t size) {
            void *ptr = pcg::alloc_align<Rgba32F> (16);
            return ptr;
        }

        static void operator delete (void *p) {
            pcg::free_align (p);
        }

        static void* operator new[] (size_t size) {
            void *ptr = pcg::alloc_align<Rgba32F> (16, size);
            return ptr;
        }

        static void operator delete[] (void* p) {
            pcg::free_align (p);
        }

		// Absolute value, by clearing the sign on all four floats
		static Rgba32F abs(const Rgba32F &a) {
			return _mm_and_ps(a, (*reinterpret_cast<const __m128*>(&abs_mask)) );
		}



		friend std::ostream & operator<<(std::ostream & os, const Rgba32F &p) 
		{
		/* To use: cout << "Elements of Rgba128 rgba128 are: " << rgba128; */
			os  << "{ [R]:" << p.r()
				<<  " [G]:" << p.g()
				<<  " [B]:" << p.b()
				<<  " [A]:" << p.a()
				<< " }";
			return os;
		}
	} ALIGN16_END;

}

#endif /* PCG_RGBA32F_H */
