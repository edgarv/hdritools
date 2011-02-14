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
