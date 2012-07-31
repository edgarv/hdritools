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

#include "PfmIO.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <ctype.h>
#include <iostream>
#include <memory>
#if defined(_MSC_VER)
#include <cstdlib>
#endif

using namespace pcg;



inline PfmIO::ByteOrder PfmIO::getNativeOrder()
{
    const int x = 1;
    return (*(char *)&x == 1) ? PfmIO::LittleEndian : PfmIO::BigEndian;
}

PfmIO::Header::Header() : 
isColor(true), width(0), height(0), order(PfmIO::getNativeOrder())
{
}

PfmIO::Header::Header(const Image<Rgba32F, TopDown> &img) :
isColor(true), width(img.Width()), height(img.Height()),
order(PfmIO::getNativeOrder())
{
}

PfmIO::Header::Header(const Image<Rgba32F, BottomUp> &img) :
isColor(true), width(img.Width()), height(img.Height()),
order(PfmIO::getNativeOrder())
{
}

PfmIO::Header::Header(const RGBAImageSoA &img) :
isColor(true), width(img.Width()), height(img.Height()),
order(PfmIO::getNativeOrder())
{
}

PfmIO::Header::Header(std::istream &is)
{
    {
        // Look for the magic number, consuming also the \n
        char a, b, junk;
        is >> a;
        is >> b;
        is.read(&junk, 1);

        if ((a != 'P') || ((b != 'F') && (b != 'f'))) {
            throw PfmIOException("Wrong magic number");
        }
        
        isColor = (b == 'F');	// 'F' = RGB,  'f' = monochrome

        std::string buffer;
        for (;;) {
            std::getline(is, buffer);
            if ( is.fail() ) {
                throw PfmIOException("Couldn't read the header");
            }
            else if (buffer[0] != '#') {
                // Buffer contains the width and the height
                std::istringstream buf(buffer);
                buf >> width;
                if ( buf.fail() ) {
                    throw PfmIOException("Couldn't read the width");
                }
                buf >> height;
                if ( buf.fail() ) {
                    throw PfmIOException("Couldn't read the width");
                }
                break;
            }
        }

        // Now read the line order and the trailing space
        float orderFloat;
        is >> orderFloat;
        if ( is.fail() ) {
            throw PfmIOException("Couldn't read the byte order");
        }
        is.read(&junk, 1);
        if ( is.fail() ) {
            throw PfmIOException("Couldn't read the separator "
                "between the header and the data");
        }
        order = orderFloat <= 0.0f ? PfmIO::LittleEndian : PfmIO::BigEndian;
    }
}


void PfmIO::Header::write(std::ostream &os)
{
    os << (isColor ? "PF" : "Pf") << '\n' << 
           width << ' ' << height << '\n' << 
           std::setiosflags(std::ios::fixed) << std::setprecision(6) <<
          (float)(order == PfmIO::LittleEndian ? -1.0f : 1.0f) << '\n';
}



namespace {

// Helper class to wrap operations for SoA Images: it is non-thread safe
class SoAHelper
{
    float * PCG_RESTRICT m_r;
    float * PCG_RESTRICT m_g;
    float * PCG_RESTRICT m_b;
    float * PCG_RESTRICT m_a;
    mutable size_t offset;

    SoAHelper() {}

public:

    // Initialize from a scanline
    static SoAHelper fromScanline(const RGBAImageSoA& img, int scanline,
        pcg::ScanLineMode mode)
    {
        SoAHelper h;
        h.m_r = img.GetScanlinePointer<RGBAImageSoA::R>(scanline, mode);
        h.m_g = img.GetScanlinePointer<RGBAImageSoA::G>(scanline, mode);
        h.m_b = img.GetScanlinePointer<RGBAImageSoA::B>(scanline, mode);
        h.m_a = img.GetScanlinePointer<RGBAImageSoA::A>(scanline, mode);
        h.offset = 0;
        return h;
    };

    SoAHelper& operator[] (size_t offset) {
        this->offset = offset;
        return *this;
    }

    const SoAHelper& operator[] (size_t offset) const {
        this->offset = offset;
        return *this;
    }

    void set(float red, float green, float blue, float alpha = 1.0) {
        m_r[offset] = red;
        m_g[offset] = green;
        m_b[offset] = blue;
        m_a[offset] = alpha;
    }

    inline float r() const {
        return m_r[offset];
    }
    inline float g() const {
        return m_g[offset];
    }
    inline float b() const {
        return m_b[offset];
    }
};

template <ScanLineMode S>
Rgba32F*
getScanlineIterator(const Image<Rgba32F, S> &img, int scanline, ScanLineMode m){
    return img.GetScanlinePointer(scanline, m);
}

SoAHelper
getScanlineIterator(const RGBAImageSoA &img, int scanline, ScanLineMode m) {
    return SoAHelper::fromScanline(img, scanline, m);
}



template <class ImageIter, class ImageType>
void PfmIO_Save_data(const ImageType &img, std::ostream &os)
{
    // Allocate one full scanline
    const size_t scanline_len = img.Width() * 3 * sizeof(float);
    float *buffer = new float[scanline_len];

    for (int i = 0; i < img.Height(); ++i) {
        ImageIter it = getScanlineIterator(img, i, BottomUp);
        for (int j = 0; j < img.Width(); ++j) {
            const int idx = j*3;
            buffer[idx]   = it[j].r();
            buffer[idx+1] = it[j].g();
            buffer[idx+2] = it[j].b();
        }
        os.write((const char*)buffer, scanline_len);
        if (os.fail()) {
            delete [] buffer;
            throw PfmIOException("Couldn't write the scanline data");
        }
    }

    delete [] buffer;
}

inline void swapByteOrder(unsigned int *ptr, int count)
{
    for (int i = 0; i < count; ++i) {
#if defined(_MSC_VER)
        ptr[i] = _byteswap_ulong(ptr[i]);
#elif defined(__GNUC__)
        ptr[i] = __builtin_bswap32(ptr[i]);
#else
        ptr[i] = ((ptr[i] << 24) & 0xFF000000u) |
                 ((ptr[i] <<  8) & 0xFF0000u) |
                 ((ptr[i] >>  8) & 0x00FFu) |
                 ((ptr[i] >> 24) & 0xFFu);
#endif
    }
}




// Load function just for the data, assumes the istream is right
// at the beginning of the pixels and the image has been allocated
template <class ImageIter, class ImageType>
void Pfm_Load_data(ImageType &img, std::istream &is, 
                   bool swapBytes, bool isColor)
{
    const int numChannels = isColor ? 3 : 1;

    // Allocate one full scanline
    const size_t scanline_len = img.Width() * numChannels * sizeof(float);
    float *buffer = new float[scanline_len];
    if (buffer == NULL) {
        throw PfmIOException("Couldn't allocate the temporary buffer.");
    }

    for (int h = 0; h < img.Height(); ++h) {
        is.read((char*)buffer, scanline_len);
        if ( is.fail() ) {
            delete [] buffer;
            throw PfmIOException("Couldn't read all the scanline data.");
        }

        if (swapBytes) {
            swapByteOrder((unsigned int *)buffer, img.Width() * numChannels);
        }

        ImageIter it = getScanlineIterator(img, h, BottomUp);
        if (isColor) {
            for (int i = 0; i < img.Width(); ++i) {
                const int idx = i*3;
                it[i].set(buffer[idx], buffer[idx+1], buffer[idx+2]);
            }
        } else {
            for (int i = 0; i < img.Width(); ++i) {
                const float &v = buffer[i];
                it[i].set(v,v,v);
            }
        }
    }
    delete [] buffer;
}



template <class ImageCls>
void PfmIO_Save_helper(const ImageCls &img, const char *filename)
{
    std::ofstream pfmFile(filename, std::ios_base::binary);
    if (! pfmFile.fail() ) {
        PfmIO::Save(img, pfmFile);
    }
    else {
        // Something terrible takes place here
        throw PfmIOException((std::string)"Couldn't save the file " + filename);
    }
}

template <class ImageCls>
void PfmIO_Load_helper(ImageCls &img, const char *filename) 
{
    std::ifstream pfmFile(filename, std::ios_base::binary);
    if (! pfmFile.fail() ) {
        PfmIO::Load(img, pfmFile);
    }
    else {
        // Something terrible takes place here
        throw PfmIOException((std::string)"Couldn't open the file " + filename);
    }
}

} // namespace



void PfmIO::Save(const Image<Rgba32F, TopDown>  &img, std::ostream &os)
{
    Header hdr(img);
    hdr.write(os);
    PfmIO_Save_data<const Rgba32F*>(img, os);
}

void PfmIO::Save(const Image<Rgba32F, BottomUp>  &img, std::ostream &os)
{
    Header hdr(img);
    hdr.write(os);
    PfmIO_Save_data<const Rgba32F*>(img, os);
}

void PfmIO::Save(const RGBAImageSoA  &img, std::ostream &os)
{
    Header hdr(img);
    hdr.write(os);
    PfmIO_Save_data<const SoAHelper>(img, os);
}

void PfmIO::Load(Image<Rgba32F, TopDown> &img, std::istream &is)
{
    // Read the header
    Header hdr(is);

    // Allocates the space
    img.Alloc(hdr.width, hdr.height);

    // Reads the pixels
    Pfm_Load_data<Rgba32F*>(img, is, hdr.order!=getNativeOrder(), hdr.isColor);
}

void PfmIO::Load(Image<Rgba32F, BottomUp> &img, std::istream &is)
{
    // Read the header
    Header hdr(is);

    // Allocates the space
    img.Alloc(hdr.width, hdr.height);

    // Reads the pixels
    Pfm_Load_data<Rgba32F*>(img, is, hdr.order!=getNativeOrder(), hdr.isColor);
}

void PfmIO::Load(RGBAImageSoA &img, std::istream &is)
{
    // Read the header
    Header hdr(is);

    // Allocates the space
    img.Alloc(hdr.width, hdr.height);

    // Reads the pixels
    Pfm_Load_data<SoAHelper>(img, is, hdr.order!=getNativeOrder(), hdr.isColor);
}


// Instanciate the templates
void PfmIO::Save(const Image<Rgba32F, TopDown> &img, const char *filename) {
    PfmIO_Save_helper(img, filename);
}
void PfmIO::Save(const Image<Rgba32F, BottomUp> &img, const char *filename) {
    PfmIO_Save_helper(img, filename);
}
void PfmIO::Save(const RGBAImageSoA &img, const char *filename) {
    PfmIO_Save_helper(img, filename);
}


void PfmIO::Load(Image<Rgba32F, TopDown>  &img, const char *filename) {
    PfmIO_Load_helper(img, filename);
}
void PfmIO::Load(Image<Rgba32F, BottomUp> &img, const char *filename) {
    PfmIO_Load_helper(img, filename);
}
void PfmIO::Load(RGBAImageSoA &img, const char *filename) {
    PfmIO_Load_helper(img, filename);
}
