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

// Common class to be used by the input pipelines

#if !defined FLOATIMAGEPROCESSOR_H
#define FLOATIMAGEPROCESSOR_H

#include <istream>

// Forward declarations
class QString;
struct ImageInfo;


class FloatImageProcessor
{

public:
    // Returns a structure ready for the tonemapper. If it can't load the file
    // it returns an invalid ImageInfo, NOT null.
    static ImageInfo* load(const QString& filenameStr, std::istream & is, 
        const QString& formatStr, int offset = 0);

private:
    // To get the output filename it adds the offset (if it makes sense)
    // and changes the extension
    static void setTargetName(QString & filename, const QString & formatStr,
        int offset);

};

#endif /* FLOATIMAGEPROCESSOR_H */
