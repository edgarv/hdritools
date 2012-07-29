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
     Bruce Walter <bjw@graphics.cornell.edu>
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/


/* This file contains code to read and write four byte rgbe file format
 developed by Greg Ward.  It handles the conversions between rgbe and
 pixels consisting of floats.  The data is assumed to be an array of floats.
 By default there are three floats per pixel in the order red, green, blue.
 (RGBE_DATA_??? values control this.)  Only the mimimal header reading and 
 writing is implemented.  Each routine does error checking and will return
 a status value as defined below.  This code is intended as a skeleton so
 feel free to modify it to suit your needs.

 (Place notice here if you modified the code.)

 2012.07.28 edgar   SIMD conversion for SoA images.
 2008.02.10 edgar   Adapted to C++ and decoupled file reading
            and pixel conversion operations.
 2002.10.29	westin	Allow additional header after FORMAT= line; such
			files are routinely emitted by RADIANCE tools.

 posted to http://www.graphics.cornell.edu/~bjw/
 written by Bruce Walter  (bjw@graphics.cornell.edu)  5/26/95
 based on code written by Greg Ward
*/


// Real implementation of the Rgbe reading stuff

#include "RgbeIO.h"
#include "RgbeIOPrivate.h"
#include "Exception.h"
#include "Vec4f.h"
#include "Vec4i.h"

#include <string.h>
#include <fstream>
#include <iomanip>

using namespace pcg;

// Alias the private namespace
namespace rgbeions = pcg::rgbeio_internal;

// For MSVC >= 2005, force sscanf_s
#if _MSC_VER >= 1400
  #define sscanf sscanf_s
#endif


int rgbeions::rgbe_error(rgbe_error_codes code, const char *msg)
{
	switch (code) {
		  case rgbe_read_error:
			  perror("RGBE read error");
			  break;
		  case rgbe_write_error:
			  perror("RGBE write error");
			  break;
		  case rgbe_format_error:
			  fprintf(stderr,"RGBE bad file format: %s\n",msg);
			  break;
		  default:
		  case rgbe_memory_error:
			  fprintf(stderr,"RGBE error: %s\n",msg);
	}
	return RGBE_RETURN_FAILURE;
}


/* default minimal header. modify if you want more information in header */
int rgbeions::writeHeader(ostream &os, int width, int height,
					const rgbe_header_info &info)
{
	const char *programtype;

	if (info.isValidProgramType())
		programtype = &info.programtype[0];
	else
		programtype = "RGBE";
	os << "#?" << programtype << '\n';
	if (os.fail()) {
		return rgbe_error(rgbe_write_error,NULL);
	}
	/* The #? is to identify file type, the programtype is optional. */
	if (info.isValidGamma()) {
		os << "GAMMA=" << info.gamma << '\n';
		if (os.fail()) {
			return rgbe_error(rgbe_write_error,NULL);
		}
	}
	if (info.isValidExposure()) {
		os << "EXPOSURE=" << info.exposure << endl;
		if (os.fail()) {
			return rgbe_error(rgbe_write_error,NULL);
		}
	}
	os << "FORMAT=32-bit_rle_rgbe" << '\n' << '\n';
	if (os.fail()) {
		return rgbe_error(rgbe_write_error,NULL);
	}
	os << "-Y " << height << " +X " << width << '\n';
	if (os.fail()) {
		return rgbe_error(rgbe_write_error,NULL);
	}
	return RGBE_RETURN_SUCCESS;
}



/* minimal header reading.  modify if you want to parse more information */
int rgbeions::readHeader(istream &is, int &width, int &height, rgbe_header_info &info)
{
	char buf[128];
	float tempf;
	int i;

	// Get the first line
	is >> std::setw(sizeof(buf));
	is.getline( &buf[0], sizeof(buf));
	if ( is.fail() ) {
		return rgbe_error(rgbe_read_error,NULL);
	}

	// Check the signature
	if ((buf[0] != '#')||(buf[1] != '?')) {
		/* if you want to require the magic token then uncomment the next line */
		return rgbe_error(rgbe_format_error,"bad initial token");
	}
	else {
		info.setValidProgramType(true);
		for(i=0; i < static_cast<int>(sizeof(info.programtype))-1; i++) {
			if ((buf[i+2] == 0) || isspace(buf[i+2]))
				break;
			info.programtype[i] = buf[i+2];
		}
		info.programtype[i] = 0;
	}

	// Gets the next line in advance
	is.getline( &buf[0], sizeof(buf));
	if ( is.fail() ) {
		return rgbe_error(rgbe_read_error,NULL);
	}

	bool found_format = false;
	for(;;) {
		// In C++, istream.getline() removes the delimiter, which is removed from the stream
		// but not appended to the input either. The delimiter by default if '\n'
		if (buf[0] == 0)
			if (!found_format)
				return rgbe_error(rgbe_format_error,"no FORMAT specifier found");
			else
				break;       /* Break out of loop; end of header */
		else if (strcmp(buf,"FORMAT=32-bit_rle_rgbe") == 0)
			found_format = true;
		else if (sscanf(buf,"GAMMA=%g",&tempf) == 1) {
			info.gamma = tempf;
			info.setValidGamma(true);
		}
		else if (sscanf(buf,"EXPOSURE=%g",&tempf) == 1) {
			info.exposure = tempf;
			info.setValidExposure(true);
		}

		// Pre-fetches the next line
		is.getline( &buf[0], sizeof(buf));
		if ( is.fail() )
			return rgbe_error(rgbe_read_error,NULL);
	}
	/* Blank line has been skipped above; next line should contain image
	size. */

	is.getline( &buf[0], sizeof(buf));
	if ( is.fail() )
		return rgbe_error(rgbe_read_error,NULL);
	if (sscanf(buf,"-Y %d +X %d", &height, &width) < 2)
		return rgbe_error(rgbe_format_error,"missing image size specifier");
	return RGBE_RETURN_SUCCESS;
}


/* The code below is only needed for the run-length encoded files. */
/* Run length encoding adds considerable complexity but does */
/* save some space.  For each scanline, each channel (r,g,b,e) is */
/* encoded separately for better compression. */
int rgbeions::writeBytes_RLE(ostream &os, const unsigned char *data, int numbytes)
{
	int cur, beg_run, run_count, old_run_count, nonrun_count;
	unsigned char buf[2];
	const int MINRUNLENGTH = 4;
	const char *bufC = reinterpret_cast<char*>(&buf[0]);	// To make os.write() happy

	cur = 0;
	while(cur < numbytes) {
		beg_run = cur;
		/* find next run of length at least 4 if one exists */
		run_count = old_run_count = 0;
		while((run_count < MINRUNLENGTH) && (beg_run < numbytes)) {
			beg_run += run_count;
			old_run_count = run_count;
			run_count = 1;
			while((beg_run + run_count < numbytes) && (run_count < 127)
				&& (data[beg_run] == data[beg_run + run_count]))
				run_count++;
		}
		/* if data before next big run is a short run then write it as such */
		if ((old_run_count > 1)&&(old_run_count == beg_run - cur)) {
			buf[0] = 128 + old_run_count;   /*write short run*/
			buf[1] = data[cur];
			os.write(bufC, 2);
			if ( os.fail() )
				return rgbe_error(rgbe_write_error,NULL);
			cur = beg_run;
		}
		/* write out bytes until we reach the start of the next run */
		while(cur < beg_run) {
			nonrun_count = beg_run - cur;
			if (nonrun_count > 128) 
				nonrun_count = 128;
			buf[0] = nonrun_count;
			os.write(bufC, 1);
			if ( os.fail() )
				return rgbe_error(rgbe_write_error,NULL);
			os.write(reinterpret_cast<const char*>(&data[cur]), nonrun_count);
			if ( os.fail() )
				return rgbe_error(rgbe_write_error,NULL);
			cur += nonrun_count;
		}
		/* write out next run if one was found */
		if (run_count >= MINRUNLENGTH) {
			buf[0] = 128 + run_count;
			buf[1] = data[beg_run];
			os.write(bufC, 2);
			if ( os.fail() )
				return rgbe_error(rgbe_write_error,NULL);
			cur += run_count;
		}
	}
	return RGBE_RETURN_SUCCESS;
}


// The real meat: the templated functions for reading and writing the actual pixels
namespace pcg {
	namespace rgbeio_internal {


		// ####################################################################
		// ####         READING         #######################################
		// ####################################################################
		
		// Forward declaration
		template <class T>
		int readPixels_RLE(istream&, T*, int, int);

		// The basic utility methods for loading from an RGBE file which has
		// already been successfully open. This handles the general case by going line by line
		template < class T, ScanLineMode S >
		int read(istream &is, Image<T,S> &img)
		{
			const int width  = img.Width();
			const int height = img.Height();
			for (int j = 0; j < height; ++j) {

				T* dest = img.GetScanlinePointer(j, TopDown);
				int retVal = readPixels_RLE(is, dest, width, 1);
				if (retVal != RGBE_RETURN_SUCCESS) {
					return retVal;
				}
			}
			return RGBE_RETURN_SUCCESS;
		}

		// Easy case: the destination is TopDown, just as the RGBE files
		template <class T>
		int read(istream &is, Image<T,TopDown> &img)
		{
			return readPixels_RLE(is, img.GetDataPointer(), img.Width(), img.Height());
		}

		/* simple read routine.  will not correctly handle run length encoding.
		 * It's also quite slow because it reads element by element and converts them on the fly
		 */
		template <class T>
		int readPixels(istream &is, T *data, int numpixels)
		{

			Rgbe rgbe;

			while(numpixels-- > 0) {
				is.read(reinterpret_cast<char*>(&rgbe), sizeof(Rgbe));
				if ( is.fail() )
					return rgbe_error(rgbe_read_error,NULL);
				*data++ = rgbe;
			}
			return RGBE_RETURN_SUCCESS;
		}

		// If we are just directly copying RGBE pixels we can do much better (about 10x faster). 
		// It assumes that the data has been already allocated!
		template <>
		int readPixels(istream &is, Rgbe *data, int numpixels)
		{
			is.read(reinterpret_cast<char*>(data), sizeof(Rgbe)*numpixels);
			if ( is.fail() )
				return rgbe_error(rgbe_read_error,NULL);
			return RGBE_RETURN_SUCCESS;
		}

		template <class T>
		int readPixels_RLE(istream &is, T *data, int scanline_width,
			int num_scanlines)
		{
			Rgbe rgbe;
			unsigned char *scanline_buffer = NULL, *ptr = NULL, *ptr_end = NULL;
			int i, count;
			unsigned char buf[2];

			if ((scanline_width < 8)||(scanline_width > 0x7fff))
				/* run length encoding is not allowed so read flat*/
				return readPixels(is,data,scanline_width*num_scanlines);
			scanline_buffer = NULL;
			/* read in each successive scanline */
			while(num_scanlines > 0) {
				is.read(reinterpret_cast<char*>(&rgbe), sizeof(rgbe));
				if ( is.fail() ) {
					delete [] scanline_buffer;
					return rgbe_error(rgbe_read_error,NULL);
				}
				if ((rgbe[0] != 2)||(rgbe[1] != 2)||(rgbe[2] & 0x80)) {
					/* this file is not run length encoded */
					*data++ = rgbe;
					delete [] scanline_buffer;
					return readPixels(is,data,scanline_width*num_scanlines-1);
				}
				if ((((int)rgbe[2])<<8 | rgbe[3]) != scanline_width) {
					delete [] scanline_buffer;
					return rgbe_error(rgbe_format_error,"wrong scanline width");
				}
				if (scanline_buffer == NULL)
					scanline_buffer = new unsigned char[4*scanline_width];
				if (scanline_buffer == NULL) 
					return rgbe_error(rgbe_memory_error,"unable to allocate buffer space");

				ptr = &scanline_buffer[0];
				/* read each of the four channels for the scanline into the buffer */
				for(i=0;i<4;i++) {
					ptr_end = &scanline_buffer[(i+1)*scanline_width];
					while(ptr < ptr_end) {
						is.read(reinterpret_cast<char*>(&buf[0]), 2);
						if ( is.fail() ) {
							delete [] scanline_buffer;
							return rgbe_error(rgbe_read_error,NULL);
						}
						if (buf[0] > 128) {
							/* a run of the same value */
							count = buf[0]-128;
							if ((count == 0)||(count > ptr_end - ptr)) {
								delete [] scanline_buffer;
								return rgbe_error(rgbe_format_error,"bad scanline data");
							}
							while(count-- > 0)
								*ptr++ = buf[1];
						}
						else {
							/* a non-run */
							count = buf[0];
							if ((count == 0)||(count > ptr_end - ptr)) {
								delete [] scanline_buffer;
								return rgbe_error(rgbe_format_error,"bad scanline data");
							}
							*ptr++ = buf[1];
							if (--count > 0) {
								is.read( reinterpret_cast<char*>(ptr), count );
								if ( is.fail() ) {
									delete [] scanline_buffer;
									return rgbe_error(rgbe_read_error,NULL);
								}
								ptr += count;
							}
						}
					}
				}
				/* now convert data from buffer into floats */
				for(i=0;i<scanline_width;i++) {
					rgbe.r = scanline_buffer[i];
					rgbe.g = scanline_buffer[i+scanline_width];
					rgbe.b = scanline_buffer[i+2*scanline_width];
					rgbe.e = scanline_buffer[i+3*scanline_width];
					*data++ = rgbe;
				}
				num_scanlines--;
			}
			delete [] scanline_buffer;
			return RGBE_RETURN_SUCCESS;
		}


		// ####################################################################
		// ####         WRITING         #######################################
		// ####################################################################
		
		// Forward declaration
		template <class T>
		int writePixels_RLE(ostream&, const T*, int, int);

		// The basic utility methods for saving to an RGBE file which has
		// already been successfully open. This handles the general case by going line by line
		template < class T, ScanLineMode S >
		int write(ostream &os, const Image<T,S> &img)
		{
			const int width  = img.Width();
			const int height = img.Height();
			for (int j = 0; j < height; ++j) {

				T* dest = img.GetScanlinePointer(j, TopDown);
				int retVal = writePixels_RLE(os, dest, width, 1);
				if (retVal != RGBE_RETURN_SUCCESS) {
					return retVal;
				}
			}
			return RGBE_RETURN_SUCCESS;
		}

		// Easy case: the destination is TopDown, just as the RGBE files
		template <class T>
		int write(ostream &os, const Image<T,TopDown> &img)
		{
			return writePixels_RLE(os, img.GetDataPointer(), img.Width(), img.Height());
		}

		/* simple write routine that does not use run length encoding */
		/* These routines can be made faster by allocating a larger buffer and
		   fread-ing and fwrite-ing the data in larger chunks.
		   For using this template it must be possible to cast the type
		   "T" into "Rgbe" as in:
		   
		     Rgbe rgbe = (Rgbe)pixels[i];
		*/
		template<class T>
		int writePixels(ostream &os, const T *pixels, const int numpixels)
		{
			Rgbe rgbe;

			for(int i = 0; i < numpixels; ++i) {
				rgbe = (Rgbe)pixels[i];
				os.write(reinterpret_cast<char*>(&rgbe), sizeof(Rgbe));
				if ( os.fail() ) {
					return rgbe_error(rgbe_write_error,NULL);
				}
			}
			return RGBE_RETURN_SUCCESS;
		}

		// As with the reading, if we are writing already Rgbe pixels we can do all of them at once
		template <>
		int writePixels(ostream &os, const Rgbe *pixels, const int numpixels)
		{
			os.write(reinterpret_cast<const char*>(pixels), sizeof(Rgbe)*numpixels);
			if ( os.fail() ) {
				return rgbe_error(rgbe_write_error,NULL);
			}
			return RGBE_RETURN_SUCCESS;
		}


		template <class T>
		int writePixels_RLE(ostream &os, const T *pixels, int scanline_width,
			int num_scanlines)
		{
			unsigned char rgbe[4];
			unsigned char *buffer;
			int i, err;

			if ((scanline_width < 8)||(scanline_width > 0x7fff))
				/* run length encoding is not allowed so write flat*/
				return writePixels(os,pixels,scanline_width*num_scanlines);
			buffer = new unsigned char[4*scanline_width];
			if (buffer == NULL) 
				/* no buffer space so write flat */
				return writePixels(os,pixels,scanline_width*num_scanlines);
			while(num_scanlines-- > 0) {
				rgbe[0] = 2;
				rgbe[1] = 2;
				rgbe[2] = scanline_width >> 8;
				rgbe[3] = scanline_width & 0xFF;
				os.write(reinterpret_cast<char*>(&rgbe[0]), 4);
				if ( os.fail() ) {
					delete [] buffer;
					return rgbe_error(rgbe_write_error,NULL);
				}
				for(i=0;i<scanline_width;i++) {

					const Rgbe rgbe = (Rgbe)*pixels++;
					buffer[i]                  = rgbe[0];
					buffer[i+scanline_width]   = rgbe[1];
					buffer[i+2*scanline_width] = rgbe[2];
					buffer[i+3*scanline_width] = rgbe[3];
				}
				/* write out each of the four channels separately run length encoded */
				/* first red, then green, then blue, then exponent */
				for(i=0;i<4;i++) {
					if ((err = writeBytes_RLE(os,&buffer[i*scanline_width],
						scanline_width)) != RGBE_RETURN_SUCCESS) {
							delete [] buffer;
							return err;
					}
				}
			}
			delete [] buffer;
			return RGBE_RETURN_SUCCESS;
		}


		// ####################################################################
		// ####         LOADING         #######################################
		// ####################################################################


		// Static method: it efficiently creates a new instance of the specified
		// image type and fills the pixels with the contents of the given RGBE file,
		// which is read from the given stream. In order for this method to work,
		// it must be possible to create a pixel from the given type from an RGBE pixel
		// using just a cast. Note that if you just want the plain RgbeImage (Rgbe pixels
		// with top-down order) it might be better just to use a normal instance.
		// It receives a reference to the destination image to that the template can be deduced
		// automatically. Be awere that the contents of img will be whipped without mercy!
		template < class T, ScanLineMode S >
		inline void Load(Image<T,S> &img, istream &is) {

			// Read the header
			int width, height;
			rgbe_header_info info;
			if (readHeader(is, width, height, info) != RGBE_RETURN_SUCCESS) {
				throw IOException("Couldn't read RGBE header.");
			}

			// Allocates the space
			img.Alloc(width, height);

			// Reads the pixels. The scanline-related issues will be handled
			// by templates
			if ( read(is, img) != RGBE_RETURN_SUCCESS ) {
				throw IOException("Couldn't read RGBE pixel data.");
			}
		}

		// The same as above, only that it creates the stream for you
		template < class T, ScanLineMode S >
		inline void Load(Image<T,S> &img, const char *filename) {
			
			ifstream rgbeFile(filename, ios_base::binary);
			if (! rgbeFile.fail() ) {
				Load(img, rgbeFile);
			}
			else {
				// Something terrible takes place here
				throw IOException("RGBE Load badness!!");
			}
		}


		// ####################################################################
		// ####         SAVING          #######################################
		// ####################################################################

		template < class T, ScanLineMode S >
		inline void Save(const Image<T,S> &img, ostream &os) {

			/* Write the header for a raw image (without gamma correction) */
			rgbe_header_info info;
			info.exposure = 1.0f;
			info.gamma    = 1.0f;
			info.setValidExposure(true);
			info.setValidGamma(true);

			// Writes the header witht the default program type
			if (writeHeader(os, img.Width(), img.Height(), info) != RGBE_RETURN_SUCCESS) {
				throw IOException("RGBE Save badness!!");
			}

			// Writes the pixels. The scanline-related issues will be handled
			// by templates
			if ( write(os, img) != RGBE_RETURN_SUCCESS ) {
				throw IOException("RGBE Save badness!!");
			}
		}
		
		template < class T, ScanLineMode S >
		inline void Save(const Image<T,S> &img, const char *filename) {

			ofstream rgbeFile(filename, ios_base::binary);
			if (! rgbeFile.fail() ) {
				Save(img, rgbeFile);
			}
			else {
				// Something terrible takes place here
				throw IOException("RGBE Save badness!!");
			}

		}

	}
}


// ################################################################################
//
// Finally, these are the "actual" loading methods which are imported. Each of them
// is just using the internal template!
//
// ################################################################################

// LOAD

// Rgbe
void RgbeIO::Load(Image<Rgbe,TopDown>  &img, istream &is) {
	rgbeions::Load(img, is);
}
void RgbeIO::Load(Image<Rgbe,BottomUp> &img, istream &is) {
	rgbeions::Load(img, is);
}
void RgbeIO::Load(Image<Rgbe,TopDown>  &img, const char *filename) {
	rgbeions::Load(img, filename);
}
void RgbeIO::Load(Image<Rgbe,BottomUp> &img, const char *filename) {
	rgbeions::Load(img, filename);
}

// Rgba32F
void RgbeIO::Load(Image<Rgba32F,TopDown>  &img, istream &is) {
	rgbeions::Load(img, is);
}
void RgbeIO::Load(Image<Rgba32F,BottomUp> &img, istream &is) {
	rgbeions::Load(img, is);
}
void RgbeIO::Load(Image<Rgba32F,TopDown>  &img, const char *filename) {
	rgbeions::Load(img, filename);
}
void RgbeIO::Load(Image<Rgba32F,BottomUp> &img, const char *filename) {
	rgbeions::Load(img, filename);
}

// Rgb32F
void RgbeIO::Load(Image<Rgb32F,TopDown>  &img, istream &is) {
	rgbeions::Load(img, is);
}
void RgbeIO::Load(Image<Rgb32F,BottomUp> &img, istream &is) {
	rgbeions::Load(img, is);
}
void RgbeIO::Load(Image<Rgb32F,TopDown>  &img, const char *filename) {
	rgbeions::Load(img, filename);
}
void RgbeIO::Load(Image<Rgb32F,BottomUp> &img, const char *filename) {
	rgbeions::Load(img, filename);
}



// SAVE

// Rgbe
void RgbeIO::Save(const Image<Rgbe,TopDown>  &img, ostream &os) {
	rgbeions::Save(img, os);
}
void RgbeIO::Save(const Image<Rgbe,BottomUp> &img, ostream &os) {
	rgbeions::Save(img, os);
}
void RgbeIO::Save(const Image<Rgbe,TopDown>  &img, const char *filename) {
	rgbeions::Save(img, filename);
}
void RgbeIO::Save(const Image<Rgbe,BottomUp> &img, const char *filename) {
	rgbeions::Save(img, filename);
}

// Rgba32F
void RgbeIO::Save(const Image<Rgba32F,TopDown>  &img, ostream &os) {
	rgbeions::Save(img, os);
}
void RgbeIO::Save(const Image<Rgba32F,BottomUp> &img, ostream &os) {
	rgbeions::Save(img, os);
}
void RgbeIO::Save(const Image<Rgba32F,TopDown>  &img, const char *filename) {
	rgbeions::Save(img, filename);
}
void RgbeIO::Save(const Image<Rgba32F,BottomUp> &img, const char *filename) {
	rgbeions::Save(img, filename);
}

// Rgb32F
void RgbeIO::Save(const Image<Rgb32F,TopDown>  &img, ostream &os) {
	rgbeions::Save(img, os);
}
void RgbeIO::Save(const Image<Rgb32F,BottomUp> &img, ostream &os) {
	rgbeions::Save(img, os);
}
void RgbeIO::Save(const Image<Rgb32F,TopDown>  &img, const char *filename) {
	rgbeions::Save(img, filename);
}
void RgbeIO::Save(const Image<Rgb32F,BottomUp> &img, const char *filename) {
	rgbeions::Save(img, filename);
}



///////////////////////////////////////////////////////////////////////////////
// SoA Image
///////////////////////////////////////////////////////////////////////////////

namespace
{

inline void stream(Vec4f& target, const Vec4f& value) {
    _mm_stream_ps(reinterpret_cast<float*>(&target), value);
}

inline Vec4i sll(const Vec4i& a, int count) {
    return _mm_slli_epi32(a, count);
}

inline Vec4i srl(const Vec4i& a, int count) {
    return _mm_srli_epi32(a, count);
}

inline Vec4f toFloat(const Vec4i& a) {
    return _mm_cvtepi32_ps(a);
}

inline Vec4f castAsFloat(const Vec4i& a) {
    return _mm_castsi128_ps(a);
}

inline Vec4f castAsFloat(const Vec4bi& a) {
    return _mm_castsi128_ps(a);
}

inline Vec4i castAsInt(const Vec4f& a) {
    return _mm_castps_si128(a);
}



void LoadImageSoA(const Image<Rgbe, TopDown>& imgRGBE, RGBAImageSoA& img)
{
    img.Alloc(imgRGBE.Width(), imgRGBE.Height());

    // Convert them using the RTGI2 method, processing multiple pixels at a time
    const Vec4i* PCG_RESTRICT vecRGBE =
        reinterpret_cast<Vec4i*>(imgRGBE.GetDataPointer());
    assert(reinterpret_cast<uintptr_t>(vecRGBE) % 16 == 0);

    const Vec4i const_0xFF(Vec4i::constant<0xFF>());
    const Vec4i const_9(Vec4i::constant<9>());
    const Vec4f const_1p(1.0f);

    typedef Vec4f* PCG_RESTRICT RVec4f;
    RVec4f rPtr=reinterpret_cast<Vec4f*>(img.GetDataPointer<RGBAImageSoA::R>());
    RVec4f gPtr=reinterpret_cast<Vec4f*>(img.GetDataPointer<RGBAImageSoA::G>());
    RVec4f bPtr=reinterpret_cast<Vec4f*>(img.GetDataPointer<RGBAImageSoA::B>());
    RVec4f aPtr=reinterpret_cast<Vec4f*>(img.GetDataPointer<RGBAImageSoA::A>());

    const int vecCount = imgRGBE.Size() / 4;
    for (int i = 0; i != vecCount; ++i) {
        const Vec4i& rgbe = vecRGBE[i];
        // Unpack the RGBE pixel
        Vec4f r = toFloat(const_0xFF & rgbe);
        Vec4f g = toFloat(const_0xFF & (srl(rgbe,  8)));
        Vec4f b = toFloat(const_0xFF & (srl(rgbe, 16)));
        Vec4i e = srl(rgbe, 24);

        // Values in the range 1 to 9 would require "denormal" multipliers and
        // are below minimum values for RGBE exponents so we truncate them to 0
        const Vec4i exponentMask(e > const_9);
        e =  sll((e - const_9), 23) & exponentMask;
        const Vec4f scale = castAsFloat(e);

        r *= scale;
        g *= scale;
        b *= scale;
        stream(rPtr[i], r);
        stream(gPtr[i], g);
        stream(bPtr[i], b);
        stream(aPtr[i], const_1p);
    }
}



void SaveImageSoA(Image<Rgbe, TopDown>& dest, const RGBAImageSoA& src)
{
    dest.Alloc(src.Width(), src.Height());

    // Convert them using the RTGI2 method, processing multiple pixels at a time
    Vec4i* PCG_RESTRICT vecRGBE=reinterpret_cast<Vec4i*>(dest.GetDataPointer());
    assert(reinterpret_cast<uintptr_t>(vecRGBE) % 16 == 0);

    typedef const Vec4f* PCG_RESTRICT const RVec4f;
    RVec4f rPtr=reinterpret_cast<Vec4f*>(src.GetDataPointer<RGBAImageSoA::R>());
    RVec4f gPtr=reinterpret_cast<Vec4f*>(src.GetDataPointer<RGBAImageSoA::G>());
    RVec4f bPtr=reinterpret_cast<Vec4f*>(src.GetDataPointer<RGBAImageSoA::B>());

    const Vec4f min_val(1e-32f);
    const Vec4i const_0xFF(Vec4i::constant<0xFF>());
    const Vec4i const_0x1FF(Vec4i::constant<0x1FF>());
    const Vec4i const_253(Vec4i::constant<253>());
    const Vec4i const_1(Vec4i::constant<1>());

    const int vecCount = src.Size() / 4;
    for (int i = 0; i != vecCount; ++i) {
        Vec4f red   = rPtr[i];
        Vec4f green = gPtr[i];
        Vec4f blue  = bPtr[i];

        // Kill NaN pixels to avoid signaling errors
        Vec4f maskValid((red == red) & (green == green) & (blue == blue));
        red   &= maskValid;
        green &= maskValid;
        blue  &= maskValid;

        // Negative values cannot be encoded, so we truncate them to zero
        red   = simd_max(red,   Vec4f::zero());
        green = simd_max(green, Vec4f::zero());
        blue  = simd_max(blue,  Vec4f::zero());

        // Find the largest value of the three color components
        Vec4f maxValue = simd_max(blue, simd_max(red, green));

        // Consider all values less than this to be zero. This constant comes
        // from Ward's definition in "Real Pixels" (Graphics Gems II)
        maskValid &= Vec4f(maxValue >= min_val);

        // Extract the exponent from the IEEE single precision value
        Vec4i biasedExponent = srl(castAsInt(maxValue), 23) & const_0xFF;
        // Overflow
        maskValid = andnot(castAsFloat(biasedExponent > const_253), maskValid);

        // Construct an additive normalizer which is just 2^(exp+1).
        // Adding this to each float will move the relevant mantissa bits to a
        // known fixed location for easy extraction
        Vec4f additiveNormalizer = castAsFloat(sll(biasedExponent+const_1, 23));
        // Initially we keep an extra bit (9-bits) so that we can perform
        // rounding to 8-bits in the next step
        Vec4i rawR = srl(castAsInt(red  +additiveNormalizer), 14) & const_0x1FF;
        Vec4i rawG = srl(castAsInt(green+additiveNormalizer), 14) & const_0x1FF;
        Vec4i rawB = srl(castAsInt(blue +additiveNormalizer), 14) & const_0x1FF;
        // rgbeBiasedExponent = (ieeeBiasedExponent-127) + 128 since IEEE single
        // float and rgbe have different exponent bias values
        Vec4i e = biasedExponent + const_1 + const_1;
        // round to nearest representable 8 bit value
        Vec4i r = srl(rawR + const_1, 1);
        Vec4i g = srl(rawG + const_1, 1);
        Vec4i b = srl(rawB + const_1, 1);

        // Check to see if rounding causes an overflow condition and fix if
        // necessary. Note that we actually avoid branches
        Vec4bi maskOverflow = (r>const_0xFF) | (g>const_0xFF) | (b>const_0xFF);
        e += Vec4i(maskOverflow) & const_1;
        const Vec4i const_2 = const_1 + const_1;
        Vec4i rOver = srl(rawR + const_2, 2);
        Vec4i gOver = srl(rawG + const_2, 2);
        Vec4i bOver = srl(rawB + const_2, 2);
        r = select(maskOverflow, rOver, r);
        g = select(maskOverflow, gOver, g);
        b = select(maskOverflow, bOver, b);
        // Overflow after rounding
        Vec4i finalMask = andnot(Vec4i(e > const_0xFF), castAsInt(maskValid));

        // Set to zero if the pixel is invalid, then build the final value
        r &= finalMask;
        g &= finalMask;
        b &= finalMask;
        e &= finalMask;
        vecRGBE[i] = sll(e, 24) | sll(b, 16) | sll(g, 8) | r;
    }
}

} // namespace



void RgbeIO::Load(RGBAImageSoA& img, istream& is)
{
    Image<Rgbe, TopDown> imgRGBE;
    Load(imgRGBE, is);
    LoadImageSoA(imgRGBE, img);
}

void RgbeIO::Load(RGBAImageSoA& img, const char* filename)
{
    Image<Rgbe, TopDown> imgRGBE;
    Load(imgRGBE, filename);
    LoadImageSoA(imgRGBE, img);
}

void RgbeIO::Save(const RGBAImageSoA& img, ostream& os)
{
    Image<Rgbe, TopDown> imgRGBE;
    SaveImageSoA(imgRGBE, img);
    Save(imgRGBE, os);
}

void RgbeIO::Save(const RGBAImageSoA& img, const char* filename)
{
    Image<Rgbe, TopDown> imgRGBE;
    SaveImageSoA(imgRGBE, img);
    Save(imgRGBE, filename);
}
