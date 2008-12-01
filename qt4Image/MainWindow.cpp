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
#if 0
#include "ImageComparator.h"
#endif

using namespace pcg;


MainWindow::MainWindow(const QApplication *application, QMainWindow *parent) : QMainWindow(parent), 
	app(application),
	scaleFactor(1.0f), minScaleFactor(0.125f), maxScaleFactor(8.0f),
#if 0
	toneMapper(4096), dataProvider(hdrImage, ldrImage),
#endif
	hdrDisplay(NULL), pixInfoDialog(NULL),
	openFileDir(QDir::currentPath())
{
	// This method ONLY configures the GUI as in Qt Designer
	setupUi(this);

	appTitle = this->windowTitle();

#if 0
	imageLabel = new HDRImageLabel;
	imageLabel->setBackgroundRole(QPalette::NoRole);
	imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
#endif 

	/* Ricorda: QScrollArea::setWidget(widget) causes the scroll area to
	/* take total control of the widget, including its destruction */ 
	hdrDisplay = new HDRImageDisplay();
	imgScrollFrame->setWidget(hdrDisplay);

	pixInfoDialog = new PixelInfoDialog(hdrDisplay->imageDataProvider(), this);

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
	connect( pixInfoDialog, SIGNAL(rejected()), this, SLOT(pixelInfoClosed()) );
#if 0
	connect( imageLabel, SIGNAL(mouseOver(QPoint)), this, SLOT(mouseOverImage(QPoint)) );
#endif
	connect( hdrDisplay, SIGNAL(mouseOverPixel(QPoint)), this, SLOT(mouseOverImage(QPoint)) );
	connect( this, SIGNAL(requestPixelInfo(QPoint)), pixInfoDialog, SLOT(showInfo(QPoint)) );

	// Standard dialog for about
	connect( action_About,   SIGNAL(triggered()), this, SLOT(about()) );
	connect( actionAbout_Qt, SIGNAL(triggered()), this, SLOT(aboutQt()) );

	// Connects the gamma and exposure controls
	connect( exposureConnect, SIGNAL(valueChanged(float)), this, SLOT(setExposure(float)) );
	connect( gammaConnect,    SIGNAL(valueChanged(float)), this, SLOT(setGamma(float)) );

	// Also connects the sRGB control
	connect( srgbChk, SIGNAL(stateChanged(int)), this, SLOT(setSRGB(int)) );

	// Initial values for the tone mapper
	setExposure(0.0f);
	setSRGB(true);

	// Finally we activate both the window's layout and the central widget's layout
	// so that everything is distributed before showing the window
	this->layout()->activate();
	this->centralWidget()->layout()->activate();
}



void MainWindow::saveAs()
{

	// Filter for the ldr files: by default we save the images as PNG
	static QString filter = 
		tr("Image files (*.png *.bmp *.jpg *.tiff *.rgbe *.hdr *.exr);;"
		"PNG (*.png);;BMP (*.bmp);;JPEG (*.jpg;*.jpeg);;PPM (*.ppm);;TIFF (*.tiff);;"
		"Radiance (*.rgbe *.hdr);;OpenEXR (*.exr);;All files (*.*)");
	static QString dir = QDir::currentPath();

	QString file = QFileDialog::getSaveFileName(this,
		tr("Save As"), dir, filter);

	if (!file.isEmpty()) {

		QFileInfo fileInfo(file);

#if 0
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
#endif

		QCursor waitCursor(Qt::WaitCursor);
		QApplication::setOverrideCursor(waitCursor);

		if (! hdrDisplay->save(file) ) {
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, appTitle,
					tr("An error occurred while saving the tone mapped image %1.").arg(file));
		}
		else {
			QApplication::restoreOverrideCursor();

			// Finally, save the directory of the saved file
			dir = fileInfo.dir().path();
		}		
	}

}

QString MainWindow::chooseHDRFile(QString message) 
{
	// Our filter for files
	static QString filter = tr("HDR Images (*.rgbe *.hdr *.exr);;"
		"Radiance (*.rgbe *.hdr);;OpenEXR (*.exr);;All files (*.*)");

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


#if 0
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
#endif


void MainWindow::compareWith()
{
	QString fileName = chooseHDRFile(tr("Open file for comparison"));
	if (fileName.isEmpty()) {
		return;
	}

	HDRImageDisplay::HdrResult result;
	QCursor waitCursor(Qt::WaitCursor);
	QApplication::setOverrideCursor(waitCursor);

	if (hdrDisplay->compareTo(fileName, ImageComparator::AbsoluteDifference, &result)) {

		QFileInfo fileInfo(fileName);
		openFileDir = fileInfo.dir().path();

		// Makes all what is necessary to the gui
		updateForLoadedImage(tr("[Absolute Difference]"));

		QApplication::restoreOverrideCursor();
	}
	else {
		Q_ASSERT( result != HDRImageDisplay::NoError );
		QApplication::restoreOverrideCursor();

		switch(result) {
			case HDRImageDisplay::UnknownType:
				QMessageBox::warning(this, appTitle,
					tr("Unknown HDR format for the input file %1.").arg(fileName));
				break;
			case HDRImageDisplay::SizeMissmatch:
				QMessageBox::warning(this, appTitle,
					tr("The image to compare must have the same size "
					   "as the one currently loaded."));
				break;
			default:
				QMessageBox::warning(this, appTitle,
					tr("An error occurred while loading %1.").arg(fileName));
		}

	}

#if 0
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

			// Now we perform the comparison operation in place
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
#endif
}

void MainWindow::open(const QString &fileName, bool adjustSize)
{
	
	HDRImageDisplay::HdrResult result;
	QCursor waitCursor(Qt::WaitCursor);
	QApplication::setOverrideCursor(waitCursor);
	
	if (hdrDisplay->open(fileName, &result)) {

		QFileInfo fileInfo(fileName);
		openFileDir = fileInfo.dir().path();

		// Makes all what is necessary to the gui
		updateForLoadedImage(fileInfo.fileName());

		// And adjust the size of the window if requested
		if (adjustSize) {
			this->adjustSize();
		}

		QApplication::restoreOverrideCursor();
	}
	else {
		Q_ASSERT( result != HDRImageDisplay::NoError );
		QApplication::restoreOverrideCursor();

		switch(result) {
			case HDRImageDisplay::UnknownType:
				QMessageBox::warning(this, appTitle,
					tr("Unknown HDR format for the input file %1.").arg(fileName));
				break;
			default:
				QMessageBox::warning(this, appTitle,
					tr("An error occurred while loading %1.").arg(fileName));
		}

	}

#if 0

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
#endif

}


void MainWindow::updateForLoadedImage(QString imgName)
{
#if 0
	ldrImage.Alloc(hdrImage.Width(), hdrImage.Height());
	toneMapper.ToneMap(ldrImage, hdrImage);

	QImage qImage(reinterpret_cast<uchar *>(ldrImage.GetDataPointer()), 
		ldrImage.Width(), ldrImage.Height(), QImage::Format_RGB32);

	imageLabel->setPixmap(QPixmap::fromImage(qImage));

	if (!action_Fit_on_Screen->isChecked()) {
		imageLabel->adjustSize();
	}
#endif

	// Updates the status of the gui actions
	action_Fit_on_Screen->setEnabled(true);
	action_Save_as->setEnabled(true);
	actionCompare_With->setEnabled(true);
	action_Pixel_Info->setEnabled(true);
	updateActions();

#if 0
	// We also need to update our data provider
	dataProvider.update();
#endif

	// And sets an appropriate title with the name of the application
	this->setWindowTitle( tr("%1 - %2 (%3x%4)")
		.arg( appTitle, imgName )
#if 0
		.arg( ldrImage.Width() )
		.arg( ldrImage.Height() ) );
#endif
		.arg( hdrDisplay->sizeOrig().width() )
		.arg( hdrDisplay->sizeOrig().height() ) );

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
#if 0
		imageLabel->setScaledContents(true);
#endif

		// Less elegant, but in order to keep the aspect ratio, we need to
		// do a manual resizing. First we retrieve the size of the viewport
		// without the scroll bars
		imgScrollFrame->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		imgScrollFrame->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

#if 0
		// Now we set an appropriate size
		Q_ASSERT(imageLabel->pixmap() != NULL && imageLabel->pixmap()->size().isValid());
		QSize size = imageLabel->pixmap()->size();
		size.scale(imgScrollFrame->viewport()->size(), Qt::KeepAspectRatio);
		imageLabel->resize(size);

		// And manually set the scaling factor used
		scaleFactor = (float)size.width() / imageLabel->pixmap()->size().width();
#endif
		QSize size = hdrDisplay->sizeOrig();
		size.scale(imgScrollFrame->viewport()->size(), Qt::KeepAspectRatio);

		// And manually set the scaling factor used
		const float sf = (float)size.width() / hdrDisplay->sizeOrig().width();
		scaleFactor = 1.0f;
		scaleImage(sf);

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
#if 0
	Q_ASSERT(imageLabel->pixmap());
#endif
	scaleFactor *= factor;

	// Correct from floating point errors close to 1
	if (abs(scaleFactor - 1.0f) < 1e-3f) {
		scaleFactor = 1.0f;
	}
#if 0
	// This is a bug/horrible limitation: we only enable the exposure and
	// gamma settings when the scale factor is 1. This is the kind of
	// stuff which just works perfect and smooth using a GPU version
	imageLabel->setScaledContents(scaleFactor != 1.0f);

	imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());
#endif

	adjustScrollBar(imgScrollFrame->horizontalScrollBar(), factor);
	adjustScrollBar(imgScrollFrame->verticalScrollBar(), factor);

	hdrDisplay->setScale(scaleFactor);

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

	const bool enableTone = !(hdrDisplay->size().isEmpty());
	exposureConnect->setEnabled(enableTone);
	srgbChk->setEnabled(enableTone);

	const bool enableGamma = enableTone && !srgbChk->isChecked();
	gammaConnect->setEnabled(enableGamma);
	gammaLabel->setEnabled(enableGamma);

#if 0
	const bool isOriginalSize = scaleFactor == 1.0f;
	exposureConnect->setEnabled(!action_Fit_on_Screen->isChecked()  && isOriginalSize);
	gammaConnect->setEnabled(!action_Fit_on_Screen->isChecked()  && isOriginalSize);
#endif
} 


void MainWindow::about()
{
	// TODO Parametrize the version and update it automatically somehow
	QMessageBox::about(this, tr("About %1").arg(appTitle), 
		tr("<p><b>%1</b></p>"
		"<p>Version %2.<br/>"
		"%1 is a simple, fast viewer for High Dynamic Range (HDR) images.</p>"
		"<p>Copyright (C) 2008 Program of Computer Graphics, Cornell University.</p>")
			.arg(appTitle)
			.arg(tr("0.1.0")) );
}

void MainWindow::aboutQt()
{
	QApplication::aboutQt();
}

void MainWindow::pixelInfo()
{
	const bool showPixInfo = action_Pixel_Info->isChecked();
	if (showPixInfo) {
		pixInfoDialog->show();
	}
	else {
		pixInfoDialog->hide();
	}
}

void MainWindow::pixelInfoClosed()
{
	action_Pixel_Info->setChecked(false);
}

void MainWindow::mouseOverImage( QPoint pos )
{
	// Request that query point if it makes sense
	if (pixInfoDialog->isVisible()) {

#if 0
		// We scale this by the inverse of the scale factor to get the actual pixel location.
		// Note that just calling "pos /= scaleFactor" fails, especially when using the zoom,
		// because of the rounding. Instead we need to use "floor" to play safe
		pos.setX((int)floor(pos.x() / scaleFactor));
		pos.setY((int)floor(pos.y() / scaleFactor));
		Q_ASSERT(pos.x() >= 0 && pos.x() < hdrImage.Width());
		Q_ASSERT(pos.y() >= 0 && pos.y() < hdrImage.Height());
#endif

		qDebug() << "Requesting info at: " << pos;
		requestPixelInfo( pos );
	}
}


void MainWindow::adjustSize()
{
#if 0
	Q_ASSERT(ldrImage.Width() > 0 && ldrImage.Height() > 0);
#endif

	// First we need to know how big is the difference between the
	// window and our frame:
	QSize offset = this->size() - imgScrollFrame->size();

	// We add the actual size of the image and set that as our size + the small 1px border
#if 0
	offset += QSize(ldrImage.Width(), ldrImage.Height()) + QSize(2,2);
#endif
	offset += hdrDisplay->sizeOrig() + QSize(2,2);

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
	hdrDisplay->setGamma(gamma);
#if 0
	if (gamma != toneMapper.Gamma()) {
		toneMapper.SetGamma(gamma);
		toneMapper.ToneMap(ldrImage, hdrImage);
		imageLabel->update();
	}
#endif
}

void MainWindow::setExposure(float exposure)
{
	hdrDisplay->setExposure(exposure);
#if 0
	if (exposure != toneMapper.Exposure()) {
		toneMapper.SetExposure(exposure);
		toneMapper.ToneMap(ldrImage, hdrImage);
		imageLabel->update();
	}
#endif
}

void MainWindow::setSRGB(int value)
{
	hdrDisplay->setSRGB(value != Qt::Unchecked);
	updateActions();
}
