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

// A class to represent our fancy version of a Qt Label

#if !defined(HDRIMAGELABEL_H)
#define HDRIMAGELABEL_H

#include <QLabel>

class HDRImageLabel : public QLabel {
    
    Q_OBJECT

protected:
    void mouseMoveEvent(QMouseEvent * event);

signals:

    // This signal is like a "mouseOver" event, sending the relative TopDown position on
    // the image AFTER any resizing
    void mouseOver( QPoint pos );

public:
    HDRImageLabel(QWidget * parent = 0, Qt::WindowFlags f = 0);

    // To signal the aspect ratio
    int heightForWidth(int w) const;

    

};

#endif /* HDRIMAGELABEL_H */
