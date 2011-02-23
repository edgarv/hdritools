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

// Implementation file

#include "PixelInfoDialog.h"

// Includes everything that we might need
#include <QtGui>

PixelInfoDialog::PixelInfoDialog(const ImageDataProvider &imgDataProvider, QWidget *parent) : 
    QDialog(parent, Qt::Tool), dataProvider(imgDataProvider), lastQuery(-1,-1)
{
    // Sets up the UI
    setupUi(this);

    // We make a connection to the image provider for setting up properly
    // the limits on the spinboxes
    connect( &dataProvider, SIGNAL(sizeChanged(QSize)), this, SLOT(setImageSize(QSize)) );

    // Also sets up the connections for the spin values
    connect( xSpin, SIGNAL(valueChanged(int)), this, SLOT(queryX(int)) );
    connect( ySpin, SIGNAL(valueChanged(int)), this, SLOT(queryY(int)) );

    // We also want to update the value of y when we change the scanline order
    connect( bottomUpRadio, SIGNAL(toggled(bool)), this, SLOT(updateYscanlineOrder()) );

    // Choose a better location than the default one (centered with respect to the parent)
    move(10,10);


}

void PixelInfoDialog::setImageSize( QSize newSize )
{
    // We reset the maxima for the spin boxes and reset the values to 0
    xSpin->setValue(0);
    ySpin->setValue(0);

    xSpin->setMaximum(newSize.width() - 1);
    ySpin->setMaximum(newSize.height() - 1);

    showInfo( QPoint(0, 0) );
}


void PixelInfoDialog::queryX( int newX )
{
    if ( isTopDown() ) {
        showInfo( QPoint(newX, ySpin->value()) );
    }
    else {
        showInfo( QPoint(newX, dataProvider.size().height() - ySpin->value() - 1) );
    }
}

void PixelInfoDialog::queryY( int newY )
{
    if ( isTopDown() ) {
        showInfo( QPoint(xSpin->value(), newY) );
    }
    else {
        showInfo( QPoint(xSpin->value(), dataProvider.size().height() - newY - 1) );
    }
}

void PixelInfoDialog::updateYscanlineOrder()
{
    // Because the buttons are mutually exclusive, we are always doing the same operation
    ySpin->setValue(dataProvider.size().height() - ySpin->value() - 1);
}

void PixelInfoDialog::showInfo( QPoint newPoint )
{
    // Sanity checks
    Q_ASSERT( newPoint.x() >= 0 && newPoint.x() < dataProvider.size().width() );
    Q_ASSERT( newPoint.y() >= 0 && newPoint.y() < dataProvider.size().height() );

    // If this is a different point that the one just queried, we update the gui
    if (lastQuery != newPoint) {
        lastQuery = newPoint;
        const int &x = lastQuery.rx();
        const int &y = lastQuery.ry();

        // Update the labels
        float rF,gF,bF;
        unsigned char r,g,b;

        dataProvider.getLdrPixel(x, y, r, g, b);
        rDispLabel->setText(QString("%1").arg(r, /*fieldWidth=*/3));
        gDispLabel->setText(QString("%1").arg(g, 3));
        bDispLabel->setText(QString("%1").arg(b, 3));

        dataProvider.getHdrPixel(x, y, rF, gF, bF);
        rLabel->setText(QString::number(rF, /*format=*/'e', /*precision=*/ 8));
        gLabel->setText(QString::number(gF, 'e', 8));
        bLabel->setText(QString::number(bF, 'e', 8));


        // And update the spins. We take care when assingning "y" because of
        // the scanline order thing
        xSpin->setValue(x);
        if ( isTopDown() ) {
            ySpin->setValue(y);
        }
        else {
            ySpin->setValue(dataProvider.size().height() - y - 1);
        }
    }

}
