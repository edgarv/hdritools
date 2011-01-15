#include <QDoubleValidator>

class QFixupDoubleValidator : public QDoubleValidator
{
    Q_OBJECT

public:
    QFixupDoubleValidator(QObject * parent = 0) :
    QDoubleValidator(parent) {}

    QFixupDoubleValidator(double bottom, double top, int decimals,
        QObject * parent) :
    QDoubleValidator(bottom, top, decimals, parent) {}
    ~QFixupDoubleValidator() {}

    virtual void fixup (QString & input) const;
};
