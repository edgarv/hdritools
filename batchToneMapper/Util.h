// Misc utilities for the program

#if !defined(UTIL_H)
#define UTIL_H

#include <vector>
#include <string>

class Util {

public:
	// Returns a vector of strings with the supported extensions for writing image.
	// The supported extension are lowercase and without points (i.e. png,jpg)
	static const std::vector<std::string> & supportedWriteImageFormats();

	// Tells if a file is readable (it's a file, readable and it exists) and
	// sets the variables telling whether it's a zip file or a supported hdr file
	static bool isReadable(const std::string & filename, bool & isZip, bool & isHdr);

	static bool isPngSupported();
	static bool isJpgSupported();

	// Returns the number of processor available in the system
	static int numberOfProcessors();

private:
	// Cache of the number of processors
	static volatile int number_of_processors;

	// List of supported formats
	static std::vector<std::string> supported;

	// Direct format support flags
	static bool hasPng;
	static bool hasJpg;

};


#endif /* UTIL_H */
