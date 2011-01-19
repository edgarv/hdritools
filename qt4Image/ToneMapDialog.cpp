#include "ToneMapDialog.h"
#include "QLightness88Interpolator.h"
#include <QDebug>

// Static array with the label for the zones. The last one is just to avoid
// clamping for the very last value
namespace
{
const QString zones_txt[] = {
    QObject::tr("Zone 0"),
    QObject::tr("Zone I"),
    QObject::tr("Zone II"),
    QObject::tr("Zone III"),
    QObject::tr("Zone IV"),
    QObject::tr("Zone V"),
    QObject::tr("Zone VI"),
    QObject::tr("Zone VII"),
    QObject::tr("Zone VIII"),
    QObject::tr("Zone IX"),
    QObject::tr("Zone X"),
    QObject::tr("Zone X")
};
} // namespace



ToneMapDialog::ToneMapDialog(QWidget *parent) :
QDialog(parent, Qt::Dialog),
//l_min(0.0f), l_max(1.0f)
m_zoneIdx(0)
{
    // Setup things as in the designer
    setupUi(this);

    // Align some labels
    keyLbl->setMinimumSize(whitePointLbl->sizeHint());
    darkLbl->setMinimumWidth(minLbl->sizeHint().width());
    brightLbl->setMinimumWidth(maxLbl->sizeHint().width());

    connect ( keySldr, SIGNAL(valueChanged(int)), 
              this,    SLOT(keySliderChanged(int)) );

    // Configure the interpolators
    whitePointInterpolator = new QLinearInterpolator(0.0, 1.0,
        whitePointSldr, whitePointTxt, this);
    keyInterpolator = new QLightness88Interpolator(keySldr, keyTxt, this);
}

void ToneMapDialog::updateData(float whitePoint, float key,
                               float minimum, float maximum)
{
    // Don't allow to enable if the values are sense-less
    this->reinhard02Chk->setEnabled(!(minimum > maximum ||
        qFuzzyCompare(minimum, maximum) || whitePoint < minimum));

    // Set the new values and ranges
    whitePointTxt->setText(QString::number(whitePoint));
    keyTxt->setText(QString::number(key));
    const double l_min = minimum;
    const double l_max = 2.0f * qMax(maximum, whitePoint);
    whitePointInterpolator->setRange(l_min, l_max);
}

void ToneMapDialog::keySliderChanged(int rawValue)
{
    Q_ASSERT(keySldr->minimum() == 0 && keySldr->maximum() == 88);
    const int idx = rawValue >> 3;
    if (m_zoneIdx != idx) {
        m_zoneIdx = idx;
        zoneLbl->setText(zones_txt[idx]);
    }
}
