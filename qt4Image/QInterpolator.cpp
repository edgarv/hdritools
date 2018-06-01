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
#include <QLocale>
#include <QDebug>

#if defined(__INTEL_COMPILER)
# include <mathimf.h>
#else
# include <cmath>
#endif

#include <limits>

typedef std::numeric_limits<double> double_limits;


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
            QString text = locale.toString(value);
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
    if (ok) {
        setValue(value);
    } else {
        qDebug() << "QInterpolator::textEdited: not a double: " << m_edit->text();
    }
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



namespace
{
// Functor for linear interpolation between the logarithm of two values,
// assumes 0 < min < max:
//
// y == Power(E,(x*(Log(vMax) - Log(vMin)))/(sliderMax - sliderMin) + 
//      (-(sliderMin*Log(vMax)) + sliderMax*Log(vMin))/(sliderMax - sliderMin))
//
// x == (sliderMin*Log(vMax) - sliderMax*Log(vMin))/(Log(vMax) - Log(vMin)) + 
//      ((sliderMax - sliderMin)*Log(y))/(Log(vMax) - Log(vMin))
//
class LogLinearFunctor
{
public:
    LogLinearFunctor() :
    m_slope(double_limits::signaling_NaN()),
    m_offset(double_limits::signaling_NaN()),
    m_invSlope(double_limits::signaling_NaN()),
    m_invOffset(double_limits::signaling_NaN()),
    m_vMin(double_limits::signaling_NaN()),
    m_vMax(double_limits::signaling_NaN()),
    m_sliderMin(double_limits::signaling_NaN()),
    m_sliderMax(double_limits::signaling_NaN()),
    m_valid(false)
    {}

    void update(double vMin, double vMax, double sliderMin, double sliderMax)
    {
        Q_ASSERT(0.0<vMin && vMin<vMax && vMax<double_limits::infinity());
        Q_ASSERT(sliderMin<sliderMax && sliderMax<double_limits::infinity());
        m_vMin = vMin;
        m_vMax = vMax;
        m_sliderMin = sliderMin;
        m_sliderMax = sliderMax;
        m_valid = true;
        const double valMax = log(vMax);
        const double valMin = log(vMin);

        // To transform from slider to values
        m_slope = (valMax - valMin)/(sliderMax - sliderMin);
        m_offset = (-(sliderMin*valMax) + sliderMax*valMin) / 
            (sliderMax - sliderMin);

        // To transform from values to slider
        m_invSlope  = (sliderMax - sliderMin)/(valMax - valMin);
        m_invOffset = (sliderMin*valMax - sliderMax*valMin) / (valMax-valMin);
    }

    inline void invalidate()  {
        m_valid = false;
    }

    inline double toValue(double slider) const {
        Q_ASSERT(m_valid);
        Q_ASSERT(m_sliderMin <= slider && slider <= m_sliderMax);
        
        double y = m_slope * slider + m_offset;
        y = exp(y);
        Q_ASSERT(m_vMin <= y || qFuzzyCompare(m_vMin, y));
        Q_ASSERT(m_vMax >= y || qFuzzyCompare(m_vMax, y));
        return y;
    }

    inline double toSlider(double value) const {
        Q_ASSERT(m_valid);
        Q_ASSERT(m_vMin <= value && value <= m_vMax);
        
        const double y = log(value);
        double x = m_invSlope * y + m_invOffset;
        Q_ASSERT(m_sliderMin <= x || qFuzzyCompare(m_sliderMin, x) || ((m_invSlope * log(m_vMin) + m_invOffset) <= value));
        Q_ASSERT(m_sliderMax >= x || qFuzzyCompare(m_sliderMax, x) || ((m_invSlope * log(m_vMax) + m_invOffset) >= value));
        x = qBound(m_sliderMin, x, m_sliderMax);
        return x;
    }

private:
    double m_slope;
    double m_offset;
    double m_invSlope;
    double m_invOffset;
    double m_vMin;
    double m_vMax;
    double m_sliderMin;
    double m_sliderMax;
    double m_valid;
};



/**
 * For a slider with range [sliderMin,sliderMax] interpolating linearly
 * between [vMin,vMax] in log space, the function returns the number
 * of stops covered by the reduced range [sliderMin,sliderMax]-sliderDelta
 * assuming:
 *  vMax > vMin > 0 && sliderMax > sliderMin && sliderDelta >= 0
 *
 * The formula was derived in Mathematica:
 ******************************************************************************
 toValue[x_] := 
 Exp[((Log[vMax] - Log[vMin])/(sliderMax - sliderMin)
      x + (-sliderMin Log[vMax] + sliderMax Log[vMin])/(
    sliderMax - sliderMin))]

PowerExpand[
 FullSimplify[
  Log[2, toValue[sliderMax - delta]] - Log[2, toValue[sliderMin]], 
  vMax > vMin > 0 && sliderMax > sliderMin && delta >= 0]]
(* The next expression yields the same result *)
PowerExpand[
 FullSimplify[
  Log[2, toValue[sliderMax]] - Log[2, toValue[sliderMin + delta]], 
  vMax > vMin > 0 && sliderMax > sliderMin && delta > 0]]
(* Evaluation result: *)
-(((delta - sliderMax + sliderMin) (Log[vMax] - 
    Log[vMin]))/((sliderMax - sliderMin) Log[2]))
 ******************************************************************************
 */
inline double stopsInReducedSliderRange(int sliderMin, int sliderMax,
    int sliderDelta, double vMin, double vMax)
{
    Q_ASSERT(0.0<vMin && vMin<vMax && sliderMin<sliderMax && 0<=sliderDelta);
    double logRange    = log(vMax) - log(vMin);
    double sliderRange = sliderMax - sliderMin;
    const double invLog2 = 1.442695040888963; // 1/log(2)
    double stops = invLog2*(((sliderRange-sliderDelta)*logRange)/sliderRange);
    return stops;
}

} // namespace



struct QBiLinearLogInterpolator::Data
{
    LogLinearFunctor loLinear;
    LogLinearFunctor lo;
    LogLinearFunctor hi;
    LogLinearFunctor hiLinear;
    double lowPoint;
    int sliderLowPoint;
    double midpoint;
    int sliderMidpoint;
    double hiPoint;
    int sliderHiPoint;
};

QBiLinearLogInterpolator::QBiLinearLogInterpolator(double minimum,
    double midpoint, double maximum,
    QAbstractSlider *slider, QLineEdit *edit, QObject *parent) :
QInterpolator(minimum, midpoint, maximum, slider, edit, parent), d(new Data)
{
    Q_CHECK_PTR(d);
    updateState(minimum,midpoint,maximum, slider->minimum(),slider->maximum());
}

QBiLinearLogInterpolator::~QBiLinearLogInterpolator()
{
    if (d != NULL) {
        delete d;
    }
}

double QBiLinearLogInterpolator::middle() const
{
    return d->midpoint;
}

void QBiLinearLogInterpolator::updateState(double minimum, double midpoint,
    double maximum, int sliderMinimum, int sliderMaximum)
{
    Q_ASSERT(minimum < midpoint && midpoint < maximum);
    Q_ASSERT(sliderMinimum < sliderMaximum && (sliderMaximum-sliderMinimum)>1);
    const int sliderRange = sliderMaximum - sliderMinimum;
    d->midpoint = midpoint;
    d->sliderMidpoint = sliderRange / 2;
    Q_ASSERT(sliderMinimum < d->sliderMidpoint);
    Q_ASSERT(d->sliderMidpoint < sliderMaximum);

    const int sliderDelta = qMax(1, qRound(0.05 * sliderRange));

    // If there are more than 10 stops between the minimum and the midpoint,
    // the bottom 5% values of the slider provide very steep linear
    // interpolation from (midpoint - 10stops) to the minmum value
    double stopsLo = stopsInReducedSliderRange(sliderMinimum,d->sliderMidpoint,
        sliderDelta, minimum, d->midpoint);
    if (stopsLo > 10.0 && sliderMinimum + sliderDelta < d->sliderMidpoint) {
        d->lowPoint       = (1.0/(1<<10)) * midpoint; // 2^(log2(midpoint)-10)
        d->sliderLowPoint = sliderMinimum + sliderDelta;
        d->loLinear.update(minimum, d->lowPoint,
            sliderMinimum, d->sliderLowPoint);
        d->lo.update(d->lowPoint, midpoint,
            d->sliderLowPoint, d->sliderMidpoint);
    }
    else {
        d->lowPoint       = -std::numeric_limits<double>::infinity();
        d->sliderLowPoint =  std::numeric_limits<int>::min();
        d->loLinear.invalidate();
        d->lo.update(minimum, midpoint, sliderMinimum, d->sliderMidpoint);
    }

    // Similarly, if there are more than 10 stops between the midpoint and the
    // maximum, the top 5% values of the slider provide very steep linear
    // interpolation from (midpoint + 10stops) to the maximum value
    double stopsHi = stopsInReducedSliderRange(d->sliderMidpoint,sliderMaximum,
        sliderDelta, d->midpoint, maximum);
    if (stopsHi > 10.0 && d->sliderMidpoint < sliderMaximum - sliderDelta) {
        d->hiPoint       = (1<<10) * midpoint; // 2^(log2(midpoint)+10)
        d->sliderHiPoint = sliderMaximum - sliderDelta;
        d->hi.update(midpoint, d->hiPoint,
            d->sliderMidpoint, d->sliderHiPoint);
        d->hiLinear.update(d->hiPoint, maximum,
            d->sliderHiPoint, sliderMaximum);
    }
    else {
        d->hiPoint       = std::numeric_limits<double>::infinity();
        d->sliderHiPoint = std::numeric_limits<int>::max();
        d->hi.update(midpoint, maximum, d->sliderMidpoint, sliderMaximum);
        d->hiLinear.invalidate();
    }
}

int QBiLinearLogInterpolator::toSliderValue(double value) const
{
    double result;
    if (value < d->lowPoint) {
        result = d->loLinear.toSlider(value);
    } else if (value < d->midpoint) {
        result = d->lo.toSlider(value);
    } else if (value < d->hiPoint) {
        result = d->hi.toSlider(value);
    } else {
        result = d->hiLinear.toSlider(value);
    }
    const int sliderResult = qRound(result);
    return sliderResult;
}

double QBiLinearLogInterpolator::toValue(int sliderValue) const
{
    double value;
    if (sliderValue < d->sliderLowPoint) {
        value = d->loLinear.toValue(sliderValue);
    } else if (sliderValue < d->sliderMidpoint) {
        value = d->lo.toValue(sliderValue);
    } else if (sliderValue < d->sliderHiPoint) {
        value = d->hi.toValue(sliderValue);
    } else {
        value = d->hiLinear.toValue(sliderValue);
    }
    return value;
}
