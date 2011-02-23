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

// The core of the application, to decouple parameter
// parsing and those other ugly things

#if !defined(BATCH_TONE_MAPPER_H)
#define BATCH_TONE_MAPPER_H

#include "ToneMappingFilter.h"

#include <ostream>

#include <QString>
#include <QStringList>

#include <ToneMapper.h>

class BatchToneMapper {

    friend std::ostream& operator<<(std::ostream& os, const BatchToneMapper& b);

public:
    BatchToneMapper(const QStringList& files, bool bpp16);

    // Sets up the tonemapper with a specific gamma
    void setupToneMapper(float exposure, float gamma);

    // Sets up the tonemapper just with the exposure, using sRGB
    void setupToneMapper(float exposure);

    // Specifies the values for the Reinhard02 TMO. If a parameter is to be
    // determined automatically just set it to the value returned by
    // ToneMappingFilter::AutoParam(). By default all parameters are automatic
    void setReinhard02Params(float key, float whitePoint, float logLumAvg);

    // Sets up a specific TMO technique to use. The default is EXPOSURE
    void setTechnique(pcg::TmoTechnique tmo) {
        technique = tmo;
    }

    // To know if it has any valid files to process when
    // execute() is called.
    bool hasWork() const {
        return zipFiles.size() > 0 || hdrFiles.size() > 0;
    }

    // Main method: once everything is setup, process the files
    void execute();

    // Sets the offset for the filenames (it's zero by default)
    void setOffset(int newOffset) {
        offset = newOffset;
    }

    // Tries to set the format. It must be one of those exactly
    // as returned from Util::supportedWriteImageFormats().
    // If it isn't it doesn't do anything and the current format remains
    void setFormat(const QString & newFormat);

    // Gets the default format string
    static const QString getDefaultFormat();

    // Gets the version string
    static const QString getVersion();

private:

    const static int LUT_SIZE = 8192;

    // General parameters
    int offset;
    QString format;

    // Tone mapper
    pcg::ToneMapper toneMapper;

    // Number of tokens in the pipeline
    int tokens;

    // Whether to use 16 bpp in the LDR files or the default 8
    const bool useBpp16;

    // Lists of files to process
    QStringList zipFiles;
    QStringList hdrFiles;

    // Tone mapping settings
    pcg::TmoTechnique technique;
    float key;
    float whitePoint;
    float logLumAvg;

    // Cache the default format
    static QString defaultFormat;

    // Cache the version string
    static QString version;


    // Utility method to separate the elements from a raw file list into a
    // list of zip files and other of HDR files. The two new lists contains
    // copies of the strings. Before adding them to the list this method
    // checks that the files actually exists and are readable.
    void classifyFiles(const QStringList & files);

    // Creates the new tone mapping filter. The caller is responsible
    // for deletion of the returned object
    ToneMappingFilter* createToneMappingFilter();

    // Individual pipelines
    void executeZip();
    void executeHdr();
};


#endif /* BATCH_TONE_MAPPER_H */
