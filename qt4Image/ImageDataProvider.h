// This is an abstraction used to get the information of an image.
// This encapsulates the way stuff actually works and keeps a clean design.
// All pixel coordinates used here are assumed to be TOP-DOWN, so
// handle with care!

#if !defined(IMAGEDATAPROVIDER_H)
#define IMAGEDATAPROVIDER_H

#include <QObject>
#include <QSize>
#include <QPair>

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

    // Range for the white point
    typedef QPair<double,double> range_t;
    range_t _whitePointRange;

    // Sets a new white point range
    void setWhitePointRange( const range_t &otherRange );

    inline void setWhitePointRange( double a, double b ) {
        setWhitePointRange(qMakePair(a,b));
    }


signals:
    // Signal to be fired when the size changes
    void sizeChanged( QSize newSize );

    // Signal to the indicate that the white point range changed
    void whitePointRangeChanged ( double whitePointMin, double whitePointMax );

public:

    // Returns a copy of the size of this provider
    inline QSize size() const { return QSize(_size); }

    inline virtual ~ImageDataProvider() {}

    // This is the magic: it writes the RGB values of the
    // given pixel into the output variables
    virtual void getLdrPixel(int x, int y, unsigned char &rOut, unsigned char &gOut, unsigned char &bOut) const = 0;
    virtual void getHdrPixel(int x, int y, float &rOut, float &gOut, float &bOut) const = 0;

    // This function will get sane tone mapping defaults
    virtual void getToneMapDefaults(double &whitePointOut, double &keyOut) const = 0;

    // Returns the average log luminance
    virtual double avgLogLuminance() const = 0;
};


// To keep things simple, we provide a concrete implementation here

class ImageIODataProvider : public ImageDataProvider {

    Q_OBJECT

protected:

    // Reference to the backing hdr image
    const Image<Rgba32F> &hdr;

    // Reference to the backing ldr image
    const Image<Bgra8>   &ldr;

    // Good tone mapping defaults
    double whitePoint;
    double key;
    double lw;

public:
    // The constructor just stores the references to the images
    ImageIODataProvider(const Image<Rgba32F> &hdrImage, const Image<Bgra8> &ldrImage);

    // Gets the given pixel from the ldr image
    virtual void getLdrPixel(int x, int y, unsigned char &rOut, unsigned char &gOut, unsigned char &bOut) const;

    // Gets the given pixel from the hdr image
    virtual void getHdrPixel(int x, int y, float &rOut, float &gOut, float &bOut) const;

    // Gets sane tone mapping defaults
    virtual void getToneMapDefaults(double &whitePointOut, double &keyOut) const;

    // Returns the average log luminance
    virtual double avgLogLuminance() const;

public slots:
    // Request to update the size of the provider from the backing images.
    // Of course if their size if different all sorts of terrible things will haunt you
    void update();
};



#endif /* IMAGEDATAPROVIDER_H */
