// Implementation file

#include "HDRImageLabel.h"

#include <QtGui>
#include <QtDebug>

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

HDRImageLabel::HDRImageLabel(QWidget *parent, Qt::WindowFlags f) : QLabel(parent, f)
{
    // By default we want to receive events whenever the mouse moves around
    setMouseTracking(true);
}

void HDRImageLabel::mouseMoveEvent(QMouseEvent * event)
{
    // The position of the event is relative to the image: this means that we
    // don't have to worry about scrollbars!
    mouseOver(event->pos());
}

int HDRImageLabel::heightForWidth(int w) const
{
    if (pixmap() != NULL && pixmap()->size().isValid()) {
        // Get the height while keeping the aspect ratio
        return (int)floor((float(pixmap()->size().height()*w) / pixmap()->size().width())+0.5f);
    }
    else {
        return QLabel::heightForWidth(w);
    }
}
