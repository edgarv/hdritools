#include "ImageApp.h"

#include <QFileOpenEvent>


ImageApp::ImageApp(int& argc, char **argv) :
    QApplication(argc, argv), mainWin(this)
{
}


bool ImageApp::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::FileOpen:
        open(static_cast<QFileOpenEvent *>(event)->file(), true);
        return true;
    default:
        return QApplication::event(event);
    }
}

