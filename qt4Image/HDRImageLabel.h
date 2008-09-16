// A class to represent our fancy version of a Qt Label

#if !defined(HDRIMAGELABEL_H)
#define HDRIMAGELABEL_H

#include <QLabel>

class HDRImageLabel : public QLabel {
	
	Q_OBJECT

protected:
	void mouseMoveEvent(QMouseEvent * event);

signals:

	// This signal is like a "mouseOver" event, sending the relative TopDown position on
	// the image AFTER any resizing
	void mouseOver( QPoint pos );

public:
	HDRImageLabel(QWidget * parent = 0, Qt::WindowFlags f = 0);

	// To signal the aspect ratio
	int heightForWidth(int w) const;

	

};

#endif /* HDRIMAGELABEL_H */
