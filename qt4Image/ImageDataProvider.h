// This is an abstraction used to get the information of a pixel.
// This encapsulates the way stuff actually works and keeps a clean design.
// All pixel coordinates used here are assumed to be TOP-DOWN, so
// handle with care!

#if !defined(IMAGEDATAPROVIDER_H)
#define IMAGEDATAPROVIDER_H

#include <QObject>
#include <QSize>

#include "Image.h"
#include "Rgba32F.h"
#include "LDRPixels.h"

using namespace pcg;

class ImageDataProvider : public QObject {

	Q_OBJECT

protected:
	// The size of the underlying image. Any access beyond these boundaries
	// can make the universe collapse!
	QSize _size;

	// Sets a new size for the provider
	void setSize( const QSize &otherSize );


signals:
	// Signal to be fired when the size changes
	void sizeChanged( QSize newSize );

public:

	// Returns a copy of the size of this provider
	inline QSize size() const { return QSize(_size); }

	inline virtual ~ImageDataProvider() {}

	// This is the magic: it writes the RGB values of the
	// given pixel into the output variables
	virtual void getLdrPixel(int x, int y, unsigned char &rOut, unsigned char &gOut, unsigned char &bOut) const = 0;
	virtual void getHdrPixel(int x, int y, float &rOut, float &gOut, float &bOut) const = 0;

};


// To keep things simple, we provide a concrete implementation here

class ImageIODataProvider : public ImageDataProvider {

	Q_OBJECT

protected:

	// Reference to the backing hdr image
	const Image<Rgba32F> &hdr;

	// Reference to the backing ldr image
	const Image<Bgra8>   &ldr;

public:
	// The constructor just stores the references to the images
	ImageIODataProvider(const Image<Rgba32F> &hdrImage, const Image<Bgra8> &ldrImage);

	// Gets the given pixel from the ldr image
	virtual void getLdrPixel(int x, int y, unsigned char &rOut, unsigned char &gOut, unsigned char &bOut) const;

	// Gets the given pixel from the hdr image
	virtual void getHdrPixel(int x, int y, float &rOut, float &gOut, float &bOut) const;

	// Request to update the size of the provider from the backing images.
	// Of course if their size if different all sorts of terrible things will haunt you
	void update();
};



#endif /* IMAGEDATAPROVIDER_H */
