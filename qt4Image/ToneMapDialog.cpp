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



ToneMapDialog::ToneMapDialog(const ImageDataProvider &imgDataProvider,
                             QWidget *parent) :
QDialog(parent, Qt::Dialog), dataProvider(imgDataProvider), m_zoneIdx(0),
m_isSet(false)
{
    // Setup things as in the designer
    setupUi(this);

    // Align some labels
    keyLbl->setMinimumSize(whitePointLbl->sizeHint());
    darkLbl->setMinimumWidth(minLbl->sizeHint().width());
    brightLbl->setMinimumWidth(maxLbl->sizeHint().width());

    // Configure the interpolators
    whitePointInterpolator = new QLinearInterpolator(0.0, 1.0,
        whitePointSldr, whitePointTxt, this);
    keyInterpolator = new QLightness88Interpolator(keySldr, keyTxt, this);

    connect ( keySldr, SIGNAL(valueChanged(int)), 
              this,    SLOT(keySliderChanged(int)) );
    connect ( &dataProvider, SIGNAL(whitePointRangeChanged(double,double)),
              this,          SLOT(updateWhitePointRange(double,double)) );
    connect ( this->autoBtn, SIGNAL(clicked()),
              this,          SLOT(autoClicked()) );
}


void ToneMapDialog::updateWhitePointRange(double minimum, double maximum)
{
    // Don't allow to enable if the values are sense-less
    double whitePoint, key;
    dataProvider.getToneMapDefaults(whitePoint, key);
    bool isValid = minimum < maximum && (0.0 <= key && key <= 1.0) && 
        (minimum <= whitePoint && whitePoint <= maximum);
    this->reinhard02Chk->setEnabled(isValid);
    if (!isValid) {
        qDebug() << "TODO do something else when the tonemapping settings don't make sense";
        return;
    }

    whitePointInterpolator->setRange(minimum, maximum);
    if (!m_isSet) {
        whitePointInterpolator->setValue(whitePoint);
        keyInterpolator->setValue(key);
        m_isSet = true;
    }
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


void ToneMapDialog::autoClicked()
{
    double whitePoint, key;
    dataProvider.getToneMapDefaults(whitePoint, key);
    whitePointInterpolator->setValue(whitePoint);
    keyInterpolator->setValue(key);
}
