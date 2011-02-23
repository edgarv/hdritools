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

#include "ImageApp.h"

#include <QFileOpenEvent>


ImageApp::ImageApp(int& argc, char **argv) :
    QApplication(argc, argv), mainWin(this)
{
}



void ImageApp::open(const QString &filename)
{
    if (filename.isNull() || filename.isEmpty()) return;
    if (mainWin.isEmpty()) {
        mainWin.open(filename, true);
    } else {
        // This occurs on Mac OS X when opening a file through finder.
        // In this case allocate a new window which gets automatically deleted
        // when it's closed
        MainWindow *mw = new MainWindow(this);
        Q_ASSERT(mw != NULL && mw->isEmpty());
        mw->setAttribute(Qt::WA_DeleteOnClose);
        if (mw->open(filename, true)) {
            mw->show();
        } else {
            mw->close();
        }
    }
}



bool ImageApp::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::FileOpen:
        open(static_cast<QFileOpenEvent *>(event)->file());
        return true;
    default:
        return QApplication::event(event);
    }
}

