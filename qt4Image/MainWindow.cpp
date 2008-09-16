// Implementation of the main window stuff
#include "MainWindow.h"

// This is the easiest way to solve the includes, altough it must be defining much
// more stuff than it's actually needed
#include <QtGui>

#include "DoubleSpinSliderConnect.h"
#include "PixelInfoDialog.h"


// ImageIO includes
#include "rgbe.h"
#include "Rgba32F.h"
#include "Image.h"
#include "RgbeIO.h"
#include "OpenEXRIO.h"
#include "ImageComparator.h"

using namespace pcg;


MainWindow::MainWindow(const QApplication *application, QMainWindow *parent) : QMainWindow(parent), 
	app(application),
	toneMapper(4096), scaleFactor(1.0f), minScaleFactor(0.125f), maxScaleFactor(8.0f),
	dataProvider(hdrImage, ldrImage), pixInfoDialog(dataProvider, this),
	openFileDir(QDir::currentPath())
{
	// This method ONLY configures the GUI as in Qt Designer
	setupUi(this);

	appTitle = this->windowTitle();

	imageLabel = new HDRImageLabel;
	imageLabel->setBackgroundRole(QPalette::NoRole);
	imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	imgScrollFrame->setWidget(imageLabel);

	// We are also aware of drag and drop
	setAcceptDrops(true);

	// Synchronizes the exposure and gamma controls, originally they are disabled
	exposureConnect = new DoubleSpinSliderConnect(exposureSlider, exposureSpinBox);
	gammaConnect    = new DoubleSpinSliderConnect(gammaSlider, gammaSpinBox);
	exposureConnect->setEnabled(false);
	gammaConnect->setEnabled(false);

	// Creates the connection for the file related actions
	connect( action_Open, SIGNAL(triggered()), this, SLOT(open()) );
	connect( action_Save_as, SIGNAL(triggered()), this, SLOT(saveAs()) );
	connect( actionCompare_With, SIGNAL(triggered()), this, SLOT(compareWith()) );

	// Connection for the close button
	connect( actionE_xit, SIGNAL(triggered()), this, SLOT(close()) );

	// Connections for the view menu
	connect( actionZoom_In,  SIGNAL(triggered()), this, SLOT(zoomIn()) );
	connect( actionZoom_Out, SIGNAL(triggered()), this, SLOT(zoomOut()) );
	connect( action_Fit_on_Screen, SIGNAL(triggered()), this, SLOT(fitToWindow()) );
	connect( action_Actual_Pixels, SIGNAL(triggered()), this, SLOT(actualPixels()) );
	connect( action_Pixel_Info, SIGNAL(triggered()), this, SLOT(pixelInfo()) );

	// For adjusting the window on request 
	connect( action_AdjustSize, SIGNAL(triggered()), this, SLOT(adjustSize()) );

	// Connections for the pixel info dialog and receiving the files dropped
	connect( &pixInfoDialog, SIGNAL(rejected()), this, SLOT(pixelInfoClosed()) );
	connect( imageLabel, SIGNAL(mouseOver(QPoint)), this, SLOT(mouseOverImage(QPoint)) );
	connect( this, SIGNAL(requestPixelInfo(QPoint)), &pixInfoDialog, SLOT(showInfo(QPoint)) );

	// An standard dialog for about QT
	connect( actionAbout_Qt, SIGNAL(triggered()), this, SLOT(aboutQt()) );

	// Connects the gamma and exposure controls
	connect( exposureConnect, SIGNAL(valueChanged(float)), this, SLOT(setExposure(float)) );
	connect( gammaConnect,    SIGNAL(valueChanged(float)), this, SLOT(setGamma(float)) );

	// Initial values for the tone mapper
	setGamma(2.2f);
	setExposure(0.0f);

	// Finally we activate both the window's layout and the central widget's layout
	// so that everything is distributed before showing the window
	this->layout()->activate();
	this->centralWidget()->layout()->activate();
}



void MainWindow::saveAs()
{
	// TODO: save also as rgbe/hdr and exr

	// Filter for the ldr files: by default we save the images as PNG
	static QString filter = tr("Image files (*.png;*.bmp;*.jpg;*.tiff;*.rgbe;*.hdr;*.exr);;PNG (*.png);;"
		"BMP (*.bmp);;JPEG (*.jpg;*.jpeg);;PPM (*.ppm);;TIFF (*.tiff);;"
		"Radiance (*.rgbe;*.hdr);;OpenEXR (*.exr);;All files (*.*)");
	static QString dir = QDir::currentPath();

	QString file = QFileDialog::getSaveFileName(this,
		tr("Save As"), dir, filter);

	if (!file.isEmpty()) {

		QFileInfo fileInfo(file);
		QString suffix = fileInfo.suffix();

		// We must have something to save
		Q_ASSERT( hdrImage.Width() > 0 && hdrImage.Height() > 0 );

		try {
			// First we try to save it as an HDR File
			if (suffix.compare("rgbe", Qt::CaseInsensitive) == 0 ||
				suffix.compare("hdr",  Qt::CaseInsensitive) == 0) 
			{
				// Saves a RGBE Image
				RgbeIO::Save(hdrImage, file.toStdString().c_str());
			}
			else if (suffix.compare("exr", Qt::CaseInsensitive) == 0)
			{
				// Saves an OpenEXR Image
				OpenEXRIO::Save(hdrImage, file.toStdString().c_str());
			}
			else {

				// We just save the currently displayed image, that's it!
				Q_ASSERT( ldrImage.Width() > 0 && ldrImage.Height() > 0 );

				QImage qImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), 
							ldrImage.Width(), ldrImage.Height(), QImage::Format_RGB32);

				const bool wasSaved = qImage.save(file);
				if (!wasSaved) {
					QMessageBox::warning(this, appTitle,
							tr("An error occurred while saving the tone mapped image %1.").arg(file));
					return;
				}
			}
		}
		catch(...) {
			QMessageBox::warning(this, appTitle,
				tr("An error occurred while saving %1 in a HDR format.").arg(file));
		}

		// Finally, save the directory of the saved file
		dir = fileInfo.dir().path();
	}

}

QString MainWindow::chooseHDRFile(QString message) 
{
	// Our filter for files
	static QString filter = tr("HDR Images (*.rgbe;*.hdr;*.exr);;"
		"Radiance (*.rgbe; *.hdr);;OpenEXR (*.exr);;All files (*.*)");

	QString file = QFileDialog::getOpenFileName(this,
		message, openFileDir, filter);

	return file;
}

void MainWindow::open() 
{
	QString fileName = chooseHDRFile();

	if (!fileName.isEmpty()) {
		open(fileName);
	}
}



bool MainWindow::loadHdr(QString filename, Image<Rgba32F> &dest, QString *imgDir)
{
	QFileInfo fileInfo(filename);
	QString suffix = fileInfo.suffix();		// Suffix without the trailing "."
	
	// Will only work if the suffix is .rgbe or .hdr
	if (suffix.compare("rgbe", Qt::CaseInsensitive) == 0 ||
		suffix.compare("hdr",  Qt::CaseInsensitive) == 0) 
	{
		// Loads the RGBE Image
		RgbeIO::Load(dest, filename.toStdString().c_str());
	}
	else if (suffix.compare("exr", Qt::CaseInsensitive) == 0)
	{
		// Loads from an OpenEXR Image
		OpenEXRIO::Load(dest, filename.toStdString().c_str());
	}
	else {
		return false;
	}

	if (imgDir != NULL) {
		*imgDir = fileInfo.dir().path();
	}
	return true;
}


void MainWindow::compareWith()
{
	QString fileName = chooseHDRFile(tr("Open file for comparison"));

	if (!fileName.isEmpty()) {

		Image<Rgba32F> other;

		try {

			// If there is actually a file, we try to load it
			if (!loadHdr(fileName, other, &openFileDir)) {
				// Terrible case: we don't know what kind of file is this one!
				QMessageBox::warning(this, appTitle,
					tr("Unknown HDR format for the input file %1.").arg(fileName));
				return;
			}

			// The sizes must be the same
			if (hdrImage.Width() != other.Width() || hdrImage.Height() != other.Height()) {
				QMessageBox::warning(this, appTitle,
					tr("The image to compare must have the same size as the one currently loaded."));
				return;
			}

			// Now we perfor the comparison operation in place
			ImageComparator::Compare(ImageComparator::AbsoluteDifference,
				hdrImage, hdrImage, other);

			// Makes all what is necessary to the gui
			updateForLoadedImage(tr("[Absolute Difference]"));
		}
		catch(...) {
			QMessageBox::warning(this, appTitle,
				tr("An error occurred while loading %1.").arg(fileName));
		}
	}

}

void MainWindow::open(const QString &fileName, bool adjustSize)
{
	QFileInfo fileInfo(fileName);
	QString suffix = fileInfo.suffix();		// Suffix without the trailing "."

	try {

		// Try to load the image
		if (!loadHdr(fileName, hdrImage, &openFileDir)) {
			// Terrible case: we don't know what kind of file is this one!
			QMessageBox::warning(this, appTitle,
				tr("Unknown HDR format for the input file %1.").arg(fileName));
			return;
		}

		// At this point we must have a valid HDR image loaded
		Q_ASSERT(hdrImage.Width() > 0 && hdrImage.Height() > 0);

		// Makes all what is necessary to the gui
		updateForLoadedImage(fileInfo.fileName());

		// And adjust the size of the window if requested
		if (adjustSize) {
			this->adjustSize();
		}

	}
	catch (...) {
		QMessageBox::warning(this, appTitle,
			tr("An error occurred while loading %1.").arg(fileName));
	}

}


void MainWindow::updateForLoadedImage(QString imgName)
{
	ldrImage.Alloc(hdrImage.Width(), hdrImage.Height());
	toneMapper.ToneMap(ldrImage, hdrImage);

	QImage qImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), 
		ldrImage.Width(), ldrImage.Height(), QImage::Format_RGB32);

	imageLabel->setPixmap(QPixmap::fromImage(qImage));

	if (!action_Fit_on_Screen->isChecked()) {
		imageLabel->adjustSize();
	}

	// Updates the status of the gui actions
	action_Fit_on_Screen->setEnabled(true);
	action_Save_as->setEnabled(true);
	actionCompare_With->setEnabled(true);
	updateActions();

	// We also need to update our data provider
	dataProvider.update();

	// And sets an appropriate title with the name of the application
	this->setWindowTitle( tr("%1 - %2 (%3x%4)")
		.arg( appTitle, imgName )
		.arg( ldrImage.Width() )
		.arg( ldrImage.Height() ) );

}


void MainWindow::zoomIn()
{
	scaleImage(1.25f);
}

void MainWindow::zoomOut()
{
	scaleImage(0.8f);
}

void MainWindow::actualPixels()
{
	scaleFactor = 1.0f;
	scaleImage(1.0f);
}

void MainWindow::fitToWindow()
{
	const bool fitToWin = action_Fit_on_Screen->isChecked();
	
	if (fitToWin) {
		imageLabel->setScaledContents(true);

		// Less elegant, but in order to keep the aspect ratio, we need to
		// do a manual resizing. First we retrieve the size of the viewport
		// without the scroll bars
		imgScrollFrame->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		imgScrollFrame->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		// Now we set an appropriate size
		Q_ASSERT(imageLabel->pixmap() != NULL && imageLabel->pixmap()->size().isValid());
		QSize size = imageLabel->pixmap()->size();
		size.scale(imgScrollFrame->viewport()->size(), Qt::KeepAspectRatio);
		imageLabel->resize(size);

		// And manually set the scaling factor used
		scaleFactor = (float)size.width() / imageLabel->pixmap()->size().width();

		//imgScrollFrame->setWidgetResizable(true);		/* From the tutorial */
	}
	else {
		//imgScrollFrame->setWidgetResizable(false);	/* From the tutorial */

		// To fix it we re-enable the scrollbars and call our nice function
		imgScrollFrame->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		imgScrollFrame->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

		actualPixels();
	}
	updateActions();

}

void MainWindow::scaleImage(float factor)
{
	Q_ASSERT(imageLabel->pixmap());
	scaleFactor *= factor;

	// Correct from floating point errors close to 1
	if (abs(scaleFactor - 1.0f) < 1e-3f) {
		scaleFactor = 1.0f;
	}

	// This is a bug/horrible limitation: we only enable the exposure and
	// gamma settings when the scale factor is 1. This is the kind of
	// stuff which just works perfect and smooth using a GPU version
	imageLabel->setScaledContents(scaleFactor != 1.0f);

	imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

	adjustScrollBar(imgScrollFrame->horizontalScrollBar(), factor);
	adjustScrollBar(imgScrollFrame->verticalScrollBar(), factor);

	updateActions();
}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, float factor)
{
	scrollBar->setValue(int(factor * scrollBar->value()
		+ ((factor - 1) * scrollBar->pageStep()/2)));
}

void MainWindow::updateActions()
{
	actionZoom_In->setEnabled(  !action_Fit_on_Screen->isChecked() && scaleFactor < maxScaleFactor);
	actionZoom_Out->setEnabled( !action_Fit_on_Screen->isChecked() && scaleFactor > minScaleFactor);
	action_Actual_Pixels->setEnabled( !action_Fit_on_Screen->isChecked());
	action_AdjustSize->setEnabled( !action_Fit_on_Screen->isChecked());

	const bool isOriginalSize = scaleFactor == 1.0f;
	exposureConnect->setEnabled(!action_Fit_on_Screen->isChecked()  && isOriginalSize);
	gammaConnect->setEnabled(!action_Fit_on_Screen->isChecked()  && isOriginalSize);
} 


void MainWindow::aboutQt()
{
	QApplication::aboutQt();
}

void MainWindow::pixelInfo()
{
	const bool showPixInfo = action_Pixel_Info->isChecked();
	if (showPixInfo) {
		pixInfoDialog.show();
	}
	else {
		pixInfoDialog.hide();
	}
}

void MainWindow::pixelInfoClosed()
{
	action_Pixel_Info->setChecked(false);
}

void MainWindow::mouseOverImage( QPoint pos )
{
	// Request that query point if it makes sense
	if (pixInfoDialog.isVisible()) {

		// We scale this by the inverse of the scale factor to get the actual pixel location.
		// Note that just calling "pos /= scaleFactor" fails, especially when using the zoom,
		// because of the rounding. Instead we need to use "floor" to play safe
		pos.setX((int)floor(pos.x() / scaleFactor));
		pos.setY((int)floor(pos.y() / scaleFactor));
		Q_ASSERT(pos.x() >= 0 && pos.x() < hdrImage.Width());
		Q_ASSERT(pos.y() >= 0 && pos.y() < hdrImage.Height());

		requestPixelInfo( pos );
	}
}


void MainWindow::adjustSize()
{
	Q_ASSERT(ldrImage.Width() > 0 && ldrImage.Height() > 0);

	// First we need to know how big is the difference between the
	// window and our frame:
	QSize offset = this->size() - imgScrollFrame->size();

	// We add the actual size of the image and set that as our size + the small 1px border
	offset += QSize(ldrImage.Width(), ldrImage.Height()) + QSize(2,2);

	// We will limit to the available geometry if we can know it by intersecting the regions
	if (app != NULL) {
		QDesktopWidget *desktop = app->desktop();
		QRect available = desktop->availableGeometry(desktop->primaryScreen());
		offset = offset.boundedTo(QSize(available.width(), available.height())*0.95);
	}

	this->resize(offset);
}



void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	// In Windows, dragging a file provides an URI with its name
	// We only accept one!
	if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1) {
		QFileInfo info(event->mimeData()->urls()[0].toLocalFile());

		// To accept it, the file must exist and have a valid extension
		if (info.exists()) {
			QString suffix = info.suffix();

			if (suffix.compare("rgbe", Qt::CaseInsensitive) == 0 ||
				suffix.compare("hdr",  Qt::CaseInsensitive) == 0 ||
				suffix.compare("exr",  Qt::CaseInsensitive) == 0 )
			{
				qDebug() << "Accepting the event for:" << info.absoluteFilePath();
				event->acceptProposedAction();
			}
		}
	}

}

void MainWindow::dropEvent(QDropEvent *event)
{
	// For the drop event we assume that we had previously accepted something that made sense,
	// so we just convert the name to a local file and fire the signal
	if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1) {

		QString filename = event->mimeData()->urls()[0].toLocalFile();
		open(filename);
	}
}



// ### This should go in a different class, shouldn't it?
void MainWindow::setGamma(float gamma)
{
	if (gamma != toneMapper.Gamma()) {
		toneMapper.SetGamma(gamma);
		toneMapper.ToneMap(ldrImage, hdrImage);
		imageLabel->update();
	}

}

void MainWindow::setExposure(float exposure)
{
	if (exposure != toneMapper.Exposure()) {
		toneMapper.SetExposure(exposure);
		toneMapper.ToneMap(ldrImage, hdrImage);
		imageLabel->update();
	}

}
