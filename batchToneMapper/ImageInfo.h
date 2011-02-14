#if !defined IMAGEINFO_H
#define IMAGEINFO_H

#include <Image.h>
#include <Rgba32F.h>
#include <QString>

using namespace pcg;

/** Small Transfer object to pass info between the pipeline stages */

struct ImageInfo {
    const Image<Rgba32F> *img;
    const QString originalFile;
    const QString filename;
    const bool isValid;

    ImageInfo(Image<Rgba32F> *image, const char *input, const char *outname) :
        img(image), originalFile(input), filename(outname), isValid(true) {}

    ImageInfo(Image<Rgba32F> *image, const QString &input, const QString &outname) :
        img(image), originalFile(input), filename(outname), isValid(true) {}

    ImageInfo() : img(NULL), isValid(false) {}

    ~ImageInfo() {
        if (img != NULL) {
            delete img;
        }
    }
};

#endif /* IMAGEINFO_H */
