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

