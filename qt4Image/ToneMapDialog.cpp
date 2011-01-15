#include "ToneMapDialog.h"
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
whitePointValidator(0.0, 1.0, 16, this), keyValidator(0.0, 1.0, 16, this),
l_min(0.0f), l_max(1.0f)
{
    // Setup things as in the designer
    setupUi(this);

    // Align some labels
    keyLbl->setMinimumSize(whitePointLbl->sizeHint());
    darkLbl->setMinimumWidth(minLbl->sizeHint().width());
    brightLbl->setMinimumWidth(maxLbl->sizeHint().width());

    // Connect the sliders' signals
    connect ( whitePointSldr, SIGNAL(valueChanged(int)), 
              this,           SLOT(whitePointSliderChanged(int)) );
    connect ( keySldr, SIGNAL(valueChanged(int)), 
              this,    SLOT(keySliderChanged(int)) );
    // Connect the text fields' signals
    connect ( whitePointTxt, SIGNAL(editingFinished(void)),
              this,          SLOT(whitePointTxtEdited(void)) );
    connect ( keyTxt, SIGNAL(editingFinished(void)),
              this,   SLOT(keyTxtEdited(void)) );


    // Configure the validators
    whitePointTxt->setValidator(&whitePointValidator);
    whitePointValidator.setParent(whitePointTxt);
    keyTxt->setValidator(&keyValidator);
    keyValidator.setParent(keyTxt);
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
    l_min = minimum;
    l_max = 2.0f * qMax(maximum, whitePoint);
    qDebug() << "white: " << whitePoint << " min: " << l_min << " max: " << l_max;
}

void ToneMapDialog::keySliderChanged(int rawValue)
{
    qDebug() << "Key value: " << rawValue << " zone: " << (rawValue >> 3);
}

void ToneMapDialog::keyTxtEdited()
{
    qDebug() << "Key text: " << keyTxt->text();
}

void ToneMapDialog::whitePointSliderChanged(int rawValue)
{
    qDebug() << "White point: " << rawValue;

}

void ToneMapDialog::whitePointTxtEdited()
{
    qDebug() << "White text: " << whitePointTxt->text();
}
