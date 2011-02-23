/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

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
#include "ToneMapDialog.h"

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

    // This function updates the gui when a new file has been already loaded.
    // The parameter indicates the name to display in the title bar
    void updateForLoadedImage(QString imgName);

    // The actual comparison method
    void compareWith(ImageComparator::Type type, const QString & description);

    // The current scale factor
    float scaleFactor;

    // To limit the zoom in and out factor
    const float minScaleFactor;
    const float maxScaleFactor;

    // We want to keep a reference to the underlying application
    const QApplication *app;

    // A copy of the original window tittle
    QString appTitle;

    // The tone mapping window
    ToneMapDialog *toneMapDialog;

    // Utility functions related to the scaling of the image
    void scaleImage(float factor);
    void adjustScrollBar(QScrollBar *scrollBar, float factor);
    void updateActions();
    

protected slots:
    // Slots for the open and save actions
    void open();
    void saveAs();

    // The different comparison methods
    void compareAbsDifference();
    void compareAdd();
    void compareDivide();
    void compareRelError();
    void comparePosNegDifference();
    void comparePosNegRelError();

    // View mode adjustments
    void zoomIn();
    void zoomOut();
    void actualPixels();
    void fitToWindow();

    // To hide/show the pixel info
    void pixelInfo();

    // Help menu slots
    void about();
    void aboutQt();

    // Adjusts the size of the window so that the image doesn't require any 
    // scrollbars
    void adjustSize();

    // Used to react when the user closes the pixel info window: we need to 
    // update the window gui
    void pixelInfoClosed();

    // Receive raw "onMouseOver" events from the image label
    void mouseOverImage( QPoint pos );

    // React to the change of the sRGB checkbox
    void setSRGB(bool enabled);

    // Display the tone mapping window
    void toneMap();
    // To be used when the tone mapping dialog is closed, to update the GUI
    void toneMapClosed();



signals:
    void requestPixelInfo( QPoint pos );

public:
    // Basic constructor, it initializes all the elements of the window
    MainWindow(const QApplication *application = 0, QMainWindow *parent = 0);

    // Tries to load the given image into the GUI, and if requested it will
    // try to resize the window so that the image fits without scrollbars
    bool open(const QString &fileName, bool adjustSize = false);

    // Events for dropping files into the window, so that we can open them
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

    // Returns true if the hdr display is currently empty
    inline bool isEmpty() const {
        Q_ASSERT(hdrDisplay != NULL);
        return hdrDisplay->isEmpty();
    }
};


#endif /* MAINWINDOW_H */
