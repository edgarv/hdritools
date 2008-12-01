#if !defined HDRIMAGEDISPLAY_H
#define HDRIMAGEDISPLAY_H

#include <QWidget>

// ImageIO includes
#include "rgbe.h"
#include "Rgba32F.h"
#include "LDRPixels.h"
#include "Image.h"
#include "RgbeIO.h"
#include "OpenEXRIO.h"
#include "ImageComparator.h"

#include "ToneMapper.h"

#include "ImageDataProvider.h"

using namespace pcg;

// Widget to encapsulate the loading and display of the tone mapped images
class HDRImageDisplay : public QWidget {

	Q_OBJECT

public:

	enum HdrResult {
		NoError,
		UnknownType,
		ExceptionError,
		SizeMissmatch,
		IllegalState
	};

	HDRImageDisplay() : scaleFactor(1), toneMapper(0.0f, 4096), needsToneMap(true),
		dataProvider(hdrImage, ldrImage)
	{
		// By default we want to receive events whenever the mouse moves around
		setMouseTracking(true);
/*
		sizeAux.setWidth(ldrImage.Width());
		sizeAux.setHeight(ldrImage.Height());

		resize(sizeAux);
		*/
	}

	virtual QSize sizeHint() const {

		return scaleFactor*QSize(ldrImage.Width(), ldrImage.Height());
	}

	QSizePolicy sizePolicy () const {
		return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	}


	bool open(const QString &fileName, HdrResult * result = 0);

	bool compareTo(const QString &fileName, ImageComparator::Type compareMethod,
		HdrResult * result = 0);


	bool save(const QString & fileName);

	void setGamma(float gamma)
	{
		if (gamma != toneMapper.Gamma()) {
			toneMapper.SetGamma(gamma);
			needsToneMap = true;
			update();
		}
	}

	void setExposure(float exposure)
	{
		if (exposure != toneMapper.Exposure()) {
			toneMapper.SetExposure(exposure);
			needsToneMap = true;
			update();
		}
	}

	void setSRGB(bool enable)
	{
		if (enable != toneMapper.isSRGB()) {
			toneMapper.SetSRGB(enable);
			needsToneMap = true;
			update();
		}
	}



	void setScale(float scale)
	{
		Q_ASSERT(scale > 0);
		scaleFactor = scale;
		sizeAux = scale * sizeOrig();
		resize(sizeAux);
		needsToneMap = true;
		update();
	}

	QSize sizeOrig() const 
	{
		return QSize(ldrImage.Width(), ldrImage.Height());
	}

	float scale() const
	{
		return scaleFactor;
	}

	const ImageDataProvider & imageDataProvider() const {
		return dataProvider;
	}

	void mouseMoveEvent(QMouseEvent * event);

protected:
	virtual void paintEvent(QPaintEvent *event);


	qreal scaleFactor;
	bool needsToneMap;

signals:

	// This signal is like a "mouseOver" event, sending the 
	// absolute TopDown position, taking into account any resizing
	void mouseOverPixel( QPoint pos );

private:


	// The internal representation of the HDR Image
	Image<Rgba32F> hdrImage;

	// The tone mapped version of the Image
	Image<Bgra8> ldrImage;

	// The abstraction to communicate information about the images
	//ImageIODataProvider dataProvider;

	// Our nice tonemapper
	ToneMapper toneMapper;

	// QImage version of the stuff, uses implicit sharing
	QImage qImage;

	// A data provider for querying info
	ImageIODataProvider dataProvider;

	// Internal cache of the size
	QSize sizeAux;



	static bool loadHdr(const QString & fileName, Image<Rgba32F> &hdr);

};

#endif /* HDRIMAGEDISPLAY_H */
