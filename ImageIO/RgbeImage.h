// Just a convenience class

#if !defined(PCG_RGBEIMAGE_H)
#define PCG_RGBEIMAGE_H

#include "Image.h"
#include "rgbe.h"
#include "RgbeIO.h"

namespace pcg {

	// An RgbeImage: its scanline order is always TopDown:
	// The RGBE file spec says its order is bottom up, but the scanlines
	// are stored top-down by default (that's why the header says "-Y")
	class RgbeImage : public Image<Rgbe, TopDown> {

	public:

		// Create a new image with the specified dimensions. The content of the pixels
		// is undefined! The space for them is already allocated though, you only
		// need to fill them with something useful
		RgbeImage(int width, int height) : Image<Rgbe, TopDown>(width, height) {}

		// Creates a new instance by reading from the specified stream.
		// If something fails an exception is thrown
		RgbeImage(istream &is) {
			RgbeIO::Load(*this, is);
		}

		// Creates a new instance by reading from the specified file.
		// If something fails an exception is thrown. By default
		// it closes the stream after it's done, but you can change
		// that by setting the behaviour explicitly
		RgbeImage(const char *filename, bool closeStream = true) {
			RgbeIO::Load(*this, filename, closeStream);
		}
	};

}


#endif /* PCG_RGBEIMAGE_H */
