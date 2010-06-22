#include <QApplication>

//#include "MainWindow.h"
#include "ImageApp.h"

// Required when building with the static Qt
#if QT_STATICPLUGIN

#include <QtPlugin>
Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qgif)
Q_IMPORT_PLUGIN(qmng)
Q_IMPORT_PLUGIN(qico)
Q_IMPORT_PLUGIN(qtiff)

#endif /* QT_STATICPLUGIN */


int main(int argc, char *argv[])
{
    // TODO: This is no longer required in windows, but how does it
    // behave in Linux or Mac??
    //tbb::task_scheduler_init init;

    //QApplication app(argc, argv);
    //MainWindow mainWin(&app);
    ImageApp app(argc, argv);

    // The application accepts for now one optional argument: the file to load
    // (The first argument is the executable name)
    QStringList args = app.arguments();
    if (args.size() == 2) {
        //mainWin.open(args.at(1), true);
	app.open(args.at(1), true);
    }

    //mainWin.show();
    app.show();
    return app.exec();
}
