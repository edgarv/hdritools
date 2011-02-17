#include "Util.h"

#include <QString>
#include <QImageWriter>
#include <QList>
#include <QByteArray>
#include <QFileInfo>
#include <QtDebug>
#include <QRegExp>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>

// The function for getting the number of threads
// is pretty much a ripoff of tbb
#include <tbb/tbb_stddef.h>
#include <tbb/tbb_machine.h>

#if defined(_WIN32)
# define WIN32_LEAN_AND_MEAN
# define NOMINMAX
# include <windows.h>
#elif __linux__
# include <sys/sysinfo.h>
#elif __APPLE__
# include <sys/types.h>
# include <sys/sysctl.h>
#endif

using namespace std;

volatile int Util::number_of_processors = 0;

bool Util::hasPng = false;
bool Util::hasJpg = false;

const QString Util::PNG16_FORMAT_STR("png16");

QStringList Util::supported;


#if defined(_WIN32)
namespace
{
// From MSDN, retrieve the system error message for the last-error code
QString GetLastErrorMessage() 
{ 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &lpMsgBuf,
        0, NULL );

    // Assign the error message to a QString and clean up
    QString msg = QString::fromUtf16(reinterpret_cast<ushort*>(lpMsgBuf));
    LocalFree(lpMsgBuf);
    return msg;
}


// Helper struct for WoW64 stuff, which is not directly available in WinXP
// Based on code from MSDN
// http://msdn.microsoft.com/en-us/library/ms684139%28v=vs.85%29.aspx
struct Wow64Helper
{
private:
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    typedef BOOL (WINAPI *LPFN_WOW64DISABLEWOW64FSREDIRECTION) (PVOID *);
    typedef BOOL (WINAPI *LPFN_WOW64REVERTWOW64FSREDIRECTION) (PVOID);

    bool m_isWow64;
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    LPFN_WOW64DISABLEWOW64FSREDIRECTION fnWow64DisableWow64FsRedirection;
    LPFN_WOW64REVERTWOW64FSREDIRECTION fnWow64RevertWow64FsRedirection;

    Wow64Helper() : m_isWow64(false),
      fnIsWow64Process(NULL), 
      fnWow64DisableWow64FsRedirection(NULL),
      fnWow64RevertWow64FsRedirection(NULL)
    {
        fnIsWow64Process = (LPFN_ISWOW64PROCESS)
            GetProcAddress(GetModuleHandle(TEXT("kernel32")),
            "IsWow64Process");
        fnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)
            GetProcAddress(GetModuleHandle(TEXT("kernel32")),
            "Wow64DisableWow64FsRedirection");
        fnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)
            GetProcAddress(GetModuleHandle(TEXT("kernel32")),
            "Wow64RevertWow64FsRedirection");

        // Initialize IsWow64
        if(NULL != fnIsWow64Process) {
            BOOL bIsWow64 = FALSE;
            if (fnIsWow64Process(GetCurrentProcess(),&bIsWow64) != FALSE) {
                m_isWow64 = bIsWow64 != FALSE;
            } else {
                qCritical() << "IsWow64Process: " << GetLastErrorMessage();
            }
        }
    }

    // Also the copy constructor and the assignment operators are private
    Wow64Helper(Wow64Helper const&) {}
    Wow64Helper& operator=(Wow64Helper const&) {}

    static Wow64Helper* m_instance;

public:
    static Wow64Helper* Instance() {
        if (m_instance == NULL) {
            m_instance = new Wow64Helper;
        }
        return m_instance;
    }

    bool IsWow64() const {
        return m_isWow64;
    }

    void DisableWow64FsRedirection(PVOID * OldValue) const {
        if (IsWow64() && fnWow64DisableWow64FsRedirection != NULL) {
            if (fnWow64DisableWow64FsRedirection(OldValue) == FALSE) {
                qCritical() << "Wow64DisableWow64FsRedirection: " 
                            << GetLastErrorMessage();
            }
        }
    }

    void RevertWow64FsRedirection(PVOID OldValue) const {
        if (IsWow64() && fnWow64RevertWow64FsRedirection != NULL) {
            if (fnWow64RevertWow64FsRedirection(OldValue) == FALSE) {
                qCritical() << "Wow64RevertWow64FsRedirection: " 
                            << GetLastErrorMessage();
            }
        }
    }
};

Wow64Helper* Wow64Helper::m_instance = NULL;


QStringList globWin32(const QString & pattern)
{
    QStringList list;

    // Unicode version
    const WCHAR * expr = reinterpret_cast<const WCHAR *>(pattern.utf16());

    PVOID OldValue = NULL;
    Wow64Helper::Instance()->DisableWow64FsRedirection(&OldValue);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // Try to find the first file
    hFind = FindFirstFileW(expr, &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {

        // Exhaust the pattern
        do {
            list.append (QString::fromUtf16 (
                reinterpret_cast<ushort*> (&ffd.cFileName[0])));
        } while (FindNextFileW(hFind, &ffd) != 0);

        FindClose(hFind);
    } else {
        // In case of error just append the pattern, verbatim
        list.append(pattern);

        const DWORD errorCode = GetLastError();
        if (errorCode != ERROR_FILE_NOT_FOUND && errorCode != ERROR_PATH_NOT_FOUND &&
            errorCode != ERROR_INVALID_NAME) {
            qCritical() << "FindFirstFile: " << GetLastErrorMessage();
        }
    }

    Wow64Helper::Instance()->RevertWow64FsRedirection(OldValue);

    list.sort();
    return list;
}

} // namespace
#endif // defined(_WIN32)


namespace
{

//////////////////////////////////////////////////////////////////////////////
// Qt version of the Python implementation for glob:
// http://svn.python.org/view/python/branches/release27-maint/Lib/glob.py?view=markup
// 
// Original code is under the Python Software Foundation License
// http://docs.python.org/license.html
// 
//
// These 2 helper functions non-recursively glob inside a literal directory.
// They return a list of basenames. `glob1` accepts a pattern while `glob0`
// takes a literal basename (so it only has to check for its existence)

inline bool hasMagic(const QString & pathname)
{
    static const QRegExp reg("[*?[]");
    Q_ASSERT(reg.isValid());
    return reg.indexIn(pathname) != -1;
}


QStringList glob1(const QString& dirname, const QString& pattern)
{
    Q_ASSERT(!pattern.isEmpty());
    QStringList list;
    QDirIterator it(dirname);
#if defined(Q_WS_WIN)
    const QRegExp reg(pattern, Qt::CaseInsensitive, QRegExp::WildcardUnix);
#else
    const QRegExp reg(pattern, Qt::CaseSensitive, QRegExp::WildcardUnix);
#endif
    const bool skip_startsWith_dot_check = pattern.startsWith('.');
    while (it.hasNext()) {
        it.next();
        const QString fname = it.fileName();
        if (reg.exactMatch(fname) &&
            (skip_startsWith_dot_check || !fname.startsWith('.'))) {
            list.append(fname);
        }
    }
    return list;
}


QStringList glob0(const QString& dirname, const QString& basename)
{
    QStringList list;
    if (basename.isEmpty()) {
        // QFileInfo::fileName() returns an empty value if the path ends with
        // a separator. 'q*x/' should match only directories
        QFileInfo info(dirname);
        if (info.isDir()) {
            list.append(dirname);
        }
    } else {
        QFileInfo info(dirname, basename);
        if (info.exists() || info.isSymLink()) {
            list.append(basename);
        }
    }
    return list;
}


QStringList QGlob(const QString& pathname)
{
    QStringList list;

    if (!hasMagic(pathname)) {
        QFileInfo info(pathname);
        if (info.exists()) {
            list.append(pathname);
        }
        return list;
    }
    QFileInfo info(pathname);

    // The variable names are using python's conventions!
    const QString basename = info.fileName();
    const QString dirname  = info.path();

    if (dirname.isEmpty() || dirname == ".") {
        // Simple case: just query the current directory for the pattern
        return glob1(dirname, basename);
    }

    // Windows chokes if the dirname has wildcards but POSIX allows that
    QStringList dirs;
    if (hasMagic(dirname)) {
        // Recurse to find all directories which match
        dirs = ::QGlob(dirname);
    } else {
        dirs.append(dirname);
    }

    const bool magicBasename = hasMagic(basename);
    for (QStringList::const_iterator it = dirs.constBegin();
         it != dirs.constEnd(); ++it)
    {
        const QDir dir(*it);
        const QStringList names = magicBasename ?
            glob1(*it, basename) : glob0(*it, basename);
        for (QStringList::const_iterator itName = names.constBegin();
             itName != names.constEnd(); ++itName)
        {
            list.append(QDir::toNativeSeparators(dir.filePath(*itName)));
        }
    }
    list.sort();
    return list;
}
} // namespace



const QStringList & Util::supportedWriteImageFormats()
{
    if (supported.size() == 0) {

        // Signal png with 16 bpp as a format
        supported.push_back(PNG16_FORMAT_STR);

        QList<QByteArray> list = QImageWriter::supportedImageFormats();
        for (QList<QByteArray>::const_iterator it = list.constBegin();
             it != list.constEnd(); ++it)
        {
            QString format(*it);
            format = format.toLower();
            if (format == "jpg" || format == "jpeg") {
                hasJpg = true;
            }
            else if (format == "png") {
                hasPng = true;
            }
            supported.append( format );
        }
        supported.sort();
    }

    return supported;
}


bool Util::isPngSupported()
{
    if (supported.size() == 0) {
        supportedWriteImageFormats();
    }
    return hasPng;
}

bool Util::isJpgSupported()
{
    if (supported.size() == 0) {
        supportedWriteImageFormats();
    }
    return hasJpg;
}




bool Util::isReadable(const QString & filename, bool & isZip, bool & isHdr)
{
    QFileInfo info(filename);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        isZip = false;
        isHdr = false;
        return false;
    }

    if (info.suffix().compare("zip", Qt::CaseInsensitive) == 0) {
        isZip = true;
        isHdr = false;
    }
    else {
        QRegExp hdrExp("rgbe|hdr|exr|pfm", Qt::CaseInsensitive);
        isHdr = hdrExp.exactMatch(info.suffix());
        isZip = false;
    }

    return true;
}


int Util::numberOfProcessors()
{
    if (!number_of_processors) {

#if defined(_WIN32)
    
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        number_of_processors = static_cast<int>(si.dwNumberOfProcessors);

#elif __linux__ 

        number_of_processors = get_nprocs();

#elif __APPLE__

        int name[2] = {CTL_HW, HW_AVAILCPU};
        int ncpu;
        size_t size = sizeof(ncpu);
        sysctl( name, 2, &ncpu, &size, NULL, 0 );
        number_of_processors = ncpu;

#else

#error Unknown OS

#endif /* os kind */

    }

    return number_of_processors;
}


QStringList Util::glob(const QString & pattern, bool useQt)
{
    QStringList result;
    if (useQt) {
        result = ::QGlob(pattern);
    } else {
#if defined(_WIN32)
        result = ::globWin32(pattern);
#else
        Q_ASSERT("Not implemented!" == 0);
#endif
    }
    // If the pattern failed, just return it
    if (result.isEmpty()) {
        result.append(pattern);
    }
    return result;
}
