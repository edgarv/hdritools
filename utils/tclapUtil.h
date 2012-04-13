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

// Utilities for arguments processing with TCLAP

#pragma once

#include <OpenEXRIO.h>

#include <tclap/Arg.h>

#include <algorithm>
#include <vector>
#include <string>
#include <istream>
#include <ostream>
#include <cctype>
#include <cassert>

#if !defined(_MSC_VER) || _MSC_VER >= 1600
#include <stdint.h>
#endif


namespace util
{

namespace
{
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef unsigned __int32 uint32_t;
#endif

inline uint32_t jHash(const std::string &s)
{
    uint32_t h = 0;
    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
        h = 31 * h + static_cast<uint32_t>(*it);
    }
    return h;
}
} // namespace


// Custom type to recognize the OpenEXR compression types
class Compression
{
public:
    Compression(pcg::OpenEXRIO::Compression v = pcg::OpenEXRIO::None) :
    m_value(v)
    {}

    Compression(const std::string &str) : m_value(INVALID)
    {
        std::string s(str);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        const uint32_t hash = jHash(s);
        switch (hash) {
        case 0x00017662:
            if (s == "b44") m_value = pcg::OpenEXRIO::B44;
            break;
        case 0x0001b1a1:
            if (s == "piz") m_value = pcg::OpenEXRIO::PIZ;
            break;
        case 0x0001b96b:
            if (s == "rle") m_value = pcg::OpenEXRIO::RLE;
            break;
        case 0x0662872c:
            if (s == "pxr24") m_value = pcg::OpenEXRIO::PXR24;
            break;
        case 0x0033af38:
            if (s == "none") m_value = pcg::OpenEXRIO::None;
            break;
        case 0x00390d72:
            if (s == "zips") m_value = pcg::OpenEXRIO::ZIPS;
            break;
        case 0x002d563f:
            if (s == "b44a") m_value = pcg::OpenEXRIO::B44A;
            break;
        case 0x0001d721:
            if (s == "zip") m_value = pcg::OpenEXRIO::ZIP;
            break;
        }
    }

    inline operator pcg::OpenEXRIO::Compression() const {
        assert(m_value != INVALID);
        return static_cast<pcg::OpenEXRIO::Compression>(m_value);
    }

    operator const char*() const {
        switch (m_value) {
        case pcg::OpenEXRIO::B44:
            return "b44";
        case pcg::OpenEXRIO::PIZ:
            return "piz";
        case pcg::OpenEXRIO::RLE:
            return "rle";
        case pcg::OpenEXRIO::PXR24:
            return "pxr24";
        case pcg::OpenEXRIO::None:
            return "none";
        case pcg::OpenEXRIO::ZIPS:
            return "zips";
        case pcg::OpenEXRIO::B44A:
            return "b44a";
        case pcg::OpenEXRIO::ZIP:
            return "zip";
        default:
            return "unknown";
        }
    }

    inline bool operator== (const Compression &v) const {
        return m_value == v.m_value;
    }

    inline bool operator!= (const Compression &v) const {
        return m_value != v.m_value;
    }

    inline bool operator< (const Compression &v) const {
        return m_value < v.m_value;
    }

    inline bool operator> (const Compression &v) const {
        return m_value > v.m_value;
    }

    inline bool operator<= (const Compression &v) const {
        return m_value <= v.m_value;
    }

    inline bool operator>= (const Compression &v) const {
        return m_value >= v.m_value;
    }

    static const std::vector<Compression>& values() {
        return VALUES;
    }


private:

    const static int INVALID = 0x7FFFFFFF;
    const static std::vector<Compression> VALUES;

    static std::vector<Compression> allValues();

    friend std::istream& operator>> (std::istream &is, Compression &v)
    {
        std::string val;
        is >> val;
        Compression tmp(val);
        v.m_value = tmp.m_value;
        return is;
    }

    friend std::ostream& operator<< (std::ostream &os, Compression &v)
    {
        const char* val = static_cast<const char*>(v);
        os << val;
        return os;
    }

    int m_value;
};



class WriteChannels
{
public:
    WriteChannels(pcg::OpenEXRIO::RgbaChannels v = pcg::OpenEXRIO::WRITE_RGBA) :
    m_value(v)
    {}

    WriteChannels(const std::string &str) : m_value(INVALID)
    {
        std::string s(str);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        const uint32_t hash = jHash(s);
        switch (hash) {
        case 0x00356134:
            if (s == "rgba") m_value = pcg::OpenEXRIO::WRITE_RGBA;
            break;
        case 0x00000f08:
            if (s == "ya") m_value = pcg::OpenEXRIO::WRITE_YA;
            break;
        case 0x00000067:
            if (s == "g") m_value = pcg::OpenEXRIO::WRITE_G;
            break;
        case 0x00000062:
            if (s == "b") m_value = pcg::OpenEXRIO::WRITE_B;
            break;
        case 0x00000072:
            if (s == "r") m_value = pcg::OpenEXRIO::WRITE_R;
            break;
        case 0x0001b8cd:
            if (s == "rgb") m_value = pcg::OpenEXRIO::WRITE_RGB;
            break;
        case 0x00000f0a:
            if (s == "yc") m_value = pcg::OpenEXRIO::WRITE_YC;
            break;
        case 0x0001d297:
            if (s == "yca") m_value = pcg::OpenEXRIO::WRITE_YCA;
            break;
        case 0x00000079:
            if (s == "y") m_value = pcg::OpenEXRIO::WRITE_Y;
            break;
        case 0x00000061:
            if (s == "a") m_value = pcg::OpenEXRIO::WRITE_A;
            break;
        }
    }

    inline operator pcg::OpenEXRIO::RgbaChannels() const {
        assert(m_value != INVALID);
        return static_cast<pcg::OpenEXRIO::RgbaChannels>(m_value);
    }

    operator const char*() const {
        switch (m_value) {
        case pcg::OpenEXRIO::WRITE_RGBA:
            return "rgba";
        case pcg::OpenEXRIO::WRITE_YA:
            return "ya";
        case pcg::OpenEXRIO::WRITE_G:
            return "g";
        case pcg::OpenEXRIO::WRITE_B:
            return "b";
        case pcg::OpenEXRIO::WRITE_R:
            return "r";
        case pcg::OpenEXRIO::WRITE_RGB:
            return "rgb";
        case pcg::OpenEXRIO::WRITE_YC:
            return "yc";
        case pcg::OpenEXRIO::WRITE_YCA:
            return "yca";
        case pcg::OpenEXRIO::WRITE_Y:
            return "y";
        case pcg::OpenEXRIO::WRITE_A:
            return "a";
        default:
            return "unknown";
        }
    }

    inline bool operator== (const WriteChannels &v) const {
        return m_value == v.m_value;
    }

    inline bool operator!= (const WriteChannels &v) const {
        return m_value != v.m_value;
    }

    inline bool operator< (const WriteChannels &v) const {
        return m_value < v.m_value;
    }

    inline bool operator> (const WriteChannels &v) const {
        return m_value > v.m_value;
    }

    inline bool operator<= (const WriteChannels &v) const {
        return m_value <= v.m_value;
    }

    inline bool operator>= (const WriteChannels &v) const {
        return m_value >= v.m_value;
    }

    static const std::vector<WriteChannels>& values() {
        return VALUES;
    }


private:

    const static int INVALID = 0x7FFFFFFF;
    const static std::vector<WriteChannels> VALUES;

    static std::vector<WriteChannels> allValues();

    friend std::istream& operator>> (std::istream &is, WriteChannels &v)
    {
        std::string val;
        is >> val;
        WriteChannels tmp(val);
        v.m_value = tmp.m_value;
        return is;
    }

    friend std::ostream& operator<< (std::ostream &os, WriteChannels &v)
    {
        const char* val = static_cast<const char*>(v);
        os << val;
        return os;
    }

    int m_value;
};

} // namespace util



// TCLAP Argument Traits defined in the global namespace
template <>
struct TCLAP::ArgTraits<util::Compression>
{
    typedef ValueLike ValueCategory;
};

template <>
struct TCLAP::ArgTraits<util::WriteChannels>
{
    typedef ValueLike ValueCategory;
};
