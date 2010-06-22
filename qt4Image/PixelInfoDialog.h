// Class that represents the dialog with the Pixel Information

#if !defined(PIXELINFODIALOG_H)
#define PIXELINFODIALOG_H

#include "ui_info.h"

#include "ImageDataProvider.h"

// Define the dialog using the multiple inheritance approach
class PixelInfoDialog : public QDialog, public Ui::Dialog
{
    // We need this macro to process signals and slots
    Q_OBJECT

protected:
    // We will keep a reference to the image data provider
    const ImageDataProvider &dataProvider;

    // To remember the last location in the image we required
    QPoint lastQuery;

    // Lets keep the code nice: some sugar:
    inline bool isTopDown() const { return topDownRadio->isChecked(); }

protected slots:
    // To connect with the data provider
    void setImageSize( QSize newSize );

    // Builds a new query point when the x spin changes
    void queryX( int newX );

    // Builds a new query point when the y spin changes taking into account the scanline order
    void queryY( int newY );

    // To change the value of Y when the scanline order changes
    void updateYscanlineOrder();

public slots:
    // To know when we need to update info for a particular point
    void showInfo( QPoint newPoint );


public:
    // Basic constructor, it initializes all the elements of the window
    PixelInfoDialog(const ImageDataProvider &imgDataProvider, QWidget *parent = 0);

};

#endif /* PIXELINFODIALOG_H */
