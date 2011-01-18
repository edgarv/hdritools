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
    virtual void updateState(double minimum, double maximum,
        int sliderMinimum, int sliderMaximum);
    virtual int toSliderValue(double value);
    virtual double toValue(int sliderValue);
};

#endif // PCG_QLIGHTNESS88INTERPOLATOR_H
