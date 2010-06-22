#pragma once
#if !defined(PCG_IMAGEAPP_H)
#define PCG_IMAGEAPP_H

// Header file for the QApplication which will launch the actual viewer window
#include <QApplication>

#include "MainWindow.h"

class ImageApp : public QApplication
{
    Q_OBJECT
public:
    ImageApp(int& argc, char **argv);

    void open(const QString &filename);

    inline void show() {
        mainWin.show();
    }

    inline void setVisible(bool visible) {
        mainWin.setVisible(visible);
    }

protected:
    virtual bool event(QEvent *);

private:
    // For now keep only one window
    MainWindow mainWin;
};


#endif /* PCG_IMAGEAPP_H */

