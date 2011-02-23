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

#if !defined(ZIPFILEINPUTFILTER_H)
#define ZIPFILEINPUTFILTER_H

#include <ZipFile.h>

#include <QStringList>


// TBB import for the filter stuff
#include <tbb/pipeline.h>

using std::vector;
using std::string;
using pcg::ZipFile;
using pcg::ZipEntry;


// Input filter class. It opens each zip file from the input and
// sends it through the pipeline
class ZipfileInputFilter : public tbb::filter {

private:

    // Helper structure to keep an opened zipfile and its original filename
    struct zipfile_t;

    // The list of files to process
    const QStringList &zipfiles;

    // The output format to use. It is assumed to be one of those returned by
    // QImageWriter::supportedImageFormats() and lowercase
    const QString formatStr;

    // Iterator to the list of files
    QStringList::const_iterator filename;

    // Each entry in the current zipfile
    ZipFile::const_iterator entry;
    zipfile_t *zipfile;

    zipfile_t* nextZipFile();

    ZipEntry* nextEntry();

    // The optional offset to add to the numeric filenames
    const int offset;


public:
    ZipfileInputFilter(const QStringList &zipfiles, const QString &format, int filenameOffset = 0);

    // This will be invoked serially, its job is to return pointers to the opened zipfiles.
    // The next filter in the chain must close the zip files when it's done!
    void* operator()(void*);
};


#endif /* ZIPFILEINPUTFILTER_H */
