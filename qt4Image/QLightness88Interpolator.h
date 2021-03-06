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

#pragma once
#if !defined(PCG_QLIGHTNESS88INTERPOLATOR_H)
#define PCG_QLIGHTNESS88INTERPOLATOR_H

#include "QInterpolator.h"

// Uber-custom interpolator: it interpolates non-linearly the key using
// the CIELAB lightness modified by Pauli (1976). The associated slider
// *MUST* have the range [0,88], since that is a built-in assumption
class QLightness88Interpolator : public QInterpolator
{
    Q_OBJECT

public:
    QLightness88Interpolator(QAbstractSlider *slider, QLineEdit *edit,
        QObject *parent = 0);

protected:
    virtual void updateState(double minimum, double midpoint, double maximum,
        int sliderMinimum, int sliderMaximum);
    virtual int toSliderValue(double value) const;
    virtual double toValue(int sliderValue) const;
};

#endif // PCG_QLIGHTNESS88INTERPOLATOR_H
