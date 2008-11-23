// This is the implementation of the actual QWidget which
// will contain the main window of the application
//
// The instructions for this come from the document
// "Porting .ui Files to Qt 4" in the Qt Assistant
// (POD = Plain Old Data, something like a POJO)

#if !defined(MAINWINDOW_H)
#define MAINWINDOW_H

#include "ui_main.h"

#include "Image.h"
#include "Rgba32F.h"
#include "LDRPixels.h"
#include "ToneMapper.h"

#include "PixelInfoDialog.h"
#include "ImageDataProvider.h"
#include "HDRImageDisplay.h"

#if 0
#include "HDRImageLabel.h"
#endif

// Uses forward declarations
class QLabel;
class DoubleSpinSliderConnect;
class QScrollBar;
class QApplication;

using namespace pcg;


// Define the window form using the multiple inheritance approach
class MainWindow : public QMainWindow, public Ui::MainWindow
{
	// We need this macro to process signals and slots
	Q_OBJECT


protected:
#if 0
	// The container for the image
	HDRImageLabel *imageLabel;
#endif

	// The actual widget with all the magic
	HDRImageDisplay *hdrDisplay;

	// The dialog for the pixel information
	PixelInfoDialog *pixInfoDialog;

	// The synchronization objects for the pairs of slider-spin
	DoubleSpinSliderConnect *exposureConnect;
	DoubleSpinSliderConnect *gammaConnect;

	// Function to open a dialog for an HDR file. If the user
	// cancels the dialog it returns an empty string
	QString chooseHDRFile(QString message = tr("Open File"));

	// A String with the current directory for the open dialog
	QString openFileDir;

#if 0
	// Helper function just to load an HDR file from a non-empty filename
	// Returns true if it could open the file and load it into dest, false if
	// it didn't recognize the HDR file. If the last parameter is not null,
	// it will be set to the directory of the loaded filename.
	// When something really bad occurs the
	// method can throw an exception
	static bool loadHdr(QString filename, Image<Rgba32F> &dest, QString *imgDir = NULL);
#endif

	// This function updates the gui when a new file has been already loaded.
	// The parameter indicates the name to display in the title bar
	void updateForLoadedImage(QString imgName);

#if 0
	// The internal representation of the HDR Image
	Image<Rgba32F> hdrImage;

	// The tone mapped version of the Image
	Image<Bgra8> ldrImage;

	// The abstraction to communicate information about the images
	ImageIODataProvider dataProvider;

	// Our nice tonemapper
	ToneMapper toneMapper;
#endif

	// The current scale factor
	float scaleFactor;

	// To limit the zoom in and out factor
	const float minScaleFactor;
	const float maxScaleFactor;

	// We want to keep a reference to the underlying application
	const QApplication *app;

	// A copy of the original window tittle
	QString appTitle;

	// Utility functions related to the scaling of the image
	void scaleImage(float factor);
	void adjustScrollBar(QScrollBar *scrollBar, float factor);
	void updateActions();

	

protected slots:
	// Slots for the open and save actions
	void open();
	void saveAs();

	// The comparison action
	void compareWith();

	// View mode adjustments
	void zoomIn();
	void zoomOut();
	void actualPixels();
	void fitToWindow();

	// To hide/show the pixel info
	void pixelInfo();

	// Help menu slots
	void aboutQt();

	// Adjusts the size of the window so that the image doesn't require any scrollbars
	void adjustSize();

	// Used to react when the user closes the pixel info window: we need to update the window gui
	void pixelInfoClosed();

	// Receive raw "onMouseOver" events from the image label
	void mouseOverImage( QPoint pos );

	// TODO This should go to a class than contains the tone mapper and
	// all the related stuff
	void setGamma(float gamma);
	void setExposure(float exposure);



signals:
	void requestPixelInfo( QPoint pos );

public:
	// Basic constructor, it initializes all the elements of the window
	MainWindow(const QApplication *application = 0, QMainWindow *parent = 0);

	// Tries to load the given image into the GUI, and if requested it will
	// try to resize the window so that the image fits without scrollbars
	void open(const QString &fileName, bool adjustSize = false);

	// Events for dropping files into the window, so that we can open them
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
};


#endif /* MAINWINDOW_H */
