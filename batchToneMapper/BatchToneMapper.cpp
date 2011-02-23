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

#include "BatchToneMapper.h"

// Misc utitilities
#include "Util.h"

// Pipeline filters
#include "FileInputFilter.h"
#include "ZipfileInputFilter.h"
#include "ToneMappingFilter.h"

#include <HDRITools_version.h>
#include <QString>

#include <cstdio>
#include <QTextStream>
namespace
{
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream cout(stdout, QIODevice::WriteOnly);
}

using namespace std;


QString BatchToneMapper::defaultFormat;
QString BatchToneMapper::version;


BatchToneMapper::BatchToneMapper(const QStringList& files, bool bpp16) :
offset(0), format(!bpp16 ? getDefaultFormat() : "png"),
toneMapper(LUT_SIZE), tokens(0), useBpp16(bpp16), technique(pcg::EXPOSURE),
key(ToneMappingFilter::AutoParam()),whitePoint(ToneMappingFilter::AutoParam()),
logLumAvg(ToneMappingFilter::AutoParam())
{
    classifyFiles(files);

    // Runs the pipeline with 2.5x the number of working threads
    tokens = (int)(2.5f * Util::numberOfProcessors());
    assert(tokens > 0);
}


void BatchToneMapper::setupToneMapper(float exposure, float gamma) {
    toneMapper.SetExposure(exposure);
    toneMapper.SetGamma(gamma);
    toneMapper.SetSRGB(false);
}


void BatchToneMapper::setupToneMapper(float exposure) {
    toneMapper.SetExposure(exposure);
    toneMapper.SetSRGB(true);
}


void BatchToneMapper::setReinhard02Params(float k, float wp, float lw)
{
    key        = k;
    whitePoint = wp;
    logLumAvg  = lw;
}


void BatchToneMapper::setFormat(const QString & newFormat)
{
    if (!useBpp16) {
        // There are not many formats, so this is not that bad
        const QStringList & formats = Util::supportedWriteImageFormats();
        for(QStringList::const_iterator it = formats.constBegin();
            it != formats.constEnd(); ++it)
        {
            if (newFormat == *it) {
                format = newFormat;
                return;
            }
        }
        cerr << "Warning: unsupported format \"" << newFormat << "\"" << endl;
    }
    else {
        // Only png is supported
        if (newFormat != "png" && newFormat != Util::PNG16_FORMAT_STR) {
            cerr << "Warning: unsupported format for bpp16 \"" << newFormat 
                 << "\". Using png." << endl;
        }
        format = "png";
    }
}


void BatchToneMapper::execute() {

    if (!zipFiles.isEmpty()) {
        executeZip();
        cout << "All Zip files have been processed." << endl;
    }

    if (!hdrFiles.isEmpty()) {
        executeHdr();
        cout << "All HDR files have been processed." << endl;
    }
}


const QString BatchToneMapper::getDefaultFormat() {

    if (defaultFormat.isEmpty()) {

        // Default to png, jpg, or whatever is first
        if (Util::isPngSupported()) {
            defaultFormat = "png";
        }
        else if (Util::isJpgSupported()) {
            defaultFormat = "jpg";
        }
        else {
            defaultFormat = (Util::supportedWriteImageFormats())[0];
        }
    }

    return defaultFormat;
}


const QString BatchToneMapper::getVersion()
{
    if (version.isEmpty()) {
#if HDRITOOLS_HAS_VALID_REV
        version = QString("%1-hg%2")
            .arg(pcg::version::versionString())
            .arg(pcg::version::globalRevision());
#else
        version = pcg::version::versionString();
#endif
    }
    return version;
}


void BatchToneMapper::classifyFiles(const QStringList & files) {

    for(QStringList::const_iterator it = files.constBegin();
        it != files.constEnd(); ++it)
    {
        bool isZip;
        bool isHdr;
        const bool readable = Util::isReadable(*it, isZip, isHdr);

        if (!readable) {
            cerr << "Warning: cannot read " << *it << "." << endl;
        }
        else {
            if (isZip) {
                zipFiles.append(*it);
            }
            else if (isHdr) {
                hdrFiles.append(*it);
            }
            else {
                cerr << "Warning: the file " << *it << " doesn't have a recognized type." << endl;
            }
        }
    }
}


ToneMappingFilter* BatchToneMapper::createToneMappingFilter()
{
    ToneMappingFilter *filter;
    if (technique == pcg::EXPOSURE) {
        filter = new ToneMappingFilter(toneMapper, useBpp16);
    } else if (technique == pcg::REINHARD02) {
        filter = new ToneMappingFilter(toneMapper, useBpp16,
            key, whitePoint, logLumAvg);
    } else {
        filter = NULL;
    }
    Q_ASSERT(filter != NULL);
    return filter;
}

void BatchToneMapper::executeZip() {
    
    // Creates and uses a TBB pipeline
    tbb::pipeline pipeline;

    // Adds the zip-reading filter
    ZipfileInputFilter zipFilter(zipFiles, format, offset);
    pipeline.add_filter(zipFilter);

    // Adds the tone mapping filter
    ToneMappingFilter *toneFilter = createToneMappingFilter();
    pipeline.add_filter(*toneFilter);

    
    pipeline.run(tokens);

    // Clears the filters after it's done
    pipeline.clear();
    delete toneFilter;
}


void BatchToneMapper::executeHdr() {

    // Creates and uses a TBB pipeline
    tbb::pipeline pipeline;

    // Adds the file input and loading filters
    FileInputFilter  inputFilter(hdrFiles);
    FileLoaderFilter loaderFilter(format, offset);
    
    pipeline.add_filter(inputFilter);
    pipeline.add_filter(loaderFilter);

    // Adds the tone mapping filter
    ToneMappingFilter *toneFilter = createToneMappingFilter();
    pipeline.add_filter(*toneFilter);

    pipeline.run(tokens);

    // Clears the filters after it's done
    pipeline.clear();
    delete toneFilter;
}


ostream& operator<<(ostream& os, const BatchToneMapper& b)
{
    os << "BatchToneMapper: LUT size " << BatchToneMapper::LUT_SIZE 
       << ", using " << b.tokens << " pipeline tokens." << endl
       << "Conversion parameters:" << endl
       << "  Exposure:  " << b.toneMapper.Exposure() << endl
       << "  Gamma:     " ;

    if ( b.toneMapper.isSRGB() ) {
        os << "NA (using sRGB)" << endl;
    }
    else {
       os << b.toneMapper.Gamma() << endl;
    }

    os << "  Offset:    " << b.offset << endl
       << "  BPP:       " << (b.useBpp16 ? 16 : 8) << endl
       << "  Format:    " << b.format.toStdString() << endl;
    if(!b.zipFiles.isEmpty()) {
        os << "  Zip Files: ";
        for (QStringList::const_iterator it = b.zipFiles.constBegin(); 
             it != b.zipFiles.constEnd(); ++it) {
            os << it->toStdString() << ' ';
        }
        os << endl;
    }
    if(!b.hdrFiles.isEmpty()) {
        os << "  HDR Files: ";
        for (QStringList::const_iterator it = b.hdrFiles.constBegin();
             it != b.hdrFiles.constEnd(); ++it) {
            os << it->toStdString() << ' ';
        }
        os << endl;
    }

    return os;
}
