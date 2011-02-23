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

#if defined(_MSC_VER)
#pragma once
#endif
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

