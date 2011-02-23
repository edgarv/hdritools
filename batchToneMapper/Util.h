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

// Misc utilities for the program

#if !defined(UTIL_H)
#define UTIL_H

#include <QStringList>

class Util {

public:
    // Returns a vector of strings with the supported extensions for writing image.
    // The supported extension are lowercase and without points (i.e. png,jpg)
    static const QStringList & supportedWriteImageFormats();

    // Tells if a file is readable (it's a file, readable and it exists) and
    // sets the variables telling whether it's a zip file or a supported hdr file
    static bool isReadable(const QString & filename, bool & isZip, bool & isHdr);

    static bool isPngSupported();
    static bool isJpgSupported();

    // Returns the number of processor available in the system
    static int numberOfProcessors();

    // Expands a globbing pattern. By default it will use the Qt-based version
    static QStringList glob(const QString & pattern, bool useQt = true);

    // Format string for 16-bit png
    static const QString PNG16_FORMAT_STR;

private:
    // Cache of the number of processors
    static volatile int number_of_processors;

    // List of supported formats
    static QStringList supported;

    // Direct format support flags
    static bool hasPng;
    static bool hasJpg;

};


#endif /* UTIL_H */
