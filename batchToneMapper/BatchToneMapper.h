// The core of the application, to decouple parameter
// parsing and those other ugly things

#if !defined(BATCH_TONE_MAPPER_H)
#define BATCH_TONE_MAPPER_H


#include <vector>
#include <string>
#include <ostream>

#include <ToneMapper.h>

class BatchToneMapper {

	friend std::ostream& operator<<(std::ostream& os, const BatchToneMapper& b);

public:
	BatchToneMapper(const std::vector<std::string> &files, bool bpp16);

	// Sets up the tonemapper with a specific gamma
	void setupToneMapper(float exposure, float gamma);

	// Sets up the tonemapper just with the exposure, using sRGB
	void setupToneMapper(float exposure);

	// To know if it has any valid files to process when
	// execute() is called.
	bool hasWork() const {
		return zipFiles.size() > 0 || hdrFiles.size() > 0;
	}

	// Main method: once everything is setup, process the files
	void execute() const;

	// Sets the offset for the filenames (it's zero by default)
	void setOffset(int newOffset) {
		offset = newOffset;
	}

	// Tries to set the format. It must be one of those exactly
	// as returned from Util::supportedWriteImageFormats().
	// If it isn't it doesn't do anything and the current format remains
	void setFormat(const std::string & newFormat);

	// Gets the default format string
	static const std::string & getDefaultFormat();

private:

	const static int LUT_SIZE = 8192;

	// General parameters
	int offset;
	std::string format;

	// Tone mapper
	pcg::ToneMapper toneMapper;

	// Number of tokens in the pipeline
	int tokens;

	// Whether to use 16 bpp in the LDR files or the default 8
	const bool useBpp16;

	// Lists of files to process
	vector<string> zipFiles;
	vector<string> hdrFiles;

	// Cache the default format
	static std::string defaultFormat;


	// Utility method to separate the elements from a raw file list into a
	// list of zip files and other of HDR files. The two new lists contains copies
	// of the strings. Before adding them to the list this method checks that
	// the files actually exists and are readable.
	void classifyFiles(const std::vector<std::string> & files);

	// Individual pipelines
	void executeZip() const;
	void executeHdr() const;

};


#endif /* BATCH_TONE_MAPPER_H */
