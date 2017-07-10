/*============================================================================
  HDRITools - High Dynamic Range Image Tools
  Copyright 2008-2011 Program of Computer Graphics, Cornell University

  Distributed under the OSI-approved MIT License (the "License");
  see accompanying file LICENSE for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
 ----------------------------------------------------------------------------- 
 Primary author:
     Edgar Velazquez-Armendariz <cs#cornell#edu - eva5>
============================================================================*/

#if defined(_MSC_VER)
# pragma once
#endif
#if !defined (PCG_QFIXUPDOUBLEVALIDATOR_H)

#include <QDoubleValidator>
#include <QLocale>

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
    QLocale m_locale;
};

#endif // PCG_QFIXUPDOUBLEVALIDATOR_H
