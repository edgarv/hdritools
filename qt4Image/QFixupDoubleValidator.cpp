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

#include "QFixupDoubleValidator.h"

#include <QLineEdit>
#include <QDebug>


// Disable the balloon code since it doesn't seem to work
#define USE_BALLOONTIP 0

// For now this just works in Windows
#if USE_BALLOONTIP && defined(_WIN32)

#if _MSC_VER >= 1400 // VS2005 added this directive
#pragma comment(linker, \
    "\"/manifestdependency:type='win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "processorArchitecture='*' "\
    "language='*'\"")
#endif

#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commctrl.h>

#endif // USE_BALLOONTIP && defined(_WIN32)

namespace
{
#if USE_BALLOONTIP
void editShowBalloonTip(QObject *qobject,
                        const QString & title, const QString & message)
{
    QLineEdit *edit = qobject_cast<QLineEdit *>(qobject);
    if (edit == NULL)
        return;

#if !defined(_WIN32)
    qDebug() << "Balloon tip: \"" << title << "\": " << message;
#else
    
    // Setup the balloon tip structure
    EDITBALLOONTIP tip;
    tip.cbStruct = sizeof(EDITBALLOONTIP);
    tip.pszText  = (LPCWSTR) message.utf16();
    tip.pszTitle = (LPCWSTR) title.utf16();
    tip.ttiIcon  = TTI_WARNING;

    const HWND editHWND = edit->winId();

    ::SendMessage(editHWND, WM_COPY, 0, 0);
    qDebug() << "Copy message returned: " << GetLastError();

    if (!Edit_ShowBalloonTip(editHWND, &tip)) {
        qDebug() << "Showing the tip failed!";
    }
#endif // !defined(_WIN32)
}
#endif // USE_BALLOONTIP

template <bool greaterThan>
inline void createBoundary(QString &str, const double n)
{
    double delta = 1e-6 * n;
    bool isValid = false;
    str.setNum(n);
    double value = str.toDouble(&isValid);
    Q_ASSERT(isValid);
    while (greaterThan ? value > n : value < n) {
        value += greaterThan ? -delta : delta;
        delta *= 8.0;
        str.setNum(value);
        value = str.toDouble(&isValid);
        Q_ASSERT(isValid);
    }
}

} // namespace


QFixupDoubleValidator::QFixupDoubleValidator(double bottom, double top,
                                             int decimals, QObject * parent) :
QDoubleValidator(bottom, top, decimals, parent)
{
    createBoundary</*LT*/false>(m_bottomFixup, bottom);
    createBoundary</*GT*/true> (m_topFixup, top);
}



void QFixupDoubleValidator::setRange(double minimum, double maximum,
                                     int decimals)
{
    QDoubleValidator::setRange(minimum, maximum, decimals);
    createBoundary</*LT*/false>(m_bottomFixup, minimum);
    createBoundary</*GT*/true> (m_topFixup, maximum);
}



void QFixupDoubleValidator::fixup (QString & input) const
{
    bool isValid = false;
    const double value = input.toDouble(&isValid);
    if (!isValid) {
        return;
    }

    // Show the balloon tip
#if USE_BALLOONTIP
    QString message = QString(tr("The value is outside the valid range "
        "[%1,%2]")).arg(this->bottom()).arg(this->top());
    editShowBalloonTip(this->parent(), tr("Invalid value"), message);
#endif

    // Fixup numbers by clamping them to the proper limit
    if (value > this->top()) {
        input = m_topFixup;
    } else if (value < this->bottom()) {
        input = m_bottomFixup;
    } else {
        qFatal("Strange state!");
    }
}
