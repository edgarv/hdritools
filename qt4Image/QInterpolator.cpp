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

#include "QInterpolator.h"
#include <QDebug>

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

QInterpolator::QInterpolator(double minimum, double midpoint, double maximum,
                             QAbstractSlider *slider, QLineEdit *edit,
                             QObject *parent):
QObject(parent), m_slider(slider), m_edit(edit),
m_validator(minimum, maximum, 16, this), m_value(minimum)
{
    Q_ASSERT(!m_slider.isNull());
    Q_ASSERT(!m_edit.isNull());
    Q_ASSERT(minimum < maximum);
    m_edit->setValidator(&m_validator);
    setRange(minimum, midpoint, maximum);

    // Connect the signals
    connect ( m_slider, SIGNAL(rangeChanged(int,int)),
        this, SLOT(sliderRangeChanged(int,int)) );
    connect ( m_slider, SIGNAL(valueChanged(int)), 
        this, SLOT(sliderChanged(int)) );
    connect ( m_edit, SIGNAL(editingFinished(void)),
        this, SLOT(textEdited(void)) );
}


void QInterpolator::setRange(double minimum, double midpoint, double maximum)
{
    Q_ASSERT(minimum < maximum);
    m_validator.setRange(minimum, maximum, m_validator.decimals());
    updateState(minimum, midpoint, maximum,
        m_slider->minimum(), m_slider->maximum());
    const double val = value();
    m_value = val;
    setValue(qBound(minimum, val, maximum));
}


void QInterpolator::setValue(double value)
{
    value = qBound(bottom(), value, top());
    if (!qFuzzyCompare(value, m_value)) {
        m_value = value;

        bool textOk;
        const double textValue = m_edit->text().toDouble(&textOk);
        if (!textOk || !qFuzzyCompare(value, textValue)) {
            QString text = QString::number(value);
            int txtPos;
            QValidator::State state = m_validator.validate(text, txtPos);
            if (state != QValidator::Acceptable) {
                m_validator.fixup(text);
            }
            Q_ASSERT(m_validator.validate(text, txtPos) == QValidator::Acceptable);
            m_edit->setText(text);
        }

        const int pos = toSliderValue(value);
        if (pos != m_slider->value()) {
            Q_ASSERT(m_slider->minimum() <= pos && pos <= m_slider->maximum());
            m_slider->setValue(pos);
        }

        emit valueChanged(value);
        emit valueChanged(static_cast<float> (value));
    }
}


void QInterpolator::sliderRangeChanged(int minimum, int maximum)
{
    updateState(bottom(), middle(), top(), minimum, maximum);
}


void QInterpolator::sliderChanged(int sliderValue)
{
    // Don't update if the current value would map to the same slider position
    const int currSliderValue = toSliderValue(m_value);
    if (sliderValue != currSliderValue) {
        const double value = toValue(sliderValue);
        Q_ASSERT(bottom() <= value || qFuzzyCompare(value, bottom()));
        Q_ASSERT(top()    >= value || qFuzzyCompare(value, top()));
        setValue(value);
    }
}


void QInterpolator::textEdited()
{
    bool ok;
    const double value = m_edit->text().toDouble(&ok);
    Q_ASSERT(ok);
    setValue(value);
}



QLinearInterpolator::QLinearInterpolator(double minimum, double maximum,
                                         QAbstractSlider *slider,
                                         QLineEdit *edit, QObject *parent)
: QInterpolator(minimum, 0.0, maximum, slider, edit, parent),
  m_slope(0.0), m_slopeInv(0.0), m_intercept(0.0)
{
    // C++ doesn't like virtual functions inside constructors
    updateState(minimum, 0.0, maximum, slider->minimum(), slider->maximum());
}


void QLinearInterpolator::updateState(double minimum, double, double maximum,
                                      int sliderMinimum, int sliderMaximum)
{
    m_slope = (maximum - minimum) / 
        static_cast<double>(sliderMaximum - sliderMinimum);
    m_slopeInv = 1.0/m_slope;
    m_intercept = minimum - m_slope*static_cast<double>(sliderMinimum);
}


int QLinearInterpolator::toSliderValue(double value) const
{
    Q_ASSERT(m_slopeInv != 0.0);
    int pos = qRound((value - bottom())*m_slopeInv + 
        static_cast<double>(sliderMinimum()));
    return pos;
}


double QLinearInterpolator::toValue(int sliderValue) const
{
    Q_ASSERT(m_slope != 0.0);
    double value = m_slope*static_cast<double>(sliderValue) + m_intercept;
    return value;
}



QPowerInterpolator::QPowerInterpolator(double exponent, double minimum,
                                       double maximum, QAbstractSlider *slider,
                                       QLineEdit *edit, QObject *parent)
: QInterpolator(minimum, 0.0, maximum, slider, edit, parent),
  m_exponent(exponent), m_exponentInv(1.0/exponent),
  m_valueRange(maximum-minimum), m_valueRangeInv(1.0/(maximum-minimum)),
  m_valueMin(minimum), m_sliderRange(slider->maximum()-slider->minimum()),
  m_sliderRangeInv(1.0/(slider->maximum()-slider->minimum())),
  m_sliderMin(slider->minimum())
{
    // C++ doesn't like virtual functions inside constructors
    updateState(minimum, 0.0, maximum, slider->minimum(), slider->maximum());
}


void QPowerInterpolator::setExponent(double value)
{
    m_exponent    = value;
    m_exponentInv = 1.0 / value;
    updateState(bottom(), 0.0, top(), sliderMinimum(), sliderMaximum());
}


void QPowerInterpolator::updateState(double minimum, double, double maximum,
                                     int sliderMinimum, int sliderMaximum)
{
    m_valueRange     = maximum-minimum;
    m_valueRangeInv  = 1.0 / m_valueRange;
    m_valueMin       = minimum;
    m_sliderRange    = static_cast<double>(sliderMaximum - sliderMinimum);
    m_sliderRangeInv = 1.0 / m_sliderRange;
    m_sliderMin      = static_cast<double>(sliderMinimum);
}


int QPowerInterpolator::toSliderValue(double value) const
{
    const double v1 = (value - m_valueMin) * m_valueRangeInv;
    const double v2 = pow(v1, m_exponentInv) * m_sliderRange + m_sliderMin;
    const int pos = qRound(v2);
    return pos;
}


double QPowerInterpolator::toValue(int sliderValue) const
{
    const double v1 = (sliderValue - m_sliderMin) * m_sliderRangeInv;
    const double v2 = pow(v1, m_exponent) * m_valueRange + m_valueMin;
    return v2;
}

