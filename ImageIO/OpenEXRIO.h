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

// Base functions used to load an OpenEXR file which
// is assumed to be in the RGB format, anything else
// will be supported.

#if !defined (PCG_OPENEXRIO_H)
#define PCG_OPENEXRIO_H

#include "ImageIO.h"
#include "Image.h"
#include "Rgba32F.h"
#include "ImageSoA.h"

#include <istream>

namespace pcg {

    // The base class has only static methods
    class OpenEXRIO {

    public:

        // An enum for the different types of compression available when saving the file
        // (Valid as of OpenEXR 1.7.1)
        enum Compression {
            None,
            RLE,
            ZIPS,
            ZIP,
            PIZ,
            PXR24,
            B44,
            B44A
        };

        // Modes indicating which channels to write
        enum RgbaChannels {
            WRITE_R,
            WRITE_G,
            WRITE_B,
            WRITE_A,
            WRITE_RGB,
            WRITE_RGBA,
            WRITE_YC,   // Luminance, chroma
            WRITE_YCA,  // Luminance, chroma, alpha
            WRITE_Y,    // Luminance only
            WRITE_YA    // Luminance, alpha
        };

        static void Load(Image<Rgba32F, TopDown> &img, const char *filename) {
            LoadHelper(img, filename);
        }

        static void Load(Image<Rgba32F, TopDown> &img, std::istream &is) {
            LoadHelper(img, is);
        }

        static void Load(Image<Rgba32F, BottomUp> &img, const char *filename) {
            // TODO Fix this lazy, lame method
            Image<Rgba32F, TopDown> tmp;
            LoadHelper(tmp, filename);
            img.Alloc(tmp.Width(), tmp.Height());
            
            // Stupid copy!
            for (int i = 0; i < img.Height(); ++i) {

                const Rgba32F *src = tmp.GetScanlinePointer(i, BottomUp);
                Rgba32F *dest = img.GetScanlinePointer(i, BottomUp);

                for (int j = 0; j < img.Width(); ++j) {
                    _mm_stream_ps((float*)dest[j], src[j]);
                }
            }
        }

        static void IMAGEIO_API Load(RGBAImageSoA& img, std::istream& is);

        static void IMAGEIO_API Load(RGBAImageSoA& img, const char* filename);

        // To save the images with a different scanline order we only set a flag!
        static void IMAGEIO_API Save(const Image<Rgba32F, TopDown> &img, std::ofstream& os,
            Compression compression = ZIP);
        static void IMAGEIO_API Save(const Image<Rgba32F, BottomUp> &img, std::ofstream& os,
            Compression compression = ZIP);
        static void IMAGEIO_API Save(const Image<Rgba32F, TopDown> &img, const char *filename,
            Compression compression = ZIP);
        static void IMAGEIO_API Save(const Image<Rgba32F, BottomUp> &img, const char *filename,
            Compression compression = ZIP);

        // New methods to specify the channels to write, to preserve binary compatibility
        static void IMAGEIO_API Save(const Image<Rgba32F, TopDown> &img, std::ofstream& os,
            RgbaChannels rgbaChannels, Compression compression = ZIP);
        static void IMAGEIO_API Save(const Image<Rgba32F, BottomUp> &img, std::ofstream& os,
            RgbaChannels rgbaChannels, Compression compression = ZIP);
        static void IMAGEIO_API Save(const Image<Rgba32F, TopDown> &img, const char *filename,
            RgbaChannels rgbaChannels, Compression compression = ZIP);
        static void IMAGEIO_API Save(const Image<Rgba32F, BottomUp> &img, const char *filename,
            RgbaChannels rgbaChannels, Compression compression = ZIP);

        // Save SoA images
        static void IMAGEIO_API Save(const RGBAImageSoA& img, std::ofstream& os,
            RgbaChannels rgbaChannels, Compression compression = ZIP);
        inline static void Save(const RGBAImageSoA& img, std::ofstream& os,
            Compression comression = ZIP) {
            Save(img, os, WRITE_RGB, ZIP);
        }
        static void IMAGEIO_API Save(const RGBAImageSoA& img, const char* filename,
            RgbaChannels rgbaChannels, Compression compression = ZIP);
        inline static void Save(const RGBAImageSoA& img, const char* filename,
            Compression comression = ZIP) {
            Save(img, filename, WRITE_RGB, ZIP);
        }

        // Lazily set the number of threads to use for OpenEXR IO
        static void IMAGEIO_API setNumThreads(int num);

    private:
        static void IMAGEIO_API LoadHelper(Image<Rgba32F, TopDown> &img, const char *filename);
        static void IMAGEIO_API LoadHelper(Image<Rgba32F, TopDown> &img, std::istream &is);

        // Declare the super utility functions for saving
        template <ScanLineMode S, class OStreamArgT>
        static void SaveHelper(const Image<Rgba32F, S> &image, OStreamArgT &ostreamArg,
            Compression compression, RgbaChannels rgbaChannels);
        
        static int numThreads;
    };
}


#endif
