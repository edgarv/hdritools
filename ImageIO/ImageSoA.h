/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2012 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#pragma once
#if !defined(PCG_IMAGESOA_H)
#define PCG_IMAGESOA_H

#include "ImageIO.h"
#include "Image.h"
#include "Exception.h"
#include "Rgba32F.h"

#include <algorithm>
#include <cassert>

#if !defined(_MSC_VER) || _MSC_VER >= 1600
#include <stdint.h>
#endif

namespace pcg
{

/** Tag-structure to query each channel in a type-safe way */
template <typename T, int channelIdx>
struct ChannelSpecTag
{
    static const int IDX = channelIdx;
    typedef T data_t;
};



/**
 * Base for a multi-channel image as Struct-of-Arrays (SoA) which is better
 * for SIMD CPU processing.
 */
template <typename T1, typename T2, typename T3>
class ImageBaseSoA
{
public:

#if defined(_MSC_VER) && _MSC_VER < 1600
    typedef __int8 byte_t;
#else
    typedef int8_t byte_t;
#endif

    // Padding per channel in bytes. For now this is hard-coded to 64 bytes
    // so that 16-single precision number may be read without issues
    static const int PADDING = 64;

    // Typedef for each channel
    typedef ChannelSpecTag<T1, 0> Channel_1;
    typedef ChannelSpecTag<T2, 1> Channel_2;
    typedef ChannelSpecTag<T3, 2> Channel_3;

    static const int NUM_CHANNELS = 3;


    // Default constructor: creates an empty image. To do anything
    // useful afterwards you need to use the Alloc(int,int) method.
    ImageBaseSoA() : m_width(0), m_height(0), m_data(0)
    {
        std::fill(m_offsets, m_offsets + NUM_CHANNELS, -1);
    }

    // Creates a new image allocating the required space
    ImageBaseSoA(int w, int h) : m_width(0), m_height(0), m_data(0)
    {
        assert(w > 0 && h > 0);
        std::fill(m_offsets, m_offsets + NUM_CHANNELS, -1);
        Alloc(w, h);
    }

    // Destructor, it reclaims the space previously allocated
    ~ImageBaseSoA() {
        Clear();
    }

    // Allocates new space for the image data, deleting the previous one
    void Alloc(int w, int h) {
        assert(w > 0 && h > 0);
        assert(((long long)w)*h <= 0x7fffffff);
        Clear();
        m_width  = w;
        m_height = h;
        
        // Allocate each channel, padding to 64 bytes and with 16-byte alignment
        const size_t numel = static_cast<size_t>(w) * static_cast<size_t>(h);

        // This is coupled with the number of channels
        const static size_t sizes[] = {
            sizeof(T1), sizeof(T2), sizeof(T3)
        };

        size_t offset = 0;
        m_offsets[0] = offset;
        for (int i = 0; i < NUM_CHANNELS; ++i) {
            m_offsets[i] = offset;
            offset += ((numel * sizes[i]) + 63) & ~0x3F;
            assert(offset % 16 == 0);
        }

        // At this point offset contains the total requested memory
        const size_t totalBytes = offset;
        m_data = alloc_align<byte_t>(16, totalBytes);
        if (m_data == NULL) {
            throw RuntimeException("Couldn't allocate memory for the image.");
        }
    }

    // Deallocates the memory and resets the image dimensions to 0
    void Clear() {
        if (m_data != 0) {
            free_align(m_data);
            m_data = 0;
            std::fill(m_offsets, m_offsets + NUM_CHANNELS, -1);
        }

        m_width = m_height = 0;
    }

    // Width of the image
    int Width()  const { return m_width; }

    // Height of the image
    int Height() const { return m_height; }

    // Number of pixels in the image (Width*Height)
    int Size()   const { return m_width*m_height; }

    // Provides access to the scanline mode of the image
    ScanLineMode GetMode() const { return TopDown; }

    // Returns a reference to the i-th pixel in the j-th scanline
    // (indices are zero-based) according to the scanline order of the image.
    template <class ChannelSpec>
    inline typename ChannelSpec::data_t & ElementAt(int i, int j,
        ScanLineMode mode = TopDown) const
    {
        assert(m_width*j+i >=0 && m_width*j+i < m_width*m_height && 
            j >= 0 && j < m_height && i >=0 && i < m_width);

        typedef typename ChannelSpec::data_t data_t;
        data_t * d = reinterpret_cast<data_t*>(
            m_data + m_offsets[ChannelSpec::IDX]);
        return (mode == TopDown) ? d[m_width*j + i] :
                                   d[(m_height-j-1)*m_width + i];
    }

    // Returns a reference to the idx-th pixel of the image, which are in
    // scanline order according to the specified mode.
    template <class ChannelSpec>
    inline typename ChannelSpec::data_t & ElementAt(int idx) const
    {
        assert(idx >=0 && idx < m_width*m_height);
        typedef typename ChannelSpec::data_t data_t;
        data_t * d = reinterpret_cast<data_t*>(
            m_data + m_offsets[ChannelSpec::IDX]);
        return d[idx];
    }

    // Raw pointer to the pixels
    template <class ChannelSpec>
    inline typename ChannelSpec::data_t * GetDataPointer() const
    {
        typedef typename ChannelSpec::data_t data_t;
        data_t * d = reinterpret_cast<data_t*>(
            m_data + m_offsets[ChannelSpec::IDX]);
        return d;
    }

    // Writes in the i,j parameters the coordinates necessary to access the
    // idx-pixel in the image using ElementAt(int,int), according to the
    // scanline order of the image.
    inline void GetIndices(int idx, int &i, int &j) const { 
        assert(0 <= idx && idx < m_width*m_height);
        i = idx % m_width; 
        j = idx / m_width; 
    }

    // Returns the index (zero based) of the i-th pixel at the j-th scanline
    // using the scanline order of the image.
    inline int GetIndex(int i, int j) const {
        assert(0 <= i && i < m_width);
        assert(0 <= j && j < m_height);
        return m_width*j+i;
    }

    // Gets a pointer to the beginning of the j-th scanline in the specified
    // mode. By default the mode is the same of the image. You should not use
    // data through this pointer for more than a scanline! Instead get a new
    // pointer to the next scanline using this method.
    template <class ChannelSpec>
    inline typename ChannelSpec::data_t * 
    GetScanlinePointer(int j, ScanLineMode mode = TopDown) const
    {
        assert(j >= 0 && j < m_height);

        typedef typename ChannelSpec::data_t data_t;
        data_t * d = reinterpret_cast<data_t*>(
            m_data + m_offsets[ChannelSpec::IDX]);

        // There are only have two modes, so this works nicely
        return (mode == TopDown) ? &(d[j * m_width]) :
                                   &(d[(m_height - j - 1) * m_width]);
    }


protected:
    int m_width;
    int m_height;

    // Data for all each channels
    byte_t * m_data;

    // Offsets for each channel
    size_t m_offsets[NUM_CHANNELS];
};



// Helper typedef for an SoA image with only RGB channels, for bulk operations
class RGBImageSoA : public ImageBaseSoA<float, float, float>
{
public:
    typedef Channel_1 R;
    typedef Channel_2 G;
    typedef Channel_3 B;

    RGBImageSoA() : ImageBaseSoA() {}

    RGBImageSoA(int w, int h) : ImageBaseSoA(w, h) {}

    template <typename PixelRGB>
    RGBImageSoA(const Image<PixelRGB, pcg::TopDown> &img) :
    ImageBaseSoA(img.Width(), img.Height())
    {
        float * r = GetDataPointer<R>();
        float * g = GetDataPointer<G>();
        float * b = GetDataPointer<B>();

        const PixelRGB * pixels = img.GetDataPointer();

        for (int i = 0; i < img.Size(); ++i) {
            const PixelRGB &p = pixels[i];
            r[i] = p.r();
            g[i] = p.g();
            b[i] = p.b();
        }
    }

protected:
    void IMAGEIO_API copyImage(const Image<pcg::Rgba32F, pcg::TopDown> &img);
};

// Constructor specialization
template <>
RGBImageSoA::RGBImageSoA(const Image<Rgba32F, pcg::TopDown> &img) :
ImageBaseSoA(img.Width(), img.Height())
{
    copyImage(img);
}

} // namespace pcg

#endif /* PCG_IMAGESOA_H */
