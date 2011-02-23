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

#if !defined(FILEINPUTFILTER_H)
#define FILEINPUTFILTER_H

#include <QStringList>

// TBB import for the filter stuff
#include <tbb/pipeline.h>


// A simple input filter for the TBB pipeline: it just feeds each
// filename into the parallel loader filter
class FileInputFilter : public tbb::filter {

    // The list of files to process. Note that it just holds a reference!
    const QStringList &files;

    // Iterator to the list of files
    QStringList::const_iterator filename;

public:
    FileInputFilter(const QStringList &fileNames);

    // This will be invoked serially, it returns a pointer to the QString
    // version of each filename in the list.
    void* operator()(void*);
};


class FileLoaderFilter : public tbb::filter {

public:
    FileLoaderFilter(const QString &format, int filenameOffset = 0);

    // The input of this filter are the const QString* from FileInputFilter
    // with the name of the standard file to open. It returns pointers
    // to the proper ImageInfo structures, NOT null.
    void* operator()(void* arg);

protected:
    const QString formatStr;
    const int offset;

};


#endif /* FILEINPUTFILTER_H */
