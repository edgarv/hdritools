#include "FileInputFilter.h"
#include "FloatImageProcessor.h"
#include "ImageInfo.h"

#include <fstream>

#include <cstdio>
#include <QTextStream>
namespace
{
QTextStream cerr(stderr, QIODevice::WriteOnly);
QTextStream cout(stdout, QIODevice::WriteOnly);
}

using tbb::filter;

using std::ifstream;
using std::ios_base;


FileInputFilter::FileInputFilter(const QStringList &fileNames) :
	filter(/*is_serial*/ true),
	files(fileNames)
{
	filename = files.constBegin();
}

void* FileInputFilter::operator()(void*)
{
	// Super simple!
	if (filename != files.end()) {
        // Postfix ++ has greater precedence than *
        return const_cast<QString*>(&(*filename++));
	}
	else {
		return NULL;
	}
}



FileLoaderFilter::FileLoaderFilter(const QString &format, int filenameOffset) :
  tbb::filter(/*is_serial*/false),
  formatStr(format),
  offset(filenameOffset) 
{}


void* FileLoaderFilter::operator()(void* arg)
{
	const QString *filename = static_cast<const QString*>(arg);

	// Opens the input stream
	ifstream is(filename->toLocal8Bit(), ios_base::binary);
	if (! is.bad() ) {
		return FloatImageProcessor::load(*filename, is, formatStr, offset);
	}
	else {
		cerr << "Ooops! Unable to open " << *filename << " for reading.";
		return new ImageInfo;
	}

}
