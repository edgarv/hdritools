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

#include "tclapUtil.h"


std::vector<util::Compression> util::Compression::allValues()
{
    // C++11 Initializer lists would make this much easier
    std::vector<util::Compression> vec;
    vec.push_back(util::Compression(pcg::OpenEXRIO::None));
    vec.push_back(util::Compression(pcg::OpenEXRIO::RLE));
    vec.push_back(util::Compression(pcg::OpenEXRIO::ZIPS));
    vec.push_back(util::Compression(pcg::OpenEXRIO::ZIP));
    vec.push_back(util::Compression(pcg::OpenEXRIO::PIZ));
    vec.push_back(util::Compression(pcg::OpenEXRIO::PXR24));
    vec.push_back(util::Compression(pcg::OpenEXRIO::B44));
    vec.push_back(util::Compression(pcg::OpenEXRIO::B44A));
    return vec;
}

const std::vector<util::Compression> util::Compression::VALUES =
    util::Compression::allValues();



std::vector<util::WriteChannels> util::WriteChannels::allValues()
{
    // C++11 Initializer lists would make this much easier
    std::vector<util::WriteChannels> vec;
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_R));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_G));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_B));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_A));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_RGB));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_RGBA));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_YC));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_YCA));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_Y));
    vec.push_back(util::WriteChannels(pcg::OpenEXRIO::WRITE_YA));
    return vec;
}

const std::vector<util::WriteChannels> util::WriteChannels::VALUES =
    util::WriteChannels::allValues();
