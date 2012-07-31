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

#include "LoadHDR.h"
#include "StdAfx.h"
#include "Exception.h"
#include "OpenEXRIO.h"
#include "RgbeIO.h"
#include "PfmIO.h"

#include <fstream>
#include <sstream>
#include <cassert>


using namespace pcg;


namespace
{

template <class ImageCls>
void LoadHDRImpl(ImageCls &img, std::istream &is)
{
    // Try to read the first 4 bytes to get the magic numbers
    const std::istream::pos_type origPosition = is.tellg();
    if (is.fail()) {
        throw IOException("Could not get the position of the stream.");
    }

    union {
        int32_t magic;
        char magic_buffer[4];
    };
    magic = 0;

    is.read(magic_buffer, sizeof(magic_buffer));
    if (!is.good()) {
        throw IOException("Could not read the magic number.");
    }

    // Known types
    enum {
        OpenEXR,
        RGBE,
        PFM,
        UNKNOWN
    };
    int type = UNKNOWN;

    if (magic == 20000630) {
        // Assume OpenEXR
        type = OpenEXR;
    } else {
        // RGBE and PFM only use the first two bytes
        magic &= 0xFFFF;
        if (magic == 16163) {
            // Assume RGBE: "#?"
            type = RGBE;
        } else if (magic == 18000 || magic == 26192) {
            // Assume PFM: "PF" or "Pf"
            type = PFM;
        } else {
            std::stringstream ss;
            ss << "Unknown magic number ["
               << std::showbase << std::hex
               << (int)magic_buffer[0] << ", " << (int)magic_buffer[1] << "]";
            throw UnkownFileType(ss.str());
        }
    }
    assert(type != UNKNOWN);

    is.seekg(origPosition);
    if (!is) {
        throw IOException("Could not reposition the stream.");
    }

    switch (type) {
    case OpenEXR:
        pcg::OpenEXRIO::Load(img, is);
        break;
    case RGBE:
        pcg::RgbeIO::Load(img, is);
        break;
    case PFM:
        pcg::PfmIO::Load(img, is);
        break;
    }
}



template <class ImageCls>
void LoadHDRImpl(ImageCls &img, const char *filename)
{
    if (filename == NULL) {
        throw IllegalArgumentException("The filename cannot be null.");
    }

    std::ifstream is(filename, std::ios::binary);

    if(!is) {
        std::string msg("Could not open the file \"");
        msg += filename;
        msg += "\".";
        throw IOException(msg);
    }

    LoadHDRImpl(img, is);
}

} // namespace



void pcg::LoadHDR(Image<Rgba32F,TopDown>  &img, std::istream &is) {
    LoadHDRImpl(img, is);
}
void pcg::LoadHDR(Image<Rgba32F,TopDown> &img, const char *filename) {
    LoadHDRImpl(img, filename);
}
void pcg::LoadHDR(RGBAImageSoA &img, std::istream &is) {
    LoadHDRImpl(img, is);
}
void pcg::LoadHDR(RGBAImageSoA &img, const char *filename) {
    LoadHDRImpl(img, filename);
}
