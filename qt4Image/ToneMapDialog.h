// This is the implementation of the actual QWidget which
// will contain the main window of the application
//
#if !defined(PCG_TONEMAPDIALOG_H)
#define PCG_TONEMAPDIALOG_H

#include <ui_tonemap.h>

#include <QDoubleValidator>


// Define the window form using the multiple inheritance approach
class ToneMapDialog : public QDialog, public Ui::ToneMapDialog
{
    // We need this macro to process signals and slots
    Q_OBJECT

public:
    /// Transfer object used to set up the dialog
    struct Data {
        float whitePoint;
        float key;
        float minimum;
        float maximum;
    };

    // Basic constructor, it initializes all the elements of the window
    ToneMapDialog(QWidget *parent = 0);

    // Update the GUI values
    void updateData(float whitePoint, float key, float minimum, float maximum);

signals:
    // Emmit the request for a new white point
    void whitePointUpdate(float value);

    // Emmit the new key value
    void keyUpdate(float value);

private slots:
    // React to a key change
    void keySliderChanged(int rawValue);
    void keyTxtEdited();

    // React to a white point change
    void whitePointSliderChanged(int rawValue);
    void whitePointTxtEdited();


private:
    QDoubleValidator whitePointValidator;
    QDoubleValidator keyValidator;
    float l_min;
    float l_max;
};


#endif /* PCG_TONEMAPDIALOG_H */
