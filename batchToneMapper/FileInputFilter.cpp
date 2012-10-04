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
#if !defined(_WIN32)
    ifstream is(qPrintable(*filename), ios_base::binary);
#else
    const wchar_t * wFilename =
        reinterpret_cast<const wchar_t*>(filename->constData());
    ifstream is(wFilename, ios_base::binary);
#endif
    if (! is.bad() ) {
        return FloatImageProcessor::load(*filename, is, formatStr, offset);
    }
    else {
        cerr << "Ooops! Unable to open " << *filename << " for reading.";
        return new ImageInfo;
    }
}
