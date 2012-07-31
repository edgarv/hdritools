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

// Implementation file for loading the OpenEXR Images

#include "OpenEXRIO.h"
#include "Exception.h"
#include "ImageIterators.h"

// OpenEXR includes
#include <half.h>
#include <Iex.h>
#include <ImathBox.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfIO.h>
#include <IlmThreadPool.h>

#include <tbb/task_scheduler_init.h>

#include <cerrno>

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

        inline bool checkError (std::istream &is)
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


namespace
{

// Helper to generate Slices for a framebuffer. The offsets are specified as
// multiples of the given type, *NOT* bytes!
template <typename T>
inline Imf::Slice newSlice(T* base, size_t xStride, size_t yStride,
    double fillValue = 0.0, Imf::PixelType type = Imf::FLOAT)
{
    Imf::Slice slice(type, reinterpret_cast<char*>(base),
        sizeof(T) * xStride, sizeof(T) * yStride,
        1, 1, fillValue);
    return slice;
}



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



// Small function to actually copy the pixels from the already open file into
// the SoA image
void ReadImage(RGBAImageSoA &img, Imf::RgbaInputFile &file)
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

    // Convert everything to full floating point
    const Imf::Rgba *halfPixel = &halfPixels[0][0];
    float* r = img.GetDataPointer<RGBAImageSoA::R>();
    float* g = img.GetDataPointer<RGBAImageSoA::G>();
    float* b = img.GetDataPointer<RGBAImageSoA::B>();
    float* a = img.GetDataPointer<RGBAImageSoA::A>();
    for(int i = 0; i < width*height; ++i) {
        r[i] = halfPixel[i].r;
        g[i] = halfPixel[i].g;
        b[i] = halfPixel[i].b;
        a[i] = halfPixel[i].a;
    }
}



// Version using the general purpose interface, assumes RGBA channels
void ReadImage(Image<Rgba32F, TopDown> &img, Imf::InputFile &file)
{
    Imath::Box2i dw = file.header().dataWindow();
    const int width  = dw.max.x - dw.min.x + 1;
    const int height = dw.max.y - dw.min.y + 1;

    // We will read the RGBA data, requesting full floating point values

    // Build a framebuffer
    Imf::FrameBuffer framebuffer;
    img.Alloc(width, height);
    float *pixels = reinterpret_cast<float *>(img.GetDataPointer());

    // The code assumes that sizeof(Rgba32F) == 4 * sizeof(float), and that the
    // pixels inside Rgba32F are at certain fixed offsets. If
    //   Rgba32F pixel;
    //   float * ptr = reinterpret_cast<float*>(&pixel)
    // Then the following is true:
    //   ptr[0] == pixel.a()
    //   ptr[1] == pixel.b()
    //   ptr[2] == pixel.g()
    //   ptr[3] == pixel.r()
    const off_t baseOffset = - 4 * (dw.min.x + dw.min.y*width);
    pixels += baseOffset;

    framebuffer.insert("R", newSlice(pixels+3, 4, 4*width));
    framebuffer.insert("G", newSlice(pixels+2, 4, 4*width));
    framebuffer.insert("B", newSlice(pixels+1, 4, 4*width));
    framebuffer.insert("A", newSlice(pixels,   4, 4*width, 1.0));

    // Read all the pixels from the image
    file.setFrameBuffer (framebuffer);
    file.readPixels (dw.min.y, dw.max.y);	// Ossia, (0, height-1);
}



// Version using the general purpose interface, assumes RGBA channels (SoA)
void ReadImage(RGBAImageSoA &img, Imf::InputFile &file)
{
    Imath::Box2i dw = file.header().dataWindow();
    const int width  = dw.max.x - dw.min.x + 1;
    const int height = dw.max.y - dw.min.y + 1;

    // We will read the RGBA data, requesting full floating point values

    // Build a framebuffer
    Imf::FrameBuffer framebuffer;
    img.Alloc(width, height);
    float* r = img.GetDataPointer<RGBAImageSoA::R>();
    float* g = img.GetDataPointer<RGBAImageSoA::G>();
    float* b = img.GetDataPointer<RGBAImageSoA::B>();
    float* a = img.GetDataPointer<RGBAImageSoA::A>();

    const off_t baseOffset = - (dw.min.x + dw.min.y*width);
    r += baseOffset;
    g += baseOffset;
    b += baseOffset;
    a += baseOffset;

    framebuffer.insert("R", newSlice(r, 1, width));
    framebuffer.insert("G", newSlice(g, 1, width));
    framebuffer.insert("B", newSlice(b, 1, width));
    framebuffer.insert("A", newSlice(a, 1, width, 1.0));

    // Read all the pixels from the image
    file.setFrameBuffer (framebuffer);
    file.readPixels (dw.min.y, dw.max.y);	// Ossia, (0, height-1);
}



template <class ImageCls>
void LoadImpl(ImageCls& img, std::istream &is, int nThreads = 0) {

    try {
        IlmThread::ThreadPool::globalThreadPool().setNumThreads(nThreads);
        StdIStream stdis(is);
        Imf::InputFile file(stdis);
        ReadImage(img, file);
    }
    catch (Iex::BaseExc &e) {
        throw IOException(e);
    }
}

template <class ImageCls>
void LoadImpl(ImageCls& img,  const char *filename, int nThreads = 0) {

    try {
        IlmThread::ThreadPool::globalThreadPool().setNumThreads(nThreads);
        Imf::InputFile file(filename);
        const Imf::ChannelList & channels = file.header().channels();
        const bool isYC = channels.findChannel("Y")  != NULL || 
                          channels.findChannel("RY") != NULL || 
                          channels.findChannel("BY") != NULL;
        if (!isYC) {
            ReadImage(img, file);
        } else {
            Imf::RgbaInputFile ycFile(filename);
            ReadImage(img, ycFile);
        }
    }
    catch (Iex::BaseExc &e) {
        throw IOException(e);
    }
}

} // namespace




int OpenEXRIO::numThreads = tbb::task_scheduler_init::default_num_threads();


void OpenEXRIO::setNumThreads(int num)
{
    if (num < 0) {
        throw IllegalArgumentException("The number of threads for OpenEXR IO "
            "cannot be negative.");
    }
    numThreads = num;
}


void OpenEXRIO::LoadHelper(Image<Rgba32F, TopDown> &img,  const char *filename){
    LoadImpl(img, filename, numThreads);
}

void OpenEXRIO::LoadHelper(Image<Rgba32F, TopDown> &img,  std::istream &is) {
    LoadImpl(img, is, numThreads);
}

void OpenEXRIO::Load(RGBAImageSoA& img, const char* filename) {
    LoadImpl(img, filename, numThreads);
}

void OpenEXRIO::Load(RGBAImageSoA& img, std::istream& is) {
    LoadImpl(img, is, numThreads);
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


// Small function to translate between our enum and the actual Ilm type
inline Imf::RgbaChannels getImfRgbaChannels(OpenEXRIO::RgbaChannels channels)
{
    switch(channels) {
    case OpenEXRIO::WRITE_R:
        return Imf::WRITE_R;
    case OpenEXRIO::WRITE_G:
        return Imf::WRITE_G;
    case OpenEXRIO::WRITE_B:
        return Imf::WRITE_B;
    case OpenEXRIO::WRITE_A:
        return Imf::WRITE_A;
    case OpenEXRIO::WRITE_RGB:
        return Imf::WRITE_RGB;
    case OpenEXRIO::WRITE_RGBA:
        return Imf::WRITE_RGBA;
    case OpenEXRIO::WRITE_YC:
        return Imf::WRITE_YC;
    case OpenEXRIO::WRITE_YCA:
        return Imf::WRITE_YCA;
    case OpenEXRIO::WRITE_Y:
        return Imf::WRITE_Y;
    case OpenEXRIO::WRITE_YA:
        return Imf::WRITE_YA;

    default:
        assert(false);
        return Imf::WRITE_RGBA;
    }
}



template <typename ImgIterator>
void SaveImpl(ImgIterator begin, const char* filename, int width, int height,
    pcg::ScanLineMode scanlineMode,
    OpenEXRIO::Compression compression, OpenEXRIO::RgbaChannels rgbaChannels,
    int nThreads)
{
    try {
        IlmThread::ThreadPool::globalThreadPool().setNumThreads(nThreads);

        // Temporal buffer to convert from our floating point pixels into half
        Imf::Array2D<Imf::Rgba> halfPixels(width, height);
        
        Imf::Rgba *halfPixel = &halfPixels[0][0];
        ImgIterator pixel = begin;
        for(int i = 0; i < width*height; ++i, ++pixel, ++halfPixel) {
            halfPixel->r = pixel->r();
            halfPixel->g = pixel->g();
            halfPixel->b = pixel->b();
            halfPixel->a = pixel->a();
        }

        // Retrieve the compression type and the scanline order to use
        const Imf::Compression c   = getImfCompression(compression);
        const Imf::RgbaChannels cn = getImfRgbaChannels(rgbaChannels);
        const Imf::LineOrder order =
            scanlineMode == TopDown ? Imf::INCREASING_Y : Imf::DECREASING_Y;

        // Hyper-easy IlmImf-based file creation:
        // Filename, width, height, channels, pixel aspect ratio,
        // screen window center, screen window width, line order, compression
        Imf::RgbaOutputFile file(filename, width, height, cn, 1, 
            Imath::V2f(0,0), 1, order, c);
        file.setFrameBuffer(&halfPixels[0][0], 1, width);
        file.writePixels(height);
    }
    catch (Iex::BaseExc &e) {
        throw IOException(e);
    }
}


template<ScanLineMode S>
void OpenEXRIO::SaveHelper(Image<Rgba32F, S> &img, const char *filename,
    Compression compression, RgbaChannels rgbaChannels)
{
    SaveImpl(img.GetDataPointer(), filename, img.Width(), img.Height(),
        S, compression, rgbaChannels, numThreads);
}


// Actually instanciate the saving template
void OpenEXRIO::Save(Image<Rgba32F, TopDown> &img, const char *filename,
    Compression compression)
{
    SaveHelper(img, filename, compression, WRITE_RGB);
}
void OpenEXRIO::Save(Image<Rgba32F, BottomUp> &img, const char *filename,
    Compression compression)
{
    SaveHelper(img, filename, compression, WRITE_RGB);
}

void OpenEXRIO::Save(Image<Rgba32F, TopDown> &img, const char *filename,
    RgbaChannels rgbaChannels, Compression compression)
{
    SaveHelper(img, filename, compression, rgbaChannels);
}
void OpenEXRIO::Save(Image<Rgba32F, BottomUp> &img, const char *filename,
    RgbaChannels rgbaChannels, Compression compression)
{
    SaveHelper(img, filename, compression, rgbaChannels);
}

void OpenEXRIO::Save(RGBAImageSoA& img, const char* filename,
    RgbaChannels rgbaChannels, Compression compression)
{
    RGBA32FScalarImageSoAIterator it=RGBA32FScalarImageSoAIterator::begin(img);
    SaveImpl(it, filename, img.Width(), img.Height(), img.GetMode(),
        compression, rgbaChannels, numThreads);
}
