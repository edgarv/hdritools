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
} // namespace

void QFixupDoubleValidator::fixup (QString & input) const
{
    bool isValid = false;
    const double value = input.toFloat(&isValid);
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
        input = QString::number(this->top());
    } else if (value < this->bottom()) {
        input = QString::number(this->bottom());
    } else {
        qFatal("Strange state!");
    }
}
