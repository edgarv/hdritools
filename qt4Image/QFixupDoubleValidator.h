#include <QDoubleValidator>

class QFixupDoubleValidator : public QDoubleValidator
{
    Q_OBJECT

public:
    QFixupDoubleValidator(QObject * parent = 0) :
    QDoubleValidator(parent) {}

    QFixupDoubleValidator(double bottom, double top, int decimals,
        QObject * parent);
    ~QFixupDoubleValidator() {}

    virtual void setRange(double minimum, double maximum, int decimals = 0);
    virtual void fixup (QString & input) const;

private:
    QString m_bottomFixup;
    QString m_topFixup;
};
