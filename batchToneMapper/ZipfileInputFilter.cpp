#include "ZipfileInputFilter.h"

#include "FloatImageProcessor.h"
#include "ImageInfo.h"

#include <cstdio>
#include <QTextStream>
namespace
{
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream cout(stdout, QIODevice::WriteOnly);
}


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


ZipFile* ZipfileInputFilter::nextZipFile() {
    while (filename != zipfiles.end()) {
        cout << "Opening " << *filename << "..." << endl;

        try {
            ZipFile *zip = new ZipFile( (filename++)->toLocal8Bit());
            return zip;
        }
        catch (exception &e) {
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
            entry = zipfile->begin();
        }

        // We have something already open, check if it's the last one,
        // otherwise return and advance
        if (entry != zipfile->end()) {
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

            return FloatImageProcessor::load(entry->GetName(), 
                zipfile->GetInputStream(entry), formatStr, offset);

        }
        catch(std::exception &e) {
            cerr << "Ooops! " << e.what() << endl;
        }
    }
}
