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

#include "ZipfileInputFilter.h"

#include "FloatImageProcessor.h"
#include "ImageInfo.h"

#include <QFileInfo>
#include <QDir>

#include <cstdio>
#include <QTextStream>
namespace
{
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream cout(stdout, QIODevice::WriteOnly);
}


struct ZipfileInputFilter::zipfile_t {
    ZipFile *zip;
    QString filename;
    QFileInfo info;
    
    zipfile_t(ZipFile *zipfile, QString name) 
        : zip(zipfile), filename(name), info(name) {}

    ~zipfile_t() {
        delete zip;
    }

    inline QString cleanFilePath(const QString & filename) {
        return QDir::cleanPath(info.dir().filePath(filename));
    }
};


ZipfileInputFilter::ZipfileInputFilter(const QStringList &zipfiles, 
                                       const QString &format,
                                       int filenameOffset) :
    filter(/*is_serial=*/true),
    zipfiles(zipfiles),
    formatStr(format),
    zipfile(NULL),
    offset(filenameOffset)
{
    filename = this->zipfiles.begin();
}


ZipfileInputFilter::zipfile_t* ZipfileInputFilter::nextZipFile() {
    while (filename != zipfiles.end()) {
        cout << "Opening " << *filename << "..." << endl;

        try {
            zipfile_t *zip = new zipfile_t(
                new ZipFile(filename->toLocal8Bit()), *filename);
            ++filename;
            return zip;
        }
        catch (std::exception &e) {
            cerr << "Ooops! " << e.what() << endl;
        }
    }
    // When we get here means we reached the end
    return NULL;
}


ZipEntry* ZipfileInputFilter::nextEntry() {

    for(;;) {
        if (zipfile == NULL) {
            // Gets the next ZipFile, if there is one
            zipfile = nextZipFile();
            if (zipfile == NULL) {
                return NULL;
            }
            entry = zipfile->zip->begin();
        }

        // We have something already open, check if it's the last one,
        // otherwise return and advance
        if (entry != zipfile->zip->end()) {
            return *entry++;
        }
        else {
            // This was the end of this zipfile. Reset the entry pointer
            // and ask for the next one from the next zip file
            delete zipfile;
            zipfile = NULL;
        }

    }
}


void* ZipfileInputFilter::operator()(void*) {

    // Gets the next entry, this is also our condition to continue
    for(;;) {
        try {
            // We try to load the file from the current entry, convert it into the
            // suitable Rgba32F image and submit it to the next stage

            ZipEntry *entry = nextEntry();
            if (entry == NULL) {
                return NULL;
            }

            // Make the target name relative to the parent of the zip file
            QString entryName = zipfile->cleanFilePath(entry->GetName());

            return FloatImageProcessor::load(entryName, 
                zipfile->zip->GetInputStream(entry), formatStr, offset);

        }
        catch(std::exception &e) {
            cerr << "Ooops! " << e.what() << endl;
        }
    }
}
