#include "Util.h"

#include <QImageWriter>
#include <QList>
#include <QByteArray>
#include <QFileInfo>

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
