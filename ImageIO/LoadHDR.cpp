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

#if defined(_WIN32)
# ifdef NOMINMAX
#  undef NOMINMAX
# endif
# define NOMINMAX
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
#endif


using namespace pcg;


namespace
{
// Returns the 'magic number' of the two characters when read as little endian
template <size_t N>
inline int magic(const char (&m)[N]) {
    return (m[0] | (m[1] << 8));
}

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
    } u = {0};

    is.read(u.magic_buffer, sizeof(u.magic_buffer));
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

    if (u.magic == 20000630) {
        // Assume OpenEXR
        type = OpenEXR;
    } else {
        // RGBE and PFM only use the first two bytes
        u.magic &= 0xFFFF;
        if (u.magic == magic("#?")) {
            type = RGBE;
        } else if (u.magic == magic("PF") || magic("Pf")) {
            type = PFM;
        } else {
            std::stringstream ss;
            ss << "Unknown magic number [" << std::showbase << std::hex
               << static_cast<int>(u.magic_buffer[0]) << ", "
               << static_cast<int>(u.magic_buffer[1]) << "]";
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



inline const char* toPrintable(const char* str) {
    return str;
}

#if defined(_WIN32)
inline std::string toPrintable(const wchar_t* str) {
    // Code after
    // https://buildsecurityin.us-cert.gov/bsi/articles/knowledge/coding/870-BSI.html

    int nLenUnicode = ::lstrlenW(str);
    int nLen = ::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS,
        str, nLenUnicode, // string to convert
        NULL, 0,          // no output buffer since we are calculating length
        NULL, NULL);      // use default unrepresented char replacement

    if (nLen == 0) {
        return "";
    }

    // Once more, with feeling
    std::vector<char> result(nLen); // nLen includes the null character
    nLen = ::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, str, nLenUnicode,
        &result[0], nLen, NULL, NULL);
    if (nLen != 0) {
        return std::string(result.begin(), result.end());
    } else {
        return "";
    }
}
#endif

template <class ImageCls, typename CharT>
void LoadHDRImpl(ImageCls &img, const CharT *filename)
{
    if (filename == NULL) {
        throw IllegalArgumentException("The filename cannot be null.");
    }

    std::ifstream is(filename, std::ios::binary);

    if(!is) {
        std::string msg("Could not open the file \"");
        msg += toPrintable(filename);
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

#if defined(_WIN32)
void pcg::LoadHDR(Image<Rgba32F,TopDown> &img, const wchar_t *filename) {
    LoadHDRImpl(img, filename);
}
void pcg::LoadHDR(RGBAImageSoA &img, const wchar_t *filename) {
    LoadHDRImpl(img, filename);
}
#endif
