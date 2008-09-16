// This is a simple class to provide the extra signals so that one
// can connect a double spin box and a normal slider. This class
// will just assume that the slider makes sense with respect to the
// double spin thing.

#if !defined(DOUBLESPINSLIDERCONNECT_H)
#define DOUBLESPINSLIDERCONNECT_H

#include <QObject>

// Forward references
class QAbstractSlider;
class QDoubleSpinBox;

class DoubleSpinSliderConnect : public QObject {

	Q_OBJECT

protected slots:

	void setSliderValue( double val );
	void setSpinValue( int val );

public slots:

	// Sets the value of both the spiner and the slider
	void setValue(float val);

	// Signals cannot have access specifier
signals:

	// Notifies of the new value of both the slider and the spinbox
	void valueChanged(float newValue);


protected:

	// References to the objects which we want to synchronize
	QAbstractSlider *slider;
	QDoubleSpinBox  *spin;

	// The actual value of this guy
	float value;

public:
	DoubleSpinSliderConnect(QAbstractSlider *slider, QDoubleSpinBox  *spin);

	// Utility function to enable/disable both components at the same time
	void setEnabled(bool isEnabled);

};

#endif /* DOUBLESPINSLIDERCONNECT_H */
