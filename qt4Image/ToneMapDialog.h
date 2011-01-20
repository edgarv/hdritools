// This is the implementation of the actual QWidget which
// will contain the main window of the application
//
#if !defined(PCG_TONEMAPDIALOG_H)
#define PCG_TONEMAPDIALOG_H

#include <ui_tonemap.h>

#include <QPointer>
#include "QInterpolator.h"
#include "ImageDataProvider.h"


// Define the window form using the multiple inheritance approach
class ToneMapDialog : public QDialog, public Ui::ToneMapDialog
{
    // We need this macro to process signals and slots
    Q_OBJECT

public:
    // Basic constructor, it initializes all the elements of the window
    ToneMapDialog(const ImageDataProvider &imgDataProvider,
        QWidget *parent = 0);

signals:
    // Notify a new white point
    void whitePointChanged(double value);

    // Notify a new key value
    void keyChanged(double value);

    // Notify when the overall dialog is toggled
    void toggled(bool enabled);


private slots:
    // React to a key change
    void keySliderChanged(int rawValue);

    // React to the white point range from the data provider
    void updateWhitePointRange(double minimum, double maximum);

    // When the automatic values are requested
    void autoClicked();


private:
    const ImageDataProvider &dataProvider;
    QPointer<QInterpolator> whitePointInterpolator;
    QPointer<QInterpolator> keyInterpolator;
    int m_zoneIdx;
    bool m_isSet;
};


#endif /* PCG_TONEMAPDIALOG_H */
