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

// Small program to open HDR files and compress them into OpenEXR files
// using either full RGB[A] channels or YC[A] (chroma subsampled)

#include "tclapUtil.h"

#include <HDRITools_version.h>

#include <LoadHDR.h>
#include <OpenEXRIO.h>

#include <tclap/CmdLine.h>

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>



namespace
{

enum ReturnCode {
    SUCCESS = 0,
    UNKNOWN_TYPE,
    OTHER_ERROR
};


// Helper structure with the parameters to avoid huge method signatures
struct Params
{
    std::string src;
    std::string dest;
    pcg::OpenEXRIO::RgbaChannels channels;
    pcg::OpenEXRIO::Compression compression;
};


int process(const Params& p)
{
    try {
        pcg::Image<pcg::Rgba32F> img;
        pcg::LoadHDR(img, p.src);
        pcg::OpenEXRIO::Save(img, p.dest.c_str(), p.channels, p.compression);
        return SUCCESS;
    }
    catch (pcg::UnkownFileType &ex1) {
        std::cerr << "Unknown HDR file type: " << p.src << std::endl
                  << "  " << ex1.what() << std::endl;
        return UNKNOWN_TYPE;
    }
    catch (pcg::Exception &ex2) {
        std::cerr << "Error: " << ex2.what() << std::endl;
        return OTHER_ERROR;
    }
}


void parseArgs(int argc, char **argv, Params &params)
{
    using namespace pcg;
    using util::Compression;
    using util::WriteChannels;

    std::string version(version::versionString());
#if HDRITOOLS_HAS_VALID_REV
    version += "-hg";
    version += version::globalRevision();
#endif

    TCLAP::CmdLine cmdline("Write a HDR image into the OpenEXR format, "
        "selecting the channels to write and the compression to use.",' ',version);
    
    const Compression cDefault(OpenEXRIO::ZIP);
    TCLAP::ValuesConstraint<Compression> allowedCompressions(
        const_cast<std::vector<Compression>& >(Compression::values()));
    TCLAP::ValueArg<Compression> compressionArg("c", "compression",
        "OpenEXR compression to use (default: zip).",
        false, cDefault, &allowedCompressions);
    cmdline.add(compressionArg);

    const WriteChannels channelsDefault(OpenEXRIO::WRITE_RGBA);
    TCLAP::ValuesConstraint<WriteChannels> allowedChannels(
        const_cast<std::vector<WriteChannels>& >(WriteChannels::values()));
    TCLAP::ValueArg<WriteChannels> channelsArg("w", "write_channels",
        "Channels to write in the target file (default: rgba).",
        false, channelsDefault, &allowedChannels);
    cmdline.add(channelsArg);

    TCLAP::UnlabeledValueArg<std::string> srcArg("source_file",
        "Source HDR file [exr|rgbe/hdr|pfm].", true, "", "source");
    cmdline.add(srcArg);

    TCLAP::UnlabeledValueArg<std::string> destArg("destination_file",
        "Destination OpenEXR file.", true, "", "destination");
    cmdline.add(destArg);

    // Parse the arguments and assign the values
    cmdline.parse(argc, argv);
    params.src         = srcArg.getValue();
    params.dest        = destArg.getValue();
    params.channels    = channelsArg.getValue();
    params.compression = compressionArg.getValue();
}

} // namespace



int main(int argc, char **argv)
{
    Params params;
    parseArgs(argc, argv, params);
    return process(params);
}
