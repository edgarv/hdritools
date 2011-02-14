#include "ToneMappingFilter.h"
#include "ImageInfo.h"

#include <QImage>
#include <PngIO.h>

#include <cstdio>
#include <QTextStream>
namespace
{
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream cout(stdout, QIODevice::WriteOnly);
}


ToneMappingFilter::ToneMappingFilter(const ToneMapper &toneMapper, bool useBpp16) : 
	filter(/*is_serial=*/false),
	toneMapper(toneMapper),
	useBpp16(useBpp16)
{}

void* ToneMappingFilter::operator()(void* item)
{
	try {
		ImageInfo *info = static_cast<ImageInfo *>(item);
		assert( info != NULL );

		// Abort if its invalid
		if (! info->isValid) {
			delete info;
			return NULL;
		}

		const Image<Rgba32F> &floatImage = *(info->img);

		// Allocates the LDR Image and tonemaps it
		if (!useBpp16) {
			Image<Bgra8> ldrImage(floatImage.Width(), floatImage.Height());
			toneMapper.ToneMap(ldrImage, floatImage);

			// Finally wraps the ldrImage into a QImage and saves it 
            // with the specified name
			QImage qImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), 
				ldrImage.Width(), ldrImage.Height(), QImage::Format_RGB32);

			// TODO: The name might contain a path, so should we create it if
            // it doesn't exist?
			if ( !qImage.save(info->filename) ) {
				cerr << "Ooops! unable to save " << info->filename << ". Are you sure it's valid?" << endl;
			}
		}
		else {
			Image<Rgba16> ldrImage(floatImage.Width(), floatImage.Height());
			toneMapper.ToneMap(ldrImage, floatImage);

			try {
				PngIO::Save(ldrImage, info->filename.toLocal8Bit(),
                    toneMapper.isSRGB(), toneMapper.InvGamma());
			}
			catch (std::exception &e) {
				cerr << "Ooops! unable to save " << info->filename << ": " << e.what() << endl;
			}
		}

		cout << info->originalFile << " -> " << info->filename << endl;

		// Deletes the info structure when it's done
		delete info;
	}
	catch(std::exception &e) {
		cerr << "Ooops! " << e.what() << endl;
	}

	// Always returns null, as it's in the last part of the pipeline
	return NULL;
}
