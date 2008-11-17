// Implementation file for loading the OpenEXR Images

#include "OpenEXRIO.h"
#include "Exception.h"

// OpenEXR includes
#include <half.h>
#include <Iex.h>
#include <ImathBox.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <ImfIO.h>
#include <errno.h>

// Almost a copy of Imf::StdIFStream, but this one works with any kind of istreams,
// it won't close them when it's done
namespace pcg {

	class StdIStream: public Imf::IStream
	{
	public:

		//---------------------------------------------------------
		// A constructor that uses a std::ifstream that has already
		// been opened by the caller.  The StdIFStream's destructor
		// will not close the std::ifstream.
		//---------------------------------------------------------
		StdIStream (std::istream &is, const char fileName[] = "internalBuffer.exr") :
		  IStream(fileName),
		  _is(is)
		{
			// empty
		}


		  virtual ~StdIStream () {}

		  virtual bool	read (char c[/*n*/], int n)
		  {
			  if (!_is)
				  throw Iex::InputExc ("Unexpected end of file.");

			  clearError();
			  _is.read (c, n);
			  return checkError (_is);
		  }

		  virtual Imath::Int64	tellg ()
		  {
			  return std::streamoff (_is.tellg());
		  }

		  virtual void	seekg (Imath::Int64 pos)
		  {
			  _is.seekg (pos);
			  checkError (_is);
		  }

		  virtual void	clear ()
		  {
			  _is.clear();
		  }

	private:

		std::istream &	_is;

		inline bool checkError (istream &is)
		{
			if (!is)
			{
				if (errno)
					Iex::throwErrnoExc();
				return false;
			}

			return true;
		}

		inline void clearError ()
		{
			errno = 0;
		}
	};

}

using namespace pcg;


// Small function to actually copy the pixels from the already open file into the image
void ReadImage(Image<Rgba32F, TopDown> &img, Imf::RgbaInputFile &file)
{
	Imath::Box2i dw = file.dataWindow();
	const int width  = dw.max.x - dw.min.x + 1;
	const int height = dw.max.y - dw.min.y + 1;

	// Memory for reading the all pixels
	Imf::Array2D<Imf::Rgba> halfPixels(width, height);

	// Read all the pixels from the image
	file.setFrameBuffer (&halfPixels[0][0], 1, width);
	file.readPixels (dw.min.y, dw.max.y);	// Ossia, (0, height-1);

	// OpenEXR files loaded this way are always organized TopDown according
	// to our point of view
	img.Alloc(width, height);

	// Convert everything to full floating point, storing in the top-down order
	const Imf::Rgba *halfPixel = &halfPixels[0][0];
	Rgba32F *pixel = img.GetDataPointer();
	for(int i = 0; i < width*height; ++i) {
		Rgba32F temp(halfPixel[i].r, halfPixel[i].g, halfPixel[i].b, halfPixel[i].a);
		_mm_stream_ps((float*)pixel[i], temp);
	}
}

void OpenEXRIO::LoadHelper(Image<Rgba32F, TopDown> &img,  const char *filename) {

	try {
		Imf::RgbaInputFile file(filename);
		ReadImage(img, file);
	}
	catch (Iex::BaseExc &e) {

		// TODO throw our custom exception
		throw std::exception(e);
	}
}

void OpenEXRIO::LoadHelper(Image<Rgba32F, TopDown> &img,  istream &is) {

	try {
		StdIStream stdis(is);
		Imf::RgbaInputFile file(stdis);
		ReadImage(img, file);
	}
	catch (Iex::BaseExc &e) {
		throw IOException(e);
	}
}

// Small function to translate between our enum and the actual Ilm type
inline Imf::Compression getImfCompression(OpenEXRIO::Compression c) {
	switch(c) {

		case OpenEXRIO::RLE:
			return Imf::RLE_COMPRESSION;
			break;
		case OpenEXRIO::ZIPS:
			return Imf::ZIPS_COMPRESSION;
			break;
		case OpenEXRIO::ZIP:
			return Imf::ZIP_COMPRESSION;
			break;
		case OpenEXRIO::PIZ:
			return Imf::PIZ_COMPRESSION;
			break;
		case OpenEXRIO::PXR24:
			return Imf::PXR24_COMPRESSION;
			break;
		case OpenEXRIO::B44:
			return Imf::B44_COMPRESSION;
			break;
		case OpenEXRIO::B44A:
			return Imf::B44A_COMPRESSION;
			break;

		default:
			return Imf::NO_COMPRESSION;
			break;
	}

}

template<ScanLineMode S>
void OpenEXRIO::SaveHelper(Image<Rgba32F, S> &img, const char *filename, Compression compression)
{
	try {
		const int width  = img.Width();
		const int height = img.Height();

		// Temporal buffer to convert from our floating point pixels into half
		Imf::Array2D<Imf::Rgba> halfPixels(width, height);
		
		Imf::Rgba *halfPixel = &halfPixels[0][0];
		const Rgba32F *pixel = img.GetDataPointer();
		for(int i = 0; i < width*height; ++i) {
			halfPixel[i].r = pixel[i].r();
			halfPixel[i].g = pixel[i].g();
			halfPixel[i].b = pixel[i].b();
			halfPixel[i].a = pixel[i].a();
		}

		// Retrieve the compression type and the scanline order to use
		const Imf::Compression c = getImfCompression(compression);
		const Imf::LineOrder order = S == TopDown ? Imf::INCREASING_Y : Imf::DECREASING_Y;

		// Hyper-easy IlmImf-based file creation:
		// Filename, width, height, channels, pixel aspect ratio, screen windows center, 
		// screen window width, line order, compression
		Imf::RgbaOutputFile file(filename, width, height, Imf::WRITE_RGB, 1, 
			Imath::V2f(0,0), 1, order, c);
		file.setFrameBuffer(&halfPixels[0][0], 1, width);
		file.writePixels(height);
	}
	catch (Iex::BaseExc &e) {

		throw IOException(e);
	}

}


// Actually instanciate the saving template
void OpenEXRIO::Save(Image<Rgba32F, TopDown> &img, const char *filename, Compression compression) {
	SaveHelper(img, filename);
}
void OpenEXRIO::Save(Image<Rgba32F, BottomUp> &img, const char *filename, Compression compression) {
	SaveHelper(img, filename);
}
