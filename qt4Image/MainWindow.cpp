// Implementation of the main window stuff
#include "MainWindow.h"
#include <HDRITools_version.h>

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

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

using namespace pcg;



MainWindow::MainWindow(const QApplication *application, QMainWindow *parent) : 
    QMainWindow(parent),
    hdrDisplay(NULL), pixInfoDialog(NULL),
    openFileDir(QDir::currentPath()),
    scaleFactor(1.0f), minScaleFactor(1.0f/512), maxScaleFactor(64.0f),
    app(application)
{
    // This method ONLY configures the GUI as in Qt Designer
    setupUi(this);

    appTitle = this->windowTitle();

    // Ricorda: QScrollArea::setWidget(widget) causes the scroll area to
    // take total control of the widget, including its destruction
    hdrDisplay = new HDRImageDisplay(this);
    Q_ASSERT(hdrDisplay->isEmpty());
    imgScrollFrame->setWidget(hdrDisplay);

    pixInfoDialog = new PixelInfoDialog(hdrDisplay->imageDataProvider(), this);
    toneMapDialog = new ToneMapDialog(hdrDisplay->imageDataProvider(), this);

    // We are also aware of drag and drop
    setAcceptDrops(true);

    // Synchronizes the exposure and gamma controls, initially they are disabled
    exposureConnect=new DoubleSpinSliderConnect(exposureSlider,exposureSpinBox);
    gammaConnect   =new DoubleSpinSliderConnect(gammaSlider, gammaSpinBox);
    exposureConnect->setEnabled(false);
    gammaConnect->setEnabled(false);

    // Creates the connection for the file related actions
    connect( action_Open, SIGNAL(triggered()), this, SLOT(open()) );
    connect( action_Save_as, SIGNAL(triggered()), this, SLOT(saveAs()) );

    // Connections for the comparison methods
    connect( actionAbsDifference, SIGNAL(triggered()), 
        this, SLOT(compareAbsDifference()) );
    connect( actionAdd, SIGNAL(triggered()), 
        this, SLOT(compareAdd()) );
    connect( actionDivide, SIGNAL(triggered()), 
        this, SLOT(compareDivide()) );
    connect( actionRelError, SIGNAL(triggered()), 
        this, SLOT(compareRelError()) );
    connect( actionPosNegDifference, SIGNAL(triggered()), 
        this, SLOT(comparePosNegDifference()) );
    connect( actionPosNegRelError, SIGNAL(triggered()), 
        this, SLOT(comparePosNegRelError()) );

    // Connection for the exit (Quit on Mac OS X) and close buttons
    connect( actionE_xit, SIGNAL(triggered()), app, SLOT(closeAllWindows()) );
    connect( action_Close_window, SIGNAL(triggered()), this, SLOT(close()) );

    // Connections for the view menu
    connect( actionZoom_In,  SIGNAL(triggered()), this, SLOT(zoomIn()) );
    connect( actionZoom_Out, SIGNAL(triggered()), this, SLOT(zoomOut()) );
    connect( action_Fit_on_Screen, SIGNAL(triggered()), 
        this, SLOT(fitToWindow()) );
    connect( action_Actual_Pixels, SIGNAL(triggered()), 
        this, SLOT(actualPixels()) );
    connect( action_Pixel_Info, SIGNAL(triggered()), this, SLOT(pixelInfo()) );
    connect( action_Tone_mapping, SIGNAL(triggered()), this, SLOT(toneMap()) );

    // For adjusting the window on request 
    connect( action_AdjustSize, SIGNAL(triggered()), this, SLOT(adjustSize()));

    // Connections for the pixinfo/TMO dialogs and receiving the files dropped
    connect( pixInfoDialog, SIGNAL(rejected()), this, SLOT(pixelInfoClosed()));
    connect( toneMapDialog, SIGNAL(rejected()), this, SLOT(toneMapClosed()) );
    connect( hdrDisplay, 
        SIGNAL(mouseOverPixel(QPoint)), this, SLOT(mouseOverImage(QPoint)) );
    connect( this, SIGNAL(requestPixelInfo(QPoint)), 
        pixInfoDialog, SLOT(showInfo(QPoint)) );

    // Standard dialog for about
    connect( action_About,   SIGNAL(triggered()), this, SLOT(about()) );
    connect( actionAbout_Qt, SIGNAL(triggered()), this, SLOT(aboutQt()) );

    // Connects the gamma, exposure and tone mapping controls
    connect( exposureConnect, SIGNAL(valueChanged(float)), 
        hdrDisplay, SLOT(setExposure(float)) );
    connect( gammaConnect,    SIGNAL(valueChanged(float)), 
        hdrDisplay, SLOT(setGamma(float)) );
    connect( toneMapDialog, SIGNAL(whitePointChanged(double)),
        hdrDisplay, SLOT(setWhitePoint(double)) );
    connect( toneMapDialog, SIGNAL(keyChanged(double)),
        hdrDisplay, SLOT(setKey(double)) );
    connect( toneMapDialog, SIGNAL(toggled(bool)),
        hdrDisplay, SLOT(setReinhard02(bool)) );

    // Also connects the sRGB control
    connect( srgbChk, SIGNAL(toggled(bool)), this, SLOT(setSRGB(bool)) );

    // Initial values for the tone mapper
    hdrDisplay->setExposure(0.0f);
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
        tr("Image files (*.png *.bmp *.jpg *.tiff *.rgbe *.hdr *.exr *.pfm);;"
        "PNG (*.png);;BMP (*.bmp);;JPEG (*.jpg;*.jpeg);;"
        "PPM (*.ppm);;TIFF (*.tiff);;"
        "Radiance (*.rgbe *.hdr);;OpenEXR (*.exr);;Portable floatmap (*.pfm);;"
        "All files (*.*)");
    static QString dir = QDir::currentPath();

    QString file = QFileDialog::getSaveFileName(this,
        tr("Save As"), dir, filter);

    if (!file.isEmpty()) {
        QFileInfo fileInfo(file);
        QCursor waitCursor(Qt::WaitCursor);
        QApplication::setOverrideCursor(waitCursor);

        if (! hdrDisplay->save(file) ) {
            QApplication::restoreOverrideCursor();
            QMessageBox::warning(this, appTitle,
                tr("An error occurred while saving the tone mapped image %1.")
                .arg(file));
        } else {
            QApplication::restoreOverrideCursor();

            // Finally, save the directory of the saved file
            dir = fileInfo.dir().path();
        }		
    }
}



QString MainWindow::chooseHDRFile(QString message) 
{
    // Our filter for files
    static QString filter = tr("HDR Images (*.rgbe *.hdr *.exr *.pfm);;"
        "Radiance (*.rgbe *.hdr);;OpenEXR (*.exr);;Portable floatmap (*.pfm);;"
        "All files (*.*)");

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



void MainWindow::compareWith(ImageComparator::Type type, 
                             const QString & description)
{
    QString fileName = chooseHDRFile(tr("Open file for comparison"));
    if (fileName.isEmpty()) {
        return;
    }

    HDRImageDisplay::HdrResult result;
    QCursor waitCursor(Qt::WaitCursor);
    QApplication::setOverrideCursor(waitCursor);

    if (hdrDisplay->compareTo(fileName, type, &result)) {

        QFileInfo fileInfo(fileName);
        openFileDir = fileInfo.dir().path();

        // Makes all what is necessary to the gui
        updateForLoadedImage(tr("[%1]").arg(description));

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
}



bool MainWindow::open(const QString &fileName, bool adjustSize)
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
        return true;
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
        return false;
    }
}



void MainWindow::updateForLoadedImage(QString imgName)
{
    // Updates the status of the gui actions
    action_Fit_on_Screen->setEnabled(true);
    action_Save_as->setEnabled(true);
    menuCompare->setEnabled(true);
    action_Pixel_Info->setEnabled(true);
    updateActions();

    // And sets an appropriate title with the name of the application
    this->setWindowTitle( tr("%1 - %2 (%3x%4)")
        .arg( appTitle, imgName )
        .arg( hdrDisplay->sizeOrig().width() )
        .arg( hdrDisplay->sizeOrig().height() ) );
}


void MainWindow::zoomIn()
{
    //scaleImage(1.25f);
    scaleImage(2.0f);
}



void MainWindow::zoomOut()
{
    //scaleImage(0.8f);
    scaleImage(0.5f);
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
        // Less elegant, but in order to keep the aspect ratio, we need to
        // do a manual resizing. First we retrieve the size of the viewport
        // without the scroll bars
        imgScrollFrame->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        imgScrollFrame->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        QSize size = hdrDisplay->sizeOrig();
        size.scale(imgScrollFrame->viewport()->size(), Qt::KeepAspectRatio);

        // And manually set the scaling factor used
        const float sf = size.width() > size.height() ?
            (float)size.width() / hdrDisplay->sizeOrig().width() :
            (float)size.height() / hdrDisplay->sizeOrig().height();
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
    scaleFactor *= factor;

    // Correct from floating point errors close to 1
    if (fabsf(scaleFactor - 1.0f) < 1e-3f) {
        scaleFactor = 1.0f;
    }
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
    actionZoom_In->setEnabled(  !action_Fit_on_Screen->isChecked() && 
                                scaleFactor < maxScaleFactor);
    actionZoom_Out->setEnabled( !action_Fit_on_Screen->isChecked() && 
                                scaleFactor > minScaleFactor);
    action_Actual_Pixels->setEnabled( !action_Fit_on_Screen->isChecked());
    action_AdjustSize->setEnabled( !action_Fit_on_Screen->isChecked());

    const bool enableTone = !(hdrDisplay->size().isEmpty());
    exposureConnect->setEnabled(enableTone);
    srgbChk->setEnabled(enableTone);
    action_Tone_mapping->setEnabled(enableTone);

    const bool enableGamma = enableTone && !srgbChk->isChecked();
    gammaConnect->setEnabled(enableGamma);
    gammaLabel->setEnabled(enableGamma);
} 



void MainWindow::about()
{
    QMessageBox::about(this, tr("About %1").arg(appTitle), 
        tr("<p><b>%1</b></p>"
        "<p>Version %2"
#if HDRITOOLS_HAS_VALID_REV
        ", hg revision %4"
#endif
        ".<br/>"
        "Build date: %3.<br/>"
        "%1 is a simple, fast viewer for High Dynamic Range (HDR) images.</p>"
        "<p>Copyright &copy; 2008-2011 Program of Computer Graphics, "
        "Cornell University.</p>")
            .arg(appTitle)
            .arg(tr(pcg::version::versionString()))
            .arg(tr(__DATE__))
#if HDRITOOLS_HAS_VALID_REV
            .arg(pcg::version::globalRevision())
#endif
    );
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
    } else {
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
        qDebug() << "Requesting info at: " << pos;
        requestPixelInfo( pos );
    }
}


void MainWindow::adjustSize()
{
    // First we need to know how big is the difference between the
    // window and our frame:
    const int frameWidth = imgScrollFrame->frameWidth();
    QSize offset = this->size() - imgScrollFrame->size();

    // We add the actual size of the image and set that as our size 
    // plus the frame border
    offset += hdrDisplay->sizeOrig() + 2*QSize(frameWidth, frameWidth);

    // We will limit to the available geometry if we can know it by intersecting 
    // the regions
    if (app != NULL) {
        QDesktopWidget *desktop = app->desktop();
        QRect available = desktop->availableGeometry(desktop->primaryScreen());
        offset = offset.boundedTo(QSize(available.width(), 
            available.height())*0.95);
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
                suffix.compare("exr",  Qt::CaseInsensitive) == 0 ||
                suffix.compare("pfm",  Qt::CaseInsensitive) == 0)
            {
                qDebug() << "Accepting the event for:" 
                         << info.absoluteFilePath();
                event->acceptProposedAction();
            }
        }
    }

}



void MainWindow::dropEvent(QDropEvent *event)
{
    // For the drop event we assume that we had previously accepted something 
    // that made sense, so we just convert the name to a local file and fire the 
    // signal
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1) {

        QString filename = event->mimeData()->urls()[0].toLocalFile();
        open(filename);
    }
}



void MainWindow::setSRGB(bool enabled)
{
    hdrDisplay->setSRGB(enabled);
    updateActions();
}



void MainWindow::compareAbsDifference() {
    compareWith(ImageComparator::AbsoluteDifference, tr("Absolute difference"));
}



void MainWindow::compareAdd() {
    compareWith(ImageComparator::Addition, tr("Addition"));
}



void MainWindow::compareDivide() {
    compareWith(ImageComparator::Division, tr("Division"));
}



void MainWindow::compareRelError() {
    compareWith(ImageComparator::RelativeError, tr("Relative error"));
}



void MainWindow::comparePosNegDifference() {
    compareWith(ImageComparator::PositiveNegative, 
        tr("Positive/negative difference"));
}



void MainWindow::comparePosNegRelError() {
    compareWith(ImageComparator::PositiveNegativeRelativeError, 
        tr("Positive/negative relative error"));
}


void MainWindow::toneMap()
{
    const bool showToneMap = action_Tone_mapping->isChecked();
    if (showToneMap) {
        toneMapDialog->show();
    } else {
        toneMapDialog->hide();
    }
}


void MainWindow::toneMapClosed()
{
    action_Tone_mapping->setChecked(false);
}
