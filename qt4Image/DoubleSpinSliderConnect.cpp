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

// The implementation of the nice connector

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

#include "DoubleSpinSliderConnect.h"
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#else
#include <QtGui>
#endif

DoubleSpinSliderConnect::DoubleSpinSliderConnect(QAbstractSlider *slider,
    QDoubleSpinBox  *spin, QObject * parent) :
QObject (parent),
m_value(0.0f)
{
    Q_ASSERT( slider != NULL && spin != NULL );
    this->slider = slider;
    this->spin   = spin;

    // We disable the keyboard tracking so that the value changed signal is emitted only
    // when the typing is finished (only works with Qt >= 4.3)
    spin->setKeyboardTracking(false);

    // Creates the connections for synchronization between the two items
    connect( slider, SIGNAL( valueChanged(int) ), this, SLOT( setSpinValue(int) ) );
    connect( spin,   SIGNAL( valueChanged(double) ), this, SLOT( setSliderValue(double) ) );
}

void DoubleSpinSliderConnect::setValue(float val) {

    if (val != m_value) {
        m_value = val;

        // The signals will also update the slider
        spin->setValue(val);

        // And launches our supper Qt Signal
        emit valueChanged(val);

        qDebug() << "New value:" << val;
    }
}

void DoubleSpinSliderConnect::setSliderValue( double val )
{
    // Scales the incomming double within the integer scale of the
    // slider. It assumes that the lowest and highest values of
    // both widgets are synchronized
    const int lenSlider  = slider->maximum() - slider->minimum();
    const double lenSpin = spin->maximum() - spin->minimum();

    // Gets the relative distance of the new val from the spin minimum
    const double ratio = (val - spin->minimum()) / lenSpin;

    // We convert that relative distance into the slider units, add
    // the offset (ossia the minimum) and set the value to that
    // For some stupid reason, round() is not defined in MSVC!
    const int newVal = (int)floor((ratio * lenSlider)+0.5) + slider->minimum();
    slider->setValue(newVal);

    qDebug() << "SetSliderValue" << val << ',' << newVal;
    setValue((float)val);
}

void DoubleSpinSliderConnect::setSpinValue( int val )
{
    // The same stuff as before, but going from the slider to the spin
    const int lenSlider  = slider->maximum() - slider->minimum();
    const double lenSpin = spin->maximum() - spin->minimum();

    const double ratio = double(val - slider->minimum()) / lenSlider;
    const double newVal = (ratio * lenSpin) + spin->minimum();
    spin->setValue(newVal);

    setValue((float)newVal);

    qDebug() << "SetSpinValue  " << val << ',' << newVal;
}

void DoubleSpinSliderConnect::setEnabled(bool isEnabled) {
    slider->setEnabled(isEnabled);
    spin->setEnabled(isEnabled);
}
