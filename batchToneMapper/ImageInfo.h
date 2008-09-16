#if !defined IMAGEINFO_H
#define IMAGEINFO_H

#include <Image.h>
#include <Rgba32F.h>
#include <string>

using namespace pcg;
using std::string;

/** Small Transfer object to pass info between the pipeline stages */

struct ImageInfo {
	const Image<Rgba32F> *img;
	const string originalFile;
	const string filename;
	const bool isValid;

	ImageInfo(Image<Rgba32F> *image, const char *input, const char *outname) :
		img(image), originalFile(input), filename(outname), isValid(true) {}

	ImageInfo(Image<Rgba32F> *image, const string &input, const string &outname) :
		img(image), originalFile(input), filename(outname), isValid(true) {}

	ImageInfo() : img(NULL), isValid(false) {}

	~ImageInfo() {
		if (img != NULL) {
			delete img;
		}
	}
};

#endif /* IMAGEINFO_H */
